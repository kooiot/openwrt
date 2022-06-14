# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2020 Tobias Maedel

define Device/friendlyarm_nanopi-r2s
  DEVICE_VENDOR := FriendlyARM
  DEVICE_MODEL := NanoPi R2S
  SOC := rk3328
  UBOOT_DEVICE_NAME := nanopi-r2s-rk3328
  IMAGE/sysupgrade.img.gz := boot-common | boot-script nanopi-r2s | pine64-img | gzip | append-metadata
  DEVICE_PACKAGES := kmod-usb-net-rtl8152
endef
TARGET_DEVICES += friendlyarm_nanopi-r2s

define Device/friendlyarm_nanopi-r4s
  DEVICE_VENDOR := FriendlyARM
  DEVICE_MODEL := NanoPi R4S
  DEVICE_VARIANT := 4GB LPDDR4
  SOC := rk3399
  UBOOT_DEVICE_NAME := nanopi-r4s-rk3399
  IMAGE/sysupgrade.img.gz := boot-common | boot-script nanopi-r4s | pine64-img | gzip | append-metadata
  DEVICE_PACKAGES := kmod-r8169
endef
TARGET_DEVICES += friendlyarm_nanopi-r4s

define Device/pine64_rockpro64
  DEVICE_VENDOR := Pine64
  DEVICE_MODEL := RockPro64
  SOC := rk3399
  UBOOT_DEVICE_NAME := rockpro64-rk3399
  IMAGE/sysupgrade.img.gz := boot-common | boot-script | pine64-img | gzip | append-metadata
endef
TARGET_DEVICES += pine64_rockpro64

define Device/radxa_rock-pi-4a
  DEVICE_VENDOR := Radxa
  DEVICE_MODEL := ROCK Pi 4A
  SOC := rk3399
  SUPPORTED_DEVICES := radxa,rockpi4a radxa,rockpi4
  UBOOT_DEVICE_NAME := rock-pi-4-rk3399
  IMAGE/sysupgrade.img.gz := boot-common | boot-script | pine64-img | gzip | append-metadata
endef
TARGET_DEVICES += radxa_rock-pi-4a

define Device/firefly_firefly-roc-pc
  DEVICE_VENDOR := Firefly
  DEVICE_MODEL := Roc PC (RK3568)
  SOC := rk3568
  UBOOT_DEVICE_NAME := firefly-rk3568
  IMAGE/sysupgrade.img.gz := boot-common | boot-script-rk | pine64-img | gzip | append-metadata
  DEVICE_PACKAGES:=kmod-gpio-button-hotplug \
    kmod-leds-gpio kmod-ledtrig-heartbeat \
    kmod-ledtrig-netdev kmod-ledtrig-gpio \
    kmod-rtc-hym8563 kmod-usb-net-asix \
    kmod-usb-serial kmod-usb-serial-option \
    kmod-usb-serial-qualcomm kmod-usb-net-qmi-wwan \
	kmod-usb-net-cdc-eem kmod-usb-net-cdc-ether \
	kmod-usb-net-cdc-mbim kmod-usb-net-rndis \
	kmod-i2c-fusb30x \
	kmod-can kmod-can-rockchip-canfd \
	luci-app-freeioe luci-proto-qmi luci-proto-3g \
    wpad-basic-wolfssl \
    tinc-freeioe-tunnel iperf3 \
    uqmi fdisk usbutils freeioe wwanleds \
    blockd kmod-eeprom-at24 fdisk
endef
TARGET_DEVICES += firefly_firefly-roc-pc

define Device/kooiot_tlink-rk3568
  DEVICE_VENDOR := KooIoT
  DEVICE_MODEL := ThingsLink RK3568
  SOC := rk3568
  UBOOT_DEVICE_NAME := tlink-rk3568
  IMAGE/sysupgrade.img.gz := boot-common | boot-script-rk | pine64-img | gzip | append-metadata
  DEVICE_PACKAGES:=kmod-gpio-button-hotplug \
    kmod-leds-gpio kmod-ledtrig-heartbeat \
    kmod-ledtrig-netdev kmod-ledtrig-gpio \
    kmod-rtc-sd3078 kmod-usb-net-asix \
    kmod-usb-serial kmod-usb-serial-option \
    kmod-usb-serial-qualcomm kmod-usb-net-qmi-wwan \
	kmod-usb-net-cdc-eem kmod-usb-net-cdc-ether \
	kmod-usb-net-cdc-mbim kmod-usb-net-rndis \
	kmod-usb-xhci-pci-renesas upd72020x-firmware \
	kmod-i2c-fusb30x \
	kmod-can kmod-can-rockchip-canfd \
	luci-app-freeioe luci-proto-qmi luci-proto-3g \
    wpad-basic-wolfssl \
    tinc-freeioe-tunnel iperf3 \
    uqmi fdisk usbutils freeioe wwanleds \
    blockd kmod-eeprom-at24 fdisk
endef
TARGET_DEVICES += kooiot_tlink-rk3568

define Device/kooiot_tlink-r5
  DEVICE_VENDOR := KooIoT
  DEVICE_MODEL := ThingsLink R5
  SOC := rk3568
  UBOOT_DEVICE_NAME := tlink-r5
  IMAGE/sysupgrade.img.gz := boot-common | boot-script-rk | pine64-img | gzip | append-metadata
  DEVICE_PACKAGES:=kmod-gpio-button-hotplug \
    kmod-leds-gpio kmod-ledtrig-heartbeat \
    kmod-ledtrig-netdev kmod-ledtrig-gpio \
    kmod-rtc-sd3078 kmod-usb-net-asix \
    kmod-usb-serial kmod-usb-serial-option \
    kmod-usb-serial-qualcomm kmod-usb-net-qmi-wwan \
	kmod-usb-xhci-pci-renesas upd72020x-firmware \
	mmod-can-rockchip-canfd \
	luci-app-freeioe luci-proto-qmi luci-proto-3g \
    wpad-basic-wolfssl \
    tinc-freeioe-tunnel iperf3 \
    uqmi fdisk usbutils freeioe wwanleds \
    blockd kmod-eeprom-at24 fdisk
endef
TARGET_DEVICES += kooiot_tlink-r5
