/*
 * Copyright (c) 2014 Siarhei Siamashka
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <assert.h>
#include <math.h>
#include <pthread.h>

#include "limare.h"
#include "formats.h"

#include "lima-memspeed.h"
#include "arm-neon.h"
#include "memspeed_gpu.h"
#include "memspeed_fb.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a)  (sizeof((a)) / sizeof((a)[0]))
#endif

double gettime(void)
{
	struct timespec t;
	int clock_gettime_result = clock_gettime(CLOCK_MONOTONIC, &t);
	assert(clock_gettime_result == 0);
	return t.tv_sec + 0.000000001 * t.tv_nsec;
}

pthread_mutex_t bandwidth_counters_mutex = PTHREAD_MUTEX_INITIALIZER;

/******************************************************************************/

#define BUFFER_SIZE (32 * 1024 * 1024)

static void *cpu_thread(void *data)
{
	workload_t *w = (workload_t *)data;
	void (*f)(int64_t *, int64_t *, int) = w->extra_data;
	int64_t *buffer;
	int size_multiplier = w->size_multiplier;
	if (!size_multiplier)
		size_multiplier = 1;

	if (posix_memalign((void **)&buffer, 4096, BUFFER_SIZE) != 0) {
		assert(0);
	}
	memset(buffer, 0xCC, BUFFER_SIZE);

	while (1) {
		f(buffer, buffer, BUFFER_SIZE);

		pthread_mutex_lock(&bandwidth_counters_mutex);
		w->bytes_counter += BUFFER_SIZE * size_multiplier;
		pthread_mutex_unlock(&bandwidth_counters_mutex);
	}

	free(buffer);

	return 0;
}

static workload_t workloads_list[] = {
	{
		.name = "fb_blank",
		.description = "blank the screen in order not to drain memory bandwidth",
		.thread_func = fb_blank_thread,
	},
	{
		.name = "fb_scanout",
		.description = "take the framebuffer scanout bandwidth into account",
		.thread_func = fb_scanout_thread,
	},
	{
		.name = "gpu_write",
		.description = "use the lima driver to solid fill the screen",
		.thread_func = gpu_write_thread,
	},
	{
		.name = "gpu_copy",
		.description = "use the lima driver to copy a texture to the screen",
		.thread_func = gpu_copy_thread,
	},
#ifdef __ARM__
	{
		.name = "neon_write",
		.description = "use ARM NEON to fill a memory buffer",
		.thread_func = cpu_thread,
		.extra_data = aligned_block_fill_neon,
	},
	{
		.name = "neon_write_backwards",
		.description = "use ARM NEON to fill a memory buffer",
		.thread_func = cpu_thread,
		.extra_data = aligned_block_fill_backwards_neon,
	},
	{
		.name = "neon_read_pf32",
		.description = "use ARM NEON to read from a memory buffer",
		.thread_func = cpu_thread,
		.extra_data = aligned_block_read_pf32_neon,
	},
	{
		.name = "neon_read_pf64",
		.description = "use ARM NEON to read from a memory buffer",
		.thread_func = cpu_thread,
		.extra_data = aligned_block_read_pf64_neon,
	},
	{
		.name = "neon_copy_pf64",
		.description = "use ARM NEON to copy a memory buffer",
		.thread_func = cpu_thread,
		.extra_data = aligned_block_copy_pf64_neon,
		.size_multiplier = 2,
	},
#endif
};

static void show_help_and_exit(void)
{
	int j;
	printf("Usage: lima-memspeed [workload1] [workload2] ... [workloadN]\n\n");

	printf("Where the 'workload' arguments are the identifiers of different\n");
	printf("memory bandwidth consuming workloads. Each workload is run in its\n");
	printf("own thread.\n\n");
	
	printf("The list of available workload identifiers:\n");

	for (j = 0; j < ARRAY_SIZE(workloads_list); j++) {
		if (workloads_list[j].description)
			printf("\t%-30s (%s)\n", workloads_list[j].name,
						 workloads_list[j].description);
		else
			printf("\t%s\n", workloads_list[j].name);
	}
	exit(1);
}

int main(int argc, char *argv[])
{
	int i, j, number_of_workloads = 0;
	workload_t *workloads;
	double t1, t2, bytes1, bytes2;
	double s1, s2;
	int n;
	
	if (argc < 2)
		show_help_and_exit();

	workloads = calloc(argc - 1, sizeof(workload_t));
	assert(workloads);

	/* Prepare the workloads array */
	for (i = 1; i < argc; i++) {
		int workload_found = 0;
		for (j = 0; j < ARRAY_SIZE(workloads_list); j++) {
			if (strcmp(argv[i], workloads_list[j].name) == 0) {
				workloads[number_of_workloads++] = workloads_list[j];
				workload_found = 1;
			}
		}
		if (!workload_found)
			show_help_and_exit();
	}

	/* Start the workloads threads */
	for (i = 0; i < number_of_workloads; i++) {
		printf("Starting '%s' thread\n", workloads[i].name);
		pthread_create(&workloads[i].thread_id, NULL, workloads[i].thread_func, &workloads[i]);
	}

	/* Warm-up */
	sleep(1);

	s1 = s2 = 0;
	n = 0;

	/* Do the bandwidth measurements (infinite loop) */
	while (1) {
		/* Save time and the bandwidth counters */
		pthread_mutex_lock(&bandwidth_counters_mutex);
		t1 = gettime();
		bytes1 = 0;
		for (i = 0; i < number_of_workloads; i++) {
			bytes1 += workloads[i].bytes_counter;
		}
		pthread_mutex_unlock(&bandwidth_counters_mutex);

		printf(".");
		fflush(stdout);
		sleep(2);

		pthread_mutex_lock(&bandwidth_counters_mutex);
		t2 = gettime();
		bytes2 = 0;
		for (i = 0; i < number_of_workloads; i++) {
			bytes2 += workloads[i].bytes_counter;
		}
		pthread_mutex_unlock(&bandwidth_counters_mutex);

		double bw = (bytes2 - bytes1) / (t2 - t1) / 1000000.;

		n++;
		s1 += bw;
		s2 += bw * bw;
		
		if (n >= 3) {
			double stddev = sqrt((n * s2 - s1 * s1) / (n * (n - 1)));
			double sem = stddev / sqrt(n);

			if (sem < (s1 / n) * 0.002)
				break;
		}

		if (n >= 15)
			break;
	}

	printf("\n");
	printf("Total combined memory bandwidth: %.1f MB/s\n", (s1 / n));

	return 0;
}
