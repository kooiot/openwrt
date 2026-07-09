# SPDX-License-Identifier: GPL-2.0-or-later

obj-y += lowlevel_common.o

obj-y += platform/

obj-$(CONFIG_AW_GPIO_ISOLATION_DRIVER) += gpio/
obj-$(CONFIG_AW_DMA_ISOLATION_DRIVER) += dma/

# RTOS
ccflags-y += -I drivers/rtos-hal/hal/source/amp_rsc/core/include

# Linux and U-Boot
ccflags-y += -I $(srctree)/bsp/drivers/amp_rsc/core/include
