#
# Copyright (C) 2022 Dirk Chang <dirk@kooiot.com>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=arm-trusted-firmware-rkbin
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL=https://github.com/kooiot/rkbin.git
PKG_SOURCE_DATE:=2022-05-19
PKG_SOURCE_VERSION:=fc5c16b0deb3502c88f70ca12a51153d32915356
PKG_MIRROR_HASH:=17445b6252d2b49a56edc8ae1f4443fc298ec6f3f8ce63c3aa0cb29197a5efbf

PKG_MAINTAINER:=Dirk Chang <dirk@kooiot.com>
PKG_LICENSE:=MIT
PKG_LICENSE_FILES:=

include $(INCLUDE_DIR)/package.mk

define Package/arm-trusted-firmware-rkbin
    SECTION:=boot
    CATEGORY:=Boot Loaders
    TITLE:=ARM Trusted Firmware for Rockchip
    DEPENDS:=@TARGET_rockchip_armv8
endef

define Build/Compile
endef

define Build/InstallDev
	$(INSTALL_DIR) -p $(STAGING_DIR_IMAGE)
	$(CP) $(PKG_BUILD_DIR)/rk35/rk3568_bl31_v1.32.elf $(STAGING_DIR_IMAGE)/rk3568_bl31.elf
	$(CP) $(PKG_BUILD_DIR)/rk35/rk3568_ddr_1560MHz_v1.13.bin $(STAGING_DIR_IMAGE)/rk3568_ddr_1560MHz.bin
endef

define Package/arm-trusted-firmware-rkbin/install
endef

$(eval $(call BuildPackage,arm-trusted-firmware-rkbin))
