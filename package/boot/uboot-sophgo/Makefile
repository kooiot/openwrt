# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2023 OpenWrt.org
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_VERSION:=2024.10
PKG_RELEASE:=1
PKG_HASH:=b28daf4ac17e43156363078bf510297584137f6df50fced9b12df34f61a92fb0

include $(INCLUDE_DIR)/u-boot.mk
include $(INCLUDE_DIR)/package.mk

define U-Boot/Default
  BUILD_TARGET:=sophgo
  BUILD_SUBTARGET:=generic
  UBOOT_IMAGE:=u-boot-sophgo-with-spl.bin
  UENV:=default
  DTS_DIR:=arch/riscv/dts
endef

define U-Boot/milkv_duo_1
  NAME:=Milkv Duo SD
  OPENSBI:=generic
  DEPENDS:=+opensbi_generic
  UBOOT_DTS:=cv1800b-milkv-duo.dtb
  BUILD_DEVICES:=milkv_duo_1
endef

UBOOT_TARGETS := \
	milkv_duo_1 \

UBOOT_MAKE_FLAGS += \
	OPENSBI=$(STAGING_DIR_IMAGE)/fw_dynamic-${OPENSBI}.bin

define Build/Configure
	$(call Build/Configure/U-Boot)
	sed -i 's/CONFIG_TOOLS_LIBCRYPTO=y/# CONFIG_TOOLS_LIBCRYPTO is not set/' $(PKG_BUILD_DIR)/.config
endef

define Build/InstallDev
	$(INSTALL_DIR) $(STAGING_DIR_IMAGE)
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/$(DTS_DIR)/$(UBOOT_DTS) $(STAGING_DIR_IMAGE)/$(UBOOT_DTS)
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/$(UBOOT_IMAGE) $(STAGING_DIR_IMAGE)/$(BUILD_VARIANT)-$(UBOOT_IMAGE)
	mkimage -C none -A riscv -T script -d uEnv-$(UENV).txt \
		$(STAGING_DIR_IMAGE)/$(BUILD_DEVICES)-boot.scr
endef

define Package/u-boot/install/default
endef

$(eval $(call BuildPackage/U-Boot))
