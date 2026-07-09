/* SPDX-License-Identifier: GPL-2.0-or-later */

/* Copyright(c) 2020 - 2025 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * ddr ecc driver
 *
 * Copyright(c) 2015-2025 Allwinnertech Co., Ltd.
 *      http://www.allwinnertech.com
 *
 * Author: fanqinghua <fanqinghua@allwinnertech.com>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <sunxi-log.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/of_device.h>

#define DEVICE_NAME "io_driver"

#define MC_SRAM_ECC			0x30
#define MC_ECC_INJ_DATA(n)	(0x30 + (n) * 4)
#define MC_ECC_INJ_STA(n)	(0x44 + (n) * 4)
#define MC_ECC_ORI_DATA(n)	(0x80 + (n) * 4)
#define MX_SWCTL			0x10320
#define MX_ECCSTAT			0x10078
#define MX_ECCCLR			0x1007C

#define DRIVER_NAME	"ddrecc Driver"

static inline void clrsetbitsl(volatile void __iomem *addr, u32 mask, u32 set)
{
	writel((readl(addr) & ~mask) | (set & mask), addr);
}

struct sunxi_ddrecc {
	struct device *dev;
	struct kobject *io_kobj;
	void __iomem *common_base;
	int inlinecc_irq;
	int sramecc_irq;
};

static irqreturn_t sunxi_inlinecc_isr(int irq, void *data)
{
	struct sunxi_ddrecc *ddrecc = data;
	struct device *dev = ddrecc->dev;
	unsigned int reg_val = 0, ecc_uncorrected_err, ecc_corrected_err, ecc_corrected_bit;

	writel(0, ddrecc->common_base + MX_SWCTL);
	reg_val = readl(ddrecc->common_base + MX_ECCSTAT);
	sunxi_err(dev, "MX_ECCSTAT:0x%x\n", reg_val);

	ecc_uncorrected_err = (reg_val >> 16) & 0xffff;
	ecc_corrected_err	= (reg_val >> 8) & 0x00ff;
	ecc_corrected_bit	= (reg_val >> 0) & 0x007f;

	/* lock	0;unlock 1;*/
	clrsetbitsl(ddrecc->common_base + MX_ECCCLR, (0x3 << 8), 0);
	clrsetbitsl(ddrecc->common_base + MX_ECCCLR, 0x1f, -1U);

	writel(1, ddrecc->common_base + MX_SWCTL);

	if (ecc_uncorrected_err)
		sunxi_err(dev, "inline_ecc_uncorrected_err:%d\n", ecc_uncorrected_err);

	if (ecc_corrected_err) {
		sunxi_err(dev, "inline_ecc_corrected_err:%d\n", ecc_corrected_err);
		sunxi_err(dev, "ecc_corrected_bit:%d\n", ecc_corrected_bit);
	}

	writel(0, ddrecc->common_base + MX_SWCTL);
	clrsetbitsl(ddrecc->common_base + MX_ECCCLR, (0x3 << 8), -1U);
	writel(1, ddrecc->common_base + MX_SWCTL);

	return IRQ_HANDLED;
}

static irqreturn_t sunxi_sramecc_isr(int irq, void *data)
{
	struct sunxi_ddrecc *ddrecc = data;
	struct device *dev = ddrecc->dev;
	unsigned int reg_val = 0, i = 0;

	/* close irq enable */
	clrsetbitsl(ddrecc->common_base + MC_SRAM_ECC, (0x1 << 2), 0);

	for (i = 1; i <= 4; i++) {
		reg_val = readl(ddrecc->common_base + MC_ECC_INJ_DATA(i));
		sunxi_err(dev, "MC_ECC_INJ_DATA%d:0x%x\n", i, reg_val);
	}

	for (i = 0; i < 16; i++) {
		reg_val = readl(ddrecc->common_base + MC_ECC_INJ_STA(i));
		sunxi_err(dev, "MC_ECC_INJ_STA%d:0x%x\n", i, reg_val);
	}

	for (i = 1; i <= 4; i++) {
		reg_val = readl(ddrecc->common_base + MC_ECC_ORI_DATA(i));
		sunxi_err(dev, "MC_ECC_ORI_DATA%d:0x%x\n", i, reg_val);
	}
	/* clear ecc status */
	clrsetbitsl(ddrecc->common_base + MC_SRAM_ECC, (0x1 << 0), -1U);
	clrsetbitsl(ddrecc->common_base + MC_SRAM_ECC, (0x1 << 0), 0);
	clrsetbitsl(ddrecc->common_base + MC_SRAM_ECC, (0x1 << 2), -1U);

	return IRQ_HANDLED;
}

static void __iomem *io_virt_addrs;
static resource_size_t io_phy_addrs;

static ssize_t address_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "0x%llx\n", (unsigned long long)io_phy_addrs);
}

static ssize_t address_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned long long new_addr;

	if (kstrtoull(buf, 0, &new_addr) != 0)
		return -EINVAL;

	if (io_virt_addrs) {
		iounmap(io_virt_addrs);
		io_virt_addrs = NULL;
	}
	io_phy_addrs = new_addr;
	io_virt_addrs = ioremap(io_phy_addrs, PAGE_SIZE);
	if (!io_virt_addrs) {
		pr_err("Failed to map physical address 0x%llx\n", io_phy_addrs);
		return -ENOMEM;
	}
	return count;
}

static ssize_t read_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	if (!io_virt_addrs)
		return -EIO;

	return snprintf(buf, PAGE_SIZE, "0x%x\n", readl(io_virt_addrs));
}

static ssize_t write_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	uint32_t value;

	if (!io_virt_addrs)
		return -EIO;

	if (kstrtou32(buf, 0, &value) != 0)
		return -EINVAL;

	writel(value, io_virt_addrs);
	return count;
}

static struct kobj_attribute address_attr =
	__ATTR(address, 0660, address_show, address_store);

static struct kobj_attribute read_attr =
	__ATTR(read, 0440, read_show, NULL);

static struct kobj_attribute write_attr =
	__ATTR(write, 0220, NULL, write_store);

static struct attribute *ddrecc_attrs[] = {
	&address_attr.attr,
	&read_attr.attr,
	&write_attr.attr,
	NULL,
};

static struct attribute_group ddrecc_attr_group = {
	.attrs = ddrecc_attrs,
};

static const struct of_device_id sunxi_ddrecc_match[] = {
	{ .compatible = "allwinner,sun55iw6-ddrecc"},
	{},
};
MODULE_DEVICE_TABLE(of, sunxi_ddrecc_match);

static int sunxi_io_driver_init(struct device *dev)
{
	struct sunxi_ddrecc *ddrecc = dev_get_drvdata(dev);
	int ret;

	ddrecc->io_kobj = kobject_create_and_add(DEVICE_NAME, kernel_kobj);
	if (!ddrecc->io_kobj)
		return -ENOMEM;

	ret = sysfs_create_group(ddrecc->io_kobj, &ddrecc_attr_group);
	if (ret) {
		kobject_put(ddrecc->io_kobj);
		return ret;
	}

	return 0;
}

static void sunxi_io_driver_exit(struct device *dev)
{
	struct sunxi_ddrecc *ddrecc = dev_get_drvdata(dev);

	sysfs_remove_group(ddrecc->io_kobj, &ddrecc_attr_group);
	kobject_put(ddrecc->io_kobj);

	if (io_virt_addrs) {
		iounmap(io_virt_addrs);
		io_virt_addrs = NULL;
	}
}


static int sunxi_ddrecc_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;
	struct sunxi_ddrecc *ddrecc;
	int rc = 0;

	ddrecc = devm_kzalloc(dev, sizeof(*ddrecc), GFP_KERNEL);
	if (!ddrecc)
		return -ENOMEM;

	ddrecc->common_base = devm_of_iomap(&pdev->dev, np, 0, NULL);
	if (IS_ERR(ddrecc->common_base)) {
		sunxi_err(&pdev->dev, "devm_ioremap_resource error!\n");
		return PTR_ERR(ddrecc->common_base);
	}

	ddrecc->inlinecc_irq = platform_get_irq(pdev, 0);
	if (ddrecc->inlinecc_irq < 0) {
		sunxi_err(&pdev->dev, "inlinecc_irq get error!\n");
		return ddrecc->inlinecc_irq;
	}

	ddrecc->sramecc_irq = platform_get_irq(pdev, 1);
	if (ddrecc->sramecc_irq < 0) {
		sunxi_err(&pdev->dev, "sramecc_irq get error!\n");
		return ddrecc->sramecc_irq;
	}

	rc = devm_request_threaded_irq(&pdev->dev, ddrecc->inlinecc_irq, NULL,
					sunxi_inlinecc_isr, IRQF_ONESHOT,
					"sunxi-inlinecc", ddrecc);
	if (rc) {
		sunxi_err(&pdev->dev, "Inlinecc interrupt request failed: %d\n", rc);
		return rc;
	}

	rc = devm_request_threaded_irq(&pdev->dev, ddrecc->sramecc_irq, NULL,
					sunxi_sramecc_isr, IRQF_ONESHOT,
					"sunxi-sramecc", ddrecc);
	if (rc) {
		sunxi_err(&pdev->dev, "Sramecc interrupt request failed: %d\n", rc);
		return rc;
	}

	ddrecc->dev = dev;
	platform_set_drvdata(pdev, ddrecc);

	rc = sunxi_io_driver_init(&pdev->dev);
	if (rc) {
		sunxi_err(&pdev->dev, "Inlinecc interrupt request failed: %d\n", rc);
		return rc;
	}

	return rc;
}

static int sunxi_ddrecc_remove(struct platform_device *pdev)
{
	sunxi_io_driver_exit(&pdev->dev);
	return 0;
}

static struct platform_driver sunxi_ddrecc_driver = {
	.probe  = sunxi_ddrecc_probe,
	.remove  = sunxi_ddrecc_remove,
	.driver = {
		.name = "sunxi-ddrecc",
		.of_match_table = sunxi_ddrecc_match,
	},
};

module_platform_driver(sunxi_ddrecc_driver);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("SUNXI ddrecc driver");
MODULE_ALIAS("platform:" DRIVER_NAME);
MODULE_AUTHOR("fanqinghua <fanqinghua@allwinnertech.com>");
MODULE_VERSION("2.0.0");
