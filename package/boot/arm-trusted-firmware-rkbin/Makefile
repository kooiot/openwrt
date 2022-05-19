#
# Copyright (C) 2022 Dirk Chang <dirk@kooiot.com>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=arm-trusted-firmware-rkbin
PKG_RELEASE:=2

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL=https://github.com/rockchip-linux/rkbin
PKG_SOURCE_DATE:=2021-06-01
PKG_SOURCE_VERSION:=7d631e0d5b2d373b54d4533580d08fb9bd2eaad4
PKG_MIRROR_HASH:=972c978e2df63e1d2fc38f4537d2ad92f87c656c0dd89b7f8deffd6075fff632

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
	$(CP) $(PKG_BUILD_DIR)/bin/rk35/rk3568_bl31_v1.24.elf $(STAGING_DIR_IMAGE)/rk3568_bl31.elf
endef

define Package/arm-trusted-firmware-rkbin/install
endef

$(eval $(call BuildPackage,arm-trusted-firmware-rkbin))
