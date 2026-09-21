/*
 * USB ethernet driver for USB2.0 to 100Mbps ethernet chip ch397.
 *
 * Copyright (C) 2026 Nanjing Qinheng Microelectronics Co., Ltd.
 * Web: http://wch.cn
 * Author: WCH <tech@wch.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Update Log:
 * V1.0 - initial version
 * V1.1 - add support for kernel version beyond 2.6.33
 * V1.2 - add usb packet protocol length judgment
 *      - add support for VLAN network
 *      - add support for ch396, ch339, ch336
 * V1.3 - add support for frame in multiple usb packets
 * V1.4 - add support for multicast setting and parameters saving when autoneg off
 * V1.5 - add support for mac address filtering and fixed tx_fixup/rx_fixup
 * V1.5.1 - add flow control and fixed use after free
 *        - add autosuspend and phy flow control
 * V1.5.2 - improve link handling and speed/duplex configuration
 *        - improve TX/RX processing and VLAN/multicast handling
 *        - fix configuration synchronization and error handling
 */

#define DEBUG
#define VERBOSE

#undef DEBUG
#undef VERBOSE

#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/ethtool.h>
#include <linux/mii.h>
#include <linux/usb.h>
#include <linux/usb/usbnet.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/if_vlan.h>
#include <linux/workqueue.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/crc32.h>
#include <linux/bitrev.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <asm/unaligned.h>

#define DRIVER_AUTHOR "WCH"
#define DRIVER_DESC "USB ethernet driver for ch397, etc."
#define VERSION_DESC "V1.5.2 On 2026.09"

/* control requests */
#define CH397_USB_GET_INFO   0x10
#define CH397_USB_RD_REG     0x11
#define CH397_USB_WR_REG     0x12
#define CH397_USB_RD_OTP     0x13
#define CH397_USB_WR_OTP     0x14
#define CH397_USB_RD_PHY     0x15
#define CH397_USB_WR_PHY     0x16
#define CH397_RESET_CMD	     0x18
#define CH397_WR_ETH_MACCFG  0x1E
#define CH397_WR_ETH_AUTONEG 0x1F

#define CH397_MAX_MCAST		12

#define CH397_TX_OVERHEAD 8
#define CH397_RX_OVERHEAD 8
#define CH397_RX_URB_SIZE (16 * 1024)

#define CH397_LINK_REAPPLY_DELAY	200
#define CH397_FORCED_REAPPLY_DELAY	1100

/* Keep the saved capabilities in the legacy ethtool bitmap format. */
#define PHY_ADVERTISED_10_HALF ADVERTISED_10baseT_Half
#define PHY_ADVERTISED_10_FULL ADVERTISED_10baseT_Full
#define PHY_ADVERTISED_100_HALF ADVERTISED_100baseT_Half
#define PHY_ADVERTISED_100_FULL ADVERTISED_100baseT_Full
#define CH397_ADVERTISED_MODES                             \
	(PHY_ADVERTISED_10_HALF | PHY_ADVERTISED_10_FULL | \
	 PHY_ADVERTISED_100_HALF | PHY_ADVERTISED_100_FULL)

#define CH397_ETH_MAC_CFG  0x40000700
#define CH397_ETH_MAC_H	   0x40000710
#define CH397_ETH_MAC_L	   0x40000714
#define CH397_ETH_BMSR	   0x00000740
#define CH397_ETH_MAC_HTHR 0x40000730
#define CH397_ETH_MAC_HTLR 0x40000734

#define CH397_PHY_PAGE_REG		0x1f
#define CH397_PHY_LED_PAGE		0x0007
#define CH397_PHY_LED_MODE_REG		0x11
#define CH397_PHY_LED_SIGNAL_REG	0x15
#define CH397_PHY_LED_MODE_MASK		0x000f

/* CH397 revision D: select one mode per LED; these are not bit flags. */
enum ch397_led_mode {
	CH397_LED_MODE_FLOAT = 0x0, /* Floating / high impedance */
	CH397_LED_MODE_LINK_10M = 0x1, /* link10 */
	CH397_LED_MODE_LINK_100M = 0x2, /* link100 */
	CH397_LED_MODE_LINK_10M_100M = 0x3, /* link10 + link100 */
	CH397_LED_MODE_ACT_10M_100M = 0x4, /* act10 + act100 */
	CH397_LED_MODE_LINK_ACT_10M = 0x5, /* link10 + act10 */
	CH397_LED_MODE_LINK_ACT_100M = 0x6, /* link100 + act100 */
	CH397_LED_MODE_LINK_ACT_ALL = 0x7, /* link10 + link100 + act10 + act100 */
};

#define CH397_LINK_STATUS (1 << 6)

#define CH397_MEDIUM_PS	  (1 << 7)
#define CH397_MEDIUM_FD	  (1 << 8)
#define CH397_FLOWCTRL_EN (1 << 11)
#define CH397_TX_CTRL_EN  (1 << 13)
#define CH397_RX_CTRL_EN  (1 << 14)
#define CH397_RX_CTRL_PLM (1 << 23)
#define CH397_RX_CTRL_PBD (1 << 22)
#define CH397_RX_CTRL_PUF (1 << 21)
#define CH397_RX_CTRL_PAM (1 << 20)
#define CH397_RX_CTRL_RA  (1 << 19)

#define CH397_USB_DELAY	 10
#define CH397_MT_DELAY	 10
#define CH397_WORK_DELAY 100

#define ETH_HEADER_SIZE 14 /* size of ethernet header */

#define ETH_MIN_DATA_SIZE   46 /* minimum eth data size */
#define ETH_MIN_PACKET_SIZE (ETH_HEADER_SIZE + ETH_MIN_DATA_SIZE)

#define ETH_DEF_DATA_SIZE   1500 /* default data size */
#define ETH_DEF_PACKET_SIZE (ETH_HEADER_SIZE + ETH_DEF_DATA_SIZE)

/* ch397 flags */
enum ch397_flags {
	CH397_SET_RX_MODE = 0,
	CH397_LINK_CHG,
	CH397_SET_PHY_CFG,
	CH397_LINK_CHG_LED_CFG,
};

struct ch397_intr_stats {
	u32 link_stat;
	u32 rx_packets;
	u16 rx_overflow_cnt;
	u16 rx_crc_cnt;
	u32 tx_packets;
};

struct ch397_intr_event {
	__le32 link_stat;
#define CH397_LINK_SPEED BIT(7)
#define CH397_LINK_RDY BIT(6)
#define CH397_DUPLEX_MODE BIT(0)
	__le32 rx_packets;
	__le16 rx_overflow_cnt;
	__le16 rx_crc_cnt;
	__le32 tx_packets;
} __packed;

struct ch397_chip_info {
	u8 chiptype;
	u8 hwver;
	u8 fwver;
	u8 sta;
	u8 reserved[4];
} __packed;

struct ch397_ndev_cfg {
	bool link;
	bool phy_wol;

	u16 speed; /* user configured forced speed */
	u8 duplex; /* user configured forced duplex */
	u8 autoneg;
	u32 advertising;
	u16 link_speed; /* current resolved link speed */
	u8 link_duplex; /* current resolved link duplex */
};

struct ch397_common_private {
	struct usbnet *dev;
	struct delayed_work schedule_work;
	struct workqueue_struct *wq;
	struct ch397_ndev_cfg ndev_cfg;
	struct ch397_intr_stats ch397_intr;
	unsigned long flags;
	/* USB sequences may sleep; completion only takes state_lock.
	 * Lock order: control_mutex -> state_lock. Release state_lock before I/O.
	 */
	struct mutex control_mutex;
	spinlock_t state_lock;
	unsigned long link_change_jiffies; /* earliest PHY reapply time */
	unsigned int link_change_count; /* reject setup across link changes */
	u16 crc_num;
	u32 presvd_mac_mask_set;
	u32 presvd_mac_mask_clear;
	u32 presvd_mac_cfg;
	struct ch397_chip_info chip_info;
};

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 8, 0))

static int __usbnet_read_cmd(struct usbnet *dev, u8 cmd, u8 reqtype, u16 value,
			     u16 index, void *data, u16 size)
{
	void *buf = NULL;
	int err = -ENOMEM;

	netdev_dbg(dev->net,
		   "usbnet_read_cmd cmd=0x%02x reqtype=%02x"
		   " value=0x%04x index=0x%04x size=%d\n",
		   cmd, reqtype, value, index, size);

	if (data) {
		buf = kmalloc(size, GFP_KERNEL);
		if (!buf)
			goto out;
	}

	err = usb_control_msg(dev->udev, usb_rcvctrlpipe(dev->udev, 0), cmd,
			      reqtype, value, index, buf, size,
			      USB_CTRL_GET_TIMEOUT);
	if (err > 0 && err <= size)
		memcpy(data, buf, err);
	kfree(buf);
out:
	return err;
}

static int __usbnet_write_cmd(struct usbnet *dev, u8 cmd, u8 reqtype, u16 value,
			      u16 index, const void *data, u16 size)
{
	void *buf = NULL;
	int err = -ENOMEM;

	netdev_dbg(dev->net,
		   "usbnet_write_cmd cmd=0x%02x reqtype=%02x"
		   " value=0x%04x index=0x%04x size=%d\n",
		   cmd, reqtype, value, index, size);

	if (data) {
		buf = kmemdup(data, size, GFP_KERNEL);
		if (!buf)
			goto out;
	}

	err = usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0), cmd,
			      reqtype, value, index, buf, size,
			      USB_CTRL_SET_TIMEOUT);
	kfree(buf);

out:
	return err;
}

/*
 * The function can't be called inside suspend/resume callback,
 * otherwise deadlock will be caused.
 */
int usbnet_read_cmd(struct usbnet *dev, u8 cmd, u8 reqtype, u16 value,
		    u16 index, void *data, u16 size)
{
	int ret;

	if (usb_autopm_get_interface(dev->intf) < 0)
		return -ENODEV;

	ret = __usbnet_read_cmd(dev, cmd, reqtype, value, index, data, size);
	usb_autopm_put_interface(dev->intf);

	return ret;
}

/*
 * The function can't be called inside suspend/resume callback,
 * otherwise deadlock will be caused.
 */
int usbnet_write_cmd(struct usbnet *dev, u8 cmd, u8 reqtype, u16 value,
		     u16 index, const void *data, u16 size)
{
	int ret;

	if (usb_autopm_get_interface(dev->intf) < 0)
		return -ENODEV;

	ret = __usbnet_write_cmd(dev, cmd, reqtype, value, index, data, size);
	usb_autopm_put_interface(dev->intf);

	return ret;
}

#endif

/* Payloads are raw bytes; USB core encodes CPU-endian SETUP value/index. */
static int ch397_read(struct usbnet *dev, u8 cmd, u32 reg, u16 length,
		      void *data)
{
	int err, i;
	u16 value = (u16)(reg & 0xFFFF);
	u16 index = (u16)((reg >> 16) & 0xFFFF);

	if (length && !data)
		return -EINVAL;

	err = usbnet_read_cmd(dev, cmd,
			      USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
			      value, index, data, length);
	if (err != length && err >= 0)
		err = -EIO;

	dev_dbg(&dev->intf->dev, "ch397_read() cmd=0x%02x, reg=0x%08x, read=\n",
		cmd, reg);
	for (i = 0; err >= 0 && i < length; i++)
		dev_dbg(&dev->intf->dev, "\t0x%2x\n", *((u8 *)data + i));

	msleep(CH397_USB_DELAY);

	return err;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 17, 0))
static int ch397_write(struct usbnet *dev, u8 cmd, u32 reg, u16 length,
		       const void *data)
#else
static int ch397_write(struct usbnet *dev, u8 cmd, u32 reg, u16 length,
		       void *data)
#endif
{
	int err, i;
	u16 value = (u16)(reg & 0xFFFF);
	u16 index = (u16)((reg >> 16) & 0xFFFF);

	if (length && !data)
		return -EINVAL;

	err = usbnet_write_cmd(dev, cmd,
			       USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
			       value, index, data, length);
	if (err >= 0 && err != length)
		err = -EIO;

	dev_dbg(&dev->intf->dev,
		"ch397_write() cmd=0x%02x, reg=0x%08x, write=\n", cmd, reg);
	for (i = 0; i < length; i++)
		dev_dbg(&dev->intf->dev, "\t0x%2x\n", *((u8 *)data + i));

	msleep(CH397_USB_DELAY);

	return err;
}

static int ch397_read_reg(struct usbnet *dev, u32 reg, u32 *value)
{
	__le32 val;

	int ret = ch397_read(dev, CH397_USB_RD_REG, reg, sizeof(val), &val);
	if (ret < 0)
		return ret;

	*value = le32_to_cpu(val);

	return 0;
}

static int ch397_write_reg(struct usbnet *dev, u32 reg, u32 value)
{
	__le32 val = cpu_to_le32(value);

	int ret = ch397_write(dev, CH397_USB_WR_REG, reg, sizeof(val), &val);
	return ret < 0 ? ret : 0;
}

static int ch397_read_shared_word(struct usbnet *dev, int phy, u8 reg,
				  u16 *value)
{
	__le16 val;

	int ret = ch397_read(dev, CH397_USB_RD_PHY,
			     CH397_ETH_BMSR | ((u32)reg << 16), sizeof(val),
			     &val);
	if (ret < 0)
		return ret;

	*value = le16_to_cpu(val);

	return 0;
}

static int ch397_write_shared_word(struct usbnet *dev, int phy, u8 reg,
				   u16 value)
{
	__le16 val = cpu_to_le16(value);

	int ret = ch397_write(dev, CH397_USB_WR_PHY,
			      CH397_ETH_BMSR | ((u32)reg << 16), sizeof(val),
			      &val);
	return ret < 0 ? ret : 0;
}

static int ch397_mdio_read(struct net_device *net, int phy_id, int loc)
{
	struct usbnet *dev = netdev_priv(net);
	u16 res;
	int err;

	if (phy_id) {
		netdev_dbg(dev->net, "Only internal phy supported\n");
		return -EINVAL;
	}

	err = ch397_read_shared_word(dev, 1, loc, &res);
	if (err < 0)
		return err;

	netdev_dbg(
		dev->net,
		"ch397_mdio_read() phy_id=0x%02x, loc=0x%02x, returns=0x%04x\n",
		phy_id, loc, res);

	return res;
}

static void ch397_mdio_write(struct net_device *net, int phy_id, int loc,
			     int val)
{
	struct usbnet *dev = netdev_priv(net);
	int err;

	if (phy_id) {
		netdev_dbg(dev->net, "Only internal phy supported\n");
		return;
	}

	/* Filtering only by val here also discarded unrelated PHY writes. */
	err = ch397_write_shared_word(dev, 1, loc, val);
	if (err < 0)
		netdev_err(net, "Error writing PHY register %d: %d\n", loc,
			   err);

	netdev_dbg(dev->net,
		   "ch397_mdio_write() phy_id=0x%02x, loc=0x%02x, val=0x%04x\n",
		   phy_id, loc, val);
}

/* Caller holds control_mutex (except during bind) and has selected PAGE 7.
 * ch397_set_ledcfg() owns page selection/restoration for both LEDs.
 */
static int ch397_cfg_led(struct usbnet *dev, int led_num, int led_mode,
			 bool signal_high)
{
	u16 led_cfg, signal_cfg, mode_mask, signal_mask;
	int shift, ret;

	if (led_num < 0 || led_num > 1 ||
	    led_mode < CH397_LED_MODE_FLOAT ||
	    led_mode > CH397_LED_MODE_LINK_ACT_ALL) {
		netdev_err(dev->net, "Invalid LED number %d or mode %d\n",
			   led_num, led_mode);
		return -EINVAL;
	}

	ret = ch397_read_shared_word(dev, 1, CH397_PHY_LED_MODE_REG,
				     &led_cfg);
	if (ret < 0)
		return ret;

	ret = ch397_read_shared_word(dev, 1, CH397_PHY_LED_SIGNAL_REG,
				     &signal_cfg);
	if (ret < 0)
		return ret;

	netdev_err(dev->net, "Current LED[%d] cfg: %08x signal: %08x\n",
			led_num, led_cfg, signal_cfg);

	/* PAGE 7 / 0x11: LED0 = bit[3:0], LED1 = bit[7:4]. */
	shift = led_num * 4;
	mode_mask = CH397_PHY_LED_MODE_MASK << shift;
	led_cfg = (led_cfg & ~mode_mask) | (led_mode << shift);

	/* PAGE 7 / 0x15: bit0 = LED0, bit1 = LED1; 1 = low, 0 = high. */
	signal_mask = BIT(led_num);
	if (signal_high)
		signal_cfg &= ~signal_mask;
	else
		signal_cfg |= signal_mask;

	netdev_err(dev->net, "Write LED[%d] cfg: %08x signal: %08x\n",
			led_num, led_cfg, signal_cfg);

	ret = ch397_write_shared_word(dev, 1, CH397_PHY_LED_MODE_REG,
				      led_cfg);
	if (ret < 0)
		return ret;

	return ch397_write_shared_word(dev, 1, CH397_PHY_LED_SIGNAL_REG,
				       signal_cfg);
}

/* Caller holds control_mutex, except during bind before device registration. */
static int ch397_set_ledcfg(struct usbnet *dev)
{
	int ret;

	ret = ch397_write_shared_word(dev, 1, CH397_PHY_PAGE_REG,
				      CH397_PHY_LED_PAGE);
	if (ret < 0) {
		netdev_err(dev->net, "%s: Error selecting page 7: %d\n",
			   __func__, ret);
		goto restore_page;
	}

	/* Edit the two ch397_cfg_led() calls below to configure the LEDs.
	 *   led_num     : 0 = LED0, 1 = LED1.
	 *   led_mode    : Select one enum ch397_led_mode value; do not OR modes.
	 *                 LINK indicates a link at the selected speed(s).
	 *                 ACT indicates activity at the selected speed(s).
	 *                 FLOAT leaves the pin floating regardless of polarity.
	 *   signal_high : true = high level (clear the corresponding 0x15 bit);
	 *                 false = low level (set the corresponding 0x15 bit).
	 *
	 * Example: LED0 indicates a 100M link, high level:
	 *   ch397_cfg_led(dev, 0, CH397_LED_MODE_LINK_100M, true);
	 * Example: LED1 indicates 10M/100M activity only, low level:
	 *   ch397_cfg_led(dev, 1, CH397_LED_MODE_ACT_10M_100M, false);
	 *
	 * Defaults retain the original modes: LED0 floating, LED1 all link/ACT.
	 * Both polarity bits are set to low level. Change false to true below
	 * for a high-level signal on the corresponding LED.
	 */
	ret = ch397_cfg_led(dev, 0, CH397_LED_MODE_LINK_100M, false);
	if (ret < 0)
		netdev_err(dev->net, "%s: LED0 configuration failed: %d\n",
			   __func__, ret);

	ret = ch397_cfg_led(dev, 1, CH397_LED_MODE_LINK_ACT_ALL, false);
	if (ret < 0)
		netdev_err(dev->net, "%s: LED1 configuration failed: %d\n",
			   __func__, ret);

restore_page:
	/* Always try PAGE 0, including after a failed PAGE 7 selection. */
	ret = ch397_write_shared_word(dev, 1, CH397_PHY_PAGE_REG, 0);
	if (ret < 0)
		netdev_err(dev->net, "%s: Error restoring page 0: %d\n",
			   __func__, ret);

	/* LED errors are logged above; return only the PAGE 0 restore result. */
	return ret;
}

static void ch397_get_drvinfo(struct net_device *net,
			      struct ethtool_drvinfo *info)
{
	/* Inherit standard device info */
	usbnet_get_drvinfo(net, info);
}

static u32 ch397_get_link(struct net_device *net)
{
	struct usbnet *dev = netdev_priv(net);

	struct ch397_common_private *dp = dev->driver_priv;
	u32 link;

	mutex_lock(&dp->control_mutex);
	link = mii_link_ok(&dev->mii);
	mutex_unlock(&dp->control_mutex);

	return link;
}

static int ch397_ioctl(struct net_device *net, struct ifreq *rq, int cmd)
{
	struct usbnet *dev = netdev_priv(net);
	struct ch397_common_private *dp = dev->driver_priv;
	int ret;

	mutex_lock(&dp->control_mutex);
	ret = generic_mii_ioctl(&dev->mii, if_mii(rq), cmd, NULL);
	mutex_unlock(&dp->control_mutex);

	return ret;
}

static int ch397_set_speed(struct ch397_common_private *dp, u8 autoneg,
			   u16 speed, u8 duplex, u32 advertising);

/* Validate requested modes and fill omitted advertising; no PHY I/O. */
static int ch397_normalize_advertising(u8 autoneg, u32 speed, u8 duplex,
				       u32 advertising, u32 *normalized)
{
	u32 req = advertising & CH397_ADVERTISED_MODES;

	if (autoneg == AUTONEG_DISABLE) {
		if ((speed != SPEED_10 && speed != SPEED_100) ||
		    (duplex != DUPLEX_HALF && duplex != DUPLEX_FULL))
			return -EINVAL;
		*normalized = 0;

		return 0;
	}

	if (autoneg != AUTONEG_ENABLE || (advertising && !req))
		return -EINVAL;

	if (req) {
		*normalized = req;
		return 0;
	}
	/* Preserve an explicit speed/duplex request without adding slower
	 * modes. */
	if (speed == SPEED_10) {
		*normalized =
			duplex == DUPLEX_HALF ? PHY_ADVERTISED_10_HALF :
			duplex == DUPLEX_FULL ? PHY_ADVERTISED_10_FULL :
						PHY_ADVERTISED_10_HALF |
							PHY_ADVERTISED_10_FULL;
	} else if (speed == SPEED_100) {
		*normalized =
			duplex == DUPLEX_HALF ? PHY_ADVERTISED_100_HALF :
			duplex == DUPLEX_FULL ? PHY_ADVERTISED_100_FULL :
						PHY_ADVERTISED_100_HALF |
							PHY_ADVERTISED_100_FULL;
	} else {
		*normalized = CH397_ADVERTISED_MODES;
	}

	return 0;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 12, 0))
static int ch397_get_settings(struct net_device *net,
			      struct ethtool_link_ksettings *cmd)
#else
static int ch397_get_settings(struct net_device *net, struct ethtool_cmd *cmd)
#endif
{
	struct usbnet *dev = netdev_priv(net);
	struct ch397_common_private *dp = dev->driver_priv;
	int ret;

	mutex_lock(&dp->control_mutex);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 13, 0))
	ret = usbnet_get_link_ksettings_mii(net, cmd);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 12, 0))
	ret = usbnet_get_link_ksettings(net, cmd);
#else
	ret = usbnet_get_settings(net, cmd);
#endif
	mutex_unlock(&dp->control_mutex);

	return ret;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 12, 0))
static int ch397_set_settings(struct net_device *net,
			      const struct ethtool_link_ksettings *cmd)
#else
static int ch397_set_settings(struct net_device *net, struct ethtool_cmd *cmd)
#endif
{
	struct usbnet *dev = netdev_priv(net);
	struct ch397_common_private *dp = dev->driver_priv;
	u32 advertising = 0, normalized, speed;
	u8 autoneg, duplex;
	unsigned long flags;
	int ret;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 12, 0))
	autoneg = cmd->base.autoneg;
	speed = cmd->base.speed;
	duplex = cmd->base.duplex;

	if (test_bit(ETHTOOL_LINK_MODE_10baseT_Half_BIT,
		     cmd->link_modes.advertising))
		advertising |= PHY_ADVERTISED_10_HALF;
	if (test_bit(ETHTOOL_LINK_MODE_10baseT_Full_BIT,
		     cmd->link_modes.advertising))
		advertising |= PHY_ADVERTISED_10_FULL;
	if (test_bit(ETHTOOL_LINK_MODE_100baseT_Half_BIT,
		     cmd->link_modes.advertising))
		advertising |= PHY_ADVERTISED_100_HALF;
	if (test_bit(ETHTOOL_LINK_MODE_100baseT_Full_BIT,
		     cmd->link_modes.advertising))
		advertising |= PHY_ADVERTISED_100_FULL;
	/* Unsupported gigabit-only requests must not become a default 100M set.
	 */
	if (test_bit(ETHTOOL_LINK_MODE_1000baseT_Half_BIT,
		     cmd->link_modes.advertising) ||
	    test_bit(ETHTOOL_LINK_MODE_1000baseT_Full_BIT,
		     cmd->link_modes.advertising))
		advertising |= BIT(4);
#else
	autoneg = cmd->autoneg;
	speed = ethtool_cmd_speed(cmd);
	duplex = cmd->duplex;
	advertising = cmd->advertising & CH397_ADVERTISED_MODES;

	if (cmd->advertising &
	    (ADVERTISED_1000baseT_Half | ADVERTISED_1000baseT_Full))
		advertising |= BIT(4);
#endif
	ret = ch397_normalize_advertising(autoneg, speed, duplex, advertising,
					  &normalized);
	if (ret < 0)
		return ret;

	/* The PHY setter ignores speed in autoneg mode when a mask is present.
	 */
	if (autoneg == AUTONEG_ENABLE && speed != SPEED_10 &&
	    speed != SPEED_100)
		speed = SPEED_100;

	mutex_lock(&dp->control_mutex);
	ret = ch397_set_speed(dp, autoneg, speed, duplex, normalized);
	if (!ret) {
		spin_lock_irqsave(&dp->state_lock, flags);
		dp->ndev_cfg.autoneg = autoneg;
		dp->ndev_cfg.speed = speed;
		dp->ndev_cfg.duplex = duplex;
		dp->ndev_cfg.advertising = normalized;
		spin_unlock_irqrestore(&dp->state_lock, flags);
	}
	mutex_unlock(&dp->control_mutex);

	return ret;
}

static const char ch397_gstrings[][ETH_GSTRING_LEN] = {
	"rx_packets",
	"tx_packets",
	"rx_overflow_packets",
	"rx_crc_errors",
};

static void ch397_get_strings(struct net_device *net, u32 stringset, u8 *data)
{
	if (stringset == ETH_SS_STATS)
		memcpy(data, ch397_gstrings, sizeof(ch397_gstrings));
}

static int ch397_get_sset_count(struct net_device *net, int sset)
{
	switch (sset) {
	case ETH_SS_STATS:
		return ARRAY_SIZE(ch397_gstrings);
	default:
		return -EOPNOTSUPP;
	}
}

static void ch397_get_ethtool_stats(struct net_device *net,
				    struct ethtool_stats *stats, u64 *data)
{
	struct usbnet *dev = netdev_priv(net);
	struct ch397_common_private *dp = dev->driver_priv;
	struct ch397_intr_stats event;
	unsigned long flags;

	/* Snapshot the latest counters published by the interrupt handler. */
	spin_lock_irqsave(&dp->state_lock, flags);
	event = dp->ch397_intr;
	spin_unlock_irqrestore(&dp->state_lock, flags);

	data[0] = event.rx_packets;
	data[1] = event.tx_packets;
	data[2] = event.rx_overflow_cnt;
	data[3] = event.rx_crc_cnt;
}

static const struct ethtool_ops ch397_ethtool_ops = {
	.get_drvinfo = ch397_get_drvinfo,
	.get_link = ch397_get_link,
	.get_msglevel = usbnet_get_msglevel,
	.set_msglevel = usbnet_set_msglevel,
	.nway_reset = usbnet_nway_reset,
	.get_strings = ch397_get_strings,
	.get_sset_count = ch397_get_sset_count,
	.get_ethtool_stats = ch397_get_ethtool_stats,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 13, 0))
	.get_link_ksettings = ch397_get_settings,
	.set_link_ksettings = ch397_set_settings,
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 12, 0))
	.get_link_ksettings = ch397_get_settings,
	.set_link_ksettings = ch397_set_settings,
#else
	.get_settings = ch397_get_settings,
	.set_settings = ch397_set_settings,
#endif
};

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0))
static void ch397_tx_timeout(struct net_device *ndev, unsigned int txqueue)
#else
static void ch397_tx_timeout(struct net_device *ndev)
#endif
{
	struct usbnet *dev = netdev_priv(ndev);

	netif_warn(dev, tx_err, ndev, "ch397 tx timeout!\n");

	usb_queue_reset_device(dev->intf);
}

static int ch397_get_mac_address(struct usbnet *dev, void *mac_addr)
{
	int err;
	u8 buf[4];

	memset(buf, 0, sizeof(buf));
	err = ch397_read(dev, CH397_USB_RD_REG, CH397_ETH_MAC_L, 4, buf);
	if (err < 0) {
		netdev_err(dev->net, "Error getting MAC low address.\n");
		return err;
	}
	memcpy(mac_addr, buf, 4);

	memset(buf, 0, sizeof(buf));
	err = ch397_read(dev, CH397_USB_RD_REG, CH397_ETH_MAC_H, 2, buf);
	if (err < 0) {
		netdev_err(dev->net, "Error getting MAC high address.\n");
		return err;
	}

	memcpy(mac_addr + 4, buf, 2);

	return 0;
}

static int ch397_get_info(struct usbnet *dev, void *chip_info)
{
	int err;
	u8 buf[8];

	memset(buf, 0, sizeof(buf));
	err = ch397_read(dev, CH397_USB_GET_INFO, 0x00, 8, buf);
	if (err < 0) {
		netdev_err(dev->net, "Error getting chip info.\n");
		return err;
	}
	memcpy(chip_info, buf, 8);

	return 0;
}

static int ch397_set_flowctrl(struct usbnet *dev)
{
	int anar;

	anar = ch397_mdio_read(dev->net, dev->mii.phy_id, MII_ADVERTISE);
	if (anar < 0)
		return anar;
	anar |= (ADVERTISE_PAUSE_CAP | ADVERTISE_PAUSE_ASYM);
	return ch397_write_shared_word(dev, dev->mii.phy_id, MII_ADVERTISE,
				       anar);
}

static int __ch397_set_mac_address(struct usbnet *dev)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 17, 0))
	const u8 *dev_addr = dev->net->dev_addr;
#else
	u8 *dev_addr = dev->net->dev_addr;
#endif

	int err;

	err = ch397_write(dev, CH397_USB_WR_REG, CH397_ETH_MAC_L, 4, dev_addr);
	if (err < 0)
		return err;

	err = ch397_write(dev, CH397_USB_WR_REG, CH397_ETH_MAC_H, 2,
			  dev_addr + 4);
	return err < 0 ? err : 0;
}

static int ch397_set_mac_address(struct net_device *net, void *p)
{
	struct sockaddr *addr = p;
	struct usbnet *dev = netdev_priv(net);
	struct ch397_common_private *dp = dev->driver_priv;
	u8 old_addr[ETH_ALEN];
	int err;

	if (!is_valid_ether_addr(addr->sa_data)) {
		dev_err(&net->dev, "not setting invalid mac address %pM\n",
			addr->sa_data);
		return -EINVAL;
	}

	mutex_lock(&dp->control_mutex);
	memcpy(old_addr, net->dev_addr, ETH_ALEN);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 16, 0))
	eth_hw_addr_set(net, addr->sa_data);
#else
	memcpy(net->dev_addr, addr->sa_data, net->addr_len);
#endif
	err = __ch397_set_mac_address(dev);
	if (err < 0) {
		dev_err(&net->dev, "not setting mac address %pM\n",
			addr->sa_data);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 16, 0))
		eth_hw_addr_set(net, old_addr);
#else
		memcpy(net->dev_addr, old_addr, ETH_ALEN);
#endif
	}
	mutex_unlock(&dp->control_mutex);
	return err;
}

static void ch397_set_multicast(struct net_device *net)
{
	struct usbnet *dev = netdev_priv(net);
	struct ch397_common_private *dp = dev->driver_priv;

	/* Link/reset applies the current address list before enabling carrier. */
	if (netif_carrier_ok(net)) {
		set_bit(CH397_SET_RX_MODE, &dp->flags);
		mod_delayed_work(dp->wq, &dp->schedule_work,
				 msecs_to_jiffies(CH397_MT_DELAY));
	}
}

static int ch397_stop(struct net_device *net)
{
	struct usbnet *dev = netdev_priv(net);
	struct ch397_common_private *dp = dev->driver_priv;
	int ret = usbnet_stop(net);

	/* usbnet has stopped status URBs and its link worker before we drain
	 * ours. */
	cancel_delayed_work_sync(&dp->schedule_work);
	netif_carrier_off(net);

	return ret;
}

static const struct net_device_ops ch397_netdev_ops = {
	.ndo_open = usbnet_open,
	.ndo_stop = ch397_stop,
	.ndo_start_xmit = usbnet_start_xmit,
	.ndo_tx_timeout = ch397_tx_timeout,
	.ndo_change_mtu = usbnet_change_mtu,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0))
	.ndo_get_stats64 = dev_get_tstats64,
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 12, 0))
	.ndo_get_stats64 = usbnet_get_stats64,
#endif
	.ndo_validate_addr = eth_validate_addr,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
	.ndo_eth_ioctl = ch397_ioctl,
#else
	.ndo_do_ioctl = ch397_ioctl,
#endif
	.ndo_set_mac_address = ch397_set_mac_address,
	.ndo_set_rx_mode = ch397_set_multicast,
};

static int ch397_set_speed(struct ch397_common_private *dp, u8 autoneg,
			   u16 speed, u8 duplex, u32 advertising)
{
	struct usbnet *dev = dp->dev;
	u16 bmcr = 0, anar, new_anar = 0;
	u16 adv_mask = ADVERTISE_10HALF | ADVERTISE_10FULL | ADVERTISE_100HALF |
		       ADVERTISE_100FULL;
	int ret;

	if (autoneg != AUTONEG_ENABLE && autoneg != AUTONEG_DISABLE)
		return -EINVAL;

	if (autoneg == AUTONEG_DISABLE) {
		if ((speed != SPEED_10 && speed != SPEED_100) ||
		    (duplex != DUPLEX_HALF && duplex != DUPLEX_FULL))
			return -EINVAL;

		if (speed == SPEED_100)
			bmcr |= BMCR_SPEED100;
		if (duplex == DUPLEX_FULL)
			bmcr |= BMCR_FULLDPLX;
	} else {
		if (!(advertising & CH397_ADVERTISED_MODES))
			return -EINVAL;

		ret = ch397_read_shared_word(dev, dev->mii.phy_id,
					     MII_ADVERTISE, &anar);
		if (ret < 0)
			return ret;

		new_anar = (anar & ~(adv_mask | ADVERTISE_SLCT)) |
			   ADVERTISE_CSMA;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 3, 0))
		new_anar |= ethtool_adv_to_mii_adv_t(advertising &
						     CH397_ADVERTISED_MODES);
#else
		if (advertising & PHY_ADVERTISED_10_HALF)
			new_anar |= ADVERTISE_10HALF;
		if (advertising & PHY_ADVERTISED_10_FULL)
			new_anar |= ADVERTISE_10FULL;
		if (advertising & PHY_ADVERTISED_100_HALF)
			new_anar |= ADVERTISE_100HALF;
		if (advertising & PHY_ADVERTISED_100_FULL)
			new_anar |= ADVERTISE_100FULL;
#endif

		ret = ch397_write_shared_word(dev, dev->mii.phy_id,
					      MII_ADVERTISE, new_anar);
		if (ret < 0)
			return ret;

		bmcr = BMCR_ANENABLE | BMCR_ANRESTART;
	}

	ret = ch397_write_shared_word(dev, dev->mii.phy_id, MII_BMCR, bmcr);
	if (ret < 0)
		return ret;

	dev->mii.force_media = autoneg == AUTONEG_DISABLE;
	dev->mii.full_duplex = autoneg == AUTONEG_DISABLE &&
			       duplex == DUPLEX_FULL;

	if (autoneg == AUTONEG_ENABLE)
		dev->mii.advertising = new_anar;

	return 0;
}

static u32 swap_bytes(u32 value)
{
	u32 byte1 = (value & 0xFFFF0000) >> 16;
	u32 byte2 = (value & 0x0000FFFF) << 16;

	return byte1 | byte2;
}

/* Caller holds control_mutex. Address-list locking ends before USB I/O. */
static int _ch397_set_rx_mode(struct usbnet *dev)
{
	struct ch397_common_private *dp = dev->driver_priv;
	struct net_device *net = dev->net;
	struct netdev_hw_addr *ha;
	u32 val_set = 0, val_clear;
	u32 crc_bits, hash[2], value;
	u32 multi_filter[2] = { 0 };
	int err;

	val_clear = CH397_RX_CTRL_RA | CH397_RX_CTRL_PAM | CH397_RX_CTRL_PUF |
		    CH397_RX_CTRL_PBD | CH397_RX_CTRL_PLM;

	netif_addr_lock_bh(net);
	if (net->flags & IFF_PROMISC) {
		val_set |= CH397_RX_CTRL_RA;
	} else {
		val_set |= CH397_RX_CTRL_PUF | CH397_RX_CTRL_PBD;
		if ((net->flags & IFF_ALLMULTI) ||
		    netdev_mc_count(net) > CH397_MAX_MCAST) {
			val_set |= CH397_RX_CTRL_PAM;
		} else if (!netdev_mc_empty(net)) {
			val_set |= CH397_RX_CTRL_PLM;
			netdev_for_each_mc_addr(ha, net) {
				crc_bits = crc32_le(~0, ha->addr, ETH_ALEN);
				crc_bits = bitrev32(~crc_bits) >> 26;
				multi_filter[crc_bits >> 5] |=
					1U << (crc_bits & 31);
			}
		}
	}
	netif_addr_unlock_bh(net);
	val_clear &= ~val_set;
	dp->presvd_mac_mask_set = val_set;
	dp->presvd_mac_mask_clear = val_clear;

	hash[0] = swap_bytes(multi_filter[1]);
	hash[1] = swap_bytes(multi_filter[0]);

	err = ch397_write_reg(dev, CH397_ETH_MAC_HTHR, hash[0]);
	if (err < 0) {
		netdev_err(dp->dev->net,
			   "%s, Error setting mac hash high reg.\n", __func__);
		goto out;
	}

	err = ch397_write_reg(dev, CH397_ETH_MAC_HTLR, hash[1]);
	if (err < 0) {
		netdev_err(dp->dev->net,
			   "%s, Error setting mac hash low reg.\n", __func__);
		goto out;
	}

	err = ch397_read_reg(dev, CH397_ETH_MAC_CFG, &value);
	if (err < 0)
		goto out;

	value = (value | val_set | CH397_FLOWCTRL_EN) & ~val_clear;

	if (dp->chip_info.fwver >= 0x37)
		err = ch397_write(dev, CH397_WR_ETH_MACCFG, value, 0, NULL);
	else
		err = ch397_write_reg(dev, CH397_ETH_MAC_CFG, value);
	if (!err)
		dp->presvd_mac_cfg = value;
out:
	if (err < 0)
		netdev_err(net, "%s: configuration failed: %d\n", __func__,
			   err);
	return err;
}

static void work_func(struct work_struct *work)
{
	struct ch397_common_private *dp = container_of(
		work, struct ch397_common_private, schedule_work.work);
	struct ch397_ndev_cfg cfg;
	unsigned long flags, now, deadline;
	int err;

	mutex_lock(&dp->control_mutex);
	if (test_and_clear_bit(CH397_SET_RX_MODE, &dp->flags))
		_ch397_set_rx_mode(dp->dev);

	if (test_and_clear_bit(CH397_LINK_CHG, &dp->flags)) {
		spin_lock_irqsave(&dp->state_lock, flags);
		cfg = dp->ndev_cfg;
		deadline = dp->link_change_jiffies;
		spin_unlock_irqrestore(&dp->state_lock, flags);

		if (!cfg.link) {
			/* RX-mode and pause updates may wake this work early.
			 * Keep the PHY deadline even when the work is rescheduled;
			 * time_before() also handles jiffies wraparound.
			 */
			now = jiffies;
			if (time_before(now, deadline)) {
				set_bit(CH397_LINK_CHG, &dp->flags);
				/* Wait for the PHY deadline, not an error retry. */
				if (test_bit(EVENT_DEV_OPEN, &dp->dev->flags))
					queue_delayed_work(dp->wq,
							   &dp->schedule_work,
							   deadline - now);
			} else {
				err = ch397_set_speed(dp, cfg.autoneg,
						      cfg.speed, cfg.duplex,
						      cfg.advertising);
				/* Reapplying PHY settings may clear pause advertising. */
				set_bit(CH397_SET_PHY_CFG, &dp->flags);
				if (err < 0)
					netdev_err(
						dp->dev->net,
						"ch397_set_speed error: %d\n",
						err);
			}
		}
	}

	if (test_and_clear_bit(CH397_SET_PHY_CFG, &dp->flags)) {
		err = ch397_set_flowctrl(dp->dev);
		if (err < 0)
			netdev_err(dp->dev->net,
				   "ch397_set_flowctrl error: %d\n", err);
	}

	if (test_and_clear_bit(CH397_LINK_CHG_LED_CFG, &dp->flags)) {
		err = ch397_set_ledcfg(dp->dev);
		if (err < 0)
			netdev_err(dp->dev->net,
				   "%s: Error setting LED configuration: %d\n",
				   __func__, err);
	}
	mutex_unlock(&dp->control_mutex);
}

static int ch397_bind(struct usbnet *dev, struct usb_interface *intf)
{
	int ret;
	u8 mac[ETH_ALEN];
	u32 value;
	struct ch397_common_private *dp;

	ret = usbnet_get_endpoints(dev, intf);
	if (ret)
		goto out;

	msleep(CH397_USB_DELAY);

	/* Set device led config */
	ret = ch397_set_ledcfg(dev);
	if (ret < 0) {
		printk(KERN_ERR "Error setting LED config: %d\n", ret);
		goto out;
	}

	/* Get the MAC address */
	if (ch397_get_mac_address(dev, mac) < 0) {
		printk(KERN_ERR "Error reading MAC address\n");
		ret = -ENODEV;
		goto out;
	}

	if (ch397_read_reg(dev, CH397_ETH_MAC_CFG, &value) < 0) {
		printk(KERN_ERR "Error getting mac configure value.\n");
		ret = -ENODEV;
		goto out;
	}

	/*
	 * Overwrite the auto-generated address only with good ones.
	 */
	if (is_valid_ether_addr(mac)) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 16, 0))
		eth_hw_addr_set(dev->net, mac);
#else
		memcpy(dev->net->dev_addr, mac, ETH_ALEN);
#endif
	} else {
		printk(KERN_WARNING
		       "ch397: No valid MAC address in EEPROM, using %pM\n",
		       dev->net->dev_addr);
		ret = __ch397_set_mac_address(dev);
		if (ret < 0)
			goto out;
	}

	dev->net->netdev_ops = &ch397_netdev_ops;
	dev->net->ethtool_ops = &ch397_ethtool_ops;
	dev->rx_urb_size = CH397_RX_URB_SIZE;

	dev->net->needed_headroom = CH397_TX_OVERHEAD;
	dev->net->needed_tailroom = 4;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0))
	dev->net->min_mtu = 68;
	dev->net->max_mtu = ETH_DATA_LEN;
#endif

	dev->mii.dev = dev->net;
	dev->mii.mdio_read = ch397_mdio_read;
	dev->mii.mdio_write = ch397_mdio_write;
	dev->mii.phy_id_mask = 0x1f;
	dev->mii.reg_num_mask = 0x1f;
	dev->mii.phy_id = 0;

	dev->driver_priv =
		kzalloc(sizeof(struct ch397_common_private), GFP_KERNEL);
	if (!dev->driver_priv)
		return -ENOMEM;

	dp = dev->driver_priv;
	dp->dev = dev;
	mutex_init(&dp->control_mutex);
	spin_lock_init(&dp->state_lock);
	dp->ndev_cfg.autoneg = AUTONEG_ENABLE;
	dp->ndev_cfg.speed = SPEED_100;
	dp->ndev_cfg.duplex = DUPLEX_FULL;
	dp->ndev_cfg.advertising = CH397_ADVERTISED_MODES;
	dp->ndev_cfg.link_duplex = DUPLEX_UNKNOWN;
	dp->presvd_mac_cfg = value;

	dp->wq = create_workqueue("ch397_link_workqueue");
	if (!dp->wq) {
		printk(KERN_ERR "ch397 create link workqueue failed!\n");
		ret = -ENOMEM;
		goto free_private;
	}

	INIT_DELAYED_WORK(&dp->schedule_work, work_func);

	ret = ch397_set_flowctrl(dev);
	if (ret < 0)
		goto destroy_wq;

	if (ch397_get_info(dev, &dp->chip_info) < 0) {
		printk(KERN_ERR "Error getting chip info\n");
		ret = -ENODEV;
		goto destroy_wq;
	}

	/* Apply defaults once; opening the netdev must not restart autoneg. */
	ret = ch397_set_speed(dp, dp->ndev_cfg.autoneg, dp->ndev_cfg.speed,
			      dp->ndev_cfg.duplex, dp->ndev_cfg.advertising);
	if (ret < 0)
		goto destroy_wq;

	netif_carrier_off(dev->net);
	return 0;

destroy_wq:
	destroy_workqueue(dp->wq);
free_private:
	kfree(dev->driver_priv);
	dev->driver_priv = NULL;
out:
	return ret;
}

static void ch397_unbind(struct usbnet *dev, struct usb_interface *intf)
{
	struct ch397_common_private *dp = dev->driver_priv;

	cancel_delayed_work_sync(&dp->schedule_work);
	flush_workqueue(dp->wq);
	destroy_workqueue(dp->wq);
	kfree(dev->driver_priv);
	dev->driver_priv = NULL;
}

static int ch397_reset(struct usbnet *dev)
{
	struct ch397_common_private *dp = dev->driver_priv;
	unsigned long flags;

	/* usbnet_open() calls this without resetting the PHY. Preserve its
	 * settings and any pending link reapply, which status will reschedule.
	 * Link-reset restores the MAC address, RX filters and MAC flow control.
	 */
	mutex_lock(&dp->control_mutex);

	spin_lock_irqsave(&dp->state_lock, flags);
	clear_bit(CH397_SET_RX_MODE, &dp->flags);
	dp->link_change_count++;
	dp->ndev_cfg.link = false;
	dp->ndev_cfg.link_speed = 0;
	dp->ndev_cfg.link_duplex = DUPLEX_UNKNOWN;
	netif_carrier_off(dev->net);
	spin_unlock_irqrestore(&dp->state_lock, flags);

	mutex_unlock(&dp->control_mutex);

	return 0;
}

static int ch397_rx_fixup(struct usbnet *dev, struct sk_buff *skb)
{
	u32 offset = 0, frame_len, padded_len;
	struct sk_buff *frame;

	if (skb->len > CH397_RX_URB_SIZE)
		goto bad_length;

	while (offset < skb->len) {
		if (skb->len - offset < CH397_RX_OVERHEAD)
			goto bad_length;

		frame_len = get_unaligned_le32(skb->data + offset);
		offset += CH397_RX_OVERHEAD;

		if (frame_len < ETH_ZLEN ||
		    frame_len > ETH_FRAME_LEN + VLAN_HLEN)
			goto bad_length;

		padded_len = ALIGN(frame_len, 4);

		if (padded_len > skb->len - offset)
			goto bad_length;

		frame = netdev_alloc_skb_ip_align(dev->net, frame_len);
		if (!frame) {
			dev->net->stats.rx_dropped++;
			/* This record is complete, so later frames remain
			 * recoverable. */
			offset += padded_len;
			continue;
		}

		memcpy(skb_put(frame, frame_len), skb->data + offset,
		       frame_len);
		usbnet_skb_return(dev, frame);
		offset += padded_len;
	}
	return 1;

bad_length:
	dev->net->stats.rx_length_errors++;
	return 0;
}

static struct sk_buff *ch397_tx_fixup(struct usbnet *dev, struct sk_buff *skb,
				      gfp_t flags)
{
	u32 frame_len, packet_len, padded_len, pad, max_len;
	u16 proto;
	struct sk_buff *copy;

	/* EtherType access and VLAN insertion need the existing Ethernet
	 * header in linear data; headroom only reserves space before it.
	 */
	if (!pskb_may_pull(skb, ETH_HLEN))
		goto drop;

	/* No hardware VLAN insertion is advertised. Materialize an unexpected
	 * metadata tag as well, preserving its 802.1Q/802.1ad protocol and TCI.
	 */
	if (skb_vlan_tag_present(skb)) {
		skb = __vlan_hwaccel_push_inside(skb);
		if (!skb)
			goto dropped;
	}

	proto = get_unaligned_be16(skb->data + 2 * ETH_ALEN);
	max_len = ETH_FRAME_LEN;

	if (proto == ETH_P_8021Q || proto == ETH_P_8021AD) {
		/* No VLAN fields are read here; validate the full header length. */
		if (skb->len < VLAN_ETH_HLEN)
			goto drop;
		max_len += VLAN_HLEN;
	}

	if (skb->len > max_len)
		goto drop;

	frame_len = skb->len;
	packet_len = max_t(u32, frame_len, ETH_ZLEN);
	padded_len = ALIGN(packet_len, 4);
	pad = padded_len - frame_len;

	if (skb_shared(skb) || skb_cloned(skb) || skb_is_nonlinear(skb) ||
	    skb_headroom(skb) < CH397_TX_OVERHEAD || skb_tailroom(skb) < pad) {
		copy = skb_copy_expand(skb, CH397_TX_OVERHEAD, pad, flags);
		dev_kfree_skb_any(skb);
		skb = copy;
		if (!skb)
			goto dropped;
	}

	if (pad)
		memset(skb_put(skb, pad), 0, pad);

	memset(skb_push(skb, CH397_TX_OVERHEAD), 0, CH397_TX_OVERHEAD);
	put_unaligned_le32(packet_len, skb->data);

	usbnet_set_skb_tx_stats(skb, 1, (long)frame_len - skb->len);
	return skb;

drop:
	dev_kfree_skb_any(skb);
dropped:
	dev->net->stats.tx_dropped++;
	return NULL;
}

static void ch397_status(struct usbnet *dev, struct urb *urb)
{
	struct ch397_common_private *dp = dev->driver_priv;
	struct ch397_intr_stats *event = &dp->ch397_intr;
	struct ch397_intr_event tmp;
	struct ch397_ndev_cfg *cfg = &dp->ndev_cfg;
	int link;
	unsigned long flags, now;

	if (urb->status || urb->actual_length < sizeof(tmp))
		return;

	memcpy(&tmp, urb->transfer_buffer, sizeof(tmp));

	spin_lock_irqsave(&dp->state_lock, flags);

	event->link_stat = le32_to_cpu(tmp.link_stat);
	event->rx_packets = le32_to_cpu(tmp.rx_packets);
	event->rx_overflow_cnt = le16_to_cpu(tmp.rx_overflow_cnt);
	event->rx_crc_cnt = le16_to_cpu(tmp.rx_crc_cnt);
	event->tx_packets = le32_to_cpu(tmp.tx_packets);

	link = !!(event->link_stat & CH397_LINK_RDY);
	if (link != cfg->link) {
		dp->link_change_count++;
		if (!link) {
			dp->link_change_jiffies =
				jiffies +
				msecs_to_jiffies(
					cfg->autoneg == AUTONEG_DISABLE ?
						CH397_FORCED_REAPPLY_DELAY :
						CH397_LINK_REAPPLY_DELAY);
			set_bit(CH397_LINK_CHG, &dp->flags);
			set_bit(CH397_SET_PHY_CFG, &dp->flags);
			set_bit(CH397_LINK_CHG_LED_CFG, &dp->flags);
			netdev_info(dev->net, "link down\n");
		}
	}

	cfg->link = link;
	if (link) {
		cfg->link_speed = (event->link_stat & CH397_LINK_SPEED) ?
					  SPEED_10 :
					  SPEED_100;
		cfg->link_duplex = (event->link_stat & CH397_DUPLEX_MODE) ?
					   DUPLEX_FULL :
					   DUPLEX_HALF;
	} else {
		cfg->link_speed = 0;
		cfg->link_duplex = DUPLEX_UNKNOWN;
	}

	/* Retry link setup on later link-up status while carrier remains off. */
	if (link != netif_carrier_ok(dev->net))
		usbnet_link_change(dev, link, link);

	/* Restore pause advertising promptly, even if the link has recovered. */
	if (test_bit(CH397_SET_PHY_CFG, &dp->flags) ||
		test_bit(CH397_LINK_CHG_LED_CFG, &dp->flags)) {
		/* Cancel any pending work to avoid a double update. */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0))
		mod_delayed_work(dp->wq, &dp->schedule_work, 0);
#else
		cancel_delayed_work(&dp->schedule_work);
		queue_delayed_work(dp->wq, &dp->schedule_work, 0);
#endif
	} else if (test_bit(CH397_LINK_CHG, &dp->flags)) {
		/* Resume pending link reapply without changing its deadline. */
		now = jiffies;
		queue_delayed_work(dp->wq, &dp->schedule_work,
				   time_before(now, dp->link_change_jiffies) ?
					   dp->link_change_jiffies - now :
					   0);
	}
	spin_unlock_irqrestore(&dp->state_lock, flags);
}

static int ch397_link_reset(struct usbnet *dev)
{
	struct ch397_common_private *dp = dev->driver_priv;
	struct ch397_ndev_cfg cfg;
	unsigned long flags;
	unsigned int link_change_count;
	bool carrier, announce = false;
	int ret = 0, lpa = 0;

	mutex_lock(&dp->control_mutex);

	spin_lock_irqsave(&dp->state_lock, flags);

	cfg = dp->ndev_cfg;
	link_change_count = dp->link_change_count;
	carrier = netif_carrier_ok(dev->net);
	if (!cfg.link && carrier)
		usbnet_link_change(dev, false, false);

	spin_unlock_irqrestore(&dp->state_lock, flags);

	if (!cfg.link || carrier)
		goto out;

	ret = __ch397_set_mac_address(dev);
	if (ret < 0)
		goto out;

	ret = _ch397_set_rx_mode(dev);
	if (ret < 0)
		goto out;

	lpa = ch397_mdio_read(dev->net, dev->mii.phy_id, MII_LPA);

	/* USB commands sleep. Discard setup if the link changed during I/O,
	 * even if it is already up again. The new link needs its own setup.
	 */
	spin_lock_irqsave(&dp->state_lock, flags);

	cfg = dp->ndev_cfg;
	if (cfg.link && !netif_carrier_ok(dev->net) &&
	    link_change_count == dp->link_change_count) {
		announce = true;
		dev->mii.full_duplex = cfg.link_duplex == DUPLEX_FULL;
		usbnet_link_change(dev, true, false);
	}

	spin_unlock_irqrestore(&dp->state_lock, flags);
out:
	mutex_unlock(&dp->control_mutex);
	if (announce)
		netdev_info(
			dev->net,
			"link up, %uMbps, %s-duplex, autoneg-%s, lpa 0x%04x\n",
			cfg.link_speed,
			cfg.link_duplex == DUPLEX_FULL ? "full" : "half",
			cfg.autoneg == AUTONEG_ENABLE ? "on" : "off",
			lpa < 0 ? 0 : lpa);
	return ret;
}

static const struct driver_info ch397_info = {
	.description = "WCH CH397 USB2.0 Ethernet",
	.flags = FLAG_ETHER | FLAG_LINK_INTR | FLAG_MULTI_PACKET |
		 FLAG_SEND_ZLP,
	.bind = ch397_bind,
	.unbind = ch397_unbind,
	.reset = ch397_reset,
	.rx_fixup = ch397_rx_fixup,
	.tx_fixup = ch397_tx_fixup,
	.status = ch397_status,
	.link_reset = ch397_link_reset,
};

static int ch397_probe(struct usb_interface *intf,
		       const struct usb_device_id *id)
{
	struct usb_device *udev = interface_to_usbdev(intf);

	if (udev->actconfig->desc.bConfigurationValue != 1) {
		usb_driver_set_configuration(udev, 1);
		return -ENODEV;
	}

	printk(KERN_INFO "ch397 device probe, driver version: %s\n",
	       VERSION_DESC);

	msleep(20);

	return usbnet_probe(intf, id);
}

static const struct usb_device_id ch397_ids[] = {
	{
		USB_DEVICE_INTERFACE_CLASS(
			0x1a86, 0x5397, USB_CLASS_VENDOR_SPEC), /* ch397 chip */
		.driver_info = (unsigned long)&ch397_info,
	},

	{
		USB_DEVICE(0x1a86, 0x5397), /* ch397 chip */
		.driver_info = (unsigned long)&ch397_info,
	},

	{
		USB_DEVICE_INTERFACE_CLASS(
			0x1a86, 0x5396, USB_CLASS_VENDOR_SPEC), /* ch396 chip */
		.driver_info = (unsigned long)&ch397_info,
	},

	{
		USB_DEVICE(0x1a86, 0x5396), /* ch396 chip */
		.driver_info = (unsigned long)&ch397_info,
	},

	{
		USB_DEVICE_INTERFACE_CLASS(
			0x1a86, 0x5395, USB_CLASS_VENDOR_SPEC), /* ch339 chip */
		.driver_info = (unsigned long)&ch397_info,
	},

	{
		USB_DEVICE(0x1a86, 0x5395), /* ch339 chip */
		.driver_info = (unsigned long)&ch397_info,
	},

	{
		USB_DEVICE_INTERFACE_CLASS(
			0x1a86, 0x5394, USB_CLASS_VENDOR_SPEC), /* ch336 chip */
		.driver_info = (unsigned long)&ch397_info,
	},

	{
		USB_DEVICE(0x1a86, 0x5394), /* ch336 chip */
		.driver_info = (unsigned long)&ch397_info,
	},

	{},
};

MODULE_DEVICE_TABLE(usb, ch397_ids);

static struct usb_driver ch397_driver = {
	.name = "usb_ch397",
	.id_table = ch397_ids,
	.probe = ch397_probe,
	.disconnect = usbnet_disconnect,
	.suspend = usbnet_suspend,
	.resume = usbnet_resume,
	.disable_hub_initiated_lpm = 1,
	.supports_autosuspend = 1,
};

module_usb_driver(ch397_driver);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_VERSION(VERSION_DESC);
MODULE_LICENSE("GPL");
