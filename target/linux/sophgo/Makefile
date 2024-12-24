# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2023 Yan Lu <yan.lu@sophgo.com>
#

include $(TOPDIR)/rules.mk

ARCH:=riscv64
BOARD:=sophgo
BOARDNAME:=Sophgo RISC-V SoCs
FEATURES:=usb rootfs-part rtc squashfs ext4 nand boot-part
KERNELNAME:=Image dtbs
SUBTARGETS:=generic
#SUBTARGETS:=cv180x cv181x cv1813h

KERNEL_PATCHVER:=6.6

include $(INCLUDE_DIR)/target.mk

define Target/Description
	Build firmware images for the Sophgo RISC-V Boards. 
endef

$(eval $(call BuildTarget))
