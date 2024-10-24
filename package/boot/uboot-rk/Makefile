#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_VERSION:=rk-2022.04.28
PKG_RELEASE:=$(AUTORELEASE)

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://gitlab.com/firefly-linux/u-boot.git
PKG_SOURCE_VERSION:=fc354ea827411c1cc35ddf76162aed02f7a9c7d5
#PKG_MIRROR_HASH:=c6e3cf815d9e32c053ccbccca05aedab9085f424f1652706618a6d792b8a850a
PKG_MIRROR_HASH:=skip

PKG_MAINTAINER:=Dirk Chang <dirk@kooiot.com>

include $(INCLUDE_DIR)/u-boot.mk
include $(INCLUDE_DIR)/package.mk

define U-Boot/Default
  BUILD_TARGET:=rockchip
  UENV:=default
  HIDDEN:=1
endef

## RK3568 boards

define U-Boot/tlink-r4x-1
  BUILD_SUBTARGET:=armv8
  NAME:=ThingsLink RK3568
  BUILD_DEVICES:= \
    kooiot_tlink-r4x-1
  DEPENDS:=+PACKAGE_u-boot-tlink-r4x-1:arm-trusted-firmware-rkbin
  PKG_BUILD_DEPENDS:=arm-trusted-firmware-rkbin
  ATF:=rk3568_bl31.elf
  TEE_FILE:=rk3568_bl32.bin
  DDR_FILE:=rk3568_ddr_1560MHz.bin
  DDR_LOW_FILE:=rk3568_ddr_528MHz.bin
  UBOOT_SOC_PLAT:=rk3568
endef

define U-Boot/tlink-r7-1
  BUILD_SUBTARGET:=armv8
  NAME:=ThingsLink RK3568
  BUILD_DEVICES:= \
    kooiot_tlink-r7-1
  DEPENDS:=+PACKAGE_u-boot-tlink-r7:arm-trusted-firmware-rkbin
  PKG_BUILD_DEPENDS:=arm-trusted-firmware-rkbin
  ATF:=rk3568_bl31.elf
  TEE_FILE:=rk3568_bl32.bin
  DDR_FILE:=rk3568_ddr_1560MHz.bin
  DDR_LOW_FILE:=rk3568_ddr_528MHz.bin
  UBOOT_SOC_PLAT:=rk3568
endef

define U-Boot/firefly-rk3568
  BUILD_SUBTARGET:=armv8
  NAME:=Firefly RK3568
  BUILD_DEVICES:= \
    firefly_firefly-roc-pc
  DEPENDS:=+PACKAGE_u-boot-firefly-rk3568:arm-trusted-firmware-rkbin
  PKG_BUILD_DEPENDS:=arm-trusted-firmware-rkbin
  ATF:=rk3568_bl31.elf
  TEE_FILE:=rk3568_bl32.bin
  DDR_FILE:=rk3568_ddr_1560MHz.bin
  UBOOT_SOC_PLAT:=rk3568
endef

UBOOT_TARGETS := \
  tlink-r4x-1 \
  tlink-r7-1 \
  firefly-rk3568

UBOOT_CONFIGURE_VARS += USE_PRIVATE_LIBGCC=yes

UBOOT_MAKE_FLAGS += \
  BL31=$(STAGING_DIR_IMAGE)/$(ATF)

define Build/Configure
	$(call Build/Configure/U-Boot)

	$(SED) 's#CONFIG_MKIMAGE_DTC_PATH=.*#CONFIG_MKIMAGE_DTC_PATH="$(PKG_BUILD_DIR)/scripts/dtc/dtc"#g' $(PKG_BUILD_DIR)/.config
	echo 'CONFIG_IDENT_STRING=" OpenWrt"' >> $(PKG_BUILD_DIR)/.config

	rm -f $(PKG_BUILD_DIR)/bl31.elf
	$(CP) $(STAGING_DIR_IMAGE)/$(ATF) $(PKG_BUILD_DIR)/bl31.elf
	rm -f $(PKG_BUILD_DIR)/tee.bin
	$(CP) $(STAGING_DIR_IMAGE)/$(TEE_FILE) $(PKG_BUILD_DIR)/tee.bin

	$(CP) -f ./py_files/make_fit_atf.py $(PKG_BUILD_DIR)/arch/arm/mach-rockchip/
	$(CP) -f ./py_files/decode_bl31.py $(PKG_BUILD_DIR)/arch/arm/mach-rockchip/
endef

define Build/InstallDev
	$(INSTALL_DIR) $(STAGING_DIR_IMAGE)

	$(if $(strip $(DDR_LOW_FILE)),
		$(CP) $(PKG_BUILD_DIR)/idbloader_low.img $(STAGING_DIR_IMAGE)/$(BUILD_VARIANT)-idbloader_low.img)

	$(CP) $(PKG_BUILD_DIR)/idbloader.img $(STAGING_DIR_IMAGE)/$(BUILD_VARIANT)-idbloader.img
	$(CP) $(PKG_BUILD_DIR)/u-boot.itb $(STAGING_DIR_IMAGE)/$(BUILD_VARIANT)-u-boot.itb
	$(CP) $(PKG_BUILD_DIR)/boot.scr $(STAGING_DIR_IMAGE)/$(BUILD_VARIANT)-boot.scr
endef

define Build/AA_Compile
	cd $(PKG_BUILD_DIR) &&./make.sh tlink-rk3568 --bl31 "$(STAGING_DIR_IMAGE)/$(ATF)" CROSS_COMPILE="$(TOOLCHAIN_DIR)/bin/$(TARGET_CROSS)"
	cd $(PKG_BUILD_DIR) && $(PKG_BUILD_DIR)/make.sh trust 
	cd $(PKG_BUILD_DIR) && $(PKG_BUILD_DIR)/make.sh loader
	cd $(PKG_BUILD_DIR) && $(PKG_BUILD_DIR)/make.sh --spl

endef

define Build/idbloader-image
	+$(MAKE) $(PKG_JOBS) -C $(PKG_BUILD_DIR) \
		CROSS_COMPILE=$(TARGET_CROSS) \
		$(UBOOT_MAKE_FLAGS) \
		u-boot.itb

	$(PKG_BUILD_DIR)/tools/mkimage -n $(UBOOT_SOC_PLAT) -T rksd \
		-d  $(STAGING_DIR_IMAGE)/$(DDR_FILE):$(PKG_BUILD_DIR)/spl/u-boot-spl.bin $(PKG_BUILD_DIR)/idbloader.img

	$(if $(strip $(DDR_LOW_FILE)),
		$(PKG_BUILD_DIR)/tools/mkimage -n $(UBOOT_SOC_PLAT) -T rksd \
			-d  $(STAGING_DIR_IMAGE)/$(DDR_LOW_FILE):$(PKG_BUILD_DIR)/spl/u-boot-spl.bin $(PKG_BUILD_DIR)/idbloader_low.img)
endef

define Build/build-bootscript
	$(PKG_BUILD_DIR)/tools/mkimage -A arm -O linux -T script -C none -a 0 -e 0 -d ./mmc.bootscript $(PKG_BUILD_DIR)/boot.scr
endef


define Build/Compile
	$(call Build/Compile/U-Boot)
	$(call Build/idbloader-image)
	$(call Build/build-bootscript)
endef

define Package/u-boot/install/default
endef

$(eval $(call BuildPackage/U-Boot))
