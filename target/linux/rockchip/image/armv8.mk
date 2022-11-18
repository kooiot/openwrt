# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2020 Tobias Maedel

# FIT will be loaded at 0x02080000. Leave 16M for that, align it to 2M and load the kernel after it.
KERNEL_LOADADDR := 0x03200000

define Device/ezpro_mrkaio-m68s
  DEVICE_VENDOR := EZPRO
  DEVICE_MODEL := Mrkaio M68S
  SOC := rk3568
  UBOOT_DEVICE_NAME := mrkaio-m68s-rk3568
  IMAGE/sysupgrade.img.gz := boot-common | boot-script nanopi-r5s | pine64-img | gzip | append-metadata
  DEVICE_PACKAGES := kmod-ata-ahci kmod-ata-ahci-platform
endef
TARGET_DEVICES += ezpro_mrkaio-m68s

define Device/hinlink_opc-h68k
  DEVICE_VENDOR := HINLINK
  DEVICE_MODEL := OPC-H68K
  SOC := rk3568
  UBOOT_DEVICE_NAME := opc-h68k-rk3568
  IMAGE/sysupgrade.img.gz := boot-common | boot-script nanopi-r5s | pine64-img | gzip | append-metadata
  DEVICE_PACKAGES := kmod-mt7921e kmod-r8125
endef
TARGET_DEVICES += hinlink_opc-h68k

define Device/fastrhino_common
  DEVICE_VENDOR := FastRhino
  SOC := rk3568
  UBOOT_DEVICE_NAME := r66s-rk3568
  IMAGE/sysupgrade.img.gz := boot-common | boot-script nanopi-r5s | pine64-img | gzip | append-metadata
  DEVICE_PACKAGES := kmod-r8125
endef

define Device/fastrhino_r66s
$(call Device/fastrhino_common)
  DEVICE_MODEL := R66S
endef
TARGET_DEVICES += fastrhino_r66s

define Device/fastrhino_r68s
$(call Device/fastrhino_common)
  DEVICE_MODEL := R68S
endef
TARGET_DEVICES += fastrhino_r68s

define Device/friendlyarm_nanopi-neo3
  DEVICE_VENDOR := FriendlyARM
  DEVICE_MODEL := NanoPi NEO3
  SOC := rk3328
  UBOOT_DEVICE_NAME := nanopi-r2s-rk3328
  IMAGE/sysupgrade.img.gz := boot-common | boot-script nanopi-r2s | pine64-bin | gzip | append-metadata
endef
TARGET_DEVICES += friendlyarm_nanopi-neo3

define Device/friendlyarm_nanopi-r2c
  DEVICE_VENDOR := FriendlyARM
  DEVICE_MODEL := NanoPi R2C
  SOC := rk3328
  UBOOT_DEVICE_NAME := nanopi-r2c-rk3328
  IMAGE/sysupgrade.img.gz := boot-common | boot-script nanopi-r2s | pine64-bin | gzip | append-metadata
  DEVICE_PACKAGES := kmod-usb-net-rtl8152
endef
TARGET_DEVICES += friendlyarm_nanopi-r2c

define Device/friendlyarm_nanopi-r2s
  DEVICE_VENDOR := FriendlyARM
  DEVICE_MODEL := NanoPi R2S
  SOC := rk3328
  UBOOT_DEVICE_NAME := nanopi-r2s-rk3328
  IMAGE/sysupgrade.img.gz := boot-common | boot-script nanopi-r2s | pine64-bin | gzip | append-metadata
  DEVICE_PACKAGES := kmod-usb-net-rtl8152
endef
TARGET_DEVICES += friendlyarm_nanopi-r2s

define Device/friendlyarm_nanopi-r4s
  DEVICE_VENDOR := FriendlyARM
  DEVICE_MODEL := NanoPi R4S
  SOC := rk3399
  UBOOT_DEVICE_NAME := nanopi-r4s-rk3399
  IMAGE/sysupgrade.img.gz := boot-common | boot-script nanopi-r4s | pine64-bin | gzip | append-metadata
  DEVICE_PACKAGES := kmod-r8168 -urngd
endef
TARGET_DEVICES += friendlyarm_nanopi-r4s

define Device/friendlyarm_nanopi-r4se
  DEVICE_VENDOR := FriendlyARM
  DEVICE_MODEL := NanoPi R4SE
  SOC := rk3399
  UBOOT_DEVICE_NAME := nanopi-r4se-rk3399
  IMAGE/sysupgrade.img.gz := boot-common | boot-script nanopi-r4s | pine64-bin | gzip | append-metadata
  DEVICE_PACKAGES := kmod-r8168 -urngd
endef
TARGET_DEVICES += friendlyarm_nanopi-r4se

define Device/rongpin_king3399
  DEVICE_VENDOR := Rongpin
  DEVICE_MODEL := King3399
  SOC := rk3399
  UBOOT_DEVICE_NAME := rongpin-king3399-rk3399
  IMAGE/sysupgrade.img.gz := boot-common | boot-script nanopi-r4s | pine64-bin | gzip | append-metadata
  DEVICE_PACKAGES := kmod-r8168 -urngd
endef
TARGET_DEVICES += rongpin_king3399

define Device/friendlyarm_nanopi-r5s
  DEVICE_VENDOR := FriendlyARM
  DEVICE_MODEL := NanoPi R5S
  SOC := rk3568
  UBOOT_DEVICE_NAME := nanopi-r5s-rk3568
  IMAGE/sysupgrade.img.gz := boot-common | boot-script nanopi-r5s | pine64-img | gzip | append-metadata
  DEVICE_PACKAGES := kmod-r8125 kmod-nvme kmod-scsi-core
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

define Device/pine64_rockpro64
  DEVICE_VENDOR := Pine64
  DEVICE_MODEL := RockPro64
  SOC := rk3399
  UBOOT_DEVICE_NAME := rockpro64-rk3399
  IMAGE/sysupgrade.img.gz := boot-common | boot-script | pine64-img | gzip | append-metadata
  DEVICE_PACKAGES := -urngd
endef
TARGET_DEVICES += pine64_rockpro64

define Device/radxa_rock-3a
  DEVICE_VENDOR := Radxa
  DEVICE_MODEL := ROCK3 A
  SOC := rk3568
  SUPPORTED_DEVICES := radxa,rock3a
  UBOOT_DEVICE_NAME := rock-3a-rk3568
  IMAGE/sysupgrade.img.gz := boot-common | boot-script nanopi-r5s | pine64-img | gzip | append-metadata
endef
TARGET_DEVICES += radxa_rock-3a

define Device/radxa_rock-pi-4
  DEVICE_VENDOR := Radxa
  DEVICE_MODEL := ROCK Pi 4
  SOC := rk3399
  SUPPORTED_DEVICES := radxa,rockpi4
  UBOOT_DEVICE_NAME := rock-pi-4-rk3399
  IMAGE/sysupgrade.img.gz := boot-common | boot-script | pine64-img | gzip | append-metadata
  DEVICE_PACKAGES := -urngd
endef
TARGET_DEVICES += radxa_rock-pi-4

define Device/sharevdi_guangmiao-g4c
  DEVICE_VENDOR := SHAREVDI
  DEVICE_MODEL := GuangMiao G4C
  SOC := rk3399
  UBOOT_DEVICE_NAME := guangmiao-g4c-rk3399
  IMAGE/sysupgrade.img.gz := boot-common | boot-script nanopi-r4s | pine64-img | gzip | append-metadata
  DEVICE_PACKAGES := kmod-r8168 -urngd
endef
TARGET_DEVICES += sharevdi_guangmiao-g4c

define Device/xunlong_orangepi-r1-plus
  DEVICE_VENDOR := Xunlong
  DEVICE_MODEL := Orange Pi R1 Plus
  SOC := rk3328
  UBOOT_DEVICE_NAME := orangepi-r1-plus-rk3328
  IMAGE/sysupgrade.img.gz := boot-common | boot-script nanopi-r2s | pine64-bin | gzip | append-metadata
  DEVICE_PACKAGES := kmod-usb-net-rtl8152
endef
TARGET_DEVICES += xunlong_orangepi-r1-plus

define Device/xunlong_orangepi-r1-plus-lts
  DEVICE_VENDOR := Xunlong
  DEVICE_MODEL := Orange Pi R1 Plus LTS
  SOC := rk3328
  UBOOT_DEVICE_NAME := orangepi-r1-plus-lts-rk3328
  IMAGE/sysupgrade.img.gz := boot-common | boot-script nanopi-r2s | pine64-bin | gzip | append-metadata
  DEVICE_PACKAGES := kmod-usb-net-rtl8152
endef
TARGET_DEVICES += xunlong_orangepi-r1-plus-lts

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
	kmod-brcmfmac kmod-ikconfig kmod-ata-ahci-platform \
	firefly-roc-pc-firmware wpad-basic-wolfssl \
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
	kmod-usb-xhci-pci upd72020x-firmware \
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
