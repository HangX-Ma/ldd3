# QEMU Development

Linux device driver based on LDD3(Linux Device Drivers, Third Edition), taking reference to [Linux-Device-Driver](https://github.com/d0u9/Linux-Device-Driver) repository, which develops the examples complied against Linux Kernel 5.10.

LDD3, which is authored by Jonathan Corbet, Alessandro Rubini, and Greg Kroah-Hartman, is available online, and you may request a copy of the PDF from <https://lwn.net/Kernel/LDD3/>.

## 1. Record

```sh
qemu_rootfs            # Directory which is shared between host and QEMU via NFS protocol.
├── bin                # Symbolic links of all executable binary files.
├── initramfs          # Directory containing files to build initramfs image.
├── initramfs.cpio.gz
├── kernel
├── LDD-Development
├── qemu               # The QEMU source files, git cloned from official repo
└── qemu_run.sh        # QEMU launching script
```

### 1.1 Git Patch

QEMU setting is quite frustrating though the repository author provide the guidance. `QEMU_LDD.patch` will cause conflicts, generating the `.rej` files.

- Move the non-conflicting code into the patch while preserving the conflicting parts.

    ```bash
    git apply --reject  xxxx.patch
    ```

- Remove the files with suffix `.rej` after resolving the conflicts. Execute `git add .` to add changes to the staging area.
- Perform `git am --resolved`.

### 1.2 Network Configuration

NFS setting encounters plenty of troubles. I have modified the launch file as follow. The QEMU create the `host_machine` ip to `10.0.2.2` and its own ip to `10.0.2.15`. However, no bridge between them that allows network communication. Therefore, I create a virtual network adapter using `tunctl` command.

```bash
#!/bin/bash

qemu-5.2.0-x86_64 -enable-kvm \
        -kernel $LDD_ROOT/kernel/bzImage \
        -initrd $LDD_ROOT/initramfs.cpio.gz \
        -append 'console=ttyS0' \
        -nographic \
        -device e1000,netdev=dev0,mac='00:00:00:01:00:01' \
        # Choose one of the following option
        -netdev tap,ifname=tap0,id=dev0,script=no,downscript=no,vhost=no
        -netdev user,id=host_net,hostfwd=tcp::7023-:23
```

#### Virtual Network Adapter

- Install `tunctl` components.

    ```bash
    sudo apt-get install uml-utilities
    ```

- Create a device named `tap0` with specific ip address. Afterwards, you need to open up this device.

    ```bash
    sudo tunctl -t tap0 -u <user-name>
    sudo ifconfig tap0 10.0.2.2 up
    # delete this tap0 interface if it is not needed
    sudo tunctl -d tap0
    ```

I take a reference to <https://unix.stackexchange.com/questions/243382/making-dev-net-tun-available-to-qemu> and <https://www.cnblogs.com/hugetong/p/8808752.html> to create a feasible `.sh` file.
