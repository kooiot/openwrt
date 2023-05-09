// SPDX-License-Identifier: GPL-2.0+
/*
 * Driver for Motorcomm YT8521S
 *
 * Author: Dirk Chang <dirk@kooiot.com>
 *
 * Copyright (c) 2021 KooIoT.COM
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mii.h>
#include <linux/phy.h>
#include <linux/netdevice.h>
#include <linux/of.h>

/* YT8521 phy identifier values */
#define YT8521_PHY_ID	0x0000011a

#define YT8521_SPEC_STS_REG		0x1E
#define YT8521_EXT_ADDR_REG		0x1E
#define YT8521_EXT_DATA_REG		0x1F

#define YT8521_RGMII_CFG_REG	0xA003
#define YT8521_LED0_CFG_REG		0xA00C
#define YT8521_LED1_CFG_REG		0xA00D
#define YT8521_LED2_CFG_REG		0xA00E

struct yt8521_phy_priv {
	bool use_led0;
	u16 led0;
	bool use_led1;
	u16 led1;
	bool use_led2;
	u16 led2;
};

static int yt_ext_write(struct phy_device *phydev, u16 reg, u16 data) {
	int ret;
	ret = phy_write(phydev, YT8521_EXT_ADDR_REG, reg);
	if (ret < 0)
		return ret;
	return phy_write(phydev, YT8521_EXT_DATA_REG, data);
}

static int yt_ext_read(struct phy_device *phydev, u16 reg) {
	int ret;
	ret = phy_write(phydev, YT8521_EXT_ADDR_REG, reg);
	if (ret < 0)
		return ret;
	return phy_read(phydev, YT8521_EXT_DATA_REG);
}

static int yt8521_probe(struct phy_device *phydev) {
	struct device *dev = &phydev->mdio.dev;
	struct device_node *of_node = dev->of_node;
	struct yt8521_phy_priv *priv;
	u32 reg;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	if (of_property_read_u32(of_node, "motorcomm,led0", &reg)) {
		priv->use_led0 = false;
		phydev_info(phydev, "no led0 config val");
	} else {
		priv->use_led0 = true;
		priv->led0 = (u16)reg;
		phydev_info(phydev, "led0 config val: %x of_node:%pOF", priv->led0, of_node);
	}

	if (of_property_read_u32(of_node, "motorcomm,led1", &reg)) {
		priv->use_led1 = false;
		phydev_info(phydev, "no led1 config val");
	} else {
		priv->use_led1 = true;
		priv->led1 = (u16)reg;
		phydev_info(phydev, "led1 config val: %x of_node:%pOF", priv->led1, of_node);
	}

	if (of_property_read_u32(of_node, "motorcomm,led2", &reg)) {
		priv->use_led2 = false;
		phydev_info(phydev, "no led2 config val");
	} else {
		priv->use_led2 = true;
		priv->led2 = (u16)reg;
		phydev_info(phydev, "led2 config val: %x of_node:%pOF", priv->led2, of_node);
	}

	phydev->priv = priv;

	return 0;
}

static int yt8521_config_init(struct phy_device *phydev)
{
	int ret;
	struct yt8521_phy_priv *priv = phydev->priv;

	ret = yt_ext_read(phydev, YT8521_RGMII_CFG_REG);
	if (ret < 0) {
		phydev_err(phydev, "read RGMII Config failed!\n");
		goto error_return;
	} else {
		phydev_info(phydev, "RGMII Config val: %x", ret);
	}

	/* Begin YT8521s Hacks, set the correct tx delay for 1000M mode */
	ret = yt_ext_write(phydev, YT8521_RGMII_CFG_REG, 0xff);
	if (ret < 0) {
		phydev_err(phydev, "write RGMII Config failed!\n");
		goto error_return;
	}

	ret = phy_write(phydev, MII_BMCR, 0x9040); // ???
	if (ret < 0) {
		phydev_err(phydev, "write MII_BMCR failed!\n");
		goto error_return;
	}
	/* End YT8521s Hacks */

	usleep_range(10, 50);
	ret = phy_read(phydev, MII_BMSR);
	if (ret < 0) {
		phydev_err(phydev, "read MII_BMSR failed!\n");
		goto error_return;
	} else {
		phydev_info(phydev, "MII_BMSR val: %x", ret);
	}

	ret = yt_ext_read(phydev, YT8521_LED0_CFG_REG);
	phydev_info(phydev, "read led0 config val: %x", ret);

	if (priv->use_led0) {
		phydev_info(phydev, "write led0 config val: %x", priv->led0);
		ret = yt_ext_write(phydev, YT8521_LED0_CFG_REG, priv->led0);
		if (ret < 0) {
			phydev_err(phydev, "write led0 config failed!\n");
			goto error_return;
		}
	}

	ret = yt_ext_read(phydev, YT8521_LED1_CFG_REG);
	phydev_info(phydev, "read led1 config val: %x", ret);

	if (priv->use_led1) {
		phydev_info(phydev, "write led1 config val: %x", priv->led1);
		ret = yt_ext_write(phydev, YT8521_LED1_CFG_REG, priv->led1);
		if (ret < 0) {
			phydev_err(phydev, "write led0 config failed!\n");
			goto error_return;
		}
	}

	ret = yt_ext_read(phydev, YT8521_LED2_CFG_REG);
	phydev_info(phydev, "read led2 config val: %x", ret);

	if (priv->use_led2) {
		phydev_info(phydev, "write led2 config val: %x", priv->led2);
		ret = yt_ext_write(phydev, YT8521_LED2_CFG_REG, priv->led2);
		if (ret < 0) {
			phydev_err(phydev, "write led0 config failed!\n");
			goto error_return;
		}
	}

	// TODO: MORE? https://gitlab.wise-paas.com/RISC/android-kernel/-/commit/303bdc4e2101ebbe617047e69c6b58064387c89e?view=inline

	return 0;

error_return:
	return ret;
}

static struct phy_driver yt8521s_driver[] = { {
	.phy_id = YT8521_PHY_ID,
	.phy_id_mask = 0xfffffff0,
	.name = "MOTORCOMM YT8521S",
	/* PHY_GBIT_FEATURES */
	.probe = yt8521_probe,
	.config_init = yt8521_config_init,
} };

module_phy_driver(yt8521s_driver);

MODULE_DESCRIPTION("MOTORCOMM YT8521S PHY driver");
MODULE_AUTHOR("Dirk Chang");
MODULE_LICENSE("GPL");

static struct mdio_device_id __maybe_unused phy_tbl[] = {
	{ YT8521_PHY_ID, 0xfffffff0 },
	{ }
};

MODULE_DEVICE_TABLE(mdio, phy_tbl);
