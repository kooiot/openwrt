# SPDX-License-Identifier: GPL-2.0
#
#  Makefile for the Linux Controller Area Network drivers.
#


ccflags-y +=  -I $(srctree)/drivers/xenomai/can/

obj-$(CONFIG_AW_XENO_CAN_SUN55I)		+= rtdm_sunxican.o
obj-$(CONFIG_AW_XENO_CAN_SUN8I)		+= rtdm_sunxican.o

rtdm_sunxican-objs := sunxi_can_plat_rtdm.o sunxi_can_rtdm.o

