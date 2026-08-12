#
# Copyright (C) 2026 Dirk Chang <dirk@kooiot.com>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=arm-trusted-firmware-a523
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL=https://gitee.com/srdgame/arm-trusted-firmware-a523.git
# PKG_SOURCE_URL:=/opt/cch/github/arm-trusted-firmware-a523
PKG_SOURCE_DATE:=2026-01-28
PKG_SOURCE_VERSION:=808ad4700db076e894c879119d39863ffb1cca30
PKG_MIRROR_HASH:=skip

PKG_MAINTAINER:=Dirk Chang <dirk@kooiot.com>

include $(INCLUDE_DIR)/kernel.mk
include $(INCLUDE_DIR)/trusted-firmware-a.mk
include $(INCLUDE_DIR)/package.mk

define Package/arm-trusted-firmware-a523
    SECTION:=boot
    CATEGORY:=Boot Loaders
    TITLE:=ARM Trusted Firmware for Allwinner A523/T527/T536
    DEPENDS:=@TARGET_tina
endef

define Trusted-Firmware-A/Default
  BUILD_TARGET:=tina
  TFA_IMAGE:=bl31.bin
endef

define Trusted-Firmware-A/sunxi-t536
  NAME:=Allwinner T536
  BUILD_SUBTARGET:=t536
  PLAT:=sun55i_t536
endef

TFA_TARGETS:= \
	sunxi-t536

TFA_MAKE_FLAGS += \
	DEBUG=1

define Package/trusted-firmware-a/install
	$(INSTALL_DIR) $(STAGING_DIR_IMAGE)
	# $(INSTALL_DATA) $(PKG_BUILD_DIR)/build/$(PLAT)/release/bl31.bin $(STAGING_DIR_IMAGE)/$(BUILD_VARIANT)-bl31.bin
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/build/$(PLAT)/debug/bl31.bin $(STAGING_DIR_IMAGE)/$(BUILD_VARIANT)-bl31.bin
endef

$(eval $(call BuildPackage/Trusted-Firmware-A))
