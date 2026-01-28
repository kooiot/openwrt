/* SPDX-License-Identifier: GPL-2.0-or-later*/
/* Copyright(c) 2020 - 2025 Allwinner Technology Co.,Ltd. All rights reserved. */
 /*
  * Allwinner LBC support
  *
  * Copyright (C) 2015 AllWinnertech Ltd.
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
  */

#ifndef SUNXI_LBC_COMMON_H
#define SUNXI_LBC_COMMON_H

enum LBC_IOCTL_CMD {
	CMD_NONE = 0,
	CMD_SET_MMAP_REGION,	// set mmap region index
	CMD_GET_MMAP_REGION,	// get mmap region info
	CMD_SET_CONVST_IO,		// pull adc convst io
	CMD_RESET_SLAVE,		// do slave reset
	CMD_EXPORT_DMA_TX_BUF,	// export dma tx buf;
	CMD_EXPORT_DMA_RX_BUF,	// export dma rx buf;
	CMD_DMA_TX_START,
	CMD_DMA_RX_START,
};

enum MMAP_REGION {
	MMAP_REGION_NONE = 0,
	MMAP_REGION_REG,
	MMAP_REGION_CS0,
	MMAP_REGION_CS1,
	MMAP_REGION_CS2,
	MMAP_REGION_CS3,
	MMAP_REGION_MAX,
};

struct dma_trans_params{
	uint32_t target_addr;
	uint32_t burst_type;
	uint32_t burst_length;
	uint32_t data_length;
    uint32_t cs;
	uint32_t reserved[4];
} __attribute__((packed));

struct cs_command_header {
    uint8_t cs;
    uint8_t flags;
    uint32_t reserved;
    uint32_t offset;
} __attribute__((packed));

#endif // SUNXI_LBC_COMMON_H
