/*
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
#include <stdio.h>
#include <unistd.h>

#include <GLES2/gl2.h>

#include "limare.h"
#include "formats.h"

#include "esUtil.h"
#include "cube_mesh.h"
#include "companion.h"

int
main(int argc, char *argv[])
{
	struct limare_state *state;
	int ret;

#ifndef HAVE_NO_LIBMALI_BLOB
	const char *vertex_shader_source =
		"uniform mat4 modelviewprojectionMatrix;\n"
		"\n"
		"attribute vec4 in_position;  \n"
		"attribute vec2 in_coord;     \n"
		"                             \n"
		"varying vec2 coord;          \n"
		"                             \n"
		"void main()                  \n"
		"{                            \n"
		"    gl_Position = modelviewprojectionMatrix * in_position;\n"
		"    coord = in_coord;        \n"
		"}                            \n";
	const char *fragment_shader_source =
		"precision mediump float;     \n"
		"                             \n"
		"varying vec2 coord;          \n"
		"                             \n"
		"uniform sampler2D in_texture; \n"
		"                             \n"
		"void main()                  \n"
		"{                            \n"
		"    gl_FragColor = texture2D(in_texture, coord);\n"
		"}                            \n";
#endif

	state = limare_init();
	if (!state)
		return -1;

	//limare_buffer_clear(state);

	ret = limare_state_setup(state, 0, 0, 0xFF505050);
	if (ret)
		return ret;

	int width, height;
	limare_buffer_size(state, &width, &height);
	float aspect = (float) height / width;

	limare_enable(state, GL_DEPTH_TEST);
	limare_enable(state, GL_CULL_FACE);
	limare_depth_mask(state, 1);

	int program = limare_program_new(state);
#ifndef HAVE_NO_LIBMALI_BLOB
	vertex_shader_attach(state, program, vertex_shader_source);
	fragment_shader_attach(state, program, fragment_shader_source);
#else
	vertex_shader_attach_mbs_file(state, program, "shader_v.mbs");
	fragment_shader_attach_mbs_file(state, program, "shader_f.mbs");
#endif

	limare_link(state);

	limare_attribute_pointer(state, "in_position", LIMARE_ATTRIB_FLOAT,
				 3, 0, CUBE_VERTEX_COUNT, cube_vertices);
	limare_attribute_pointer(state, "in_coord", LIMARE_ATTRIB_FLOAT,
				 2, 0, CUBE_VERTEX_COUNT,
				 cube_texture_coordinates);

	int texture = limare_texture_upload(state, companion_texture_flat,
					    COMPANION_TEXTURE_WIDTH,
					    COMPANION_TEXTURE_HEIGHT,
					    COMPANION_TEXTURE_FORMAT, 0);
	limare_texture_attach(state, "in_texture", texture);

	int i = 0;

	while (1) {
		i++;
		if (i == 0xFFFFFFF)
			i = 0;

		float angle = 0.5 * i;

		ESMatrix modelview;
		esMatrixLoadIdentity(&modelview);
		esTranslate(&modelview, 0.0, 0.0, -4.0);
		esRotate(&modelview, angle * 0.97, 1.0, 0.0, 0.0);
		esRotate(&modelview, angle * 1.13, 0.0, 1.0, 0.0);
		esRotate(&modelview, angle * 0.73, 0.0, 0.0, 1.0);

		ESMatrix projection;
		esMatrixLoadIdentity(&projection);
		esFrustum(&projection, -1.0, +1.0, -1.0 * aspect, +1.0 * aspect,
			  1.0, 10.0);

		ESMatrix modelviewprojection;
		esMatrixLoadIdentity(&modelviewprojection);
		esMatrixMultiply(&modelviewprojection, &modelview, &projection);

		limare_uniform_attach(state, "modelviewprojectionMatrix", 16,
				      &modelviewprojection.m[0][0]);

		limare_frame_new(state);

		ret = limare_draw_elements(state, GL_TRIANGLES,
					   CUBE_INDEX_COUNT, &cube_indices,
					   GL_UNSIGNED_BYTE);
		if (ret)
			return ret;

		ret = limare_frame_flush(state);
		if (ret)
			return ret;

		limare_buffer_swap(state);
	}

	limare_finish(state);

	return 0;
}
