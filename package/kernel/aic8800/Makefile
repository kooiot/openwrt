#
# Copyright (C) 2017 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=aic8800
PKG_RELEASE:=1

PKG_MAINTAINER:=Dirk Chang <dirk@kooiot.com>
PKG_LICENSE:=GPLv2

include $(INCLUDE_DIR)/package.mk

define KernelPackage/aic8800
  SUBMENU:=Wireless Drivers
  TITLE:=AIC8800 AW869A wifi-drivers support
  DEPENDS:=@TARGET_sunxi +kmod-mac80211 \
	+@DRIVER_11AC_SUPPORT +@DRIVER_11N_SUPPORT
  FILES:=$(PKG_BUILD_DIR)/aic8800_fdrv/aic8800_fdrv.ko \
	$(PKG_BUILD_DIR)/aic8800_bsp/aic8800_bsp.ko
  KCONFIG:=CONFIG_AIC8800_WLAN_SUPPORT \
  	CONFIG_AIC_WLAN_SUPPORT \
	CONFIG_AIC_INTF_SDIO
  AUTOLOAD:=$(call AutoProbe,aic8800_fdrv aic8800_bsp)
endef

define KernelPackage/aic8800/description
  Support for AIC8800 or AW869A WiFi6 module 
endef

NOSTDINC_FLAGS = \
	$(KERNEL_NOSTDINC_FLAGS) \
	-I$(PKG_BUILD_DIR) \
	-I$(STAGING_DIR)/usr/include/mac80211-backport/uapi \
	-I$(STAGING_DIR)/usr/include/mac80211-backport \
	-I$(STAGING_DIR)/usr/include/mac80211/uapi \
	-I$(STAGING_DIR)/usr/include/mac80211 \
	-include backport/autoconf.h \
	-include backport/backport.h

AIC_MAKEDEFS = CONFIG_AIC8800_WLAN_SUPPORT=m \
  	CONFIG_AIC_WLAN_SUPPORT=m \
	CONFIG_AIC_INTF_SDIO=y

define Build/Compile
	+$(KERNEL_MAKE) $(AIC_MAKEDEFS) $(PKG_JOBS) \
		M="$(PKG_BUILD_DIR)" \
		NOSTDINC_FLAGS="$(NOSTDINC_FLAGS)" \
		modules
endef

$(eval $(call KernelPackage,aic8800))
