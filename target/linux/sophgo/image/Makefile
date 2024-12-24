# SPDX-License-Identifier: GPL-2.0-only

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/image.mk

FAT32_BLOCK_SIZE=1024
FAT32_BLOCKS=$(shell echo $$(($(CONFIG_SOPHGO_SD_BOOT_PARTSIZE)*1024*1024/$(FAT32_BLOCK_SIZE))))
BOOTFS_SIZE=$(shell echo $$(($(CONFIG_SOPHGO_SD_BOOT_PARTSIZE)*1024*1024)))

KERNEL_LOADADDR:=0x80200000

define Build/riscv-sdcard-1
	rm -f $@.boot
	rm -rf $@.bootfs
	mkdir $@.bootfs
	cp -i $(STAGING_DIR_IMAGE)/$(DEVICE_NAME)-boot.scr $@.bootfs/boot.scr
	cp -i $(DTS_DIR)/$(DEVICE_DTS).dtb $@.bootfs/dtb
	cp -i $(IMAGE_KERNEL) $@.bootfs/uImage

	echo $$(stat -c%s $(IMAGE_ROOTFS)) > $@.rootfs.size
	cp -i $@.rootfs.size $@.bootfs/rootfs.size

	$(STAGING_DIR_HOST)/bin/make_ext4fs \
		-l $(BOOTFS_SIZE) -b $(CONFIG_TARGET_EXT4_BLOCKSIZE) \
		$(if $(CONFIG_TARGET_EXT4_RESERVED_PCT),-m $(CONFIG_TARGET_EXT4_RESERVED_PCT)) \
		$(if $(CONFIG_TARGET_EXT4_JOURNAL),,-J) \
		$(if $(SOURCE_DATE_EPOCH),-T $(SOURCE_DATE_EPOCH)) \
		$@.boot $@.bootfs/

	./gen_sophgo_sdcard_img.sh \
		$@ \
		$@.boot \
		$(IMAGE_ROOTFS) \
		$(CONFIG_SOPHGO_SD_BOOT_PARTSIZE) \
		$(CONFIG_TARGET_ROOTFS_PARTSIZE) \
		$(STAGING_DIR_IMAGE)/$(DEVICE_NAME)-u-boot-sophgo-with-spl.bin
endef

define Build/riscv-sdcard
	rm -f $@.boot
	rm -rf $@.bootfs
	mkdir $@.bootfs

	cp $(IMAGE_KERNEL) $@.bootfs/Image.lzma
	cp $(DTS_DIR)/$(DEVICE_DTS).dtb $@.bootfs/dtb

	cp ./multi.its $@.multi.its
	sed -i "s#\$${images_dir}#$(@).bootfs#g" $@.multi.its
	mkimage -f $@.multi.its $@.bootfs/boot.sd

	mkfs.fat $@.boot  -C $(FAT32_BLOCKS)

	mcopy -i $@.boot $(STAGING_DIR_IMAGE)/$(DEVICE_NAME)-fip.bin ::fip.bin
	mcopy -i $@.boot $@.bootfs/boot.sd ::boot.sd

	./gen_sophgo_sdcard_img.sh \
		$@ \
		$@.boot \
		$(IMAGE_ROOTFS) \
		$(CONFIG_SOPHGO_SD_BOOT_PARTSIZE) \
		$(CONFIG_TARGET_ROOTFS_PARTSIZE) \
		$(STAGING_DIR_IMAGE)/$(DEVICE_NAME)-u-boot-sophgo-with-spl.bin
endef

define Device/Default
  PROFILES := Default
  KERNEL_NAME := Image
  KERNEL := kernel-bin | lzma
  IMAGES := sdcard.img.gz
  IMAGE/sdcard.img.gz := riscv-sdcard | append-metadata | gzip
endef

include $(SUBTARGET).mk

$(eval $(call BuildImage))
