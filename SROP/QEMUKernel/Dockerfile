FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive
ENV LANG=C.UTF-8
ENV LC_ALL=C.UTF-8


# ============================================================
# Phase 1: Basic pwn / development environment
# ============================================================

RUN apt-get update && apt-get install -y \
    build-essential \
    gcc \
    g++ \
    gcc-multilib \
    g++-multilib \
    gdb \
    gdb-multiarch \
    binutils \
    binutils-multiarch \
    libc6-dev \
    libc6-dev-i386 \
    make \
    cmake \
    git \
    curl \
    wget \
    file \
    vim \
    nano \
    tmux \
    less \
    unzip \
    xz-utils \
    ca-certificates \
    python3 \
    python3-dev \
    python3-pip \
    python3-setuptools \
    python3-wheel \
    pkg-config \
    libssl-dev \
    libffi-dev \
    strace \
    ltrace \
    patchelf \
    socat \
    netcat-openbsd \
    procps \
    ruby \
    ruby-dev \
    && rm -rf /var/lib/apt/lists/*


# ============================================================
# Phase 2: Pwn tools
#
# Ubuntu 24.04 uses PEP 668.
# Since this is a disposable analysis container, install the
# Python tools directly into the system Python environment.
#
# pwntools provides commands such as:
#
#   checksec
#   cyclic
#   pwn
#
# Do NOT install Ubuntu's standalone "checksec" package here.
# ============================================================

RUN pip install --break-system-packages \
    pwntools \
    ROPgadget \
    ropper


# ============================================================
# Ruby-based pwn tools
# ============================================================

RUN gem install \
    one_gadget \
    seccomp-tools


# ============================================================
# Pwndbg
#
# pwndbg runs on the Docker side.
#
# It is NOT installed inside the QEMU guest.
#
# For kernel debugging:
#
#   Docker GDB/pwndbg
#          |
#          | GDB Remote Protocol
#          v
#       QEMU Kernel
#
# ============================================================

WORKDIR /opt

RUN git clone --depth 1 \
    https://github.com/pwndbg/pwndbg.git

WORKDIR /opt/pwndbg

RUN ./setup.sh


# ============================================================
# Phase 3: QEMU / Linux kernel development environment
#
# Target kernel:
#
#   Linux 6.8.12
#
# ============================================================

USER root

RUN apt-get update && apt-get install -y \
    qemu-system-x86 \
    qemu-utils \
    bc \
    bison \
    flex \
    libelf-dev \
    libssl-dev \
    libncurses-dev \
    dwarves \
    cpio \
    busybox-static \
    rsync \
    kmod \
    && rm -rf /var/lib/apt/lists/*


# ============================================================
# Linux kernel 6.8.12 source
#
# Source:
#
#   /root/linux-6.8.12
#
# ============================================================

WORKDIR /root

RUN git clone \
    --depth 1 \
    --branch v6.8.12 \
    https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git \
    /root/linux-6.8.12


# ============================================================
# Kernel configuration
#
# DEBUG_INFO:
#   Generate DWARF debugging information.
#
# DEBUG_INFO_DWARF_TOOLCHAIN_DEFAULT:
#   Use the compiler/toolchain default DWARF version.
#
# GDB_SCRIPTS:
#   Enable Linux kernel GDB helper scripts.
#
# RANDOMIZE_BASE=n:
#   Disable kernel KASLR.
#
#
# ------------------------------------------------------------
# VirtIO 9P support
# ------------------------------------------------------------
#
# CONFIG_NET_9P=y
# CONFIG_NET_9P_VIRTIO=y
# CONFIG_9P_FS=y
#
# These allow us to share:
#
#       Docker                     QEMU
#
#       /root/share    <------>    /mnt/share
#
#
# These are compiled directly into the kernel (=y), rather
# than modules, because our minimal initramfs does not contain
# kernel modules.
#
# ============================================================

WORKDIR /root/linux-6.8.12

RUN make x86_64_defconfig && \
    ./scripts/config --enable DEBUG_INFO && \
    ./scripts/config --enable DEBUG_INFO_DWARF_TOOLCHAIN_DEFAULT && \
    ./scripts/config --enable GDB_SCRIPTS && \
    ./scripts/config --disable RANDOMIZE_BASE && \
    ./scripts/config --enable NET_9P && \
    ./scripts/config --enable NET_9P_VIRTIO && \
    ./scripts/config --enable 9P_FS && \
    make olddefconfig


# ============================================================
# Build Linux kernel
#
# Outputs:
#
#   /root/linux-6.8.12/vmlinux
#
#       Uncompressed ELF kernel.
#       Contains symbols/DWARF information.
#       Used by GDB.
#
#
#   /root/linux-6.8.12/arch/x86/boot/bzImage
#
#       Compressed bootable kernel image.
#       Used by QEMU.
#
# ============================================================

RUN make -j$(nproc)


# ============================================================
# Phase 4: Minimal initramfs
#
# Result:
#
#   /root/rootfs/
#   /root/rootfs.cpio.gz
#
#
# Layout:
#
#   rootfs/
#   |
#   +-- init
#   +-- bin/
#   |   +-- busybox
#   |   +-- sh -> busybox
#   |   +-- mount -> busybox
#   |   +-- ...
#   |
#   +-- dev/
#   +-- proc/
#   +-- sys/
#   +-- tmp/
#   +-- root/
#   |
#   +-- mnt/
#       +-- share/
#
# ============================================================


# ============================================================
# Docker <-> QEMU shared directory
#
#
# Docker side:
#
#       /root/share
#
#              |
#              | VirtIO 9P
#              v
#
# QEMU guest:
#
#       /mnt/share
#
#
# ------------------------------------------------------------
# IMPORTANT: Where to put target binaries
# ------------------------------------------------------------
#
# Compile the target binary on the Docker side directly into:
#
#       /root/share/
#
#
# Example:
#
#   gcc -static -g -O0 \
#       /root/test.c \
#       -o /root/share/test
#
#
# The SAME file will then be visible inside the running
# QEMU guest as:
#
#       /mnt/share/test
#
#
# Run it inside QEMU:
#
#       / # /mnt/share/test
#
#
# If test.c is modified, simply compile it again:
#
#   gcc -static -g -O0 \
#       /root/test.c \
#       -o /root/share/test
#
#
# The running QEMU guest immediately sees the new file.
#
# You DO NOT need to:
#
#   - rebuild the Linux kernel
#   - rebuild rootfs.cpio.gz
#   - restart QEMU
#
# ============================================================

WORKDIR /root

RUN mkdir -p \
    /root/rootfs/bin \
    /root/rootfs/sbin \
    /root/rootfs/etc \
    /root/rootfs/proc \
    /root/rootfs/sys \
    /root/rootfs/dev \
    /root/rootfs/tmp \
    /root/rootfs/root \
    /root/rootfs/mnt/share \
    /root/share


# ============================================================
# Install static BusyBox
#
# busybox-static is copied into the initramfs.
#
# The required commands are provided as symlinks to BusyBox.
# ============================================================

RUN cp /usr/bin/busybox /root/rootfs/bin/busybox && \
    cd /root/rootfs/bin && \
    for cmd in \
        sh \
        mount \
        mountpoint \
        setsid \
        cttyhack \
        uname \
        ps \
        ls \
        cat \
        echo \
        mkdir \
        poweroff \
        id; \
    do \
        ln -s busybox "$cmd"; \
    done


# ============================================================
# Create /init
#
# /init is the first userspace program started by the kernel.
#
# It runs as:
#
#       PID 1
#
#
# Initialization:
#
#   1. Mount /proc
#   2. Mount /sys
#   3. Mount /dev
#   4. Mount Docker /root/share -> QEMU /mnt/share
#   5. Start an interactive shell
#   6. When the shell exits, power off QEMU
#
#
# ------------------------------------------------------------
# Why we DO NOT use:
#
#       exec setsid cttyhack /bin/sh
#
# ------------------------------------------------------------
#
# "exec" would replace PID 1 (/init) with the shell:
#
#       PID 1: /init
#
#              exec
#                |
#                v
#
#       PID 1: /bin/sh
#
#
# Then:
#
#       / # exit
#
# would terminate PID 1.
#
# Linux would respond with:
#
#       Kernel panic - not syncing:
#       Attempted to kill init!
#
#
# Instead we use:
#
#       setsid cttyhack /bin/sh
#
#
# Therefore:
#
#       PID 1: /init
#                  |
#                  +---- child: /bin/sh
#
#
# When /bin/sh exits, /init continues and executes:
#
#       poweroff -f
#
# ============================================================

RUN printf '%s\n' \
    '#!/bin/sh' \
    '' \
    'mount -t proc proc /proc' \
    'mount -t sysfs sysfs /sys' \
    'mount -t devtmpfs devtmpfs /dev' \
    '' \
    'echo "[+] Mounting Docker shared directory..."' \
    'mkdir -p /mnt/share' \
    'mount -t 9p -o trans=virtio hostshare /mnt/share' \
    '' \
    'if mountpoint -q /mnt/share 2>/dev/null; then' \
    '    echo "[+] Shared directory mounted successfully"' \
    'else' \
    '    echo "[!] Shared directory mount failed"' \
    'fi' \
    '' \
    'echo "    Docker : /root/share"' \
    'echo "    QEMU   : /mnt/share"' \
    '' \
    'echo' \
    'echo "========================================"' \
    'echo " Linux 6.8.12 Kernel Debug Environment "' \
    'echo "========================================"' \
    'echo' \
    'echo "Kernel:"' \
    'uname -a' \
    'echo' \
    'echo "PID 1:"' \
    'ps' \
    'echo' \
    '' \
    'setsid cttyhack /bin/sh' \
    '' \
    'echo' \
    'echo "[+] Shell exited."' \
    'echo "[+] Powering off QEMU guest..."' \
    '' \
    'poweroff -f' \
    > /root/rootfs/init && \
    chmod +x /root/rootfs/init


# ============================================================
# Create compressed initramfs
#
#       /root/rootfs
#             |
#             | find + cpio
#             v
#        rootfs.cpio
#             |
#             | gzip
#             v
#      rootfs.cpio.gz
#
# ============================================================

RUN cd /root/rootfs && \
    find . -print0 | \
    cpio --null -ov --format=newc | \
    gzip -9 > /root/rootfs.cpio.gz


# ============================================================
# QEMU NORMAL BOOT
# ============================================================
#
# Inside the Docker container:
#
#   qemu-system-x86_64 \
#       -kernel /root/linux-6.8.12/arch/x86/boot/bzImage \
#       -initrd /root/rootfs.cpio.gz \
#       -append "console=ttyS0 nokaslr" \
#       -virtfs local,path=/root/share,mount_tag=hostshare,security_model=none,id=hostshare \
#       -nographic
#
#
# ------------------------------------------------------------
# -virtfs
# ------------------------------------------------------------
#
#   path=/root/share
#
#       Directory on the Docker side.
#
#
#   mount_tag=hostshare
#
#       9P filesystem identifier presented to the guest.
#
#
#   security_model=none
#
#       Simplifies UID/GID handling for this local debugging
#       environment.
#
#
# Guest /init performs:
#
#   mount -t 9p \
#       -o trans=virtio \
#       hostshare \
#       /mnt/share
#
#
# Therefore:
#
#       Docker                    QEMU
#
#       /root/share    <------>   /mnt/share
#
# ============================================================


# ============================================================
# Typical target binary workflow
# ============================================================
#
# Terminal #1:
#
#   Start QEMU.
#
#
# Terminal #2:
#
#   Connect to the same Docker container:
#
#       docker exec -it pwn_24.04_kernel bash
#
#
#   Compile:
#
#       gcc -static -g -O0 \
#           /root/test.c \
#           -o /root/share/test
#
#
# QEMU:
#
#       / # ls -l /mnt/share
#
#       / # /mnt/share/test
#
#
# Modify /root/test.c and compile again:
#
#       gcc -static -g -O0 \
#           /root/test.c \
#           -o /root/share/test
#
#
# Then inside the SAME QEMU instance:
#
#       / # /mnt/share/test
#
#
# QEMU does NOT need to be restarted.
# ============================================================


# ============================================================
# QEMU + GDB kernel debugging
# ============================================================
#
# Start QEMU with its GDB server enabled:
#
#   qemu-system-x86_64 \
#       -kernel /root/linux-6.8.12/arch/x86/boot/bzImage \
#       -initrd /root/rootfs.cpio.gz \
#       -append "console=ttyS0 nokaslr" \
#       -virtfs local,path=/root/share,mount_tag=hostshare,security_model=none,id=hostshare \
#       -nographic \
#       -s \
#       -S
#
#
# -s:
#
#       Start QEMU GDB server on:
#
#           TCP :1234
#
#
# -S:
#
#       Do not start the virtual CPU.
#
#       QEMU waits until GDB issues:
#
#           continue
#
#
# ------------------------------------------------------------
# Open another Windows terminal
# ------------------------------------------------------------
#
#   docker exec -it pwn_24.04_kernel bash
#
#
# Then:
#
#   cd /root/linux-6.8.12
#
#   gdb vmlinux
#
#
# pwndbg should automatically load.
#
#
# Inside GDB:
#
#   target remote :1234
#
#
# Example:
#
#   break __x64_sys_rt_sigreturn
#
#   continue
#
#
# This allows syscall/kernel code to be stepped through
# instruction-by-instruction.
#
# ============================================================


# ============================================================
# QEMU shutdown
# ============================================================
#
# Because /init remains PID 1, typing:
#
#       / # exit
#
# exits only the interactive shell.
#
# /init then executes:
#
#       poweroff -f
#
# and QEMU shuts down.
#
#
# You can also explicitly use:
#
#       / # poweroff -f
#
#
# QEMU monitor escape:
#
#       Ctrl+A
#       X
#
# Press Ctrl+A, release it, then press X.
#
# ============================================================


# ============================================================
# Useful verification commands
# ============================================================
#
# Docker:
#
#   gcc --version
#   gdb --version
#   checksec --help
#   ROPgadget --help
#   ropper --help
#   one_gadget --version
#   seccomp-tools --version
#   qemu-system-x86_64 --version
#
#
# Verify kernel configuration:
#
#   grep -E \
#       'CONFIG_(NET_9P|NET_9P_VIRTIO|9P_FS|GDB_SCRIPTS|RANDOMIZE_BASE)=' \
#       /root/linux-6.8.12/.config
#
#
# Expected important values:
#
#   CONFIG_NET_9P=y
#   CONFIG_NET_9P_VIRTIO=y
#   CONFIG_9P_FS=y
#   CONFIG_GDB_SCRIPTS=y
#
# RANDOMIZE_BASE should not be enabled.
#
# ============================================================


WORKDIR /root

CMD ["/bin/bash"]