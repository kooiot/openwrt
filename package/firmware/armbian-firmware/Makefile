#
# Copyright (C) 2021 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=armbian-firmware
PKG_SOURCE_DATE:=2021-01-14
PKG_SOURCE_VERSION:=292e1e5b5bc5756e9314ea6d494d561422d23264
#PKG_MIRROR_HASH:=1cb20a749696852be0a512d51961365dd9c031362af0af1a2b9f5a3fb894885f
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://github.com/armbian/firmware.git

PKG_MAINTAINER:=Dirk Chang <dirk@kooiot.com>

include $(INCLUDE_DIR)/package.mk

define Package/armbian-firmware-default
  SECTION:=firmware
  CATEGORY:=Firmware
  URL:=https://armbian.com
endef

define Build/Compile
	true
endef

# Armbian AP6212 firmware
define Package/armbian-firmware-xr819
  $(Package/armbian-firmware-default)
  TITLE:=XR819 firmware from Armbian
endef

define Package/armbian-firmware-xr819/install
	$(INSTALL_DIR) $(1)/lib/firmware/xr819
	$(INSTALL_DATA) \
		$(PKG_BUILD_DIR)/xr819/boot_xr819.bin \
		$(1)/lib/firmware/xr819/boot_xr819.bin
	$(INSTALL_DATA) \
		$(PKG_BUILD_DIR)/xr819/fw_xr819.bin \
		$(1)/lib/firmware/xr819/fw_xr819.bin
	$(INSTALL_DATA) \
		$(PKG_BUILD_DIR)/xr819/sdd_xr819.bin \
		$(1)/lib/firmware/xr819/sdd_xr819.bin
endef

$(eval $(call BuildPackage,armbian-firmware-xr819))
