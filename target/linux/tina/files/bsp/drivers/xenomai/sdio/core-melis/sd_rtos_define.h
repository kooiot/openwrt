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

#ifndef _SD_RTOS_DEFINE_H_
#define _SD_RTOS_DEFINE_H_

#define CONFIG_USE_SD
#define CONFIG_SDMMC_SPEED_OPTIMIZE

#define SD_LOGW printk
#define SD_LOGN printk
#define SD_LOGD printk
#define SD_LOGE_RAW(mask, format, args...) printk("SDC:"format, ##args)
#define ROM_ERR_MASK 8
#define SDC_LOGE_RAW(mask, format, args...) printk("SDC:"format, ##args)
#define SD_LOGE printk
#define SDC_Memset(d, c, l) memset(d, c, l)
#define mmc_mdelay(ms) mdelay(ms)
#define HAL_Memcpy(d, s, l) memcpy(d, s, l)
#define CONFIG_USE_SD

#define MMC_CMD_RETRIES        3
#define SDXC_MAX_TRANS_LEN              (1 << 22)       /* max len is 4M */


#define MMC_STATE_HIGHSPEED_DDR         (1 << 4)        /* card is in high speed mode */

#define mmc_card_ddr_mode(c)    ((c)->state & MMC_STATE_HIGHSPEED_DDR)

// card.h
/* Card states */
#define MMC_STATE_PRESENT	(1<<0)		/* present in sysfs */
#define MMC_STATE_READONLY	(1<<1)		/* card is read-only */
#define MMC_STATE_BLOCKADDR	(1<<2)		/* card uses block-addressing */
#define MMC_CARD_SDXC		(1<<3)		/* card is SDXC */
#define MMC_CARD_REMOVED	(1<<4)		/* card has been removed */
#define MMC_STATE_SUSPENDED	(1<<5)		/* card is suspended */

#define mmc_card_present(c)	((c)->state & MMC_STATE_PRESENT)
#define mmc_card_readonly(c)	((c)->state & MMC_STATE_READONLY)
#define mmc_card_blockaddr(c)	((c)->state & MMC_STATE_BLOCKADDR)
#define mmc_card_ext_capacity(c) ((c)->state & MMC_CARD_SDXC)
#define mmc_card_removed(c)	((c) && ((c)->state & MMC_CARD_REMOVED))
#define mmc_card_suspended(c)	((c)->state & MMC_STATE_SUSPENDED)

#define mmc_card_set_present(c)	((c)->state |= MMC_STATE_PRESENT)
#define mmc_card_set_readonly(c) ((c)->state |= MMC_STATE_READONLY)
#define mmc_card_set_blockaddr(c) ((c)->state |= MMC_STATE_BLOCKADDR)
#define mmc_card_set_ext_capacity(c) ((c)->state |= MMC_CARD_SDXC)
#define mmc_card_set_removed(c) ((c)->state |= MMC_CARD_REMOVED)
#define mmc_card_set_suspended(c) ((c)->state |= MMC_STATE_SUSPENDED)
#define mmc_card_clr_suspended(c) ((c)->state &= ~MMC_STATE_SUSPENDED)


/* SD_SWITCH function groups */
#define SD_SWITCH_GRP_ACCESS_MODE               0
#define SD_SWITCH_GRP_CMD_SYSTEM                1
#define SD_SWITCH_GRP_DRV_STRENGTH              2
#define SD_SWITCH_GRP_CUR_LIMIT                 3

/*
 * SD_SWITCH mode
 */
#define SD_SWITCH_CHECK                 0
#define SD_SWITCH_SET                   1

/*
 * SD_SWITCH access modes
 */
#define SD_SWITCH_ACCESS_DEF            0
#define SD_SWITCH_ACCESS_HS             1

#define SET_BUS_WIDTH                   6

int rtdm_mmc_set_blocklen(struct mmc_card *card, unsigned int blocklen);
int rtdm_mmc_rescan(struct mmc_card *card);
void rtdm_mmc_deattach_sd(struct mmc_card *card, struct mmc_host *host);
int32_t rtdm_mmc_block_write(struct mmc_card *card, const uint8_t *buf, uint64_t sblk, uint32_t nblk);
int32_t rtdm_mmc_block_read(struct mmc_card *card, uint8_t *buf, uint64_t sblk, uint32_t nblk);
int32_t rtdm_mmc_attach_sd(struct mmc_card *card, struct mmc_host *host);
void rtdm_mmc_deattach_sd(struct mmc_card *card, struct mmc_host *host);

#endif /* _SD_RTOS_DEFINE_H_ */
