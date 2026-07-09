// SPDX-License-Identifier: GPL-2.0
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

#include "sunxi_rproc_boot_reason_dev.h"
#include <linux/pm_domain.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/version.h>

#define dev_to_boot_reason_dev(_dev)		container_of(_dev, struct sunxi_rproc_boot_reason_dev, dev)
#define subdev_to_boot_reason_dev(_subdev)	container_of(_subdev, struct sunxi_rproc_boot_reason_dev, subdev)

static inline void sunxi_rproc_boot_reason_trace(struct sunxi_rproc_boot_reason_dev *boot_reason_dev, const char *event)
{
	struct device *dev = &boot_reason_dev->dev;

	dev_dbg(dev, "%s\n", event);
}

#ifdef SUNXI_RPROC_BOOT_REASON_CLASS
static ssize_t force_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sunxi_rproc_boot_reason_dev *boot_reason_dev = dev_to_boot_reason_dev(dev);

	return sprintf(buf, "%u\n", boot_reason_dev->force);
}

static ssize_t force_store(struct device *dev, struct device_attribute *attr,
			       const char *buf, size_t count)
{
	struct sunxi_rproc_boot_reason_dev *boot_reason_dev = dev_to_boot_reason_dev(dev);
	long val;

	if (kstrtol(buf, 10, &val) < 0)
		return -EINVAL;

	if (val < 0)
		return -EINVAL;

	if (val != 0)
		val = 1;

	boot_reason_dev->force = val;

	return count;
}

static ssize_t boot_reason_id_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sunxi_rproc_boot_reason_dev *boot_reason_dev = dev_to_boot_reason_dev(dev);
	enum boot_reason_t reason = get_boot_reason(boot_reason_dev->rtc_base, boot_reason_dev->data_idx);

	return sprintf(buf, "%u\n", (unsigned int)reason);
}

static ssize_t boot_reason_id_store(struct device *dev, struct device_attribute *attr,
			       const char *buf, size_t count)
{
	struct sunxi_rproc_boot_reason_dev *boot_reason_dev = dev_to_boot_reason_dev(dev);
	long val;
	enum boot_reason_t reason;

	if (kstrtol(buf, 10, &val) < 0)
		return -EINVAL;

	if (val <= 0)
		return -EINVAL;

	if (val > BOOT_REASON_MCU_CLEAR)
		return -EINVAL;

	reason = val;
	if (boot_reason_dev->force)
		set_boot_reason_force(boot_reason_dev->rtc_base, boot_reason_dev->data_idx, reason);
	else
		set_boot_reason(boot_reason_dev->rtc_base, boot_reason_dev->data_idx, reason);

	return count;
}

static ssize_t boot_reason_str_show(struct device *dev, struct device_attribute *attr,
				       char *buf)
{
	struct sunxi_rproc_boot_reason_dev *boot_reason_dev = dev_to_boot_reason_dev(dev);
	enum boot_reason_t reason = get_boot_reason(boot_reason_dev->rtc_base, boot_reason_dev->data_idx);

	return sprintf(buf, "%s\n", boot_reason_str(reason));
}

static ssize_t boot_reason_str_store(struct device *dev, struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct sunxi_rproc_boot_reason_dev *boot_reason_dev = dev_to_boot_reason_dev(dev);
	unsigned int i;
	enum boot_reason_t reason;

	for (i = 0; i < (BOOT_REASON_MCU_CLEAR + 1); i++) {
		if (sysfs_streq(buf, boot_reason_str(i)))
			goto set_val;
	}
	return -EINVAL;
set_val:
	reason = i;
	if (boot_reason_dev->force)
		set_boot_reason_force(boot_reason_dev->rtc_base, boot_reason_dev->data_idx, reason);
	else
		set_boot_reason(boot_reason_dev->rtc_base, boot_reason_dev->data_idx, reason);
	return count;
}

static ssize_t writer_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sunxi_rproc_boot_reason_dev *boot_reason_dev = dev_to_boot_reason_dev(dev);
	enum writer_t writer;

	get_boot_reason_with_writer(boot_reason_dev->rtc_base, boot_reason_dev->data_idx, &writer);
	return sprintf(buf, "%u\n", (unsigned int)writer);
}

static DEVICE_ATTR_RW(force);
static DEVICE_ATTR_RW(boot_reason_id);
static DEVICE_ATTR_RW(boot_reason_str);
static DEVICE_ATTR_RO(writer);

static struct attribute *boot_reason_attrs[] = {
	&dev_attr_force.attr,
	&dev_attr_boot_reason_id.attr,
	&dev_attr_boot_reason_str.attr,
	&dev_attr_writer.attr,
	NULL
};

static const struct attribute_group boot_reason_devgroup = {
	.attrs = boot_reason_attrs
};

static const struct attribute_group *boot_reason_devgroups[] = {
	&boot_reason_devgroup,
	NULL
};

static int boot_reason_uevent(struct device *dev, struct kobj_uevent_env *env)
{
	return 0;
}

static struct class boot_reason_class = {
	.name		= "rproc_boot_reason",
	.dev_groups	= boot_reason_devgroups,
	.dev_uevent	= boot_reason_uevent,
};

int boot_reason_class_init;

static inline struct class *get_boot_reason_class(void)
{
	int ret;

	if (boot_reason_class_init)
		return &boot_reason_class;

	ret = class_register(&boot_reason_class);
	if (ret) {
		pr_err("rproc_boot_reason: unable to register class\n");
		return NULL;
	}

	boot_reason_class_init = 1;
	return &boot_reason_class;
}
#endif

#if IS_ENABLED(CONFIG_PM)
static int sunxi_rproc_boot_reason_dev_resume(struct device *dev)
{
	struct sunxi_rproc_boot_reason_dev *boot_reason_dev = dev_to_boot_reason_dev(dev);

	sunxi_rproc_boot_reason_trace(boot_reason_dev, "resume");
	return 0;
}

static int sunxi_rproc_boot_reason_dev_suspend(struct device *dev)
{
	struct sunxi_rproc_boot_reason_dev *boot_reason_dev = dev_to_boot_reason_dev(dev);

	sunxi_rproc_boot_reason_trace(boot_reason_dev, "suspend");
	return 0;
}

static struct dev_pm_domain pm_domain = {
	.ops = {
		.runtime_suspend = sunxi_rproc_boot_reason_dev_suspend,
		.runtime_resume = sunxi_rproc_boot_reason_dev_resume,
	},
};
#endif

static void sunxi_rproc_boot_reason_release(struct device *dev)
{
	struct sunxi_rproc_boot_reason_dev *boot_reason_dev = dev_to_boot_reason_dev(dev);

	sunxi_rproc_boot_reason_trace(boot_reason_dev, "release");

	if (boot_reason_dev->on_release)
		boot_reason_dev->on_release(boot_reason_dev->on_release_priv);
}

static inline int sunxi_rproc_boot_reason_dev_init(struct sunxi_rproc_boot_reason_dev *boot_reason_dev)
{
	int ret;
	struct device *dev = &boot_reason_dev->dev;

	device_initialize(dev);
	// the parent has not been init yet, this dev will be moved to the parent later.
	//dev->parent = &boot_reason_dev->rproc->dev;

#if IS_ENABLED(CONFIG_PM)
	dev_pm_domain_set(dev, &pm_domain);
#endif

	dev_set_name(dev, "%s-boot_reason", dev_name(&boot_reason_dev->rproc->dev));
	dev->release = sunxi_rproc_boot_reason_release;

#ifdef SUNXI_RPROC_BOOT_REASON_CLASS
	dev->class = get_boot_reason_class();
#endif

	ret = device_add(dev);
	if (ret < 0) {
		dev_err(&boot_reason_dev->rproc->dev, "device_add failed, ret: %d\n", ret);
		put_device(dev);
	}

	return ret;
}

#ifdef SUNXI_RPROC_BOOT_REASON_SUBDEV
static int sunxi_rproc_boot_reason_subdev_prepare(struct rproc_subdev *subdev)
{
	struct sunxi_rproc_boot_reason_dev *boot_reason_dev = subdev_to_boot_reason_dev(subdev);

	sunxi_rproc_boot_reason_trace(boot_reason_dev, "prepare");
	return 0;
}

static int sunxi_rproc_boot_reason_subdev_start(struct rproc_subdev *subdev)
{
	struct sunxi_rproc_boot_reason_dev *boot_reason_dev = subdev_to_boot_reason_dev(subdev);

	sunxi_rproc_boot_reason_trace(boot_reason_dev, "start");
	return 0;
}

static void sunxi_rproc_boot_reason_subdev_stop(struct rproc_subdev *subdev, bool crashed)
{
	struct sunxi_rproc_boot_reason_dev *boot_reason_dev = subdev_to_boot_reason_dev(subdev);
	enum boot_reason_t reason = crashed ? BOOT_REASON_MCU_WDT_TO : BOOT_REASON_USER_STOP;
	enum writer_t writer;
	enum boot_reason_t last;

	sunxi_rproc_boot_reason_trace(boot_reason_dev, crashed ? "stop crashed" : "stop normal");

	last = get_boot_reason_with_writer(boot_reason_dev->rtc_base, boot_reason_dev->data_idx, &writer);
	if (boot_reason_priority(last) > boot_reason_priority(reason) && writer != WRITER_LINUX)
			reason = last;

	set_boot_reason_force(boot_reason_dev->rtc_base, boot_reason_dev->data_idx, reason);
}

static void sunxi_rproc_boot_reason_subdev_unprepare(struct rproc_subdev *subdev)
{
	struct sunxi_rproc_boot_reason_dev *boot_reason_dev = subdev_to_boot_reason_dev(subdev);

	sunxi_rproc_boot_reason_trace(boot_reason_dev, "unprepare");
}
#endif

static inline void sunxi_rproc_boot_reason_dev_cleanup(struct sunxi_rproc_boot_reason_dev *boot_reason_dev)
{
	memset(boot_reason_dev, 0, sizeof(*boot_reason_dev));
}

int sunxi_rproc_boot_reason_dev_register(struct sunxi_rproc_boot_reason_dev *boot_reason_dev,
				 struct rproc *rproc, struct sunxi_rproc_boot_reason_dev_res *res)
{
	struct device *dev = &rproc->dev;
	int ret;

	sunxi_rproc_boot_reason_dev_cleanup(boot_reason_dev);
	boot_reason_dev->rproc = rproc;

	boot_reason_dev->data_idx = res->data_idx;
	boot_reason_dev->rtc_base = ioremap(res->reg_res.start, resource_size(&res->reg_res));
	if (!boot_reason_dev->rtc_base) {
		dev_err(dev, "ioremap failed!");
		ret = -EIO;
		goto err_out;
	}

	ret = sunxi_rproc_boot_reason_dev_init(boot_reason_dev);
	if (ret) {
		dev_err(dev, "sunxi_rproc_boot_reason_dev_init failed! ret: %d", ret);
		goto err_out;
	}

#ifdef SUNXI_RPROC_BOOT_REASON_SUBDEV
	boot_reason_dev->subdev.prepare = sunxi_rproc_boot_reason_subdev_prepare;
	boot_reason_dev->subdev.start = sunxi_rproc_boot_reason_subdev_start;
	boot_reason_dev->subdev.stop = sunxi_rproc_boot_reason_subdev_stop;
	boot_reason_dev->subdev.unprepare = sunxi_rproc_boot_reason_subdev_unprepare;
#endif

	sunxi_rproc_boot_reason_trace(boot_reason_dev, "register");
	return 0;
err_out:
	return ret;
}
EXPORT_SYMBOL(sunxi_rproc_boot_reason_dev_register);

int sunxi_rproc_boot_reason_dev_unregister(struct sunxi_rproc_boot_reason_dev *boot_reason_dev)
{
	struct device *dev = &boot_reason_dev->dev;

	sunxi_rproc_boot_reason_trace(boot_reason_dev, "unregister");

	if (boot_reason_dev->rtc_base) {
		iounmap(boot_reason_dev->rtc_base);
		boot_reason_dev->rtc_base = NULL;
	}

	device_del(dev);
	put_device(dev);

	return 0;
}
EXPORT_SYMBOL(sunxi_rproc_boot_reason_dev_unregister);

int sunxi_rproc_boot_reason_dev_set_release(struct sunxi_rproc_boot_reason_dev *boot_reason_dev,
					    void (*release)(void *), void *priv)
{
	boot_reason_dev->on_release = release;
	boot_reason_dev->on_release_priv = priv;

	return 0;
}
EXPORT_SYMBOL(sunxi_rproc_boot_reason_dev_set_release);

int sunxi_rproc_boot_reason_dev_set_parent(struct sunxi_rproc_boot_reason_dev *boot_reason_dev, struct device *parent)
{
	return device_move(&boot_reason_dev->dev, parent, DPM_ORDER_PARENT_BEFORE_DEV);
}
EXPORT_SYMBOL(sunxi_rproc_boot_reason_dev_set_parent);
