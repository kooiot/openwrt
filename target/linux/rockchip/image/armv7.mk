# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2024 RK3506 OpenWrt Port

define Device/rk3506
  $(Device/arm32)
  SOC := rk3506
  BOOT_SCRIPT:= rk3506
  RKIMG_TYPE := rk3506-img
  KERNEL_LOADADDR := 0x03200000
  DEVICE_DTS_LOADADDR := 0x02000000
endef

define Device/rk3506-nand
  $(Device/NAND)
  SOC := rk3506
  BOOT_SCRIPT := rk3506
  KERNEL_LOADADDR := 0x03200000
  DEVICE_DTS_LOADADDR := 0x02000000
endef

### Devices ###

define Device/hzhy_mini_evm_emmc
  $(Device/rk3506)
  DEVICE_VENDOR := HZHY
  DEVICE_MODEL := RK3506SP MiniEVM (eMMC)
  DEVICE_DTS := rockchip/HZ-RK3506SP_MiniEVM_EMMC
  DEVICE_PACKAGES := kmod-usb-hid kmod-usb-ohci kmod-usb2 kmod-usb-storage \
    kmod-usb-storage-extras kmod-usb-net kmod-usb-core kmod-gpio-button-hotplug \
    urandom-seed
endef
TARGET_DEVICES += hzhy_mini_evm_emmc

define Device/hzhy_mini_evm_nand
  $(Device/rk3506)
  DEVICE_VENDOR := HZHY
  DEVICE_MODEL := RK3506SP MiniEVM (NAND)
  DEVICE_DTS := rockchip/HZ-RK3506SP_MiniEVM_NAND
  DEVICE_PACKAGES := kmod-usb-hid kmod-usb-ohci kmod-usb2 kmod-usb-storage \
    kmod-usb-storage-extras kmod-usb-net kmod-usb-core kmod-gpio-button-hotplug \
    kmod-mtd-rw urandom-seed
endef
TARGET_DEVICES += hzhy_mini_evm_nand

define Device/hzhy_mini_evm_sd
  $(Device/rk3506)
  DEVICE_VENDOR := HZHY
  DEVICE_MODEL := RK3506SP MiniEVM (SD)
  DEVICE_DTS := rockchip/HZ-RK3506SP_MiniEVM_SD
  DEVICE_PACKAGES := kmod-usb-hid kmod-usb-ohci kmod-usb2 kmod-usb-storage \
    kmod-usb-storage-extras kmod-usb-net kmod-usb-core kmod-gpio-button-hotplug \
    urandom-seed
  UBOOT_DEVICE_NAME := evb-rk3506
endef
TARGET_DEVICES += hzhy_mini_evm_sd

define Device/vanxoak_vx-hd-rk3506-iot
  $(Device/rk3506)
  DEVICE_VENDOR := Vanxoak
  DEVICE_MODEL := HD-RK3506-IOT (SD)
  UBOOT_DEVICE_NAME := evb-rk3506
  DEVICE_PACKAGES:=kmod-gpio-button-hotplug \
    kmod-leds-gpio kmod-ledtrig-heartbeat \
    kmod-ledtrig-netdev kmod-ledtrig-gpio \
    kmod-rtc-sd3078 kmod-usb-net-asix \
    kmod-usb-serial kmod-usb-serial-option \
    kmod-usb-serial-qualcomm kmod-usb-net-qmi-wwan \
    kmod-usb-net-cdc-eem kmod-usb-net-cdc-ether \
    kmod-usb-net-cdc-mbim kmod-usb-net-rndis \
    kmod-usb-xhci-pci kmod-i2c-fusb30x \
    kmod-phy-motorcomm kmod-mmc \
    kmod-can kmod-can-rockchip-canfd \
    kmod-ata-ahci kmod-ata-ahci-dwc \
    luci-app-freeioe luci-proto-qmi \
    luci-proto-3g luci-proto-wwan \
    tlink-r7-firmware wpad-basic-mbedtls \
    tinc-freeioe-tunnel iperf3 \
    uqmi fdisk usbutils freeioe wwan-watch \
    blockd kmod-eeprom-at24 fdisk
endef
TARGET_DEVICES += vanxoak_vx-hd-rk3506-iot

define Device/vanxoak_vx-hd-rk3506-iot-spinand
  $(Device/rk3506-nand)
  DEVICE_VENDOR := Vanxoak
  DEVICE_MODEL := HD-RK3506-IOT (SPINAND)
  DEVICE_PACKAGES:=kmod-gpio-button-hotplug \
    kmod-leds-gpio kmod-ledtrig-heartbeat \
    kmod-ledtrig-netdev kmod-ledtrig-gpio \
    kmod-rtc-sd3078 kmod-usb-net-asix \
    kmod-usb-serial kmod-usb-serial-option \
    kmod-usb-serial-qualcomm kmod-usb-net-qmi-wwan \
    kmod-usb-net-cdc-eem kmod-usb-net-cdc-ether \
    kmod-usb-net-cdc-mbim kmod-usb-net-rndis \
    kmod-usb-xhci-pci kmod-i2c-fusb30x \
    kmod-phy-motorcomm kmod-mmc \
    kmod-can kmod-can-rockchip-canfd \
    kmod-ata-ahci kmod-ata-ahci-dwc \
    luci-app-freeioe luci-proto-qmi \
    luci-proto-3g luci-proto-wwan \
    tlink-r7-firmware wpad-basic-mbedtls \
    tinc-freeioe-tunnel iperf3 \
    uqmi fdisk usbutils freeioe wwan-watch \
    blockd kmod-eeprom-at24 fdisk
endef
TARGET_DEVICES += vanxoak_vx-hd-rk3506-iot-spinand
