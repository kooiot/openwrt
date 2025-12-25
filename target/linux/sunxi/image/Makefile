# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2013-2016 OpenWrt.org
# Copyright (C) 2016 Yousong Zhou

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/image.mk

BOOTFS_SIZE=$(shell echo $$(($(CONFIG_SUNXI_SD_BOOT_PARTSIZE)*1024*1024)))

DEVICE_VARS := SUNXI_DTS SUNXI_DTS_DIR

# UBIFS_OPTS := -F -m 2048 -e 124KiB -c 4096 -U
# UBI_OPTS = -m 2048 -p 128KiB -s 512 -O 2048

define Build/sunxi-sdcard
	rm -f $@.boot
	rm -rf $@.bootfs
	mkdir $@.bootfs
	cp -i $(STAGING_DIR_IMAGE)/$(DEVICE_NAME)-boot.scr $@.bootfs/boot.scr
	cp -i $(DTS_DIR)/$(SUNXI_DTS).dtb $@.bootfs/dtb
	cp -i $(IMAGE_KERNEL) $@.bootfs/uImage

	echo $$(stat -c%s $(IMAGE_ROOTFS)) > $@.rootfs.size
	cp -i $@.rootfs.size $@.bootfs/rootfs.size

	$(STAGING_DIR_HOST)/bin/make_ext4fs \
		-l $(BOOTFS_SIZE) -b $(CONFIG_TARGET_EXT4_BLOCKSIZE) \
		$(if $(CONFIG_TARGET_EXT4_RESERVED_PCT),-m $(CONFIG_TARGET_EXT4_RESERVED_PCT)) \
		$(if $(CONFIG_TARGET_EXT4_JOURNAL),,-J) \
		$(if $(SOURCE_DATE_EPOCH),-T $(SOURCE_DATE_EPOCH)) \
		$@.boot $@.bootfs/

	./gen_sunxi_sdcard_img.sh $@ \
		$@.boot \
		$(IMAGE_ROOTFS) \
		$(CONFIG_SUNXI_SD_BOOT_PARTSIZE) \
		$(CONFIG_TARGET_ROOTFS_PARTSIZE) \
		$(STAGING_DIR_IMAGE)/$(DEVICE_NAME)-u-boot-with-spl.bin
	rm -f $@.boot
	rm -rf $@.bootfs
endef

define Build/sunxi-spinandboot-v3s
	./gen_sunxi_spinand_onlyboot_img.sh $@ \
		$(STAGING_DIR_IMAGE)/$(DEVICE_NAME)-u-boot-with-spl.bin \
		$(CONFIG_SUNXI_SPINAND_PAGESIZE) \
		$(CONFIG_SUNXI_SPINAND_BLOCKSIZE) \
		$(CONFIG_SUN8I_V3S_SPINAND_DUPLICATE_BOOT_SPL) \
		$(CONFIG_SUN8I_V3S_SPINAND_DUPLICATE_BOOT_UBOOT)
endef

define Build/sunxi-spl-bin
	rm -f $@
	cp $(STAGING_DIR_IMAGE)/$(DEVICE_NAME)-u-boot-with-spl.bin $@
endef

define Build/sunxi-kernelubifs
	./gen_sunxi_kernel_ubifs.sh $@ \
		$(DTS_DIR)/$(SUNXI_DTS).dtb \
		$(CONFIG_SUNXI_SPINAND_PAGESIZE) \
		$(CONFIG_SUNXI_SPINAND_BLOCKSIZE)
endef

# why \x00\x00\x00\x00 for zImage-initramfs
define Device/Default
  PROFILES := Default
  KERNEL_NAME := zImage
  KERNEL := kernel-bin | uImage none
  IMAGES := sdcard.img.gz
  IMAGE/sdcard.img.gz := sunxi-sdcard | append-metadata | gzip
  SUNXI_DTS_DIR := allwinner/
  SUNXI_DTS = $$(SUNXI_DTS_DIR)$$(SOC)-$(lastword $(subst _, ,$(1)))
endef

define Device/NAND_V3S
  BLOCKSIZE := 128k
  PAGESIZE := 2048
  SUBPAGESIZE := 2048
  VID_HDR_OFFSET := 2048
  IMAGE_SIZE := 65536k
  KERNEL_IN_UBI := 1
  UBOOTENV_IN_UBI := 1
  UBINIZE_OPTS := -vv
  KERNEL := kernel-bin | lzma | uImage lzma | sunxi-kernelubifs
  IMAGES := ubispinand.img.gz
  UBIFS_OPTS := -F -m $(CONFIG_SUNXI_SPINAND_PAGESIZE) \
				-e $(shell echo $$(($(CONFIG_SUNXI_SPINAND_BLOCKSIZE) - (($(CONFIG_SUNXI_SPINAND_PAGESIZE)/1024)*2))))KiB \
				-c 880 -U
  IMAGE/ubispinand.img.gz := \
      sunxi-spinandboot-v3s | \
      pad-to $$(CONFIG_SUN8I_V3S_OFFSET_UBI) | \
      append-ubi | \
      gzip
endef

define Device/NAND
  BLOCKSIZE := 128k
  PAGESIZE := 2048
  SUBPAGESIZE := 2048
  VID_HDR_OFFSET := 2048
  IMAGE_SIZE := 131072k
  KERNEL_IN_UBI := 1
  UBOOTENV_IN_UBI := 1
  UBINIZE_OPTS := -E 5
  KERNEL := kernel-bin | lzma | uImage lzma | sunxi-kernelubifs
  IMAGES := factory.img sysupgrade.bin
  IMAGE/factory.img := \
	  sunxi-spl-bin | \
      pad-to $$(CONFIG_SUN8I_V3S_OFFSET_UBI) | \
      append-ubi | check-size $$$$(IMAGE_SIZE)
  IMAGE/sysupgrade.bin := \
	  sysupgrade-tar | append-metadata | check-size $$$$(IMAGE_SIZE)
endef

define Device/FitImageLzma
  KERNEL = kernel-bin | lzma | fit lzma $$(DTS_DIR)/$$(SUNXI_DTS).dtb
endef

define Device/FitImageGzip
  KERNEL = kernel-bin | gzip | fit gzip $$(DTS_DIR)/$$(SUNXI_DTS).dtb
endef

include $(SUBTARGET).mk

$(eval $(call BuildImage))
