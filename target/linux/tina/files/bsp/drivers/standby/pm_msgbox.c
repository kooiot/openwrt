/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 *
 * Copyright (c) 2012 Allwinner.
 * 2012-05-01 Written by sunny (sunny@allwinnertech.com).
 * 2012-10-01 Written by superm (superm@allwinnertech.com).
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <sunxi-log.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/suspend.h>
#include <asm/io.h>
#include <linux/mailbox_client.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/completion.h>

static u32 set_dram_refresh;
static u32 set_sram_suspend;

static int send_pm_set_dram_refresh(u32 en);
static int send_pm_set_sram_suspend(u32 en);

struct pm_msgbox_dev {
	struct device *dev;
	struct mbox_chan *chan;
	struct mbox_client client;
};
static struct mbox_chan *pm_msgbox_chan;

enum pm_msg_type {
	PM_SET_DRAM_REFRESH     = 0x80,
	PM_SET_SRAM_SUSPEND     = 0x81,
};

#define PM_MSGBOX_NAME	"pm-msgbox"

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
static ssize_t set_dram_refresh_show(struct class *class, struct class_attribute *attr,
		char *buf)
#else
static ssize_t set_dram_refresh_show(const struct class *class, const struct class_attribute *attr,
		char *buf)
#endif
{
	ssize_t size = 0;

	size = sprintf(buf, "Last setting: %u, echo 1/0 to set dram refresh/normal\n", set_dram_refresh);

	return size;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
static ssize_t set_dram_refresh_store(struct class *class, struct class_attribute *attr,
		const char *buf, size_t count)
#else
static ssize_t set_dram_refresh_store(const struct class *class, const struct class_attribute *attr,
		const char *buf, size_t count)
#endif
{
	u32 value = 0;
	u32 ret = 0;

	ret = kstrtoint(buf, 10, &value);
	if (ret) {
		sunxi_err(NULL, "%s,%d err, invalid para!\n", __func__, __LINE__);
		return -EINVAL;
	}

	set_dram_refresh = value ? 1:0;

	ret = send_pm_set_dram_refresh(set_dram_refresh);
	return count;
}
static CLASS_ATTR_RW(set_dram_refresh);

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
static ssize_t set_sram_suspend_show(struct class *class, struct class_attribute *attr,
		char *buf)
#else
static ssize_t set_sram_suspend_show(const struct class *class, const struct class_attribute *attr,
		char *buf)
#endif
{
	ssize_t size = 0;

	size = sprintf(buf, "Last setting: %u, echo 1/0 to set sram suspend/keep running\n", set_sram_suspend);

	return size;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
static ssize_t set_sram_suspend_store(struct class *class, struct class_attribute *attr,
		const char *buf, size_t count)
#else
static ssize_t set_sram_suspend_store(const struct class *class, const struct class_attribute *attr,
		const char *buf, size_t count)
#endif
{
	u32 value = 0;
	u32 ret = 0;

	if (!set_dram_refresh) {
		sunxi_err(NULL, "please set dram refresh mode first\n");
		return -EINVAL;
	}

	ret = kstrtoint(buf, 10, &value);
	if (ret) {
		sunxi_err(NULL, "%s,%d err, invalid para!\n", __func__, __LINE__);
		return -EINVAL;
	}

	set_sram_suspend = value ? 1:0;

	ret = send_pm_set_sram_suspend(set_sram_suspend);
	return count;
}
static CLASS_ATTR_RW(set_sram_suspend);

static struct attribute *pm_msgbox_class_attrs[] = {
	&class_attr_set_dram_refresh.attr,
	&class_attr_set_sram_suspend.attr,
	NULL,
};
ATTRIBUTE_GROUPS(pm_msgbox_class);

#define CHAN_RECV_BUFF_SIZE                     (20)
#define CHAN_RECV_ACK_TIMEOUT                   (500)
#define ARISC_MESSAGE_INITIALIZED               (0x02)
#define ARISC_MESSAGE_ATTR_SOFTSYN              (1 << 0)
#define ARISC_MESSAGE_ATTR_HARDSYN              (1 << 1)
#define HEAD_COMBINE(result, type, attr, state) (((u32)result << 24) | ((u32)type << 16) | ((u32)attr << 8) | ((u32)state))

static struct device *parm_dev;
volatile static u32 chan_recv_buff[CHAN_RECV_BUFF_SIZE];
volatile static u32 chan_recv_num;
static DECLARE_COMPLETION(ack_completion);

static void chan_recv_callback(struct mbox_client *cl, void *mssg)
{
	if (chan_recv_num >= CHAN_RECV_BUFF_SIZE) {
		sunxi_err(NULL, "chan recv buff is full\n");
		return ;
	}
	chan_recv_buff[chan_recv_num++] = *(u32 *)mssg;
	complete(&ack_completion);
}

static void pm_wait_ack_reset(void)
{
	chan_recv_num = 0;
}

int pm_wait_ack(u8 result, u8 type, u8 attr, u8 state, u16 len, u32 *param)
{
	int32_t ret;
	u32 t_head = HEAD_COMBINE(0, type, attr, state);

	reinit_completion(&ack_completion);

	ret = wait_for_completion_timeout(&ack_completion, msecs_to_jiffies(CHAN_RECV_ACK_TIMEOUT));
	if (!ret) {
		sunxi_err(NULL, "pm_wait_ack timeout\n");
		return -ETIMEDOUT;
	}

	if (t_head != chan_recv_buff[0]) {
		sunxi_err(NULL, "pm_wait_ack head error\n");
		return -EINVAL;
	}

	return 0;
}

int pm_send_msg(u8 result, u8 type, u8 attr, u8 state, u16 len, u32 *param)
{
	u32 i;
	u32 t_head, t_len;

	t_head = HEAD_COMBINE(result, type, attr, state);
	t_len = (u32)len;
	if (mbox_send_message(pm_msgbox_chan, &t_head) < 0)
		return -1;

	if (mbox_send_message(pm_msgbox_chan, &t_len) < 0)
		return -1;

	for (i = 0; i < t_len; i++) {
		if (mbox_send_message(pm_msgbox_chan, &param[i]) < 0)
			return -1;
	}
	return 0;
}

int pm_send_msg_wait_ack(u16 type, u8 len, u32 *param)
{
	pm_wait_ack_reset();
	if (pm_send_msg(0, type, ARISC_MESSAGE_ATTR_HARDSYN, ARISC_MESSAGE_INITIALIZED, len, param)) {
		sunxi_err(NULL, "pm send msg fail\n");
		return -1;
	}
	if (pm_wait_ack(0, type, ARISC_MESSAGE_ATTR_HARDSYN, ARISC_MESSAGE_INITIALIZED, len, param)) {
		sunxi_err(NULL, "pm wait ack fail\n");
		return -1;
	}
	return 0;
}

static int send_pm_set_dram_refresh(u32 en)
{
	u32 param[1];

	if (en)
		param[0] = 1;
	else
		param[0] = 0;
	return pm_send_msg_wait_ack(PM_SET_DRAM_REFRESH, 1, param);
}

static int send_pm_set_sram_suspend(u32 en)
{
	u32 param[1];

	if (en)
		param[0] = 1;
	else
		param[0] = 0;
	return pm_send_msg_wait_ack(PM_SET_SRAM_SUSPEND, 1, param);
}

struct class pm_msgbox_class = {
	.name = "pm_msgbox",
	.class_groups = pm_msgbox_class_groups,
};

static int pm_msgbox_probe(struct platform_device *pdev)
{
	int ret;
	struct pm_msgbox_dev *pm_msgbox;
	struct device *dev = &pdev->dev;
	pm_msgbox = devm_kzalloc(dev, sizeof(*pm_msgbox), GFP_KERNEL);
	if (IS_ERR_OR_NULL(pm_msgbox)) {
		sunxi_err(dev, "alloc failed\n");
		return -ENOMEM;
	}

	parm_dev = dev;
	pm_msgbox->dev = dev;
	pm_msgbox->client.rx_callback = chan_recv_callback;
	pm_msgbox->client.tx_done = NULL;
	pm_msgbox->client.tx_block = NULL;
	pm_msgbox->client.dev = dev;
	pm_msgbox_chan = mbox_request_channel_byname(&pm_msgbox->client,
						PM_MSGBOX_NAME);
	if (IS_ERR(pm_msgbox_chan)) {
		pm_msgbox_chan = NULL;
		sunxi_err(dev, "request mbox channel failed\n");
		return -EFAULT;
	}
	pm_msgbox->chan = pm_msgbox_chan;
	platform_set_drvdata(pdev, pm_msgbox);

	ret = class_register(&pm_msgbox_class);
	if (ret < 0) {
		sunxi_err(dev, "class register err, ret:%d\n", ret);
		goto mbox_free;
	}
	return ret;

mbox_free:
	mbox_free_channel(pm_msgbox_chan);
	return ret;
}

static int pm_msgbox_remove(struct platform_device *pdev)
{
	class_unregister(&pm_msgbox_class);
	mbox_free_channel(pm_msgbox_chan);

	return 0;
}

static struct of_device_id pm_msgbox_ids[] = {
	{ .compatible = "allwinner,sun8iw22-pm-msgbox" },
	{}
};

static struct platform_driver pm_msgbox_driver = {
	.probe = pm_msgbox_probe,
	.remove = pm_msgbox_remove,
	.driver = {
		.name = "pm_msgbox",
		.owner = THIS_MODULE,
		.of_match_table = pm_msgbox_ids,
	}
};

static int __init pm_msgbox_init(void)
{
	int err;

	err = platform_driver_register(&pm_msgbox_driver);
	if (err)
		sunxi_err(NULL, "register pm_msgbox failed\n");

	return err;
}
static void __exit pm_msgbox_exit(void)
{
	platform_driver_unregister(&pm_msgbox_driver);
}
late_initcall(pm_msgbox_init);
module_exit(pm_msgbox_exit);

MODULE_DESCRIPTION("PM MSGBOX DRIVER");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("ALLWINNER");
MODULE_VERSION("1.0.0");
