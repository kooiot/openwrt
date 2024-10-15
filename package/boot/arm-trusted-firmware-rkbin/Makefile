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
PKG_SOURCE_URL=https://gitlab.com/firefly-linux/rkbin.git
PKG_SOURCE_DATE:=2022-04-24
PKG_SOURCE_VERSION:=bfc17ed17a7e9703da510afb52840ebb2ba22675
#PKG_MIRROR_HASH:=54bfe7d031c56767def50398c17ef40480c8dff44b73ae864a8d92f224a0cf1e
PKG_MIRROR_HASH:=skip

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
	$(CP) $(PKG_BUILD_DIR)/bin/rk35/rk3568_bl31_v1.32.elf $(STAGING_DIR_IMAGE)/rk3568_bl31.elf
	$(CP) $(PKG_BUILD_DIR)/bin/rk35/rk3568_bl32_v2.08.bin $(STAGING_DIR_IMAGE)/rk3568_bl32.bin
	$(CP) $(PKG_BUILD_DIR)/bin/rk35/rk3568_ddr_1560MHz_v1.13.bin $(STAGING_DIR_IMAGE)/rk3568_ddr_1560MHz.bin
	$(CP) $(PKG_BUILD_DIR)/bin/rk35/rk3568_ddr_528MHz_v1.13.bin $(STAGING_DIR_IMAGE)/rk3568_ddr_528MHz.bin

	$(CP) $(PKG_BUILD_DIR)/tools/ddrbin_param.txt $(PKG_BUILD_DIR)/ddrbin_param_new.txt
	$(SED) 's/uart baudrate=/uart baudrate=115200/g' $(PKG_BUILD_DIR)/ddrbin_param_new.txt
	$(PKG_BUILD_DIR)/tools/ddrbin_tool $(PKG_BUILD_DIR)/ddrbin_param_new.txt  $(STAGING_DIR_IMAGE)/rk3568_ddr_1560MHz.bin
	$(PKG_BUILD_DIR)/tools/ddrbin_tool $(PKG_BUILD_DIR)/ddrbin_param_new.txt  $(STAGING_DIR_IMAGE)/rk3568_ddr_528MHz.bin
endef

define Package/arm-trusted-firmware-rkbin/install
endef

$(eval $(call BuildPackage,arm-trusted-firmware-rkbin))
