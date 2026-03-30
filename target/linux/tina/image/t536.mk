# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2025-2026 KooIoT.com

#define Device/avaotasbc_avaota-a1
#  KERNEL_NAME := Image
#  DEVICE_VENDOR := AvaotaSBC
#  DEVICE_MODEL := Avaota-A1
#  DEVICE_DTS = allwinner/sun55i-t527-avaota-a1
#  SYTERKIT_DEV = avaota-a1
#  DEVICE_PACKAGES := kmod-mac80211 aic8800-firmware kmod-aic8800-bt kmod-aic8800-wlan kmod-tft-st7789v kmod-aw-nna
#  IMAGE/sysupgrade.img.gz := syterkit-img | gzip | append-metadata
#endef
#TARGET_DEVICES += avaotasbc_avaota-a1

define Device/kooiot_tlink-t536
  DEVICE_VENDOR := KooIoT
  DEVICE_MODEL := ThingsLink T536
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
  SOC := sun55i-t536
endef
TARGET_DEVICES += kooiot_tlink-t536

define Device/rp-dr4-t536
  DEVICE_VENDOR := RongPin
  DEVICE_MODEL := DR4-T536
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
  SOC := sun55i-t536
endef
TARGET_DEVICES += rp-dr4-t536

