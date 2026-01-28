#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_VERSION:=a523-2025.01
PKG_RELEASE:=$(AUTORELEASE)

PKG_SOURCE_PROTO:=git
#PKG_SOURCE_URL:=https://github.com/jernejsk/u-boot.git
PKG_SOURCE_URL:=/opt/cch/github/u-boot-a523
PKG_SOURCE_VERSION:=bfa0d56999ef9aa8b5731f5fb9e0e7de78c1f660
PKG_MIRROR_HASH:=skip

PKG_MAINTAINER:=Dirk Chang <dirk@kooiot.com>

UBOOT_USE_BINMAN:=1
UBOOT_USE_INTREE_DTC:=1

include $(INCLUDE_DIR)/u-boot.mk
include $(INCLUDE_DIR)/package.mk

define U-Boot/Default
  BUILD_TARGET:=tina
  UENV:=default
  HIDDEN:=1
endef


# T536 boards

define U-Boot/sunxi-t536/Default
  BUILD_SUBTARGET:=t536
  DEPENDS:=+PACKAGE_u-boot-$(1):trusted-firmware-a-sunxi-t536
  ATF:=sunxi-t536-bl31.bin
  SCP_FILE:=bin/t536/scp.bin
endef

define U-Boot/tlink-t536
  $(U-Boot/sunxi-t536/Default)
  NAME:=ThingsLink T536
  BUILD_DEVICES:= \
    kooiot_tlink-t536
endef

UBOOT_TARGETS := \
  tlink-t536

UBOOT_CONFIGURE_VARS += USE_PRIVATE_LIBGCC=yes

UBOOT_CUSTOMIZE_CONFIG := \
	--disable TOOLS_MKEFICAPSULE \
	--set-str MKIMAGE_DTC_PATH $(PKG_BUILD_DIR)/scripts/dtc/dtc

UBOOT_MAKE_FLAGS += \
  BL31=$(STAGING_DIR_IMAGE)/$(ATF) \
  SCP=$(STAGING_DIR_IMAGE)/$(BUILD_DEVICES)-scp.bin

define Build/Configure
	$(call Build/Configure/U-Boot)

	# $(CP) $(SCP_FILE) $(STAGING_DIR_IMAGE)/$(BUILD_DEVICES)-scp.bin
endef

define Build/InstallDev
	$(INSTALL_DIR) $(STAGING_DIR_IMAGE)
	$(CP) $(PKG_BUILD_DIR)/u-boot-sunxi-with-spl.bin \
		$(STAGING_DIR_IMAGE)/$(BUILD_DEVICES)-u-boot-with-spl.bin

	mkimage -C none -A arm -T script -d uEnv-$(UENV).txt \
		$(STAGING_DIR_IMAGE)/$(BUILD_DEVICES)-boot.scr
endef

define Package/u-boot/install/default
endef

$(eval $(call BuildPackage/U-Boot))
