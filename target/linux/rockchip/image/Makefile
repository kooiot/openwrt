# SPDX-License-Identifier: GPL-2.0-only

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/image.mk

DEVICE_VARS += UBOOT_DEVICE_NAME RKIMG_TYPE BOOT_SCRIPT

define Build/Compile
	$(CP) $(LINUX_DIR)/COPYING $(KDIR)/COPYING.linux
endef

### Image scripts ###
define Build/boot-common
	# This creates a new folder copies the dtb (as rockchip.dtb) 
	# and the kernel image (as kernel.img)
	rm -fR $@.boot
	mkdir -p $@.boot

	$(CP) $(DTS_DIR)/$(DEVICE_DTS).dtb $@.boot/rockchip.dtb
	$(CP) $(IMAGE_KERNEL) $@.boot/kernel.img
endef

define Build/boot-script
	# Make an U-boot image and copy it to the boot partition
	mkimage -A arm -O linux -T script -C none -a 0 -e 0 -d $(if $(1),$(1),default).bootscript $@.boot/boot.scr
	echo $$(stat -c%s $(IMAGE_ROOTFS)) > $@.boot/rootfs.size
endef

define Build/boot-script-rk
	$(CP) $(STAGING_DIR_IMAGE)/$(UBOOT_DEVICE_NAME)-boot.scr $@.boot/boot.scr
	echo $$(stat -c%s $(IMAGE_ROOTFS)) > $@.boot/rootfs.size
endef

define Build/pine64-img
	# Creates the final SD/eMMC images, 
	# combining boot partition, root partition as well as the u-boot bootloader

	# Generate a new partition table in $@ with 32 MiB of 
	# alignment padding for the u-boot-rockchip.bin (idbloader + u-boot) to fit:
	# http://opensource.rock-chips.com/wiki_Boot_option#Boot_flow
	#
	# U-Boot SPL expects the U-Boot ITB to be located at sector 0x4000 (8 MiB) on the MMC storage
	$(SCRIPT_DIR)/gen_image_generic.sh \
		$@ \
		$(CONFIG_TARGET_KERNEL_PARTSIZE) $@.boot \
		$(CONFIG_TARGET_ROOTFS_PARTSIZE) $(IMAGE_ROOTFS) \
		32768

	# Build/pine64-img
	# Copy the u-boot-rockchip.bin to the image at sector 0x40
	dd if="$(STAGING_DIR_IMAGE)/$(UBOOT_DEVICE_NAME)-u-boot-rockchip.bin" of="$@" seek=64 conv=notrunc
endef

### Devices ###
define Device/Default
  PROFILES := Default
  KERNEL_NAME := Image
  KERNEL = kernel-bin | lzma | fit lzma $$(DTS_DIR)/$$(DEVICE_DTS).dtb
  BOOT_SCRIPT :=
  RKIMG_TYPE := pine64-img
  IMAGES := sysupgrade.img.gz
  IMAGE/sysupgrade.img.gz = boot-common | boot-script $$(BOOT_SCRIPT) | $$(RKIMG_TYPE) | gzip | append-metadata
  DEVICE_DTS = rockchip/$$(SOC)-$(lastword $(subst _, ,$(1)))
  UBOOT_DEVICE_NAME = $(lastword $(subst _, ,$(1)))-$$(SOC)
endef

define Device/arm32
  KERNEL_NAME := zImage
  KERNEL := kernel-bin | uImage none
  UBOOT_DEVICE_NAME = $(lastword $(subst _, ,$(1)))
endef

define Build/rockchip-uboot-bin
	rm -f $@
	cp $(STAGING_DIR_IMAGE)/$(UBOOT_DEVICE_NAME)-u-boot-rockchip.bin $@
endef

define Build/armv7-kernelubifs
	mkdir -p $@.ubifs-kernel
	# populate the kernel fs with the dtb and the kernel image
	$(CP) $(DTS_DIR)/$(DEVICE_DTS).dtb $@.ubifs-kernel/dtb
	$(CP) $@ $@.ubifs-kernel/uImage

	# create ubifs
	$(STAGING_DIR_HOST)/bin/mkfs.ubifs ${MKUBIFS_OPTS} -r $@.ubifs-kernel -o $@.new
	rm -rf $@.ubifs-kernel
	mv $@.new $@

	$(Build/armv7-backup-ubifs)
endef

define Build/armv7-backup-ubifs
	rm -rf $@.backup
	mkdir -p $@.backup

	$(STAGING_DIR_HOST)/bin/mkfs.ubifs \
		-r $@.backup -F -m $(PAGESIZE) -e 124KiB -c 64 -U -v -o $@.backup.ubifs
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
  UBINIZE_PARTS = backup=$$(KDIR_KERNEL_IMAGE).backup.ubifs=8
  MKUBIFS_OPTS := \
      --min-io-size=$$(PAGESIZE) --leb-size=124KiB --max-leb-cnt=96 \
      --log-lebs=2 --space-fixup --squash-uids
  KERNEL := kernel-bin | lzma | uImage lzma | armv7-kernelubifs
  IMAGES := factory.img factory.ubi sysupgrade.bin
  IMAGE/factory.img := \
      rockchip-uboot-bin | pad-to 2M | \
      append-ubi | check-size $$$$(IMAGE_SIZE)
  IMAGE/factory.ubi =  append-ubi | check-size $$$$(IMAGE_SIZE)
  IMAGE/sysupgrade.bin = \
      sysupgrade-tar | append-metadata | check-size $$$$(IMAGE_SIZE)
endef

include $(SUBTARGET).mk

$(eval $(call BuildImage))
