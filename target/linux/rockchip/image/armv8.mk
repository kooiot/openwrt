# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2020 Tobias Maedel

# FIT will be loaded at 0x02080000. Leave 16M for that, align it to 2M and load the kernel after it.
KERNEL_LOADADDR := 0x03200000

define Device/friendlyarm_nanopi-neo3
  DEVICE_VENDOR := FriendlyARM
  DEVICE_MODEL := NanoPi NEO3
  SOC := rk3328
  UBOOT_DEVICE_NAME := nanopi-r2s-rk3328
endef
TARGET_DEVICES += friendlyarm_nanopi-neo3

define Device/firefly_roc-rk3328-cc
  DEVICE_VENDOR := Firefly
  DEVICE_MODEL := ROC-RK3328-CC
  SOC := rk3328
  DEVICE_DTS := rockchip/rk3328-roc-cc
  UBOOT_DEVICE_NAME := roc-cc-rk3328
endef
TARGET_DEVICES += firefly_roc-rk3328-cc

define Device/friendlyarm_nanopc-t4
  DEVICE_VENDOR := FriendlyARM
  DEVICE_MODEL := NanoPC T4
  SOC := rk3399
  DEVICE_PACKAGES := kmod-brcmfmac brcmfmac-nvram-4356-sdio cypress-firmware-4356-sdio
endef
TARGET_DEVICES += friendlyarm_nanopc-t4

define Device/friendlyarm_nanopi-r2c
  DEVICE_VENDOR := FriendlyARM
  DEVICE_MODEL := NanoPi R2C
  SOC := rk3328
  DEVICE_PACKAGES := kmod-usb-net-rtl8152
endef
TARGET_DEVICES += friendlyarm_nanopi-r2c

define Device/friendlyarm_nanopi-r2c-plus
  DEVICE_VENDOR := FriendlyARM
  DEVICE_MODEL := NanoPi R2C Plus
  SOC := rk3328
  UBOOT_DEVICE_NAME := nanopi-r2c-plus-rk3328
  DEVICE_PACKAGES := kmod-usb-net-rtl8152
endef
TARGET_DEVICES += friendlyarm_nanopi-r2c-plus

define Device/friendlyarm_nanopi-r2s
  DEVICE_VENDOR := FriendlyARM
  DEVICE_MODEL := NanoPi R2S
  SOC := rk3328
  DEVICE_PACKAGES := kmod-usb-net-rtl8152
endef
TARGET_DEVICES += friendlyarm_nanopi-r2s

define Device/friendlyarm_nanopi-r4s
  DEVICE_VENDOR := FriendlyARM
  DEVICE_MODEL := NanoPi R4S
  DEVICE_VARIANT := 4GB LPDDR4
  SOC := rk3399
  DEVICE_PACKAGES := kmod-r8168 -urngd
endef
TARGET_DEVICES += friendlyarm_nanopi-r4s

define Device/friendlyarm_nanopi-r4se
  DEVICE_VENDOR := FriendlyARM
  DEVICE_MODEL := NanoPi R4SE
  DEVICE_VARIANT := 4GB LPDDR4
  SOC := rk3399
  DEVICE_PACKAGES := kmod-r8168 -urngd
endef
TARGET_DEVICES += friendlyarm_nanopi-r4se

define Device/friendlyarm_nanopi-r4s-enterprise
  DEVICE_VENDOR := FriendlyARM
  DEVICE_MODEL := NanoPi R4S Enterprise Edition
  DEVICE_VARIANT := 4GB LPDDR4
  SOC := rk3399
  UBOOT_DEVICE_NAME := nanopi-r4s-rk3399
  DEVICE_PACKAGES := kmod-r8169
endef
TARGET_DEVICES += friendlyarm_nanopi-r4s-enterprise

define Device/friendlyarm_nanopi-r5c
  DEVICE_VENDOR := FriendlyARM
  DEVICE_MODEL := NanoPi R5C
  SOC := rk3568
  DEVICE_PACKAGES := kmod-r8169 kmod-rtw88-8822ce rtl8822ce-firmware wpad-basic-mbedtls kmod-nvme kmod-scsi-core
endef
TARGET_DEVICES += friendlyarm_nanopi-r5c

define Device/friendlyarm_nanopi-r5s
  DEVICE_VENDOR := FriendlyARM
  DEVICE_MODEL := NanoPi R5S
  SOC := rk3568
  DEVICE_PACKAGES := kmod-r8169 kmod-nvme kmod-scsi-core
endef
TARGET_DEVICES += friendlyarm_nanopi-r5s

define Device/firefly_station-p2
  DEVICE_VENDOR := Firefly
  DEVICE_MODEL := Station P2
  DEVICE_DTS := rockchip/rk3568-roc-pc
  UBOOT_DEVICE_NAME := station-p2-rk3568
  IMAGE/sysupgrade.img.gz := boot-common | boot-script nanopi-r5s | pine64-img | gzip | append-metadata
  DEVICE_PACKAGES := kmod-brcmfmac kmod-ikconfig kmod-ata-ahci-platform station-p2-firmware wpad-openssl
endef
TARGET_DEVICES += firefly_station-p2

define Device/hinlink_common
  DEVICE_VENDOR := HINLINK
  UBOOT_DEVICE_NAME := opc-h68k-rk3568
  IMAGE/sysupgrade.img.gz := boot-common | boot-script nanopi-r5s | pine64-img | gzip | append-metadata
  DEVICE_PACKAGES := kmod-ata-ahci-platform kmod-hwmon-pwmfan kmod-mt7921e kmod-r8125 wpad-openssl
endef

define Device/hinlink_opc-h66k
$(call Device/hinlink_common)
  DEVICE_MODEL := OPC-H66K
  SOC := rk3568
endef
TARGET_DEVICES += hinlink_opc-h66k

define Device/hinlink_opc-h68k
$(call Device/hinlink_common)
  DEVICE_MODEL := OPC-H68K
  SOC := rk3568
endef
TARGET_DEVICES += hinlink_opc-h68k

define Device/hinlink_opc-h69k
$(call Device/hinlink_common)
  DEVICE_MODEL := OPC-H69K
  SOC := rk3568
  DEVICE_PACKAGES += kmod-mt7916-firmware kmod-usb-serial-option uqmi
endef
TARGET_DEVICES += hinlink_opc-h69k

define Device/pine64_rock64
  DEVICE_VENDOR := Pine64
  DEVICE_MODEL := Rock64
  SOC := rk3328
endef
TARGET_DEVICES += pine64_rock64

define Device/pine64_rockpro64
  DEVICE_VENDOR := Pine64
  DEVICE_MODEL := RockPro64
  SOC := rk3399
  DEVICE_PACKAGES := -urngd
endef
TARGET_DEVICES += pine64_rockpro64

define Device/radxa_cm3-io
  DEVICE_VENDOR := Radxa
  DEVICE_MODEL := CM3 IO
  SOC := rk3566
  DEVICE_DTS := rockchip/rk3566-radxa-cm3-io
  UBOOT_DEVICE_NAME := radxa-cm3-io-rk3566
endef
TARGET_DEVICES += radxa_cm3-io

define Device/radxa_e25
  DEVICE_VENDOR := Radxa
  DEVICE_MODEL := E25
  SOC := rk3568
  DEVICE_DTS := rockchip/rk3568-radxa-e25
  UBOOT_DEVICE_NAME := radxa-e25-rk3568
  DEVICE_PACKAGES := kmod-r8169 kmod-ata-ahci-platform
endef
TARGET_DEVICES += radxa_e25

define Device/radxa_rock-pi-4a
  DEVICE_VENDOR := Radxa
  DEVICE_MODEL := ROCK Pi 4A
  SOC := rk3399
  SUPPORTED_DEVICES := radxa,rockpi4a radxa,rockpi4
  UBOOT_DEVICE_NAME := rock-pi-4-rk3399
endef
TARGET_DEVICES += radxa_rock-pi-4a

define Device/radxa_rock-pi-e
  DEVICE_VENDOR := Radxa
  DEVICE_MODEL := ROCK Pi E
  SOC := rk3328
endef
TARGET_DEVICES += radxa_rock-pi-e

define Device/xunlong_orangepi-r1-plus
  DEVICE_VENDOR := Xunlong
  DEVICE_MODEL := Orange Pi R1 Plus
  SOC := rk3328
  DEVICE_PACKAGES := kmod-usb-net-rtl8152
endef
TARGET_DEVICES += xunlong_orangepi-r1-plus

define Device/xunlong_orangepi-r1-plus-lts
  DEVICE_VENDOR := Xunlong
  DEVICE_MODEL := Orange Pi R1 Plus LTS
  SOC := rk3328
  DEVICE_PACKAGES := kmod-usb-net-rtl8152
endef
TARGET_DEVICES += xunlong_orangepi-r1-plus-lts

define Device/firefly_firefly-roc-pc
  DEVICE_VENDOR := Firefly
  DEVICE_MODEL := Roc PC (RK3568)
  KERNEL := kernel-bin
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
	kmod-mmc kmod-brcmfmac \
  	kmod-ikconfig kmod-ata-ahci-platform \
	firefly-roc-pc-firmware wpad-basic-mbedtls \
	tinc-freeioe-tunnel iperf3 \
    uqmi fdisk usbutils freeioe wwanleds \
    blockd kmod-eeprom-at24 fdisk
endef

TARGET_DEVICES += firefly_firefly-roc-pc

define Device/kooiot_tlink-r4x
  DEVICE_VENDOR := KooIoT
  DEVICE_MODEL := ThingsLink R4x (RK3568)
  KERNEL := kernel-bin
  SOC := rk3568
  UBOOT_DEVICE_NAME := tlink-r4x
  IMAGE/sysupgrade.img.gz := boot-common | boot-script-rk | pine64-img | gzip | append-metadata
  DEVICE_PACKAGES:=kmod-gpio-button-hotplug \
    kmod-leds-gpio kmod-ledtrig-heartbeat \
    kmod-ledtrig-netdev kmod-ledtrig-gpio \
    kmod-rtc-sd3078 kmod-usb-net-asix \
    kmod-usb-serial kmod-usb-serial-option \
    kmod-usb-serial-qualcomm kmod-usb-net-qmi-wwan \
	kmod-usb-net-cdc-eem kmod-usb-net-cdc-ether \
	kmod-usb-net-cdc-mbim kmod-usb-net-rndis \
	kmod-usb-xhci-pci upd72020x-firmware \
	kmod-i2c-fusb30x \
	kmod-mmc kmod-brcmfmac \
	kmod-can kmod-can-rockchip-canfd \
	kmod-ata-ahci kmod-ata-ahci-platform \
	luci-app-freeioe luci-proto-qmi luci-proto-3g \
    tlink-r7-firmware wpad-basic-mbedtls \
    tinc-freeioe-tunnel iperf3 \
    uqmi fdisk usbutils freeioe wwanleds \
    blockd kmod-eeprom-at24 fdisk
endef
TARGET_DEVICES += kooiot_tlink-r4x

define Device/kooiot_tlink-r7
  DEVICE_VENDOR := KooIoT
  DEVICE_MODEL := ThingsLink R7 (RK3568)
  KERNEL := kernel-bin
  SOC := rk3568
  UBOOT_DEVICE_NAME := tlink-r7
  IMAGE/sysupgrade.img.gz := boot-common | boot-script-rk | pine64-img | gzip | append-metadata
  DEVICE_PACKAGES:=kmod-gpio-button-hotplug \
    kmod-leds-gpio kmod-ledtrig-heartbeat \
    kmod-ledtrig-netdev kmod-ledtrig-gpio \
    kmod-rtc-sd3078 kmod-usb-net-asix \
    kmod-usb-serial kmod-usb-serial-option \
    kmod-usb-serial-qualcomm kmod-usb-net-qmi-wwan \
	kmod-usb-net-cdc-eem kmod-usb-net-cdc-ether \
	kmod-usb-net-cdc-mbim kmod-usb-net-rndis \
	kmod-usb-xhci-pci upd72020x-firmware \
	kmod-i2c-fusb30x \
	kmod-mmc kmod-brcmfmac \
	kmod-can kmod-can-rockchip-canfd \
	kmod-ata-ahci kmod-ata-ahci-platform \
	luci-app-freeioe luci-proto-qmi luci-proto-3g \
    tlink-r7-firmware wpad-basic-mbedtls \
    tinc-freeioe-tunnel iperf3 \
    uqmi fdisk usbutils freeioe wwanleds \
    blockd kmod-eeprom-at24 fdisk
endef
TARGET_DEVICES += kooiot_tlink-r7
