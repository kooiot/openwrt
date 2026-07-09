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

#ifndef SUNXI_LBC_V2_REG_H
#define SUNXI_LBC_V2_REG_H

//=============================================================================
// registers start
//=============================================================================
/*-------------basic address---------------*/
#define LBC_REG_BASE					(0x02810000)

#define TSA_SRAM_AW						(0x10000000)
#define TSA_FIFO_AW						(0x20000000)

#define TSA_SRAM_INV					(0xC0000000)


#define LBC_CS0_LADDR				(0x00000000)
#define LBC_CS0_HADDR				(0x20FFFFFF)
#define LBC_CS1_LADDR				(0x21000000)
#define LBC_CS1_HADDR				(0x21FFFFFF)
#define LBC_CS2_LADDR				(0x22000000)
#define LBC_CS2_HADDR				(0x22FFFFFF)
#define LBC_CS3_LADDR				(0x23000000)
#define LBC_CS3_HADDR				(0x23FFFFFF)

/*-------------offset address--------------*/

#define LBC_CS_TIMING_REG_OFFSET	(0x080)
//-----------------CS0----------------------
#define LBC_MODE_CTRL_REG			(0x000)
#define LBC_TIMESCALE_CTRL_REG		(0x004)
#define LBC_READY_CTRL_REG			(0x008)
#define LBC_DP_CTRL_REG				(0x00C)
#define LBC_BE_CTRL_REG				(0x010)
#define LBC_WAIT_CTRL_REG			(0x014)
#define LBC_CS_TIMING_CTRL_REG		(0x018)
#define LBC_ALE_TIMING0_CTRL_REG	(0x01C)
#define LBC_ALE_TIMING1_CTRL_REG	(0x020)
#define LBC_WE_TIMING_CTRL_REG		(0x024)
#define LBC_OE_TIMING0_CTRL_REG		(0x028)
#define LBC_OE_TIMING1_CTRL_REG		(0x02C)
#define LBC_OE_TIMING2_CTRL_REG		(0x030)
#define LBC_CYCLE_TIMING_CTRL_REG	(0x034)
#define LBC_ACCESS_TIMING_CTRL_REG	(0x038)
#define LBC_PAGE_TIMING_CTRL_REG	(0x03C)
#define LBC_DATAMUX_TIMING_CTRL_REG (0x040)
#define LBC_CLE_TIMING_CTRL_REG		(0x044)
#define LBC_SPECIAL_TIMING0_CTRL_REG (0x048)
#define LBC_SPECIAL_TIMING1_CTRL_REG (0x04C)
#define LBC_SPECIAL_TIMING2_CTRL_REG (0x050)
#define LBC_TURNAROUND_CTRL_REG		(0x060)
#define LBC_WAIT_DATA_NUM_REG		(0x070)


#define CH_MODE_CTRL_REG		(0x200)

#define CS_HLADDR_OFFSET		(0x10)

#define CS0_DA_HADDR_REG		(0x210)
#define CS0_DA_LADDR_REG		(0x214)
#define CS0_IDA_HADDR_REG		(0x218)
#define CS0_IDA_LADDR_REG		(0x21C)
#define CS1_DA_HADDR_REG		(0x220)
#define CS1_DA_LADDR_REG		(0x224)
#define CS1_IDA_HADDR_REG		(0x228)
#define CS1_IDA_LADDR_REG		(0x22C)
#define CS2_DA_HADDR_REG		(0x230)
#define CS2_DA_LADDR_REG		(0x234)
#define CS2_IDA_HADDR_REG		(0x238)
#define CS2_IDA_LADDR_REG		(0x23C)
#define CS3_DA_HADDR_REG		(0x240)
#define CS3_DA_LADDR_REG		(0x244)
#define CS3_IDA_HADDR_REG		(0x248)
#define CS3_IDA_LADDR_REG		(0x24C)

#define FIX_CS_REG				(0x250)
#define USER_CS_REG				(0x254)
#define LBC_CLK_REG				(0x280)

#define LBC_SOFT_RST_REG		(0x288)
#define SMP_DELAY_CTRL_REG		(0x290)

#define IDA_TS_CTRL_REG			(0x300)
#define IDA_TS_ADDR_REG			(0x304)
#define IDA_TS_DATA_LEN_REG		(0x308)
#define IDA_WDATA_REG			(0x340)
#define IDA_RDATA_REG			(0x350)
#define DA_BST_SEL_REG			(0x400)
#define DA_BST_TYPE_REG			(0x404)
#define DA_CS0_SADDR_REG		(0x410)
#define DA_CS1_SADDR_REG		(0x414)
#define DA_CS2_SADDR_REG		(0x418)
#define DA_CS3_SADDR_REG		(0x41C)
#define DA_TX_TO_FIX_DONE_REG	(0x420)
#define LBC_TIMEOUT_CLR_REG		(0x424)

#define LBC_ARB_PRI_CFG_REG		(0x440)

#define DMA_MODE_CFG_REG		(0x460)
#define DMA_DES_ADDR_REG		(0x464)
#define DMA_DES0_DBG_REG		(0x4D0)
#define DMA_DES1_DBG_REG		(0x4D4)
#define DMA_DES2_DBG_REG		(0x4D8)
#define DMA_DES3_DBG_REG		(0x4DC)
#define LBC_INT0_EN_REG			(0x500)
#define LBC_INT1_EN_REG			(0x504)
#define LBC_INT0_PENDING_REG	(0x508)
#define LBC_INT1_PENDING_REG	(0x50C)
#define DA_INTF_STA_REG			(0x510)
#define DA_FIFO_STA_REG			(0x514)
#define IDA_FIFO_STA_REG		(0x518)
#define DMA_FIFO_STA_REG		(0x51C)
#define DA_FIFO_TRIGGER_LVL_REG	(0x524)
#define IDA_FIFO_TRIGGER_LVL_REG (0x528)
#define DMA_FIFO_TRIGGER_LVL_REG (0x52C)
#define LBC_FIFO_CLR_REG		(0x530)
#define LBC_IDA_MIRROR_REG		(0x600)
#define LBC_DA_CMD0_REG			(0x604)
#define LBC_DA_CMD1_REG			(0x608)
#define LBC_CMD_BUSY_REG		(0x60C)
#define LBC_CMD0_REG			(0x610)
#define LBC_CMD1_REG			(0x614)
#define LBC_CMD2_REG			(0x618)
#define LBC_DA_TO_CMD0_REG		(0x620)
#define LBC_DA_TO_CMD1_REG		(0x624)
#define LBC_IDA_TO_CMD0_REG		(0x628)
#define LBC_IDA_TO_CMD1_REG		(0x62C)
#define LBC_DMA_TO_CMD0_REG		(0x630)
#define LBC_DMA_TO_CMD1_REG		(0x634)

//=============================================================================
// registers end
//=============================================================================


//=============================================================================
// Mask and Shift start
//=============================================================================
// LBC_MODE_CTRL_REG	(0x000) MC
#define MS_MC_ADDR_MUX_TYPE_SHIFT		(28)
#define MS_MC_ADDR_MUX_TYPE_MASK			(0x3 << 28)
#define MS_MC_TIME_MODE_SHIFT			(24)
#define MS_MC_TIME_MODE_MASK				(0x3 << 24)
#define MS_MC_BUS_ENDIAN_SHIFT			(16)
#define MS_MC_BUS_ENDIAN_MASK			(0x1 << 16)
#define MS_MC_BUS_DATA_WIDTH_SHIFT		(12)
#define MS_MC_BUS_DATA_WIDTH_MASK		(0x3 << 12)
#define MS_MC_BUS_ADDR_OFFSET_SHIFT		(8)
#define MS_MC_BUS_ADDR_OFFSET_MASK		(0x1 << 8)
#define MS_MC_RD_SYNC_TYPE_SHIFT			(4)
#define MS_MC_RD_SYNC_TYPE_MASK			(0x1 << 4)
#define MS_MC_WR_SYNC_TYPE_SHIFT			(0)
#define MS_MC_WR_SYNC_TYPE_MASK			(0x1 << 0)

// LBC_TIMESCALE_CTRL_REG	(0x004)	TSC
#define MS_TSC_CLK_DIVIDER_SHIFT			(8)
#define MS_TSC_CLK_DIVIDER_MASK				(0x3 << 8)
#define MS_TSC_CLK_DELAY_TIME_SHIFT			(0)
#define MS_TSC_CLK_DELAY_TIME_MASK			(0x7 << 0)

// LBC_READY_CTRL_REG (0x008) RDYC
#define MS_RDYC_READY_TIMEOUT_TIME_SHIFT	(16)
#define MS_RDYC_READY_TIMEOUT_TIME_MASK	(0xFFFF << 16)
#define MS_RDYC_RD_READY_EN_SHIFT			(12)
#define MS_RDYC_RD_READY_EN_MASK			(0x1 << 12)
#define MS_RDYC_WR_READY_EN_SHIFT			(8)
#define MS_RDYC_WR_READY_EN_MASK			(0x1 << 8)
#define MS_RDYC_READY_DELAY_TIME_SHIFT	(6)
#define MS_RDYC_READY_DELAY_TIME_MASK		(0x3 << 6)
#define MS_RDYC_RD_READY_MODE_SHIFT		(5)
#define MS_RDYC_RD_READY_MODE_MASK		(0x1 << 5)
#define MS_RDYC_WR_READY_MODE_SHIFT		(4)
#define MS_RDYC_WR_READY_MODE_MASK		(0x1 << 4)
#define MS_RDYC_READY_POLARITY_SHIFT		(0)
#define MS_RDYC_READY_POLARITY_MASK		(0x1 << 0)

// LBC_DP_CTRL_REG	(0x00C) DPC
#define MS_DPC_DP_MODE_SHIFT				(12)
#define MS_DPC_DP_MODE_MASK					(0x1 << 12)
#define MS_DPC_DP_PARITY_SHIFT				(8)
#define MS_DPC_DP_PARITY_MASK				(0X1 << 8)
#define MS_DPC_DP_EN_SHIFT					(0)
#define MS_DPC_DP_EN_MASK					(0x1 << 0)

// LBC_BE_CTRL_REG	(0x010) BEC
#define MS_BEC_BE_PARITY_EN_SHIFT			(16)
#define MS_BEC_BE_PARITY_EN_MASK			(0x1 << 16)
#define MS_BEC_DE_POLARITY_SHIFT			(8)
#define MS_BEC_DE_POLARITY_MASK				(0X1 << 8)
#define MS_BEC_BE_POLARITY_SHIFT			(0)
#define MS_BEC_BE_POLARITY_MASK				(0X1 << 0)


// LBC_WAIT_CTRL_REG	(0x014) WTC
#define MS_WTC_WAIT_TIMEOUT_TIME_SHIFT		(16)
#define MS_WTC_WAIT_TIMEOUT_TIME_MASK		(0xFFFF << 16)
#define MS_WTC_WAIT_POLARITY_SHIFT			(8)
#define MS_WTC_WAIT_POLARITY_MASK			(0X1 << 8)
#define MS_WTC_WAIT_EN_SHIFT				(0)
#define MS_WTC_WAIT_EN_MASK					(0x1 << 0)


// LBC_CS_TIMING_CTRL_REG	(0x018) CSTC
#define MS_CSTC_CS_RDOFF_TIME_SHIFT			(24)
#define MS_CSTC_CS_RDOFF_TIME_MASK			(0xFF << 24)
#define MS_CSTC_CS_WROFF_TIME_SHIFT			(16)
#define MS_CSTC_CS_WROFF_TIME_MASK			(0xFF << 16)
#define MS_CSTC_CS_ON_TIME_SHIFT			(8)
#define MS_CSTC_CS_ON_TIME_MASK				(0xFF << 8)
#define MS_CSTC_CS_POLARITY_SHIFT			(0)
#define MS_CSTC_CS_POLARITY_MASK			(0x1 << 0)

// LBC_ALE_TIMING0_CTRL_REG	(0x01C) ALET0C
#define MS_ALET0C_ALE_RDOFF_TIME_SHIFT		(24)
#define MS_ALET0C_ALE_RDOFF_TIME_MASK		(0xFF << 24)
#define MS_ALET0C_ALE_WROFF_TIME_SHIFT		(16)
#define MS_ALET0C_ALE_WROFF_TIME_MASK		(0xFF << 16)
#define MS_ALET0C_ALE_ON_TIME_SHIFT			(8)
#define MS_ALET0C_ALE_ON_TIME_MASK			(0xFF << 8)
#define MS_ALET0C_ALE_POLARITY_SHIFT		(0)
#define MS_ALET0C_ALE_POLARITY_MASK			(0X1 << 0)


// LBC_ALE_TIMING1_CTRL_REG	(0x020) ALET1C
#define MS_ALET1C_ALE_AAD_RDOFF_TIME_SHIFT	(24)
#define MS_ALET1C_ALE_AAD_RDOFF_TIME_MASK	(0xFF << 24)
#define MS_ALET1C_ALE_AAD_WROFF_TIME_SHIFT	(16)
#define MS_ALET1C_ALE_AAD_WROFF_TIME_MASK	(0xFF << 16)
#define MS_ALET1C_ALE_AAD_ON_TIME_SHIFT		(8)
#define MS_ALET1C_ALE_AAD_ON_TIME_MASK		(0xFF << 8)

// LBC_WE_TIMING_CTRL_REG	(0x024)	WETC
#define MS_WETC_WE_OFF_TIME_SHIFT			(16)
#define MS_WETC_WE_OFF_TIME_MASK			(0xFF << 16)
#define MS_WETC_WE_ON_TIME_SHIFT			(8)
#define MS_WETC_WE_ON_TIME_MASK				(0xFF << 8)
#define MS_WETC_WE_POLARITY_SHIFT			(0)
#define MS_WETC_WE_POLARITY_MASK			(0x1 << 0)

// LBC_OE_TIMING0_CTRL_REG	(0x028)	OET0C
#define MS_OET0C_OE_OFF_TIME_SHIFT			(16)
#define MS_OET0C_OE_OFF_TIME_MASK			(0xFF << 16)
#define MS_OET0C_OE_ON_TIME_SHIFT			(8)
#define MS_OET0C_OE_ON_TIME_MASK			(0xFF << 8)
#define MS_OET0C_OE_POLARITY_SHIFT			(0)
#define MS_OET0C_OE_POLARITY_MASK			(0x1 << 0)

// LBC_OE_TIMING1_CTRL_REG	(0x02C) OET1C
#define MS_OET1C_OE_AAD_OFF_TIME_SHIFT		(16)
#define MS_OET1C_OE_AAD_OFF_TIME_MASK		(0xFF << 16)
#define MS_OET1C_OE_AAD_ON_TIME_SHIFT		(8)
#define MS_OET1C_OE_AAD_ON_TIME_MASK		(0xFF << 8)

// LBC_OE_TIMING2_CTRL_REG	(0x030) OET2C
#define MS_OET2C_OE_PAGE_TIME_SHIFT			(8)
#define MS_OET2C_OE_PAGE_TIME_MASK			(0xFF << 8)
#define MS_OET2C_OE_PAGE_EN_SHIFT			(0)
#define MS_OET2C_OE_PAGE_EN_MASK			(0x1 << 0)

// LBC_CYCLE_TIMING_CTRL_REG	(0x034) CYCTC
#define MS_CYCTC_RD_CYCLE_TIME_SHIFT		(16)
#define MS_CYCTC_RD_CYCLE_TIME_MASK			(0xFFFF << 16)
#define MS_CYCTC_WR_CYCLE_TIME_SHIFT		(0)
#define MS_CYCTC_WR_CYCLE_TIME_MASK			(0xFFFF << 0)

// LBC_ACCESS_TIMING_CTRL_REG	(0x038)	ATC
#define MS_ATC_RD_ACCESS_TIME_SHIFT			(16)
#define MS_ATC_RD_ACCESS_TIME_MASK			(0xFFFF << 16)
#define MS_ATC_WR_ACCESS_TIME_SHIFT			(0)
#define MS_ATC_WR_ACCESS_TIME_MASK			(0xFFFF << 0)

// LBC_PAGE_TIMING_CTRL_REG	(0x03C)	PGTC
#define	MS_PGTC_PAGE_ACCESS_TIME_SHIFT		(0)
#define MS_PGTC_PAGE_ACCESS_TIME_MASK		(0xFF << 0)

// LBC_DATAMUX_TIMING_CTRL_REG (0x040)	DATTC
#define MS_DATTC_WR_DATA_ON_TIME_SHIFT		(0)
#define MS_DATTC_WR_DATA_ON_TIME_MASK		(0xFFFF << 0)

// LBC_CLE_TIMING_CTRL_REG		(0x044) CLETC
#define MS_CLETC_CLE_OFF_TIME_SHIFT			(16)
#define MS_CLETC_CLE_OFF_TIME_MASK			(0xFF << 16)
#define MS_CLETC_CLE_ON_TIME_SHIFT			(8)
#define MS_CLETC_CLE_ON_TIME_MASK			(0xFF << 8)
#define MS_CLETC_CLE_MODE_SHIFT				(4)
#define MS_CLETC_CLE_MODE_MASK				(0x1 << 4)
#define MS_CLETC_CLE_POLARITY_SHIFT			(0)
#define MS_CLETC_CLE_POLARITY_MASK			(0x1 << 0)

// LBC_SPECIAL_TIMING0_CTRL_REG (0x048)	ST0C
#define MS_ST0C_WE_PAGE_TIME_SHIFT			(8)
#define MS_ST0C_WE_PAGE_TIME_MASK			(0xFF << 8)
#define MS_ST0C_WE_PAGE_EN_SHIFT			(0)
#define MS_ST0C_WE_PAGE_EN_MASK				(0x1 << 0)

// LBC_SPECIAL_TIMING1_CTRL_REG (0x04C) ST1C
#define MS_ST1C_WE_FOR_READ_EN_SHIFT		(0)
#define MS_ST1C_WE_FOR_READ_EN_MASK			(0x1 << 0)

// LBC_SPECIAL_TIMING2_CTRL_REG (0x050) ST2C
#define MS_ST2C_LCD_DATA_TYPE_SHIFT			(24)
#define MS_ST2C_LCD_DATA_TYPE_MASK			(0x3 << 24)
#define MS_ST2C_LCD_MODE_EN_SHIFT			(16)
#define MS_ST2C_LCD_MODE_EN_MASK			(0x1 << 16)
#define MS_ST2C_NAND_DATA_TYPE_SHIFT		(8)
#define MS_ST2C_NAND_DATA_TYPE_MASK			(0x3 << 8)
#define MS_ST2C_NAND_MODE_EN_SHIFT			(0)
#define MS_ST2C_NAND_MODE_EN_MASK			(0x1 << 0)

// LBC_TURNAROUND_CTRL_REG		(0x060) TRC
#define MS_TRC_TRUNAROUND_TIME_SHIFT		(0)
#define MS_TRC_TRUNAROUND_TIME_MASK			(0xFF << 0)

// LBC_WAIT_DATA_NUM_REG		(0x070) WDN
#define MS_WDN_DMA_WAIT_DATA_NUM_SHIFT		(20)
#define MS_WDN_DMA_WAIT_DATA_NUM_MASK		(0x1FF << 20)
#define MS_WDN_IDA_WAIT_DATA_NUM_SHIFT		(8)
#define MS_WDN_IDA_WAIT_DATA_NUM_MASK		(0x1FF << 8)
#define MS_WDN_DA_WAIT_DATA_NUM_SHIFT		(0)
#define MS_WDN_DA_WAIT_DATA_NUM_MASK		(0x3F << 0)


// CH_MODE_CTRL_REG		(0x200) CHMC
#define MS_CHMC_USER_SEL_EN_SHIFT			(16)
#define MS_CHMC_USER_SEL_EN_MASK			(0x1 << 16)
#define MS_CHMC_FIX_SEL_EN_SHIFT			(8)
#define MS_CHMC_FIX_SEL_EN_MASK				(0x1 << 8)
#define MS_CHMC_ADDR_SEL_EN_SHIFT			(0)
#define MS_CHMC_ADDR_SEL_EN_MASK			(0x1 << 0)


// CS_HLADDR_OFFSET		(0x10)
// CS0_DA_HADDR_REG		(0x210) CS0DAHA
#define MS_CS0DAHA_CS0_DA_HADDR_SHIFT	(0)
#define MS_CS0DAHA_CS0_DA_HADDR_MASK		(0xFFFFFFFF << 0)

// CS0_DA_LADDR_REG		(0x214)	CS0DALA
#define MS_CS0DALA_CS0_DA_LADDR_SHIFT	(0)
#define MS_CS0DALA_CS0_DA_LADDR_MASK		(0xFFFFFFFF << 0)

// CS0_IDA_HADDR_REG		(0x218)	CS0IDAHA
#define MS_CS0IDAHA_CS0_IDA_HADDR_SHIFT	(0)
#define MS_CS0IDAHA_CS0_IDA_HADDR_MASK		(0xFFFFFFFF << 0)

// CS0_IDA_LADDR_REG		(0x21C)	CS0IDALA
#define MS_CS0IDALA_CS0_IDA_LADDR_SHIFT	(0)
#define MS_CS0IDALA_CS0_IDA_LADDR_MASK		(0xFFFFFFFF << 0)

// CS1_DA_HADDR_REG		(0x220)		CS1DAHA
#define MS_CS1DAHA_CS1_DA_HADDR_SHIFT	(0)
#define MS_CS1DAHA_CS1_DA_HADDR_MASK		(0xFFFFFFFF << 0)

// CS1_DA_LADDR_REG		(0x224)		CS1DALA
#define MS_CS1DALA_CS1_DA_LADDR_SHIFT	(0)
#define MS_CS1DALA_CS1_DA_LADDR_MASK		(0xFFFFFFFF << 0)

// CS1_IDA_HADDR_REG		(0x228)	CS1IDAHA
#define MS_CS1IDAHA_CS1_IDA_HADDR_SHIFT	(0)
#define MS_CS1IDAHA_CS1_IDA_HADDR_MASK		(0xFFFFFFFF << 0)

// CS1_IDA_LADDR_REG		(0x22C)	CS1IDALA
#define MS_CS1IDALA_CS1_IDA_LADDR_SHIFT	(0)
#define MS_CS1IDALA_CS1_IDA_LADDR_MASK		(0xFFFFFFFF << 0)

// CS2_DA_HADDR_REG		(0x230)		CS2DAHA
#define MS_CS2DAHA_CS2_DA_HADDR_SHIFT	(0)
#define MS_CS2DAHA_CS2_DA_HADDR_MASK		(0xFFFFFFFF << 0)

// CS2_DA_LADDR_REG		(0x234)		CS2DALA
#define MS_CS2DALA_CS2_DA_LADDR_SHIFT	(0)
#define MS_CS2DALA_CS2_DA_LADDR_MASK		(0xFFFFFFFF << 0)

// CS2_IDA_HADDR_REG		(0x238) CS2IDAHA
#define MS_CS2IDAHA_CS2_IDA_HADDR_SHIFT	(0)
#define MS_CS2IDAHA_CS2_IDA_HADDR_MASK		(0xFFFFFFFF << 0)

// CS2_IDA_LADDR_REG		(0x23C) CS2IDALA
#define MS_CS2IDALA_CS2_IDA_LADDR_SHIFT	(0)
#define MS_CS2IDALA_CS2_IDA_LADDR_MASK		(0xFFFFFFFF << 0)

// CS3_DA_HADDR_REG		(0x240)		CS3DAHA
#define MS_CS3DAHA_CS3_DA_HADDR_SHIFT	(0)
#define MS_CS3DAHA_CS3_DA_HADDR_MASK		(0xFFFFFFFF << 0)

// CS3_DA_LADDR_REG		(0x244)		CS3DALA
#define MS_CS3DALA_CS3_DA_LADDR_SHIFT	(0)
#define MS_CS3DALA_CS3_DA_LADDR_MASK		(0xFFFFFFFF << 0)

// CS3_IDA_HADDR_REG		(0x248)	CS3IDAHA
#define MS_CS3IDAHA_CS3_IDA_HADDR_SHIFT	(0)
#define MS_CS3IDAHA_CS3_IDA_HADDR_MASK		(0xFFFFFFFF << 0)

// CS3_IDA_LADDR_REG		(0x24C)	CS3IDALA
#define MS_CS3IDALA_CS3_IDA_LADDR_SHIFT	(0)
#define MS_CS3IDALA_CS3_IDA_LADDR_MASK		(0xFFFFFFFF << 0)


// FIX_CS_REG				(0x250) FCS
#define MS_FCS_DMA_FIX_CS_SHIFT			(16)
#define MS_FCS_DMA_FIX_CS_MASK			(0x3 << 16)
#define MS_FCS_IDA_FIX_CS_SHIFT			(8)
#define MS_FCS_IDA_FIX_CS_MASK			(0x3 << 8)
#define MS_FCS_DA_FIX_CS_SHIFT			(0)
#define MS_FCS_DA_FIX_CS_MASK			(0x3 << 8)

// USER_CS_REG				(0x254) UCS
#define MS_UCS_CS3_USER_SEL_SHIFT		(24)
#define MS_UCS_CS3_USER_SEL_MASK		(0x7 << 24)
#define MS_UCS_CS2_USER_SEL_SHIFT		(16)
#define MS_UCS_CS2_USER_SEL_MASK		(0x7 << 16)
#define MS_UCS_CS1_USER_SEL_SHIFT		(8)
#define MS_UCS_CS1_USER_SEL_MASK		(0x7 << 8)
#define MS_UCS_CS0_USER_SEL_SHIFT		(0)
#define MS_UCS_CS0_USER_SEL_MASK		(0x7 << 0)


// LBC_CLK_REG				(0x280) CLK
#define MS_CLK_CLK_MODE_SHIFT			(16)
#define MS_CLK_CLK_MODE_MASK			(0x1 << 16)
#define MS_CLK_RX_NEGATIVE_SEL_SHIFT	(3)
#define MS_CLK_RX_NEGATIVE_SEL_MASK		(0x1 << 3)
#define MS_CLK_TX_NEGATIVE_SEL_SHIFT	(2)
#define MS_CLK_TX_NEGATIVE_SEL_MASK		(0x1 << 2)
#define MS_CLK_FCLK_SEL_SHIFT			(1)
#define MS_CLK_FCLK_SEL_MASK			(0x1 << 1)
#define MS_CLK_FCLK_GATE_SHIFT			(0)
#define MS_CLK_FCLK_GATE_MASK			(0x1 << 0)

// LBC_SOFT_RST_REG		(0x288) SRST
#define MS_SRST_MASTER_SRST_SHIFT		(5)
#define MS_SRST_MASTER_SRST_MASK		(0x1 << 5)
#define MS_SRST_DATACTRL_SRST_SHIFT		(4)
#define MS_SRST_DATACTRL_SRST_MASK		(0x1 << 4)
#define MS_SRST_ARB_SRST_SHIFT			(3)
#define MS_SRST_ARB_SRST_MASK			(0x1 << 3)
#define MS_SRST_DMA_SRST_SHIFT			(2)
#define MS_SRST_DMA_SRST_MASK			(0x1 << 2)
#define MS_SRST_IDA_SRST_SHIFT			(1)
#define MS_SRST_IDS_SRST_MASK			(0x1 << 1)
#define MS_SRST_DA_SRST_SHIFT			(0)
#define MS_SRST_DA_SRST_MASK			(0x1 << 0)

// SMP_DELAY_CTRL_REG		(0x290) SDC
#define MS_SDC_SAMP_DL_BYPASS_SHIFT		(16)
#define MS_SDC_SAMP_DL_BYPASS_MASK		(0x1 << 16)
#define MS_SDC_SAMP_DL_CAL_START_SHIFT	(15)
#define MS_SDC_SAMP_DL_CAL_START_MASK	(0x1 << 15)
#define MS_SDC_SAMP_DL_CAL_DONE_SHIFT	(14)
#define MS_SDC_SAMP_DL_CAL_DONE_MASK	(0x1 << 14)
#define MS_SDC_SAMP_DL_CAL_VALUE_SHIFT	(8)
#define MS_SDC_SAMP_DL_CAL_VALUE_MASK	(0x3F << 8)
#define MS_SDC_SAMP_DL_CAL_SW_EN_SHIFT	(7)
#define MS_SDC_SAMP_DL_CAL_SW_EN_MASK	(0x1 << 7)
#define MS_SDC_SAMP_DL_SW_VALUE_SHIFT	(0)
#define MS_SDC_SAMP_DL_SW_VALUE_MASK	(0x3F << 0)

// IDA_TS_CTRL_REG			(0x300) IDATC
#define MS_IDATC_IDA_BURST_ARB_GRAIN_SHIFT	(16)
#define MS_IDATC_IDA_BURST_ARB_GRAIN_MASK	(0x3 << 16)
#define MS_IDATC_IDA_BURST_TYPE_SHIFT		(15)
#define MS_IDATC_IDA_BURST_TYPE_MASK			(0x1 << 15)
#define MS_IDATC_IDA_BURST_LENGTH_SHIFT		(8)
#define MS_IDATC_IDA_BURST_LENGTH_MASK		(0x7F << 8)
#define MS_IDATC_IDA_DMA_ON_SHIFT			(2)
#define MS_IDATC_IDA_DMA_ON_MASK				(0x1 << 2)
#define MS_IDATC_IDA_TS_DIR_SHIFT			(1)
#define MS_IDATC_IDA_TS_DIR_MASK				(0x1 << 1)
#define MS_IDATC_IDA_TS_START_SHIFT			(0)
#define MS_IDATC_IDA_TS_START_MASK			(0x1 << 0)

// IDA_TS_ADDR_REG			(0x304)
// IDA_TS_DATA_LEN_REG		(0x308) IDATDL
#define MS_IDATDL_IDA_TS_DATA_LEN_SHIFT		(0)
#define MS_IDATDL_IDA_TS_DATA_LEN_MASK		(0xFFFF << 0)

// IDA_WDATA_REG			(0x340)
// IDA_RDATA_REG			(0x350)
// DA_BST_SEL_REG			(0x400) DABSTS
#define MS_DABSTS_DA_CS3_BURST_SEL_SHIFT		(3)
#define MS_DABSTS_DA_CS3_BURST_SEL_MASK		(0x1 << 3)
#define MS_DABSTS_DA_CS2_BURST_SEL_SHIFT		(2)
#define MS_DABSTS_DA_CS2_BURST_SEL_MASK		(0x1 << 2)
#define MS_DABSTS_DA_CS1_BURST_SEL_SHIFT		(1)
#define MS_DABSTS_DA_CS1_BURST_SEL_MASK		(0x1 << 1)
#define MS_DABSTS_DA_CS0_BURST_SEL_SHIFT		(0)
#define MS_DABSTS_DA_CS0_BURST_SEL_MASK		(0x1 << 0)

// DA_BST_TYPE_REG	(0x404) DABSTT
#define MS_DABSTT_DA_CS3_BURST_TYPE_SHIFT		(31)
#define MS_DABSTT_DA_CS3_BURST_TYPE_MASK		(0x1 << 31)
#define MS_DABSTT_DA_CS3_BURST_LENGTH_SHIFT	(24)
#define MS_DABSTT_DA_CS3_BURST_LENGTH_MASK	(0x7F << 24)
#define MS_DABSTT_DA_CS2_BURST_TYPE_SHIFT		(23)
#define MS_DABSTT_DA_CS2_BURST_TYPE_MASK		(0x1 << 23)
#define MS_DABSTT_DA_CS2_BURST_LENGTH_SHIFT	(16)
#define MS_DABSTT_DA_CS2_BURST_LENGTH_MASK	(0x7F << 16)
#define MS_DABSTT_DA_CS1_BURST_TYPE_SHIFT		(15)
#define MS_DABSTT_DA_CS1_BURST_TYPE_MASK		(0x1 << 15)
#define MS_DABSTT_DA_CS1_BURST_LENGTH_SHIFT	(8)
#define MS_DABSTT_DA_CS1_BURST_LENGTH_MASK	(0x7F << 8)
#define MS_DABSTT_DA_CS0_BURST_TYPE_SHIFT		(7)
#define MS_DABSTT_DA_CS0_BURST_TYPE_MASK		(0x1 << 7)
#define MS_DABSTT_DA_CS0_BURST_LENGTH_SHIFT	(0)
#define MS_DABSTT_DA_CS0_BURST_LENGTH_MASK	(0x7F << 0)


// DA_CS0_SADDR_REG		(0x410)
// DA_CS1_SADDR_REG		(0x414)
// DA_CS2_SADDR_REG		(0x418)
// DA_CS3_SADDR_REG		(0x41C)
// DA_TX_TO_FIX_DONE_REG	(0x420) DATXTFD
#define MS_DATXTFD_DA_RX_TO_FIX_DONE_SHIFT	(0)
#define MS_DATXTFD_DA_RX_TO_FIX_DONE_MASK		(0x1 << 0)

// LBC_TIMEOUT_CLR_REG		(0x424)	TOC
#define MS_TOC_DMA_TIMEOUT_CLR_SHIFT		(2)
#define MS_TOC_DMA_TIMEOUT_CLR_MASK			(0x1 << 2)
#define MS_TOC_IDA_TIMEOUT_CLR_SHIFT		(1)
#define MS_TOC_IDA_TIMEOUT_CLR_MASK			(0x1 << 1)
#define MS_TOC_DA_TIMEOUT_CLR_SHIFT			(0)
#define MS_TOC_DA_TIMEOUT_CLR_MASK			(0x1 << 0)

// LBC_ARB_PRI_CFG_REG		(0x440) PRIC
#define MS_PRIC_DMA_PRIORITY_SHIFT			(8)
#define MS_PRIC_DMA_PRIORITY_MASK			(0x3 << 8)
#define MS_PRIC_IDA_PRIORITY_SHIFT			(4)
#define MS_PRIC_IDA_PRIORITY_MASK			(0x3 << 4)
#define MS_PRIC_DA_PRIORITY_SHIFT			(0)
#define MS_PRIC_DA_PRIORITY_MASK				(0x3 << 0)


// DMA_MODE_CFG_REG		(0x460) DMAMC
#define MS_DMAMC_DMA_BURST_ARB_GRAIN_SHIFT	(4)
#define MS_DMAMC_DMA_BURST_ARB_GRAIN_MASK	(0x3 << 4)
#define MS_DMAMC_DMA_DRQ_EN_SHIFT			(1)
#define MS_DMAMC_DMA_DRQ_EN_MASK				(0x1 << 1)
#define MS_DMAMC_DMA_MODE_ON_DMCR_SHIFT		(0)
#define MS_DMAMC_DMA_MODE_ON_DMCR_MASK		(0x1 << 0)

// DMA_DES_ADDR_REG		(0x464)
// DMA_DES0_DBG_REG		(0x4D0)
// DMA_DES1_DBG_REG		(0x4D4)
// DMA_DES2_DBG_REG		(0x4D8)
// DMA_DES3_DBG_REG		(0x4DC)
// LBC_INT0_EN_REG			(0x500) IT0E
#define MS_IT0E_IDA_RX_TRIG_EN_SHIFT			(31)			// lbc IDA RX fifo waterline higher than trigger level interrupt
#define MS_IT0E_IDA_RX_TRIG_EN_MASK			(0x1 << 31)		// lbc IDA RX fifo waterline higher than trigger level interrupt
#define MS_IT0E_IDA_TX_TRIG_EN_SHIFT			(30)			// lbc IDA TX fifo waterline lower than trigger level interrupt
#define MS_IT0E_IDA_TX_TRIG_EN_MASK			(0x1 << 30)		// lbc IDA TX fifo waterline lower than trigger level interrupt

#define MS_IT0E_DA_RX_TRIG_EN_SHIFT			(29)			// lbc DA RX fifo waterline higher than trigger level interrupt
#define MS_IT0E_DA_RX_TRIG_EN_MASK			(0x1 << 29)		// lbc DA RX fifo waterline higher than trigger level interrupt
#define MS_IT0E_DA_TX_TRIG_EN_SHIFT			(28)			// lbc DA TX fifo waterline lower than trigger level interrupt
#define MS_IT0E_DA_TX_TRIG_EN_MASK			(0x1 << 28)		// lbc DA TX fifo waterline lower than trigger level interrupt

#define MS_IT0E_DP_EN_SHIFT					(7)				// lbc data parity interrupt
#define MS_IT0E_DP_EN_MASK					(0x1 << 7)		// lbc data parity interrupt
#define MS_IT0E_ODP_EN_SHIFT					(6)				// lbc one-bit data parity interrupt
#define MS_IT0E_ODP_EN_MASK					(0x1 << 6)		// lbc one-bit data parity interrupt

#define MS_IT0E_DMA_DES_INVLD_EN_SHIFT		(5)				// lbc DMA descriptor invld interrupt
#define MS_IT0E_DMA_DES_INVLD_EN_MASK		(0x1 << 5)		// lbc DMA descriptor invld interrupt
#define MS_IT0E_DMA_TRSF_DONE_EN_SHIFT		(4)				// lbc DMA transfer done interrupt
#define MS_IT0E_DMA_TRSF_DONE_EN_MASK		(0x1 << 4)		// lbc DMA transfer done interrupt
#define MS_IT0E_IDA_TRSF_DONE_EN_SHIFT		(3)				// lbc IDA transfer done interrupt
#define MS_IT0E_IDA_TRSF_DONE_EN_MASK		(0x1 << 3)		// lbc IDA transfer done interrupt
#define MS_IT0E_DA_TRSF_DONE_EN_SHIFT		(2)				// lbc DA transfer done interrupt
#define MS_IT0E_DA_TRSF_DONE_EN_MASK			(0x1 << 2)		// lbc DA transfer done interrupt
#define MS_IT0E_READY_TIMEOUT_EN_SHIFT		(1)				// lbc ready timeout interrupt
#define MS_IT0E_READY_TIMEOUT_EN_MASK		(0x1 << 1)		// lbc ready timeout interrupt
#define MS_IT0E_CMD_DONE_EN_SHIFT			(0)				// lbc transfer done interrupt
#define MS_IT0E_CMD_DONE_EN_MASK				(0x1 << 0)		// lbc transfer done interrupt


// LBC_INT1_EN_REG			(0x504) IT1E
#define MS_IT1E_DMA_TIMEOUT_EN_SHIFT			(6)				// lbc DMA timeout interrupt
#define MS_IT1E_DMA_TIMEOUT_EN_MASK			(0x1 << 6)		// lbc DMA timeout interrupt
#define MS_IT1E_IDA_TIMEOUT_EN_SHIFT			(5)				// lbc IDA timeout interrupt
#define MS_IT1E_IDA_TIMEOUT_EN_MASK			(0x1 << 5)		// lbc IDA timeout interrupt
#define MS_IT1E_DA_TIMEOUT_EN_SHIFT			(4)				// lbc DA timeout interrupt
#define MS_IT1E_DA_TIMEOUT_EN_MASK			(0x1 << 4)		// lbc DA timeout interrupt
#define MS_IT1E_DMA_DES_DONE_EN_SHIFT		(3)				// lbc DMA descriptor done interrupt
#define MS_IT1E_DMA_DES_DONE_EN_MASK			(0x1 << 3)		// lbc DMA descriptor done interrupt
#define MS_IT1E_DATA_REQ_TIMEOUT_EN_SHIFT	(2)				// lbc data req timeout interrupt
#define MS_IT1E_DATA_REQ_TIMEOUT_EN_MASK		(0x1 << 2)		// lbc data req timeout interrupt
#define MS_IT1E_DMA_RX_TRIG_EN_SHIFT			(1)				// lbc DMA RX fifo waterline higher than trigger level interrupt
#define MS_IT1E_DMA_RX_TRIG_EN_MASK			(0x1 << 1)		// lbc DMA RX fifo waterline higher than trigger level interrupt
#define MS_IT1E_DMA_TX_TRIG_EN_SHIFT			(0)				// lbc DMA TX fifo waterline lower than trigger level interrupt
#define MS_IT1E_DMA_TX_TRIG_EN_MASK			(0x1 << 0)		// lbc DMA TX fifo waterline lower than trigger level interrupt



// LBC_INT0_PENDING_REG	(0x508) IT0P
#define MS_IT0P_IDA_RX_TRIG_IRPR_SHIFT		(31)	// lbc IDA RX fifo waterline higher than trigger level interrupt
#define MS_IT0P_IDA_RX_TRIG_IRPR_MASK		(0x1 << 31)	// lbc IDA RX fifo waterline higher than trigger level interrupt
#define MS_IT0P_IDA_TX_TRIG_IRPR_SHIFT		(30)	// lbc IDA TX fifo waterline lower than trigger level interrupt
#define MS_IT0P_IDA_TX_TRIG_IRPR_MASK		(0x1 << 30)	// lbc IDA TX fifo waterline lower than trigger level interrupt

#define MS_IT0P_DA_RX_TRIG_IRPR_SHIFT		(29)	// lbc DA RX fifo waterline higher than trigger level interrupt
#define MS_IT0P_DA_RX_TRIG_IRPR_MASK			(0x1 << 29)	// lbc DA RX fifo waterline higher than trigger level interrupt
#define MS_IT0P_DA_TX_TRIG_IRPR_SHIFT		(28)	// lbc DA TX fifo waterline lower than trigger level interrupt
#define MS_IT0P_DA_TX_TRIG_IRPR_MASK			(0x1 << 28)	// lbc DA TX fifo waterline lower than trigger level interrupt

#define MS_IT0P_DP_IRPR_SHIFT				(7)	// lbc data parity interrupt
#define MS_IT0P_DP_IRPR_MASK					(0x1 << 7)	// lbc data parity interrupt
#define MS_IT0P_ODP_IRPR_SHIFT				(6)	// lbc one-bit data parity interrupt
#define MS_IT0P_ODP_IRPR_MASK				(0x1 << 6)	// lbc one-bit data parity interrupt

#define MS_IT0P_DMA_DES_INVLD_IRPR_SHIFT		(5)	// lbc DMA descriptor invld interrupt
#define MS_IT0P_DMA_DES_INVLD_IRPR_MASK		(0x1 << 5)	// lbc DMA descriptor invld interrupt
#define MS_IT0P_DMA_TRSF_DONE_IRPR_SHIFT		(4)	// lbc DMA transfer done interrupt
#define MS_IT0P_DMA_TRSF_DONE_IRPR_MASK		(0x1 << 4)	// lbc DMA transfer done interrupt
#define MS_IT0P_IDA_TRSF_DONE_IRPR_SHIFT		(3)	// lbc IDA transfer done interrupt
#define MS_IT0P_IDA_TRSF_DONE_IRPR_MASK		(0x1 << 3)	// lbc IDA transfer done interrupt
#define MS_IT0P_DA_TRSF_DONE_IRPR_SHIFT		(2)	// lbc DA transfer done interrupt
#define MS_IT0P_DA_TRSF_DONE_IRPR_MASK		(0x1 << 2)	// lbc DA transfer done interrupt
#define MS_IT0P_READY_TIMEOUT_IRPR_SHIFT		(1)	// lbc ready timeout interrupt
#define MS_IT0P_READY_TIMEOUT_IRPR_MASK		(0x1 << 1)	// lbc ready timeout interrupt
#define MS_IT0P_CMD_DONE_IRPR_SHIFT			(0)	// lbc transfer done interrupt
#define MS_IT0P_CMD_DONE_IRPR_MASK			(0x1 << 0)	// lbc transfer done interrupt

#define MS_IT0P_OVER_UNDER_FLOW_IRPR_SHIFT	(0x0FFFFF00)

// LBC_INT1_PENDING_REG	(0x50C) IT1P
#define MS_IT1P_DMA_TIMEOUT_IRPR_SHIFT		(6)	// lbc DMA timeout interrupt
#define MS_IT1P_DMA_TIMEOUT_IRPR_MASK		(0x1 << 6)	// lbc DMA timeout interrupt
#define MS_IT1P_IDA_TIMEOUT_IRPR_SHIFT		(5)	// lbc IDA timeout interrupt
#define MS_IT1P_IDA_TIMEOUT_IRPR_MASK		(0x1 << 5)	// lbc IDA timeout interrupt
#define MS_IT1P_DA_TIMEOUT_IRPR_SHIFT		(4)	// lbc DA timeout interrupt
#define MS_IT1P_DA_TIMEOUT_IRPR_MASK		(0x1 << 4)	// lbc DA timeout interrupt
#define MS_IT1P_DMA_DES_DONE_IRPR_SHIFT		(3)	// lbc DMA descriptor done interrupt
#define MS_IT1P_DMA_DES_DONE_IRPR_MASK		(0x1 << 3)	// lbc DMA descriptor done interrupt
#define MS_IT1P_DATA_REQ_TIMEOUT_IRPR_SHIFT	(2)	//lbc data req timeout interrupt
#define MS_IT1P_DATA_REQ_TIMEOUT_IRPR_MASK	(0x1 << 2)	//lbc data req timeout interrupt
#define MS_IT1P_DMA_RX_TRIG_IRPR_SHIFT		(1)	// lbc DMA RX fifo waterline higher than trigger level interrupt
#define MS_IT1P_DMA_RX_TRIG_IRPR_MASK		(0x1 << 1)	// lbc DMA RX fifo waterline higher than trigger level interrupt
#define MS_IT1P_DMA_TX_TRIG_IRPR_SHIFT		(0)	// lbc DMA TX fifo waterline lower than trigger level interrupt
#define MS_IT1P_DMA_TX_TRIG_IRPR_MASK		(0x1 << 0)	// lbc DMA TX fifo waterline lower than trigger level interrupt


// DA_INTF_STA_REG			(0x510) DAIS
#define MS_DAIS_DA_ORDER_FIFO_WL_SHIFT		(20)
#define MS_DAIS_DA_ORDER_FIFO_WL_MASK		(0x1F << 20)
#define MS_DAIS_DA_ORDER_FIFO_DATA_SHIFT	(16)
#define MS_DAIS_DA_ORDER_FIFO_DATA_MASK		(0x1 << 16)
#define MS_DAIS_DA_RX_OT_FIFO_WL_SHIFT		(12)
#define MS_DAIS_DA_RX_OT_FIFO_WL_MASK		(0x7 << 12)
#define MS_DAIS_DA_TX_OT_FIFO_WL_SHIFT		(8)
#define MS_DAIS_DA_TX_OT_FIFO_WL_MASK		(0x7 << 8)
#define MS_DAIS_DA_RX_LOAD_FIFO_WL_SHIFT	(4)
#define MS_DAIS_DA_RX_LOAD_FIFO_WL_MASK		(0xF << 4)
#define MS_DAIS_DA_TX_LOAD_FIFO_WL_SHIFT	(0)
#define MS_DAIS_DA_TX_LOAD_FIFO_WL_MASK		(0xF << 0)

// DA_FIFO_STA_REG			(0x514) DAFS
#define MS_DAFS_DA_RX_FIFO_WL_SHIFT			(16)
#define MS_DAFS_DA_RX_FIFO_WL_MASK			(0x7F << 16)
#define MS_DAFS_DA_TX_FIFO_WL_SHIFT			(8)
#define MS_DAFS_DA_TX_FIFO_WL_MASK			(0x1F << 8)
#define MS_DAFS_DA_CMD_FIFO_WL_SHIFT		(0)
#define MS_DAFS_DA_CMD_FIFO_WL_MASK			(0x7 << 0)

// IDA_FIFO_STA_REG		(0x518) IDAFS
#define MS_IDAFS_IDA_RX_FIFO_WL_SHIFT		(16)
#define MS_IDAFS_IDA_RX_FIFO_WL_MASK		(0x1FF << 16)
#define MS_IDAFS_IDA_TX_FIFO_WL_SHIFT		(4)
#define MS_IDAFS_IDA_TX_FIFO_WL_MASK		(0x1FF << 4)
#define MS_IDAFS_IDA_CMD_FIFO_WL_SHIFT		(0)
#define MS_IDAFS_IDA_CMD_FIFO_WL_MASK		(0x7 << 0)

// DMA_FIFO_STA_REG		(0x51C) DMAFS
#define MS_DMAFS_DMA_RX_FIFO_WL_SHIFT		(16)
#define MS_DMAFS_DMA_RX_FIFO_WL_MASK		(0xFF << 16)
#define MS_DMAFS_DMA_TX_FIFO_WL_SHIFT		(8)
#define MS_DMAFS_DMA_TX_FIFO_WL_MASK		(0xFF << 8)
#define MS_DMAFS_DMA_CMD_FIFO_WL_SHIFT		(0)
#define MS_DMAFS_DMA_CMD_FIFO_WL_MASK		(0x7 << 0)


// DA_FIFO_TRIGGER_LVL_REG	(0x524) DAFTL
#define MS_DAFTL_DA_RX_TRIGGER_LVL_SHIFT	(8)
#define MS_DAFTL_DA_RX_TRIGGER_LVL_MASK		(0x3F << 8)
#define MS_DAFTL_DA_TX_TRIGGER_LVL_SHIFT	(0)
#define MS_DAFTL_DA_TX_TRIGGER_LVL_MASK		(0xF << 0)

// IDA_FIFO_TRIGGER_LVL_REG (0x528) IDAFTL
#define MS_IDAFTL_IDA_RX_TRIGGER_LVL_SHIFT	(16)
#define MS_IDAFTL_IDA_RX_TRIGGER_LVL_MASK	(0xFF << 16)
#define MS_IDAFTL_IDA_TX_TRIGGER_LVL_SHIFT	(0)
#define MS_IDAFTL_IDA_TX_TRIGGER_LVL_MASK	(0xFF << 0)

// DMA_FIFO_TRIGGER_LVL_REG (0x52C) DMAFTL
#define MS_DMAFTL_DMA_RX_TRIGGER_LVL_SHIFT	(16)
#define MS_DMAFTL_DMA_RX_TRIGGER_LVL_MASK	(0x7F << 16)
#define MS_DMAFTL_DMA_TX_TRIGGER_LVL_SHIFT	(0)
#define MS_DMAFTL_DMA_TX_TRIGGER_LVL_MASK	(0x7F << 0)

// LBC_FIFO_CLR_REG		(0x530)
// LBC_IDA_MIRROR_REG		(0x600)
// LBC_DA_CMD0_REG			(0x604)
// LBC_DA_CMD1_REG			(0x608)
// LBC_CMD_BUSY_REG		(0x60C)
// LBC_CMD0_REG			(0x610)
// LBC_CMD1_REG			(0x614)
// LBC_CMD2_REG			(0x618)
// LBC_DA_TO_CMD0_REG		(0x620)
// LBC_DA_TO_CMD1_REG		(0x624)
// LBC_IDA_TO_CMD0_REG		(0x628)
// LBC_IDA_TO_CMD1_REG		(0x62C)
// LBC_DMA_TO_CMD0_REG		(0x630)
// LBC_DMA_TO_CMD1_REG		(0x634)



//=============================================================================
// mask and shift end
//=============================================================================


#if 1
// >>>>> interrupt 0 enable
#define IDA_RX_TRIG_EN		(1 << 31)	// lbc IDA RX fifo waterline higher than trigger level interrupt
#define IDA_TX_TRIG_EN		(1 << 30)	// lbc IDA TX fifo waterline lower than trigger level interrupt

#define DA_RX_TRIG_EN		(1 << 29)	// lbc DA RX fifo waterline higher than trigger level interrupt
#define DA_TX_TRIG_EN		(1 << 28)	// lbc DA TX fifo waterline lower than trigger level interrupt
#define	DA_TX_OTFIFO_OF_EN	(1 << 27)	// lbc DA TX outstanding fifo overflow interrupt
#define DA_RX_OTFIFO_OF_EN	(1 << 26)	// lbc DA RX ourstanding fifo overflow interrupt

#define DA_CMDFIFO_OF_EN	(1 << 25)	// lbc DA cmd fifo overflow interrupt
#define DA_CMDFIFO_UF_EN	(1 << 24)	// lbc DA cmd fifo underflow interrupt
#define DA_TXFIFO_OF_EN		(1 << 23)	// lbc DA TX data fifo overflow interrupt
#define DA_TXFIFO_UF_EN		(1 << 22)	// lbc DA TX data fifo underflow interrupt
#define DA_RXFIFO_OF_EN		(1 << 21)	// lbc DA RX data fifo overflow interrupt
#define DA_RXFIFO_UF_EN		(1 << 20)	// lbc DA RX data fifo underflow interrupt

#define IDA_CMDFIFO_OF_EN	(1 << 19)	// lbc IDA cmd fifo overflow interrupt
#define IDA_CMDFIFO_UF_EN	(1 << 18)	// lbc IDA cmd fifo underfow interrupt
#define IDA_TXFIFO_OF_EN	(1 << 17)	// lbc IDA TX data fifo overflow interrupt
#define IDA_TXFIFO_UF_EN	(1 << 16)	// lbc IDA TX data fifo underflow interrupt
#define IDA_RXFIFO_OF_EN	(1 << 15)	// lbc IDA RX data fifo overflow interrupt
#define IDA_RXFIFO_UF_EN	(1 << 14)	// lbc IDA RX data fifo underflow interrupt

#define DMA_CMDFIFO_OF_EN	(1 << 13)	// lbc DMA cmd fifo overflow interrupt
#define DMA_CMDFIFO_UF_EN	(1 << 12)	// lbc DMA cmd fifo underflow interrupt
#define DMA_TXFIFO_OF_EN	(1 << 11)	// lbc DMA TX data fifo overflow interrupt
#define DMA_TXFIFO_UF_EN	(1 << 10)	// lbc DMA TX data fifo underflow interrupt
#define DMA_RXFIFO_OF_EN	(1 << 9)	// lbc DMA RX data fifo overflow interrupt
#define DMA_RXFIFO_UF_EN	(1 << 8)	// lbc DMA RX data fifo underflow interrupt

#define DP_EN				(1 << 7)	// lbc data parity interrupt
#define ODP_EN				(1 << 6)	// lbc one-bit data parity interrupt

#define DMA_DES_INVLD_EN	(1 << 5)	// lbc DMA descriptor invld interrupt
#define DMA_TRSF_DONE_EN	(1 << 4)	// lbc DMA transfer done interrupt
#define IDA_TRSF_DONE_EN	(1 << 3)	// lbc IDA transfer done interrupt
#define DA_TRSF_DONE_EN		(1 << 2)	// lbc DA transfer done interrupt
#define READY_TIMEOUT_EN	(1 << 1)	// lbc ready timeout interrupt
#define CMD_DONE_EN			(1 << 0)	// lbc transfer done interrupt

// <<<<< interrupt 0 enable

// >>>>> interrupt 1 enable

#define DMA_TIMEOUT_EN		(1 << 6)	// lbc DMA timeout interrupt
#define IDA_TIMEOUT_EN		(1 << 5)	// lbc IDA timeout interrupt
#define DA_TIMEOUR_EN		(1 << 4)	// lbc DA timeout interrupt
#define DMA_DES_DONE_EN		(1 << 3)	// lbc DMA descriptor done interrupt
#define DATA_REQ_TIMEOUT_EN	(1 << 2)	//lbc data req timeout interrupt
#define DMA_RX_TRIG_EN		(1 << 1)	// lbc DMA RX fifo waterline higher than trigger level interrupt
#define DMA_TX_TRIG_EN		(1 << 0)	// lbc DMA TX fifo waterline lower than trigger level interrupt

// <<<<< interrupt 1 enable

// >>>>> interrupt 0 pending

#define IDA_RX_TRIG_IRPR		(1 << 31)	// lbc IDA RX fifo waterline higher than trigger level interrupt
#define IDA_TX_TRIG_IRPR		(1 << 30)	// lbc IDA TX fifo waterline lower than trigger level interrupt

#define DA_RX_TRIG_IRPR			(1 << 29)	// lbc DA RX fifo waterline higher than trigger level interrupt
#define DA_TX_TRIG_IRPR			(1 << 28)	// lbc DA TX fifo waterline lower than trigger level interrupt
#define	DA_TX_OTFIFO_OF_IRPR	(1 << 27)	// lbc DA TX outstanding fifo overflow interrupt
#define DA_RX_OTFIFO_OF_IRPR	(1 << 26)	// lbc DA RX ourstanding fifo overflow interrupt

#define DA_CMDFIFO_OF_IRPR		(1 << 25)	// lbc DA cmd fifo overflow interrupt
#define DA_CMDFIFO_UF_IRPR		(1 << 24)	// lbc DA cmd fifo underflow interrupt
#define DA_TXFIFO_OF_IRPR		(1 << 23)	// lbc DA TX data fifo overflow interrupt
#define DA_TXFIFO_UF_IRPR		(1 << 22)	// lbc DA TX data fifo underflow interrupt
#define DA_RXFIFO_OF_IRPR		(1 << 21)	// lbc DA RX data fifo overflow interrupt
#define DA_RXFIFO_UF_IRPR		(1 << 20)	// lbc DA RX data fifo underflow interrupt

#define IDA_CMDFIFO_OF_IRPR		(1 << 19)	// lbc IDA cmd fifo overflow interrupt
#define IDA_CMDFIFO_UF_IRPR		(1 << 18)	// lbc IDA cmd fifo underfow interrupt
#define IDA_TXFIFO_OF_IRPR		(1 << 17)	// lbc IDA TX data fifo overflow interrupt
#define IDA_TXFIFO_UF_IRPR		(1 << 16)	// lbc IDA TX data fifo underflow interrupt
#define IDA_RXFIFO_OF_IRPR		(1 << 15)	// lbc IDA RX data fifo overflow interrupt
#define IDA_RXFIFO_UF_IRPR		(1 << 14)	// lbc IDA RX data fifo underflow interrupt

#define DMA_CMDFIFO_OF_IRPR		(1 << 13)	// lbc DMA cmd fifo overflow interrupt
#define DMA_CMDFIFO_UF_IRPR		(1 << 12)	// lbc DMA cmd fifo underflow interrupt
#define DMA_TXFIFO_OF_IRPR		(1 << 11)	// lbc DMA TX data fifo overflow interrupt
#define DMA_TXFIFO_UF_IRPR		(1 << 10)	// lbc DMA TX data fifo underflow interrupt
#define DMA_RXFIFO_OF_IRPR		(1 << 9)	// lbc DMA RX data fifo overflow interrupt
#define DMA_RXFIFO_UF_IRPR		(1 << 8)	// lbc DMA RX data fifo underflow interrupt

#define DP_IRPR					(1 << 7)	// lbc data parity interrupt
#define ODP_IRPR				(1 << 6)	// lbc one-bit data parity interrupt

#define DMA_DES_INVLD_IRPR		(1 << 5)	// lbc DMA descriptor invld interrupt
#define DMA_TRSF_DONE_IRPR		(1 << 4)	// lbc DMA transfer done interrupt
#define IDA_TRSF_DONE_IRPR		(1 << 3)	// lbc IDA transfer done interrupt
#define DA_TRSF_DONE_IRPR		(1 << 2)	// lbc DA transfer done interrupt
#define READY_TIMEOUT_IRPR		(1 << 1)	// lbc ready timeout interrupt
#define CMD_DONE_IRPR			(1 << 0)	// lbc transfer done interrupt


#define OVER_UNDER_FLOW_IRPR	(0x0FFFFF00)

// <<<<< interrupt 0 pending

// >>>>> interrupt 1 pending

#define DMA_TIMEOUT_IRPR		(1 << 6)	// lbc DMA timeout interrupt
#define IDA_TIMEOUT_IRPR		(1 << 5)	// lbc IDA timeout interrupt
#define DA_TIMEOUR_IRPR			(1 << 4)	// lbc DA timeout interrupt
#define DMA_DES_DONE_IRPR		(1 << 3)	// lbc DMA descriptor done interrupt
#define DATA_REQ_TIMEOUT_IRPR	(1 << 2)	//lbc data req timeout interrupt
#define DMA_RX_TRIG_IRPR		(1 << 1)	// lbc DMA RX fifo waterline higher than trigger level interrupt
#define DMA_TX_TRIG_IRPR		(1 << 0)	// lbc DMA TX fifo waterline lower than trigger level interrupt

#endif
// <<<<< interrupt 1 pending


// >>>>>> param maroc
enum SWITCH {
	DISABLE,
	ENABLE,
};

enum POLARITY {
	ACTIVE_LOW,
	ACTIVE_HIGH,
};

// 0x00 LBC_MODE_CTRL_REG
enum {
	ADDR_MUX_NON_TYPE,
	ADDR_MUX_AD_TYPE,
	ADDR_MUX_AAD_TYPE,
};

enum {
	TIME_MODE_LBC0,
	TIME_MODE_LBC1,
	TIME_MODE_GPMC,
};

enum {
	BIG_BUS_ENDIAN,
	LITTLE_BUS_ENDIAN,
};

enum {
	DATA_WIDTH_8_BIT,
	DATA_WIDTH_16_BIT,
	DATA_WIDTH_32_BIT,
};

enum{
	ASYNC_RD_WR_TYPE,
	SYNC_RD_WR_TYPE,
};

// 0x04 LBC_TIMESCALE_CTRL_REG
enum {
	FCLK_DIVIDE_1,
	FCLK_DIVIDE_2,
	FCLK_DIVIDE_4,
	FCLK_DIVIDE_8,
};

enum {
	CLK_DELAY_0_CYCLE,
	CLK_DELAY_1_CYCLE,
	CLK_DELAY_2_CYCLE,
	CLK_DELAY_3_CYCLE,
	CLK_DELAY_4_CYCLE,
	CLK_DELAY_5_CYCLE,
	CLK_DELAY_6_CYCLE,
	CLK_DELAY_7_CYCLE,
};

// 0x08 LBC_READY_CTRL_REG
enum {
	READY_DELAY_0_CYCLE,
	READY_DELAY_1_CYCLE,
	READY_DELAY_2_CYCLE,
	READY_DELAY_3_CYCLE,
};

enum {
	DETECT_READY_FIRST_TRANS,
	DETECT_READY_EVERY_TRANS,
};

// 0x0C LBC_DP_CTRL_REG
enum {
	ONE_DP_PER_BYTE,
	ONE_DP_ONLY,
};

enum {
	ODD_PARITY,
	EVEN_PARITY,
};

// 0x44 LBC_CLE_TIMING_CTRL_REG
enum {
	DISABLE_IN_SINGLE_TRANS,
	ENABLE_IN_SINGLE_TRANS,
};

// 0x250 FIX_CS_REG
enum {
	FIX_TO_CHANNEL0,
	FIX_TO_CHANNEL1,
	FIX_TO_CHANNEL2,
	FIX_TO_CHANNEL3,
};

// 0x280 LBC_CLK_REG
enum {
	CLK_CLOSE_WHEN_IDLE,
	CLK_ALWAYS_ON,
};

enum {
	NOT_REVERSE_PHASE,
	REVERSE_PHASE,
};

enum {
	CLK_FROM_CCU,
	CLK_FROM_GPIO,
};

// 0x300 IDA_TS_CTRL_REG;
enum {
	IDA_BURST_ARB_GRAIN_64_BYTE,
	IDA_BURST_ARB_GRAIN_128_BYTE,
	IDA_BURST_ARB_GRAIN_256_BYTE,
	IDA_BURST_ARB_GRAIN_512_BYTE,
};

enum {
	BURST_TYPE_FIX,
	BURST_TYPE_INCR
};

enum {
	IDA_READ_DIRECTION,
	IDA_WRITE_DIRECTION
};

// 0x400 DA_BST_SEL_REG;
enum {
	BURST_SEL_AXI,
	BURST_SEL_SOFT
};



// 0x440 LBC_ARB_PRI_CFG_REG
enum {
	LOW_PRIORITY,
	MIDDLE_PRIORITY,
	HIGH_PRIORITY,
};

// 0x45f DMA_MODE_CFG_REG

enum {
	DMA_BURST_ARB_GRAIN_64_BYTE,
	DMA_BURST_ARB_GRAIN_128_BYTE,
	DMA_BURST_ARB_GRAIN_256_BYTE,
	DMA_BURST_ARB_GRAIN_512_BYTE,
};

enum {
	CS0,
	CS1,
	CS2,
	CS3,
	CS_NUMS
};

enum {
	DIRECT_ACCESS_TRANS,
	INTERRUPT_TRANS,
	DMA_TRANS,
};


enum {
	SUB_TM_NONE,
	SUB_TM_NRDD1,
	SUB_TM_NRDD15,
	SUB_TM_ADC_7616,
	SUB_TM_NAND,
	SUB_TM_ASIC,
};

enum {
	NAND_DATA_TYPE,
	NAND_CMD_TYPE,
	NAND_ADDR_TYPE,
};

// dma desc
enum {
	DMA_CUR_DESC_INVALID,
	DMA_CUR_DESC_VALID,
};

enum {
	DMA_WRITE_BACK_DISABLE,
	DMA_WRITE_BACK_ENABLE,
};

enum {
	DMA_READ_DIRECTION,
	DMA_WRITE_DIRECTION,
};

enum{
	DMA_NOT_FINISH_DESC,
	DMA_FINISH_DESC,
};

#endif
