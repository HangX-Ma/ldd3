# :rocket:`ZJUER:` Linux Device Driver Development

Linux device drivers and kernel related coding will be record in this repository.

## Prerequisites

- Ubuntu20.04, x86

### Setting up the host machine

On the host machine, you need to install a few packages, as follows:

```bash
sudo apt update
sudo apt install gawk wget git diffstat unzip \
        texinfo gcc-multilib build-essential chrpath socat \
        libsdl1.2-dev xterm ncurses-dev lzop libelf-dev make
```

### Toolchains

You can install the native compilation and cross compilation toolchains both.

```bash
# native
sudo apt install gcc binutils
# cross, 32-bit ARM machine
sudo apt install gcc-arm-linux-gnueabihf binutils-arm-linux-gnueabihf
# cross, 64-bit ARM machine
sudo apt install make gcc-aarch64-linux-gnu binutils-aarch64-linux-gnu
```

### Linux kernel headers

You can download the Linux kernel 5.10 source code or base on your host machine to prepare the pre-built kernel files. With a native kernel module build, the easiest way is to install the prebuilt kernel headers and to use their directory path as the kernel directory in the makefile.

```bash
sudo apt update
sudo apt install linux-headers-$(uname -r)
```

Pre-built kernel modules can be found in `/lib/modules/<kernel-version>/build` directory. `/usr/src/linux-headers-<kernel-version>` contains all header files.
