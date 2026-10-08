# [library](./library/)
Header files containing structure definitions relevant to understanding SROP on x86-64 Linux.

* [stack_t.h](./library/stack_t.h)
* [ucontext_t.h](./library/ucontext.h)

# [QEMUKernel](./QEMUKernel/)
Files for setting up a QEMU-based Linux kernel debugging environment and analyzing the kernel's `rt_sigreturn` implementation.

* [Dockerfile](./QEMUKernel/Dockerfile)
* [signal_64.c](./QEMUKernel/signal_64.c)

# [target]
Target source code and statically linked executable used for analyzing SROP.

* [sigReturn](./target/sigReturn)
* [sigReturn.c](./target/sigReturn.c)
