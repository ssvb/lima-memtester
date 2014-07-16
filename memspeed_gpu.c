/*
 * Copyright (c) 2014 Siarhei Siamashka
 *
 * Based on the textured cube demo from the lima driver
 *
 * Copyright (c) 2011-2013 Luc Verhaegen <libv@skynet.be>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <assert.h>
#include <pthread.h>

#include "limare.h"
#include "fb.h"
#include "formats.h"

#include "lima-memspeed.h"
#include "memspeed_gpu.h"
#include "load_mali_kernel_module.h"

void *gpu_write_thread(void *data)
{
	workload_t *w = (workload_t *)data;
	struct limare_state *state;
	double t1, t2;
	int i = 0;
	int ret;
	int width, height;

	load_mali_kernel_module();

	state = limare_init();
	assert(state);

	ret = limare_state_setup(state, 0, 0, 0xFF505050);
	assert(ret == 0);

	limare_buffer_size(state, &width, &height);

	while (1) {
		state->clear_color = 0xFF000040 + abs((i++ * 1) %
				((255 - 0x40) * 2) - (255 - 0x40));
		limare_frame_new(state);
		ret = limare_frame_flush(state);
		assert(!ret);
		limare_buffer_swap(state);

		pthread_mutex_lock(&bandwidth_counters_mutex);
		w->bytes_counter += width * height * (state->fb->bpp / 8);
		pthread_mutex_unlock(&bandwidth_counters_mutex);
	}

	limare_finish(state);
	return 0;
}
