# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2020 Sarah Maedel

define Device/rk3308
  SOC := rk3308
  KERNEL_LOADADDR := 0x03000000
endef

define Device/rk3328
  SOC := rk3328
  KERNEL_LOADADDR := 0x03200000
endef

define Device/rk3399
  SOC := rk3399
  KERNEL_LOADADDR := 0x03200000
endef

define Device/rk3566
  SOC := rk3566
  KERNEL_LOADADDR := 0x03000000
endef

define Device/rk3568
  SOC := rk3568
  KERNEL_LOADADDR := 0x03000000
endef

define Device/rk3576
  SOC := rk3576
  KERNEL_LOADADDR := 0x43000000
endef

define Device/rk3582
  SOC := rk3582
  KERNEL_LOADADDR := 0x03000000
endef

define Device/rk3588
  SOC := rk3588
  KERNEL_LOADADDR := 0x03000000
endef

define Device/rk3588s
  SOC := rk3588s
  KERNEL_LOADADDR := 0x03000000
endef

define Device/friendlyarm_nanopi-neo3
  DEVICE_VENDOR := FriendlyARM
  DEVICE_MODEL := NanoPi NEO3
  SOC := rk3328
  UBOOT_DEVICE_NAME := nanopi-r2s-rk3328
endef
TARGET_DEVICES += friendlyarm_nanopi-neo3

define Device/armsom_sige7
  $(Device/rk3588)
  DEVICE_VENDOR := ArmSoM
  DEVICE_MODEL := Sige7
  DEVICE_ALT0_VENDOR := Bananapi
  DEVICE_ALT0_MODEL := BPi-M7
  DEVICE_DTS := rk3588-armsom-sige7
  DEVICE_PACKAGES := kmod-r8169
endef
TARGET_DEVICES += armsom_sige7

define Device/firefly_roc-rk3328-cc
  $(Device/rk3328)
  DEVICE_VENDOR := Firefly
  DEVICE_MODEL := ROC-RK3328-CC
  DEVICE_DTS := rk3328-roc-cc
  UBOOT_DEVICE_NAME := roc-cc-rk3328
endef
TARGET_DEVICES += firefly_roc-rk3328-cc

define Device/friendlyarm_nanopc-t4
  $(Device/rk3399)
  DEVICE_VENDOR := FriendlyARM
  DEVICE_MODEL := NanoPC T4
  DEVICE_PACKAGES := kmod-brcmfmac brcmfmac-nvram-4356-sdio cypress-firmware-4356-sdio
endef
TARGET_DEVICES += friendlyarm_nanopc-t4

define Device/friendlyarm_nanopc-t6
  $(Device/rk3588)
  DEVICE_VENDOR := FriendlyARM
  DEVICE_MODEL := NanoPC T6
  DEVICE_PACKAGES := kmod-r8169
endef
TARGET_DEVICES += friendlyarm_nanopc-t6

define Device/friendlyarm_nanopi-r2c
  $(Device/rk3328)
  DEVICE_VENDOR := FriendlyARM
  DEVICE_MODEL := NanoPi R2C
  DEVICE_PACKAGES := kmod-usb-net-rtl8152
endef
TARGET_DEVICES += friendlyarm_nanopi-r2c

define Device/friendlyarm_nanopi-r2c-plus
  $(Device/rk3328)
  DEVICE_VENDOR := FriendlyARM
  DEVICE_MODEL := NanoPi R2C Plus
  DEVICE_PACKAGES := kmod-usb-net-rtl8152
endef
TARGET_DEVICES += friendlyarm_nanopi-r2c-plus

define Device/friendlyarm_nanopi-r2s
  $(Device/rk3328)
  DEVICE_VENDOR := FriendlyARM
  DEVICE_MODEL := NanoPi R2S
  DEVICE_PACKAGES := kmod-usb-net-rtl8152
endef
TARGET_DEVICES += friendlyarm_nanopi-r2s

define Device/friendlyarm_nanopi-r3s
  $(Device/rk3566)
  DEVICE_VENDOR := FriendlyARM
  DEVICE_MODEL := NanoPi R3S
  DEVICE_PACKAGES := kmod-r8169
endef
TARGET_DEVICES += friendlyarm_nanopi-r3s

define Device/friendlyarm_nanopi-r4s
  $(Device/rk3399)
  DEVICE_VENDOR := FriendlyARM
  DEVICE_MODEL := NanoPi R4S
  DEVICE_VARIANT := 4GB LPDDR4
  DEVICE_PACKAGES := kmod-r8169
endef
TARGET_DEVICES += friendlyarm_nanopi-r4s

define Device/friendlyarm_nanopi-r4s-enterprise
  $(Device/rk3399)
  DEVICE_VENDOR := FriendlyARM
  DEVICE_MODEL := NanoPi R4S Enterprise Edition
  DEVICE_VARIANT := 4GB LPDDR4
  UBOOT_DEVICE_NAME := nanopi-r4s-rk3399
  DEVICE_PACKAGES := kmod-r8169
endef
TARGET_DEVICES += friendlyarm_nanopi-r4s-enterprise

define Device/friendlyarm_nanopi-r5c
  $(Device/rk3568)
  DEVICE_VENDOR := FriendlyARM
  DEVICE_MODEL := NanoPi R5C
  DEVICE_PACKAGES := kmod-r8169 kmod-rtw88-8822ce rtl8822ce-firmware wpad-basic-mbedtls
endef
TARGET_DEVICES += friendlyarm_nanopi-r5c

define Device/friendlyarm_nanopi-r5s
  $(Device/rk3568)
  DEVICE_VENDOR := FriendlyARM
  DEVICE_MODEL := NanoPi R5S
  DEVICE_PACKAGES := kmod-r8169
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

define Device/friendlyarm_nanopi-r6c
  $(Device/rk3588s)
  DEVICE_VENDOR := FriendlyARM
  DEVICE_MODEL := NanoPi R6C
  DEVICE_PACKAGES := kmod-r8169
endef
TARGET_DEVICES += friendlyarm_nanopi-r6c

define Device/friendlyarm_nanopi-r6s
  $(Device/rk3588s)
  DEVICE_VENDOR := FriendlyARM
  DEVICE_MODEL := NanoPi R6S
  DEVICE_PACKAGES := kmod-r8169
endef
TARGET_DEVICES += friendlyarm_nanopi-r6s

define Device/lunzn_fastrhino-r66s
  $(Device/rk3568)
  DEVICE_VENDOR := Lunzn
  DEVICE_MODEL := FastRhino R66S
  DEVICE_PACKAGES := kmod-r8169
endef
TARGET_DEVICES += lunzn_fastrhino-r66s

define Device/pine64_rock64
  $(Device/rk3328)
  DEVICE_VENDOR := Pine64
  DEVICE_MODEL := Rock64
endef
TARGET_DEVICES += pine64_rock64

define Device/pine64_rockpro64
  $(Device/rk3399)
  DEVICE_VENDOR := Pine64
  DEVICE_MODEL := RockPro64
  SUPPORTED_DEVICES += pine64,rockpro64-v2.1
endef
TARGET_DEVICES += pine64_rockpro64

define Device/radxa_cm3-io
  $(Device/rk3566)
  DEVICE_VENDOR := Radxa
  DEVICE_MODEL := CM3 IO
  DEVICE_DTS := rk3566-radxa-cm3-io
  UBOOT_DEVICE_NAME := radxa-cm3-io-rk3566
endef
TARGET_DEVICES += radxa_cm3-io

define Device/radxa_e25
  $(Device/rk3568)
  DEVICE_VENDOR := Radxa
  DEVICE_MODEL := E25
  DEVICE_DTS := rk3568-radxa-e25
  BOOT_SCRIPT := radxa-e25
  UBOOT_DEVICE_NAME := radxa-e25-rk3568
  DEVICE_PACKAGES := kmod-r8169 kmod-ata-ahci-dwc
endef
TARGET_DEVICES += radxa_e25

define Device/radxa_e52c
  $(Device/rk3582)
  DEVICE_VENDOR := Radxa
  DEVICE_MODEL := E52C
  UBOOT_DEVICE_NAME := generic-rk3588
  DEVICE_DTS := rk3582-radxa-e52c
  DEVICE_PACKAGES := blkdiscard kmod-r8169
endef
TARGET_DEVICES += radxa_e52c

define Device/radxa_rock-3a
  $(Device/rk3568)
  DEVICE_VENDOR := Radxa
  DEVICE_MODEL := ROCK 3A
  SUPPORTED_DEVICES := radxa,rock3a
  DEVICE_PACKAGES := kmod-usb-net-cdc-ncm kmod-usb-net-rndis
endef
TARGET_DEVICES += radxa_rock-3a

define Device/radxa_rock-3b
  $(Device/rk3568)
  DEVICE_VENDOR := Radxa
  DEVICE_MODEL := ROCK 3B
  DEVICE_PACKAGES := kmod-usb-net-cdc-ncm kmod-usb-net-rndis
endef
TARGET_DEVICES += radxa_rock-3b

define Device/radxa_rock-3c
  $(Device/rk3566)
  DEVICE_VENDOR := Radxa
  DEVICE_MODEL := ROCK 3C
  DEVICE_PACKAGES := kmod-usb-net-cdc-ncm kmod-usb-net-rndis
endef
TARGET_DEVICES += radxa_rock-3c

define Device/radxa_rock-4c-plus
  $(Device/rk3399)
  DEVICE_VENDOR := Radxa
  DEVICE_MODEL := ROCK 4C+
endef
TARGET_DEVICES += radxa_rock-4c-plus

define Device/radxa_rock-4d
  $(Device/rk3576)
  DEVICE_VENDOR := Radxa
  DEVICE_MODEL := ROCK 4D
endef
TARGET_DEVICES += radxa_rock-4d

define Device/radxa_rock-4se
  $(Device/rk3399)
  DEVICE_VENDOR := Radxa
  DEVICE_MODEL := ROCK 4SE
endef
TARGET_DEVICES += radxa_rock-4se

define Device/radxa_rock-5-itx
  $(Device/rk3588)
  DEVICE_VENDOR := Radxa
  DEVICE_MODEL := ROCK 5 ITX/ITX+
  DEVICE_PACKAGES := blkdiscard block-mount kmod-ata-ahci kmod-hwmon-pwmfan kmod-nvme kmod-r8169 kmod-rtw89-8852be wpad-basic-mbedtls
endef
TARGET_DEVICES += radxa_rock-5-itx

define Device/radxa_rock-5a
  $(Device/rk3588s)
  DEVICE_VENDOR := Radxa
  DEVICE_MODEL := ROCK 5A
  UBOOT_DEVICE_NAME := rock5a-rk3588s
  DEVICE_PACKAGES := blkdiscard block-mount kmod-ata-ahci kmod-hwmon-pwmfan kmod-nvme kmod-r8169 kmod-rtw89-8852be wpad-basic-mbedtls
endef
TARGET_DEVICES += radxa_rock-5a

define Device/radxa_rock-5b
  $(Device/rk3588)
  DEVICE_VENDOR := Radxa
  DEVICE_MODEL := ROCK 5B
  UBOOT_DEVICE_NAME := rock5b-rk3588
  DEVICE_PACKAGES := blkdiscard block-mount kmod-hwmon-pwmfan kmod-nvme kmod-r8169 kmod-rtw89-8852be wpad-basic-mbedtls
endef
TARGET_DEVICES += radxa_rock-5b

define Device/radxa_rock-5b-plus
  $(Device/rk3588)
  DEVICE_VENDOR := Radxa
  DEVICE_MODEL := ROCK 5B+
  UBOOT_DEVICE_NAME := generic-rk3588
  DEVICE_DTS := rk3588-rock-5b-plus
  DEVICE_PACKAGES := blkdiscard block-mount kmod-hwmon-pwmfan kmod-nvme kmod-r8169 kmod-rtw89-8852be wpad-basic-mbedtls
endef
TARGET_DEVICES += radxa_rock-5b-plus

define Device/radxa_rock-5c
  $(Device/rk3588s)
  DEVICE_VENDOR := Radxa
  DEVICE_MODEL := ROCK 5C/5C Lite
  DEVICE_PACKAGES := blkdiscard block-mount kmod-ata-ahci kmod-hwmon-pwmfan kmod-nvme kmod-r8169
endef
TARGET_DEVICES += radxa_rock-5c

define Device/radxa_rock-5t
  $(Device/rk3588)
  DEVICE_VENDOR := Radxa
  DEVICE_MODEL := ROCK 5T
  UBOOT_DEVICE_NAME := generic-rk3588
  DEVICE_DTS := rk3588-rock-5t
  DEVICE_PACKAGES := blkdiscard block-mount iwlwifi-firmware-ax210 kmod-hwmon-pwmfan kmod-iwlwifi kmod-nvme kmod-r8169 wpad-basic-mbedtls
endef
TARGET_DEVICES += radxa_rock-5t

define Device/radxa_rock-pi-4a
  $(Device/rk3399)
  DEVICE_VENDOR := Radxa
  DEVICE_MODEL := ROCK Pi 4A
  SUPPORTED_DEVICES := radxa,rockpi4a radxa,rockpi4
  UBOOT_DEVICE_NAME := rock-pi-4-rk3399
endef
TARGET_DEVICES += radxa_rock-pi-4a

define Device/radxa_rock-pi-e
  $(Device/rk3328)
  DEVICE_VENDOR := Radxa
  DEVICE_MODEL := ROCK Pi E
endef
TARGET_DEVICES += radxa_rock-pi-e

define Device/radxa_rock-pi-e-v3
  $(Device/rk3328)
  DEVICE_VENDOR := Radxa
  DEVICE_MODEL := ROCK Pi E v3.0
  DEVICE_DTS := rk3328-rock-pi-e
  DEVICE_PACKAGES := kmod-rtw88-8723du kmod-rtw88-8821cu kmod-usb-net-cdc-ncm kmod-usb-net-rndis wpad-basic-mbedtls
endef
TARGET_DEVICES += radxa_rock-pi-e-v3

define Device/radxa_rock-pi-s
  $(Device/rk3308)
  DEVICE_VENDOR := Radxa
  DEVICE_MODEL := ROCK Pi S
  SUPPORTED_DEVICES := radxa,rockpis
  BOOT_SCRIPT := rock-pi-s
  DEVICE_PACKAGES := kmod-rtw88-8723ds kmod-usb-net-cdc-ncm kmod-usb-net-rndis wpad-basic-mbedtls
endef
TARGET_DEVICES += radxa_rock-pi-s

define Device/radxa_zero-3e
  $(Device/rk3566)
  DEVICE_VENDOR := Radxa
  DEVICE_MODEL := ZERO 3E
  DEVICE_DTS := rk3566-radxa-zero-3e
  UBOOT_DEVICE_NAME := radxa-zero-3-rk3566
  DEVICE_PACKAGES := kmod-usb-net-cdc-ncm kmod-usb-net-rndis
endef
TARGET_DEVICES += radxa_zero-3e

define Device/radxa_zero-3w
  $(Device/rk3566)
  DEVICE_VENDOR := Radxa
  DEVICE_MODEL := ZERO 3W
  DEVICE_DTS := rk3566-radxa-zero-3w
  UBOOT_DEVICE_NAME := radxa-zero-3-rk3566
  DEVICE_PACKAGES := kmod-usb-net-cdc-ncm kmod-usb-net-rndis
endef
TARGET_DEVICES += radxa_zero-3w

define Device/sinovoip_bpi-r2-pro
  $(Device/rk3568)
  DEVICE_VENDOR := Sinovoip
  DEVICE_MODEL := Bananapi-R2 Pro
  SUPPORTED_DEVICES := sinovoip,rk3568-bpi-r2pro
  DEVICE_PACKAGES := kmod-ata-ahci-dwc
endef
TARGET_DEVICES += sinovoip_bpi-r2-pro

define Device/xunlong_orangepi-r1-plus
  $(Device/rk3328)
  DEVICE_VENDOR := Xunlong
  DEVICE_MODEL := Orange Pi R1 Plus
  DEVICE_PACKAGES := kmod-usb-net-rtl8152
endef
TARGET_DEVICES += xunlong_orangepi-r1-plus

define Device/xunlong_orangepi-r1-plus-lts
  $(Device/rk3328)
  DEVICE_VENDOR := Xunlong
  DEVICE_MODEL := Orange Pi R1 Plus LTS
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
  BOOT_SCRIPT := tlink
  UBOOT_DEVICE_NAME := tlink-r4x-rk3568
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
	kmod-phy-realtek kmod-r8168 \
	kmod-mmc kmod-brcmfmac \
	kmod-can kmod-can-rockchip-canfd \
	kmod-ata-ahci kmod-ata-ahci-dwc \
	luci-app-freeioe luci-proto-qmi luci-proto-3g \
    tlink-r4x-firmware wpad-basic-mbedtls \
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
  BOOT_SCRIPT := tlink
  UBOOT_DEVICE_NAME := tlink-r7-rk3568
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
	kmod-phy-realtek kmod-r8168 \
	kmod-mmc kmod-brcmfmac \
	kmod-can kmod-can-rockchip-canfd \
	kmod-ata-ahci kmod-ata-ahci-dwc \
	luci-app-freeioe luci-proto-qmi luci-proto-3g \
    tlink-r7-firmware wpad-basic-mbedtls \
    tinc-freeioe-tunnel iperf3 \
    uqmi fdisk usbutils freeioe wwanleds \
    blockd kmod-eeprom-at24 fdisk
endef
TARGET_DEVICES += kooiot_tlink-r7
