# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2013-2016 OpenWrt.org

define KernelPackage/mfd-ac100
    SUBMENU:=$(OTHER_MENU)
    TITLE:=X-Powers AC100 MFD support
    DEPENDS:=@TARGET_sunxi
    KCONFIG:= \
	CONFIG_MFD_AC100
    FILES:=$(LINUX_DIR)/drivers/mfd/ac100.ko
    AUTOLOAD:=$(call AutoLoad,50,ac100)
endef

define KernelPackage/mfd-ac100/description
 Support for the X-Powers AC100 RTC/audio chip
endef

$(eval $(call KernelPackage,mfd-ac100))

define KernelPackage/rtc-ac100
    SUBMENU:=$(OTHER_MENU)
    TITLE:=X-Powers AC100 RTC support
    DEPENDS:=@TARGET_sunxi +kmod-mfd-ac100
    $(call AddDepends/rtc)
    KCONFIG:= \
	CONFIG_RTC_DRV_AC100 \
	CONFIG_RTC_CLASS=y
    FILES:=$(LINUX_DIR)/drivers/rtc/rtc-ac100.ko
    AUTOLOAD:=$(call AutoLoad,50,rtc-ac100)
endef

define KernelPackage/rtc-ac100/description
 Support for the X-Powers AC100 RTC
endef

$(eval $(call KernelPackage,rtc-ac100))

define KernelPackage/rtc-sunxi
    SUBMENU:=$(OTHER_MENU)
    TITLE:=Sunxi SoC built-in RTC support
    DEPENDS:=@(TARGET_sunxi&&RTC_SUPPORT)
    KCONFIG:= \
	CONFIG_RTC_DRV_SUNXI \
	CONFIG_RTC_CLASS=y
    FILES:=$(LINUX_DIR)/drivers/rtc/rtc-sunxi.ko
    AUTOLOAD:=$(call AutoLoad,50,rtc-sunxi)
endef

define KernelPackage/rtc-sunxi/description
 Support for the AllWinner sunXi SoC's onboard RTC
endef

$(eval $(call KernelPackage,rtc-sunxi))

define KernelPackage/sunxi-ir
    SUBMENU:=$(OTHER_MENU)
    TITLE:=Sunxi SoC built-in IR support
    DEPENDS:=@(TARGET_sunxi&&RTC_SUPPORT) +kmod-input-core
    KCONFIG:= \
	CONFIG_MEDIA_SUPPORT=y \
	CONFIG_MEDIA_RC_SUPPORT=y \
	CONFIG_RC_DEVICES=y \
	CONFIG_RC_CORE=y \
	CONFIG_IR_SUNXI
    FILES:=$(LINUX_DIR)/drivers/media/rc/sunxi-cir.ko
    AUTOLOAD:=$(call AutoLoad,50,sunxi-cir)
endef

define KernelPackage/sunxi-ir/description
 Support for the AllWinner sunXi SoC's onboard IR
endef

$(eval $(call KernelPackage,sunxi-ir))

define KernelPackage/ata-sunxi
    TITLE:=AllWinner sunXi AHCI SATA support
    SUBMENU:=$(BLOCK_MENU)
    DEPENDS:=@TARGET_sunxi +kmod-ata-ahci-platform +kmod-scsi-core
    KCONFIG:=CONFIG_AHCI_SUNXI
    FILES:=$(LINUX_DIR)/drivers/ata/ahci_sunxi.ko
    AUTOLOAD:=$(call AutoLoad,41,ahci_sunxi,1)
endef

define KernelPackage/ata-sunxi/description
 SATA support for the AllWinner sunXi SoC's onboard AHCI SATA
endef

$(eval $(call KernelPackage,ata-sunxi))

define KernelPackage/sun4i-emac
  SUBMENU:=$(NETWORK_DEVICES_MENU)
  TITLE:=AllWinner EMAC Ethernet support
  DEPENDS:=@TARGET_sunxi +kmod-of-mdio +kmod-libphy
  KCONFIG:=CONFIG_SUN4I_EMAC
  FILES:=$(LINUX_DIR)/drivers/net/ethernet/allwinner/sun4i-emac.ko
  AUTOLOAD:=$(call AutoLoad,50,sun4i-emac,1)
endef

$(eval $(call KernelPackage,sun4i-emac))

define KernelPackage/sound-soc-sunxi
  TITLE:=AllWinner built-in SoC sound support
  KCONFIG:=CONFIG_SND_SUN4I_CODEC
  FILES:=$(LINUX_DIR)/sound/soc/sunxi/sun4i-codec.ko
  AUTOLOAD:=$(call AutoLoad,65,sun4i-codec)
  DEPENDS:=@TARGET_sunxi +kmod-sound-soc-core
  $(call AddDepends/sound)
endef

define KernelPackage/sound-soc-sunxi/description
  Kernel support for AllWinner built-in SoC audio
endef

$(eval $(call KernelPackage,sound-soc-sunxi))

define KernelPackage/sound-soc-sunxi-spdif
  TITLE:=Allwinner A10 SPDIF Support
  KCONFIG:=CONFIG_SND_SUN4I_SPDIF
  FILES:=$(LINUX_DIR)/sound/soc/sunxi/sun4i-spdif.ko
  AUTOLOAD:=$(call AutoLoad,65,sun4i-spdif)
  DEPENDS:=@TARGET_sunxi +kmod-sound-soc-spdif
  $(call AddDepends/sound)
endef

define KernelPackage/sound-soc-sunxi-spdif/description
  Kernel support for Allwinner A10 SPDIF Support
endef

$(eval $(call KernelPackage,sound-soc-sunxi-spdif))

define KernelPackage/usb-otg-sunxi
  TITLE:=AllWinner built-in SoC USB OTG support
  KCONFIG:= \
	  CONFIG_USB_OTG=y \
	  CONFIG_NOP_USB_XCEIV \
	  CONFIG_USB_MUSB_HDRC \
	  CONFIG_USB_MUSB_DUAL_ROLE=y \
	  CONFIG_USB_MUSB_SUNXI \
	  CONFIG_MUSB_PIO_ONLY=y
  FILES:= \
	  $(LINUX_DIR)/drivers/usb/phy/phy-generic.ko \
	  $(LINUX_DIR)/drivers/usb/musb/musb_hdrc.ko \
	  $(LINUX_DIR)/drivers/usb/musb/sunxi.ko
  AUTOLOAD:=$(call AutoLoad,50,sunxi,1)
  DEPENDS:=@TARGET_sunxi
  $(call AddDepends/usb)
endef

define KernelPackage/usb-otg-sunxi/description
  Kernel support for AllWinner built-in SoC USB OTG
endef

$(eval $(call KernelPackage,usb-otg-sunxi))

define KernelPackage/input-touchscreen-goodix
  SUBMENU:=$(INPUT_MODULES_MENU)
  TITLE:=Goodix I2C touchscreen
  KCONFIG:=CONFIG_TOUCHSCREEN_GOODIX
  DEPENDS:=+kmod-input-evdev +kmod-i2c-core
  FILES:= \
    $(LINUX_DIR)/drivers/input/touchscreen/goodix.ko@lt6.1 \
    $(LINUX_DIR)/drivers/input/touchscreen/goodix_ts.ko@ge6.1 \
    $(LINUX_DIR)/drivers/input/touchscreen/of_touchscreen.ko@lt5.13
  AUTOLOAD:=$(call AutoProbe,goodix)
endef

define KernelPackage/input-touchscreen-goodix/description
 Kernel modules for Goodix I2C touchscreen driver
endef

$(eval $(call KernelPackage,input-touchscreen-goodix))

define KernelPackage/usb-hub-251xb
  TITLE:=Support for USB 251XB Hub chips
  KCONFIG:=CONFIG_USB_HUB_USB251XB
  DEPENDS:=+kmod-i2c-core
  FILES:=$(LINUX_DIR)/drivers/usb/misc/usb251xb.ko
  AUTOLOAD:=$(call AutoLoad,54,usb251xb.ko,1)
  $(call AddDepends/usb)
endef

define KernelPackage/usb-hub-251xb/description
  USB HUB 251XB chips
endef

$(eval $(call KernelPackage,usb-hub-251xb))


