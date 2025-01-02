# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2013-2019 OpenWrt.org
# Copyright (C) 2016 Yousong Zhou

KERNEL_LOADADDR:=0x40008000

define Device/cubietech_cubieboard2
  DEVICE_VENDOR := Cubietech
  DEVICE_MODEL := Cubieboard2
  DEVICE_PACKAGES:=kmod-ata-sunxi kmod-sun4i-emac kmod-rtc-sunxi
  SOC := sun7i-a20
endef
TARGET_DEVICES += cubietech_cubieboard2

define Device/cubietech_cubietruck
  DEVICE_VENDOR := Cubietech
  DEVICE_MODEL := Cubietruck
  DEVICE_PACKAGES:=kmod-ata-sunxi kmod-rtc-sunxi kmod-brcmfmac
  SOC := sun7i-a20
endef
TARGET_DEVICES += cubietech_cubietruck

define Device/friendlyarm_nanopi-m1-plus
  DEVICE_VENDOR := FriendlyARM
  DEVICE_MODEL := NanoPi M1 Plus
  DEVICE_PACKAGES:=kmod-leds-gpio kmod-brcmfmac \
	cypress-firmware-43430-sdio wpad-basic-mbedtls
  SOC := sun8i-h3
endef
TARGET_DEVICES += friendlyarm_nanopi-m1-plus

define Device/friendlyarm_nanopi-neo
  DEVICE_VENDOR := FriendlyARM
  DEVICE_MODEL := NanoPi NEO
  SOC := sun8i-h3
endef
TARGET_DEVICES += friendlyarm_nanopi-neo

define Device/friendlyarm_nanopi-neo-air
  DEVICE_VENDOR := FriendlyARM
  DEVICE_MODEL := NanoPi NEO Air
  DEVICE_PACKAGES := kmod-leds-gpio kmod-brcmfmac \
	brcmfmac-firmware-43430a0-sdio wpad-basic-mbedtls
  SOC := sun8i-h3
endef
TARGET_DEVICES += friendlyarm_nanopi-neo-air

define Device/friendlyarm_nanopi-r1
  DEVICE_VENDOR := FriendlyARM
  DEVICE_MODEL := NanoPi R1
  DEVICE_PACKAGES := kmod-usb-net-rtl8152 kmod-leds-gpio \
	kmod-brcmfmac cypress-firmware-43430-sdio wpad-basic-mbedtls
  SOC := sun8i-h3
endef
TARGET_DEVICES += friendlyarm_nanopi-r1

define Device/friendlyarm_zeropi
  DEVICE_VENDOR := FriendlyARM
  DEVICE_MODEL := ZeroPi
  DEVICE_PACKAGES := kmod-rtc-sunxi
  SOC := sun8i-h3
endef
TARGET_DEVICES += friendlyarm_zeropi

define Device/lamobo_lamobo-r1
  DEVICE_VENDOR := Lamobo
  DEVICE_MODEL := Lamobo R1
  DEVICE_ALT0_VENDOR := Bananapi
  DEVICE_ALT0_MODEL := BPi-R1
  DEVICE_PACKAGES := kmod-ata-sunxi kmod-rtl8192cu wpad-basic-mbedtls
  DEVICE_COMPAT_VERSION := 1.1
  DEVICE_COMPAT_MESSAGE := Config cannot be migrated from swconfig to DSA
  SOC := sun7i-a20
endef
TARGET_DEVICES += lamobo_lamobo-r1

define Device/lemaker_bananapi
  DEVICE_VENDOR := LeMaker
  DEVICE_MODEL := Banana Pi
  DEVICE_PACKAGES:=kmod-rtc-sunxi kmod-ata-sunxi
  SOC := sun7i-a20
endef
TARGET_DEVICES += lemaker_bananapi

define Device/sinovoip_bananapi-m2-berry
  DEVICE_VENDOR := Sinovoip
  DEVICE_MODEL := Banana Pi M2 Berry
  DEVICE_PACKAGES:=kmod-ata-sunxi kmod-brcmfmac \
	cypress-firmware-43430-sdio wpad-basic-mbedtls
  SUPPORTED_DEVICES:=lemaker,bananapi-m2-berry
  SOC := sun8i-v40
endef
TARGET_DEVICES += sinovoip_bananapi-m2-berry

define Device/sinovoip_bananapi-m2-ultra
  DEVICE_VENDOR := Sinovoip
  DEVICE_MODEL := Banana Pi M2 Ultra
  DEVICE_PACKAGES:=kmod-ata-sunxi kmod-brcmfmac \
	brcmfmac-firmware-43430a0-sdio wpad-basic-mbedtls
  SUPPORTED_DEVICES:=lemaker,bananapi-m2-ultra
  SOC := sun8i-r40
endef
TARGET_DEVICES += sinovoip_bananapi-m2-ultra

define Device/lemaker_bananapro
  DEVICE_VENDOR := LeMaker
  DEVICE_MODEL := Banana Pro
  DEVICE_PACKAGES:=kmod-rtc-sunxi kmod-ata-sunxi kmod-brcmfmac \
	cypress-firmware-43362-sdio wpad-basic-mbedtls
  SOC := sun7i-a20
endef
TARGET_DEVICES += lemaker_bananapro

define Device/licheepi_licheepi-zero-dock
  DEVICE_VENDOR := LicheePi
  DEVICE_MODEL := Zero with Dock (V3s)
  DEVICE_PACKAGES:=kmod-rtc-sunxi
  SOC := sun8i-v3s
endef
TARGET_DEVICES += licheepi_licheepi-zero-dock

define Device/linksprite_pcduino3
  DEVICE_VENDOR := LinkSprite
  DEVICE_MODEL := pcDuino3
  DEVICE_PACKAGES:=kmod-sun4i-emac kmod-rtc-sunxi kmod-ata-sunxi kmod-rtl8xxxu \
	rtl8188eu-firmware
  SOC := sun7i-a20
endef
TARGET_DEVICES += linksprite_pcduino3

define Device/linksprite_pcduino3-nano
  DEVICE_VENDOR := LinkSprite
  DEVICE_MODEL := pcDuino3 Nano
  DEVICE_PACKAGES:=kmod-rtc-sunxi kmod-ata-sunxi
  SOC := sun7i-a20
endef
TARGET_DEVICES += linksprite_pcduino3-nano

define Device/mele_m9
  DEVICE_VENDOR := Mele
  DEVICE_MODEL := M9
  DEVICE_PACKAGES:=kmod-sun4i-emac kmod-rtl8192cu
  SOC := sun6i-a31
endef
TARGET_DEVICES += mele_m9

define Device/olimex_a20-olinuxino-lime
  DEVICE_VENDOR := Olimex
  DEVICE_MODEL := A20-OLinuXino-LIME
  DEVICE_PACKAGES:=kmod-ata-sunxi kmod-rtc-sunxi
  SOC := sun7i
endef
TARGET_DEVICES += olimex_a20-olinuxino-lime

define Device/olimex_a20-olinuxino-lime2
  DEVICE_VENDOR := Olimex
  DEVICE_MODEL := A20-OLinuXino-LIME2
  DEVICE_PACKAGES:=kmod-ata-sunxi kmod-rtc-sunxi kmod-usb-hid
  SOC := sun7i
endef
TARGET_DEVICES += olimex_a20-olinuxino-lime2

define Device/olimex_a20-olinuxino-lime2-emmc
  DEVICE_VENDOR := Olimex
  DEVICE_MODEL := A20-OLinuXino-LIME2
  DEVICE_VARIANT := eMMC
  DEVICE_PACKAGES:=kmod-ata-sunxi kmod-rtc-sunxi kmod-usb-hid
  SOC := sun7i
endef
TARGET_DEVICES += olimex_a20-olinuxino-lime2-emmc

define Device/olimex_a20-olinuxino-micro
  DEVICE_VENDOR := Olimex
  DEVICE_MODEL := A20-OLinuXino-MICRO
  DEVICE_PACKAGES:=kmod-ata-sunxi kmod-sun4i-emac kmod-rtc-sunxi
  SOC := sun7i
endef
TARGET_DEVICES += olimex_a20-olinuxino-micro

define Device/sinovoip_bananapi-m2-plus
  DEVICE_VENDOR := Sinovoip
  DEVICE_MODEL := Banana Pi M2+
  DEVICE_PACKAGES:=kmod-leds-gpio kmod-brcmfmac \
	brcmfmac-firmware-43430a0-sdio wpad-basic-mbedtls
  SOC := sun8i-h3
endef
TARGET_DEVICES += sinovoip_bananapi-m2-plus

define Device/sinovoip_bananapi-m3
  DEVICE_VENDOR := Sinovoip
  DEVICE_MODEL := Banana Pi M3
  DEVICE_PACKAGES:=kmod-rtc-sunxi kmod-leds-gpio kmod-rtc-ac100 \
	kmod-brcmfmac cypress-firmware-43430-sdio wpad-basic-mbedtls
  SOC := sun8i-a83t
endef
TARGET_DEVICES += sinovoip_bananapi-m3

define Device/sinovoip_bananapi-p2-zero
  DEVICE_VENDOR := Sinovoip
  DEVICE_MODEL := Banana Pi P2 Zero
  DEVICE_PACKAGES:=kmod-leds-gpio kmod-brcmfmac \
	cypress-firmware-43430-sdio wpad-basic-mbedtls
  SOC := sun8i-h2-plus
endef
TARGET_DEVICES += sinovoip_bananapi-p2-zero

define Device/xunlong_orangepi-one
  DEVICE_VENDOR := Xunlong
  DEVICE_MODEL := Orange Pi One
  DEVICE_PACKAGES:=kmod-rtc-sunxi
  SOC := sun8i-h3
endef
TARGET_DEVICES += xunlong_orangepi-one

define Device/xunlong_orangepi-pc
  DEVICE_VENDOR := Xunlong
  DEVICE_MODEL := Orange Pi PC
  DEVICE_PACKAGES:=kmod-gpio-button-hotplug
  SOC := sun8i-h3
endef
TARGET_DEVICES += xunlong_orangepi-pc

define Device/xunlong_orangepi-pc-plus
  DEVICE_VENDOR := Xunlong
  DEVICE_MODEL := Orange Pi PC Plus
  DEVICE_PACKAGES:=kmod-gpio-button-hotplug
  SOC := sun8i-h3
endef
TARGET_DEVICES += xunlong_orangepi-pc-plus

define Device/xunlong_orangepi-plus
  DEVICE_VENDOR := Xunlong
  DEVICE_MODEL := Orange Pi Plus
  DEVICE_PACKAGES:=kmod-rtc-sunxi
  SOC := sun8i-h3
endef
TARGET_DEVICES += xunlong_orangepi-plus

define Device/xunlong_orangepi-r1
  DEVICE_VENDOR := Xunlong
  DEVICE_MODEL := Orange Pi R1
  DEVICE_PACKAGES:=kmod-usb-net-rtl8152
  SOC := sun8i-h2-plus
endef
TARGET_DEVICES += xunlong_orangepi-r1

define Device/xunlong_orangepi-zero
  DEVICE_VENDOR := Xunlong
  DEVICE_MODEL := Orange Pi Zero
  DEVICE_PACKAGES:=kmod-rtc-sunxi
  SOC := sun8i-h2-plus
endef
TARGET_DEVICES += xunlong_orangepi-zero

define Device/xunlong_orangepi-2
  DEVICE_VENDOR := Xunlong
  DEVICE_MODEL := Orange Pi 2
  DEVICE_PACKAGES:=kmod-rtc-sunxi
  SOC := sun8i-h3
 endef
TARGET_DEVICES += xunlong_orangepi-2

define Device/sipeed_lichee-zero-plus
  DEVICE_VENDOR := SiPEED
  DEVICE_MODEL := SiPEED lichee zero plus
  DEVICE_PACKAGES:=kmod-rtc-sunxi
  SOC := sun8i-s3
endef
TARGET_DEVICES += sipeed_lichee-zero-plus

define Device/kooiot_tlink-s1
  DEVICE_VENDOR := KooIoT
  DEVICE_MODEL := ThingsLink S1
  DEVICE_PACKAGES:=kmod-rtc-sunxi kmod-gpio-button-hotplug \
    kmod-leds-gpio kmod-ledtrig-heartbeat \
    kmod-ledtrig-netdev kmod-ledtrig-gpio \
    kmod-rtc-sd3078 kmod-usb-net-asix \
    kmod-usb-serial kmod-usb-serial-option \
    kmod-usb-serial-qualcomm kmod-usb-net-qmi-wwan \
	luci-app-freeioe luci-proto-qmi luci-proto-3g \
    kmod-xradio melsem-xr819-firmware wpad-basic-wolfssl \
    tinc-freeioe-tunnel iperf3 \
    uqmi fdisk usbutils freeioe wwanleds \
    blockd usb-otg-sunxi kmod-eeprom-at24 fdisk
  SOC := sun8i-s3
endef
TARGET_DEVICES += kooiot_tlink-s1

define Device/sinlinx_sinlinx-sin-v3s
  DEVICE_VENDOR := SINLINX
  DEVICE_MODEL := SINLINX Sin V3s
  DEVICE_PACKAGES:=kmod-rtc-sunxi
  SOC := sun8i-v3s
endef
TARGET_DEVICES += sinlinx_sinlinx-sin-v3s

define Device/kooiot_tlink-x1
  DEVICE_VENDOR := KooIoT
  DEVICE_MODEL := ThingsLink X1
  DEVICE_PACKAGES:=kmod-rtc-sunxi kmod-gpio-button-hotplug \
    kmod-usb-net kmod-usb-net-rtl8152 \
    kmod-leds-gpio kmod-ledtrig-heartbeat \
    kmod-ledtrig-netdev kmod-ledtrig-gpio \
    kmod-rtc-rx8010 kmod-usb-net-asix \
    kmod-usb-serial kmod-usb-serial-option \
    kmod-usb-serial-qualcomm kmod-usb-net-qmi-wwan \
    uqmi fdisk usbutils freeioe wwanleds \
    luci-app-freeioe luci-proto-qmi luci-proto-3g \
    tinc-freeioe-tunnel ser2net shellinabox iperf3 ethtool \
    kmod-eeprom-at24 fdisk blockd
  SOC := sun8i-h3
endef
TARGET_DEVICES += kooiot_tlink-x1

define Device/kooiot_tlink-x1s
  DEVICE_VENDOR := KooIoT
  DEVICE_MODEL := ThingsLink X1s
  DEVICE_PACKAGES:=kmod-rtc-sunxi kmod-gpio-button-hotplug \
    kmod-usb-net kmod-usb-net-rtl8152 \
    kmod-leds-gpio kmod-ledtrig-heartbeat \
    kmod-ledtrig-netdev kmod-ledtrig-gpio \
    kmod-rtc-sd3078 kmod-usb-net-asix \
    kmod-usb-serial kmod-usb-serial-option \
    kmod-usb-serial-qualcomm kmod-usb-net-qmi-wwan \
    uqmi fdisk usbutils freeioe wwanleds \
    luci-app-freeioe luci-proto-qmi luci-proto-3g \
    tinc-freeioe-tunnel ser2net shellinabox iperf3 ethtool \
    kmod-eeprom-at24 fdisk
  SOC := sun8i-h3
endef
TARGET_DEVICES += kooiot_tlink-x1s

define Device/kooiot_tlink-x3
  DEVICE_VENDOR := KooIoT
  DEVICE_MODEL := ThingsLink X3
  DEVICE_PACKAGES:=kmod-rtc-sunxi kmod-gpio-button-hotplug \
    kmod-usb-net kmod-usb-net-asix \
    kmod-leds-gpio kmod-ledtrig-heartbeat \
    kmod-ledtrig-netdev kmod-ledtrig-gpio \
    kmod-usb-serial kmod-usb-serial-option \
    kmod-usb-serial-qualcomm kmod-usb-net-qmi-wwan \
    uqmi fdisk usbutils freeioe wwanleds \
    luci-app-freeioe luci-proto-qmi luci-proto-3g \
    tinc-freeioe-tunnel iperf3 ethtool \
    kmod-rtc-sd3078 kmod-eeprom-at24 fdisk
  SOC := sun8i-x3
endef
TARGET_DEVICES += kooiot_tlink-x3

define Device/kooiot_tlink-r1
  DEVICE_VENDOR := KooIoT
  DEVICE_MODEL := ThingsLink R1
  DEVICE_PACKAGES:=kmod-rtc-sunxi kmod-gpio-button-hotplug \
    kmod-usb-net kmod-usb-net-rtl8152 \
    kmod-leds-gpio kmod-ledtrig-heartbeat \
    kmod-ledtrig-netdev kmod-ledtrig-gpio \
    kmod-rtc-hym8563 kmod-usb-net-asix \
    kmod-net-rtl8723be fdisk
  SOC := sun8i-h3
endef
TARGET_DEVICES += kooiot_tlink-r1

define Device/kooiot_tlink-ok-a40i
  DEVICE_VENDOR := KooIoT
  DEVICE_MODEL := ThingsLink OK-A40i
  DEVICE_PACKAGES:=kmod-rtc-sunxi kmod-ata-sunxi kmod-brcmfmac \
    kmod-sun4i-emac kmod-rtc-rx8010 \
    brcmfmac-firmware-43430a0-sdio wpad-basic-wolfssl \
    kmod-usb2 kmod-usb-ohci kmod-gpio-button-hotplug \
    kmod-leds-gpio kmod-ledtrig-heartbeat \
    kmod-ledtrig-netdev kmod-ledtrig-gpio \
    kmod-usb-serial kmod-usb-serial-option \
    kmod-usb-serial-qualcomm kmod-usb-net-qmi-wwan \
    uqmi fdisk usbutils freeioe wwanleds \
    luci-app-freeioe luci-proto-qmi luci-proto-3g \
    tinc-freeioe-tunnel ser2net shellinabox iperf3 ethtool \
    blockd kmod-usb-storage kmod-drm-sunxi \
	usb-otg-sunxi kmod-usb-gadget-serial
  SOC := sun8i-r40
endef
TARGET_DEVICES += kooiot_tlink-ok-a40i

define Device/kooiot_tlink-dj-a40i-e
  DEVICE_VENDOR := KooIoT
  DEVICE_MODEL := ThingsLink DJ-A40i-E
  DEVICE_PACKAGES:=kmod-rtc-sunxi kmod-gpio-button-hotplug \
    kmod-leds-gpio kmod-ledtrig-gpio \
    kmod-usb-net kmod-usb-net-rtl8152 \
    kmod-usb-serial kmod-usb-serial-option \
    kmod-usb-serial-qualcomm kmod-usb-net-qmi-wwan \
	kmod-usb-net-rndis kmod-usb-otg-sunxi \
    kmod-eeprom-at24 kmod-rtc-pcf8563 \
    kmod-usb-storage kmod-drm-sunxi \
	kmod-backlight kmod-drm-sun8i \
	kmod-drm-sun8i-dsi kmod-drm-sun8i-hdmi \
	kmod-rtl8xxxu rtl8723bu-firmware \
	kmod-input-core kmod-input-evdev \
	kmod-sound-soc-sunxi kmod-backlight-pwm \
	kmod-pwm-sun8i kmod-mmc usb-modeswitch \
	usb-otg-sunxi kmod-usb-gadget-serial \
    luci-app-freeioe luci-proto-qmi luci-proto-3g \
	luci-app-ddns luci-app-mosquitto luci-app-ser2net \
    tinc-freeioe-tunnel ser2net shellinabox iperf3 ethtool \
    uqmi umbim usbutils freeioe wwanleds \
	blockd siridb-server mosquitto-nossl \
	wpad-basic-wolfssl uclibcxx curl fdisk
  SOC := sun8i-r40
endef
TARGET_DEVICES += kooiot_tlink-dj-a40i-e

define Device/kooiot_tlink-dr4-a40i
  DEVICE_VENDOR := KooIoT
  DEVICE_MODEL := ThingsLink DR4-A40i
  DEVICE_PACKAGES:=kmod-rtc-sunxi kmod-gpio-button-hotplug \
    kmod-leds-gpio kmod-ledtrig-gpio \
    kmod-usb-net kmod-usb-net-rtl8152 \
    kmod-usb-serial kmod-usb-serial-option \
    kmod-usb-serial-qualcomm kmod-usb-net-qmi-wwan \
	kmod-usb-net-rndis kmod-usb-otg-sunxi \
    kmod-eeprom-at24 kmod-rtc-pcf8563 \
    kmod-usb-storage kmod-drm-sunxi \
	kmod-backlight kmod-drm-sun8i \
	kmod-drm-sun8i-dsi kmod-drm-sun8i-hdmi \
	kmod-rtl8xxxu rtl8723bu-firmware \
	kmod-input-core kmod-input-evdev \
	kmod-sound-soc-sunxi kmod-backlight-pwm \
	kmod-pwm-sun8i kmod-mmc usb-modeswitch \
	usb-otg-sunxi kmod-usb-gadget-serial \
    luci-app-freeioe luci-proto-qmi luci-proto-3g \
	luci-app-ddns luci-app-mosquitto luci-app-ser2net \
    tinc-freeioe-tunnel ser2net shellinabox iperf3 ethtool \
    uqmi umbim usbutils freeioe wwanleds \
	blockd siridb-server mosquitto-nossl \
	wpad-basic-wolfssl uclibcxx curl fdisk
  SOC := sun8i-r40
endef
TARGET_DEVICES += kooiot_tlink-dr4-a40i

define Device/kooiot_tlink-nano-a40i
  DEVICE_VENDOR := KooIoT
  DEVICE_MODEL := ThingsLink Nano-A40i
  DEVICE_PACKAGES:=kmod-rtc-sunxi kmod-gpio-button-hotplug \
    kmod-leds-gpio kmod-ledtrig-gpio \
    kmod-usb-net kmod-usb-net-rtl8152 \
    kmod-usb-serial kmod-usb-serial-option \
    kmod-usb-serial-qualcomm kmod-usb-net-qmi-wwan \
	kmod-usb-net-rndis kmod-usb-otg-sunxi \
    kmod-eeprom-at24 kmod-rtc-pcf8563 \
    kmod-usb-storage kmod-drm-sunxi \
	kmod-backlight kmod-drm-sun8i \
	kmod-drm-sun8i-dsi kmod-drm-sun8i-hdmi \
	kmod-rtl8xxxu rtl8723bu-firmware \
	kmod-input-core kmod-input-evdev \
	kmod-sound-soc-sunxi kmod-backlight-pwm \
	kmod-pwm-sun8i kmod-mmc usb-modeswitch \
	usb-otg-sunxi kmod-usb-gadget-serial \
    luci-app-freeioe luci-proto-qmi luci-proto-3g \
	luci-app-ddns luci-app-mosquitto luci-app-ser2net \
    tinc-freeioe-tunnel ser2net shellinabox iperf3 ethtool \
    uqmi umbim usbutils freeioe wwanleds \
	blockd siridb-server mosquitto-nossl \
	wpad-basic-wolfssl uclibcxx curl fdisk
  SOC := sun8i-r40
endef
TARGET_DEVICES += kooiot_tlink-nano-a40i

define Device/kooiot_tlink-qh-x40
  DEVICE_VENDOR := KooIoT
  DEVICE_MODEL := ThingsLink QH-X40
  DEVICE_PACKAGES:=kmod-rtc-sunxi kmod-gpio-button-hotplug \
    kmod-leds-gpio kmod-ledtrig-gpio \
    kmod-usb-net kmod-usb-net-rtl8152 \
    kmod-usb-serial kmod-usb-serial-option \
    kmod-usb-serial-qualcomm kmod-usb-net-qmi-wwan \
	kmod-usb-net-rndis kmod-usb-otg-sunxi \
    kmod-eeprom-at24 kmod-rtc-pcf8563 \
    kmod-usb-storage kmod-drm-sunxi \
	kmod-backlight kmod-drm-sun8i kmod-sun4i-emac \
	kmod-drm-sun8i-dsi kmod-drm-sun8i-hdmi \
	kmod-rtl8723bs rtl8723bs-firmware \
	kmod-input-core kmod-input-evdev \
	kmod-sound-soc-sunxi kmod-backlight-pwm \
	kmod-pwm-sun8i kmod-mmc usb-modeswitch \
	usb-otg-sunxi kmod-usb-gadget-serial \
    luci-app-freeioe luci-proto-qmi luci-proto-3g \
	luci-app-ddns luci-app-mosquitto luci-app-ser2net \
    tinc-freeioe-tunnel ser2net shellinabox iperf3 ethtool \
    uqmi umbim usbutils freeioe wwanleds \
	blockd siridb-server mosquitto-nossl \
	wpad-basic-wolfssl uclibcxx curl fdisk
  SOC := sun8i-r40
endef
TARGET_DEVICES += kooiot_tlink-qh-x40

define Device/kooiot_tlink-k1
  DEVICE_VENDOR := KooIoT
  DEVICE_MODEL := ThingsLink K1
  DEVICE_PACKAGES:=kmod-rtc-sunxi kmod-ata-sunxi \
    kmod-sun4i-emac kmod-rtc-rx8010 \
    kmod-xradio melsem-xr819-firmware wpad-basic-wolfssl \
    kmod-usb2 kmod-usb-ohci kmod-gpio-button-hotplug \
    kmod-leds-gpio kmod-ledtrig-heartbeat \
    kmod-ledtrig-netdev kmod-ledtrig-gpio \
    kmod-usb-serial kmod-usb-serial-option \
    kmod-usb-serial-qualcomm kmod-usb-net-qmi-wwan \
    kmod-serial-wk2xxx-spi kmod-eeprom-at24 \
	kmod-motorcomm-phy \
    uqmi fdisk usbutils freeioe wwanleds \
    luci-app-freeioe luci-proto-qmi luci-proto-3g \
    tinc-freeioe-tunnel ser2net shellinabox iperf3 ethtool \
    blockd kmod-usb-storage kmod-drm-sunxi \
    usb-otg-sunxi kmod-usb-gadget-serial
  SOC := sun8i-r40
endef
TARGET_DEVICES += kooiot_tlink-k1

define Device/kooiot_tlink-k2
  DEVICE_VENDOR := KooIoT
  DEVICE_MODEL := ThingsLink K2
  DEVICE_PACKAGES:=kmod-rtc-sunxi kmod-ata-sunxi \
    kmod-sun4i-emac kmod-rtc-sd3078 \
    kmod-xradio melsem-xr819-firmware wpad-basic-wolfssl \
    kmod-usb2 kmod-usb-ohci kmod-gpio-button-hotplug \
    kmod-leds-gpio kmod-ledtrig-heartbeat \
    kmod-ledtrig-netdev kmod-ledtrig-gpio \
    kmod-usb-serial kmod-usb-serial-option \
    kmod-usb-serial-qualcomm kmod-usb-net-qmi-wwan \
    kmod-usb-net-asix kmod-serial-wk2xxx-spi \
	kmod-motorcomm-phy \
    uqmi fdisk usbutils freeioe wwanleds \
    luci-app-freeioe luci-proto-qmi luci-proto-3g \
    tinc-freeioe-tunnel ser2net shellinabox iperf3 ethtool \
    blockd kmod-usb-storage kmod-drm-sunxi \
    usb-otg-sunxi kmod-usb-gadget-serial \
	kmod-eeprom-at24
  SOC := sun8i-r40
endef
TARGET_DEVICES += kooiot_tlink-k2

define Device/kooiot_tlink-k2x
  DEVICE_VENDOR := KooIoT
  DEVICE_MODEL := ThingsLink K2x
  DEVICE_PACKAGES:=kmod-rtc-sunxi kmod-ata-sunxi \
    kmod-sun4i-emac kmod-rtc-sd3078 \
    kmod-xradio melsem-xr819-firmware wpad-basic-wolfssl \
    kmod-usb2 kmod-usb-ohci kmod-gpio-button-hotplug \
    kmod-leds-gpio kmod-ledtrig-heartbeat \
    kmod-ledtrig-netdev kmod-ledtrig-gpio \
    kmod-usb-serial kmod-usb-serial-option \
    kmod-usb-serial-qualcomm kmod-usb-net-qmi-wwan \
    kmod-usb-net-asix kmod-serial-wk2xxx-spi \
	kmod-can kmod-can-mcp251x \
	kmod-can-bcm kmod-can-raw ip-full\
	kmod-motorcomm-phy \
    uqmi fdisk usbutils freeioe wwanleds \
	luci-proto-qmi luci-proto-3g luci-proto-ncm \
	luci-app-freeioe tinc-freeioe-tunnel \
	ser2net shellinabox iperf3 ethtool minicom \
    blockd kmod-usb-storage kmod-drm-sunxi \
    usb-otg-sunxi kmod-usb-gadget-serial \
	kmod-eeprom-at24
  SOC := sun8i-r40
endef
TARGET_DEVICES += kooiot_tlink-k2x


define Device/kooiot_tlink-k4a
  DEVICE_VENDOR := KooIoT
  DEVICE_MODEL := ThingsLink K4A
  DEVICE_PACKAGES:=kmod-rtc-sunxi kmod-ata-sunxi \
    kmod-sun4i-emac kmod-rtc-sd3078 \
    kmod-xradio melsem-xr819-firmware wpad-basic-wolfssl \
    kmod-usb2 kmod-usb-ohci kmod-gpio-button-hotplug \
    kmod-leds-gpio kmod-ledtrig-heartbeat \
    kmod-ledtrig-netdev kmod-ledtrig-gpio \
    kmod-usb-net kmod-usb-net-asix \
    kmod-usb-serial kmod-usb-serial-option \
    kmod-usb-serial-qualcomm kmod-usb-net-qmi-wwan \
    kmod-serial-ch9434 kmod-can kmod-can-mcp251x \
    kmod-can-bcm kmod-can-raw ip-full\
    kmod-motorcomm-yt8521s kmod-motorcomm-yt8512c \
    uqmi fdisk usbutils freeioe wwanleds \
    luci-proto-qmi luci-proto-3g luci-proto-ncm \
    luci-app-freeioe tinc-freeioe-tunnel\
    ser2net shellinabox iperf3 ethtool minicom \
    blockd kmod-usb-storage kmod-drm-sunxi \
    usb-otg-sunxi kmod-usb-gadget-serial \
    kmod-eeprom-at24
  SOC := sun8i-r40
endef
TARGET_DEVICES += kooiot_tlink-k4a


define Device/kooiot_tlink-k4g
  DEVICE_VENDOR := KooIoT
  DEVICE_MODEL := ThingsLink K4G
  DEVICE_PACKAGES:=kmod-rtc-sunxi kmod-ata-sunxi \
    kmod-sun4i-emac kmod-rtc-sd3078 \
    kmod-xradio melsem-xr819-firmware wpad-basic-wolfssl \
    kmod-usb2 kmod-usb-ohci kmod-gpio-button-hotplug \
    kmod-leds-gpio kmod-ledtrig-heartbeat \
    kmod-ledtrig-netdev kmod-ledtrig-gpio \
    kmod-usb-net kmod-usb-net-asix \
    kmod-usb-serial kmod-usb-serial-option \
    kmod-usb-serial-qualcomm kmod-usb-net-qmi-wwan \
    kmod-serial-ch9434 kmod-can kmod-can-mcp251x \
    kmod-can-bcm kmod-can-raw ip-full\
    kmod-motorcomm-yt8521s kmod-motorcomm-yt8512c \
    uqmi fdisk usbutils freeioe wwanleds \
    luci-proto-qmi luci-proto-3g luci-proto-ncm \
    luci-app-freeioe tinc-freeioe-tunnel\
    ser2net shellinabox iperf3 ethtool minicom \
    blockd kmod-usb-storage kmod-drm-sunxi \
    usb-otg-sunxi kmod-usb-gadget-serial \
    kmod-eeprom-at24
  SOC := sun8i-r40
endef
TARGET_DEVICES += kooiot_tlink-k4g


define Device/kooiot_tlink-m408
  DEVICE_VENDOR := KooIoT
  DEVICE_MODEL := ThingsLink M408
  DEVICE_PACKAGES:=kmod-rtc-sunxi kmod-ata-sunxi \
    kmod-sun4i-emac kmod-rtc-rx8025t \
    wpad-basic-wolfssl \
    kmod-usb2 kmod-usb-ohci kmod-gpio-button-hotplug \
    kmod-leds-gpio kmod-ledtrig-heartbeat \
    kmod-ledtrig-netdev kmod-ledtrig-gpio \
    kmod-usb-serial kmod-usb-serial-option \
    kmod-usb-serial-qualcomm kmod-usb-net-qmi-wwan \
	kmod-serial-xr14xx-usb kmod-can kmod-can-mcp251x \
	kmod-can-bcm kmod-can-raw ip-full\
    uqmi fdisk usbutils freeioe wwanleds \
    luci-app-freeioe luci-proto-qmi luci-proto-3g \
    tinc-freeioe-tunnel ser2net shellinabox iperf3 ethtool \
    blockd kmod-usb-storage kmod-drm-sunxi \
	usb-otg-sunxi kmod-usb-gadget-serial
  SOC := sun8i-t3
endef
TARGET_DEVICES += kooiot_tlink-m408

define Device/kooiot_tlink-m416
  DEVICE_VENDOR := KooIoT
  DEVICE_MODEL := ThingsLink M416
  DEVICE_PACKAGES:=kmod-rtc-sunxi kmod-ata-sunxi \
    kmod-sun4i-emac kmod-rtc-rx8025t \
    wpad-basic-wolfssl \
    kmod-usb2 kmod-usb-ohci kmod-gpio-button-hotplug \
    kmod-leds-gpio kmod-ledtrig-heartbeat \
    kmod-ledtrig-netdev kmod-ledtrig-gpio \
    kmod-usb-serial kmod-usb-serial-option \
    kmod-usb-serial-qualcomm kmod-usb-net-qmi-wwan \
	kmod-serial-xr14xx-usb kmod-can kmod-can-mcp251x \
	kmod-can-bcm kmod-can-raw ip-full\
    uqmi fdisk usbutils freeioe wwanleds \
    luci-app-freeioe luci-proto-qmi luci-proto-3g \
    tinc-freeioe-tunnel ser2net shellinabox iperf3 ethtool \
    blockd kmod-usb-storage kmod-drm-sunxi \
	usb-otg-sunxi kmod-usb-gadget-serial
  SOC := sun8i-t3
endef
TARGET_DEVICES += kooiot_tlink-m416

define Device/kooiot_tlink-rp-t113
  DEVICE_VENDOR := KooIoT
  DEVICE_MODEL := ThingsLink RP-T113
  DEVICE_PACKAGES:=kmod-rtc-sunxi \
    wpad-basic-mbedtls \
    kmod-usb2 kmod-usb-ohci kmod-gpio-button-hotplug \
    kmod-leds-gpio kmod-ledtrig-heartbeat \
    kmod-ledtrig-netdev kmod-ledtrig-gpio \
    kmod-usb-serial kmod-usb-serial-option \
    kmod-usb-serial-qualcomm kmod-usb-net-qmi-wwan \
	kmod-can-bcm kmod-can-raw ip-full\
    uqmi fdisk usbutils freeioe wwanleds \
    luci-app-freeioe luci-proto-qmi luci-proto-3g \
    tinc-freeioe-tunnel ser2net shellinabox iperf3 ethtool \
    blockd kmod-usb-storage \
	usb-otg-sunxi kmod-usb-gadget-serial
  SOC := sun8i-t113
endef
TARGET_DEVICES += kooiot_tlink-rp-t113

define Device/kooiot_tlink-e1-v0
  DEVICE_VENDOR := KooIoT
  DEVICE_MODEL := ThingsLink E1 (V0)
  DEVICE_PACKAGES:=kmod-rtc-sunxi \
    wpad-basic-mbedtls \
    kmod-usb2 kmod-usb-ohci kmod-gpio-button-hotplug \
    kmod-leds-gpio kmod-ledtrig-heartbeat \
    kmod-ledtrig-netdev kmod-ledtrig-gpio \
    kmod-usb-serial kmod-usb-serial-option \
    kmod-usb-serial-qualcomm kmod-usb-net-qmi-wwan \
	kmod-can-bcm kmod-can-raw ip-full\
    uqmi fdisk usbutils freeioe wwanleds \
    luci-app-freeioe luci-proto-qmi luci-proto-3g \
    tinc-freeioe-tunnel ser2net shellinabox iperf3 ethtool \
    blockd kmod-usb-storage \
	usb-otg-sunxi kmod-usb-gadget-serial
  SOC := sun8i-t113
endef
TARGET_DEVICES += kooiot_tlink-e1-v0

define Device/kooiot_tlink-e1
  DEVICE_VENDOR := KooIoT
  DEVICE_MODEL := ThingsLink E1
  DEVICE_PACKAGES:=kmod-rtc-sunxi \
    wpad-basic-mbedtls \
    kmod-usb2 kmod-usb-ohci kmod-gpio-button-hotplug \
    kmod-leds-gpio kmod-ledtrig-heartbeat \
    kmod-ledtrig-netdev kmod-ledtrig-gpio \
    kmod-usb-serial kmod-usb-serial-option \
    kmod-usb-serial-qualcomm kmod-usb-net-qmi-wwan \
	kmod-can-bcm kmod-can-raw ip-full\
    uqmi fdisk usbutils freeioe wwanleds \
    luci-app-freeioe luci-proto-qmi luci-proto-3g \
    tinc-freeioe-tunnel ser2net shellinabox iperf3 ethtool \
    blockd kmod-usb-storage \
	usb-otg-sunxi kmod-usb-gadget-serial
  SOC := sun8i-t113
endef
TARGET_DEVICES += kooiot_tlink-e1

define Device/kooiot_tlink-dly-e102
  DEVICE_VENDOR := KooIoT
  DEVICE_MODEL := ThingsLink DLY-E102
  DEVICE_PACKAGES:=kmod-rtc-sunxi \
    kmod-rtc-isl1208 wpad-basic-mbedtls \
    kmod-usb2 kmod-usb-ohci kmod-gpio-button-hotplug \
    kmod-leds-gpio kmod-ledtrig-heartbeat \
    kmod-ledtrig-netdev kmod-ledtrig-gpio \
    kmod-usb-serial kmod-usb-serial-option \
    kmod-usb-serial-qualcomm kmod-usb-net-qmi-wwan \
	kmod-can-bcm kmod-can-raw ip-full\
    uqmi fdisk usbutils freeioe wwanleds \
    luci-app-freeioe luci-proto-qmi luci-proto-3g \
    tinc-freeioe-tunnel ser2net shellinabox iperf3 ethtool \
    blockd kmod-usb-storage kmod-motorcomm-yt8521s \
	usb-otg-sunxi kmod-usb-gadget-serial
  SOC := sun8i-t113
  IMAGES := sdcard.img.gz
endef
TARGET_DEVICES += kooiot_tlink-dly-e102

define Device/kooiot_tlink-dly-e102-spinand
  $(Device/NAND)
  DEVICE_VENDOR := KooIoT
  DEVICE_MODEL := ThingsLink DLY-E102 (SPI NAND)
  DEVICE_PACKAGES:=kmod-rtc-sunxi \
    kmod-rtc-isl1208 wpad-basic-mbedtls \
    kmod-usb2 kmod-usb-ohci kmod-gpio-button-hotplug \
    kmod-leds-gpio kmod-ledtrig-heartbeat \
    kmod-ledtrig-netdev kmod-ledtrig-gpio \
    kmod-usb-serial kmod-usb-serial-option \
    kmod-usb-serial-qualcomm kmod-usb-net-qmi-wwan \
	kmod-can-bcm kmod-can-raw ip-full\
    uqmi fdisk usbutils freeioe wwanleds \
    luci-app-freeioe luci-proto-qmi luci-proto-3g \
    tinc-freeioe-tunnel ser2net shellinabox iperf3 ethtool \
    blockd kmod-usb-storage kmod-motorcomm-yt8521s \
	usb-otg-sunxi kmod-usb-gadget-serial
  SOC := sun8i-t113
endef
TARGET_DEVICES += kooiot_tlink-dly-e102-spinand

define Device/kooiot_tlink-dly-e204
  DEVICE_VENDOR := KooIoT
  DEVICE_MODEL := ThingsLink DLY-E204
  DEVICE_PACKAGES:=kmod-rtc-sunxi \
    kmod-rtc-isl1208 wpad-basic-mbedtls \
    kmod-usb-net-ch397 \
    kmod-usb2 kmod-usb-ohci kmod-gpio-button-hotplug \
    kmod-leds-gpio kmod-ledtrig-heartbeat \
    kmod-ledtrig-netdev kmod-ledtrig-gpio \
    kmod-usb-serial kmod-usb-serial-option \
    kmod-usb-serial-qualcomm kmod-usb-net-qmi-wwan \
	kmod-can-bcm kmod-can-raw ip-full\
    uqmi fdisk usbutils freeioe wwanleds \
    luci-app-freeioe luci-proto-qmi luci-proto-3g \
    tinc-freeioe-tunnel ser2net shellinabox iperf3 ethtool \
    blockd kmod-usb-storage kmod-motorcomm-yt8521s \
	usb-otg-sunxi kmod-usb-gadget-serial
  SOC := sun8i-t113
  IMAGES := sdcard.img.gz
endef
TARGET_DEVICES += kooiot_tlink-dly-e204

