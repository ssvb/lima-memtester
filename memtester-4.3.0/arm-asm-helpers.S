/*
 * Copyright © 2014 Siarhei Siamashka <siarhei.siamashka@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#if defined(__linux__) && defined(__ELF__)
.section .note.GNU-stack,"",%progbits
#endif

#ifdef __arm__

        .text
        .syntax unified
        .fpu neon
        .arch armv7a
        .object_arch armv4
        .arm
        .altmacro
        .p2align 2

.macro asm_function function_name
        .global \function_name
#ifdef __ELF__
        .hidden \function_name
        .type \function_name, %function
#endif
.func \function_name
\function_name:
.endm

/*
 * void compare_regions_helper_neon(uint32_t *buf1, uint32_t *buf2,
 *                                  uint32_t count,
 *                                  compare_regions_helper_result *res)
 *
 * typedef struct compare_regions_helper_result {
 *     uint32_t failed_index[8];
 *     uint32_t failed_value1[8];
 *     uint32_t failed_value2[8];
 * } compare_regions_helper_result;
 *
 * This function compares two arrays composed of 32-bit elements and
 * fills in the 'compare_regions_result' structure. In the case if
 * some element of the 'failed_index' array is not equal to 0xFFFFFFFF,
 * this means that the difference has been spotted at this
 * particular array index. The corresponding mismatched values read
 * from the buffers are stored in the 'failed_value1' and 'failed_value2'
 * arrays. We additionally store these values just in case if the mismatch
 * has been caused by a sporadic DRAM read failure (we are not going to
 * easily retrieve the same failed values again).
 *
 * This function scans memory in the forward direction and allows
 * to get the index of the *last* detected failure.
 */

.balign 16
compare_regions_neon_data:
    .long 0, 1, 2, 3

asm_function compare_regions_helper_neon
        /* r0 - buf1           */
        /* r1 - buf2           */
        /* r2 - count          */
        /* r3 - results buffer */

        /* Round down the size to a multiple of 64 */
        bics            r2, r2, #(16 - 1)
        bxeq            lr

        vpush           {d8-d15}

        /* Problematic value 1 */
        vmov.u8         q8,  #0xFF
        vmov.u8         q9,  #0xFF
        /* Problematic value 2 */
        vmov.u8         q10, #0xFF
        vmov.u8         q11, #0xFF
        /* Problematic indexes */
        vmov.u8         q12, #0xFF
        vmov.u8         q13, #0xFF
        /* Indexes tracking */
        vmov.u32        q14, #8
        adr             ip, compare_regions_neon_data
        vld1.32         {q15}, [ip]

0:      /* Main loop */
.rept 2
        vld1.32         {q0, q1}, [r0]!
        vld1.32         {q2, q3}, [r1]!
        vceq.u32        q4,  q2,  q0
        vceq.u32        q5,  q3,  q1
        vbif.32         q8,  q0,  q4
        vbif.32         q10, q2,  q4
        vbif.32         q12, q15, q4
        vbif.32         q9,  q1,  q5
        vbif.32         q11, q3,  q5
        vbif.32         q13, q15, q5
        vadd.u32        q15, q15, q14
.endr
        pld             [r0, #512]
        pld             [r1, #512]
        subs            r2, r2, #16
        bne             0b

        /* Forcefully fetch the output structure into L1 cache */
        ldrb            r2, [r3, #0]
        ldrb            r2, [r3, #32]
        ldrb            r2, [r3, #64]
        ldrb            r2, [r3, #(96 - 1)]

        /* Store results to the output structure */
        vst1.32         {q12}, [r3]!
        vmov.u32        q14, #4
        vqadd.u32       q13, q13, q14
        vst1.32         {q13}, [r3]!
        vst1.32         {q8,  q9 }, [r3]!
        vst1.32         {q10, q11}, [r3]!

        vpop            {d8-d15}
        bx              lr
.endfunc

#endif
