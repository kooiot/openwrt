#
# Copyright (C) 2022 Dirk Chang <dirk@kooiot.com>
#

include $(TOPDIR)/rules.mk

PKG_NAME:=upd70202x-firmware
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE_DATE:=2019-07-09
PKG_SOURCE_URL:=https://github.com/denisandroid/uPD72020x-Firmware.git
PKG_SOURCE_VERSION:=1cf8542aa64d816301ea286a8937d9b09ebcbc99
# PKG_MIRROR_HASH:=

PKG_MAINTAINER:=Dirk Chang <dirk@kooiot.com>

include $(INCLUDE_DIR)/package.mk

define Package/upd72020x-firmware-
  SECTION:=firmware
  CATEGORY:=Firmware
  TITLE:=uPD72020x firmware files
  URL:=https://github.com/denisandroid/uPD72020x-Firmware
endef

define Build/Compile
	true
endef

define Package/upd72020x-firmware/install
	$(INSTALL_DIR) $(1)/lib/firmware/
	$(INSTALL_DATA) \
		$(PKG_BUILD_DIR)/UPDATE.mem \
		$(1)/lib/firmware/renesas_usb_fw.mem
endef

$(eval $(call BuildPackage,upd72020x-firmware))
