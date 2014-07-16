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
#include <stdio.h>
#include <string.h>

static void check_kernel_cmdline(void)
{
	char buffer[1024];
	FILE *f = fopen("/proc/cmdline", "r");
	if (!f) {
		printf("Warning: can't open /proc/cmdline\n");
		return;
	}
	if (fgets(buffer, sizeof(buffer), f) &&
	    strstr(buffer, "sunxi_no_mali_mem_reserve"))
	{
		fprintf(stderr, "Please remove 'sunxi_no_mali_mem_reserve' option from\n");
		fprintf(stderr, "your kernel command line. Otherwise the mali kernel\n");
		fprintf(stderr, "driver may be non-functional and actually knock down\n");
		fprintf(stderr, "your system with some old linux-sunxi kernels.\n");
		abort();
	}
	fclose(f);
}

void load_mali_kernel_module(void)
{
	check_kernel_cmdline();

	if (system("modprobe mali >/dev/null 2>&1")) {
		fprintf(stderr, "Failed to 'modprobe mali'.\n");
		abort();
	}
}
