#
# Copyright (C) 2017 Hauke Mehrtens
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=arm-trusted-firmware-sunxi
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL=https://github.com/ARM-software/arm-trusted-firmware
PKG_SOURCE_DATE:=2020-04-20
PKG_SOURCE_VERSION:=8ff55a9e14a23d7c7f89f52465bcc6307850aa33
PKG_MIRROR_HASH:=996496e9fc7c561e012dc644893df307c56180aed4933e56a4fd7ce8ccbac82a

PKG_LICENSE:=BSD-3-Clause
PKG_LICENSE_FILES:=license.md

PKG_MAINTAINER:=Hauke Mehrtens <hauke@hauke-m.de>

include $(INCLUDE_DIR)/package.mk

define Package/arm-trusted-firmware-sunxi
    SECTION:=boot
    CATEGORY:=Boot Loaders
    TITLE:=ARM Trusted Firmware for Allwinner
    DEPENDS:=@TARGET_sunxi_cortexa53
endef

define Package/arm-trusted-firmware-sunxi-a64
$(call Package/arm-trusted-firmware-sunxi)
    TITLE:=ARM Trusted Firmware for Allwinner (A64)
endef

define Package/arm-trusted-firmware-sunxi-h6
$(call Package/arm-trusted-firmware-sunxi)
    TITLE:=ARM Trusted Firmware for Allwinner (H6)
endef

ATF_PLAT:=none
ifeq ($(CONFIG_PACKAGE_arm-trusted-firmware-sunxi-h6), y)
	ATF_PLAT=sun50i_h6
else
	AFT_PLAT=sun50i_a64
endif

export GCC_HONOUR_COPTS=s

MAKE_VARS = \
	CROSS_COMPILE="$(TARGET_CROSS)"

MAKE_FLAGS += \
	PLAT=$(ATF_PLAT) \
	bl31

define Build/InstallDev
	$(INSTALL_DIR) $(STAGING_DIR_IMAGE)
	$(CP) $(PKG_BUILD_DIR)/build/$(ATF_PLAT)/release/bl31.bin $(STAGING_DIR_IMAGE)/bl31.bin
endef

$(eval $(call BuildPackage,arm-trusted-firmware-sunxi-a64))
$(eval $(call BuildPackage,arm-trusted-firmware-sunxi-h6))
