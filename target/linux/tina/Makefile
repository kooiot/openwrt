# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2025-2026 OpenWrt.org

include $(TOPDIR)/rules.mk

BOARD:=tina
BOARDNAME:=Allwinner Tina SDK boards
FEATURES:=fpu usb ext4 display boot-part rootfs-part rtc squashfs usbgadget
SUBTARGETS:=t536

KERNEL_PATCHVER:=5.15

include $(INCLUDE_DIR)/target.mk

DEFAULT_PACKAGES += uboot-envtools
DEFAULT_PACKAGES += partx-utils mkf2fs e2fsprogs blkid

$(eval $(call BuildTarget))
