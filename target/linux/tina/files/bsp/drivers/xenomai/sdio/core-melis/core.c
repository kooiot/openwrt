/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2017 ALLWINNERTECH TECHNOLOGY CO., LTD. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of ALLWINNERTECH TECHNOLOGY CO., LTD. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/pagemap.h>
#include <linux/err.h>
#include <linux/leds.h>
#include <linux/scatterlist.h>
#include <linux/log2.h>
#include <linux/pm_runtime.h>
#include <linux/pm_wakeup.h>
#include <linux/suspend.h>
#include <linux/fault-inject.h>
#include <linux/random.h>
#include <linux/slab.h>
#include <linux/of.h>

#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/sd.h>
#include <linux/mmc/core.h>
#include "sunxi-mmc-interface.h"

#include "sd_rtos_define.h"
#include "../host-bsp/sunxi-mmc.h"

/**
 *	mmc_request_done - finish processing an MMC request
 *	@host: MMC host which completed request
 *	@mrq: MMC request which request
 *
 *	MMC drivers should call this function when they have completed
 *	their processing of a request.
 */
void rtdm_mmc_request_done(struct mmc_host *host, struct mmc_request *mrq)
{
	struct sunxi_mmc_host *sunxi_host = mmc_priv(host);
	rtdm_event_signal(&sunxi_host->transfer_done);
}

static int rtdm_mmc_mrq_prep(struct mmc_host *host, struct mmc_request *mrq)
{
	unsigned int i, sz = 0;
	struct scatterlist *sg;

	if (mrq->cmd) {
		mrq->cmd->error = 0;
		mrq->cmd->mrq = mrq;
		mrq->cmd->data = mrq->data;
	}
	if (mrq->sbc) {
		mrq->sbc->error = 0;
		mrq->sbc->mrq = mrq;
	}
	if (mrq->data) {
		if (mrq->data->blksz > host->max_blk_size ||
		    mrq->data->blocks > host->max_blk_count ||
		    mrq->data->blocks * mrq->data->blksz > host->max_req_size)
				return -EINVAL;

		for_each_sg(mrq->data->sg, sg, mrq->data->sg_len, i)
			sz += sg->length;
		if (sz != mrq->data->blocks * mrq->data->blksz)
			return -EINVAL;

		mrq->data->error = 0;
		mrq->data->mrq = mrq;
		if (mrq->stop) {
			mrq->data->stop = mrq->stop;
			mrq->stop->error = 0;
			mrq->stop->mrq = mrq;
		}
	}

	return 0;
}

/**
 *	mmc_wait_for_req - start a request and wait for completion
 *	@host: MMC host to start command
 *	@mrq: MMC request to start
 *
 *	Start a new MMC custom command request for a host, and wait
 *	for the command to complete. Does not attempt to parse the
 *	response.
 */
int rtdm_mmc_wait_for_req(struct mmc_host *host, struct mmc_request *mrq)
{
	int err;
	unsigned long timeout = 2000000000; //ns
	struct sunxi_mmc_host *sunxi_host = mmc_priv(host);
	mrq->cmd->data = mrq->data;
	rtdm_event_init(&sunxi_host->transfer_done, 0);

	err = rtdm_mmc_mrq_prep(host, mrq);
	if (err) {
		printk("error : Function: %s, Line: %d - \n", __func__, __LINE__);
		return err;
	}

	err = HAL_SDC_Request(host, mrq);
	if (err) {
		mrq->cmd->error = err;
	} else {
		err = rtdm_event_timedwait(&sunxi_host->transfer_done, timeout, NULL);
		if (err) {
			printk("error : Function: %s, Line: %d - \n", __func__, __LINE__);
			err = -ETIME;
		}
	}
	return err;
}

/**
 *	mmc_wait_for_cmd - start a command and wait for completion
 *	@host: MMC host to start command
 *	@cmd: MMC command to start
 *	@retries: maximum number of retries
 *
 *	Start a new MMC command for a host, and wait for the command
 *	to complete.  Return any error that occurred while the command
 *	was executing.  Do not attempt to parse the response.
 */
int rtdm_mmc_wait_for_cmd(struct mmc_host *host, struct mmc_command *cmd)
{
	struct mmc_request mrq = {0};

	SDC_Memset(cmd->resp, 0, sizeof(cmd->resp));

	mrq.cmd = cmd;

	return rtdm_mmc_wait_for_req(host, &mrq);
}

int rtdm_mmc_go_idle(struct mmc_host *host)
{
	int32_t err;
	struct mmc_command cmd = {0};

	cmd.opcode = MMC_GO_IDLE_STATE;
	cmd.arg = 0;
	cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_NONE | MMC_CMD_BC;

	err = rtdm_mmc_wait_for_cmd(host, &cmd);

	mmc_mdelay(1);

	return err;
}

#define MMC_READ_SINGLE_BLOCK           17      /* adtc [31:0] data addr   R1, reads a block of the size seclected by SET_BLOCKLEN  */
#define MMC_READ_MULTIPLE_BLOCK         18      /* adtc [31:0] data addr   R1, continuously send blocks of data until interrupted by a stop transmission commmad  */

#define MMC_WRITE_SINGLE_BLOCK          24      /* adtc [31:0] data addr   R1, writes a block of the size seclected by SET_BLOCKLEN  */
#define MMC_WRITE_MULTIPLE_BLOCK        25      /* adtc                    R1, continuously writes blocks of data until interrupted by a stop transmission commmad  */
int32_t __sdmmc_block_rw(struct mmc_card *card, uint32_t blk_num, uint32_t blk_cnt,
			uint32_t sg_len, struct scatterlist *sg, int write)
{
	struct mmc_command cmd = {0};
	struct mmc_data data = {0};
	struct mmc_request mrq = {0};

	//debug("%s %s blk_num:%u, blk_cnt:%u, sg_len:%u \n", __func__,
		//write?"wirte":"read", (unsigned int)blk_num, (unsigned int)blk_cnt, (unsigned int)sg_len);

	if (likely(blk_cnt > 1)) {
		cmd.opcode = write ? MMC_WRITE_MULTIPLE_BLOCK : MMC_READ_MULTIPLE_BLOCK;
	} else {
		cmd.opcode = write ? MMC_WRITE_SINGLE_BLOCK : MMC_READ_SINGLE_BLOCK;
	}
	cmd.arg = blk_num;
	cmd.flags = MMC_RSP_R1 | MMC_CMD_ADTC;
	//cmd.stop = (blk_cnt == 1) ? 0 : 1;

	data.blksz = 512;
	data.flags = write ? MMC_DATA_WRITE : MMC_DATA_READ;
	data.blocks = blk_cnt;

	data.sg = sg;
	data.sg_len = sg_len;

	mrq.cmd = &cmd;
	mrq.data = &data;

	if (rtdm_mmc_wait_for_req(card->host, &mrq)) {
		printk("%s,%d %s sector:%x BSZ:%u Err!!\n", __func__, __LINE__,
			write?"W":"R", (unsigned int)blk_num, (unsigned int)blk_cnt);
		return -1;
	}

	return 0;
}

#if ((defined CONFIG_USE_SD) || (defined CONFIG_USE_MMC))
int rtdm_mmc_set_blkcnt(struct mmc_card *card, uint32_t blkcnt)
{
	struct mmc_command cmd = {0};

	cmd.opcode = MMC_SET_BLOCK_COUNT;
	cmd.arg = blkcnt & 0xffff;
	cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_AC;
	return rtdm_mmc_wait_for_cmd(card->host, &cmd);
}

int rtdm_mmc_set_blocklen(struct mmc_card *card, unsigned int blocklen)
{
	struct mmc_command cmd = {0};

	//if (mmc_card_blockaddr(card) || mmc_card_ddr_mode(card))
		//return 0;

	cmd.opcode = MMC_SET_BLOCKLEN;
	cmd.arg = blocklen;
	cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_AC;

	return rtdm_mmc_wait_for_cmd(card->host, &cmd);
}

/**
 * @brief read SD card.
 * @param card:
 *        @arg card->card handler.
 * @param buf:
 *        @arg buf->for store readed data.
 * @param sblk:
 *        @arg sblk->start block num.
 * @param nblk:
 *        @arg nblk->number of blocks.
 * @retval  0 if success or other if failed.
 */
int32_t rtdm_mmc_block_read(struct mmc_card *card, uint8_t *buf, uint64_t sblk, uint32_t nblk)
{
	int32_t err;
	struct scatterlist sg = {0};
	struct sunxi_mmc_host *sunxi_host = mmc_priv(card->host);
	if (!card || !card->host) {
		SD_LOGE_RAW(ROM_ERR_MASK, "%s,%d err", __func__, __LINE__);
		return -1;
	}

	if (nblk > SDXC_MAX_TRANS_LEN/512) {
		SD_LOGW("%s only support len < %d\n", __func__, SDXC_MAX_TRANS_LEN/512);
		return -1;
	}
#ifdef CONFIG_SD_PM
	if (card->suspend) {
		SD_LOGW("%s id:%d has suspend\n", __func__, (unsigned int)card->id);
		return -1;
	}
#endif

#ifndef CONFIG_SDMMC_SPEED_OPTIMIZE
	err = rtdm_mmc_set_blocklen(card, 512);
	if (err)
		goto out;
#endif
	if (nblk > 1) {
		err = rtdm_mmc_set_blkcnt(card, nblk);
		if (err)
			goto out;
	}
	//sg.len = 512 * nblk;
	//sg.buffer = buf;
	sg_init_one(&sg, buf, nblk*512);
	sunxi_host->sg_buf = buf;
	//if ((unsigned int)buf & 0x03) {
	//	SD_LOGW("%s buf not align 4!!!\n", __func__);
	//	return -1;
	//}

	err = __sdmmc_block_rw(card, sblk, nblk, 1, &sg, 0);

out:
	return err;
}

/**
 * @brief write SD card.
 * @param card:
 *        @arg card->card handler.
 * @param buf:
 *        @arg buf->data will be write.
 * @param sblk:
 *        @arg sblk->start block num.
 * @param nblk:
 *        @arg nblk->number of blocks.
 * @retval  0 if success or other if failed.
 */
int32_t rtdm_mmc_block_write(struct mmc_card *card, const uint8_t *buf, uint64_t sblk, uint32_t nblk)
{
	int32_t err;
	struct scatterlist sg = {0};
	struct sunxi_mmc_host *sunxi_host = mmc_priv(card->host);

	if (unlikely(!card || !card->host)) {
		SD_LOGE_RAW(ROM_ERR_MASK, "%s,%d err", __func__, __LINE__);
		return -1;
	}

	if (unlikely(nblk > SDXC_MAX_TRANS_LEN/512)) {
		SD_LOGW("%s only support block number < %d\n", __func__, SDXC_MAX_TRANS_LEN/512);
		return -1;
	}

#ifdef CONFIG_SD_PM
	if (unlikely(card->suspend)) {
		SD_LOGW("%s id:%d has suspend\n", __func__, (unsigned int)card->id);
		return -1;
	}
#endif

#ifndef CONFIG_SDMMC_SPEED_OPTIMIZE
	err = rtdm_mmc_set_blocklen(card, 512);
	if (err)
		goto out;
#endif
	if (nblk > 1) {
		err = rtdm_mmc_set_blkcnt(card, nblk);
		if (err)
			goto out;
	}
	//sg.len = 512 * nblk;
	//sg.buffer = (uint8_t *)buf;
	sg_init_one(&sg, buf, nblk*512);
	sunxi_host->sg_buf = (void *)buf;
	//if ((unsigned int)buf & 0x03) {
	//	SD_LOGW("%s buf not align 4!!!\n", __func__);
	//	return -1;
	//}

	err = __sdmmc_block_rw(card, sblk, nblk, 1, &sg, 1);

out:
	return err;
}
#endif

/*
int32_t rtdm_mmc_block_read_test(struct mmc_host *host, uint8_t *buf, uint64_t sblk, uint32_t nblk)
{
	int32_t err;
	struct scatterlist sg = {0};

	if (nblk > 1) {
		err = rtdm_mmc_set_blkcnt(host, nblk);
		if (err)
			return err;
	}

	sg_init_one(&sg, buf, nblk*512);
	//if ((unsigned int)buf & 0x03) {
	//	SD_LOGW("%s buf not align 4!!!\n", __func__);
	//	return -1;
	//}

	err = __sdmmc_block_rw(host, sblk, nblk, 1, &sg, 0);

	return err;
}
*/

/**
 * @brief scan or rescan SD card.
 * @param card:
 *        @arg card->card handler.
 * @param sdc_id:
 *        @arg sdc_id->SDC ID which card on.
 * @retval  0 if success or other if failed.
 */
int rtdm_mmc_rescan_test(void)
{
	struct mmc_host *host = NULL;
	uint8_t *buf = NULL;
	/* set identification clock 400KHz */
	HAL_SDC_Update_Clk(host, 400000);

	printk("********Function: %s, Line: %d - \n", __func__, __LINE__);
	//rtdm_mmc_go_idle(host);
	buf = kmalloc(1024, GFP_KERNEL);
	//rtdm_mmc_block_read_test(host, buf, 0, 2);
	printk("********Function: %s, Line: %d - buf0=0x%x buf1=0x%x\n", __func__, __LINE__, buf[0], buf[1]);

	printk("********Function: %s, Line: %d - \n", __func__, __LINE__);
	kfree(buf);

	return 0;
}


int32_t rtdm_mmc_send_status(struct mmc_card *card, uint32_t *status)
{
	int32_t err;
	struct mmc_command cmd = {0};

	cmd.opcode = MMC_SEND_STATUS;
	cmd.arg = card->rca << 16;
	cmd.flags = MMC_RSP_SPI_R2 | MMC_RSP_R1 | MMC_CMD_AC;

	err = rtdm_mmc_wait_for_cmd(card->host, &cmd);
	if (err)
		return err;

	/* NOTE: callers are required to understand the difference
	 * between "native" and SPI format status words!
	 */
	if (status)
		*status = cmd.resp[0];

	return 0;
}

#if ((defined CONFIG_USE_SD) || (defined CONFIG_USE_MMC))
/*
 * Mask off any voltages we don't support and select
 * the lowest voltage
 */
u32 rtdm_mmc_select_voltage(struct mmc_host *host, u32 ocr)
{
	int bit;

	/*
	 * Sanity check the voltages that the card claims to
	 * support.
	 */
	if (ocr & 0x7F) {
		dev_warn(mmc_dev(host),
		"card claims to support voltages below defined range\n");
		ocr &= ~0x7F;
	}

	ocr &= host->ocr_avail;
	if (!ocr) {
		dev_warn(mmc_dev(host), "no support for card's volts\n");
		return 0;
	}

	if (host->caps2 & MMC_CAP2_FULL_PWR_CYCLE) {
		bit = ffs(ocr) - 1;
		ocr &= 3 << bit;
		//mmc_power_cycle(host, ocr);
	} else {
		bit = fls(ocr) - 1;
		/*
		 * The bit variable represents the highest voltage bit set in
		 * the OCR register.
		 * To keep a range of 2 values (e.g. 3.2V/3.3V and 3.3V/3.4V),
		 * we must shift the mask '3' with (bit - 1).
		 */
		ocr &= 3 << (bit - 1);
		if (bit != host->ios.vdd)
			dev_warn(mmc_dev(host), "exceeding card's volts\n");
	}

	return ocr;
}

int32_t rtdm_mmc_sd_switch(struct mmc_card *card, uint8_t mode, uint8_t group,
			uint16_t value, uint8_t *resp)
{
	struct mmc_request mrq = {0};
	struct mmc_command cmd = {0};
	struct mmc_data data = {0};
	struct scatterlist sg;
	struct sunxi_mmc_host *sunxi_host = mmc_priv(card->host);

	/* NOTE: caller guarantees resp is heap-allocated */

	mode = !!mode;
	value &= 0xF;

	mrq.cmd = &cmd;
	mrq.data = &data;

	cmd.opcode = SD_SWITCH;
	cmd.arg = mode << 31 | 0x00FFFFFF;
	cmd.arg &= ~(0xF << (group * 4));
	cmd.arg |= value << (group * 4);
	cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;

	data.blksz = 64;
	data.blocks = 1;
	data.flags = MMC_DATA_READ;
	data.sg = &sg;
	data.sg_len = 1;

	sg_init_one(&sg, resp, 64);
	sunxi_host->sg_buf = resp;

	if (rtdm_mmc_wait_for_req(card->host, &mrq)) {
		return -1;
	}

	return 0;
}

int32_t rtdm_mmc_switch(struct mmc_card *card, uint8_t set, uint8_t index, uint8_t value)
{
	struct mmc_command cmd = {0};
	int32_t ret;
	uint32_t status = 0;

	cmd.opcode = MMC_SWITCH;
	cmd.arg = (MMC_SWITCH_MODE_WRITE_BYTE << 24) | (index << 16) | (value << 8) | set;
	cmd.flags = MMC_RSP_SPI_R1B | MMC_RSP_R1B | MMC_CMD_AC;

	if (rtdm_mmc_wait_for_cmd(card->host, &cmd)) {
		return -1;
	}

	/* Must check status to be sure of no errors */
	do {
		ret = rtdm_mmc_send_status(card, &status);
		if (ret)
			return ret;
	} while (R1_CURRENT_STATE(status) == R1_STATE_PRG);

	if (status & 0xFDFFA000)
		SD_LOGW("unexpected status %x after switch", (unsigned int)status);
	if (status & R1_SWITCH_ERROR)
		return -1;

	return 0;
}

int32_t rtdm_mmc_app_cmd(struct mmc_host *host, struct mmc_card *card)
{
	int32_t err;
	struct mmc_command cmd = {0};

	if (!host || (card && (card->host != host))) {
		SD_LOGE_RAW(ROM_ERR_MASK, "%s,%d err", __func__, __LINE__);
		return -1;
	}

	cmd.opcode = MMC_APP_CMD;

	if (card) {
		cmd.arg = card->rca << 16;
		cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_AC;
	} else {
		cmd.arg = 0;
		cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_BCR;
	}

	err = rtdm_mmc_wait_for_cmd(host, &cmd);
	if (err)
		return err;

	/* Check that card supported application commands */
	if (!(cmd.resp[0] & R1_APP_CMD))
		return -1;

	return 0;
}

/**
 *	rtdm_mmc_wait_for_app_cmd - start an application command and wait for
 *			       completion
 *	@host: MMC host to start command
 *	@card: Card to send MMC_APP_CMD to
 *	@cmd: MMC command to start
 *
 *	Sends a MMC_APP_CMD, checks the card response, sends the command
 *	in the parameter and waits for it to complete. Return any error
 *	that occurred while the command was executing.  Do not attempt to
 *	parse the response.
 */
int32_t rtdm_mmc_wait_for_app_cmd(struct mmc_host *host, struct mmc_card *card,
			struct mmc_command *cmd)
{
	struct mmc_request mrq = {NULL};

	int32_t i, err;

	if (!cmd) {
		SD_LOGE("%s,%d err", __func__, __LINE__);
		return -1;
	}

	err = -1;

	/*
	 * We have to resend MMC_APP_CMD for each attempt so
	 * we cannot use the retries field in mmc_command.
	 */
	for (i = 0; i <= MMC_CMD_RETRIES; i++) {
		err = rtdm_mmc_app_cmd(host, card);
		if (err) {
			continue;
		}

		SDC_Memset(&mrq, 0, sizeof(struct mmc_request));

		SDC_Memset(cmd->resp, 0, sizeof(cmd->resp));

		mrq.cmd = cmd;
		cmd->data = NULL;

		err = rtdm_mmc_wait_for_req(host, &mrq);
		if (!err)
			break;
	}

	return err;
}


int32_t rtdm_mmc_app_set_bus_width(struct mmc_card *card, uint32_t width)
{
	struct mmc_command cmd = {0};

	cmd.opcode = SET_BUS_WIDTH;
	cmd.flags = MMC_RSP_R1 | MMC_CMD_AC;

	switch (width) {
	case MMC_BUS_WIDTH_1:
		cmd.arg = SD_BUS_WIDTH_1;
		break;
	case MMC_BUS_WIDTH_4:
		cmd.arg = SD_BUS_WIDTH_4;
		break;
	default:
		cmd.arg = SD_BUS_WIDTH_1;
	}

	if (rtdm_mmc_wait_for_app_cmd(card->host, card, &cmd)) {
		return -1;
	}
	//card->bus_width = width;

	return 0;
}

int32_t rtdm_mmc_send_relative_addr(struct mmc_host *host, uint32_t *rca)
{
	struct mmc_command cmd = {0};

	cmd.opcode = SD_SEND_RELATIVE_ADDR;
	cmd.arg = 0;
	cmd.flags = MMC_RSP_R6 | MMC_CMD_BCR;

	do {
		if (rtdm_mmc_wait_for_cmd(host, &cmd)) {
			return -1;
		}
		*rca = cmd.resp[0] >> 16;
	} while (!*rca);

	return 0;
}

int32_t rtdm_mmc_send_if_cond(struct mmc_host *host, uint32_t ocr)
{
	struct mmc_command cmd = {0};
	int32_t err;
	static const uint8_t test_pattern = 0xAA;
	uint8_t result_pattern;

	/*
	 * To support SD 2.0 cards, we must always invoke SD_SEND_IF_COND
	 * before SD_APP_OP_COND. This command will harmlessly fail for
	 * SD 1.0 cards.
	 */
	cmd.opcode = SD_SEND_IF_COND;
	cmd.arg = ((ocr & 0xFF8000) != 0) << 8 | test_pattern;
	cmd.flags = MMC_RSP_SPI_R7 | MMC_RSP_R7 | MMC_CMD_BCR;

	err = rtdm_mmc_wait_for_cmd(host, &cmd);
	if (err)
		return err;

	result_pattern = cmd.resp[0] & 0xFF;

	if (result_pattern != test_pattern)
		return -1;

	return 0;
}

int32_t rtdm_mmc_select_card(struct mmc_card *card, uint32_t select)
{
	struct mmc_command cmd = {0};

	cmd.opcode = MMC_SELECT_CARD;
	if (select) {
		cmd.arg = card->rca << 16;
		cmd.flags = MMC_RSP_R1 | MMC_CMD_AC;
	} else {
		cmd.arg = 0;
		cmd.flags = MMC_RSP_NONE | MMC_CMD_AC;
	}

	if (rtdm_mmc_wait_for_cmd(card->host, &cmd)) {
		return -1;
	}

	return 0;
}

int32_t rtdm_mmc_all_send_cid(struct mmc_host *host)
{
	int32_t err;
	struct mmc_command cmd = {0};

	if (!host) {
		SDC_LOGE_RAW(ROM_ERR_MASK, "%s,%d err", __func__, __LINE__);
		return -1;
	}

	cmd.opcode = MMC_ALL_SEND_CID;
	cmd.arg = 0;
	cmd.flags = MMC_RSP_R2 | MMC_CMD_AC;

	err = rtdm_mmc_wait_for_cmd(host, &cmd);
	if (err)
		return err;

	//HAL_Memcpy(cid, cmd.resp, sizeof(uint32_t) * 4);

	return 0;
}
#endif

/**
 * @brief scan or rescan SD card.
 * @param card:
 *        @arg card->card handler.
 * @param sdc_id:
 *        @arg sdc_id->SDC ID which card on.
 * @retval  0 if success or other if failed.
 */
int rtdm_mmc_rescan(struct mmc_card *card)
{
	struct mmc_host *host = card->host;
	int32_t err = -1;

	host->ocr_avail = MMC_VDD_28_29 | MMC_VDD_29_30 | MMC_VDD_30_31 | MMC_VDD_31_32 |
	    MMC_VDD_32_33 | MMC_VDD_33_34;

	/* set identification clock 400KHz */
	HAL_SDC_Update_Clk(card->host, 400000);

	rtdm_mmc_go_idle(host);

#ifdef CONFIG_USE_SD
	/* cmd8 for SD2.0 */
	if (rtdm_mmc_send_if_cond(host, host->ocr_avail)) {
		SD_LOGN("sd1.0 or mmc\n");
	}
#endif

#ifdef CONFIG_USE_SD
	//if (!card->type || card->type == MMC_TYPE_SD) {
		SD_LOGN("***** Try sd *****\n");
		if (!rtdm_mmc_attach_sd(card, host)) {
			SD_LOGN("***** sd init ok *****\n");
			err = 0;
			goto out;
		}
	//}
#endif

#ifdef CONFIG_USE_SD
	rtdm_mmc_deattach_sd(card, host);
#endif

out:
	return err;
}
