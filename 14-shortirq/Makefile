obj-m := shortirq.o

export BUILDHOST = n

KERNELDIR_QEMU ?= $(HOME)/linux
KERNELDIR_HOST ?= /lib/modules/$(shell uname -r)/build

# Add your debugging flag (or not) to CFLAGS
ifeq ($(DEBUG),y)
  ccflags-y += -O -g -Wall -std=gnu99 # "-O" is needed to expand inlines
else
  ccflags-y += -O2
endif


all default: modules
install: modules_install

ifeq ($(BUILDHOST),y)
  KERNELDIR ?= $(KERNELDIR_HOST)
desc:
	@echo "\033[32m build on $(shell uname -ior)\033[0m"
else
  KERNELDIR ?= $(KERNELDIR_QEMU)
  KERNELVERS = $(shell cat ${KERNELDIR}/Makefile | grep VERSION | awk '{print $1}'| sed -n '1p' | cut -c11-12)
  KERNEL_PATCHLEVEL = $(shell cat ${KERNELDIR}/Makefile | grep PATCHLEVEL | awk '{print $1}' | sed -n '1p' | cut -c14-15)
desc:
	@echo "\033[32m build on kernel $(KERNELVERS).$(KERNEL_PATCHLEVEL) source files \033[0m"
endif


modules modules_install help clean: desc
	$(MAKE) -C $(KERNELDIR) M=$(shell pwd) $@