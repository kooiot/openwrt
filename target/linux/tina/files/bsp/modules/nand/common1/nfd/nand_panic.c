// SPDX-License-Identifier: GPL-2.0
/*
 *
 *
 * Copyright (C) 2019 liaoweixiong <liaoweixiong@gallwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include "nand_panic.h"
#include "nand_lib.h"
#include "nand_osal_for_linux.h"
#include "nand_base.h"
#include <linux/kernel.h>
#include <linux/pstore_blk.h>
#include <linux/types.h>
#include <linux/printk.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/version.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
#include <linux/sunxi_blkpstore.h>
#endif

#define AW_NFTL_MAJOR 93
#define PANIC_INFO(...) printk(KERN_INFO "[NI] " __VA_ARGS__)
#define PANIC_DBG(...) printk(KERN_DEBUG "[ND] " __VA_ARGS__)
#define PANIC_ERR(...) printk(KERN_ERR "[NE] " __VA_ARGS__)

static dma_addr_t panic_dma;
struct dma_buf {
	void *buf;
	size_t size;
};
static struct dma_buf panic_buf;
static struct dma_buf panic_map;
static struct _nftl_blk *g_nftl_blk;
static sector_t g_start_sect;
static sector_t g_sects;

int nand_panic_read(char *buf, sector_t sec_off,  sector_t sects)
{
	struct _nftl_blk *nftl_blk = g_nftl_blk;

	if (!is_panic_enable()) {
		PANIC_ERR("not support panic nand\n");
		return -EBUSY;
	}

	if (!nftl_blk) {
		PANIC_ERR("invalid nftl_blk\n");
		return -EINVAL;
	}

	if (sec_off + sects > g_sects) {
		PANIC_ERR("sectors %llu with start sector %llu is out of range, \n",
			(long long unsigned int)sects, (long long unsigned int)sec_off);
		return -EACCES;
	}

	return panic_read(nftl_blk, sec_off + g_start_sect, sects, (uchar *)buf);
}

int nand_panic_write(const char *buf, sector_t sec_off, sector_t sects)
{
	struct _nftl_blk *nftl_blk = g_nftl_blk;

	if (!is_panic_enable()) {
		PANIC_ERR("not support panic nand\n");
		return -EBUSY;
	}

	if (!nftl_blk) {
		PANIC_ERR("invalid nftl_blk\n");
		return -EINVAL;
	}

	if (sec_off + sects > g_sects) {
		PANIC_ERR("sectors %llu with start sector %llu is out of range, \n",
			(long long unsigned int)sects, (long long unsigned int)sec_off);
		return -EACCES;
	}

	return panic_write(nftl_blk, sec_off + g_start_sect, sects, (uchar *)buf);
}

struct device *nand_get_dev(void);
static int rawnand_panic_alloc_dma_buf(size_t size)
{
	void *buf;
	struct device *dev = nand_get_dev();

	buf = dma_alloc_coherent(dev, size, &panic_dma, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;
	panic_buf.buf = buf;
	panic_buf.size = size;
	return 0;
}

static void rawnand_panic_free_dma_buf(void)
{
	struct device *dev = nand_get_dev();

	dma_free_coherent(dev, panic_buf.size, (void *)panic_buf.buf, panic_dma);
	panic_buf.buf = NULL;
	panic_buf.size = 0;
}

dma_addr_t nand_panic_dma_map(__u32 rw, void *buf, __u32 len)
{
	if (!is_panic_enable())
		return -EBUSY;

	/* ONLY rawnand allow to use DMA */
	if (get_storage_type() != NAND_STORAGE_TYPE_RAWNAND)
		return -EINVAL;

	if (rw == 1) {
		size_t size = min_t(size_t, len, panic_buf.size);
		memcpy(panic_buf.buf, buf, size);
	} else {
		panic_map.buf = buf;
		panic_map.size = (size_t)len;
	}

	return panic_dma;
}

void nand_panic_dma_unmap(__u32 rw, dma_addr_t dma_handle, __u32 len)
{
	if (!is_panic_enable())
		return;

	/* ONLY rawnand allow to use DMA */
	if (get_storage_type() != NAND_STORAGE_TYPE_RAWNAND)
		return;

	if (rw == 0) {
		size_t size = min_t(size_t, panic_map.size, panic_buf.size);
		memcpy(panic_map.buf, panic_buf.buf, size);
	}

	return;
}

int nand_support_panic(void)
{
	panic_mark_enable();
	return 0;
}

#if 1
static struct bdev_info part;

#ifndef ALIGN_UP
#define ALIGN_UP(size, align)	(((size) + (align) - 1) & ~((align) - 1))
#endif

#ifndef ALIGN_DOWN
#define ALIGN_DOWN(size, align)	((size) & ~((align) - 1))
#endif

static ssize_t nand_panic_write_ng(const char *buf, size_t size, loff_t off)
{
	if ((off % 512) != 0) {
		pr_err("%s off(%lu) not align to 512\n", __func__, (unsigned long)off);
		return -1;
	}

	return nand_panic_write(buf, off / 512, ALIGN_UP(size, 512) / 512);
}

static int sunxi_nand_panic_resource_init(void)
{
	return 0;
}

static int sunxi_nand_register_pstore_panic(enum pstore_blk_notifier_type type,
		struct pstore_device_info *pdi)
{
	switch (type) {
	case PSTORE_BLK_BACKEND_REGISTER:
	case PSTORE_BLK_BACKEND_PANIC_DRV_REGISTER:
		if (sunxi_nand_panic_resource_init() != 0)
			goto err;

		pdi->zone.panic_write = nand_panic_write_ng;

		if (aw_pstore_blk_get_bdev_info(&part)) {
			pr_err("get bdev info fail\n");
			goto err;
		}

		g_sects = part.nr_sects;
		g_start_sect = part.start_sect;

		pr_info("register pstore backend %s panic write\n", pdi->zone.name);
		break;

	case PSTORE_BLK_BACKEND_UNREGISTER:
	case PSTORE_BLK_BACKEND_PANIC_DRV_UNREGISTER:
		pdi->zone.panic_write = NULL;

		break;
	default:
		goto err;

	}

	return NOTIFY_OK;

err:
	return NOTIFY_BAD;
}

static struct pstore_blk_notifier sunxi_nand_panic = {
	.notifier_call = sunxi_nand_register_pstore_panic,
};
#endif
int nand_panic_init(struct _nftl_blk *nftl_blk)
{
	int ret;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
#else
	struct pstore_blk_info info = {};
#endif

	if (!is_panic_enable())
		return -EBUSY;

	PANIC_INFO("panic nand version: %s\n", PANIC_NAND_VERSION);

	/* allocate memory for rawnand dma transfer */
	if (get_storage_type() == NAND_STORAGE_TYPE_RAWNAND) {
		ret = rawnand_panic_alloc_dma_buf(32 * 1024);
		if (ret)
			goto err_out;
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	ret = aw_register_pstore_blk_panic_notifier(&sunxi_nand_panic);
	if (ret)
		goto err_free;
	g_nftl_blk = (void *)nftl_blk;
#else
	info.major = AW_NFTL_MAJOR;
	info.flags = 0;
	info.panic_write = nand_panic_write;
	ret = register_pstore_blk(&info);
	if (ret)
		goto err_free;
	g_nftl_blk = (void *)nftl_blk;
	g_sects = info.nr_sects;
	g_start_sect = info.start_sect;
#endif

	PANIC_INFO("panic nand init ok\n");
	return 0;
err_free:
	rawnand_panic_free_dma_buf();
err_out:
	PANIC_INFO("panic nand init failed\n");
	return ret;
}
