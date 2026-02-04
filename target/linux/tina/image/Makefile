# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2025-2026 KooIoT.com

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/image.mk

BOOTFS_SIZE=$(shell echo $$(($(CONFIG_TARGET_KERNEL_PARTSIZE)*1024*1024)))

FAT32_BLOCK_SIZE=1024
FAT32_BLOCKS=$(shell echo $$(($(CONFIG_TARGET_KERNEL_PARTSIZE)*1024*1024/$(FAT32_BLOCK_SIZE))))

DEVICE_VARS := SUNXI_DTS SUNXI_DTS_DIR
KERNEL_LOADADDR:=0x41000000

define Build/sunxi-tina-img
	# Creates the final SD/eMMC images, 
	# combining boot partition, root partition as well as the u-boot bootloader

	rm -f $@.boot
	mkfs.fat $@.boot -C $(FAT32_BLOCKS)
	
	rm -f $(STAGING_DIR_IMAGE)/$(DEVICE_NAME)-uImage
	mkimage -A arm -O linux -T kernel -C none -a "0x41000000" -e "0x41000000" -n "Linux kernel" -d $(IMAGE_KERNEL) $(STAGING_DIR_IMAGE)/$(DEVICE_NAME)-uImage

	# Use boot.scr instead of extlinux
	mkimage -A arm -O linux -T script -C none -a 0 -e 0 -d $(if $(1),$(1),mmc).bootscript $(STAGING_DIR_IMAGE)/$(DEVICE_NAME)-boot.scr
	# echo $$(stat -c%s $(IMAGE_ROOTFS)) > $@.boot/rootfs.size

	mcopy -i $@.boot $(DTS_DIR)/$(SUNXI_DTS).dtb ::dtb
	mcopy -i $@.boot $(STAGING_DIR_IMAGE)/$(DEVICE_NAME)-uImage ::uImage
	mcopy -i $@.boot $(STAGING_DIR_IMAGE)/$(DEVICE_NAME)-boot.scr ::boot.scr
	./gen_sunxi_tina_sdcard_image.sh $@ \
		$@.boot \
		$(IMAGE_ROOTFS) \
		$(CONFIG_TARGET_KERNEL_PARTSIZE) \
		$(CONFIG_TARGET_ROOTFS_PARTSIZE) \
		$(SOC)/boot0_sdcard.fex \
		$(SOC)/boot_package.fex \
		36864
	rm -f $@.boot

endef

define Build/sunxi-sdcard
	rm -f $@.boot
	rm -rf $@.bootfs
	mkdir $@.bootfs
	cp -i $(STAGING_DIR_IMAGE)/$(DEVICE_NAME)-boot.scr $@.bootfs/boot.scr
	# cp -i $(STAGING_DIR_IMAGE)/$(DEVICE_NAME)-scp.bin $@.bootfs/scp.bin
	# cp -i $(STAGING_DIR_IMAGE)/$(DEVICE_NAME)-bl31.bin $@.bootfs/bl31.bin
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
		$(CONFIG_TARGET_KERNEL_PARTSIZE) \
		$(CONFIG_TARGET_ROOTFS_PARTSIZE) \
		$(STAGING_DIR_IMAGE)/$(DEVICE_NAME)-u-boot-with-spl.bin
	rm -f $@.boot
	rm -rf $@.bootfs
endef

define Device/Default
  PROFILES := Default
  # KERNEL := kernel-bin | uImage none
  KERNEL := kernel-bin
  IMAGES := sdcard.img.gz
  # IMAGE/sdcard.img.gz := sunxi-sdcard | append-metadata | gzip
  IMAGE/sdcard.img.gz := sunxi-tina-img | append-metadata | gzip
  SUNXI_DTS_DIR := allwinner/
  SUNXI_DTS = $$(SUNXI_DTS_DIR)$$(SOC)-$(lastword $(subst _, ,$(1)))
endef

include $(SUBTARGET).mk

$(eval $(call BuildImage))
