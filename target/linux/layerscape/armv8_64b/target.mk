# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2016 Jiang Yutang <jiangyutang1978@gmail.com>

ARCH:=aarch64
BOARDNAME:=ARMv8 64-bit based boards
KERNELNAME:=Image dtbs
DEPENDS:=+@KERNEL_BTRFS_FS +@KERNEL_BTRFS_FS_POSIX_ACL

define Target/Description
	Build firmware images for NXP Layerscape ARMv8 64-bit based boards.
endef
