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

#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/fb.h>
#include "lima-memspeed.h"
#include "memspeed_fb.h"

void *fb_blank_thread(void *data)
{
	int fd, ret;

	fd = open("/dev/fb0", O_RDWR);
	assert(fd != -1);

	while (1) {
		ret = ioctl(fd, FBIOBLANK, FB_BLANK_NORMAL);
		assert(!ret);
		sleep(1);
	}

	close(fd);
	return 0;
}

void *fb_scanout_thread(void *data)
{
	workload_t *w = (workload_t *)data;
	int fd, ret, i = 0;
	struct fb_var_screeninfo var;
	double refresh_rate, htotal, vtotal;
	double framebuffer_size;
	double start_time;

	fd = open("/dev/fb0", O_RDWR);
	assert(fd != -1);

	ret = ioctl(fd, FBIOGET_VSCREENINFO, &var);
	assert(!ret);

	assert(var.vmode == FB_VMODE_NONINTERLACED);

	vtotal = var.left_margin + var.xres + var.right_margin + var.hsync_len;
	htotal = var.upper_margin + var.yres + var.lower_margin + var.vsync_len;
	refresh_rate = 1e12 / var.pixclock / htotal / vtotal;
	framebuffer_size = var.xres * var.yres * (var.bits_per_pixel / 8);

	printf("Framebuffer refresh rate: %.1f Hz\n", refresh_rate);

	/* unblank the screen right from the start */
	ret = ioctl(fd, FBIOBLANK, FB_BLANK_UNBLANK);
	assert(!ret);

	start_time = gettime();

	/* Just wake up periodically and update the data counter */
	while (1) {
		/* Kick unblank at regular intervals */
		if (i++ % 600 == 0) {
			ret = ioctl(fd, FBIOBLANK, FB_BLANK_UNBLANK);
			assert(!ret);
		}

		/* Sleep a bit (does not really matter how much) */
		usleep(1000000 / 50);

		pthread_mutex_lock(&bandwidth_counters_mutex);
		w->bytes_counter = framebuffer_size * refresh_rate *
				   (gettime() - start_time);
		pthread_mutex_unlock(&bandwidth_counters_mutex);
	}

	close(fd);
	return 0;
}
