/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
* Allwinner LBC driver.
*
* Copyright(c) 2022-2027 Allwinnertech Co., Ltd.
*
* This file is licensed under the terms of the GNU General Public
* License version 2.  This program is licensed "as is" without any
* warranty of any kind, whether express or implied.
*/

#include <sunxi-log.h>

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/ctype.h>
#include <linux/bitfield.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/regulator/consumer.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/spi/spi.h>
#include <linux/spi/sunxi-spi.h>
#include <dt-bindings/spi/sunxi-spi.h>

struct aw_spi_lbc_data {
	struct spi_device       *spi;
	struct device *dev;
	struct mutex            lock;
};

static int aw_spi_lbc_read_data(struct aw_spi_lbc_data *spi_lbc_data, u8 reg, u32 *reg_result)
{
	unsigned char txbuf[8] = {0x00};
	unsigned char read_buf[8] = {0x00};
	u8 read_bit;
	unsigned long long reg_result_tmp_total = 0;
	unsigned long long reg_result_tmp_5 = 0;
	unsigned long long reg_result_tmp_4 = 0;
	unsigned long long reg_result_tmp_3 = 0;
	unsigned long long reg_result_tmp_2 = 0;
	unsigned long long reg_result_tmp_1 = 0;
	unsigned long long reg_result_tmp_0 = 0;

	u32 reg_result_trans = 0;
	int ret, i;
	struct spi_message	msg;
	struct spi_transfer	t;

	read_bit = 0;

	sunxi_info(NULL, "aw_spi_lbc_read_data target reg is 0x%02x\n", reg);

	txbuf[0] = ((reg & 0x7f) << 1) | (read_bit);
	txbuf[1] |= (reg & (0x01));

	for (i = 0; i < 8; i++) {
		sunxi_info(NULL, "aw_spi_lbc_read_data txbuf %d is 0x%02x\n", i, txbuf[i]);
	}

	if (spi_lbc_data == NULL) {
		sunxi_info(NULL, " aw_spi_lbc_read_data spi_lbc_data is nullptr\n");
		return 0;
	}

	mutex_lock(&spi_lbc_data->lock);
	spi_message_init(&msg);
	memset(&t, 0, sizeof(t));
	t.tx_buf = txbuf;
	t.len = 8;
	t.rx_buf = read_buf;
	spi_message_add_tail(&t, &msg);
	ret = spi_sync(spi_lbc_data->spi, &msg);
	if (ret) {
		sunxi_info(NULL, " aw_spi_lbc_read_data spi_lbc read reg data failed\n");
		return -1;
	}

	for (i = 0; i < 8; i++) {
		sunxi_info(NULL, "aw_spi_lbc_read_data read_buf %d is 0x%02x\n", i, read_buf[i]);
	}

	reg_result_tmp_5 |= (((unsigned long long)read_buf[5]) << 40UL);
	reg_result_tmp_4 |= (((unsigned long long)read_buf[4]) << 32UL);
	reg_result_tmp_3 |= (((unsigned long long)read_buf[3]) << 24UL);
	reg_result_tmp_2 |= (((unsigned long long)read_buf[2]) << 16UL);
	reg_result_tmp_1 |= (((unsigned long long)read_buf[1]) << 8UL);
	reg_result_tmp_0 |= (unsigned long long)read_buf[0];

	reg_result_tmp_total |= (reg_result_tmp_5 | reg_result_tmp_4 | reg_result_tmp_3 | reg_result_tmp_2 | reg_result_tmp_1 | reg_result_tmp_0);
	reg_result_trans |= (reg_result_tmp_total >> 9);
	*reg_result = reg_result_trans;
	mutex_unlock(&spi_lbc_data->lock);
	sunxi_info(NULL, "aw_spi_lbc_read_data reg 0x%02x val is 0x%08x\n", reg, *reg_result);
	return ret;
}

static int aw_spi_lbc_write_data(struct aw_spi_lbc_data *spi_lbc_data, u8 reg, u32 reg_val)
{
	unsigned char txbuf[8] = {0x00};
	unsigned char rxbuf[8] = {0x00};
	int ret, i;
	struct spi_message	msg;
	struct spi_transfer	t;
	u8 write_bit = 0x01;

	sunxi_info(NULL, "aw_spi_lbc_write_data target reg is 0x%02x\n", reg);
	sunxi_info(NULL, "aw_spi_lbc_write_data target reg_val is 0x%08x\n", reg_val);

	txbuf[0] = ((reg & 0x7f) << 1) | (write_bit);
	txbuf[1] = ((reg_val & 0x7f) << 1) | (reg & (0x01));
	txbuf[2] = (reg_val >> 7) & 0xFF;
	txbuf[3] = (reg_val >> 15) & 0xFF;
	txbuf[4] = (reg_val >> 23) & 0xFF;
	txbuf[5] |= ((reg_val >> 31) & 0x01);

	for (i = 0; i < 8; i++) {
		sunxi_info(NULL, "aw_spi_lbc_write_data txbuf %d is 0x%02x\n", i, txbuf[i]);
	}

	mutex_lock(&spi_lbc_data->lock);
	spi_message_init(&msg);
	memset(&t, 0, sizeof(t));
	t.tx_buf = txbuf;
	t.len = 8;
	t.rx_buf = rxbuf;
	spi_message_add_tail(&t, &msg);
	ret = spi_sync(spi_lbc_data->spi, &msg);
	if (ret) {
		sunxi_info(NULL, " aw_spi_lbc_write_data spi_lbc write reg data failed\n");
		return -1;
	}

	mutex_unlock(&spi_lbc_data->lock);
	return ret;
}

static ssize_t sunxi_spi_lbc_write_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t count)
{
	struct aw_spi_lbc_data *spi_lbc_data = dev_get_drvdata(dev);
	u8 reg = 0;
	int ret;
	unsigned int reg_val = 0;
	unsigned int temp1, temp2;

	ret = sscanf(buf, "0x%x,0x%x", &temp1, &temp2);
	if (ret != 2) {
		printk(KERN_ERR "Invalid format, expected: 0x00,0x12212235\n");
		return -EINVAL;
	}

	reg = temp1;
	reg_val = temp2;

	aw_spi_lbc_write_data(spi_lbc_data, reg, reg_val);
	return count;
}

static ssize_t sunxi_spi_lbc_read_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t count)
{
	struct aw_spi_lbc_data *spi_lbc_data = dev_get_drvdata(dev);
	u8 reg;
	u32 reg_result;
	int err;

	err = kstrtou8(buf, 16, &reg);
	if (err) {
		sunxi_err(NULL, "String conversion failed!\n");
		return -ERANGE;
	}
	aw_spi_lbc_read_data(spi_lbc_data, reg, &reg_result);
	return count;
}

static struct device_attribute sunxi_spi_lbc_debug_attr[] = {
	__ATTR(writeReg, S_IWUSR, NULL, sunxi_spi_lbc_write_store),
	__ATTR(readReg, S_IWUSR, NULL, sunxi_spi_lbc_read_store),
};

static void sunxi_spi_lbc_create_sysfs(struct aw_spi_lbc_data *spi_lbc_data)
{
	int i;
	struct spi_device *spi = spi_lbc_data->spi;

	for (i = 0; i < ARRAY_SIZE(sunxi_spi_lbc_debug_attr); i++)
		device_create_file(&spi->dev, &sunxi_spi_lbc_debug_attr[i]);
}

static void sunxi_spi_lbc_remove_sysfs(struct spi_device *spi)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(sunxi_spi_lbc_debug_attr); i++)
		device_remove_file(&spi->dev, &sunxi_spi_lbc_debug_attr[i]);
}

static int aw_spi_lbc_probe(struct spi_device *spi)
{
	struct aw_spi_lbc_data *spi_lbc_data;

	spi_lbc_data = devm_kzalloc(&spi->dev, sizeof(*spi_lbc_data), GFP_KERNEL);
	if (!spi_lbc_data)
		return -ENOMEM;

	spi_lbc_data->spi = spi;
	spi_lbc_data->dev = &spi->dev;
	mutex_init(&spi_lbc_data->lock);
	sunxi_spi_lbc_create_sysfs(spi_lbc_data);
	dev_set_drvdata(spi_lbc_data->dev, spi_lbc_data);
	sunxi_info(NULL, "aw_spi_lbc driver probe success!!!\n");

	return 0;
}

static int aw_spi_lbc_remove(struct spi_device *spi)
{
	sunxi_spi_lbc_remove_sysfs(spi);
	return 0;
}

static const struct of_device_id aw_spi_lbc_of_ids[] = {
	{ .compatible = "allwinner, aw-spi-lbc" },
	{ /* sentinel */ },
};

static struct spi_driver aw_spi_lbc_drv = {
	.driver = {
		.name = "allwinner, aw-spi-lbc",
		.of_match_table = of_match_ptr(aw_spi_lbc_of_ids),
	},
	.probe = aw_spi_lbc_probe,
	.remove = aw_spi_lbc_remove,
};
module_spi_driver(aw_spi_lbc_drv);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("liujing <luiujingswc@allwinnertech.com>");
MODULE_DESCRIPTION("Allwinner's spi controll localbus register");
MODULE_VERSION("1.0.0");

