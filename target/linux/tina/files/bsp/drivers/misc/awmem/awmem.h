/*
 * linux-4.9/drivers/misc/awmem/awmem.h
 *
 * Copyright (c) 2007-2021 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _AWMEM_H_
#define _AWMEM_H_

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/ctype.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/dma-mapping.h>
#include <linux/of_reserved_mem.h>
#include <linux/genalloc.h>
#include <linux/idr.h>
#include <linux/mman.h>
#include <linux/memblock.h>
#include <linux/list.h>
#include <asm/io.h>
#include <asm/cacheflush.h>
#include <linux/iommu.h>

#define BASE_AWMEM_PRIVATE 0
#define CMD_AWMEM_ALLOC \
	_IOWR('V', BASE_AWMEM_PRIVATE + 1, struct awmem_buf_request)
#define CMD_AWMEM_FREE \
	_IOWR('V', BASE_AWMEM_PRIVATE + 2, struct awmem_buf_free_arg)
#define CMD_AWMEM_GET_PHYS_ADDR \
	_IOWR('V', BASE_AWMEM_PRIVATE + 3, struct awmem_get_phys_addr_arg)
#define CMD_AWMEM_FLUSH_RANGE \
	_IOWR('V', BASE_AWMEM_PRIVATE + 4, struct awmem_flush_range_arg)

#define AWMEM_ALIGN_SIZE 0x1000

#define AWMEM_ALIGN(n, align) \
    ( \
		((n) + ((align) - 1)) & ~((align) - 1) \
    )

enum awmem_sta {
	AWMEM_REQUEST_SUCCESS = 0,
	AWMEM_REQUEST_FAILURE,
	AWMEM_ALLOC_SUCCESS,
	AWMEM_ALLOC_FAILURE,
};

struct awmem_buffer {
	int id;
	unsigned long phys_addr;
	void *phys_ptr;
	unsigned int size;
	void *viraddr;
	unsigned int count;
	struct list_head list;
};

struct awmem_buf_request {
	int id;
	unsigned int size;
	void *addr;
	int status;
};

struct awmem_buf_free_arg {
	int id;
	void *viraddr;
};

struct awmem_get_phys_addr_arg {
	int id;
	unsigned long phys_addr;
	unsigned long virt_start;
	unsigned long virt;
};

struct awmem_flush_range_arg {
	unsigned long start_addr;
	unsigned int size;
};

struct awmem_priv {
	struct gen_pool *pool;
	unsigned long pool_start;
	size_t pool_length;
	struct idr idr;
	unsigned long awmem_viraddr;
	struct vm_area_struct *awmem_vma;
	void *awmem_vaddr;
	struct list_head list;
	struct iommu_domain *domain;
};

#endif
