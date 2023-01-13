
# Note

## VSCode Configuration

If you use VSCode and have installed the `C/C++` IntelliSense plugin, you can Refer to the following configuration.

```json
{
    "configurations": [
        {
            "name": "Linux",
            "includePath": [
                "${workspaceFolder}/**",
                /* ----------- HOST DEFINITION ----------- */
                // "/usr/src/linux-headers-5.13.0-52-generic/include",
                // "/usr/src/linux-headers-5.13.0-52-generic/include/uapi",
                // "/usr/src/linux-headers-5.13.0-52-generic/include/generated",
                // "/usr/src/linux-headers-5.13.0-52-generic/arch/x86/include",
                // "/usr/src/linux-headers-5.13.0-52-generic/arch/x86/include/generated"

                /* ----------- KERNEL DEFINITION ----------- */
                "${HOME}/linux/include",
                "${HOME}/linux/include/uapi",
                "${HOME}/linux/include/generated",
                "${HOME}/linux/arch/x86/include",
                "${HOME}/linux/arch/x86/include/generated"
            ],
            "defines": [
                "__GNUC__",
                "__KERNEL__",
                "MODULE"
            ],
            "compilerPath": "/usr/bin/gcc",
            "cStandard": "gnu17",
            "cppStandard": "gnu++14",
            "intelliSenseMode": "linux-gcc-x64"
        }
    ],
    "version": 4
}
```

## LDD3 Example Files Tree

```txt
[@localhost]$ tree
.
|-- LICENSE
|-- Makefile
|-- include
|   `-- lddbus.h    示例代码中，#include "lddbus.h"。
|-- lddbus
|   |-- Makefile
|   `-- lddbus.c    CHAPTER_14.4:虚拟总线实现
|-- misc-modules
|   |-- Makefile
|   |-- complete.c    CHAPTER_5.4. Completions 机制
|   |-- faulty.c    CHAPTER_4.5.1. oops 消息
|   |-- hello.c        CHAPTER_2.2：超级用户可以加载和卸载模块示例。
|   |-- hellop.c    CHAPTER_2.8. 模块参数
|   |-- jiq.c        CHAPTER_7.6.1. 共享队列
|   |-- jit.c        CHAPTER_7.4.1. 定时器 API
|   |-- kdataalign.c    CHAPTER_11.4.4. 数据对齐展示编译器如何强制对齐
|   |-- kdatasize.c    CHAPTER_11.1：标准 C 类型的使用。显示长整型和指针在 64-位 平台   
                上的不同大小。
|   |-- seq.c        CHAPTER_4.3.1.4. seq_file 接口
|   |-- silly.c        CHAPTER_9.4.5. 在 1 MB 之下的 ISA 内存
|   `-- sleepy.c    CHAPTER_6.2.2. 简单睡眠
|-- misc-progs
|   |-- Makefile
|   |-- asynctest.c    CHAPTER_6.4. 异步通知
|   |-- dataalign    CHAPTER_11.4.4. 数据对齐展示编译器如何强制对齐
|   |-- dataalign.c    CHAPTER_11.4.4. 数据对齐展示编译器如何强制对齐
|   |-- datasize    CHAPTER_11.1：标准 C 类型的使用。
|   |-- datasize.c    CHAPTER_11.1：标准 C 类型的使用。
|   |-- gdbline        CHAPTER_4.6.1. 使用 gdb。为给定的模块可以创建这个命令
|   |-- inp        CHAPTER_9.2.3. 从用户空间的 I/O 存取。从命令行读写端口的小工具,
                在用户空间.
|   |-- inp.c
|   |-- load50        CHAPTER_7.3.1.1. 忙等待。这个程序派生出许多什么都不做的进程, 但   
                是以一种 CPU-密集的方式来做.
|   |-- load50.c
|   |-- mapcmp        Simple program to compare two mmap'd areas.
|   |-- mapcmp.c
|   |-- mapper        CHAPTER_15.2.6. 重新映射 RAM
|   |-- mapper.c
|   |-- nbtest        CHAPTER_6.2.6. 测试 scullpipe 驱动
|   |-- nbtest.c
|   |-- netifdebug    CHAPTER_17.3.3. 接口信息
|   |-- netifdebug.c
|   |-- outp        CHAPTER_9.2.3. 从用户空间的 I/O 存取。从命令行读写端口的小工具,
                在用户空间.
|   |-- outp.c
|   |-- polltest    Test out reading with poll()
|   |-- polltest.c
|   |-- setconsole    CHAPTER_4.2.2. 重定向控制台消息
|   |-- setconsole.c
|   |-- setlevel    CHAPTER_4.2.1. printk
|   `-- setlevel.c
|-- pci
|   |-- Makefile
|   `-- pci_skel.c    CHAPTER_12.1.5. 注册一个 PCI 驱动
|-- sbull
|   |-- Makefile
|   |-- sbull.c        CHAPTER_16.1. 注册
|   |-- sbull.h
|   |-- sbull_load
|   `-- sbull_unload
|-- scull        CHAPTER_3. 字符驱动。scull( Simple Character Utility for    
                Loading Localities)
|   |-- Makefile
|   |-- access.c
|   |-- main.c
|   |-- main.c.bak
|   |-- pipe.c
|   |-- pipe.c.bak
|   |-- scull.h
|   |-- scull.init
|   |-- scull_load
|   `-- scull_unload
|-- scullc        CHAPTER_8.2.1. 一个基于 Slab 缓存的 scull: scullc
|   |-- Makefile
|   |-- main.c
|   |-- mmap.c
|   |-- scullc.h
|   |-- scullc_load
|   `-- scullc_unload
|-- sculld        CHAPTER_14.4.2.3. 设备结构嵌入
|   |-- Makefile
|   |-- main.c
|   |-- mmap.c
|   |-- sculld.h
|   |-- sculld_load
|   `-- sculld_unload
|-- scullp        CHAPTER_8.3.1. 一个使用整页的 scull: scullp
|   |-- Makefile
|   |-- main.c
|   |-- mmap.c
|   |-- scullp.h
|   |-- scullp_load
|   `-- scullp_unload
|-- scullv        CHAPTER_15.2.7. 重映射内核虚拟地址
|   |-- Makefile
|   |-- main.c
|   |-- mmap.c
|   |-- scullv.h
|   |-- scullv_load
|   `-- scullv_unload
|-- short        CHAPTER_10.2.4. 实现一个处理    （中断）
|   |-- Makefile
|   |-- short.c
|   |-- short_load
|   `-- short_unload
|-- shortprint        CHAPTER_10.5.1. 一个写缓存例子
|   |-- Makefile
|   |-- shortprint.c
|   |-- shortprint.h
|   |-- shortprint_load
|   `-- shortprint_unload
|-- simple        CHAPTER_15.2. mmap 设备操作。simple( Simple Implementation    
            Mapping Pages with Little Enthusiasm)
|   |-- Makefile
|   |-- simple.c
|   |-- simple_load
|   `-- simple_unload
|-- skull        LDD2：skull.c对ISAI/O的内存探测分析
|   |-- Makefile
|   |-- skull_clean.c
|   `-- skull_init.c
|-- snull        CHAPTER_17.2. 连接到内核
|   |-- Makefile
|   |-- snull.c
|   |-- snull.h
|   |-- snull_load
|   `-- snull_unload
|-- tty            CHAPTER_18. TTY 驱动
|   |-- Makefile
|   |-- tiny_serial.c
|   `-- tiny_tty.c
`-- usb            CHAPTER_13. USB 驱动
    |-- Makefile
    `-- usb-skeleton.c

18 directories, 111 files
```
