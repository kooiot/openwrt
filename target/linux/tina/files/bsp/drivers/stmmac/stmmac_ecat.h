/* SPDX-License-Identifier: GPL-2.0-only */
/*******************************************************************************
  Copyright (C) 2007-2009  STMicroelectronics Ltd


  Author: Giuseppe Cavallaro <peppe.cavallaro@st.com>
*******************************************************************************/

#ifndef __STMMAC_ECAT_H__
#define __STMMAC_ECHAT_H__

#define STMMAC_RESOURCE_NAME   "stmmaceth"
#define DRV_MODULE_VERSION	"Jan_2016"

#include <linux/clk.h>
#include <linux/if_vlan.h>
#include <linux/stmmac.h>
#include <linux/phylink.h>
#include <linux/pci.h>
#include "common.h"
#include <linux/ptp_clock_kernel.h>
#include <linux/net_tstamp.h>
#include <linux/reset.h>
#include <net/page_pool.h>
#include "stmmac.h"

int stmmac_ethercat_dvr_probe(struct device *device, struct plat_stmmacenet_data *plat_dat, struct stmmac_resources *res);
int stmmac_ethercat_resume(struct device *dev);
int stmmac_ethercat_suspend(struct device *dev);
int stmmac_ethercat_dvr_remove(struct device *dev);
int stmmac_ethercat_bus_clks_config(struct stmmac_priv *priv, bool enabled);
int stmmac_ethercat_init_tstamp_counter(struct stmmac_priv *priv, u32 systime_flags);





#endif /* __STMMAC_H__ */
