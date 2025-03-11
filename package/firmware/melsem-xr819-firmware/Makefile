#
# Copyright (C) 2021 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=melsem-xr819-firmware
PKG_VERSION:=2021.04.01
PKG_RELEASE:=1

PKG_SOURCE:=xr819.tar.bz2
PKG_SOURCE_URL:=https://github.com/melsem/openwrt-lede_xradio-xr819_soc-audio/raw/master/firmware/
PKG_HASH:=bc9bae512267e04b6d00ffe18fe68a4181c9a5af35f765dfbabdeedd28c8f4f7

PKG_MAINTAINER:=Dirk Chang <dirk@kooiot.com>

include $(INCLUDE_DIR)/package.mk

define Download/melsem-xr819-firmware
	FILE:=melsem-xr819-$(PKG_VERSION).tar.bz2
	URL:=$(PKG_SOURCE_URL)
	URL_FILE:=$(PKG_SOURCE)
	HASH:=$(PKG_HASH)
endef
$(eval $(call Download,melsem-xr819-firmware))

define Package/melsem-xr819-firmware
  TITLE:=XR819 firmware from Armbian
  SECTION:=firmware
  CATEGORY:=Firmware
  URL:=https://github.com/melsem
endef

define Build/Prepare
	tar -C $(PKG_BUILD_DIR) -xjvf "$(DL_DIR)/melsem-xr819-$(PKG_VERSION).tar.bz2"
endef

define Build/Compile
	true
endef

define Package/melsem-xr819-firmware/install
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

$(eval $(call BuildPackage,melsem-xr819-firmware))
