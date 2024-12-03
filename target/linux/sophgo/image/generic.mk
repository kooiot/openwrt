
define Device/milkv_duo
  $(call Device/Default)
  DEVICE_VENDOR := MilkV
  DEVICE_MODEL := MilkV Duo
  DEVICE_DTS := sophgo/cv1800b-milkv-duo
  UBOOT := milkv_duo
endef
TARGET_DEVICES += milkv_duo

