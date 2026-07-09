/* SPDX-License-Identifier: GPL-2.0 */
/*
 * sunxi's rproc boot reason dev driver
 * register or unregister boot reason device for rproc.
 *
 * Copyright (C) 2023 Allwinnertech - All Rights Reserved
 *
 * Author: shihongfu <shihongfu@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SUNXI_RPROC_BOOT_REASON_DEV_H__
#define __SUNXI_RPROC_BOOT_REASON_DEV_H__

#include "sunxi_boot_reason.h"

#define SUNXI_RPROC_BOOT_REASON_CLASS
#define SUNXI_RPROC_BOOT_REASON_SUBDEV

#include <linux/device.h>
#include <linux/remoteproc.h>

struct sunxi_rproc_boot_reason_dev {
	struct device dev;
#ifdef SUNXI_RPROC_BOOT_REASON_SUBDEV
	struct rproc_subdev subdev;
#endif
	//struct sunxi_rproc_boot_reason boot_reason;
	struct rproc *rproc;

	void (*on_release)(void *priv);
	void *on_release_priv;

	void __iomem *rtc_base;
	unsigned int data_idx;
#ifdef SUNXI_RPROC_BOOT_REASON_CLASS
	unsigned int force;
#endif
};

struct sunxi_rproc_boot_reason_dev_res {
	struct resource reg_res;
	u32 data_idx;
};

int sunxi_rproc_boot_reason_dev_register(struct sunxi_rproc_boot_reason_dev *wdt_dev, struct rproc *rproc,
					 struct sunxi_rproc_boot_reason_dev_res *res);
int sunxi_rproc_boot_reason_dev_unregister(struct sunxi_rproc_boot_reason_dev *wdt_dev);
int sunxi_rproc_boot_reason_dev_set_release(struct sunxi_rproc_boot_reason_dev *wdt_dev,
					    void (*release)(void *), void *priv);
int sunxi_rproc_boot_reason_dev_set_parent(struct sunxi_rproc_boot_reason_dev *wdt_dev, struct device *parent);

#endif /* __SUNXI_RPROC_BOOT_REASON_DEV_H__ */