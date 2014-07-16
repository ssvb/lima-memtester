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
#include <inttypes.h>
#include <pthread.h>

#include "limare.h"
#include "fb.h"
#include "formats.h"

#include "esUtil.h"

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

/******************************************************************************/

#define COPYTEST_VERTEX_COUNT   4
#define COPYTEST_TRIANGLE_COUNT 2
#define COPYTEST_INDEX_COUNT    (COPYTEST_TRIANGLE_COUNT * 3)

static float copytest_vertices[COPYTEST_VERTEX_COUNT][3] = {
	{-1.0, -1.0, +1.0},
	{+1.0, -1.0, +1.0},
	{-1.0, +1.0, +1.0},
	{+1.0, +1.0, +1.0},
};

static float copytest_texture_coordinates[COPYTEST_VERTEX_COUNT][2] = {
	/* front */
	{0.0, 1.0},
	{1.0, 1.0},
	{0.0, 0.0},
	{1.0, 0.0},
};

static unsigned char copytest_indices[COPYTEST_TRIANGLE_COUNT][3] = {
	{0,  1,  2},
	{3,  2,  1},
};


void *gpu_copy_thread(void *data)
{
	workload_t *w = (workload_t *)data;
	struct limare_state *state;
	int ret, width, height, x, y;

	#include "shader_v.h"
	#include "shader_f.h"

	load_mali_kernel_module();

	state = limare_init();
	assert(state);

	ret = limare_state_setup(state, 0, 0, 0xFF505050);
	assert(state);

	limare_buffer_size(state, &width, &height);

	int program = limare_program_new(state);
	vertex_shader_attach_mbs_stream(state, program, vertex_shader_binary,
						sizeof(vertex_shader_binary));
	fragment_shader_attach_mbs_stream(state, program, fragment_shader_binary,
						sizeof(fragment_shader_binary));
	limare_link(state);

	limare_attribute_pointer(state, "in_position", LIMARE_ATTRIB_FLOAT,
				 3, 0, COPYTEST_VERTEX_COUNT, copytest_vertices);
	limare_attribute_pointer(state, "in_coord", LIMARE_ATTRIB_FLOAT,
				 2, 0, COPYTEST_VERTEX_COUNT,
				 copytest_texture_coordinates);

	/* Generate a texture */
	uint32_t *checkerboard_texture = malloc(width * height * sizeof(uint32_t));
	assert(checkerboard_texture);
	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			uint32_t color;
			if (((x * 8 / width) % 2 == 0) ^ ((y * 8 / height) % 2 == 0))
				color = (x % 2) ? 0xFFFFFFFF : 0;
			else
				color = (y % 2) ? 0xFFFFFFFF : 0;
			checkerboard_texture[y * width + x] = color;
		}
	}

	int texture = limare_texture_upload(state, checkerboard_texture,
					    width, height,
					    LIMA_TEXEL_FORMAT_RGBA_8888, 0);
	limare_texture_attach(state, "in_texture", texture);

	ESMatrix modelviewprojection;
	esMatrixLoadIdentity(&modelviewprojection);
	esTranslate(&modelviewprojection, 0.0, 0.0, -0.5);

	while (1) {
		limare_uniform_attach(state, "modelviewprojectionMatrix", 16,
				      &modelviewprojection.m[0][0]);
		limare_frame_new(state);
		ret = limare_draw_elements(state, GL_TRIANGLES,
					   COPYTEST_INDEX_COUNT,
					   &copytest_indices,
					   GL_UNSIGNED_BYTE);
		assert(!ret);
		ret = limare_frame_flush(state);
		assert(!ret);
		limare_buffer_swap(state);

		pthread_mutex_lock(&bandwidth_counters_mutex);
		w->bytes_counter += width * height * (state->fb->bpp / 8) +
				    width * height * 4;
		pthread_mutex_unlock(&bandwidth_counters_mutex);
	}

	limare_finish(state);
	free(checkerboard_texture);

	return 0;
}
