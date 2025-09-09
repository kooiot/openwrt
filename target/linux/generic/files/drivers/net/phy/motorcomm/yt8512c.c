// SPDX-License-Identifier: GPL-2.0+
/*
 * Driver for Motorcomm YT8512C
 *
 * Author: Dirk Chang <dirk@kooiot.com>
 *
 * Copyright (c) 2021 KooIoT.COM
 */

#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mii.h>
#include <linux/phy.h>
#include <linux/netdevice.h>
#include <linux/of.h>

/* YT8512 phy identifier values */
#define YT8512_PHY_ID	0x00000128

#define YT8512_SPEC_STS_REG		0x1E
#define YT8512_EXT_ADDR_REG		0x1E
#define YT8512_EXT_DATA_REG		0x1F

#define YT8512_10BT_BUG_REG		0x200A
#define YT8512_LED0_CFG_REG		0x40C0
#define YT8512_LED1_CFG_REG		0x40C3

#define PHY_ID_HACKS 1
#define MDIO_TIMEOUT		(msecs_to_jiffies(500))

struct yt8512_phy_priv {
	struct device_node *of_node;
};

static int yt_ext_read(struct phy_device *phydev, u16 reg) {
	int ret;
	ret = phy_write(phydev, YT8512_EXT_ADDR_REG, reg);
	if (ret < 0)
		return ret;

	// usleep_range(20, 50);
	msleep(1);

	return phy_read(phydev, YT8512_EXT_DATA_REG);
}

static int yt_ext_write(struct phy_device *phydev, u16 reg, u16 data) {
	int ret;
	unsigned long timeout_jiffies;
	ret = phy_write(phydev, YT8512_EXT_ADDR_REG, reg);
	if (ret < 0)
		return ret;

	// usleep_range(20, 50);
	msleep(1);

	ret = phy_write(phydev, YT8512_EXT_DATA_REG, data);

	/* Wait read complete */
	timeout_jiffies = jiffies + MDIO_TIMEOUT;
	do {
		if (time_is_before_jiffies(timeout_jiffies))
			return -ETIMEDOUT;

		msleep(1);

		ret = yt_ext_read(phydev, reg);

		// phydev_info(phydev, "r:%x - %x", ret, data);
	} while (ret != data);

	return ret;
}

static int yt8512_probe(struct phy_device *phydev) {
	struct device *dev = &phydev->mdio.dev;
	struct device_node *of_node = dev->of_node;
	struct yt8512_phy_priv *priv;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->of_node = of_node;

	phydev->priv = priv;

	return 0;
}

static int yt8512_led_init(struct phy_device *phydev)
{
	int ret = 0;
	int val;
	struct yt8512_phy_priv *priv = phydev->priv;
	struct device_node *of_node = priv->of_node;

	ret = yt_ext_read(phydev, YT8512_LED0_CFG_REG);
	phydev_info(phydev, "read led0 config val: %x", ret);

	if (!of_property_read_u32(of_node, "motorcomm,led0", &val)) {
		phydev_info(phydev, "write led0 config val: %08x", val);
		ret = yt_ext_write(phydev, YT8512_LED0_CFG_REG, val);
		if (ret < 0) {
			phydev_err(phydev, "write led0 config failed!\n");
			goto error_return;
		}

		ret = yt_ext_read(phydev, YT8512_LED0_CFG_REG);
		phydev_info(phydev, "after write led0 config val: %x", ret);
	}

	ret = yt_ext_read(phydev, YT8512_LED1_CFG_REG);
	phydev_info(phydev, "read led1 config val: %x", ret);

	if (!of_property_read_u32(of_node, "motorcomm,led1", &val)) {
		phydev_info(phydev, "write led1 config val: %08x", val);
		ret = yt_ext_write(phydev, YT8512_LED1_CFG_REG, val);
		if (ret < 0) {
			phydev_err(phydev, "write led1 config failed!\n");
			goto error_return;
		}

		ret = yt_ext_read(phydev, YT8512_LED1_CFG_REG);
		phydev_info(phydev, "after write led1 config val: %x", ret);
	}

error_return:
	return ret;
}

static int yt8512_config_init(struct phy_device *phydev)
{
	int ret;
	struct yt8512_phy_priv *priv = phydev->priv;
	if (!priv)
		return 0;

#ifdef PHY_ID_HACKS
	ret = phy_read(phydev, MII_PHYSID1) << 16;
	ret |= phy_read(phydev, MII_PHYSID2);
	phydev_info(phydev, "%s: phy_id: %x", __func__, ret);
	if (ret != YT8512_PHY_ID)
		return 0;
#endif

	// 10BT bug?
	ret = yt_ext_read(phydev, YT8512_10BT_BUG_REG);
	if (ret < 0) {
		phydev_err(phydev, "read 10BT BUG REG failed!\n");
		goto error_return;
	} else {
		phydev_info(phydev, "10BT BUG REG val: %x", ret);
	}

	if (0) {
		ret = yt_ext_write(phydev, YT8512_10BT_BUG_REG, ret & 0xFBFF);
		if (ret < 0) {
			phydev_err(phydev, "write 10BT BUG REG failed!\n");
			goto error_return;
		}
	}

	ret = yt8512_led_init(phydev);
	if (ret < 0) {
		goto error_return;
	}

	return 0;

error_return:
	return ret;
}

static struct phy_driver yt8512c_driver[] = { {
	.name = "MOTORCOMM YT8512C",
	.phy_id = YT8512_PHY_ID,
	.phy_id_mask = 0xfffffff0,
	.features = PHY_BASIC_FEATURES,
	.probe = yt8512_probe,
	.config_init = yt8512_config_init,
} };

module_phy_driver(yt8512c_driver);

MODULE_DESCRIPTION("MOTORCOMM YT8512C PHY driver");
MODULE_AUTHOR("Dirk Chang");
MODULE_LICENSE("GPL");

static struct mdio_device_id __maybe_unused phy_tbl[] = {
	{ YT8512_PHY_ID, 0xfffffff0 },
	{ }
};

MODULE_DEVICE_TABLE(mdio, phy_tbl);
