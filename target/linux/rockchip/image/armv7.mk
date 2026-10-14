# SPDX-License-Identifier: GPL-2.0-only

RKBOOT_FDT_ADDR := 0x08300000
RKBOOT_TAGS_OFFSET := 0x7f00
ROOTDEV_OVERLAY_ALIGN := 64k

DEVICE_VARS += FLS_IC FLS_SENSOR

define Build/rk-bootimg
	$(STAGING_DIR_HOST)/bin/mkrkboot \
		-k $@ \
		-d $(KDIR)/image-$(firstword $(DEVICE_DTS)).dtb \
		-a $(KERNEL_LOADADDR) \
		-f $(RKBOOT_FDT_ADDR) \
		-t $$(( $(KERNEL_LOADADDR) - $(RKBOOT_TAGS_OFFSET) )) \
		-o $@.new
	mv $@.new $@
endef

define Build/herospeed-fls
	dd if=$(IMAGE_ROOTFS) of=$@.system bs=$(ROOTDEV_OVERLAY_ALIGN) conv=sync
	dd if=/dev/zero bs=4k count=1 >> $@.system
	$(STAGING_DIR_HOST)/bin/mkhsfls \
		-i $(FLS_IC) -s $(FLS_SENSOR) \
		-e boot=$(IMAGE_KERNEL) \
		-e system=$@.system \
		-o $@.new
	mv $@.new $@
	rm -f $@.system
endef

define Device/rv1126
  SOC := rv1126
  IMAGES :=
  KERNEL_NAME := zImage
  KERNEL = kernel-bin | rk-bootimg
  KERNEL_INITRAMFS = kernel-bin | rk-bootimg
  KERNEL_LOADADDR := 0x02008000
endef

define Device/herospeed_rv1126-imx415
  $(Device/rv1126)
  DEVICE_VENDOR := Herospeed
  DEVICE_MODEL := RV1126 IMX415 Base
  DEVICE_DTS := rv1126-herospeed-imx415
  DEVICE_DTS_DIR := ../dts
  DEVICE_PACKAGES := kmod-rtc-pcf8563 kmod-video-core kmod-video-imx415 \
	kmod-video-rockchip-cif kmod-phy-rockchip-inno-csidphy \
	kmod-leds-pwm-flash kmod-crypto-hw-rockchip2 kmod-video-hantro \
	video-support
  IMAGES := sysupgrade.bin factory-RV1126_IMX415.fls
  IMAGE/sysupgrade.bin := sysupgrade-tar | append-metadata
  IMAGE/factory-RV1126_IMX415.fls := herospeed-fls
  FLS_IC := RV1126
  FLS_SENSOR := IMX415
endef
TARGET_DEVICES += herospeed_rv1126-imx415


define Device/rk3506
  $(Device/arm32)
  SOC := rk3506
  BOOT_SCRIPT:= rk3506
  KERNEL_LOADADDR := 0x03200000
  DEVICE_DTS_LOADADDR := 0x02000000
endef

define Device/rk3506-nand
  $(Device/arm32)
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
  DEVICE_DTS := HZ-RK3506SP_MiniEVM_EMMC
  DEVICE_PACKAGES := kmod-usb-hid kmod-usb-ohci kmod-usb2 kmod-usb-storage \
    kmod-usb-storage-extras kmod-usb-net kmod-usb-core kmod-gpio-button-hotplug \
    urandom-seed
endef
TARGET_DEVICES += hzhy_mini_evm_emmc

define Device/hzhy_mini_evm_nand
  $(Device/rk3506)
  DEVICE_VENDOR := HZHY
  DEVICE_MODEL := RK3506SP MiniEVM (NAND)
  DEVICE_DTS := HZ-RK3506SP_MiniEVM_NAND
  DEVICE_PACKAGES := kmod-usb-hid kmod-usb-ohci kmod-usb2 kmod-usb-storage \
    kmod-usb-storage-extras kmod-usb-net kmod-usb-core kmod-gpio-button-hotplug \
    kmod-mtd-rw urandom-seed
endef
TARGET_DEVICES += hzhy_mini_evm_nand

define Device/hzhy_mini_evm_sd
  $(Device/rk3506)
  DEVICE_VENDOR := HZHY
  DEVICE_MODEL := RK3506SP MiniEVM (SD)
  DEVICE_DTS := HZ-RK3506SP_MiniEVM_SD
  DEVICE_PACKAGES := kmod-usb-hid kmod-usb-ohci kmod-usb2 kmod-usb-storage \
    kmod-usb-storage-extras kmod-usb-net kmod-usb-core kmod-gpio-button-hotplug \
    urandom-seed
  UBOOT_DEVICE_NAME := evb-rk3506
endef
TARGET_DEVICES += hzhy_mini_evm_sd

define Device/vanxoak_vx-hd-rk3506g-iot
  $(Device/rk3506)
  DEVICE_VENDOR := Vanxoak
  DEVICE_MODEL := HD-RK3506G-IOT (SD)
  # UBOOT_DEVICE_NAME := evb-rk3506
  DEVICE_PACKAGES:=kmod-gpio-button-hotplug \
    kmod-leds-gpio kmod-ledtrig-heartbeat \
    kmod-ledtrig-netdev kmod-ledtrig-gpio \
    kmod-rtc-ds1307 kmod-usb-net-asix \
    kmod-usb-serial kmod-usb-serial-option \
    kmod-usb-serial-qualcomm kmod-usb-net-qmi-wwan \
    kmod-usb-net-cdc-eem kmod-usb-net-cdc-ether \
    kmod-usb-net-cdc-mbim kmod-usb-net-rndis \
    kmod-usb-xhci-pci kmod-i2c-fusb30x \
    kmod-phy-motorcomm kmod-mmc \
    kmod-can-rockchip-rk3576 kmod-can-raw ip-full \
    kmod-ata-ahci kmod-ata-ahci-dwc \
    luci-app-freeioe luci-proto-qmi \
    luci-proto-3g luci-proto-wwan \
    tlink-r7-firmware wpad-basic-mbedtls \
	kmod-rtw88-8723du rtl8723bu-firmware \
	kmod-bluetooth rtl8723bu-bt-firmware \
    tinc-freeioe-tunnel iperf3 \
    uqmi freeioe wwan-watch \
    kmod-eeprom-at24
endef
TARGET_DEVICES += vanxoak_vx-hd-rk3506g-iot

define Device/vanxoak_vx-hd-rk3506g-iot-spinand
  $(Device/rk3506-nand)
  DEVICE_VENDOR := Vanxoak
  DEVICE_MODEL := HD-RK3506G-IOT (SPINAND)
  DEVICE_PACKAGES:=kmod-gpio-button-hotplug \
    kmod-leds-gpio kmod-ledtrig-heartbeat \
    kmod-ledtrig-netdev kmod-ledtrig-gpio \
    kmod-rtc-ds1307 kmod-usb-net-asix \
    kmod-usb-serial kmod-usb-serial-option \
    kmod-usb-serial-qualcomm kmod-usb-net-qmi-wwan \
    kmod-usb-net-cdc-eem kmod-usb-net-cdc-ether \
    kmod-usb-net-cdc-mbim kmod-usb-net-rndis \
    kmod-usb-xhci-pci kmod-i2c-fusb30x \
    kmod-phy-motorcomm kmod-mmc \
    kmod-can-rockchip-rk3576 kmod-can-raw ip-full \
    kmod-ata-ahci kmod-ata-ahci-dwc \
    luci-app-freeioe luci-proto-qmi \
    luci-proto-3g luci-proto-wwan \
    tlink-r7-firmware wpad-basic-mbedtls \
	kmod-rtw88-8723du rtl8723bu-firmware \
	kmod-bluetooth rtl8723bu-bt-firmware \
    tinc-freeioe-tunnel iperf3 \
    uqmi freeioe wwan-watch \
    kmod-eeprom-at24
endef
TARGET_DEVICES += vanxoak_vx-hd-rk3506g-iot-spinand

define Device/vanxoak_vx-hd-rk3506g-m-iot
  $(Device/rk3506)
  DEVICE_VENDOR := Vanxoak
  DEVICE_MODEL := HD-RK3506G-M-IOT (SD)
  DEVICE_PACKAGES:=kmod-gpio-button-hotplug \
    kmod-leds-gpio kmod-ledtrig-heartbeat \
    kmod-ledtrig-netdev kmod-ledtrig-gpio \
    kmod-rtc-hym8563 kmod-usb-net-asix \
    kmod-usb-serial kmod-usb-serial-option \
    kmod-usb-serial-qualcomm kmod-usb-net-qmi-wwan \
    kmod-usb-net-cdc-eem kmod-usb-net-cdc-ether \
    kmod-usb-net-cdc-mbim kmod-usb-net-rndis \
    kmod-usb-xhci-pci kmod-i2c-fusb30x \
    kmod-phy-motorcomm kmod-mmc \
    kmod-can-rockchip-rk3576 kmod-can-raw ip-full \
    kmod-ata-ahci kmod-ata-ahci-dwc \
	kmod-regulator-userspace-consumer \
    luci-app-freeioe luci-proto-qmi \
    luci-proto-3g luci-proto-wwan \
    tlink-r7-firmware wpad-basic-mbedtls \
	kmod-rtw88-8723du rtl8723bu-firmware \
    tinc-freeioe-tunnel iperf3 \
    uqmi freeioe wwan-watch \
    kmod-eeprom-at24
endef
TARGET_DEVICES += vanxoak_vx-hd-rk3506g-m-iot

define Device/vanxoak_vx-hd-rk3506g-m-iot-spinand
  $(Device/rk3506-nand)
  DEVICE_VENDOR := Vanxoak
  DEVICE_MODEL := HD-RK3506G-M-IOT (SPINAND)
  DEVICE_PACKAGES:=kmod-gpio-button-hotplug \
    kmod-leds-gpio kmod-ledtrig-heartbeat \
    kmod-ledtrig-netdev kmod-ledtrig-gpio \
    kmod-rtc-hym8563 kmod-usb-net-asix \
    kmod-usb-serial kmod-usb-serial-option \
    kmod-usb-serial-qualcomm kmod-usb-net-qmi-wwan \
    kmod-usb-net-cdc-eem kmod-usb-net-cdc-ether \
    kmod-usb-net-cdc-mbim kmod-usb-net-rndis \
    kmod-usb-xhci-pci kmod-i2c-fusb30x \
    kmod-phy-motorcomm kmod-mmc \
    kmod-can-rockchip-rk3576 kmod-can-raw ip-full \
    kmod-ata-ahci kmod-ata-ahci-dwc \
    luci-app-freeioe luci-proto-qmi \
    luci-proto-3g luci-proto-wwan \
    tlink-r7-firmware wpad-basic-mbedtls \
	kmod-rtw88-8723du rtl8723bu-firmware \
    tinc-freeioe-tunnel iperf3 \
    uqmi freeioe wwan-watch \
    kmod-eeprom-at24
endef
TARGET_DEVICES += vanxoak_vx-hd-rk3506g-m-iot-spinand

