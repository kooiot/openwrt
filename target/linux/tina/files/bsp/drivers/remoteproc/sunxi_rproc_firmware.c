/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * sunxi's firmware loader driver
 * it implement search elf file from boot_package.
 *
 * Copyright (C) 2022 Allwinnertech - All Rights Reserved
 *
 * Author: lijiajian <lijiajian@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/firmware.h>
#include <linux/uaccess.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/string.h>
#include <linux/memblock.h>
#include <linux/elf.h>

#include "sunxi_rproc_firmware.h"
#include "sunxi_rproc_load_partition.h"

struct fw_mem_info {
	const char *name;
	void *addr;
	phys_addr_t pa;
	uint32_t len;
	struct list_head list;
};
static LIST_HEAD(g_memory_fw_list);
static DEFINE_MUTEX(g_list_lock);

#ifdef DEBUG
static int sunxi_firmware_dump_data(void *buf, int len)
{
	int i, j, line;
	u8 *p = buf;

	pr_info("mem firmware:\n");

	for (i = 0; i < len; i += 16) {
		j = 0;
		if (len > (i + 16))
			line = 16;
		else
			line = len - i;
		for (j = 0; j < line; j++)
			pr_cont("%02x ", p[i + j]);

		pr_cont("\t\t|");
		for (j = 0; j < line; j++) {
			if (p[i + j] > 32 && p[i + j] < 126)
				pr_cont("%c", p[i + j]);
			else
				pr_cont(".");
		}
		pr_cont("|\n");
	}
	pr_cont("\n");

	return 0;
}
#endif

static int sunxi_fw_unpack(u8 *dst, size_t max_size, const u8 *src, size_t len)
{
	const sunxi_image_header_t *ih = (const sunxi_image_header_t *) src;
	size_t new_size, offset;

	if (ih->ih_magic != SUNXI_IH_MAGIC)
		return 0;

	offset = ih->ih_hsize;
	new_size = ih->ih_psize;

	if (0 >= new_size)
		return -1;
	if (max_size < new_size)
		return -1;
	if (len < (offset + new_size))
		return -1;

	memmove(dst, src + offset, new_size);
	return new_size;
}

static int sunxi_check_elf_fw_len(const char *fw_name, const u8 *elf_data, uint32_t fw_size)
{
	int ret;
	struct elf32_hdr *ehdr;
	struct elf32_shdr *shdr;
	u32 i, shdr_table_end_offset, section_end_offset = 0, max_offset = 0;

	ehdr = (struct elf32_hdr *)elf_data;

	if (fw_size < sizeof(*ehdr)) {
		pr_err("Image is too small\n");
		return -EINVAL;
		goto error_exit;
	}

	if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG)) {
		pr_err("Image is corrupted (bad magic)\n");
		ret = -EINVAL;
		goto error_exit;
	}

	shdr_table_end_offset = ehdr->e_shoff + ehdr->e_shentsize * ehdr->e_shnum;
	if (fw_size < shdr_table_end_offset) {
		pr_err("error! the fw_size(%u) is little than the shdr_table_end_offset(%u)!\n",
		       fw_size, shdr_table_end_offset);
		ret = -EINVAL;
		goto error_exit;
	}

	shdr = (struct elf32_shdr *)(elf_data + ehdr->e_shoff);
	for (i = 0; i < ehdr->e_shnum; i++, shdr++) {
		if (shdr->sh_type == SHT_NOBITS)
			section_end_offset = shdr->sh_offset;
		else
			section_end_offset = shdr->sh_offset + shdr->sh_size;

		if (max_offset < section_end_offset) {
			max_offset = section_end_offset;
		}
	}

	if (fw_size < max_offset) {
		pr_err("error, the fw_size(%u) is little than the max_offset(%u)!\n",
		       fw_size, max_offset);
		ret = -EINVAL;
		goto error_exit;
	}

	pr_debug("shdr_table_end_offset: %d, max_offset: %d\n", shdr_table_end_offset, max_offset);
	return 0;

error_exit:
	pr_err("please confirm the length of reserved mem in dts is greater or equal than the size of ELF firmware file('%s')\n", fw_name);
	return ret;
}

static int __sunxi_register_memory_fw(const char *name, phys_addr_t addr, uint32_t len, bool is_elf_fw)
{
	int ret;
	struct fw_mem_info *info;

	pr_info("register mem fw('%s'), addr=%pap, len=%u\n", name, &addr, len);

	if (is_elf_fw) {
		ret = sunxi_check_elf_fw_len(name, phys_to_virt(addr), len);
		if (ret) {
			return -EINVAL;
		}
	}

	info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (!info) {
		pr_err("Can't kzalloc fw_mem_info.\n");
		return -ENOMEM;
	}

	info->name = name;
	info->pa = addr;
	info->len = len;
	info->addr = phys_to_virt(addr);

	mutex_lock(&g_list_lock);
	list_add(&info->list, &g_memory_fw_list);
	mutex_unlock(&g_list_lock);

	pr_debug("register mem fw('%s') success: va=0x%px\n", name, info->addr);
	return 0;
}

int sunxi_register_memory_bin_fw(const char *name, phys_addr_t addr, uint32_t len)
{
	__sunxi_register_memory_fw(name, addr, len, false);
	return 0;
}
EXPORT_SYMBOL(sunxi_register_memory_bin_fw);


int sunxi_register_memory_fw(const char *name, phys_addr_t addr, uint32_t len)
{
	__sunxi_register_memory_fw(name, addr, len, true);
	return 0;
}
EXPORT_SYMBOL(sunxi_register_memory_fw);

void sunxi_unregister_memory_fw(const char *name)
{
	int ret;
	struct fw_mem_info *pos, *tmp;

	mutex_lock(&g_list_lock);
	list_for_each_entry_safe(pos, tmp, &g_memory_fw_list, list) {
		if (!strcmp(pos->name, name)) {
			pr_info("unregister mem fw('%s'), addr=%pad,len=%u\n",
					name, &pos->pa, pos->len);

			list_del(&pos->list);
			ret = memblock_free(pos->pa, pos->len);
			if (ret)
				pr_err("memblock_free failed, addr: %pap, len: %d, ret: %d\n", &pos->pa, pos->len, ret);
			free_reserved_area(__va(pos->pa), __va(pos->pa + pos->len), -1, NULL);
			kfree(pos);
			mutex_unlock(&g_list_lock);
			return;
		}
	}
	mutex_unlock(&g_list_lock);
}
EXPORT_SYMBOL(sunxi_unregister_memory_fw);

static int sunxi_remove_memory_fw(struct fw_mem_info *info)
{
	int ret;

	mutex_lock(&g_list_lock);
	list_del(&info->list);
	mutex_unlock(&g_list_lock);

	pr_info("remove mem fw('%s'), addr=%pap, len=%u\n",
			info->name, &info->pa, info->len);

	ret = memblock_free(info->pa, info->len);
	if (ret)
		pr_err("memblock_free failed, addr: %pap, len: %d, ret: %d\n", &info->pa, info->len, ret);
	free_reserved_area(__va(info->pa), __va(info->pa + info->len), -1, NULL);
	kfree(info);

	return 0;
}

static struct fw_mem_info *sunxi_find_memory_fw(const char *name)
{
	struct fw_mem_info *pos, *tmp;

	mutex_lock(&g_list_lock);
	list_for_each_entry_safe(pos, tmp, &g_memory_fw_list, list) {
		if (!strcmp(pos->name, name)) {
			mutex_unlock(&g_list_lock);
			return pos;
		}
	}
	mutex_unlock(&g_list_lock);

	return NULL;
}

static int sunxi_request_fw_from_mem(const struct firmware **fw,
			   const char *name, struct device *dev)
{
	int ret;
	u8 *fw_buffer;
	struct firmware *fw_p = NULL;
	struct fw_mem_info *info;

	info = sunxi_find_memory_fw(name);
	if (!info) {
		dev_err(dev, "Can't find firmware('%s') in memory.\n", name);
		return -ENODEV;
	}

	fw_buffer = vmalloc(info->len);
	if (!fw_buffer) {
		dev_err(dev, "failed to alloc firmware buffer\n");
		ret = -ENOMEM;
		goto exit_with_unregister_mem_fw;
	}

	fw_p = kmalloc(sizeof(*fw_p), GFP_KERNEL);
	if (!fw_p) {
		dev_err(dev, "failed to alloc firmware struct\n");
		ret = -ENOMEM;
		goto exit_with_free_fw_buffer;
	}

	/* read data from memory */
	memcpy(fw_buffer, info->addr, info->len);

	fw_p->size = info->len;
	fw_p->priv = NULL;
	fw_p->data = fw_buffer;
	*fw = fw_p;
	ret = 0;

#ifdef DEBUG
	sunxi_firmware_dump_data(fw_buffer, 128);
#endif

	sunxi_remove_memory_fw(info);
	return 0;

exit_with_free_fw_buffer:
	vfree(fw_buffer);

exit_with_unregister_mem_fw:
	sunxi_remove_memory_fw(info);

	return ret;
}

int sunxi_request_firmware_from_memory(const struct firmware **fw, const char *name,
				       struct device *dev)
{
	int ret;

	ret = sunxi_request_fw_from_mem(fw, name, dev);
	if (ret) {
		dev_err(dev, "request firmware('%s') in memory failed, ret: %d\n", name, ret);
	}

	return ret;
}
EXPORT_SYMBOL(sunxi_request_firmware_from_memory);

int sunxi_request_firmware_from_partition(const struct firmware **fw, const char *name,
					  size_t fw_size, struct device *dev)
{
	int ret;
	u8 *fw_buffer;
	struct firmware *fw_p = NULL;

	fw_buffer = vmalloc(fw_size);
	if (!fw_buffer) {
		dev_err(dev, "failed to alloc firmware buffer\n");
		return -ENOMEM;
	}

	ret = load_from_partition(name, fw_buffer, fw_size);
	if (ret < 0) {
		dev_err(dev, "failed to load fw from partition %s\n", name);
		goto exit_with_free_fw_buffer;
	}
	fw_size = ret;

	ret = sunxi_fw_unpack(fw_buffer, fw_size, fw_buffer, fw_size);
	if (ret > 0) {
		fw_size = ret;
		dev_info(dev, "sunxi_fw_unpack, new size: %lu\n", (unsigned long)fw_size);
	} else if (ret < 0) {
		dev_err(dev, "sunxi_fw_unpack, failed!\n");
	}

	ret = sunxi_check_elf_fw_len(name, fw_buffer, fw_size);
	if (ret < 0) {
		dev_err(dev, "failed to check fw from partition %s\n", name);
		goto exit_with_free_fw_buffer;
	}

	fw_p = kmalloc(sizeof(*fw_p), GFP_KERNEL);
	if (!fw_p) {
		dev_err(dev, "failed to alloc firmware struct\n");
		ret = -ENOMEM;
		goto exit_with_free_fw_buffer;
	}

	fw_p->size = fw_size;
	fw_p->priv = NULL;
	fw_p->data = fw_buffer;
	*fw = fw_p;

#ifdef DEBUG
	sunxi_firmware_dump_data(fw_buffer, 128);
#endif

	return 0;

exit_with_free_fw_buffer:
	vfree(fw_buffer);
	return ret;
}
EXPORT_SYMBOL(sunxi_request_firmware_from_partition);

int sunxi_firmware_unpack(struct firmware *fw, struct device *dev)
{
	int ret;

	if (!fw || !fw->data || !fw->size)
		return -EINVAL;

	ret = sunxi_fw_unpack((u8 *)fw->data, fw->size, fw->data, fw->size);
	if (ret > 0) {
		fw->size = ret;
		dev_info(dev, "sunxi_fw_unpack, new size: %lu\n", (unsigned long)fw->size);
	} else if (ret < 0) {
		dev_err(dev, "sunxi_fw_unpack, failed!\n");
	}

	return ret;
}

EXPORT_SYMBOL(sunxi_firmware_unpack);
