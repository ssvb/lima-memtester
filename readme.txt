This is a memory reliability testing tool, implemented by combining
http://limadriver.org/ and http://pyropus.ca/software/memtester/

Licensed under the terms of the GNU General Public License version 2 (only)
because the license of memtester applies to this work as a whole.

Compilation instructions:

    cmake .
    make -j2

Or alternatively, just download a pre-built static binary from:

    https://github.com/ssvb/lima-memtester/releases

Now you should have the lima-memtester static binary. Running it only
requires any Linux kernel with the mali400 kernel module version r3p2 or
older and a framebuffer driver (for example, the sunxi-3.4 kernel using
the default configuration satisfies these requirements). This test does
not depend on anything in the userland and should work with any Linux
distribution. It can be run with root privileges as:

    ./lima-memtester 100M

The usage is exactly the same as for the original memtester. The only
difference is that a spinning textured cube demo is simultaneusly
running in a separate thread. Earlier experiments with overclocking RAM
on Allwinner A10 and Allwinner A20 hardware have shown that Mali400
usually starts misbehaving first. Exposing faults, which are very
difficult to reproduce on CPU-only workloads.

If the hardware is working fine, then the spinning cube animation on
a gray background will be running non-stop. For better confidence, it
is a good idea to let it run for at least a few hours.

If the hardware is very unstable, then the system may deadlock instantly
even before showing anything on the screen. If the hardware is moderately
unstable, then the cube animation may freeze after running for a while.
If memory corruption problems are detected, then the animation may switch
to a pulsing red background (instead of the default gray), together with
something like the following error messages in the console:

memtester version 4.3.0 (32-bit)
Copyright (C) 2001-2012 Charles Cazabon.
Licensed under the GNU General Public License version 2 (only).

pagesize is 4096
pagesizemask is 0xfffff000
want 1400MB (1468006400 bytes)
got  1400MB (1468006400 bytes), trying mlock ...locked.
Loop 1:
  Stuck Address       : ok
  Random Value        : ok
  Compare XOR         : ok
  Compare SUB         : ok
  Compare MUL         : ok
  Compare DIV         : ok
  Compare OR          : ok
  Compare AND         : ok
  Sequential Increment: ok
  Solid Bits          : testing  23FAILURE: 0xffffffff != 0xffffff00 at offset 0x099e03bc.
FAILURE: 0x00000000 != 0x000000ff at offset 0x099e03c0.
FAILURE: 0xffffffff != 0xffffff00 at offset 0x099e03c4.
FAILURE: 0x00000000 != 0x000000ff at offset 0x099e03c8.

The reliability problems can be usually resolved by downclocking DRAM
and/or adjusting voltages until the lima-memtester program starts working
fine. On Allwinner hardware, DRAM is configured in the u-boot bootloader,
so that's the right place to apply the changes.

It is also a very good idea to ensure at least some safety headroom. For
example, make sure that the test does not detect problems at some DRAM clock
frequency, and then reduce it at least by one step (lower DRAM clock speed
means better reliability). And do a similar thing with the voltage (higher
voltage usually means better reliability).
