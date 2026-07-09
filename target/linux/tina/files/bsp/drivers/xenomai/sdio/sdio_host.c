/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Driver for sunxi SD/MMC host controllers
 * (C) Copyright 2007-2011 Reuuimlla Technology Co., Ltd.
 * (C) Copyright 2007-2011 Aaron Maoye <leafy.myeh@reuuimllatech.com>
 * (C) Copyright 2013-2014 O2S GmbH <www.o2s.ch>
 * (C) Copyright 2013-2014 David Lanzendrfer <david.lanzendoerfer@o2s.ch>
 * (C) Copyright 2013-2014 Hans de Goede <hdegoede@redhat.com>
 * (C) Copyright 2014-2016 lixiang <lixiang@allwinnertech>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/err.h>

#include <linux/reset/sunxi.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>

#include <rtdm/driver.h>
#include <linux/mmc/card.h>
#include "./core-melis/sd_rtos_define.h"

#define RTDM_MMC_RESCAN			0
#define RTDM_MMC_DEATTACH		1
#define RTDM_MMC_START_BLOCK		2

#define RTDM_CLASS_SDIO			14
#define RTDM_SUBCLASS_SDIO		0
struct sunxi_sdio_context {
	struct mmc_card *rtdm_card;
	char init;
	unsigned int start_block;
};

/*
int rtdm_mmc_test_rw(struct mmc_card *card)
{
	uint8_t buf_1[1024] = {0x56};
	uint8_t *buf_2 = NULL;

	buf_2 = kmalloc(1024, GFP_KERNEL);
	buf_1[0] = 0x12;
	buf_1[1] = 0x34;
	printk("write - buf0=0x%x buf1=0x%x\n", buf_1[0], buf_1[1]);
	rtdm_mmc_block_write(card, buf_1, 0, 2);
	rtdm_mmc_block_read(card, buf_2, 0, 2);
	printk("read - buf0=0x%x buf1=0x%x\n", buf_2[0], buf_2[1]);

	buf_1[0] = 0x56;
	buf_1[1] = 0x78;
	printk("write - buf0=0x%x buf1=0x%x\n", buf_1[0], buf_1[1]);
	rtdm_mmc_block_write(card, buf_1, 0, 2);
	rtdm_mmc_block_read(card, buf_2, 0, 2);
	printk("read - buf0=0x%x buf1=0x%x\n", buf_2[0], buf_2[1]);

	buf_1[0] = 0xaa;
	buf_1[1] = 0xbb;
	printk("write - buf0=0x%x buf1=0x%x\n", buf_1[0], buf_1[1]);
	rtdm_mmc_block_write(card, buf_1, 0, 2);
	rtdm_mmc_block_read(card, buf_2, 0, 2);
	printk("read - buf0=0x%x buf1=0x%x\n", buf_2[0], buf_2[1]);

	kfree(buf_2);

	return 0;
}
*/

static ssize_t sunxi_sdio_read_rt(struct rtdm_fd *fd,
				  void __user *u_buf, size_t len)
{
	void *rx;
	int ret;
	struct sunxi_sdio_context *context = rtdm_fd_to_private(fd);
	struct mmc_card *card = context->rtdm_card;
	unsigned int sblk = context->start_block;


	if (context->init == 0) {
		printk("need init\n");
		return -1;
	}

	if (len == 0)
		return 0;

	rx = xnmalloc(len * 512);
	if (rx == NULL)
		return -ENOMEM;

	ret = rtdm_mmc_block_read(card, rx, sblk, len);
	if (!ret)
		ret = rtdm_safe_copy_to_user(fd, u_buf, rx, len * 512);

	xnfree(rx);

	return ret;
}

static ssize_t sunxi_sdio_write_rt(struct rtdm_fd *fd,
				   const void __user *u_buf, size_t len)
{
	void *tx;
	int ret;
	struct sunxi_sdio_context *context = rtdm_fd_to_private(fd);
	struct mmc_card *card = context->rtdm_card;
	unsigned int sblk = context->start_block;

	if (context->init == 0) {
		printk("need init\n");
		return -1;
	}

	if (len == 0)
		return 0;

	tx = xnmalloc(len * 512);
	if (tx == NULL)
		return -ENOMEM;

	ret = rtdm_safe_copy_from_user(fd, tx, u_buf, len * 512);
	if (ret == 0) {
		ret = rtdm_mmc_block_write(card, tx, sblk, len);
	}

	xnfree(tx);

	return ret;
}

static int sunxi_sdio_ioctl_rt(struct rtdm_fd *fd, unsigned int request, void *arg)
{
	struct sunxi_sdio_context *context = rtdm_fd_to_private(fd);
	struct mmc_card *card = context->rtdm_card;
	int32_t err = -1;

	switch (request) {
	case RTDM_MMC_RESCAN:
		err = rtdm_mmc_rescan(card);
		if (err) {
			context->init = 0;
			printk("rtdm_mmc_rescan error \n");
			err = -1;
		} else {
			context->init = 1;
			err = 0;
		}
		break;
	case RTDM_MMC_DEATTACH:
		rtdm_mmc_deattach_sd(card, card->host);
		err = 0;
		context->init = 0;
		break;
	case RTDM_MMC_START_BLOCK:
		context->start_block = (unsigned int)(uintptr_t)arg;
		err = 0;
		break;
	default:
		break;
	}

	return err;
}

static int sunxi_sdio_open(struct rtdm_fd *fd, int oflags)
{
	struct rtdm_device *dev = rtdm_fd_device(fd);
	struct sunxi_sdio_context *context = rtdm_fd_to_private(fd);

	context->rtdm_card = kmalloc(sizeof(struct mmc_card), GFP_KERNEL);
	context->rtdm_card->host = dev->device_data;
	context->start_block = 0;

	return 0;
}

static void sunxi_sdio_close(struct rtdm_fd *fd)
{
	struct sunxi_sdio_context *context = rtdm_fd_to_private(fd);
	kfree(context->rtdm_card);

	return;
}

static struct rtdm_driver sunxi_sdio_driver_rtdm = {
	.profile_info		=	RTDM_PROFILE_INFO(sdio,
							  RTDM_CLASS_SDIO,
							  RTDM_SUBCLASS_SDIO,
							  0),
	.device_flags		=	RTDM_NAMED_DEVICE | RTDM_EXCLUSIVE,
	.device_count		=	1,
	.context_size		=	sizeof(struct sunxi_sdio_context),
	.ops = {
		.open		=	sunxi_sdio_open,
		.close		=	sunxi_sdio_close,
		.read_rt	=	sunxi_sdio_read_rt,
		.write_rt	=	sunxi_sdio_write_rt,
		.ioctl_rt	=	sunxi_sdio_ioctl_rt,
	},
};

static struct rtdm_device sdio_device = {
	.driver = &sunxi_sdio_driver_rtdm,
	.label = "sdio",
};

extern int HAL_SDC_probe(struct platform_device *pdev);
static int sunxi_mmc_probe_rtdm(struct platform_device *pdev)
{
	s32 ret = 0;

	ret = HAL_SDC_probe(pdev);
	if (ret) {
		return ret;
	}
	sdio_device.device_data = platform_get_drvdata(pdev);
	rtdm_dev_register(&sdio_device);
	return 0;
}

static int sunxi_mmc_remove_rtdm(struct platform_device *pdev)
{
	rtdm_dev_unregister(&sdio_device);
	return 0;
}

static const struct of_device_id sunxi_mmc_of_match_rtdm[] = {
	{.compatible = "allwinner,sunxi-mmc-v4p1x-rtdm",},
	{ /* sentinel */ }
};

MODULE_DEVICE_TABLE(of, sunxi_mmc_of_match_rtdm);

static struct platform_driver sunxi_mmc_driver_rtdm = {
	.driver = {
		   .name = "sunxi-mmc-xenomai-sdio",
		   .owner = THIS_MODULE,
		   .of_match_table = of_match_ptr(sunxi_mmc_of_match_rtdm),
		   },
	.probe = sunxi_mmc_probe_rtdm,
	.remove = sunxi_mmc_remove_rtdm,
};

module_platform_driver(sunxi_mmc_driver_rtdm);

MODULE_DESCRIPTION("Allwinner's SD/MMC Card Controller Driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Allwinner BTD");
MODULE_ALIAS("platform:sunxi-mmc");
MODULE_VERSION("1.0.0");
