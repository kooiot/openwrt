
define Device/milkv_duo
  $(call Device/Default)
  DEVICE_VENDOR := MilkV
  DEVICE_MODEL := Duo
  DEVICE_DTS := sophgo/cv1800b-milkv-duo
  UBOOT := milkv_duo
endef
TARGET_DEVICES += milkv_duo

define Device/milkv_duos
  $(call Device/Default)
  DEVICE_VENDOR := MilkV
  DEVICE_MODEL := DuoS
  DEVICE_DTS := sophgo/cv1813h-milkv-duos
  UBOOT := milkv_duos
endef
# TARGET_DEVICES += milkv_duos

define Device/milkv_duo_256m
  $(call Device/Default)
  DEVICE_VENDOR := MilkV
  DEVICE_MODEL := Duo 256M
  DEVICE_DTS := sophgo/cv1813h-milkv-duo-256m
  UBOOT := milkv_duo_256m
endef
# TARGET_DEVICES += milkv_duo_256m

