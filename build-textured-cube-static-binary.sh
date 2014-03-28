#!/bin/sh

if [ x$CC == "x" ]
then
    echo Note: using 'gcc' as a compiler. But it is possible to do something
    echo like \'export CC=foobar\' to override this.
    echo
    export CC=gcc
fi

$CC -DHAVE_NO_LIBMALI_BLOB \
    -o textured-cube-static \
    -s -static -Os \
    -Iinclude -Ilimare/lib -Ilimare/tests/common \
    limare/lib/gp.c limare/lib/limare.c limare/lib/bmp.c limare/lib/program.c \
    limare/lib/plb.c limare/lib/dump.c limare/lib/hfloat.c limare/lib/render_state.c \
    limare/lib/pp.c limare/lib/fb.c limare/lib/texture.c limare/lib/jobs.c \
    limare/lib/symbols.c limare/tests/cube_textured_spinning/limare.c \
    limare/tests/common/companion_texture_flat.c limare/tests/common/cube_mesh.c \
    limare/tests/common/esTransform.c \
    -pthread -lm -lrt || exit 1

echo
echo Now you have the \'textured-cube-static\' program, which uses the open source
echo lima driver to show a spinning cube demo. It expects to find and load
echo \'shader_f.mbs\' file with binary vertex shader for this demo. The fragment
echo shader is already provided by the open source shader compiler.
echo See \'limare/tests/cube_textured_spinning/limare.c\' source for more details
echo about the needed shaders.
echo
echo Compilation succeeded.
