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

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of_address.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/dma-mapping.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/workqueue.h>
#include <linux/fs.h>
#include <linux/random.h>
#include "sunxi_lbc_v2_drv.h"

#define LBC_DEBUG

#define LBC_VERSION "1.0.1-lbc-v2"

#define LBC_RELEASE 0

#define DEFAULT_LBC_LOG_LEVEL LOG_INFO
#define DEFAULT_CLK_DIVIDER		FCLK_DIVIDE_2
#define DEFAULT_CLE_CYCLE_NUM	1
#define DEFAULT_ALE_CYCLE_NUM	1
#define DEFAULT_DAT_CYCLE_NUM	1

#define TRANSFER_MODE_DIRECT_ACCESS	(0)
#define TRANSFER_MODE_INTERRUPT		(1)
#define TRANSFER_MODE_DMA			(2)

#define TRANSFER_WIDTH_8_BIT		(0)
#define TRANSFER_WIDTH_16_BIT		(1)
#define TRANSFER_WIDTH_32_BIT		(2)

#define DEFAULT_CHAIN_DELAY			(30)

volatile uint32_t lbc_log_level;
volatile uint32_t dma_wait_time;		/* 100 */

static DEFINE_MUTEX(lbc_lock);

static uint32_t CSn_LADDR[CS_NUMS] = {LBC_CS0_LADDR, LBC_CS1_LADDR, LBC_CS2_LADDR, LBC_CS3_LADDR};

// ----------------------------------------------------------------------------
// reg operate
// ----------------------------------------------------------------------------
/**
 *  extracts the field value from a 32-bit variable,
 *  without reading from actual register address
 */
#define LBC_GET_FIELD32(reg_val, REG_FIELD)                                       \
	(((reg_val) & (uint32_t) REG_FIELD##_MASK) >> (uint32_t) REG_FIELD##_SHIFT)

/**
 *  1. clears the specified field value
 *  2. performs "OR" of the field value(shifted and masked)
 */
#define LBC_SET_FIELD32(reg_val, REG_FIELD, field_val)                          \
	((reg_val) = ((reg_val) & (uint32_t) (~(uint32_t) REG_FIELD##_MASK)) |      \
		((((uint32_t) (field_val)) << (uint32_t) REG_FIELD##_SHIFT) & (uint32_t) REG_FIELD##_MASK))

/**
 *  1. read reg_val from reg_addr
 *  2. clears the specified field value of reg_val
 *  3. performs "OR" of the field value(shifted and masked)
 *  4. write to reg_addr
 */
#define LBC_WR_FIELD32(reg_addr, REG_FIELD, field_val)                          \
	(LBC_WR_FIELD32_RAW((reg_addr), ((uint32_t)REG_FIELD##_MASK),		\
		((uint32_t)REG_FIELD##_SHIFT), (uint32_t)(field_val)))

/**
 *  1. read reg_val from reg_addr
 *  2. return shifted and masked filed value of reg_val
 */
#define LBC_RD_FIELD32(reg_addr, REG_FIELD)                                      \
	(LBC_RD_FIELD32_RAW((reg_addr), ((uint32_t) REG_FIELD##_MASK),    \
		((uint32_t) REG_FIELD##_SHIFT)))


static inline void LBC_WR_FIELD32_RAW(void __iomem *addr, uint32_t mask, uint32_t shift, uint32_t value);
static inline uint32_t LBC_RD_FIELD32_RAW(void __iomem *addr, uint32_t mask, uint32_t shift);


static inline void LBC_WR_FIELD32_RAW(void __iomem *addr, uint32_t mask, uint32_t shift, uint32_t value)
{
	uint32_t reg_val = readl(addr);
	reg_val &= (~mask);
	reg_val |= (value << shift) & mask;
	writel(reg_val, addr);
	mb();
	return;
}

static inline uint32_t LBC_RD_FIELD32_RAW(void __iomem *addr, uint32_t mask, uint32_t shift)
{
	uint32_t reg_val = readl(addr);
	reg_val = (reg_val & mask) >> shift;
	mb();
	return reg_val;
}

// ----------------------------------------------------------------------------
// reg operate
// ----------------------------------------------------------------------------

/* support power manager API */
#if IS_ENABLED(CONFIG_PM)
static int sunxi_lbc_suspend(struct device *dev)
{
	dev_info(dev, "lbc suspend okay\n");

	return 0;
}

static int sunxi_lbc_resume(struct device *dev)
{
	dev_info(dev, "lbc resume okay\n");

	return 0;
}

static const struct dev_pm_ops sunxi_lbc_pm_ops = {
	.suspend = sunxi_lbc_suspend,
	.resume = sunxi_lbc_resume,
};

#define SUNXI_MBUS_PM_OPS (&sunxi_lbc_pm_ops)
#else
#define SUNXI_MBUS_PM_OPS NULL
#endif

static void calibrate_delay_chain(void __iomem *reg_base, u32 delay_value)
{
	u32 val;

	if (delay_value <= 63 && delay_value != 0) {
		val = readl(reg_base + SMP_DELAY_CTRL_REG) | (delay_value);
		writel(val, reg_base + SMP_DELAY_CTRL_REG);
		mb();
		mb();
		val = readl(reg_base + SMP_DELAY_CTRL_REG) | (1 << 7);
		writel(val, reg_base + SMP_DELAY_CTRL_REG);

		val = readl(reg_base + SMP_DELAY_CTRL_REG);
		val |= 0x1 << 15;
		writel(val, reg_base + SMP_DELAY_CTRL_REG);
		mb();
		udelay(100);
	}

	val = readl(reg_base + SMP_DELAY_CTRL_REG);
	val &= ~(0x1 << 15);
	writel(val, reg_base + SMP_DELAY_CTRL_REG);
	mb();
	udelay(100);
	printk("calibrate delay chain finish!\n");
}

/*-----------------------register setting-----------------------*/

static void lbc_cs_set_addr_mux_type(void __iomem *reg_base, uint32_t cs_idx, uint8_t val)
{

	LBC_WR_FIELD32(reg_base + LBC_MODE_CTRL_REG + (LBC_CS_TIMING_REG_OFFSET * cs_idx),
					MS_MC_ADDR_MUX_TYPE, val);
}

static void lbc_cs_set_bus_endian(void __iomem *reg_base, uint32_t cs_idx, uint8_t val)
{
	uint32_t reg_val;

	reg_val = readl(reg_base + LBC_MODE_CTRL_REG + (LBC_CS_TIMING_REG_OFFSET * cs_idx));
	reg_val &= ~(0x1 << 16);
	reg_val |= ((val & 0x1) << 16);
	writel(reg_val, reg_base + LBC_MODE_CTRL_REG + (LBC_CS_TIMING_REG_OFFSET * cs_idx));
}

static void lbc_cs_set_bus_width(void __iomem *reg_base, uint32_t cs_idx, uint8_t val)
{
	uint32_t reg_val;

	reg_val = readl(reg_base + LBC_MODE_CTRL_REG + (LBC_CS_TIMING_REG_OFFSET * cs_idx));
	reg_val &= ~(0x3 << 12);
	reg_val |= ((val & 0x3) << 12);
	writel(reg_val, reg_base + LBC_MODE_CTRL_REG + (LBC_CS_TIMING_REG_OFFSET * cs_idx));
}

static void lbc_cs_set_dp_mode(void __iomem *reg_base, uint32_t cs_idx, uint8_t val)
{
	uint32_t reg_val;

	reg_val = readl(reg_base + LBC_DP_CTRL_REG + (LBC_CS_TIMING_REG_OFFSET * cs_idx));
	reg_val &= ~(0x1 << 12);
	reg_val |= ((val & 0x1) << 12);
	writel(reg_val, reg_base + LBC_DP_CTRL_REG + (LBC_CS_TIMING_REG_OFFSET * cs_idx));
}

static void lbc_cs_set_dp_parity(void __iomem *reg_base, uint32_t cs_idx, uint8_t val)
{
	uint32_t reg_val;

	reg_val = readl(reg_base + LBC_DP_CTRL_REG + (LBC_CS_TIMING_REG_OFFSET * cs_idx));
	reg_val &= ~(0x1 << 8);
	reg_val |= ((val & 0x1) << 8);
	writel(reg_val, reg_base + LBC_DP_CTRL_REG + (LBC_CS_TIMING_REG_OFFSET * cs_idx));
}

static void lbc_cs_set_dp_en(void __iomem *reg_base, uint32_t cs_idx, uint8_t val)
{
	uint32_t reg_val;

	reg_val = readl(reg_base + LBC_DP_CTRL_REG + (LBC_CS_TIMING_REG_OFFSET * cs_idx));
	reg_val &= ~(0x1 << 0);
	reg_val |= ((val & 0x1) << 0);
	writel(reg_val, reg_base + LBC_DP_CTRL_REG + (LBC_CS_TIMING_REG_OFFSET * cs_idx));
}


// wirte read fifo need more check
static void tx_fifo_write(void __iomem *reg_base, u32 iodl, u8 *txdata)
{
	u32 send_data_cnt;

	for (send_data_cnt = 0; send_data_cnt < iodl; send_data_cnt++) {
		// here may need to check tx fifo if full
		writeb(*txdata, reg_base + IDA_WDATA_REG);        //write data to write entrance
		mb();
		lbc_log(LOG_DEBUG, "send cnt: %u, itrp: 0x%08x\n", send_data_cnt, readl(reg_base + LBC_INT0_PENDING_REG));
		lbc_log(LOG_DEBUG, "ida write: %x\n", *txdata);
		// tx fifo
		//writel(IDA_TX_TRIG_IRPR, reg_base + LBC_INT0_PENDING_REG);			//w1c, clear tx full int
		txdata++;
	}
}

static void rx_fifo_read(sunxi_lbc_t *lbc, u32 rx_trig_level)
{
	u32 read_cnt;
	u32 data = 0;

	int i = 0;

	for (read_cnt = 0; read_cnt < rx_trig_level; read_cnt++) {	//trig level is byte number while read is in word
		data = readl(lbc->reg_addr + IDA_FIFO_STA_REG) >> 16 ;
		lbc_log(LOG_DEBUG, "rx_trig_level: %u, waterline: %u\n", rx_trig_level, data);
		if (data) {
			if (lbc->rx_index >= IDA_TEST_DATA_SIZE)
				lbc->rx_index = 0;

			*(uint32_t *)&lbc->ida_receive_buf[lbc->rx_index] = readl(lbc->reg_addr + IDA_RDATA_REG);

			for (i = 0; i < 4; i++) {
				lbc_log(LOG_DEBUG, "ida read: %x\n", lbc->ida_receive_buf[lbc->rx_index]);

				lbc->rx_index++;

				if (lbc->rx_index >= IDA_TEST_DATA_SIZE)
					lbc->rx_index = 0;
			}
		}
		mb();
	}
}

static void lbc_ida_set_burst_arb_gain(void __iomem *reg_base, uint8_t val)
{
	LBC_WR_FIELD32(IDA_TS_CTRL_REG + reg_base, MS_IDATC_IDA_BURST_ARB_GRAIN, val);
}

static void lbc_ida_set_burst_type(void __iomem *reg_base, uint8_t val)
{
	LBC_WR_FIELD32(IDA_TS_CTRL_REG + reg_base, MS_IDATC_IDA_BURST_TYPE, val);
}

static void lbc_ida_set_burst_length(void __iomem *reg_base,  uint8_t val)
{
	LBC_WR_FIELD32(IDA_TS_CTRL_REG + reg_base, MS_IDATC_IDA_BURST_LENGTH, val);
}

static void lbc_ida_set_trans_direction(void __iomem *reg_base, uint8_t val)
{
	LBC_WR_FIELD32(IDA_TS_CTRL_REG + reg_base, MS_IDATC_IDA_TS_DIR, val);
}

static void lbc_ida_trans_start(void __iomem *reg_base)
{
	LBC_WR_FIELD32(IDA_TS_CTRL_REG + reg_base, MS_IDATC_IDA_TS_START, 1);
}

static void lbc_ida_set_ts_addr(void __iomem *reg_base, uint32_t val)
{
	writel(val, reg_base + IDA_TS_ADDR_REG);
}

static void lbc_ida_set_ts_data_length(void __iomem *reg_base, uint16_t val)
{
	writel((val - 1), reg_base + IDA_TS_DATA_LEN_REG);
}

static void lbc_print_ida_regs(void __iomem *reg_base)
{
	lbc_log(LOG_INFO, "ida ts ctrl addr: 0x%08x, 0x%08x\n", (0x02810000 + IDA_TS_CTRL_REG), (readl(reg_base + IDA_TS_CTRL_REG)));
	lbc_log(LOG_INFO, "ida ts addr addr: 0x%08x, 0x%08x\n", (0x02810000 + IDA_TS_ADDR_REG), (readl(reg_base + IDA_TS_ADDR_REG)));
	lbc_log(LOG_INFO, "ida ts iodl addr: 0x%08x, 0x%08x\n", (0x02810000 + IDA_TS_DATA_LEN_REG), (readl(reg_base + IDA_TS_DATA_LEN_REG)));
}

static void sunxi_lbc_dump_dma_desc(struct __lbc_idma_des *desc, int size)
{
	int i;
	for (i = 0; i < size; i++) {
		u32 *x = (u32 *)(desc + i);
		pr_info("\t%d [0x%08lx]: %08x %08x %08x %08x\n",
				i, (unsigned long)(&desc[i]),
				x[0], x[1], x[2], x[3]);

	}
	pr_info("\n");
}

static void dma_transfer_start(void __iomem *reg_base)
{
	unsigned int val = 0;
	val = readl(reg_base + DMA_MODE_CFG_REG);
	val |= (0x1 << 0);
	writel(val, reg_base + DMA_MODE_CFG_REG);
}

static void lbc_dma_set_target_addr(struct __lbc_idma_des *p, uint32_t addr)
{
	p->target_start_addr = addr;
}


static void lbc_dma_set_iodl(struct __lbc_idma_des *p, uint16_t val)
{
	p->desc0.reg.desc_15t0_iodl = val;
}

static void lbc_dma_set_burst_type(struct __lbc_idma_des *p, uint8_t val)
{
	p->desc0.reg.desc_23_burst_type = val;
}

static void lbc_dma_set_burst_length(struct __lbc_idma_des *p, uint8_t val)
{
	p->desc0.reg.desc_22t16_burst_len = val;
}

static void lbc_dma_desc_cs(struct __lbc_idma_des *p, uint8_t val)
{
	p->desc0.reg.desc_26t25_cs = val & 0x3;
}

static void lbc_dma_desc_finish_flag(struct __lbc_idma_des *p, uint8_t val)
{
	p->desc0.reg.desc_27_dma_fin = val;
}

static void lbc_dma_desc_direction(struct __lbc_idma_des *p, uint8_t val)
{
	p->desc0.reg.desc_28_dir = val & 0x1;
}

static void lbc_dma_desc_set_wback(struct __lbc_idma_des *p, uint8_t val)
{
	p->desc0.reg.desc_30_wback = val & 0x1;
}

static void lbc_dma_set_desc_valid(struct __lbc_idma_des *p, uint8_t val)
{
	p->desc0.reg.desc_31_vld = val & 0x1;
}


static void lbc_dma_desc_init(sunxi_lbc_t *lbc_dev)
{
	lbc_dev->dma_target_addr = CSn_LADDR[0] + 0x4000;

	// init dma tx desc
	lbc_dev->dma_tx->dma_buffer_saddr = lbc_dev->dma_tx_buffer_phy >> 2;
	lbc_dev->dma_tx->next_desc_addr = 0x00000000;
	lbc_dma_set_burst_type(lbc_dev->dma_tx, lbc_dev->dma_cfg.dma_burst_type);
	lbc_dma_set_burst_length(lbc_dev->dma_tx, lbc_dev->dma_cfg.dma_burst_length);
	// address mode no need to config cs
	//lbc_dma_desc_cs(lbc_dev->dma_tx, cs_idx);
	lbc_dma_desc_finish_flag(lbc_dev->dma_tx, DMA_FINISH_DESC);
	lbc_dma_desc_direction(lbc_dev->dma_tx, DMA_READ_DIRECTION);
	lbc_dma_desc_set_wback(lbc_dev->dma_tx, DMA_WRITE_BACK_DISABLE);


	// init dma rx desc
	lbc_dev->dma_rx->dma_buffer_saddr = lbc_dev->dma_rx_buffer_phy >> 2;
	lbc_dev->dma_rx->next_desc_addr = 0x00000000;
	lbc_dma_set_burst_type(lbc_dev->dma_rx, lbc_dev->dma_cfg.dma_burst_type);
	lbc_dma_set_burst_length(lbc_dev->dma_rx, lbc_dev->dma_cfg.dma_burst_length);
	// address mode no need to set cs
	//lbc_dma_desc_cs(lbc_dev->dma_rx, cs_idx);
	lbc_dma_desc_finish_flag(lbc_dev->dma_rx, DMA_FINISH_DESC);
	lbc_dma_desc_direction(lbc_dev->dma_rx, DMA_WRITE_DIRECTION);
	lbc_dma_desc_set_wback(lbc_dev->dma_rx, DMA_WRITE_BACK_DISABLE);
}


// lclk = fclk / 2
static const struct lbc_cs_cfg lbc_default_cs_timing = {
	.cle_cycle_num = DEFAULT_CLE_CYCLE_NUM,
	.ale_cycle_num = DEFAULT_ALE_CYCLE_NUM,
	.dat_cycle_num = DEFAULT_DAT_CYCLE_NUM,

	// 0x00 LBC_MODE_CTRL_REG
	.addr_mux_type		= ADDR_MUX_AD_TYPE,			// 00: non mux, 01: ad mux, 10: aad mux
	.lbc_protocol 		= TIME_MODE_LBC1,			// 00: lbc mode0, 01: lbc mode1, 10: gpmc
	.bus_endian 		= LITTLE_BUS_ENDIAN,		// 0: big endian, 1: little endian
	.transfer_width 	= DATA_WIDTH_32_BIT,		// 00: 8bit, 01: 16bit, 10: 32bite
	.bus_addr_offset 	= DISABLE,					// 0: disable, 1: enable
	.rd_sync_type 		= SYNC_RD_WR_TYPE,			// 0: async, 1 sync
	.wr_sync_type 		= SYNC_RD_WR_TYPE,			// 0: async, 1 sync

	// 0x04 LBC_TIMESCALE_CTRL_REG
	.clk_divider		= DEFAULT_CLK_DIVIDER,		// 00: fclk/1, 01: fclk/2, 10: fclk/4, 11: fclk/8
	.clk_delay_time 	= CLK_DELAY_0_CYCLE,		// lclk-fclk phase delay cycles (0 ~ 7 cycles)

	// 0x08 LBC_READY_CTRL_REG
	.ready_timeout_time = 0xC1,
	.rd_ready_en		= ENABLE,					// read ready enable, 0: disalbe, 1: enable
	.wr_ready_en		= DISABLE,					// write ready enable, 0: disable, 1: enalbe
	.ready_delay_time 	= READY_DELAY_0_CYCLE,
	.rd_ready_mode 		= DETECT_READY_EVERY_TRANS,
	.wr_ready_mode		= DETECT_READY_FIRST_TRANS,
	.ready_polarity		= ACTIVE_HIGH,

	// 0x0C LBC_DP_CTRL_REG
	.dp_mode 			= ONE_DP_PER_BYTE,			// 0: DP per byte, 1: one DP only
	.dp_parity			= EVEN_PARITY,				// 0: odd parity, 1: even parity
	.dp_en				= ENABLE,					// 0: disable, 1: enable

	// 0x10 LBC_BE_CTRL_REG
	.be_parity_en 		= DISABLE,					// enable BE signal to participate in parity check, 0: disable 1: enable
	.de_polarity		= ACTIVE_HIGH,
	.be_polarity		= ACTIVE_HIGH,

	// 0x14 LBC_WAIT_CTRL_REG
	.wait_timeout_time 	= 0x0400,
	.wait_polarity		= ACTIVE_HIGH,
	.wait_en			= DISABLE,

	// 0x18 LBC_CS_TIMING_CTRL_REG
	.cs_polarity 		= ACTIVE_LOW,

	// 0x1C LBC_ALE_TIMING0_CTRL_REG
	.ale_polarity		= ACTIVE_HIGH,

	// 0x20 LBC_ALE_TIMING1_CTRL_REG

	// 0x24 LBC_WE_TIMING_CTRL_REG
	.we_polarity		= ACTIVE_HIGH,

	// 0x28 LBC_OE_TIMING0_CTRL_REG
	.oe_polarity		= ACTIVE_LOW,

	// 0x2C LBC_OE_TIMING1_CTRL_REG

	// 0x30 LBC_OE_TIMING2_CTRL_REG
	.oe_page_time 		= 0x00,
	.oe_page_en			= DISABLE,

	// 0x34 LBC_CYCLE_TIMING_CTRL_REG

	// 0x44 LBC_CLE_TIMING_CTRL_REG
	.cle_mode			= ENABLE_IN_SINGLE_TRANS,
	.cle_polarity		= ACTIVE_HIGH,
};



static const struct lbc_trans_cfg lbc_default_trans_cfg = {
	// 0x200 CH_MODE_CTRL_REG
	.user_sel_en 	= DISABLE,
	.fix_sel_en		= ENABLE,
	.addr_sel_en	= DISABLE,

	// 0x210 ~ 0x24C
	.cs_da_haddr[0] 	= LBC_CS0_HADDR,
	.cs_da_laddr[0] 	= LBC_CS0_LADDR,
	.cs_ida_haddr[0] 	= LBC_CS0_HADDR,
	.cs_ida_laddr[0]	= LBC_CS0_LADDR,

	.cs_da_haddr[1] 	= LBC_CS1_HADDR,
	.cs_da_laddr[1] 	= LBC_CS1_LADDR,
	.cs_ida_haddr[1] 	= LBC_CS1_HADDR,
	.cs_ida_laddr[1]	= LBC_CS1_LADDR,

	.cs_da_haddr[2] 	= LBC_CS2_HADDR,
	.cs_da_laddr[2] 	= LBC_CS2_LADDR,
	.cs_ida_haddr[2] 	= LBC_CS2_HADDR,
	.cs_ida_laddr[2]	= LBC_CS2_LADDR,

	.cs_da_haddr[3] 	= LBC_CS3_HADDR,
	.cs_da_laddr[3] 	= LBC_CS3_LADDR,
	.cs_ida_haddr[3] 	= LBC_CS3_HADDR,
	.cs_ida_laddr[3]	= LBC_CS3_LADDR,

	// 0x250 FIX_CS_REG
	.dma_fix_cs		= FIX_TO_CHANNEL0,
	.ida_fix_cs		= FIX_TO_CHANNEL0,
	.da_fix_cs		= FIX_TO_CHANNEL0,

	// 0x254 USER_CS_REG	// fix to cpu
	.cs_user_sel[0] = 0,
	.cs_user_sel[1] = 0,
	.cs_user_sel[2] = 0,
	.cs_user_sel[3] = 0,

	// 0x280 LBC_CLK_REG
	.lbc_clk_mode			= CLK_ALWAYS_ON,
	.lbc_rx_negative_sel	= NOT_REVERSE_PHASE,
	.lbc_tx_negative_sel	= REVERSE_PHASE,
	.lbc_fclk_sel			= CLK_FROM_CCU,
	.lbc_fclk_gate			= ENABLE,

	// 0x290 SMP_DELAY_CTRL_REG
	.samp_dl_sw_value 	= 32,
	.samp_dl_sw_en		= ENABLE,

	// 0x440 LBC_ARB_PRI_CFG_REG
	.dma_priority		= MIDDLE_PRIORITY,
	.ida_priority		= MIDDLE_PRIORITY,
	.da_priority		= MIDDLE_PRIORITY,
};


static const struct lbc_ida_cfg lbc_default_ida_cfg = {
	// 0x300 IDA_TS_CTRL_REG,
	.ida_burst_arb_grain 		= IDA_BURST_ARB_GRAIN_512_BYTE,
	.ida_burst_type_type		= BURST_TYPE_INCR,
	.ida_burst_type_length		= 0x1,  // val = length + 1
	.ida_dma_on					= DISABLE,

	// 0x528 IDA_FIFO_TRIGGER_LVL_REG,
	.ida_rx_trigger_lvl		= 0x1,
	.ida_tx_trigger_lvl		= 0x80,
};


static const struct lbc_da_cfg lbc_default_da_cfg = {
	// 0x400 DA_BST_SEL_REG,
	.da_cs_burst_sel[0] = BURST_SEL_AXI,
	.da_cs_burst_sel[1] = BURST_SEL_AXI,
	.da_cs_burst_sel[2] = BURST_SEL_AXI,
	.da_cs_burst_sel[3] = BURST_SEL_AXI,

	// 0x404 DA_BST_TYPE_REG,
	.da_burst_type_type[0] 		= BURST_TYPE_FIX,
	.da_burst_type_length[0] 	= 0x0,
	.da_burst_type_type[1] 		= BURST_TYPE_FIX,
	.da_burst_type_length[1] 	= 0x0,
	.da_burst_type_type[2] 		= BURST_TYPE_FIX,
	.da_burst_type_length[2] 	= 0x0,
	.da_burst_type_type[3] 		= BURST_TYPE_FIX,
	.da_burst_type_length[3] 	= 0x0,

	// 0x41C DA_CS_SADDR_REG
	.da_cs_saddr_reg[0] = 0x0,
	.da_cs_saddr_reg[1] = 0x0,
	.da_cs_saddr_reg[2] = 0x0,
	.da_cs_saddr_reg[3] = 0x0,

	// 0x524 DA_FIFO_TRIGGER_LVL_REG,
	.da_rx_trigger_lvl 	= 0x0,
	.da_tx_trigger_lvl 	= 0x0,
};

static const struct lbc_dma_cfg lbc_default_dma_cfg = {
	// 0x460 DMA_MODE_CFG_REG
	.dma_burst_arb_grain 	= DMA_BURST_ARB_GRAIN_512_BYTE,
	.dma_drq_en 			= DISABLE,

	.dma_burst_type			= BURST_TYPE_INCR,
	.dma_burst_length		= 0x7F,

	// 0x52C DMA_FIFO_TRIGGER_LVL_REG,
	.dma_rx_trigger_lvl		= 0x0,
	.dma_tx_trigger_lvl		= 0x0,
};

static const struct lbc_intr_cfg lbc_default_intr_cfg = {
		// 0x500 LBC_INT0_EN_REG
	.lbc_ida_rx_trig_int_en 	= ENABLE,
	.lbc_ida_tx_trig_int_en		= DISABLE,
	.lbc_da_rx_trig_int_en		= DISABLE,
	.lbc_da_tx_trig_int_en  	= DISABLE,
	.lbc_dp_int_en				= ENABLE,
	.lbc_odp_int_en				= ENABLE,
	.lbc_dma_des_invld_en		= ENABLE,
	.lbc_dma_trsf_done_int_en	= ENABLE,
	.lbc_ida_trsf_done_int_en	= ENABLE,
	.lbc_da_trsf_done_int_en	= DISABLE,
	.lbc_ready_timeout_int_en	= DISABLE,
	.lbc_cmd_done_int_en		= DISABLE,

	// 0x504 LBC_INT1_EN_REG
	.lbc_dma_timeout_int_en		= ENABLE,
	.lbc_ida_timeout_int_en 	= ENABLE,
	.lbc_da_timeout_int_en		= ENABLE,
	.lbc_dma_des_done_int_en	= DISABLE,
	.lbc_data_req_timeout_int_en = DISABLE,
	.lbc_dma_rx_trig_int_en		= DISABLE,
	.lbc_dma_tx_trig_int_en		= DISABLE,
};

static void configure_cs_time(struct lbc_cs_cfg *cs_cfg)
{
	uint32_t tq = 0;
	uint32_t cle_cyc_num = 0;
	uint32_t ale_cyc_num = 0;
	uint32_t data_cyc_num = 0;

	uint8_t ale_cyc_multiple = 1;

	if (IS_ERR_OR_NULL(cs_cfg)) {
		printk("invalid params to configure cs time\n");
		return;
	}

	tq = (0x1 << cs_cfg->clk_divider);
	cle_cyc_num = cs_cfg->cle_cycle_num;
	ale_cyc_num = cs_cfg->ale_cycle_num;
	data_cyc_num = cs_cfg->dat_cycle_num;
	if (cs_cfg->addr_mux_type == ADDR_MUX_AAD_TYPE)
		ale_cyc_multiple = 2;

	// 0x18 LBC_CS_TIMING_CTRL_REG
	printk("current cs with clk divider: %u, cycle num cle: %u, ale: %u, data: %u\n",
			cs_cfg->clk_divider, cle_cyc_num, ale_cyc_num, data_cyc_num);

	cs_cfg->cs_rdoff_time		= ((tq) * (cle_cyc_num + ale_cyc_num * ale_cyc_multiple + data_cyc_num + 3));
	cs_cfg->cs_wroff_time		= ((tq) * (cle_cyc_num + ale_cyc_num * ale_cyc_multiple + data_cyc_num));
	cs_cfg->cs_on_time			= 0x00;

	// 0x44 LBC_CLE_TIMING_CTRL_REG
	cs_cfg->cle_off_time		= ((tq) * cle_cyc_num);
	cs_cfg->cle_on_time			= 0x00;

	if (cs_cfg->addr_mux_type == ADDR_MUX_AAD_TYPE) {
		// 0x20 LBC_ALE_TIMING1_CTRL_REG
		cs_cfg->ale_aad_on_time  	= cs_cfg->cle_off_time;
		cs_cfg->ale_aad_rdoff_time	= cs_cfg->ale_aad_on_time + (tq * ale_cyc_num);
		cs_cfg->ale_aad_wroff_time	= cs_cfg->ale_aad_on_time + (tq * ale_cyc_num);

		// 0x1C LBC_ALE_TIMING0_CTRL_REG
		cs_cfg->ale_on_time			= cs_cfg->ale_aad_wroff_time;
		cs_cfg->ale_rdoff_time		= cs_cfg->ale_on_time + (tq * ale_cyc_num);
		cs_cfg->ale_wroff_time		= cs_cfg->ale_on_time + (tq * ale_cyc_num);
	} else {
		// 0x1C LBC_ALE_TIMING0_CTRL_REG
		cs_cfg->ale_on_time			= cs_cfg->cle_off_time;
		cs_cfg->ale_rdoff_time		= cs_cfg->ale_on_time + (tq * ale_cyc_num);
		cs_cfg->ale_wroff_time		= cs_cfg->ale_on_time + (tq * ale_cyc_num);

		// 0x20 LBC_ALE_TIMING1_CTRL_REG
		cs_cfg->ale_aad_rdoff_time	= 0x00;
		cs_cfg->ale_aad_wroff_time	= 0x00;
		cs_cfg->ale_aad_on_time  	= 0x00;
	}

	// 0x24 LBC_WE_TIMING_CTRL_REG
	cs_cfg->we_off_time 		= cs_cfg->cs_wroff_time;
	cs_cfg->we_on_time			= 0x00;

	// 0x28 LBC_OE_TIMING0_CTRL_REG
	cs_cfg->oe_off_time			= 0x00;
	cs_cfg->oe_on_time			= 0x00;

	// 0x2C LBC_OE_TIMING1_CTRL_REG
	cs_cfg->oe_aad_off_time 	= 0x00;
	cs_cfg->oe_aad_on_time		= 0x00;
	// 0x30 LBC_OE_TIMING2_CTRL_REG

	// 0x34 LBC_CYCLE_TIMING_CTRL_REG
	cs_cfg->rd_cycle_time	 	= cs_cfg->cs_rdoff_time;
	cs_cfg->wr_cycle_time		= cs_cfg->cs_wroff_time;

	// 0x38 LBC_ACCESS_TIMING_CTRL_REG
	cs_cfg->rd_access_time		= cs_cfg->ale_rdoff_time;
	cs_cfg->wr_access_time		= cs_cfg->ale_wroff_time;

	// 0x3C LBC_PAGE_TIMING_CTRL_REG
	cs_cfg->page_access_time 	= (tq * data_cyc_num);

	// 0x40 LBC_DATAMUX_TIMING_CTRL_REG
	cs_cfg->wr_data_on_time		= cs_cfg->ale_wroff_time;
}

static void init_cs_timing(sunxi_lbc_t *lbc)
{
	void __iomem *reg_base = NULL;
	uint32_t val = 0;
	int i = 0;

	if (IS_ERR_OR_NULL(lbc))
		return;

	reg_base = lbc->reg_addr;

	for (i = 0; i < 1; i++) {
		val = 0;
		LBC_SET_FIELD32(val, MS_MC_ADDR_MUX_TYPE, 	lbc->cs_cfg[i].addr_mux_type);
		LBC_SET_FIELD32(val, MS_MC_TIME_MODE, 		lbc->cs_cfg[i].lbc_protocol);
		LBC_SET_FIELD32(val, MS_MC_BUS_ENDIAN, 		lbc->cs_cfg[i].bus_endian);
		LBC_SET_FIELD32(val, MS_MC_BUS_DATA_WIDTH, 	lbc->cs_cfg[i].transfer_width);
		LBC_SET_FIELD32(val, MS_MC_BUS_ADDR_OFFSET, lbc->cs_cfg[i].bus_addr_offset);
		LBC_SET_FIELD32(val, MS_MC_RD_SYNC_TYPE, 	lbc->cs_cfg[i].rd_sync_type);
		LBC_SET_FIELD32(val, MS_MC_WR_SYNC_TYPE, 	lbc->cs_cfg[i].wr_sync_type);
		writel(val, ((LBC_MODE_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * i) + reg_base));

		val = 0;
		LBC_SET_FIELD32(val, MS_TSC_CLK_DIVIDER, 	lbc->cs_cfg[i].clk_divider);
		LBC_SET_FIELD32(val, MS_TSC_CLK_DELAY_TIME,	lbc->cs_cfg[i].clk_delay_time);
		writel(val, ((LBC_TIMESCALE_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * i) + reg_base));

		val = 0;
		LBC_SET_FIELD32(val, MS_RDYC_READY_TIMEOUT_TIME,	lbc->cs_cfg[i].ready_timeout_time);
		LBC_SET_FIELD32(val, MS_RDYC_RD_READY_EN,			lbc->cs_cfg[i].rd_ready_en);
		LBC_SET_FIELD32(val, MS_RDYC_WR_READY_EN, 			lbc->cs_cfg[i].wr_ready_en);
		LBC_SET_FIELD32(val, MS_RDYC_READY_DELAY_TIME, 		lbc->cs_cfg[i].ready_delay_time);
		LBC_SET_FIELD32(val, MS_RDYC_RD_READY_MODE, 		lbc->cs_cfg[i].rd_ready_mode);
		LBC_SET_FIELD32(val, MS_RDYC_WR_READY_MODE, 		lbc->cs_cfg[i].wr_ready_mode);
		LBC_SET_FIELD32(val, MS_RDYC_READY_POLARITY, 		lbc->cs_cfg[i].ready_polarity);
		writel(val, ((LBC_READY_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * i) + reg_base));

		val = 0;
		LBC_SET_FIELD32(val, MS_DPC_DP_MODE, 	lbc->cs_cfg[i].dp_mode);
		LBC_SET_FIELD32(val, MS_DPC_DP_PARITY, 	lbc->cs_cfg[i].dp_parity);
		LBC_SET_FIELD32(val, MS_DPC_DP_EN, 		lbc->cs_cfg[i].dp_en);
		writel(val, ((LBC_DP_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * i) + reg_base));

		val = 0;
		LBC_SET_FIELD32(val, MS_BEC_BE_PARITY_EN, 	lbc->cs_cfg[i].be_parity_en);
		LBC_SET_FIELD32(val, MS_BEC_DE_POLARITY, 	lbc->cs_cfg[i].de_polarity);
		LBC_SET_FIELD32(val, MS_BEC_BE_POLARITY, 	lbc->cs_cfg[i].be_polarity);
		printk("be_parity_en: %d, de_polarity: %d, be_polarity: %d\n", lbc->cs_cfg[i].be_parity_en, lbc->cs_cfg[i].de_polarity, lbc->cs_cfg[i].be_polarity);
		printk("0x10 lbc_be_ctrl reg : 0x%x\n", val);
		writel(val, ((LBC_BE_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * i) + reg_base));

		val = 0;
		LBC_SET_FIELD32(val, MS_WTC_WAIT_TIMEOUT_TIME, 	lbc->cs_cfg[i].wait_timeout_time);
		LBC_SET_FIELD32(val, MS_WTC_WAIT_POLARITY, 		lbc->cs_cfg[i].wait_polarity);
		LBC_SET_FIELD32(val, MS_WTC_WAIT_EN, 			lbc->cs_cfg[i].wait_en);
		writel(val, ((LBC_WAIT_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * i) + reg_base));

		val = 0;
		LBC_SET_FIELD32(val, MS_CSTC_CS_RDOFF_TIME, lbc->cs_cfg[i].cs_rdoff_time);
		LBC_SET_FIELD32(val, MS_CSTC_CS_WROFF_TIME, lbc->cs_cfg[i].cs_wroff_time);
		LBC_SET_FIELD32(val, MS_CSTC_CS_ON_TIME, 	lbc->cs_cfg[i].cs_on_time);
		LBC_SET_FIELD32(val, MS_CSTC_CS_POLARITY, 	lbc->cs_cfg[i].cs_polarity);
		writel(val, ((LBC_CS_TIMING_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * i) + reg_base));

		val = 0;
		LBC_SET_FIELD32(val, MS_ALET0C_ALE_RDOFF_TIME, 	lbc->cs_cfg[i].ale_rdoff_time);
		LBC_SET_FIELD32(val, MS_ALET0C_ALE_WROFF_TIME, 	lbc->cs_cfg[i].ale_wroff_time);
		LBC_SET_FIELD32(val, MS_ALET0C_ALE_ON_TIME, 	lbc->cs_cfg[i].ale_on_time);
		LBC_SET_FIELD32(val, MS_ALET0C_ALE_POLARITY, 	lbc->cs_cfg[i].ale_polarity);
		writel(val, ((LBC_ALE_TIMING0_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * i) + reg_base));

		val = 0;
		LBC_SET_FIELD32(val, MS_ALET1C_ALE_AAD_RDOFF_TIME, lbc->cs_cfg[i].ale_aad_rdoff_time);
		LBC_SET_FIELD32(val, MS_ALET1C_ALE_AAD_WROFF_TIME, lbc->cs_cfg[i].ale_aad_wroff_time);
		LBC_SET_FIELD32(val, MS_ALET1C_ALE_AAD_ON_TIME, 	lbc->cs_cfg[i].ale_aad_on_time);
		writel(val, ((LBC_ALE_TIMING1_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * i) + reg_base));

		val = 0;
		LBC_SET_FIELD32(val, MS_WETC_WE_OFF_TIME, 	lbc->cs_cfg[i].we_off_time);
		LBC_SET_FIELD32(val, MS_WETC_WE_ON_TIME, 	lbc->cs_cfg[i].we_on_time);
		LBC_SET_FIELD32(val, MS_WETC_WE_POLARITY, 	lbc->cs_cfg[i].we_polarity);
		writel(val, ((LBC_WE_TIMING_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * i) + reg_base));

		val = 0;
		LBC_SET_FIELD32(val, MS_OET0C_OE_OFF_TIME, 	lbc->cs_cfg[i].oe_off_time);
		LBC_SET_FIELD32(val, MS_OET0C_OE_ON_TIME, 	lbc->cs_cfg[i].oe_on_time);
		LBC_SET_FIELD32(val, MS_OET0C_OE_POLARITY, 	lbc->cs_cfg[i].oe_polarity);
		writel(val, ((LBC_OE_TIMING0_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * i) + reg_base));

		val = 0;
		LBC_SET_FIELD32(val, MS_OET1C_OE_AAD_OFF_TIME, 	lbc->cs_cfg[i].oe_aad_off_time);
		LBC_SET_FIELD32(val, MS_OET1C_OE_AAD_ON_TIME, 	lbc->cs_cfg[i].oe_aad_on_time);
		writel(val, ((LBC_OE_TIMING1_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * i) + reg_base));

		val = 0;
		LBC_SET_FIELD32(val, MS_OET2C_OE_PAGE_TIME, 	lbc->cs_cfg[i].oe_page_time);
		LBC_SET_FIELD32(val, MS_OET2C_OE_PAGE_EN, 		lbc->cs_cfg[i].oe_page_en);
		writel(val, ((LBC_OE_TIMING2_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * i) + reg_base));

		val = 0;
		LBC_SET_FIELD32(val, MS_CYCTC_RD_CYCLE_TIME, 	lbc->cs_cfg[i].rd_cycle_time);
		LBC_SET_FIELD32(val, MS_CYCTC_WR_CYCLE_TIME, 	lbc->cs_cfg[i].wr_cycle_time);
		writel(val, ((LBC_CYCLE_TIMING_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * i) + reg_base));

		val = 0;
		LBC_SET_FIELD32(val, MS_ATC_RD_ACCESS_TIME, 	lbc->cs_cfg[i].rd_access_time);
		LBC_SET_FIELD32(val, MS_ATC_WR_ACCESS_TIME, 	lbc->cs_cfg[i].wr_access_time);
		writel(val, ((LBC_ACCESS_TIMING_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * i) + reg_base));

		val = 0;
		LBC_SET_FIELD32(val, MS_PGTC_PAGE_ACCESS_TIME, lbc->cs_cfg[i].page_access_time);
		writel(val, ((LBC_PAGE_TIMING_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * i) + reg_base));

		val = 0;
		LBC_SET_FIELD32(val, MS_DATTC_WR_DATA_ON_TIME, lbc->cs_cfg[i].wr_data_on_time);
		writel(val, ((LBC_DATAMUX_TIMING_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * i) + reg_base));

		val = 0;
		LBC_SET_FIELD32(val, MS_CLETC_CLE_OFF_TIME, lbc->cs_cfg[i].cle_off_time);
		LBC_SET_FIELD32(val, MS_CLETC_CLE_ON_TIME, 	lbc->cs_cfg[i].cle_on_time);
		LBC_SET_FIELD32(val, MS_CLETC_CLE_MODE, 	lbc->cs_cfg[i].cle_mode);
		LBC_SET_FIELD32(val, MS_CLETC_CLE_POLARITY, lbc->cs_cfg[i].cle_polarity);
		writel(val, ((LBC_CLE_TIMING_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * i) + reg_base));

		printk("init cs %d timing done\n", i);
	}

}

static void init_trans_cfg(sunxi_lbc_t *lbc)
{
	void __iomem *reg_base = NULL;
	uint32_t val = 0;
	if (IS_ERR_OR_NULL(lbc))
		return;

	reg_base = lbc->reg_addr;
	val = 0;
	LBC_SET_FIELD32(val, MS_CHMC_USER_SEL_EN, 	lbc->trans_cfg.user_sel_en);
	LBC_SET_FIELD32(val, MS_CHMC_FIX_SEL_EN, 	lbc->trans_cfg.fix_sel_en);
	LBC_SET_FIELD32(val, MS_CHMC_ADDR_SEL_EN, 	lbc->trans_cfg.addr_sel_en);
	writel(val, (CH_MODE_CTRL_REG + reg_base));

	writel(lbc->trans_cfg.cs_da_haddr[0],  (CS0_DA_HADDR_REG + reg_base));
	writel(lbc->trans_cfg.cs_da_laddr[0],  (CS0_DA_LADDR_REG + reg_base));
	writel(lbc->trans_cfg.cs_ida_haddr[0], (CS0_IDA_HADDR_REG + reg_base));
	writel(lbc->trans_cfg.cs_ida_laddr[0], (CS0_IDA_LADDR_REG + reg_base));

	writel(lbc->trans_cfg.cs_da_haddr[1],  (CS1_DA_HADDR_REG + reg_base));
	writel(lbc->trans_cfg.cs_da_laddr[1],  (CS1_DA_LADDR_REG + reg_base));
	writel(lbc->trans_cfg.cs_ida_haddr[1], (CS1_IDA_HADDR_REG + reg_base));
	writel(lbc->trans_cfg.cs_ida_laddr[1], (CS1_IDA_LADDR_REG + reg_base));

	writel(lbc->trans_cfg.cs_da_haddr[2],  (CS2_DA_HADDR_REG + reg_base));
	writel(lbc->trans_cfg.cs_da_laddr[2],  (CS2_DA_LADDR_REG + reg_base));
	writel(lbc->trans_cfg.cs_ida_haddr[2], (CS2_IDA_HADDR_REG + reg_base));
	writel(lbc->trans_cfg.cs_ida_laddr[2], (CS2_IDA_LADDR_REG + reg_base));

	writel(lbc->trans_cfg.cs_da_haddr[3],  (CS3_DA_HADDR_REG + reg_base));
	writel(lbc->trans_cfg.cs_da_laddr[3],  (CS3_DA_LADDR_REG + reg_base));
	writel(lbc->trans_cfg.cs_ida_haddr[3], (CS3_IDA_HADDR_REG + reg_base));
	writel(lbc->trans_cfg.cs_ida_laddr[3], (CS3_IDA_LADDR_REG + reg_base));

	val = 0;
	LBC_SET_FIELD32(val, MS_FCS_DMA_FIX_CS, lbc->trans_cfg.dma_fix_cs);
	LBC_SET_FIELD32(val, MS_FCS_IDA_FIX_CS, lbc->trans_cfg.ida_fix_cs);
	LBC_SET_FIELD32(val, MS_FCS_DA_FIX_CS,  lbc->trans_cfg.da_fix_cs);
	writel(val, FIX_CS_REG + reg_base);

	val = 0;
	LBC_SET_FIELD32(val, MS_UCS_CS0_USER_SEL, lbc->trans_cfg.cs_user_sel[0]);
	LBC_SET_FIELD32(val, MS_UCS_CS1_USER_SEL, lbc->trans_cfg.cs_user_sel[1]);
	LBC_SET_FIELD32(val, MS_UCS_CS2_USER_SEL, lbc->trans_cfg.cs_user_sel[2]);
	LBC_SET_FIELD32(val, MS_UCS_CS3_USER_SEL, lbc->trans_cfg.cs_user_sel[3]);
	writel(val, USER_CS_REG + reg_base);

	val = 0;
	LBC_SET_FIELD32(val, MS_CLK_CLK_MODE, 			lbc->trans_cfg.lbc_clk_mode);
	LBC_SET_FIELD32(val, MS_CLK_RX_NEGATIVE_SEL,	lbc->trans_cfg.lbc_rx_negative_sel);
	LBC_SET_FIELD32(val, MS_CLK_TX_NEGATIVE_SEL, 	lbc->trans_cfg.lbc_tx_negative_sel);
	LBC_SET_FIELD32(val, MS_CLK_FCLK_SEL, 			lbc->trans_cfg.lbc_fclk_sel);
	LBC_SET_FIELD32(val, MS_CLK_FCLK_GATE, 			lbc->trans_cfg.lbc_fclk_gate);
	printk("0x280: 0x%x", val);
	writel(val, LBC_CLK_REG + reg_base);

	val = 0;
	LBC_SET_FIELD32(val, MS_PRIC_DMA_PRIORITY, 	lbc->trans_cfg.dma_priority);
	LBC_SET_FIELD32(val, MS_PRIC_IDA_PRIORITY, 	lbc->trans_cfg.ida_priority);
	LBC_SET_FIELD32(val, MS_PRIC_DA_PRIORITY, 	lbc->trans_cfg.da_priority);
	printk("0x440: 0x%x", val);
	writel(val, LBC_ARB_PRI_CFG_REG + reg_base);

	printk("init trans config done\n");

	if (lbc->trans_cfg.samp_dl_sw_en == ENABLE) {
		calibrate_delay_chain(reg_base, lbc->trans_cfg.samp_dl_sw_value);
	}
}

static void set_da_cfg(sunxi_lbc_t *lbc)
{
	void __iomem *reg_base = NULL;
	uint32_t val = 0;

	if (IS_ERR_OR_NULL(lbc))
		return;

	reg_base = lbc->reg_addr;

	LBC_SET_FIELD32(val, MS_DABSTS_DA_CS0_BURST_SEL, lbc->da_cfg.da_cs_burst_sel[0]);
	LBC_SET_FIELD32(val, MS_DABSTS_DA_CS1_BURST_SEL, lbc->da_cfg.da_cs_burst_sel[1]);
	LBC_SET_FIELD32(val, MS_DABSTS_DA_CS2_BURST_SEL, lbc->da_cfg.da_cs_burst_sel[2]);
	LBC_SET_FIELD32(val, MS_DABSTS_DA_CS3_BURST_SEL, lbc->da_cfg.da_cs_burst_sel[3]);
	writel(val, DA_BST_SEL_REG + reg_base);

	val = 0;
	LBC_SET_FIELD32(val, MS_DABSTT_DA_CS0_BURST_TYPE, 	lbc->da_cfg.da_burst_type_type[0]);
	LBC_SET_FIELD32(val, MS_DABSTT_DA_CS0_BURST_LENGTH, lbc->da_cfg.da_burst_type_length[0]);

	LBC_SET_FIELD32(val, MS_DABSTT_DA_CS1_BURST_TYPE, 	lbc->da_cfg.da_burst_type_type[1]);
	LBC_SET_FIELD32(val, MS_DABSTT_DA_CS1_BURST_LENGTH, lbc->da_cfg.da_burst_type_length[1]);

	LBC_SET_FIELD32(val, MS_DABSTT_DA_CS2_BURST_TYPE, 	lbc->da_cfg.da_burst_type_type[2]);
	LBC_SET_FIELD32(val, MS_DABSTT_DA_CS2_BURST_LENGTH, lbc->da_cfg.da_burst_type_length[2]);

	LBC_SET_FIELD32(val, MS_DABSTT_DA_CS3_BURST_TYPE, 	lbc->da_cfg.da_burst_type_type[3]);
	LBC_SET_FIELD32(val, MS_DABSTT_DA_CS3_BURST_LENGTH, lbc->da_cfg.da_burst_type_length[3]);
	writel(val, DA_BST_TYPE_REG + reg_base);

	writel(lbc->da_cfg.da_cs_saddr_reg[0], DA_CS0_SADDR_REG + reg_base);
	writel(lbc->da_cfg.da_cs_saddr_reg[1], DA_CS1_SADDR_REG + reg_base);
	writel(lbc->da_cfg.da_cs_saddr_reg[2], DA_CS2_SADDR_REG + reg_base);
	writel(lbc->da_cfg.da_cs_saddr_reg[3], DA_CS3_SADDR_REG + reg_base);

	val = 0;
	LBC_SET_FIELD32(val, MS_DAFTL_DA_RX_TRIGGER_LVL, lbc->da_cfg.da_rx_trigger_lvl);
	LBC_SET_FIELD32(val, MS_DAFTL_DA_TX_TRIGGER_LVL, lbc->da_cfg.da_tx_trigger_lvl);
	writel(val, DA_FIFO_TRIGGER_LVL_REG + reg_base);

	printk("set da config done\n");
}

static void set_ida_cfg(sunxi_lbc_t *lbc)
{
	void __iomem *reg_base = NULL;
	uint32_t val = 0;

	if (IS_ERR_OR_NULL(lbc))
		return;

	reg_base = lbc->reg_addr;

	LBC_SET_FIELD32(val, MS_IDATC_IDA_BURST_ARB_GRAIN,  lbc->ida_cfg.ida_burst_arb_grain);
	LBC_SET_FIELD32(val, MS_IDATC_IDA_BURST_TYPE, 		lbc->ida_cfg.ida_burst_type_type);
	LBC_SET_FIELD32(val, MS_IDATC_IDA_BURST_LENGTH, 	lbc->ida_cfg.ida_burst_type_length);
	LBC_SET_FIELD32(val, MS_IDATC_IDA_DMA_ON, 			lbc->ida_cfg.ida_dma_on);
	writel(val, IDA_TS_CTRL_REG + reg_base);

	val = 0;
	LBC_SET_FIELD32(val, MS_IDAFTL_IDA_RX_TRIGGER_LVL, lbc->ida_cfg.ida_rx_trigger_lvl);
	LBC_SET_FIELD32(val, MS_IDAFTL_IDA_TX_TRIGGER_LVL, lbc->ida_cfg.ida_tx_trigger_lvl);
	writel(val, IDA_FIFO_TRIGGER_LVL_REG + reg_base);

	printk("set ida config done\n");
}

static void set_dma_cfg(sunxi_lbc_t *lbc)
{
	void __iomem *reg_base = NULL;
	uint32_t val = 0;

	if (IS_ERR_OR_NULL(lbc))
		return;

	reg_base = lbc->reg_addr;

	LBC_SET_FIELD32(val, MS_DMAMC_DMA_BURST_ARB_GRAIN, 	lbc->dma_cfg.dma_burst_arb_grain);
	LBC_SET_FIELD32(val, MS_DMAMC_DMA_DRQ_EN, 			lbc->dma_cfg.dma_drq_en);
	writel(val, DMA_MODE_CFG_REG + reg_base);

	val = 0;
	LBC_SET_FIELD32(val, MS_DMAFTL_DMA_RX_TRIGGER_LVL, lbc->dma_cfg.dma_rx_trigger_lvl);
	LBC_SET_FIELD32(val, MS_DMAFTL_DMA_TX_TRIGGER_LVL, lbc->dma_cfg.dma_tx_trigger_lvl);
	writel(val, DMA_FIFO_TRIGGER_LVL_REG + reg_base);

	lbc_dma_desc_init(lbc);
	printk("set dma config done\n");
}

static void set_intr_cfg(sunxi_lbc_t *lbc)
{
	void __iomem *reg_base = NULL;
	uint32_t val = 0;

	if (IS_ERR_OR_NULL(lbc))
		return;

	reg_base = lbc->reg_addr;

	LBC_SET_FIELD32(val, MS_IT0E_IDA_RX_TRIG_EN, 	lbc->intr_cfg.lbc_ida_rx_trig_int_en);
	LBC_SET_FIELD32(val, MS_IT0E_IDA_TX_TRIG_EN, 	lbc->intr_cfg.lbc_ida_tx_trig_int_en);
	LBC_SET_FIELD32(val, MS_IT0E_DA_RX_TRIG_EN, 	lbc->intr_cfg.lbc_da_rx_trig_int_en);
	LBC_SET_FIELD32(val, MS_IT0E_DA_TX_TRIG_EN, 	lbc->intr_cfg.lbc_da_tx_trig_int_en);
	LBC_SET_FIELD32(val, MS_IT0E_DP_EN,				lbc->intr_cfg.lbc_dp_int_en);
	LBC_SET_FIELD32(val, MS_IT0E_ODP_EN,			lbc->intr_cfg.lbc_odp_int_en);
	LBC_SET_FIELD32(val, MS_IT0E_DMA_DES_INVLD_EN, 	lbc->intr_cfg.lbc_dma_des_invld_en);
	LBC_SET_FIELD32(val, MS_IT0E_DMA_TRSF_DONE_EN, 	lbc->intr_cfg.lbc_dma_trsf_done_int_en);
	LBC_SET_FIELD32(val, MS_IT0E_IDA_TRSF_DONE_EN, 	lbc->intr_cfg.lbc_ida_trsf_done_int_en);
	LBC_SET_FIELD32(val, MS_IT0E_DA_TRSF_DONE_EN, 	lbc->intr_cfg.lbc_da_trsf_done_int_en);
	LBC_SET_FIELD32(val, MS_IT0E_READY_TIMEOUT_EN, 	lbc->intr_cfg.lbc_ready_timeout_int_en);
	LBC_SET_FIELD32(val, MS_IT0E_CMD_DONE_EN, 		lbc->intr_cfg.lbc_cmd_done_int_en);
	writel(val, LBC_INT0_EN_REG + reg_base);
	printk("int0en: 0x%x\n", val);

	val = 0;
	LBC_SET_FIELD32(val, MS_IT1E_DMA_TIMEOUT_EN, 	lbc->intr_cfg.lbc_dma_timeout_int_en);
	LBC_SET_FIELD32(val, MS_IT1E_IDA_TIMEOUT_EN, 	lbc->intr_cfg.lbc_ida_timeout_int_en);
	LBC_SET_FIELD32(val, MS_IT1E_DA_TIMEOUT_EN, 	lbc->intr_cfg.lbc_da_timeout_int_en);
	LBC_SET_FIELD32(val, MS_IT1E_DMA_DES_DONE_EN, 	lbc->intr_cfg.lbc_dma_des_done_int_en);
	LBC_SET_FIELD32(val, MS_IT1E_DATA_REQ_TIMEOUT_EN, lbc->intr_cfg.lbc_data_req_timeout_int_en);
	LBC_SET_FIELD32(val, MS_IT1E_DMA_RX_TRIG_EN, 	lbc->intr_cfg.lbc_dma_rx_trig_int_en);
	LBC_SET_FIELD32(val, MS_IT1E_DMA_TX_TRIG_EN, 	lbc->intr_cfg.lbc_dma_tx_trig_int_en);
	writel(val, LBC_INT1_EN_REG + reg_base);
	printk("int1en: 0x%x\n", val);

	printk("init interrupt en done\n");
}

static void lbc_mode1_32bit_default_config(sunxi_lbc_t *lbc)
{
	lbc->cs_cfg[0] = lbc_default_cs_timing;
	lbc->cs_cfg[1] = lbc_default_cs_timing;
	lbc->cs_cfg[2] = lbc_default_cs_timing;
	lbc->cs_cfg[3] = lbc_default_cs_timing;

	configure_cs_time(&lbc->cs_cfg[0]);
	configure_cs_time(&lbc->cs_cfg[1]);
	configure_cs_time(&lbc->cs_cfg[2]);
	configure_cs_time(&lbc->cs_cfg[3]);

	lbc->trans_cfg 	= lbc_default_trans_cfg;
	lbc->ida_cfg 	= lbc_default_ida_cfg;
	lbc->da_cfg 	= lbc_default_da_cfg;
	lbc->dma_cfg 	= lbc_default_dma_cfg;
	lbc->intr_cfg 	= lbc_default_intr_cfg;
}

static void lbc_mode1_32bit_default_config_init(sunxi_lbc_t *lbc)
{
	init_cs_timing(lbc);
	set_da_cfg(lbc);
	set_ida_cfg(lbc);
	set_dma_cfg(lbc);
	init_trans_cfg(lbc);
	set_intr_cfg(lbc);
}

/*-----------------------register setting-----------------------*/

/*
static void reg_write_check(void __iomem *reg_base, uint32_t offset, uint32_t val)
{
	uint32_t reg_val;
	int try_count = 0;
	bool failed = false;
	printk("going to set reg 0x%08x, 0x%08x\n", 0x02810000 + offset, val);
	do {
		writel(val, reg_base + offset);
		mb();
		udelay(1);
		reg_val = readl(reg_base + offset);

		try_count++;
		if (try_count > 1000) {
			failed = true;
			break;
		}
	} while (reg_val != val);

	if (failed) {
		printk("set reg failed!!!!!!\n");
	} else {
		printk("set reg done\n");
	}

}
*/

static void lbc_da_write(void __iomem *tsa_va, const uint8_t *data, size_t len)
{
	/* gpmc mode need to set to incr-1(single)
	if (gpmc_asic) {
		lbc_da_set_burst_select(lbc_dev->reg_addr, 0, BURST_SEL_SOFT);
		lbc_da_set_burst_type(lbc_dev->reg_addr, 0, BURST_TYPE_INCR);
		lbc_da_set_burst_length(lbc_dev->reg_addr, 0, 0x0);
	}
	*/

	uint32_t i = 0;
	uint32_t sum_cnt = len / 4;
	uint32_t *pdata = (uint32_t *)data;

	uint32_t addr_offset = 0;

	for (i = 0; i < sum_cnt; i++) {
		writel(*pdata, (tsa_va + addr_offset));
		lbc_log(LOG_DEBUG, "write addr: 0x%08x, val: 0x%08x\n", (uint32_t)(tsa_va + addr_offset), *pdata);
		addr_offset += 0x4;
		pdata++;
	}

	mb();
}



static void lbc_da_read(void __iomem *tsa_va, uint8_t *data, size_t len)
{
	uint32_t i = 0;
	uint32_t sum_cnt = len / 4;
	uint32_t *pdata = (uint32_t *)data;

	uint32_t addr_offset = 0;
	for (i = 0; i < sum_cnt; i++) {
		*pdata = readl(tsa_va + addr_offset);
		lbc_log(LOG_DEBUG, "read tsa: %08x, offset: %08x, data: %x\n", (uint32_t)tsa_va, addr_offset, *pdata);
		addr_offset += 0x04;
		pdata++;
	}
	mb();
}

static void lbc_ida_write(sunxi_lbc_t *lbc_dev, uint32_t tsa, uint8_t *data, size_t len, bool gpmc_asic, bool print_info)
{
	unsigned long timeout = 0;

	lbc_dev->ida_result = TRANS_NONE;
	lbc_ida_set_burst_arb_gain(lbc_dev->reg_addr, 0x3);
	if (gpmc_asic) {
		lbc_ida_set_burst_type(lbc_dev->reg_addr, BURST_TYPE_INCR);
		lbc_ida_set_burst_length(lbc_dev->reg_addr, 0);
	} else {
		lbc_ida_set_burst_type(lbc_dev->reg_addr, BURST_TYPE_INCR);
		lbc_ida_set_burst_length(lbc_dev->reg_addr, lbc_dev->burst_mode);
	}

	// write to ram
	lbc_ida_set_ts_addr(lbc_dev->reg_addr, (uint32_t)tsa);
	lbc_ida_set_ts_data_length(lbc_dev->reg_addr, len);
	lbc_ida_set_trans_direction(lbc_dev->reg_addr, IDA_WRITE_DIRECTION);

	lbc_ida_trans_start(lbc_dev->reg_addr);

	if (print_info || lbc_log_level == LOG_DEBUG)
		lbc_print_ida_regs(lbc_dev->reg_addr);

	mb();
	tx_fifo_write(lbc_dev->reg_addr, len, data);
	mb();

	timeout = wait_event_timeout(lbc_dev->ida_wait, lbc_dev->ida_result, 2*HZ);
	if (0 == timeout) {
		printk("%s->%d, ida tx time out\n", __func__, __LINE__);
	} else {
		lbc_log(LOG_DEBUG, "%s->%d , ida tx ok\n", __func__, __LINE__);
	}
	lbc_dev->ida_result = TRANS_NONE;
	mb();
}

static void lbc_ida_read(sunxi_lbc_t *lbc_dev, uint32_t tsa, size_t len, bool gpmc_asic, bool print_info)
{
	unsigned long timeout = 0;

	lbc_dev->ida_result = TRANS_NONE;
	lbc_ida_set_burst_arb_gain(lbc_dev->reg_addr, 0x3);
	if (gpmc_asic) {
		lbc_ida_set_burst_type(lbc_dev->reg_addr, BURST_TYPE_INCR);
		lbc_ida_set_burst_length(lbc_dev->reg_addr, 0x0);
	} else {
		lbc_ida_set_burst_type(lbc_dev->reg_addr, BURST_TYPE_INCR);
		lbc_ida_set_burst_length(lbc_dev->reg_addr, lbc_dev->burst_mode);
	}

	lbc_ida_set_trans_direction(lbc_dev->reg_addr, IDA_READ_DIRECTION);

	// read from ram
	lbc_ida_set_ts_addr(lbc_dev->reg_addr, (uint32_t)tsa);
	lbc_ida_set_ts_data_length(lbc_dev->reg_addr, len);

	lbc_ida_trans_start(lbc_dev->reg_addr);

	if (print_info || lbc_log_level == LOG_DEBUG)
		lbc_print_ida_regs(lbc_dev->reg_addr);

	timeout = wait_event_timeout(lbc_dev->ida_wait, lbc_dev->ida_result, 2*HZ);
	if (0 == timeout) {
		printk("%s->%d, ida rx time out\n", __func__, __LINE__);
	} else {
		lbc_log(LOG_DEBUG, "%s->%d , ida rx ok\n", __func__, __LINE__);
	}
	lbc_dev->ida_result = TRANS_NONE;
	mb();
}

static void lbc_dma_write(sunxi_lbc_t *lbc_dev, uint32_t tsa, size_t len, bool print_info)
{
	unsigned long timeout = 0;

	lbc_dev->dma_result = TRANS_NONE;

	writel((lbc_dev->dma_tx_phy >> 2), (lbc_dev->reg_addr + DMA_DES_ADDR_REG));
	lbc_dev->dma_tx->target_start_addr = (uint32_t)tsa; // (CSn_LADDR[cs_idx] + offset);
	lbc_dma_set_iodl(lbc_dev->dma_tx, (len - 1));

	dma_wmb();
	lbc_dma_set_desc_valid(lbc_dev->dma_tx, DMA_CUR_DESC_VALID);

	if (print_info || lbc_log_level == LOG_DEBUG)
		sunxi_lbc_dump_dma_desc(lbc_dev->dma_tx, 1);

	dma_transfer_start(lbc_dev->reg_addr);

	timeout = wait_event_timeout(lbc_dev->dma_wait, lbc_dev->dma_result, 2*HZ);
	if (0 == timeout) {
		printk("%s[%d]: dma tx time out\n", __func__, __LINE__);
	} else {
		lbc_log(LOG_DEBUG, "%s->%d, dma tx ok\n", __func__, __LINE__);
	}

	mb();
}

static void lbc_dma_read(sunxi_lbc_t *lbc_dev, uint32_t tsa, size_t len, bool print_info)
{
	unsigned long timeout = 0;

	lbc_dev->dma_result = TRANS_NONE;

	writel((lbc_dev->dma_rx_phy >> 2), (lbc_dev->reg_addr + DMA_DES_ADDR_REG));
	lbc_dev->dma_rx->target_start_addr = (uint32_t)tsa;

	lbc_dma_set_iodl(lbc_dev->dma_rx, (len - 1));
	dma_wmb();
	lbc_dma_set_desc_valid(lbc_dev->dma_rx, DMA_CUR_DESC_VALID);

#ifdef LBC_DEBUG
	if (print_info || lbc_log_level == LOG_DEBUG)
		sunxi_lbc_dump_dma_desc(lbc_dev->dma_rx, 1);
#endif
	dma_transfer_start(lbc_dev->reg_addr);

	timeout = wait_event_timeout(lbc_dev->dma_wait, lbc_dev->dma_result, 2*HZ);
	if (0 == timeout) {
		lbc_log(LOG_INFO, "%s[%d]: dma rx time out\n", __func__, __LINE__);
		return;
	} else  {
		lbc_log(LOG_DEBUG, "%s->%d , dma rx ok\n", __func__, __LINE__);
	}
	if (dma_wait_time > 0)
		udelay(dma_wait_time);

	mb();

}

static ssize_t lbc_write_ctrl(struct device *dev,
					 struct device_attribute *da, const char *buf, size_t len)
{
	int i = 0;

	uint32_t iodl = 0;
	uint32_t cs_idx = 0;
	sunxi_lbc_t *lbc_dev = NULL;
	bool gpmc_asic = false;

	int ret;

	uint32_t *pdata = NULL;

	ret = kstrtou32(buf, 0, &cs_idx);
	if (ret || cs_idx >= CS_NUMS) {
		dev_err(dev, "invalid cs num: %d\n", cs_idx);
		return len;
	}

	lbc_dev = dev_get_drvdata(dev);
	if (IS_ERR_OR_NULL(lbc_dev)) {
		dev_err(dev, "failed to get lbc dev\n");
		return len;
	}

	printk("write to cs %d with mode  %d\n", cs_idx, lbc_dev->cs_cfg[cs_idx].transfer_mode);

	mutex_lock(&lbc_lock);

	if (lbc_dev->cs_cfg[cs_idx].lbc_protocol == TIME_MODE_GPMC
			&& lbc_dev->cs_cfg[cs_idx].sub_time_mode == SUB_TM_ASIC) {
		gpmc_asic = true;
		printk("use gpmc to asic\n");
	}

	if (lbc_dev->cs_cfg[cs_idx].transfer_mode == TRANSFER_MODE_DIRECT_ACCESS) {
		for (i = 0; i < DA_TEST_DATA_SIZE; i++) {
			lbc_dev->da_receive_buf[i] = 0;
			//lbc_dev->da_send_buf[i] = get_random_int() % 256;
			lbc_dev->da_send_buf[i] = i % 256;
		}

		// write to 0x4 ~ 0xf
		iodl = DA_TEST_DATA_SIZE;
		lbc_da_write((lbc_dev->data_sram_addr[cs_idx]), lbc_dev->da_send_buf, iodl);
		pdata = (uint32_t *)lbc_dev->da_send_buf;
		for (i = 0; i < (iodl / 4); i++) {
			printk("write addr: %08x, data: %08x\n",  (i * 4), *pdata);
			pdata++;
		}

		dev_info(dev, "DA tx ok\n");
	} else if (lbc_dev->cs_cfg[cs_idx].transfer_mode == TRANSFER_MODE_INTERRUPT) {
		for (i = 0; i < IDA_TEST_DATA_SIZE; i++) {
			lbc_dev->ida_receive_buf[i] = 0;
			//lbc_dev->ida_send_buf[i] = get_random_int() % 256;
			lbc_dev->ida_send_buf[i] = i % 256;
		}

		// write to ram
		// write to reg 0x04 ~ 0xf
		iodl =  IDA_TEST_DATA_SIZE;
		lbc_ida_write(lbc_dev, (CSn_LADDR[cs_idx] + 0x4000), lbc_dev->ida_send_buf, iodl, gpmc_asic, true);
		pdata = (uint32_t *)lbc_dev->ida_send_buf;
		for (i = 0; i < (iodl / 4); i++) {
			printk("write addr: %08x, data: %08x\n", (0x4000 + i * 4), *pdata);
			pdata++;
		}

	} else if (lbc_dev->cs_cfg[cs_idx].transfer_mode == TRANSFER_MODE_DMA) {
		for (i = 0; i < DMA_DATA_MAX; i++) {
			lbc_dev->dma_tx_buffer[i] = i % 256;
			lbc_dev->dma_rx_buffer[i] = 0;
		}

		if (lbc_log_level == LOG_DEBUG) {
			for (i = 0; i < 32; i++) {
				printk("dma tx: %d, %x\n", i, lbc_dev->dma_tx_buffer[i]);
			}
		}

		iodl = DMA_DATA_MAX;
		lbc_dma_write(lbc_dev, (CSn_LADDR[cs_idx] + 0x4000), iodl, true);

	}

	mutex_unlock(&lbc_lock);
	return len;
}

static ssize_t lbc_fpga_io_ctrl(struct device *dev,
					 struct device_attribute *da, const char *buf, size_t len)
{
	int ret;
	sunxi_lbc_t *lbc_dev = NULL;

	lbc_dev = dev_get_drvdata(dev);
	if (IS_ERR_OR_NULL(lbc_dev)) {
		dev_err(dev, "failed to get lbc dev\n");
		return len;
	}

	ret = gpio_direction_output(lbc_dev->fpga_reset_io, 0);
	if (ret < 0) {
		dev_err(dev, "%s->%d\n", __func__, __LINE__);
		return ret;
	}

	mdelay(2000);

	ret = gpio_direction_output(lbc_dev->fpga_reset_io, 1);
	if (ret < 0) {
		dev_err(dev, "%s->%d\n", __func__, __LINE__);
		return ret;
	}
	mdelay(300);
	return len;
}

static ssize_t lbc_read_ctrl(struct device *dev,
					 struct device_attribute *da, const char *buf, size_t len)
{
	int i = 0;
	sunxi_lbc_t *lbc_dev = NULL;
	uint32_t cs_idx = 0;

	uint32_t iodl = 0;
	uint32_t *pdata = NULL;

	bool gpmc_asic = false;

	int ret;
	ret = kstrtou32(buf, 0, &cs_idx);
	if (ret || cs_idx >= CS_NUMS) {
		dev_err(dev, "invalid cs num: %d\n", cs_idx);
		return len;
	}


	lbc_dev = dev_get_drvdata(dev);
	if (IS_ERR_OR_NULL(lbc_dev)) {
		dev_err(dev, "failed to get lbc dev\n");
		return len;
	}

	printk("read from cs %d with transfer mode %d\n", cs_idx, lbc_dev->cs_cfg[cs_idx].transfer_mode);

	mutex_lock(&lbc_lock);

	if (lbc_dev->cs_cfg[cs_idx].lbc_protocol == TIME_MODE_GPMC
			&& lbc_dev->cs_cfg[cs_idx].sub_time_mode == SUB_TM_ASIC) {
		gpmc_asic = true;
		printk("use gpmc to asic\n");
	}

	if (lbc_dev->cs_cfg[cs_idx].transfer_mode == TRANSFER_MODE_DIRECT_ACCESS) {

		// 0x00
		iodl = DA_TEST_DATA_SIZE;
		lbc_da_read(lbc_dev->data_sram_addr[cs_idx], lbc_dev->da_receive_buf, iodl);
		pdata = (uint32_t *)lbc_dev->da_receive_buf;
		for (i = 0; i < (iodl / 4); i++) {
			printk("read addr: %08x, data: %08x\n", (i * 4), *pdata);
			pdata++;
		}

		for (i = 0; i < iodl; i++) {
			if (lbc_dev->da_receive_buf[i] != lbc_dev->da_send_buf[i]) {
				dev_info(dev, "check err, [%d] receive_buf = 0x%x, send_buf = 0x%x\n", \
							i, lbc_dev->da_receive_buf[i], lbc_dev->da_send_buf[i]);
				break;
			}
		}

		if (i == iodl) {
			dev_info(dev, "check da trans data ok\n");
		}

	} else if (lbc_dev->cs_cfg[cs_idx].transfer_mode == TRANSFER_MODE_INTERRUPT) {
		lbc_dev->rx_index = 0;

		// read from ram
		iodl = IDA_TEST_DATA_SIZE;
		lbc_ida_read(lbc_dev, (CSn_LADDR[cs_idx] + (0x4000)), iodl, gpmc_asic, true);
		pdata = (uint32_t *)lbc_dev->ida_receive_buf;
		for (i = 0; i < (iodl / 4); i++) {
			printk("read addr: %08x, data: %08x\n", (0x4000 + i * 4), *pdata);
			pdata++;
		}

		for (i = 0; i < iodl; i++) {
			if (lbc_dev->ida_receive_buf[i] != lbc_dev->ida_send_buf[i]) {
				dev_info(dev, "check err, [%d] receive_buf = 0x%x, send_buf = 0x%x\n", \
							i, lbc_dev->ida_receive_buf[i], lbc_dev->ida_send_buf[i]);
				break;
			}
		}

		if (i == iodl) {
			dev_info(dev, "check ida trans data ok\n");
		}


		dev_info(dev, "IDA rx\n");
	} else if (lbc_dev->cs_cfg[cs_idx].transfer_mode == TRANSFER_MODE_DMA) {
		// test 4K
		iodl = DMA_DATA_MAX;
		lbc_dma_read(lbc_dev, CSn_LADDR[cs_idx] + 0x4000, iodl, true);

		for (i = 0; i < iodl; i++) {
			if (i < 32)
				pr_info("dma_rx_buffer[%d] = 0x%x, send: 0x%x\n", i, lbc_dev->dma_rx_buffer[i], lbc_dev->dma_tx_buffer[i]);
		}
		for (i = 0; i < iodl; i++) {
			if (lbc_dev->dma_tx_buffer[i] != lbc_dev->dma_rx_buffer[i]) {
				dev_info(dev, "check err, [%d] receive_buf = 0x%x, send_buf = 0x%x\n", \
							i, lbc_dev->dma_rx_buffer[i], lbc_dev->dma_tx_buffer[i]);
				break;
			}
		}

		if (i == iodl) {
			dev_info(dev, "check dma trans data ok\n");
		}
	}

	mutex_unlock(&lbc_lock);
	return len;
}


static ssize_t lbc_config_message(struct device *dev,
					 struct device_attribute *da, char *buf)
{
	unsigned int len = 0;
	sunxi_lbc_t *lbc_dev = NULL;

	lbc_dev = dev_get_drvdata(dev);
	if (IS_ERR_OR_NULL(lbc_dev)) {
		dev_err(dev, "failed to get lbc dev\n");
		return len;
	}

	dev_info(dev, "lbc version %s\n", LBC_VERSION);
	/*
	dev_info(dev, "now config message: \n"
						"transfer_mode: %d\n"
						"transfer_width: %d\n"
						"lbc_freq: %d\n",
						lbc_dev->transfer_mode,
						lbc_dev->transfer_width,
						lbc_dev->lbc_freq);
	*/

	dev_info(dev, "[tx] desc virtual first address 0x%08lx, physical first address 0x%08lx\n",
						(long unsigned int)lbc_dev->dma_tx,
						(long unsigned int)lbc_dev->dma_tx_phy);

	dev_info(dev, "[rx] desc virtual first address 0x%08lx, physical first address 0x%08lx\n",
						(long unsigned int)lbc_dev->dma_rx,
						(long unsigned int)lbc_dev->dma_rx_phy);

	dev_info(dev, "[tx] data virtual first address 0x%08lx, physical first address 0x%08lx\n",
						(long unsigned int)lbc_dev->dma_tx_buffer,
						(long unsigned int)lbc_dev->dma_tx_buffer_phy);

	dev_info(dev, "[rx] data virtual first address 0x%08lx, physical first address 0x%08lx\n",
						(long unsigned int)lbc_dev->dma_rx_buffer,
						(long unsigned int)lbc_dev->dma_rx_buffer_phy);

	return len;
}

static void lbc_clk_init(sunxi_lbc_t *chip)
{
	int ret = 0;
	u32 rate = 0;
	struct device *dev = chip->dev;

	reset_control_assert(chip->lbc_rst);

	ret = reset_control_deassert(chip->lbc_rst);
	if (ret) {
		dev_err(dev, "lbc reset failed\n");
	}

	ret = clk_prepare_enable(chip->lbc_pll);
	if (ret) {
		dev_err(dev, "lbc pll enable failed\n");
	}

	ret = clk_set_parent(chip->lbc, chip->lbc_pll);
	if (ret) {
		dev_err(dev, "lbc pll parent set failed\n");
	}

	rate = clk_round_rate(chip->lbc, chip->lbc_freq);
	ret = clk_set_rate(chip->lbc, rate);
	if (ret) {
		dev_err(dev, "lbc clk rate set failed\n");
	}

	ret = clk_prepare_enable(chip->bus_lbc);
	if (ret) {
		dev_err(dev, "lbc bus clk enalbe failed\n");
	}

	ret = clk_prepare_enable(chip->lbc);
	if (ret) {
		dev_err(dev, "lbc clk enable failed\n");
	}

	dev_info(dev, "lbc clks init done\n");
}

static void lbc_hw_init(sunxi_lbc_t *lbc, uint32_t cs_idx)
{
	lbc_mode1_32bit_default_config_init(lbc);
#if 1
	if (lbc->cs_cfg[cs_idx].lbc_protocol == TIME_MODE_LBC0) {
		if (lbc->cs_cfg[cs_idx].transfer_width == DATA_WIDTH_8_BIT) {
			printk("lbc bus width 8\n");
		} else if (lbc->cs_cfg[cs_idx].transfer_width == DATA_WIDTH_16_BIT) {
			if (lbc->cs_cfg[cs_idx].sub_time_mode == SUB_TM_NRDD1) {
				printk("lbc sub time mode nrdd1\n");
			} else if (lbc->cs_cfg[cs_idx].sub_time_mode == SUB_TM_NRDD15) {
				printk("lbc sub time mode nrdd15\n");
			}
			printk("lbc bus width 16\n");
		} else if (lbc->cs_cfg[cs_idx].transfer_width == DATA_WIDTH_32_BIT) {
			if (lbc->cs_cfg[cs_idx].sub_time_mode == SUB_TM_NRDD1) {
				printk("lbc sub time mode nrdd1\n");
			} else if (lbc->cs_cfg[cs_idx].sub_time_mode == SUB_TM_NRDD15) {
				printk("lbc sub time mode nrdd15\n");
			}
			printk("lbc bus width 32\n");
		}
		printk("lbc mode 0\n");
	} else if (lbc->cs_cfg[cs_idx].lbc_protocol == TIME_MODE_LBC1) {
		if (lbc->cs_cfg[cs_idx].transfer_width == DATA_WIDTH_8_BIT) {

			printk("lbc bus width 8\n");
		} else if (lbc->cs_cfg[cs_idx].transfer_width == DATA_WIDTH_16_BIT) {
			if (lbc->cs_cfg[cs_idx].sub_time_mode == SUB_TM_ASIC) {
				printk("lbc bus width 16 asic\n");
			} else {
				printk("lbc bus width 16\n");
			}
		} else if (lbc->cs_cfg[cs_idx].transfer_width == DATA_WIDTH_32_BIT) {
			printk("lbc bus width 32\n");
		}
		printk("lbc mode 1\n");
	} else if (lbc->cs_cfg[cs_idx].lbc_protocol == TIME_MODE_GPMC) {
		if (lbc->cs_cfg[cs_idx].sub_time_mode == SUB_TM_ADC_7616) {
			printk("lbc gpmc adc 7616\n");
		} else if (lbc->cs_cfg[cs_idx].sub_time_mode == SUB_TM_NAND) {
			printk("lbc nand\n");
		} else if (lbc->cs_cfg[cs_idx].sub_time_mode == SUB_TM_ASIC) {
			printk("lbc gpmc 16bit asic\n");
		}
		printk("lbc mode gpmc\n");
	}
#endif
}

static ssize_t lbc_init_ctrl(struct device *dev,
					 struct device_attribute *da, const char *buf, size_t len)
{
	sunxi_lbc_t *lbc_dev;

	int ret;
	uint32_t cs_idx = 0;
	ret = kstrtou32(buf, 0, &cs_idx);
	if (ret || cs_idx >= CS_NUMS) {
		dev_err(dev, "invalid cs num: %d\n", cs_idx);
		return len;
	}

	printk("init to cs: %d\n", cs_idx);


	lbc_dev = dev_get_drvdata(dev);
	if (IS_ERR_OR_NULL(lbc_dev)) {
		dev_err(dev, "failed to get lbc dev\n");
		return len;
	}

	mutex_lock(&lbc_lock);
	//if (strncmp(buf, "init", strlen("init")) == 0) {
	//	dev_info(dev, "init lbc%d\n", lbc_dev->transfer_mode);
	//} else {
	//	dev_err(dev, "invalid input\n");
	//	return len;
	//}

	//lbc_clk_init(lbc_dev);
	lbc_hw_init(lbc_dev, cs_idx);
	calibrate_delay_chain(lbc_dev->reg_addr, 32);

	mutex_unlock(&lbc_lock);
	printk("********* lbc init ctrl version 1*********\n");
	return len;
}

/* ***************************************************************************
 * configure interface
 * **************************************************************************/

static ssize_t lbc_clk_freq_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	sunxi_lbc_t *lbc_dev = dev_get_drvdata(dev);

	return sprintf(buf, "Usage:\necho number > clk_freq"
			"\nnow clk_freq: %d\n", lbc_dev->lbc_freq);
}

static ssize_t lbc_clk_freq_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{

	sunxi_lbc_t *lbc_dev = dev_get_drvdata(dev);
	int ret;

	if (IS_ERR_OR_NULL(lbc_dev)) {
		dev_err(dev, "failed to get lbc dev\n");
		return count;
	}

	mutex_lock(&lbc_lock);
	ret = kstrtou32(buf, 0, &lbc_dev->lbc_freq);
	if (ret) {
		lbc_dev->lbc_freq = 100000000;
		dev_err(dev, "set to lbc_clk_freq fail, use default value %d\n", lbc_dev->lbc_freq);
		goto out;
	}

out:
	mutex_unlock(&lbc_lock);
	return count;
}

static ssize_t lbc_transfer_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	sunxi_lbc_t *lbc_dev = dev_get_drvdata(dev);

	return sprintf(buf, "Usage:\necho [0~3] [0~2] > transfer_mode"
			"first value: [0~3] means 0~3 cs\n"
			"second value: [0~2]\n"
			"	0: Direct access\n"
			"	1: Indirect access (interrupt)\n"
			"	2: DMA\n"
			"\nnow transfer_mode: \n"
			"	cs0: %u\n"
			"	cs1: %u\n"
			"	cs2: %u\n"
			"	cs3: %u\n"
			, lbc_dev->cs_cfg[0].transfer_mode
			, lbc_dev->cs_cfg[1].transfer_mode
			, lbc_dev->cs_cfg[2].transfer_mode
			, lbc_dev->cs_cfg[3].transfer_mode);
}

static int parse_write_str(const char *cstr, u32 *cs, u32 *val)
{
	char *ptr, *tstr, *str;
	int ret = 0;

	str = (char *)cstr;

	tstr = strim(str);

	ptr = strchr(tstr, ',');
	if (!ptr)
		return -EINVAL;

	*ptr = '\0';

	ret = kstrtou32(tstr, 0, cs);
	if (ret)
		goto out;

	ret = kstrtou32(skip_spaces(ptr + 1), 0, val);
out:
	*ptr = ',';

	printk("set cs: %u, val: %u\n", *cs, *val);
	return ret;
}

static ssize_t lbc_transfer_mode_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{

	sunxi_lbc_t *lbc_dev = dev_get_drvdata(dev);
	int ret;
	uint32_t cs = 0;
	uint32_t val = 0;

	printk("set transfer_mode\n");
	if (IS_ERR_OR_NULL(lbc_dev)) {
		dev_err(dev, "failed to get lbc dev\n");
		return count;
	}

	printk("buf: %s\n", buf);
	ret = parse_write_str(buf, &cs, &val);
	printk("buf2: %s\n", buf);

	printk("set_transfer_mode cs: %d, val: %d\n", cs, val);

	mutex_lock(&lbc_lock);
	if (ret || (cs > (CS_NUMS - 1))) {
		dev_err(dev, "set to transfer_mode fail\n");
	} else {
		lbc_dev->cs_cfg[cs].transfer_mode = val;
	}

	mutex_unlock(&lbc_lock);
	return count;
}

static ssize_t lbc_transfer_width_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	sunxi_lbc_t *lbc_dev = dev_get_drvdata(dev);

	return sprintf(buf, "Usage:\necho [0~3] [0~2] > transfer_width"
			"\nnow transfer_width: \n"
			"	cs0: %u\n"
			"	cs1: %u\n"
			"	cs2: %u\n"
			"	cs3: %u\n"
			, lbc_dev->cs_cfg[0].transfer_width
			, lbc_dev->cs_cfg[1].transfer_width
			, lbc_dev->cs_cfg[2].transfer_width
			, lbc_dev->cs_cfg[3].transfer_width);
}

static ssize_t lbc_transfer_width_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{

	sunxi_lbc_t *lbc_dev = dev_get_drvdata(dev);
	int ret;
	uint32_t cs = 0;
	uint32_t val = 0;

	if (IS_ERR_OR_NULL(lbc_dev)) {
		dev_err(dev, "failed to get lbc dev\n");
		return count;
	}

	ret = parse_write_str(buf, &cs, &val);


	mutex_lock(&lbc_lock);
	if (ret || (cs > (CS_NUMS - 1))) {
		dev_err(dev, "set to transfer_width fail");
	} else {
		lbc_dev->cs_cfg[cs].transfer_width = val;

		if (DATA_WIDTH_32_BIT == lbc_dev->cs_cfg[cs].transfer_width) {
			// do spi config

			lbc_cs_set_addr_mux_type(lbc_dev->reg_addr, cs, ADDR_MUX_AD_TYPE);
			lbc_cs_set_bus_width(lbc_dev->reg_addr, cs, DATA_WIDTH_32_BIT);
		} else if (DATA_WIDTH_16_BIT == lbc_dev->cs_cfg[cs].transfer_width) {

			lbc_cs_set_addr_mux_type(lbc_dev->reg_addr, cs, ADDR_MUX_AAD_TYPE);
			lbc_cs_set_bus_width(lbc_dev->reg_addr, cs, DATA_WIDTH_16_BIT);
		}
	}
	mutex_unlock(&lbc_lock);

	return count;
}

static ssize_t lbc_chain_delay_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	sunxi_lbc_t *lbc_dev = dev_get_drvdata(dev);

	return sprintf(buf, "Usage:\necho xxx > chain_delay"
			"\nnow chain_delay: %d\n", lbc_dev->trans_cfg.samp_dl_sw_value);
}

static ssize_t lbc_chain_delay_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	sunxi_lbc_t *lbc_dev = dev_get_drvdata(dev);
	int ret;

	if (IS_ERR_OR_NULL(lbc_dev)) {
		dev_err(dev, "failed to get lbc dev\n");
		return count;
	}

	mutex_lock(&lbc_lock);
	ret = kstrtou32(buf, 0, &lbc_dev->trans_cfg.samp_dl_sw_value);
	if (ret) {
		lbc_dev->trans_cfg.samp_dl_sw_value = 40;
		dev_err(dev, "set to chain_delay fail, use default value %d\n", \
				lbc_dev->trans_cfg.samp_dl_sw_value);
	}
	mutex_unlock(&lbc_lock);
	return count;
}

static ssize_t lbc_burst_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	sunxi_lbc_t *lbc_dev = dev_get_drvdata(dev);

	return sprintf(buf, "Usage:\necho [0~7] > burst_mode"
			"0: SINGLE\n"
			"1: inc BEAT-2\n"
			"2: fix BEAT-4\n"
			"3: inc BEAT-4\n"
			"4: fix BEAT-16\n"
			"5: inc BEAT-16\n"
			"6: fix BEAT-128\n"
			"7: inc BEAT-128\n"
			"\nnow burst_mode: %d\n", lbc_dev->burst_mode);
}

static ssize_t lbc_burst_mode_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	sunxi_lbc_t *lbc_dev = dev_get_drvdata(dev);
	int ret;

	if (IS_ERR_OR_NULL(lbc_dev)) {
		dev_err(dev, "failed to get lbc dev\n");
		return count;
	}

	mutex_lock(&lbc_lock);
	ret = kstrtou32(buf, 0, &lbc_dev->burst_mode);
	if (ret) {
		lbc_dev->burst_mode = 0;
		dev_err(dev, "set to burst_mode fail, use default value %d\n", \
				lbc_dev->burst_mode);
	}

	mutex_unlock(&lbc_lock);
	return count;
}

//	0: lbc mode 0
//	1: lbc mode 1 (default)
//	2: gpmc
static ssize_t lbc_protocol_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	sunxi_lbc_t	*lbc_dev = dev_get_drvdata(dev);

	return sprintf(buf, "Usage:\necho [0~2] [0~4] > lbc_protocol\n"
			"0: lbc mode 0\n"
			"1: lbc mode 1 (default)\n"
			"2: gpmc nor\n"
			"\nnow protocol: \n"
			"	cs0: %u\n"
			"	cs1: %u\n"
			"	cs2: %u\n"
			"	cs3: %u\n"
			, lbc_dev->cs_cfg[0].lbc_protocol
			, lbc_dev->cs_cfg[1].lbc_protocol
			, lbc_dev->cs_cfg[2].lbc_protocol
			, lbc_dev->cs_cfg[3].lbc_protocol);
}

static ssize_t lbc_protocol_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	sunxi_lbc_t *lbc_dev = dev_get_drvdata(dev);
	int ret;
	uint32_t cs = 0;
	uint32_t val = 0;

	if (IS_ERR_OR_NULL(lbc_dev)) {
		dev_err(dev, "failed to get lbc dev\n");
		return count;
	}

	ret = parse_write_str(buf, &cs, &val);

	mutex_lock(&lbc_lock);
	if (ret) {
		dev_err(dev, "set to protocol fail\n");
	} else {
		lbc_dev->cs_cfg[cs].lbc_protocol = val;
	}
	mutex_unlock(&lbc_lock);
	return count;
}

// addr mux type
//	0: non mux
//	1: ad mux
//	2: aad mux
static ssize_t lbc_addr_mux_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	sunxi_lbc_t	*lbc_dev = dev_get_drvdata(dev);

	return sprintf(buf, "Usage:\necho [0~2] [0~4] > addr_mux\n"
			"0: non mux\n"
			"1: ad mux\n"
			"2: aad mux\n"
			"\nnow addr mux type: \n"
			"	cs0: %u\n"
			"	cs1: %u\n"
			"	cs2: %u\n"
			"	cs3: %u\n"
			, lbc_dev->cs_cfg[0].addr_mux_type
			, lbc_dev->cs_cfg[1].addr_mux_type
			, lbc_dev->cs_cfg[2].addr_mux_type
			, lbc_dev->cs_cfg[3].addr_mux_type);
}

static ssize_t lbc_addr_mux_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	sunxi_lbc_t *lbc_dev = dev_get_drvdata(dev);
	int ret;
	uint32_t cs = 0;
	uint32_t val = 0;

	if (IS_ERR_OR_NULL(lbc_dev)) {
		dev_err(dev, "failed to get lbc dev\n");
		return count;
	}

	ret = parse_write_str(buf, &cs, &val);

	mutex_lock(&lbc_lock);
	if (ret) {
		dev_err(dev, "set to protocol fail\n");
	} else {
		lbc_dev->cs_cfg[cs].addr_mux_type = val;
		configure_cs_time(&lbc_dev->cs_cfg[cs]);
	}
	mutex_unlock(&lbc_lock);
	return count;
}

// 0: one DP per byte
// 1: one DP only
static ssize_t lbc_dp_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	sunxi_lbc_t	*lbc_dev = dev_get_drvdata(dev);

	return sprintf(buf, "Usage:\necho [0~3] [0~1] > data_parity"
			"0: one DP per byte\n"
			"1: one DP only\n"
			"\nnow data parity: \n"
			"	cs0: %u\n"
			"	cs1: %u\n"
			"	cs2: %u\n"
			"	cs3: %u\n"
			, lbc_dev->cs_cfg[0].dp_mode
			, lbc_dev->cs_cfg[1].dp_mode
			, lbc_dev->cs_cfg[2].dp_mode
			, lbc_dev->cs_cfg[3].dp_mode);
}

static ssize_t lbc_dp_mode_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	sunxi_lbc_t *lbc_dev = dev_get_drvdata(dev);
	int ret;
	uint32_t cs = 0;
	uint32_t val = 0;

	if (IS_ERR_OR_NULL(lbc_dev)) {
		dev_err(dev, "failed to get lbc dev\n");
		return count;
	}

	ret = parse_write_str(buf, &cs, &val);

	mutex_lock(&lbc_lock);
	if (ret) {
		dev_err(dev, "set to data_parity fail\n");
		goto out;
	} else {
		lbc_dev->cs_cfg[cs].dp_mode = val;
	}

	lbc_cs_set_dp_mode(lbc_dev->reg_addr, cs, lbc_dev->cs_cfg[cs].dp_mode);

out:
	mutex_unlock(&lbc_lock);
	return count;
}

// 0: odd parity
// 1: even parity
static ssize_t lbc_dp_parity_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	sunxi_lbc_t	*lbc_dev = dev_get_drvdata(dev);

	return sprintf(buf, "Usage:\necho [0~3] [0~1] > data_parity"
			"0:odd parity\n"
			"1:even parity\n"
			"\nnow data parity: \n"
			"	cs0: %u\n"
			"	cs1: %u\n"
			"	cs2: %u\n"
			"	cs3: %u\n"
			, lbc_dev->cs_cfg[0].dp_parity
			, lbc_dev->cs_cfg[1].dp_parity
			, lbc_dev->cs_cfg[2].dp_parity
			, lbc_dev->cs_cfg[3].dp_parity);
}

static ssize_t lbc_dp_parity_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	sunxi_lbc_t *lbc_dev = dev_get_drvdata(dev);
	int ret;
	uint32_t cs = 0;
	uint32_t val = 0;

	if (IS_ERR_OR_NULL(lbc_dev)) {
		dev_err(dev, "failed to get lbc dev\n");
		return count;
	}

	ret = parse_write_str(buf, &cs, &val);

	mutex_lock(&lbc_lock);
	if (ret) {
		dev_err(dev, "set to dp_parity fail\n");
		goto out;
	} else {
			lbc_dev->cs_cfg[cs].dp_parity = val;
	}

	lbc_cs_set_dp_parity(lbc_dev->reg_addr, cs, lbc_dev->cs_cfg[cs].dp_parity);

out:
	mutex_unlock(&lbc_lock);
	return count;
}

// 0: dp disable
// 1: dp enable
static ssize_t lbc_dp_en_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	sunxi_lbc_t	*lbc_dev = dev_get_drvdata(dev);

	return sprintf(buf, "Usage:\necho [0~3] [0~1] > dp_en"
			"0: disable DP\n"
			"1: enable DP\n"
			"\nnow dp en: \n"
			"	cs0: %u\n"
			"	cs1: %u\n"
			"	cs2: %u\n"
			"	cs3: %u\n"
			, lbc_dev->cs_cfg[0].dp_en
			, lbc_dev->cs_cfg[1].dp_en
			, lbc_dev->cs_cfg[2].dp_en
			, lbc_dev->cs_cfg[3].dp_en);
}

static ssize_t lbc_dp_en_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	sunxi_lbc_t *lbc_dev = dev_get_drvdata(dev);
	int ret;
	uint32_t cs = 0;
	uint32_t val = 0;

	if (IS_ERR_OR_NULL(lbc_dev)) {
		dev_err(dev, "failed to get lbc dev\n");
		return count;
	}

	ret = parse_write_str(buf, &cs, &val);

	mutex_lock(&lbc_lock);
	if (ret) {
		dev_err(dev, "set to dp end fail\n");
		goto out;
	} else {
			lbc_dev->cs_cfg[cs].dp_en = val;
	}

	lbc_cs_set_dp_en(lbc_dev->reg_addr, cs, lbc_dev->cs_cfg[cs].dp_en);

out:
	mutex_unlock(&lbc_lock);
	return count;
}



// 0: little endian (default)
// 1: bit endian
static ssize_t lbc_bus_endian_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	sunxi_lbc_t	*lbc_dev = dev_get_drvdata(dev);

	return sprintf(buf, "Usage:\necho [0~3] [0~1] > bus_endian"
			"0: big_endian\n"
			"1: little_endian\n"
			"\nnow endian: \n"
			"	cs0: %u\n"
			"	cs1: %u\n"
			"	cs2: %u\n"
			"	cs3: %u\n"
			, lbc_dev->cs_cfg[0].bus_endian
			, lbc_dev->cs_cfg[1].bus_endian
			, lbc_dev->cs_cfg[2].bus_endian
			, lbc_dev->cs_cfg[3].bus_endian);
}

static ssize_t lbc_bus_endian_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	sunxi_lbc_t *lbc_dev = dev_get_drvdata(dev);
	int ret;
	uint32_t cs = 0;
	uint32_t val = 0;

	if (IS_ERR_OR_NULL(lbc_dev)) {
		dev_err(dev, "failed to get lbc dev\n");
		return count;
	}

	ret = parse_write_str(buf, &cs, &val);


	mutex_lock(&lbc_lock);
	if (ret) {
		dev_err(dev, "set to bus_endian fail\n");
		goto out;
	} else {
		lbc_dev->cs_cfg[cs].bus_endian = val;
	}

	if (lbc_dev->cs_cfg[cs].bus_endian == BIG_BUS_ENDIAN) {
		lbc_cs_set_bus_endian(lbc_dev->reg_addr, 0, BIG_BUS_ENDIAN);
	} else {
		lbc_cs_set_bus_endian(lbc_dev->reg_addr, 0, LITTLE_BUS_ENDIAN);
	}
out:
	mutex_unlock(&lbc_lock);
	return count;
}

static void readl_print(void __iomem *addr, uint32_t reg_offset, char *name)
{
	uint32_t val = readl(addr + reg_offset);
	//printk("read addr: [%08lx], %08x, reg name: %s\n", (LBC_REG_BASE + reg_offset), val, name);
	printk("%08x, %08x\n", (LBC_REG_BASE + reg_offset), val);
}


// da test 0~2048, 0~7FF
static int lbc_do_da_test(void *parg)
{
	unsigned int axi_write_addr = 0;
	unsigned int axi_read_addr = 0;
	uint32_t loop_counts = 0;
	uint32_t failed_times = 0;
	struct device *dev = NULL;
	//uint32_t write_fifo_cnt = 0;
	int ret = 0;
	uint32_t cs_idx = 0;
	uint32_t iodl = 0;
	int i = 0, j = 0;

	sunxi_lbc_t *lbc_dev = (sunxi_lbc_t *)parg;
	if (IS_ERR_OR_NULL(lbc_dev)) {
		printk("failed to get lbc dev to do da test\n");
		return -1;
	}

	cs_idx = lbc_dev->current_cs_idx;
	dev = lbc_dev->dev;
	if (IS_ERR_OR_NULL(dev)) {
		printk("failed to get dev to do da test\n");
		return -1;
	}

	printk("********** da test start\n");
	while (1) {
		ret = kthread_should_stop();
		if (ret) {
			dev_info(dev, "da test thread is stop!!!\n");
			break;
		}

		axi_write_addr = 0;
		axi_read_addr = 0;

		loop_counts++;
		// 0x0000 ~ 0xfff 4K
		for (j = 0; j < (128 / DA_TEST_DATA_SIZE); j++) {
			for (i = 0; i < DA_TEST_DATA_SIZE; i++) {
				lbc_dev->da_receive_buf[i] = get_random_int() % 256;
				lbc_dev->da_send_buf[i] = get_random_int() % 256;
			}

			// do write
			iodl = DA_TEST_DATA_SIZE;
			lbc_da_write((lbc_dev->data_test_addr[cs_idx] + (iodl * j)), lbc_dev->da_send_buf, iodl);

			// do read
			lbc_da_read((lbc_dev->data_test_addr[cs_idx] + (iodl * j)), lbc_dev->da_receive_buf, iodl);

			//do data_check
			for (i = 0; i < iodl; i++) {
				if (lbc_dev->da_receive_buf[i] != lbc_dev->da_send_buf[i]) {
					dev_info(dev, "da check err, [%d] receive_buf = 0x%x, send_buf = 0x%x\n", \
						i, lbc_dev->da_receive_buf[i], lbc_dev->da_send_buf[i]);
					failed_times++;
					break;
				}
			}
		}
		if (loop_counts % 10 == 0) {
			//dev_info(dev, "do da test loop: %x, failed_times: %x\n", loop_counts, failed_times);
			dev_info(dev, "****** do  da test loops: %8u, failed_times: %8u\n", loop_counts, failed_times);
		}
	}
	return 0;
}

static int lbc_do_dma_test(void *parg)
{
	uint32_t loop_counts = 0;
	uint32_t failed_times = 0;
	int i = 0;
	struct device *dev = NULL;
	int ret = 0;
	uint32_t cs_idx = 0;
	int iodl = 0;

	sunxi_lbc_t *lbc_dev = (sunxi_lbc_t *)parg;
	if (IS_ERR_OR_NULL(lbc_dev)) {
		printk("failed to get lbc dev to do da test\n");
		return -1;
	}

	cs_idx = lbc_dev->current_cs_idx;
	dev = lbc_dev->dev;
	if (IS_ERR_OR_NULL(dev)) {
		printk("failed to get dev to do da test\n");
		return -1;
	}

	printk("test cs: %x\n", cs_idx);
#if 1
	printk("********** dma test start\n");
	while (1) {
		ret = kthread_should_stop();
		if (ret) {
			dev_info(dev, "dma test thread is stop!!!\n");
			break;
		}

		if (loop_counts % 5 == 0) {
			dev_info(dev, "------ do dma test loops: %8u, failed_times: %8u\n", loop_counts, failed_times);
		}

		loop_counts++;

		iodl = DMA_DATA_MAX;
		for (i = 0; i < iodl; i++) {
			lbc_dev->dma_rx_buffer[i] = get_random_int() % 256;
			lbc_dev->dma_tx_buffer[i] = get_random_int() % 256;
		}


		// 1. do dma tx
		lbc_dma_write(lbc_dev, (CSn_LADDR[cs_idx] + 0x2000), iodl, false);

		//udelay(100);
		// 2. do dma rx
		lbc_dma_read(lbc_dev, (CSn_LADDR[cs_idx] + 0x2000), iodl, false);

		if (dma_wait_time > 0)
			udelay(dma_wait_time);

		for (i = 0; i < 128; i++) {
			//pr_info("dma_rx_buffer[%d] = 0x%x, send: 0x%x\n", i, lbc_dev->dma_rx_buffer[i], lbc_dev->dma_tx_buffer[i]);
		}
		for (i = 0; i < iodl; i++) {
			if (lbc_dev->dma_tx_buffer[i] != lbc_dev->dma_rx_buffer[i]) {
				dev_info(dev, "dma check err, [%d] receive_buf = 0x%x, send_buf = 0x%x\n", \
					i, lbc_dev->dma_rx_buffer[i], lbc_dev->dma_tx_buffer[i]);
				failed_times++;
				break;
			}
		}
		//mdelay(5000);

	}
#endif
	return 0;
}


static int lbc_do_ida_test(void *parg)
{
	uint32_t loop_counts = 0;
	uint32_t failed_times = 0;
	int i = 0, j = 0;
	//uint64_t timeout = 0;
	struct device *dev = NULL;
	int ret = 0;
	uint32_t cs_idx = 0;
	uint32_t iodl = 0;

	sunxi_lbc_t *lbc_dev = (sunxi_lbc_t *)parg;
	if (IS_ERR_OR_NULL(lbc_dev)) {
		printk("failed to get lbc dev to do da test\n");
		return -1;
	}

	cs_idx = lbc_dev->current_cs_idx;
	dev = lbc_dev->dev;
	if (IS_ERR_OR_NULL(dev)) {
		printk("failed to get dev to do da test\n");
		return -1;
	}

	printk("********** ida test start\n");
	while (1) {
		ret = kthread_should_stop();
		if (ret) {
			dev_info(dev, "ida test thread is stop!!!\n");
			break;
		}

		loop_counts++;

		// test 0x1000 ~ 0x1fff;
		iodl = IDA_TEST_DATA_SIZE;
		for (j = 0; j < (4096 / iodl); j++) {
			for (i = 0; i < IDA_TEST_DATA_SIZE; i++) {
				lbc_dev->ida_receive_buf[i] = get_random_int() % 256;
				lbc_dev->ida_send_buf[i] = get_random_int() %256;
			}

			// do lbc ida write
			//mdelay(10);
			lbc_ida_write(lbc_dev, (CSn_LADDR[cs_idx] + 0x1000 + (iodl * j)), lbc_dev->ida_send_buf, iodl, false, false);
			udelay(dma_wait_time);
			// do lbc ida read

			//mdelay(10);
			lbc_ida_read(lbc_dev, (CSn_LADDR[cs_idx] + 0x1000 + (iodl * j)), iodl, false, false);

			//mdelay(10);
			for (i = 0; i < iodl; i++) {
				lbc_log(LOG_DEBUG, "receive_buf: %x, send: %x\n", lbc_dev->ida_receive_buf[i], lbc_dev->ida_send_buf[i]);
				if (lbc_dev->ida_receive_buf[i] != lbc_dev->ida_send_buf[i]) {
					dev_info(dev, "ida check err, [%d] receive_buf = 0x%x, send_buf = 0x%x\n", \
							i, lbc_dev->ida_receive_buf[i], lbc_dev->ida_send_buf[i]);
					failed_times++;
					break;
				}
			}
		}

		if (loop_counts % 2 == 0) {
			dev_info(dev, "****** do ida test loops: %8u, failed_times: %8u\n", loop_counts, failed_times);
		}
	}

	return 0;
}


// do da-dma concurrency test
static ssize_t lbc_da_ida_dma_con_test_start(struct device *dev,
					 struct device_attribute *da, const char *buf, size_t len)
{
	int ret = 0;
	uint32_t cs = 0;
	uint32_t mode = 0;

	sunxi_lbc_t *lbc_dev = dev_get_drvdata(dev);
	if (IS_ERR_OR_NULL(lbc_dev)) {
		dev_err(dev, "failed to get lbc dev\n");
		return len;
	}

	printk("buf: %s\n", buf);
	ret = parse_write_str(buf, &cs, &mode);
	printk("buf2: %s\n", buf);

	printk("set cs: %d, mode: %d\n", cs, mode);

	if (ret || (cs > (CS_NUMS - 1))) {
		dev_err(dev, "set to cs mode fail\n");
	} else {
		lbc_dev->current_cs_idx = cs;
	}

	ret = kstrtou32(buf, 0, &mode);

	printk("test mode: %d, (0: all , 1: da, 2: ida, 3: dma)\n", mode);

	if (mode == 0 || mode == 1) {
		lbc_dev->da_test_task = kthread_create(lbc_do_da_test, (void *)lbc_dev, "lbc da test");
		if (!lbc_dev->da_test_task) {
			dev_err(dev, "init lbc DA test task failed\n");
			lbc_dev->da_test_task = NULL;
			return len;
		}
		wake_up_process(lbc_dev->da_test_task);
	}

	if (mode == 0 || mode == 2) {
		lbc_dev->ida_test_task = kthread_create(lbc_do_ida_test, (void *)lbc_dev, "lbc ida test");
		if (!lbc_dev->ida_test_task) {
			dev_err(dev, "init lbc IDA test task failed\n");
			lbc_dev->ida_test_task = NULL;
			return len;
		}
		wake_up_process(lbc_dev->ida_test_task);
	}

	if (mode == 0 || mode == 3) {
		lbc_dev->dma_test_task = kthread_create(lbc_do_dma_test, (void *)lbc_dev, "lbc dma test");
		if (!lbc_dev->dma_test_task) {
			dev_err(dev, "init lbc DMA test task failed\n");
			lbc_dev->dma_test_task = NULL;
			return len;
		}
		wake_up_process(lbc_dev->dma_test_task);
	}
	return len;
}

static ssize_t lbc_da_ida_dma_con_test_stop(struct device *dev,
					 struct device_attribute *da, const char *buf, size_t len)
{
	sunxi_lbc_t *lbc_dev = dev_get_drvdata(dev);
	if (IS_ERR_OR_NULL(lbc_dev)) {
		dev_err(dev, "failed to get lbc dev\n");
		return len;
	}

	printk("going to stop test task\n");

	if (lbc_dev->da_test_task) {
		dev_info(dev, "going to stop da test thread......\n");
		kthread_stop(lbc_dev->da_test_task);
		lbc_dev->da_test_task = NULL;
		dev_info(dev, "stop da test thread\n");
	}

	if (lbc_dev->ida_test_task) {
		dev_info(dev, "going to stop ida test thread......\n");
		kthread_stop(lbc_dev->ida_test_task);
		lbc_dev->ida_test_task = NULL;
		dev_info(dev, "stop ida test thread\n");
	}

	if (lbc_dev->dma_test_task) {
		dev_info(dev, "going to stop dma test thread...... \n");
		kthread_stop(lbc_dev->dma_test_task);
		lbc_dev->dma_test_task = NULL;
		dev_info(dev, "stop dma test thread\n");
	}

	return len;
}



static ssize_t lbc_dump_registers_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	sunxi_lbc_t *lbc_dev = dev_get_drvdata(dev);

	readl_print(lbc_dev->reg_addr, CH_MODE_CTRL_REG, "CH_MODE_CTRL_REG");
	readl_print(lbc_dev->reg_addr, CS0_DA_HADDR_REG, "CS0_DA_HADDR_REG");
	readl_print(lbc_dev->reg_addr, CS0_DA_LADDR_REG, "CS0_DA_LADDR_REG");
	readl_print(lbc_dev->reg_addr, CS0_IDA_HADDR_REG, "CS0_IDA_HADDR_REG");
	readl_print(lbc_dev->reg_addr, CS0_IDA_LADDR_REG, "CS0_IDA_LADDR_REG");
	readl_print(lbc_dev->reg_addr, CS1_DA_HADDR_REG, "CS1_DA_HADDR_REG");
	readl_print(lbc_dev->reg_addr, CS1_DA_LADDR_REG, "CS1_DA_LADDR_REG");
	readl_print(lbc_dev->reg_addr, CS1_IDA_HADDR_REG, "CS1_IDA_HADDR_REG");
	readl_print(lbc_dev->reg_addr, CS1_IDA_LADDR_REG, "CS1_IDA_LADDR_REG");
	readl_print(lbc_dev->reg_addr, CS2_DA_HADDR_REG, "CS2_DA_HADDR_REG");
	readl_print(lbc_dev->reg_addr, CS2_DA_LADDR_REG, "CS2_DA_LADDR_REG");
	readl_print(lbc_dev->reg_addr, CS2_IDA_HADDR_REG, "CS2_IDA_HADDR_REG");
	readl_print(lbc_dev->reg_addr, CS2_IDA_LADDR_REG, "CS2_IDA_LADDR_REG");
	readl_print(lbc_dev->reg_addr, CS3_DA_HADDR_REG, "CS3_DA_HADDR_REG");
	readl_print(lbc_dev->reg_addr, CS3_DA_LADDR_REG, "CS3_DA_LADDR_REG");
	readl_print(lbc_dev->reg_addr, CS3_IDA_HADDR_REG, "CS3_IDA_HADDR_REG");
	readl_print(lbc_dev->reg_addr, CS3_IDA_LADDR_REG, "CS3_IDA_LADDR_REG");

	readl_print(lbc_dev->reg_addr, FIX_CS_REG, "FIX_CS_REG");
	readl_print(lbc_dev->reg_addr, USER_CS_REG, "USER_CS_REG");
	readl_print(lbc_dev->reg_addr, LBC_CLK_REG, "LBC_CLK_REG");

	readl_print(lbc_dev->reg_addr, LBC_SOFT_RST_REG, "LBC_SOFT_RST_REG");
	readl_print(lbc_dev->reg_addr, SMP_DELAY_CTRL_REG, "SMP_DELAY_CTRL_REG");

	readl_print(lbc_dev->reg_addr, IDA_TS_CTRL_REG, "IDA_TS_CTRL_REG");
	readl_print(lbc_dev->reg_addr, IDA_TS_ADDR_REG, "IDA_TS_ADDR_REG");
	readl_print(lbc_dev->reg_addr, IDA_TS_DATA_LEN_REG, "IDA_TS_DATA_LEN_REG");
	readl_print(lbc_dev->reg_addr, IDA_WDATA_REG, "IDA_WDATA_REG");
	readl_print(lbc_dev->reg_addr, IDA_RDATA_REG, "IDA_RDATA_REG");
	readl_print(lbc_dev->reg_addr, DA_BST_SEL_REG, "DA_BST_SEL_REG");
	readl_print(lbc_dev->reg_addr, DA_BST_TYPE_REG, "DA_BST_TYPE_REG");
	readl_print(lbc_dev->reg_addr, DA_CS0_SADDR_REG, "DA_CS0_SADDR_REG");
	readl_print(lbc_dev->reg_addr, DA_CS1_SADDR_REG, "DA_CS1_SADDR_REG");
	readl_print(lbc_dev->reg_addr, DA_CS2_SADDR_REG, "DA_CS2_SADDR_REG");
	readl_print(lbc_dev->reg_addr, DA_CS3_SADDR_REG, "DA_CS3_SADDR_REG");
	readl_print(lbc_dev->reg_addr, DA_TX_TO_FIX_DONE_REG, "DA_TX_TO_FIX_DONE_REG");
	readl_print(lbc_dev->reg_addr, LBC_TIMEOUT_CLR_REG, "LBC_TIMEOUT_CLR_REG");

	readl_print(lbc_dev->reg_addr, LBC_ARB_PRI_CFG_REG, "LBC_ARB_PRI_CFG_REG");

	readl_print(lbc_dev->reg_addr, DMA_MODE_CFG_REG, "DMA_MODE_CFG_REG");
	readl_print(lbc_dev->reg_addr, DMA_DES_ADDR_REG, "DMA_DES_ADDR_REG");
	readl_print(lbc_dev->reg_addr, DMA_DES0_DBG_REG, "DMA_DES0_DBG_REG");
	readl_print(lbc_dev->reg_addr, DMA_DES1_DBG_REG, "DMA_DES1_DBG_REG");
	readl_print(lbc_dev->reg_addr, DMA_DES2_DBG_REG, "DMA_DES2_DBG_REG");
	readl_print(lbc_dev->reg_addr, DMA_DES3_DBG_REG, "DMA_DES3_DBG_REG");
	readl_print(lbc_dev->reg_addr, LBC_INT0_EN_REG, "LBC_INT0_EN_REG");
	readl_print(lbc_dev->reg_addr, LBC_INT1_EN_REG, "LBC_INT1_EN_REG");
	readl_print(lbc_dev->reg_addr, LBC_INT0_PENDING_REG, "LBC_INT0_PENDING_REG");
	readl_print(lbc_dev->reg_addr, LBC_INT1_PENDING_REG, "LBC_INT1_PENDING_REG");
	readl_print(lbc_dev->reg_addr, DA_INTF_STA_REG, "DA_INTF_STA_REG");
	readl_print(lbc_dev->reg_addr, DA_FIFO_STA_REG, "DA_FIFO_STA_REG");
	readl_print(lbc_dev->reg_addr, IDA_FIFO_STA_REG, "IDA_FIFO_STA_REG");
	readl_print(lbc_dev->reg_addr, DMA_FIFO_STA_REG, "DMA_FIFO_STA_REG");
	readl_print(lbc_dev->reg_addr, DA_FIFO_TRIGGER_LVL_REG, "DA_FIFO_TRIGGER_LVL_REG");
	readl_print(lbc_dev->reg_addr, IDA_FIFO_TRIGGER_LVL_REG, "IDA_FIFO_TRIGGER_LVL_REG");
	readl_print(lbc_dev->reg_addr, DMA_FIFO_TRIGGER_LVL_REG, "DMA_FIFO_TRIGGER_LVL_REG");
	readl_print(lbc_dev->reg_addr, LBC_FIFO_CLR_REG, "LBC_FIFO_CLR_REG");
	readl_print(lbc_dev->reg_addr, LBC_IDA_MIRROR_REG, "LBC_IDA_MIRROR_REG");
	readl_print(lbc_dev->reg_addr, LBC_DA_CMD0_REG, "LBC_DA_CMD0_REG");
	readl_print(lbc_dev->reg_addr, LBC_DA_CMD1_REG, "LBC_DA_CMD1_REG");
	readl_print(lbc_dev->reg_addr, LBC_CMD_BUSY_REG, "LBC_CMD_BUSY_REG");
	readl_print(lbc_dev->reg_addr, LBC_CMD0_REG, "LBC_CMD0_REG");
	readl_print(lbc_dev->reg_addr, LBC_CMD1_REG, "LBC_CMD1_REG");
	readl_print(lbc_dev->reg_addr, LBC_CMD2_REG, "LBC_CMD2_REG");
	readl_print(lbc_dev->reg_addr, LBC_DA_TO_CMD0_REG, "LBC_DA_TO_CMD0_REG");
	readl_print(lbc_dev->reg_addr, LBC_DA_TO_CMD1_REG, "LBC_DA_TO_CMD1_REG");
	readl_print(lbc_dev->reg_addr, LBC_IDA_TO_CMD0_REG, "LBC_IDA_TO_CMD0_REG");
	readl_print(lbc_dev->reg_addr, LBC_IDA_TO_CMD1_REG, "LBC_IDA_TO_CMD1_REG");
	readl_print(lbc_dev->reg_addr, LBC_DMA_TO_CMD0_REG, "LBC_DMA_TO_CMD0_REG");
	readl_print(lbc_dev->reg_addr, LBC_DMA_TO_CMD1_REG, "LBC_DMA_TO_CMD1_REG");

	//-----------------CS0----------------------
	printk("--------------------CS0--------------------\n");
	readl_print(lbc_dev->reg_addr, LBC_MODE_CTRL_REG, "LBC_MODE_CTRL_REG");
	readl_print(lbc_dev->reg_addr, LBC_TIMESCALE_CTRL_REG, "LBC_TIMESCALE_CTRL_REG");
	readl_print(lbc_dev->reg_addr, LBC_READY_CTRL_REG, "LBC_READY_CTRL_REG");
	readl_print(lbc_dev->reg_addr, LBC_DP_CTRL_REG, "LBC_DP_CTRL_REG");
	readl_print(lbc_dev->reg_addr, LBC_BE_CTRL_REG, "LBC_BE_CTRL_REG");
	readl_print(lbc_dev->reg_addr, LBC_WAIT_CTRL_REG, "LBC_WAIT_CTRL_REG");
	readl_print(lbc_dev->reg_addr, LBC_CS_TIMING_CTRL_REG, "LBC_CS_TIMING_CTRL_REG");
	readl_print(lbc_dev->reg_addr, LBC_ALE_TIMING0_CTRL_REG, "LBC_ALE_TIMING0_CTRL_REG");
	readl_print(lbc_dev->reg_addr, LBC_ALE_TIMING1_CTRL_REG, "LBC_ALE_TIMING1_CTRL_REG");
	readl_print(lbc_dev->reg_addr, LBC_WE_TIMING_CTRL_REG, "LBC_WE_TIMING_CTRL_REG");
	readl_print(lbc_dev->reg_addr, LBC_OE_TIMING0_CTRL_REG, "LBC_OE_TIMING0_CTRL_REG");
	readl_print(lbc_dev->reg_addr, LBC_OE_TIMING1_CTRL_REG, "LBC_OE_TIMING1_CTRL_REG");
	readl_print(lbc_dev->reg_addr, LBC_OE_TIMING2_CTRL_REG, "LBC_OE_TIMING2_CTRL_REG");
	readl_print(lbc_dev->reg_addr, LBC_CYCLE_TIMING_CTRL_REG, "LBC_CYCLE_TIMING_CTRL_REG");
	readl_print(lbc_dev->reg_addr, LBC_ACCESS_TIMING_CTRL_REG, "LBC_ACCESS_TIMING_CTRL_REG");
	readl_print(lbc_dev->reg_addr, LBC_PAGE_TIMING_CTRL_REG, "LBC_PAGE_TIMING_CTRL_REG");
	readl_print(lbc_dev->reg_addr, LBC_DATAMUX_TIMING_CTRL_REG, "LBC_DATAMUX_TIMING_CTRL_REG");
	readl_print(lbc_dev->reg_addr, LBC_CLE_TIMING_CTRL_REG, "LBC_CLE_TIMING_CTRL_REG");
	readl_print(lbc_dev->reg_addr, LBC_SPECIAL_TIMING0_CTRL_REG, "LBC_SPECIAL_TIMING0_CTRL_REG");
	readl_print(lbc_dev->reg_addr, LBC_SPECIAL_TIMING1_CTRL_REG, "LBC_SPECIAL_TIMING1_CTRL_REG");
	readl_print(lbc_dev->reg_addr, LBC_SPECIAL_TIMING2_CTRL_REG, "LBC_SPECIAL_TIMING2_CTRL_REG");
	readl_print(lbc_dev->reg_addr, LBC_TURNAROUND_CTRL_REG, "LBC_TURNAROUND_CTRL_REG");
	readl_print(lbc_dev->reg_addr, LBC_WAIT_DATA_NUM_REG, "LBC_WAIT_DATA_NUM_REG");

	//-----------------CS1----------------------
	printk("--------------------CS1--------------------\n");
	readl_print(lbc_dev->reg_addr, (LBC_MODE_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 1), "LBC_MODE_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_TIMESCALE_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 1), "LBC_TIMESCALE_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_READY_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 1), "LBC_READY_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_DP_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 1), "LBC_DP_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_BE_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 1), "LBC_BE_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_WAIT_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 1), "LBC_WAIT_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_CS_TIMING_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 1), "LBC_CS_TIMING_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_ALE_TIMING0_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 1), "LBC_ALE_TIMING0_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_ALE_TIMING1_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 1), "LBC_ALE_TIMING1_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_WE_TIMING_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 1), "LBC_WE_TIMING_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_OE_TIMING0_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 1), "LBC_OE_TIMING0_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_OE_TIMING1_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 1), "LBC_OE_TIMING1_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_OE_TIMING2_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 1), "LBC_OE_TIMING2_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_CYCLE_TIMING_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 1), "LBC_CYCLE_TIMING_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_ACCESS_TIMING_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 1), "LBC_ACCESS_TIMING_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_PAGE_TIMING_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 1), "LBC_PAGE_TIMING_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_DATAMUX_TIMING_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 1), "LBC_DATAMUX_TIMING_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_CLE_TIMING_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 1), "LBC_CLE_TIMING_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_SPECIAL_TIMING0_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 1), "LBC_SPECIAL_TIMING0_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_SPECIAL_TIMING1_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 1), "LBC_SPECIAL_TIMING1_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_SPECIAL_TIMING2_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 1), "LBC_SPECIAL_TIMING2_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_TURNAROUND_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 1), "LBC_TURNAROUND_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_WAIT_DATA_NUM_REG + LBC_CS_TIMING_REG_OFFSET * 1), "LBC_WAIT_DATA_NUM_REG");


	//-----------------CS2----------------------
	printk("--------------------CS2--------------------\n");
	readl_print(lbc_dev->reg_addr, (LBC_MODE_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 2), "LBC_MODE_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_TIMESCALE_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 2), "LBC_TIMESCALE_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_READY_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 2), "LBC_READY_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_DP_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 2), "LBC_DP_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_BE_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 2), "LBC_BE_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_WAIT_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 2), "LBC_WAIT_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_CS_TIMING_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 2), "LBC_CS_TIMING_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_ALE_TIMING0_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 2), "LBC_ALE_TIMING0_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_ALE_TIMING1_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 2), "LBC_ALE_TIMING1_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_WE_TIMING_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 2), "LBC_WE_TIMING_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_OE_TIMING0_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 2), "LBC_OE_TIMING0_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_OE_TIMING1_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 2), "LBC_OE_TIMING1_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_OE_TIMING2_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 2), "LBC_OE_TIMING2_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_CYCLE_TIMING_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 2), "LBC_CYCLE_TIMING_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_ACCESS_TIMING_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 2), "LBC_ACCESS_TIMING_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_PAGE_TIMING_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 2), "LBC_PAGE_TIMING_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_DATAMUX_TIMING_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 2), "LBC_DATAMUX_TIMING_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_CLE_TIMING_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 2), "LBC_CLE_TIMING_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_SPECIAL_TIMING0_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 2), "LBC_SPECIAL_TIMING0_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_SPECIAL_TIMING1_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 2), "LBC_SPECIAL_TIMING1_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_SPECIAL_TIMING2_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 2), "LBC_SPECIAL_TIMING2_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_TURNAROUND_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 2), "LBC_TURNAROUND_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_WAIT_DATA_NUM_REG + LBC_CS_TIMING_REG_OFFSET * 2), "LBC_WAIT_DATA_NUM_REG");


	//-----------------CS3----------------------
	printk("--------------------CS3--------------------\n");
	readl_print(lbc_dev->reg_addr, (LBC_MODE_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 3), "LBC_MODE_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_TIMESCALE_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 3), "LBC_TIMESCALE_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_READY_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 3), "LBC_READY_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_DP_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 3), "LBC_DP_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_BE_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 3), "LBC_BE_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_WAIT_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 3), "LBC_WAIT_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_CS_TIMING_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 3), "LBC_CS_TIMING_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_ALE_TIMING0_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 3), "LBC_ALE_TIMING0_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_ALE_TIMING1_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 3), "LBC_ALE_TIMING1_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_WE_TIMING_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 3), "LBC_WE_TIMING_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_OE_TIMING0_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 3), "LBC_OE_TIMING0_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_OE_TIMING1_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 3), "LBC_OE_TIMING1_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_OE_TIMING2_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 3), "LBC_OE_TIMING2_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_CYCLE_TIMING_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 3), "LBC_CYCLE_TIMING_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_ACCESS_TIMING_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 3), "LBC_ACCESS_TIMING_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_PAGE_TIMING_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 3), "LBC_PAGE_TIMING_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_DATAMUX_TIMING_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 3), "LBC_DATAMUX_TIMING_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_CLE_TIMING_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 3), "LBC_CLE_TIMING_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_SPECIAL_TIMING0_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 3), "LBC_SPECIAL_TIMING0_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_SPECIAL_TIMING1_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 3), "LBC_SPECIAL_TIMING1_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_SPECIAL_TIMING2_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 3), "LBC_SPECIAL_TIMING2_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_TURNAROUND_CTRL_REG + LBC_CS_TIMING_REG_OFFSET * 3), "LBC_TURNAROUND_CTRL_REG");
	readl_print(lbc_dev->reg_addr, (LBC_WAIT_DATA_NUM_REG + LBC_CS_TIMING_REG_OFFSET * 3), "LBC_WAIT_DATA_NUM_REG");

	return 0;
}

static ssize_t lbc_log_level_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "Usage:\necho [0~4] > log_level\n"
			"0: NO LOG\n"
			"1: LOG ERROR\n"
			"2: LOG WARNING\n"
			"3: LOG_INFO\n"
			"4: LOG_DEBUG\n"
			"\nnow log_level: %d\n", lbc_log_level);
}

static ssize_t lbc_log_level_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int ret;
	uint32_t data;

	ret = kstrtou32(buf, 0, &data);
	if (ret) {
		dev_err(dev, "set log level fail\n");
		return ret;
	}

	lbc_log_level = data;
	return count;
}

static ssize_t lbc_sub_time_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	sunxi_lbc_t	*lbc_dev = dev_get_drvdata(dev);

	return sprintf(buf, "Usage:\necho [0~3] [0~5] > lbc_sub_time_mode\n"
			"0:NONE\n"
			"1:NRDD1\n"
			"2:NRDD15\n"
			"3:ADC 7616\n"
			"4:NAND\n"
			"5:ASIC\n"
			"\nnow protocol: \n"
			"	cs0: %u\n"
			"	cs1: %u\n"
			"	cs2: %u\n"
			"	cs3: %u\n"
			, lbc_dev->cs_cfg[0].sub_time_mode
			, lbc_dev->cs_cfg[1].sub_time_mode
			, lbc_dev->cs_cfg[2].sub_time_mode
			, lbc_dev->cs_cfg[3].sub_time_mode);
}

static ssize_t lbc_sub_time_mode_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	sunxi_lbc_t *lbc_dev = dev_get_drvdata(dev);
	int ret;
	uint32_t cs = 0;
	uint32_t val = 0;

	if (IS_ERR_OR_NULL(lbc_dev)) {
		dev_err(dev, "failed to get lbc dev\n");
		return count;
	}

	ret = parse_write_str(buf, &cs, &val);

	mutex_lock(&lbc_lock);
	if (ret) {
		dev_err(dev, "set sub time mode fail\n");
	} else {
		lbc_dev->cs_cfg[cs].sub_time_mode = val;
	}

	mutex_unlock(&lbc_lock);
	return count;
}

static ssize_t dma_wait_time_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "Usage:\necho n > dma_wait_time\n"
			"\nnow dma_wait_time: %d\n", dma_wait_time);
}

static ssize_t dma_wait_time_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int ret;
	uint32_t data;

	ret = kstrtou32(buf, 0, &data);
	if (ret) {
		dev_err(dev, "set log level fail\n");
		return ret;
	}

	dma_wait_time = data;
	return count;
}

static struct device_attribute dev_attr_lbc_init =
	__ATTR(lbc_init, 0644, NULL, lbc_init_ctrl);

static struct device_attribute dev_attr_lbc_fpga_io_ctrl =
	__ATTR(fpga_io_ctrl, 0644, NULL, lbc_fpga_io_ctrl);

static struct device_attribute dev_attr_lbc_write_ctrl =
	__ATTR(lbc_write, 0644, NULL, lbc_write_ctrl);

static struct device_attribute dev_attr_lbc_read_ctrl =
	__ATTR(lbc_read, 0644, NULL, lbc_read_ctrl);

static struct device_attribute dev_attr_lbc_msg =
	__ATTR(message, 0444, lbc_config_message, NULL);

static struct device_attribute dev_attr_lbc_clk_freq =
	__ATTR(clk_freq, 0664, lbc_clk_freq_show, lbc_clk_freq_store);

static struct device_attribute dev_attr_lbc_transfer_mode =
	__ATTR(transfer_mode, 0664, lbc_transfer_mode_show, lbc_transfer_mode_store);

static struct device_attribute dev_attr_lbc_transfer_width =
	__ATTR(transfer_width, 0664, lbc_transfer_width_show, lbc_transfer_width_store);

static struct device_attribute dev_attr_lbc_chain_delay =
	__ATTR(chain_delay, 0664, lbc_chain_delay_show, lbc_chain_delay_store);

static struct device_attribute dev_attr_lbc_burst_mode =
	__ATTR(burst_mode, 0664, lbc_burst_mode_show, lbc_burst_mode_store);

static struct device_attribute dev_attr_lbc_protocol =
	__ATTR(lbc_protocol, 0644, lbc_protocol_show, lbc_protocol_store);

static struct device_attribute dev_attr_lbc_addr_mux =
	__ATTR(addr_mux, 0644, lbc_addr_mux_show, lbc_addr_mux_store);

static struct device_attribute dev_attr_lbc_dp_mode =
	__ATTR(dp_mode, 0644, lbc_dp_mode_show, lbc_dp_mode_store);

static struct device_attribute dev_attr_lbc_dp_parity =
	__ATTR(dp_parity, 0644, lbc_dp_parity_show, lbc_dp_parity_store);

static struct device_attribute dev_attr_lbc_dp_en =
	__ATTR(dp_en, 0644, lbc_dp_en_show, lbc_dp_en_store);

static struct device_attribute dev_attr_lbc_bus_endian =
	__ATTR(bus_endian, 0644, lbc_bus_endian_show, lbc_bus_endian_store);

static struct device_attribute dev_attr_lbc_dump_registers =
	__ATTR(dump_registers, 0444, lbc_dump_registers_show, NULL);

static struct device_attribute dev_attr_lbc_da_ida_dma_test_start =
	__ATTR(da_ida_dma_test_start, 0644, NULL, lbc_da_ida_dma_con_test_start);

static struct device_attribute dev_attr_lbc_da_ida_dma_test_stop =
	__ATTR(da_ida_dma_test_stop, 0644, NULL, lbc_da_ida_dma_con_test_stop);

static struct device_attribute dev_attr_lbc_log_level =
	__ATTR(log_level, 0644, lbc_log_level_show, lbc_log_level_store);

static struct device_attribute dev_attr_lbc_sub_time_mode =
	__ATTR(sub_time_mode, 0644, lbc_sub_time_mode_show, lbc_sub_time_mode_store);

static struct device_attribute dev_attr_lbc_dma_wait_time =
	__ATTR(dma_wait_time, 0644, dma_wait_time_show, dma_wait_time_store);

static struct attribute *lbc_attributes[] = {
	&dev_attr_lbc_init.attr,
	&dev_attr_lbc_fpga_io_ctrl.attr,
	&dev_attr_lbc_write_ctrl.attr,
	&dev_attr_lbc_read_ctrl.attr,
	&dev_attr_lbc_msg.attr,
	&dev_attr_lbc_clk_freq.attr,
	&dev_attr_lbc_transfer_mode.attr,
	&dev_attr_lbc_transfer_width.attr,
	&dev_attr_lbc_chain_delay.attr,
	&dev_attr_lbc_burst_mode.attr,
	&dev_attr_lbc_protocol.attr,
	&dev_attr_lbc_addr_mux.attr,
	&dev_attr_lbc_dp_mode.attr,
	&dev_attr_lbc_dp_parity.attr,
	&dev_attr_lbc_dp_en.attr,
	&dev_attr_lbc_bus_endian.attr,
	&dev_attr_lbc_dump_registers.attr,
	&dev_attr_lbc_da_ida_dma_test_start.attr,
	&dev_attr_lbc_da_ida_dma_test_stop.attr,
	&dev_attr_lbc_log_level.attr,
	&dev_attr_lbc_sub_time_mode.attr,
	&dev_attr_lbc_dma_wait_time.attr,
	NULL,
};

static struct attribute_group lbc_group = {
	.attrs = lbc_attributes,
};

static const struct attribute_group *lbc_groups[] = {
	&lbc_group,
	NULL,
};

static int lbc_open(struct inode *inode, struct file *file)
{
	uint32_t cs_idx = 0;
	sunxi_lbc_t *lbc_dev = container_of(inode->i_cdev, sunxi_lbc_t, cdev);
	mutex_lock(&lbc_lock);
	file->private_data = lbc_dev;
	//lbc_clk_init(lbc_dev);
	lbc_hw_init(lbc_dev, cs_idx);
	mutex_unlock(&lbc_lock);

	return 0;
}

static loff_t sunxi_lbc_llseek(struct file *file, loff_t offset, int whence)
{
	sunxi_lbc_t *lbc_dev = file->private_data;
	loff_t newpos = 0;
	int ret = 0;

	mutex_lock(&lbc_lock);
	switch (whence) {
	case SEEK_SET:
		newpos = offset;
		break;
	case SEEK_CUR:
		newpos = lbc_dev->dma_f_pos + offset;
		break;
	/*case SEEK_END:
		break; */
	default:
		ret = -1;
		goto out;
	}

	if (newpos < 0) {
		ret = -1;
		goto out;
	}

	lbc_dev->dma_f_pos = newpos;
out:
	mutex_unlock(&lbc_lock);

	if (ret != 0)
		return ret;
	return newpos;
}


static int lbc_mmap(struct file *file, struct vm_area_struct *vma)
{
	sunxi_lbc_t *lbc_dev = file->private_data;
	uint32_t cs_idx = 0;
	int ret = 0;

	mutex_lock(&lbc_lock);
	vma->vm_flags |= VM_IO;
	/* addr 4K align */
	if (lbc_dev->cs_cfg[cs_idx].transfer_mode == TRANSFER_MODE_DIRECT_ACCESS) {
		vma->vm_pgoff = lbc_dev->data_sram_phy_addr[cs_idx] >> PAGE_SHIFT;
		vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	} else if (lbc_dev->cs_cfg[cs_idx].transfer_mode == TRANSFER_MODE_DMA) {
		if (lbc_dev->dma_mmap_dir == CMD_MMAP_DMA_TX_BUF) {
			vma->vm_pgoff = lbc_dev->dma_tx_buffer_phy >> PAGE_SHIFT;
			pr_err("%s->%d, lbc_dev->dma_tx_buffer_phy 0x%08lx\n",
					__func__, __LINE__, (long unsigned int)lbc_dev->dma_tx_buffer_phy);
		} else if (lbc_dev->dma_mmap_dir == CMD_MMAP_DMA_RX_BUF) {
			vma->vm_pgoff = lbc_dev->dma_rx_buffer_phy >> PAGE_SHIFT;
			pr_err("%s->%d, lbc_dev->dma_rx_buffer_phy 0x%08lx\n",
					__func__, __LINE__, (long unsigned int)lbc_dev->dma_rx_buffer_phy);
		}
	}

	if (remap_pfn_range(vma,
			vma->vm_start,
			vma->vm_pgoff,
			vma->vm_end - vma->vm_start,
			vma->vm_page_prot)) {
		ret = -EAGAIN;
	}

	mutex_unlock(&lbc_lock);
	return ret;
}

static ssize_t sunxi_lbc_write(struct file *file,
				const char __user *data,
				size_t count, loff_t *ppos)
{
	sunxi_lbc_t *lbc_dev = file->private_data;
	int ret = 0;

	if (*ppos != 0)
		return -EINVAL;

	mutex_lock(&lbc_lock);

	if (count > DMA_DATA_MAX)
		count = DMA_DATA_MAX;

	if (copy_from_user((void *)lbc_dev->dma_tx_buffer, (void __user *)data, count)) {
		ret = -EFAULT;
		goto out;
	}

	lbc_dma_write(lbc_dev, (lbc_dev->dma_target_addr + lbc_dev->dma_f_pos), count, false);

out:
	mutex_unlock(&lbc_lock);
	return ret;
}

static ssize_t sunxi_lbc_read(struct file *file,
				char __user *data,
				size_t count, loff_t *ppos)
{
	sunxi_lbc_t *lbc_dev = file->private_data;
	int ret = 0;

	if (count > DMA_DATA_MAX)
		count = DMA_DATA_MAX;

	mutex_lock(&lbc_lock);

	lbc_dma_read(lbc_dev, (lbc_dev->dma_target_addr + lbc_dev->dma_f_pos), count, false);

	if (copy_to_user((void __user *)data, (void *)lbc_dev->dma_rx_buffer, count)) {
		ret = -EFAULT;
		goto out;
	}

out:
	mutex_unlock(&lbc_lock);
	return 0;
}

static long sunxi_lbc_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 1;
	sunxi_lbc_t *lbc_dev = file->private_data;
	struct device *dev = lbc_dev->dev;
	int buf[2];
	unsigned int irpr_tcip;
	int time_cnt;
	int i = 0;

	switch (cmd) {
	case CMD_DMA_TX_START:
		if (copy_from_user((void *)&buf, (void __user *)arg, 2*sizeof(int))) {
			return -EFAULT;
		}

		for (i = 0; i < 16; i++) {
			pr_info("%s->%d, dma_tx_buffer[%d] = 0x%x", __func__, __LINE__, i, lbc_dev->dma_tx_buffer[i]);
		}

		writel((lbc_dev->dma_tx_phy >> 2), (lbc_dev->reg_addr + DMA_DES_ADDR_REG));
		sunxi_lbc_dump_dma_desc(lbc_dev->dma_tx, 1);
		dma_transfer_start(lbc_dev->reg_addr);

		while (1) {
			irpr_tcip = readl(lbc_dev->reg_addr + LBC_INT0_PENDING_REG) & DMA_TRSF_DONE_IRPR;
			if (irpr_tcip == DMA_TRSF_DONE_IRPR) {
				writel((DMA_TRSF_DONE_IRPR | CMD_DONE_IRPR), (lbc_dev->reg_addr + LBC_INT0_PENDING_REG));
				dev_err(dev, "%s->%d , dma tx ok\n", __func__, __LINE__);
				break;
			}
			if (++time_cnt >= 100) {
				dev_err(dev, "%s->%d, dma tx time out,  irpr_tcip 0x%x\n", __func__, __LINE__, irpr_tcip);
				break;
			}
			mdelay(10);
		}

		break;
	case CMD_DMA_RX_START:
		if (copy_from_user((void *)&buf, (void __user *)arg, 2*sizeof(int))) {
			return -EFAULT;
		}
		writel((lbc_dev->dma_rx_phy >> 2), (lbc_dev->reg_addr + DMA_DES_ADDR_REG));
		sunxi_lbc_dump_dma_desc(lbc_dev->dma_rx, 1);
		dma_transfer_start(lbc_dev->reg_addr);
		while (1) {
			irpr_tcip = readl(lbc_dev->reg_addr + LBC_INT0_PENDING_REG) & DMA_TRSF_DONE_IRPR;
			if (irpr_tcip == DMA_TRSF_DONE_IRPR) {
				writel((DMA_TRSF_DONE_IRPR | CMD_DONE_IRPR), (lbc_dev->reg_addr + LBC_INT0_PENDING_REG));
				dev_info(dev, "%s->%d , dma rx ok\n", __func__, __LINE__);
				break;
			}
			if (++time_cnt >= 100) {
				dev_info(dev, "%s->%d, dma rx time out,  irpr_tcip 0x%x\n", __func__, __LINE__, irpr_tcip);
				break;
			}
			mdelay(10);
		}

		for (i = 0; i < 16; i++) {
			pr_info("%s->%d, dma_rx_buffer[%d] = 0x%x", __func__, __LINE__, i, lbc_dev->dma_rx_buffer[i]);
		}
		break;
	case CMD_MMAP_DMA_TX_BUF:
		lbc_dev->dma_mmap_dir = CMD_MMAP_DMA_TX_BUF;
		break;
	case CMD_MMAP_DMA_RX_BUF:
		lbc_dev->dma_mmap_dir = CMD_MMAP_DMA_RX_BUF;
		break;
	default:
		break;
	}

	dev_info(dev, "cmd 0x%x , buf[0] = 0x%x, buf[1] = 0x%x\n", cmd, buf[0], buf[1]);
	return ret;
}

static struct file_operations lbc_fops = {
	.owner = THIS_MODULE,
	.open = lbc_open,
	.write = sunxi_lbc_write,
	.read = sunxi_lbc_read,
	.llseek = sunxi_lbc_llseek,
	.mmap = lbc_mmap,
	.unlocked_ioctl = sunxi_lbc_ioctl,
};

void delay_cycles(uint32_t num_cycles)
{
	volatile uint32_t i = 0;
	for (i = 0; i < num_cycles; i++) {
		__asm__ volatile ("nop");
	}
}

static void do_da_soft_reset(sunxi_lbc_t *lbc_dev)
{
	uint32_t val = 0x1 << 0;

	uint32_t reg_val = 0;
	uint32_t da_rx_ot_fifo_wl = 0xff;
	uint32_t da_tx_ot_fifo_wl = 0xff;

	do {
		reg_val = readl(lbc_dev->reg_addr + DA_INTF_STA_REG);
		da_rx_ot_fifo_wl = LBC_GET_FIELD32(reg_val, MS_DAIS_DA_RX_OT_FIFO_WL);
		da_tx_ot_fifo_wl = LBC_GET_FIELD32(reg_val, MS_DAIS_DA_TX_OT_FIFO_WL);
	} while (da_rx_ot_fifo_wl || da_tx_ot_fifo_wl);

	// do reset
	do {
		writel(val, lbc_dev->reg_addr + LBC_SOFT_RST_REG);
	} while ((readl(lbc_dev->reg_addr + LBC_SOFT_RST_REG) & val) == 0);
	delay_cycles(100);

	do {
		writel(0x0, lbc_dev->reg_addr + LBC_SOFT_RST_REG);
	} while ((readl(lbc_dev->reg_addr + LBC_SOFT_RST_REG) & val) != 0);
}

static void do_ida_soft_reset(sunxi_lbc_t *lbc_dev)
{
	uint32_t val = 0x1 << 1;
	do {
		writel(val, lbc_dev->reg_addr + LBC_SOFT_RST_REG);
	} while ((readl(lbc_dev->reg_addr + LBC_SOFT_RST_REG) & val) == 0);

	delay_cycles(100);

	do {
		writel(0x0, lbc_dev->reg_addr + LBC_SOFT_RST_REG);
	} while ((readl(lbc_dev->reg_addr + LBC_SOFT_RST_REG) & val) != 0);
}

static void do_dma_soft_reset(sunxi_lbc_t *lbc_dev)
{
	uint32_t val = 0x1 << 2;

	do {
		writel(val, lbc_dev->reg_addr + LBC_SOFT_RST_REG);
	} while ((readl(lbc_dev->reg_addr + LBC_SOFT_RST_REG) & val) == 0);

	delay_cycles(100);

	do {
		writel(0x0, lbc_dev->reg_addr + LBC_SOFT_RST_REG);
	} while ((readl(lbc_dev->reg_addr + LBC_SOFT_RST_REG) & val) != 0);
}

static irqreturn_t sunxi_lbc_handler(int irq, void *dev_id)
{
	u32 irs_status0;
	u32 irs_status1;
	//u32 data;
	u32 data1;
	sunxi_lbc_t *lbc_dev = (sunxi_lbc_t *)dev_id;

	irs_status0 = readl(lbc_dev->reg_addr + LBC_INT0_PENDING_REG);
	irs_status1 = readl(lbc_dev->reg_addr + LBC_INT1_PENDING_REG);

	lbc_log(LOG_DEBUG, " interrupt pending0: 0x%x, pending1: 0x%x\n", irs_status0, irs_status1);

	//irq_pending0
	/* dma */
	if (irs_status0 & DMA_TRSF_DONE_IRPR) {
		writel((DMA_TRSF_DONE_IRPR), (lbc_dev->reg_addr + LBC_INT0_PENDING_REG));
		lbc_dev->dma_result = TRANS_COMPLETED;
		wake_up(&lbc_dev->dma_wait);
	}

	/* Read Transmission interrupt finish*/
	if ((irs_status0 & IDA_RX_TRIG_IRPR) && (readl(lbc_dev->reg_addr + LBC_INT0_PENDING_REG) & IDA_RX_TRIG_IRPR)) {
			lbc_log(LOG_DEBUG, "*** ida rx trig\n");
			data1 = readl(lbc_dev->reg_addr + IDA_FIFO_TRIGGER_LVL_REG) >> 16 ;
			rx_fifo_read(lbc_dev, data1);
			writel(IDA_RX_TRIG_IRPR, lbc_dev->reg_addr + LBC_INT0_PENDING_REG);
	}

	/* Write Transmission interrupt finish*/
	if ((irs_status0 & IDA_TRSF_DONE_IRPR)) {	//Transmitt finish
		writel(IDA_TRSF_DONE_IRPR, lbc_dev->reg_addr + LBC_INT0_PENDING_REG);

		lbc_dev->ida_result = TRANS_COMPLETED;
		wake_up(&lbc_dev->ida_wait);
	}

	/* Ready wait cycle Over time interrupt*/
	if (irs_status0 & READY_TIMEOUT_IRPR) {	//TIME OUT
		writel(READY_TIMEOUT_IRPR, lbc_dev->reg_addr + LBC_INT0_PENDING_REG);
	}

	/* OVERFLOW_UNDERFLOW error*/
	if (irs_status0 & OVER_UNDER_FLOW_IRPR) {
		writel(OVER_UNDER_FLOW_IRPR, lbc_dev->reg_addr + LBC_INT0_PENDING_REG);
	}

	/* Data parity error*/
	if ((irs_status0 & DP_IRPR) || (irs_status0 & ODP_IRPR)) {
		writel((DP_IRPR | ODP_IRPR), lbc_dev->reg_addr + LBC_INT0_PENDING_REG);
	}

	/* invalid dam descriptor */
	if (irs_status0 & MS_IT0P_DMA_DES_INVLD_IRPR_MASK) {
		writel(MS_IT0P_DMA_DES_INVLD_IRPR_MASK, lbc_dev->reg_addr + LBC_INT0_PENDING_REG);
	}

	//irq_pending1
	if (irs_status1 & MS_IT1P_DMA_TIMEOUT_IRPR_MASK) {
		do_dma_soft_reset(lbc_dev);
		writel(MS_IT1P_DMA_TIMEOUT_IRPR_MASK, (lbc_dev->reg_addr + LBC_INT1_PENDING_REG));
		printk("**** dma soft reset\n");
	}

	if (irs_status1 & MS_IT1P_DA_TIMEOUT_IRPR_MASK) {
		do_da_soft_reset(lbc_dev);
		writel(MS_IT1P_DA_TIMEOUT_IRPR_MASK, (lbc_dev->reg_addr + LBC_INT1_PENDING_REG));
		printk("**** da soft reset\n");
	}

	if (irs_status1 & MS_IT1P_IDA_TIMEOUT_IRPR_MASK) {
		do_ida_soft_reset(lbc_dev);
		writel(MS_IT1P_IDA_TIMEOUT_IRPR_MASK, (lbc_dev->reg_addr + LBC_INT1_PENDING_REG));
		printk("**** ida soft reset\n");
	}

	return IRQ_HANDLED;
}

static int lbc_analy_clk_dts(struct platform_device *pdev, sunxi_lbc_t *lbc_dev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;

	lbc_dev->lbc_pll = devm_clk_get(&pdev->dev, "lbc_pll");
	if (IS_ERR_OR_NULL(lbc_dev->lbc_pll)) {
		dev_err(dev, "Could not get lbc_pll\n");
	}

	lbc_dev->bus_lbc = devm_clk_get(&pdev->dev, "bus_lbc");
	if (IS_ERR_OR_NULL(lbc_dev->bus_lbc)) {
		dev_err(dev, "Could not get bus_lbc\n");
	}

	lbc_dev->lbc = devm_clk_get(&pdev->dev, "lbc");
	if (IS_ERR_OR_NULL(lbc_dev->lbc)) {
		dev_err(dev, "Could not get lbc clk\n");
	}

	lbc_dev->lbc_rst = devm_reset_control_get(&pdev->dev, "lbc_rst");
	if (IS_ERR_OR_NULL(lbc_dev->lbc_rst)) {
		dev_err(dev, "Could not get lbc rst\n");
	}

	if (of_property_read_u32(np, "clock-frequency", &lbc_dev->lbc_freq)) {
		dev_err(dev, "Could not get clock frequency\n");
		return -1;
	}

	return 0;
}

static int lbc_analy_dts(struct platform_device *pdev, sunxi_lbc_t *lbc_dev)
{
	//int ret = 0;
	struct resource *res;
	//enum of_gpio_flags config;
	struct device *dev = &pdev->dev;
	//struct device_node *np = pdev->dev.of_node;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "reg_addr");
	if (IS_ERR_OR_NULL(res)) {
		dev_err(dev, "no find reg in dts\n");
		return -1;
	}

	lbc_dev->reg_addr = devm_ioremap_resource(&pdev->dev, res);
	if (!lbc_dev->reg_addr) {
		dev_err(dev, "reg_addr memory mapping failed\n");
		return -1;
	}

	// ------------------------cs0--------------------------
	// data reg addr
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "cs0_data_reg_addr");
	if (IS_ERR_OR_NULL(res)) {
		dev_err(dev, "no find data reg cs0 in dts\n");
		return -1;
	}

	lbc_dev->data_reg_addr[0] = devm_ioremap_resource(&pdev->dev, res);
	if (!lbc_dev->data_reg_addr[0]) {
		dev_err(dev, "data_reg_addr cs0  memory mapping failed\n");
		return -1;
	}
	lbc_dev->data_reg_phy_addr[0] = res->start;
	lbc_dev->data_reg_addr_size[0] = resource_size(res);

	dev_info(dev, "start addr 0x%x , size 0x%x\n", lbc_dev->data_reg_phy_addr[0], lbc_dev->data_reg_addr_size[0]);

	// data sram addr
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "cs0_data_sram_addr");
	if (IS_ERR_OR_NULL(res)) {
		dev_err(dev, "no find data sram cs0 in dts\n");
		return -1;
	}

	lbc_dev->data_sram_addr[0] = devm_ioremap_resource(&pdev->dev, res);
	if (!lbc_dev->data_sram_addr[0]) {
		dev_err(dev, "data_sram_addr cs0 memory mapping failed\n");
		return -1;
	}
	lbc_dev->data_sram_phy_addr[0] = res->start;
	lbc_dev->data_sram_addr_size[0] = resource_size(res);

	dev_info(dev, "start addr 0x%x , size 0x%x\n", lbc_dev->data_sram_phy_addr[0], lbc_dev->data_sram_addr_size[0]);

	// data fifo addr
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "cs0_data_test_addr");
	if (IS_ERR_OR_NULL(res)) {
		dev_err(dev, "no find data reg cs0 in dts\n");
		return -1;
	}

	lbc_dev->data_test_addr[0] = devm_ioremap_resource(&pdev->dev, res);
	if (!lbc_dev->data_test_addr[0]) {
		dev_err(dev, "data_test_addr memory cs0 mapping failed\n");
		return -1;
	}
	lbc_dev->data_test_phy_addr[0] = res->start;
	lbc_dev->data_test_addr_size[0] = resource_size(res);

	dev_info(dev, "start addr 0x%x , size 0x%x\n", lbc_dev->data_test_phy_addr[0], lbc_dev->data_test_addr_size[0]);

	// ------------------------cs1--------------------------
	// data reg addr
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "cs1_data_reg_addr");
	if (IS_ERR_OR_NULL(res)) {
		dev_err(dev, "no find data reg in dts\n");
		return -1;
	}

	lbc_dev->data_reg_addr[1] = devm_ioremap_resource(&pdev->dev, res);
	if (!lbc_dev->data_reg_addr[1]) {
		dev_err(dev, "data_reg_addr cs1  memory mapping failed\n");
		return -1;
	}
	lbc_dev->data_reg_phy_addr[1] = res->start;
	lbc_dev->data_reg_addr_size[1] = resource_size(res);

	dev_info(dev, "start addr 0x%x , size 0x%x\n", lbc_dev->data_reg_phy_addr[1], lbc_dev->data_reg_addr_size[1]);

	// data sram addr
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "cs1_data_sram_addr");
	if (IS_ERR_OR_NULL(res)) {
		dev_err(dev, "no find data sram cs1 in dts\n");
		return -1;
	}

	lbc_dev->data_sram_addr[1] = devm_ioremap_resource(&pdev->dev, res);
	if (!lbc_dev->data_sram_addr[1]) {
		dev_err(dev, "data_sram_addr cs1 memory mapping failed\n");
		return -1;
	}
	lbc_dev->data_sram_phy_addr[1] = res->start;
	lbc_dev->data_sram_addr_size[1] = resource_size(res);

	dev_info(dev, "start addr 0x%x , size 0x%x\n", lbc_dev->data_sram_phy_addr[1], lbc_dev->data_sram_addr_size[1]);

	// data fifo addr
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "cs1_data_test_addr");
	if (IS_ERR_OR_NULL(res)) {
		dev_err(dev, "no find data reg cs1 in dts\n");
		return -1;
	}

	lbc_dev->data_test_addr[1] = devm_ioremap_resource(&pdev->dev, res);
	if (!lbc_dev->data_test_addr[1]) {
		dev_err(dev, "data_test_addr memory cs1 mapping failed\n");
		return -1;
	}
	lbc_dev->data_test_phy_addr[1] = res->start;
	lbc_dev->data_test_addr_size[1] = resource_size(res);

	dev_info(dev, "start addr 0x%x , size 0x%x\n", lbc_dev->data_test_phy_addr[1], lbc_dev->data_test_addr_size[1]);




	// ------------------------cs0--------------------------
	// data reg addr
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "cs2_data_reg_addr");
	if (IS_ERR_OR_NULL(res)) {
		dev_err(dev, "no find data reg in dts\n");
		return -1;
	}

	lbc_dev->data_reg_addr[2] = devm_ioremap_resource(&pdev->dev, res);
	if (!lbc_dev->data_reg_addr[2]) {
		dev_err(dev, "data_reg_addr cs2  memory mapping failed\n");
		return -1;
	}
	lbc_dev->data_reg_phy_addr[2] = res->start;
	lbc_dev->data_reg_addr_size[2] = resource_size(res);

	dev_info(dev, "start addr 0x%x , size 0x%x\n", lbc_dev->data_reg_phy_addr[2], lbc_dev->data_reg_addr_size[2]);

	// data sram addr
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "cs2_data_sram_addr");
	if (IS_ERR_OR_NULL(res)) {
		dev_err(dev, "no find data sram cs2 in dts\n");
		return -1;
	}

	lbc_dev->data_sram_addr[2] = devm_ioremap_resource(&pdev->dev, res);
	if (!lbc_dev->data_sram_addr[2]) {
		dev_err(dev, "data_sram_addr cs2 memory mapping failed\n");
		return -1;
	}
	lbc_dev->data_sram_phy_addr[2] = res->start;
	lbc_dev->data_sram_addr_size[2] = resource_size(res);

	dev_info(dev, "start addr 0x%x , size 0x%x\n", lbc_dev->data_sram_phy_addr[2], lbc_dev->data_sram_addr_size[2]);

	// data fifo addr
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "cs2_data_test_addr");
	if (IS_ERR_OR_NULL(res)) {
		dev_err(dev, "no find data reg cs2 in dts\n");
		return -1;
	}

	lbc_dev->data_test_addr[2] = devm_ioremap_resource(&pdev->dev, res);
	if (!lbc_dev->data_test_addr[2]) {
		dev_err(dev, "data_test_addr memory cs2 mapping failed\n");
		return -1;
	}
	lbc_dev->data_test_phy_addr[2] = res->start;
	lbc_dev->data_test_addr_size[2] = resource_size(res);

	dev_info(dev, "start addr 0x%x , size 0x%x\n", lbc_dev->data_test_phy_addr[2], lbc_dev->data_test_addr_size[2]);


	// ------------------------cs3--------------------------
	// data reg addr
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "cs3_data_reg_addr");
	if (IS_ERR_OR_NULL(res)) {
		dev_err(dev, "no find data reg in dts\n");
		return -1;
	}

	lbc_dev->data_reg_addr[3] = devm_ioremap_resource(&pdev->dev, res);
	if (!lbc_dev->data_reg_addr[3]) {
		dev_err(dev, "data_reg_addr cs3  memory mapping failed\n");
		return -1;
	}
	lbc_dev->data_reg_phy_addr[3] = res->start;
	lbc_dev->data_reg_addr_size[3] = resource_size(res);

	dev_info(dev, "start addr 0x%x , size 0x%x\n", lbc_dev->data_reg_phy_addr[3], lbc_dev->data_reg_addr_size[3]);

	// data sram addr
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "cs3_data_sram_addr");
	if (IS_ERR_OR_NULL(res)) {
		dev_err(dev, "no find data sram cs3 in dts\n");
		return -1;
	}

	lbc_dev->data_sram_addr[3] = devm_ioremap_resource(&pdev->dev, res);
	if (!lbc_dev->data_sram_addr[3]) {
		dev_err(dev, "data_sram_addr cs3 memory mapping failed\n");
		return -1;
	}
	lbc_dev->data_sram_phy_addr[3] = res->start;
	lbc_dev->data_sram_addr_size[3] = resource_size(res);

	dev_info(dev, "start addr 0x%x , size 0x%x\n", lbc_dev->data_sram_phy_addr[3], lbc_dev->data_sram_addr_size[3]);

	// data fifo addr
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "cs3_data_test_addr");
	if (IS_ERR_OR_NULL(res)) {
		dev_err(dev, "no find data reg cs3 in dts\n");
		return -1;
	}

	lbc_dev->data_test_addr[3] = devm_ioremap_resource(&pdev->dev, res);
	if (!lbc_dev->data_test_addr[3]) {
		dev_err(dev, "data_test_addr memory cs3 mapping failed\n");
		return -1;
	}
	lbc_dev->data_test_phy_addr[3] = res->start;
	lbc_dev->data_test_addr_size[3] = resource_size(res);

	dev_info(dev, "start addr 0x%x , size 0x%x\n", lbc_dev->data_test_phy_addr[3], lbc_dev->data_test_addr_size[3]);



	// ana clk dts
	lbc_analy_clk_dts(pdev, lbc_dev);

	lbc_clk_init(lbc_dev);

	return 0;
}

static int do_dma_alloc(sunxi_lbc_t *lbc_dev)
{
	int ret = 0;
	struct device *dev = lbc_dev->dev;

	lbc_dev->dma_tx_buffer = dma_alloc_coherent(lbc_dev->dev,
									DMA_DATA_MAX,
									&lbc_dev->dma_tx_buffer_phy,
									GFP_KERNEL);
	if (!lbc_dev->dma_tx_buffer) {
		dev_err(dev, "failed to alloc tx buffer dma coherent\n");
		ret = -1;
		goto out_err;
	}

	lbc_dev->dma_rx_buffer = dma_alloc_coherent(lbc_dev->dev,
									DMA_DATA_MAX,
									&lbc_dev->dma_rx_buffer_phy,
									GFP_KERNEL);
	if (!lbc_dev->dma_rx_buffer) {
		dev_err(dev, "failed to alloc rx buffer dma coherent\n");
		ret = -1;
		goto tx_buffer_del;
	}

	lbc_dev->dma_tx = dma_alloc_coherent(lbc_dev->dev,
									DMA_DESCS_SIZE,
									&lbc_dev->dma_tx_phy,
									GFP_KERNEL);
	if (!lbc_dev->dma_tx) {
		dev_err(dev, "failed to alloc tx dma desc coherent\n");
		ret = -1;
		goto rx_buffer_del;
	}

	lbc_dev->dma_rx = dma_alloc_coherent(lbc_dev->dev,
									DMA_DESCS_SIZE,
									&lbc_dev->dma_rx_phy,
									GFP_KERNEL);
	if (!lbc_dev->dma_rx) {
		dev_err(dev, "failed to alloc rx dma desc coherent\n");
		ret = -1;
		goto tx_desc_del;
	}

	dev_info(dev, "dma alloc coherent done\n");
	return 0;

tx_desc_del:
	dma_free_coherent(lbc_dev->dev, DMA_DESCS_SIZE, lbc_dev->dma_tx, lbc_dev->dma_tx_phy);
rx_buffer_del:
	dma_free_coherent(lbc_dev->dev, DMA_DATA_MAX, lbc_dev->dma_rx_buffer, lbc_dev->dma_rx_buffer_phy);
tx_buffer_del:
	dma_free_coherent(lbc_dev->dev, DMA_DATA_MAX, lbc_dev->dma_tx_buffer, lbc_dev->dma_tx_buffer_phy);
out_err:
	return ret;
}

static int lbc_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct device *dev = &pdev->dev;
	char *name_str = "lbc";
	sunxi_lbc_t *sunx_lbc_dev = NULL;

	dma_wait_time = 0;
	lbc_log_level = DEFAULT_LBC_LOG_LEVEL;

	dev_info(dev, "local bus driver probe, version %s\n", LBC_VERSION);

	sunx_lbc_dev = devm_kzalloc(dev, sizeof(*sunx_lbc_dev), GFP_KERNEL);
	if (IS_ERR_OR_NULL(sunx_lbc_dev)) {
		dev_err(dev, "sunx_lbc_dev err\n");
		goto out_err;
	}

	sunx_lbc_dev->pdev = pdev;
	sunx_lbc_dev->dev = &pdev->dev;
	sunx_lbc_dev->rx_index = 0;

	ret = register_chrdev_region(MKDEV(LBC_MAJOR, 0), LBC_MINOR, name_str);
	if (ret)
		goto lbc_del;

	cdev_init(&sunx_lbc_dev->cdev, &lbc_fops);
	sunx_lbc_dev->cdev.owner = THIS_MODULE;
	ret = cdev_add(&sunx_lbc_dev->cdev, MKDEV(LBC_MAJOR, 0), 1);
	if (ret) {
		dev_err(dev, "add cdev fail\n");
		goto region_del;
	}

	sunx_lbc_dev->lbc_class = class_create(THIS_MODULE, name_str);
	if (IS_ERR(sunx_lbc_dev->lbc_class)) {
		ret = PTR_ERR(sunx_lbc_dev->lbc_class);
		goto cdev_del;
	}

	sunx_lbc_dev->dev_lbc = device_create_with_groups(sunx_lbc_dev->lbc_class,
							dev, MKDEV(LBC_MAJOR, 0), NULL,
							lbc_groups, "%s", name_str);

	//device_create_with_groups
	if (IS_ERR(sunx_lbc_dev->dev_lbc)) {
		dev_err(dev, "failed to create device with groups\n");
		goto class_del;
	}

	//do_lbc_mode1_default_config_V2(sunx_lbc_dev);

	ret = lbc_analy_dts(pdev, sunx_lbc_dev);
	if (ret < 0)
		goto group_del;

	sunx_lbc_dev->irq = platform_get_irq(pdev, 0);
	if (sunx_lbc_dev->irq < 0) {
		dev_err(dev, "Could not get irq \n");
		goto group_del;
	}

	ret = devm_request_irq(&pdev->dev, sunx_lbc_dev->irq, sunxi_lbc_handler, \
							IRQF_SHARED, dev_name(&pdev->dev), sunx_lbc_dev);
	if (ret) {
		 dev_err(dev, "Could not request irq %d\n", sunx_lbc_dev->irq);
		 goto group_del;
	}

	ret = do_dma_alloc(sunx_lbc_dev);
	if (ret < 0)
		goto group_del;

/*
	regulator_set_voltage(sunx_lbc_dev->io_supply, \
						sunx_lbc_dev->io_vol, sunx_lbc_dev->io_vol);
	ret = regulator_enable(sunx_lbc_dev->io_supply);
	if (ret) {
		dev_err(dev, "failed to enable regulator\n");
	}
*/
	sunx_lbc_dev->ida_result = TRANS_NONE;
	sunx_lbc_dev->dma_result = TRANS_NONE;

	init_waitqueue_head(&sunx_lbc_dev->ida_wait);
	init_waitqueue_head(&sunx_lbc_dev->dma_wait);

	lbc_mode1_32bit_default_config(sunx_lbc_dev);
	dev_set_drvdata(sunx_lbc_dev->dev_lbc, sunx_lbc_dev);
	platform_set_drvdata(pdev, sunx_lbc_dev);

	dev_info(dev, "local bus driver probe ok ...\n");
	return 0;

group_del:
	device_remove_groups(sunx_lbc_dev->dev_lbc, lbc_groups);
	device_destroy(sunx_lbc_dev->lbc_class, MKDEV(LBC_MAJOR, 0));
class_del:
	class_destroy(sunx_lbc_dev->lbc_class);
cdev_del:
	cdev_del(&sunx_lbc_dev->cdev);
region_del:
	unregister_chrdev_region(MKDEV(LBC_MAJOR, 0), LBC_MINOR);
lbc_del:
	devm_kfree(dev, sunx_lbc_dev);
out_err:
	dev_err(dev, "probed failed\n");
	return ret;
}

static int lbc_remove(struct platform_device *pdev)
{
	sunxi_lbc_t *lbc_dev = platform_get_drvdata(pdev);


	if (lbc_dev->dma_rx)
		dma_free_coherent(lbc_dev->dev, DMA_DESCS_SIZE,
						  lbc_dev->dma_rx, lbc_dev->dma_rx_phy);

	if (lbc_dev->dma_tx)
		dma_free_coherent(lbc_dev->dev, DMA_DESCS_SIZE,
						  lbc_dev->dma_tx, lbc_dev->dma_tx_phy);

	if (lbc_dev->dma_rx_buffer)
		dma_free_coherent(lbc_dev->dev, DMA_DATA_MAX,
						  lbc_dev->dma_rx_buffer, lbc_dev->dma_rx_buffer_phy);

	if (lbc_dev->dma_tx_buffer)
		dma_free_coherent(lbc_dev->dev, DMA_DATA_MAX,
						  lbc_dev->dma_tx_buffer, lbc_dev->dma_tx_buffer_phy);


	if (lbc_dev->dev_lbc) {
		device_remove_groups(lbc_dev->dev_lbc, lbc_groups);
		device_destroy(lbc_dev->lbc_class, MKDEV(LBC_MAJOR, 0));
		lbc_dev->dev_lbc = NULL;
	}

	class_destroy(lbc_dev->lbc_class);
	cdev_del(&lbc_dev->cdev);
	platform_set_drvdata(pdev, NULL);
	unregister_chrdev_region(MKDEV(LBC_MAJOR, 0), LBC_MINOR);

	return 0;
}

static const struct of_device_id sunxi_lbc_matches[] = {
	{.compatible = "allwinner,sunxi-lbc", },
	{},
};

static struct platform_driver lbc_driver = {
	.driver = {
		.name   = "sunxi-lbc",
		.pm     = SUNXI_MBUS_PM_OPS,
		.of_match_table = sunxi_lbc_matches,
	},
	.probe = lbc_probe,
	.remove = lbc_remove,
};

module_platform_driver(lbc_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("SUNXI localbus driver");
MODULE_AUTHOR("wangjin");
MODULE_VERSION(LBC_VERSION);
