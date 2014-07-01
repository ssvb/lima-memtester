cmake_minimum_required(VERSION 2.8)

project(lima-memtester ASM C)
find_package(Threads REQUIRED)
set(CMAKE_C_FLAGS "-s -static -Os")

add_definitions(-DHAVE_NO_LIBMALI_BLOB -DMESA_EGL_NO_X11_HEADERS)
include_directories(limadriver/include limadriver/limare/lib
                    limadriver/limare/tests/common)

add_executable(lima-textured-cube
               limadriver/limare/lib/gp.c limadriver/limare/lib/limare.c
               limadriver/limare/lib/bmp.c limadriver/limare/lib/program.c
               limadriver/limare/lib/plb.c limadriver/limare/lib/dump.c
               limadriver/limare/lib/hfloat.c limadriver/limare/lib/render_state.c
               limadriver/limare/lib/pp.c limadriver/limare/lib/fb.c
               limadriver/limare/lib/texture.c limadriver/limare/lib/jobs.c
               limadriver/limare/lib/symbols.c
               limadriver/limare/tests/cube_textured_spinning/limare.c
               limadriver/limare/tests/common/companion_texture_flat.c
               limadriver/limare/tests/common/cube_mesh.c
               limadriver/limare/tests/common/esTransform.c)

target_link_libraries(lima-textured-cube m rt ${CMAKE_THREAD_LIBS_INIT})

add_executable(lima-memtester
               main.c
               memtester-4.3.0/memtester.c memtester-4.3.0/tests.c
               memtester-4.3.0/arm-asm-helpers.S
               limadriver/limare/lib/gp.c limadriver/limare/lib/limare.c
               limadriver/limare/lib/bmp.c limadriver/limare/lib/program.c
               limadriver/limare/lib/plb.c limadriver/limare/lib/dump.c
               limadriver/limare/lib/hfloat.c limadriver/limare/lib/render_state.c
               limadriver/limare/lib/pp.c limadriver/limare/lib/fb.c
               limadriver/limare/lib/texture.c limadriver/limare/lib/jobs.c
               limadriver/limare/lib/symbols.c
               limadriver/limare/tests/cube_textured_spinning/limare.c
               limadriver/limare/tests/common/companion_texture_flat.c
               limadriver/limare/tests/common/cube_mesh.c
               limadriver/limare/tests/common/esTransform.c)

set_target_properties(lima-memtester PROPERTIES COMPILE_DEFINITIONS "MEMTESTER_MODE")
target_link_libraries(lima-memtester m rt ${CMAKE_THREAD_LIBS_INIT})

install_programs(/bin FILES lima-textured-cube lima-memtester)
