#!/bin/sh

if [ x$CC == "x" ]
then
    echo Note: using 'gcc' as a compiler. But it is possible to do something
    echo like \'export CC=foobar\' to override this.
    echo
    export CC=gcc
fi

$CC -DHAVE_NO_LIBMALI_BLOB -DMEMTESTER_MODE \
    -DMESA_EGL_NO_X11_HEADERS \
    -o lima-memtester-static \
    -s -static -Os \
    -Iinclude -Ilimare/lib -Ilimare/tests/common \
    main.c \
    memtester-4.3.0/memtester.c memtester-4.3.0/tests.c \
    limare/lib/gp.c limare/lib/limare.c limare/lib/bmp.c limare/lib/program.c \
    limare/lib/plb.c limare/lib/dump.c limare/lib/hfloat.c limare/lib/render_state.c \
    limare/lib/pp.c limare/lib/fb.c limare/lib/texture.c limare/lib/jobs.c \
    limare/lib/symbols.c limare/tests/cube_textured_spinning/limare.c \
    limare/tests/common/companion_texture_flat.c limare/tests/common/cube_mesh.c \
    limare/tests/common/esTransform.c \
    -pthread -lm -lrt || exit 1

echo
echo Compilation succeeded.
