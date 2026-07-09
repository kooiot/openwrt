/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
* Sunxi SD/MMC host driver
*
* Copyright (C) 2015 AllWinnertech Ltd.
* Author: lixiang <lixiang@allwinnertech>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed "as is" WITHOUT ANY WARRANTY of any
* kind, whether express or implied; without even the implied warranty
* of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*/


#include <linux/clk.h>
#include <linux/reset/sunxi.h>

#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/reset.h>
#include <linux/interrupt.h>

#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>

#include <linux/mmc/host.h>
#include <linux/mmc/sd.h>
#include <linux/mmc/sdio.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/core.h>
#include <linux/mmc/card.h>
#include <linux/mmc/slot-gpio.h>


#ifndef __SUNXI_MMC_INTERFACE_H__
#define __SUNXI_MMC_INTERFACE_H__

extern int HAL_SDC_Update_Clk(struct mmc_host *host, uint32_t clk);
//extern int HAL_SDC_Clk_PWR_Opt(struct mmc_host *host, uint32_t oclk_en, uint32_t pwr_save);
//extern int HAL_SDC_signal_voltage_switch(struct mmc_host *host, gpio_power_mode_t signal_voltage);
extern int HAL_SDC_PowerOn(struct mmc_host *host);
extern int HAL_SDC_PowerOff(struct mmc_host *host);
extern int HAL_SDC_Request(struct mmc_host *host, struct mmc_request *mrq);
extern void HAL_SDC_Enable_Sdio_Irq(struct mmc_host *host, int enable);
extern void HAL_SDC_set_timing(struct mmc_host *host, unsigned int timing);
extern struct mmc_host *HAL_SDC_get_mmc_host(void);
extern void HAL_SDC_Set_BusWidth(struct mmc_host *host, uint32_t width);

#endif
