include $(TOPDIR)/rules.mk

PKG_NAME:=xradio
PKG_RELEASE=1

PKG_LICENSE:=GPLv2
PKG_LICENSE_FILES:=

PKG_SOURCE_URL:=https://github.com/fifteenhex/xradio.git
PKG_SOURCE_PROTO:=git
PKG_SOURCE_DATE:=2021-04-20
PKG_SOURCE_VERSION:=6bf0e2e21c80456e2a3d4ad1267caecde7165871
PKG_HASH:=d0d72e09ce9a775afd864062a90737c330db63fd97e6f98b08939138ec67cdb8

PKG_BUILD_PARALLEL:=1

include $(INCLUDE_DIR)/kernel.mk
include $(INCLUDE_DIR)/package.mk

define KernelPackage/xradio
  SUBMENU:=Wireless Drivers
  TITLE:=Xradio xr819 wifi-drivers support
  DEPENDS:=@TARGET_sunxi +kmod-mac80211 \
	+@DRIVER_11AC_SUPPORT +@DRIVER_11N_SUPPORT
  KCONFIG:= \
	CONFIG_WLAN_VENDOR_XRADIO=m \
	CONFIG_XRADIO_USE_EXTENSIONS=y
  FILES:=$(PKG_BUILD_DIR)/xradio_wlan.ko
  AUTOLOAD:=$(call AutoProbe,xradio_wlan)
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

define Build/Compile
	+$(MAKE) $(PKG_JOBS) -C "$(LINUX_DIR)" \
		$(KERNEL_MAKE_FLAGS) \
		$(PKG_MAKE_FLAGS) \
		M="$(PKG_BUILD_DIR)" \
		NOSTDINC_FLAGS="$(NOSTDINC_FLAGS)" \
		modules
endef

define KernelPackage/xradio/install
	$(INSTALL_DIR) $(1)/lib/firmware/xr819
endef

$(eval $(call KernelPackage,xradio))
