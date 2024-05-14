#
# Copyright (C) 2024 Dirk Chang <dirk@kooiot.com>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=aic8800-firmware
PKG_VERSION:=2024-05-14
PKG_RELEASE:=1

PKG_MAINTAINER:=Dirk Chang <dirk@kooiot.com>
PKG_LICENSE:=GPLv2

include $(INCLUDE_DIR)/package.mk

define Package/aic8800-firmware
  TITLE:=XR819 firmware from T113 Tina SDK
  SECTION:=firmware
  CATEGORY:=Firmware
endef

define Build/Compile
	true
endef

define Package/aic8800-firmware/install
	$(INSTALL_DIR) $(1)/$(FIRMWARE_PATH)
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/fmacfw.bin $(1)/$(FIRMWARE_PATH)/fmacfw.bin
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/fmacfw_usb.bin $(1)/$(FIRMWARE_PATH)/fmacfw_usb.bin
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/fmacfw_rf.bin $(1)/$(FIRMWARE_PATH)/fmacfw_rf.bin
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/fmacfw_rf_usb.bin $(1)/$(FIRMWARE_PATH)/fmacfw_rf_usb.bin
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/fw_adid.bin $(1)/$(FIRMWARE_PATH)/fw_adid.bin
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/fw_adid_u03.bin $(1)/$(FIRMWARE_PATH)/fw_adid_u03.bin
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/fw_patch.bin $(1)/$(FIRMWARE_PATH)/fw_patch.bin
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/fw_patch_u03.bin $(1)/$(FIRMWARE_PATH)/fw_patch_u03.bin
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/fw_patch_table.bin $(1)/$(FIRMWARE_PATH)/fw_patch_table.bin
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/fw_patch_table_u03.bin $(1)/$(FIRMWARE_PATH)/fw_patch_table_u03.bin
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/aic_userconfig.txt $(1)/$(FIRMWARE_PATH)/aic_userconfig.txt
endef

$(eval $(call BuildPackage,aic8800-firmware))
