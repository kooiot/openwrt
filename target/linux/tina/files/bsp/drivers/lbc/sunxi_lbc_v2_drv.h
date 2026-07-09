/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
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

#define DRIVER_NAME     "LBC"
#define LBC_MAJOR		111
#define LBC_MINOR		222

#include "sunxi_lbc_v2_reg.h"

//#define TEST_DATA_SIZE	(512) //(256)
#define DA_TEST_DATA_SIZE	(128)
#define IDA_TEST_DATA_SIZE	(128)
#define DMA_DESCS_SIZE  (1 * sizeof(struct __lbc_idma_des))
//#define DMA_DATA_MAX	(0x200)		// set desc0 iodl to 0x1ff
#define DMA_DATA_MAX	(128)	// 0xfff
//#define DMA_DATA_MAX	(2*1024)	// 0x7ff
//#define DMA_DATA_MAX	(4032)		// 0xfbf

#define TRANS_NONE		0
#define TRANS_COMPLETED	1
#define TRANS_ERROR		2



/* kernel & user cmd */
#define CMD_DMA_TX_START 0x0a
#define CMD_DMA_RX_START 0x0b
#define CMD_MMAP_DMA_TX_BUF 0x0c
#define CMD_MMAP_DMA_RX_BUF 0x0d

typedef union {
	struct {
		unsigned int desc_15t0_iodl:16;			//how many bytes data to w/r
		unsigned int desc_22t16_burst_len:7;    //burst_len
		unsigned int desc_23_burst_type:1;      //burst_type
		unsigned int reversed0:1;				//24 not use
		unsigned int desc_26t25_cs:2;			//DMA CS
		unsigned int desc_27_dma_fin:1;		//DMA finish flag
		unsigned int desc_28_dir:1;			//DMA direction, DMA write process or read process
		unsigned int reversed1:1;				//29 not use
		unsigned int desc_30_wback:1;         //dma write back flag
		unsigned int desc_31_vld:1;			//dma description is/not valid
	} reg;
	unsigned int all;
} sunxi_lbc_desc0_t;


/* IDMC structure */
typedef struct __lbc_idma_des {
	sunxi_lbc_desc0_t desc0;
	unsigned int dma_buffer_saddr;              //dram start addr
	unsigned int next_desc_addr;
	unsigned int target_start_addr;             //localbus start addr (fpag)
} __attribute__((packed)) lbc_idma_des;

struct lbc_cs_cfg{
	uint8_t transfer_mode;	// da  ida  dma

	uint8_t sub_time_mode;

	uint16_t cle_cycle_num;
	uint16_t ale_cycle_num;
	uint16_t dat_cycle_num;

	// LBC_MODE_CTRL_REG
	uint8_t addr_mux_type;	// 00: non mux, 01: ad mux, 10: aad mux
	uint8_t lbc_protocol;	// 00: lbc mode0, 01: lbc mode1, 10: gpmc
	uint8_t bus_endian;		// 0: big endian, 1: little endian
	uint8_t transfer_width; // 00: 8bit, 01: 16bit, 10: 32bite
	uint8_t bus_addr_offset;	// 0: disable; 1: enable
	uint8_t rd_sync_type;		// 0: async; 1 sync
	uint8_t wr_sync_type;		// 0: async; 1 sync

	// LBC_TIMESCALE_CTRL_REG
	uint8_t clk_divider;	// 00: fclk/1, 01: fclk/2, 10: fclk/4, 11: fclk/8
	uint8_t clk_delay_time;

	// LBC_READY_CTRL_REG
	uint16_t ready_timeout_time;
	uint8_t rd_ready_en;
	uint8_t wr_ready_en;
	uint8_t ready_delay_time;
	uint8_t rd_ready_mode;
	uint8_t wr_ready_mode;
	uint8_t ready_polarity;

	// LBC_DP_CTRL_REG
	uint8_t dp_mode;	// 0: DP per byte, 1: one DP only
	uint8_t dp_parity;	// 0: odd parity, 1: even parity
	uint8_t dp_en;		// 0: disable, 1: enable

	// LBC_BE_CTRL_REG
	uint8_t be_parity_en;
	uint8_t de_polarity;
	uint8_t be_polarity;

	// LBC_WAIT_CTRL_REG
	uint16_t wait_timeout_time;
	uint8_t wait_polarity;
	uint8_t wait_en;

	// LBC_CS_TIMING_CTRL_REG
	uint8_t cs_rdoff_time;
	uint8_t cs_wroff_time;
	uint8_t cs_on_time;
	uint8_t cs_polarity;

	// LBC_ALE_TIMING0_CTRL_REG
	uint8_t ale_rdoff_time;
	uint8_t ale_wroff_time;
	uint8_t ale_on_time;
	uint8_t ale_polarity;

	// LBC_ALE_TIMING1_CTRL_REG
	uint8_t ale_aad_rdoff_time;
	uint8_t ale_aad_wroff_time;
	uint8_t ale_aad_on_time;

	// LBC_WE_TIMING_CTRL_REG
	uint8_t we_off_time;
	uint8_t we_on_time;
	uint8_t we_polarity;

	// LBC_OE_TIMING0_CTRL_REG
	uint8_t oe_off_time;
	uint8_t oe_on_time;
	uint8_t oe_polarity;

	// LBC_OE_TIMING1_CTRL_REG
	uint8_t oe_aad_off_time;
	uint8_t oe_aad_on_time;

	// LBC_OE_TIMING2_CTRL_REG
	uint8_t oe_page_time;
	uint8_t oe_page_en;

	// LBC_CYCLE_TIMING_CTRL_REG
	uint16_t rd_cycle_time;
	uint16_t wr_cycle_time;

	// LBC_ACCESS_TIMING_CTRL_REG
	uint16_t rd_access_time;
	uint16_t wr_access_time;

	// LBC_PAGE_TIMING_CTRL_REG
	uint8_t page_access_time;

	// LBC_DATAMUX_TIMING_CTRL_REG
	uint16_t wr_data_on_time;

	// LBC_CLE_TIMING_CTRL_REG
	uint8_t	cle_off_time;
	uint8_t cle_on_time;
	uint8_t cle_mode;
	uint8_t cle_polarity;

};

struct lbc_trans_cfg {
	// 0x200 CH_MODE_CTRL_REG
	uint8_t user_sel_en;
	uint8_t fix_sel_en;
	uint8_t addr_sel_en;

	// 0x210 ~ 0x24C
	uint32_t cs_da_haddr[CS_NUMS];
	uint32_t cs_da_laddr[CS_NUMS];
	uint32_t cs_ida_haddr[CS_NUMS];
	uint32_t cs_ida_laddr[CS_NUMS];

	// 0x250 FIX_CS_REG
	uint8_t dma_fix_cs;
	uint8_t ida_fix_cs;
	uint8_t da_fix_cs;

	// 0x254 USER_CS_REG
	uint8_t cs_user_sel[CS_NUMS];

	// 0x280 LBC_CLK_REG
	uint8_t lbc_clk_mode;
	uint8_t lbc_rx_negative_sel;
	uint8_t lbc_tx_negative_sel;
	uint8_t lbc_fclk_sel;
	uint8_t lbc_fclk_gate;

	// 0x290 SMP_DELAY_CTRL_REG
	uint32_t samp_dl_sw_value;
	uint16_t samp_dl_sw_en;

	// 0x440 LBC_ARB_PRI_CFG_REG
	uint8_t dma_priority;
	uint8_t ida_priority;
	uint8_t da_priority;
};

struct lbc_ida_cfg {
	// 0x300 IDA_TS_CTRL_REG;
	uint8_t ida_burst_arb_grain;
	uint8_t ida_burst_type_type;
	uint8_t ida_burst_type_length;
	uint8_t ida_dma_on;

	// 0x304 IDA_TS_ADDR_REG;
	//uint32_t ida_ts_addr;

	// 0x308 IDA_TS_DATA_LEN_REG;
	//uint16_t ida_ts_data_len;

	// 0x528 IDA_FIFO_TRIGGER_LVL_REG;
	uint8_t ida_rx_trigger_lvl;
	uint8_t ida_tx_trigger_lvl;
};

struct lbc_da_cfg {
	// 0x400 DA_BST_SEL_REG;
	uint8_t da_cs_burst_sel[CS_NUMS];

	// 0x404 DA_BST_TYPE_REG;
	uint8_t da_burst_type_type[CS_NUMS];
	uint8_t da_burst_type_length[CS_NUMS];

	// 0x41C DA_CS_SADDR_REG
	uint32_t da_cs_saddr_reg[CS_NUMS];

	// 0x524 DA_FIFO_TRIGGER_LVL_REG;
	uint8_t da_rx_trigger_lvl;
	uint8_t da_tx_trigger_lvl;
};

struct lbc_dma_cfg {
	// 0x460 DMA_MODE_CFG_REG
	uint8_t dma_burst_arb_grain;
	uint8_t dma_drq_en;

	uint8_t dma_burst_type;
	uint8_t dma_burst_length;

	// 0x52C DMA_FIFO_TRIGGER_LVL_REG;
	uint8_t dma_rx_trigger_lvl;
	uint8_t dma_tx_trigger_lvl;
};

struct lbc_intr_cfg {
	// 0x500 LBC_INT0_EN_REG
	uint8_t lbc_ida_rx_trig_int_en;
	uint8_t lbc_ida_tx_trig_int_en;
	uint8_t lbc_da_rx_trig_int_en;
	uint8_t lbc_da_tx_trig_int_en;
	uint8_t lbc_dp_int_en;
	uint8_t lbc_odp_int_en;
	uint8_t lbc_dma_des_invld_en;
	uint8_t lbc_dma_trsf_done_int_en;
	uint8_t lbc_ida_trsf_done_int_en;
	uint8_t lbc_da_trsf_done_int_en;
	uint8_t lbc_ready_timeout_int_en;
	uint8_t lbc_cmd_done_int_en;

	// 0x504 LBC_INT1_EN_REG
	uint8_t lbc_dma_timeout_int_en;
	uint8_t lbc_ida_timeout_int_en;
	uint8_t lbc_da_timeout_int_en;
	uint8_t lbc_dma_des_done_int_en;
	uint8_t lbc_data_req_timeout_int_en;
	uint8_t lbc_dma_rx_trig_int_en;
	uint8_t lbc_dma_tx_trig_int_en;
};

typedef struct sunxi_lbc {
	struct cdev cdev;
	struct device *dev_lbc;
	dev_t devid;
	struct class *lbc_class;
	int major;

	struct platform_device *pdev;
	struct device *dev;

	struct clk *lbc_pll;
	struct clk *bus_lbc;
	struct clk *lbc;
	struct reset_control *lbc_rst;
	int lbc_freq;

	struct regulator *io_supply;
	unsigned int io_vol;

	void __iomem *reg_addr;
	void __iomem *data_reg_addr[CS_NUMS];
	void __iomem *data_sram_addr[CS_NUMS];
	void __iomem *data_test_addr[CS_NUMS];
	phys_addr_t data_reg_phy_addr[CS_NUMS];
	phys_addr_t data_reg_addr_size[CS_NUMS];
	phys_addr_t data_sram_phy_addr[CS_NUMS];
	phys_addr_t data_sram_addr_size[CS_NUMS];
	phys_addr_t data_test_phy_addr[CS_NUMS];
	phys_addr_t data_test_addr_size[CS_NUMS];

	void __iomem *data_len;

	unsigned int ahb_rate;
	uint32_t period;
	uint32_t cycle;
	spinlock_t bwlock;
	int irq;

	// >>>>>> T153 params  add here
	// T153 added params here
	struct lbc_cs_cfg 		cs_cfg[CS_NUMS];
	struct lbc_trans_cfg	trans_cfg;
	struct lbc_ida_cfg 		ida_cfg;
	struct lbc_dma_cfg 		dma_cfg;
	struct lbc_da_cfg 		da_cfg;
	struct lbc_intr_cfg	 	intr_cfg;

	uint32_t burst_mode;
	// <<<<<

	int fpga_reset_io;
	int fpga_direct_io;

	struct __lbc_idma_des *dma_tx;
	dma_addr_t dma_tx_phy;

	struct __lbc_idma_des *dma_rx;
	dma_addr_t dma_rx_phy;

	u8 *dma_tx_buffer;
	dma_addr_t dma_tx_buffer_phy;

	u8 *dma_rx_buffer;
	dma_addr_t dma_rx_buffer_phy;

	unsigned int dma_target_addr;
	uint32_t dma_f_pos;
	int dma_mmap_dir;

	u8 da_receive_buf[DA_TEST_DATA_SIZE];
	u8 da_send_buf[DA_TEST_DATA_SIZE];
	u8 ida_receive_buf[IDA_TEST_DATA_SIZE];
	u8 ida_send_buf[IDA_TEST_DATA_SIZE];
	int rx_index;
	volatile int ida_result;
	volatile int dma_result;

	wait_queue_head_t ida_wait;
	wait_queue_head_t dma_wait;

	struct task_struct *da_test_task;
	struct task_struct *ida_test_task;
	struct task_struct *dma_test_task;
	uint32_t current_cs_idx;
} sunxi_lbc_t;

#define LOG_CLOSE		0
#define LOG_ERROR		1
#define LOG_WARNING		2
#define LOG_INFO		3
#define LOG_DEBUG		4


#define lbc_log(level, fmt, ...) \
{	\
	if ((level) <= (lbc_log_level)) {	\
		printk(pr_fmt(fmt), ##__VA_ARGS__);	\
	}	\
}
