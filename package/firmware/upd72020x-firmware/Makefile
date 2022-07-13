#
# Copyright (C) 2022 Dirk Chang <dirk@kooiot.com>
#

include $(TOPDIR)/rules.mk

PKG_NAME:=upd70202x-firmware
PKG_VERSION:=2022-07-11
PKG_RELEASE:=1

PKG_MAINTAINER:=Dirk Chang <dirk@kooiot.com>

include $(INCLUDE_DIR)/package.mk

define Package/upd72020x-firmware
  SECTION:=firmware
  CATEGORY:=Firmware
  TITLE:=uPD72020x firmware files
endef

define Build/Compile
	true
endef

define Package/upd72020x-firmware/install
	$(INSTALL_DIR) $(1)/lib/firmware/
	$(CP) -f ./files/K2006040.mem $(1)/lib/firmware/renesas_usb_fw.mem
endef

$(eval $(call BuildPackage,upd72020x-firmware))
