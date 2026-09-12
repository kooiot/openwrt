# SPDX-License-Identifier: GPL-2.0-only

include $(TOPDIR)/rules.mk

BOARD:=rockchip
BOARDNAME:=Rockchip
FEATURES:=ext4 audio usb usbgadget display haveged gpio fpu pci pcie rootfs-part nand boot-part squashfs
SUBTARGETS:=armv7 armv8

KERNEL_PATCHVER:=6.12

define Target/Description
	Build firmware image for Rockchip SoC devices.
endef

include $(INCLUDE_DIR)/target.mk

DEFAULT_PACKAGES += uboot-envtools partx-utils e2fsprogs mkf2fs kmod-gpio-button-hotplug \
		    ethtool fdisk blockd usbutils wpad-basic-mbedtls

$(eval $(call BuildTarget))
