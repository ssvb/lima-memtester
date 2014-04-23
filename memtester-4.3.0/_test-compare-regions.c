/*
 * Very simple but very effective user-space memory tester.
 * Originally by Simon Kirby <sim@stormix.com> <sim@neato.org>
 * Version 2 by Charles Cazabon <charlesc-memtester@pyropus.ca>
 * Version 3 not publicly released.
 * Version 4 rewrite:
 * Copyright (C) 2004-2012 Charles Cazabon <charlesc-memtester@pyropus.ca>
 * Licensed under the terms of the GNU General Public License version 2 (only).
 * See the file COPYING for details.
 *
 * This file contains a simple test for the 'compare_regions_helper_*'
 * implementations.
 *
 */
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "types.h"

#define BUFSIZE (256 * 1024)

int use_phys = 0;
int memtester_early_exit = 0;
off_t physaddrbase = 0;

size_t compare_regions_helper(ulv *bufa, ulv *bufb, size_t count, ul *va, ul *vb);

#ifdef __arm__

typedef struct compare_regions_result {
    ul iteration[8];
    ul value1[8];
    ul value2[8];
} compare_regions_result;

void write_regions_neon(ulv *buf1, ulv *buf2, ul val1, ul val2, ul bufsize);
void compare_regions_neon(ulv *buf1, ulv *buf2, ul bufsize,
                          compare_regions_result *res);

#endif

size_t compare_regions_helper_ref(ulv *bufa, ulv *bufb, size_t count,
                                  ul *va, ul *vb) {
    size_t i, result = (size_t)(-1);
    ulv *p1 = bufa;
    ulv *p2 = bufb;

    for (i = 0; i < count; i++, p1++, p2++) {
        ul v1 = *p1, v2 = *p2;
        if (v1 != v2) {
            *va = v1;
            *vb = v2;
            result = i;
        }
    }
    return result;
}

int main()
{
    int repeat;
    int i;
    ul *buf1 = malloc(BUFSIZE * sizeof(ulv));
    ul *buf2 = malloc(BUFSIZE * sizeof(ulv));

    for (repeat = 0; repeat < 10000; repeat++)
    {
        ul offs1, offs2, v1a, v1b, v2a, v2b;
        for (i = 0; i < BUFSIZE; i++)
        {
            buf1[i] = buf2[i] = i ^ 0xCCCCCCCC;
        }
        uint32_t rand_index1 = rand() % BUFSIZE;
        buf2[rand_index1] = rand();

        uint32_t rand_index2 = rand() % BUFSIZE;
        buf2[rand_index2] = rand();

        uint32_t rand_index3 = rand() % BUFSIZE;
        buf2[rand_index3] = rand();

        offs1 = compare_regions_helper(buf1, buf2, BUFSIZE, &v1a, &v1b);
        offs2 = compare_regions_helper_ref(buf1, buf2, BUFSIZE, &v2a, &v2b);

        if (offs1 != offs2)
        {
            printf("rand_index1=%08X\n", rand_index1);
            printf("rand_index2=%08X\n", rand_index2);
            printf("rand_index3=%08X\n", rand_index3);
            printf("(%08lX: %08lX != %08lX) vs. (%08lX: %08lX != %08lX)\n",
                   offs1, v1a, v1b, offs2, v2a, v2b);
            break;
        }
    }

    free(buf1);
    free(buf2);
    return 0;
}
