// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Simple synchronous userspace interface to SPI devices
 *
 * Copyright (C) 2006 SWAPP
 *	Andrea Paterniani <a.paterniani@swapp-eng.it>
 * Copyright (C) 2007 David Brownell (simplification, cleanup)
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/compat.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/acpi.h>

#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>

#include <linux/uaccess.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>
#include "esamlib.h"


/*
 * This supports access to SPI devices using normal userspace I/O calls.
 * Note that while traditional UNIX/POSIX I/O semantics are half duplex,
 * and often mask message boundaries, full SPI support requires full duplex
 * transfers.  There are several kinds of internal message boundaries to
 * handle chipselect management and other protocol options.
 *
 * SPI has a character major number assigned.  We allocate minor numbers
 * dynamically using a bitmask.  You must use hotplug tools, such as udev
 * (or mdev with busybox) to create and destroy the /dev/spidevB.C device
 * nodes, since there is no fixed association of minor numbers with any
 * particular SPI bus or device.
 */
#define SPIDEV_MAJOR			130	/* assigned */
#define N_SPI_MINORS			32	/* ... up to 256 */

//#define ESAM_DEBUG
static DECLARE_BITMAP(minors, N_SPI_MINORS);


/* Bit masks for spi_device.mode management.  Note that incorrect
 * settings for some settings can cause *lots* of trouble for other
 * devices on a shared bus:
 *
 *  - CS_HIGH ... this device will be active when it shouldn't be
 *  - 3WIRE ... when active, it won't behave as it should
 *  - NO_CS ... there will be no explicit message boundaries; this
 *	is completely incompatible with the shared bus model
 *  - READY ... transfers may proceed when they shouldn't.
 *
 * REVISIT should changing those flags be privileged?
 */
#define SPI_MODE_MASK		(SPI_CPHA | SPI_CPOL | SPI_CS_HIGH \
				| SPI_LSB_FIRST | SPI_3WIRE | SPI_LOOP \
				| SPI_NO_CS | SPI_READY | SPI_TX_DUAL \
				| SPI_TX_QUAD | SPI_TX_OCTAL | SPI_RX_DUAL \
				| SPI_RX_QUAD | SPI_RX_OCTAL)

struct power_ctrl {
	int gpio;
	int active_low;
	void (*power_on)(struct power_ctrl *);
	void (*power_off)(struct power_ctrl *);
	int (*power_init)(struct power_ctrl *);
	void (*power_exit)(struct power_ctrl *);
};

struct spidev_data {
	dev_t			devt;
	spinlock_t		spi_lock;
	struct spi_device	*spi;
	struct list_head	device_entry;

	/* TX/RX buffers are NULL unless this device is open (users > 0) */
	struct mutex		buf_lock;
	unsigned		users;
	u8			*tx_buffer;
	u8			*rx_buffer;
	u32			speed_hz;
	struct power_ctrl	*pwctl;
	int			cs_gpio;
};

static LIST_HEAD(device_list);
static DEFINE_MUTEX(device_list_lock);

static unsigned bufsiz = 4096;
module_param(bufsiz, uint, S_IRUGO);
MODULE_PARM_DESC(bufsiz, "data bytes in biggest supported SPI message");

/*-------------------------------------------------------------------------*/

static void esam_reset(struct power_ctrl *pwctl)
{
#if 0
	if (pwctl && pwctl->power_off)
		pwctl->power_off(pwctl);
#endif
	mdelay(30);
	if (pwctl && pwctl->power_on)
		pwctl->power_on(pwctl);
	mdelay(30);
}

static void esam_ssn_control(int gpio, int level)
{
	gpio_set_value(gpio, level);
}

/*-------------------------------------------------------------------------*/

static ssize_t
spidev_sync(struct spidev_data *spidev, struct spi_message *message)
{
	int status;
	struct spi_device *spi;

	spin_lock_irq(&spidev->spi_lock);
	spi = spidev->spi;
	spin_unlock_irq(&spidev->spi_lock);

	if (spi == NULL)
		status = -ESHUTDOWN;
	else
		status = spi_sync(spi, message);

	if (status == 0)
		status = message->actual_length;

	return status;
}

static inline ssize_t
spidev_sync_write(struct spidev_data *spidev, size_t len)
{
	struct spi_transfer	t = {
			.tx_buf		= spidev->tx_buffer,
			.len		= len,
			.speed_hz	= spidev->speed_hz,
		};
	struct spi_message	m;

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	return spidev_sync(spidev, &m);
}

static inline ssize_t
spidev_sync_read(struct spidev_data *spidev, size_t len)
{
	struct spi_transfer	t = {
			.rx_buf		= spidev->rx_buffer,
			.len		= len,
			.speed_hz	= spidev->speed_hz,
		};
	struct spi_message	m;

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	return spidev_sync(spidev, &m);
}

/*-------------------------------------------------------------------------*/

static u8 data[4096];
/* Read-only message with current device setup */
static ssize_t
spidev_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	struct spidev_data	*spidev;
	ssize_t			status;
	size_t			enc = 0;
	unsigned long		timeout = 0;
	int			datalen = 0;
#ifdef ESAM_DEBUG
	int i;
#endif

	/* chipselect only toggles at start or end of operation */
	/* min size = sw1 + sw2 + le1 + len2 + lrc = 5 */
	if (count > bufsiz || count < 5)
		return -EMSGSIZE;

	spidev = filp->private_data;

	mutex_lock(&spidev->buf_lock);

	udelay(10);
	esam_ssn_control(spidev->cs_gpio, 0);
	udelay(10);

	do {
		status = spidev_sync_read(spidev, 1);
		if (status <= 0) {
			pr_err("spidev_sync_read for head failed, status=%d\n", status);
			break;
		}
		if (spidev->rx_buffer[0] == 0x55)
			break;
		udelay(80);
		timeout += 80;
	} while (timeout < 2000000);

#ifdef ESAM_DEBUG
	printk("end recv head byte(0x55), status = %d, timeout = %lu\n", status, timeout);
#endif
	if (spidev->rx_buffer[0] != 0x55) {
		status = -ETIMEDOUT;
		goto out;
	}
	data[0] = 0x55;
	enc++;

	status = spidev_sync_read(spidev, 4);
	if (status != 4) {
		pr_err("esam spidev_sync_read 4 bytes failed, status=%d\n", status);
		status = -EINVAL;
		goto out;
	}
#ifdef ESAM_DEBUG
	pr_info("recv 4 bytes:");
	for (i = 0; i < status; i++) {
		pr_info("0x%x ", spidev->rx_buffer[i]);
	}
	pr_info("\n");
#endif

	memcpy(data + enc, spidev->rx_buffer, 4);
	enc += 4;
	datalen += ((spidev->rx_buffer[2] << 8) | spidev->rx_buffer[3]) & 0x7FF;
#ifdef ESAM_DEBUG
	printk("datalen is %d, count = %d\n", datalen, count);
#endif
	if ((datalen + 6) > count) {
		status = -EMSGSIZE;
		goto out;
	}

	status = spidev_sync_read(spidev, datalen + 1);
	if (status != (datalen + 1)) {
		pr_err("esam spidev_sync_read %d bytes failed, status=%d\n",
				datalen + 1, status);
		status = -EINVAL;
		goto out;
	}

	memcpy(data + enc, spidev->rx_buffer, datalen + 1);
	enc += (datalen + 1);
	status = (ssize_t)enc;
out:
	esam_ssn_control(spidev->cs_gpio, 1);
	udelay(10);

	if (status > 0) {
		unsigned long	missing;

		missing = copy_to_user(buf, data, status);
		if (missing == status)
			status = -EFAULT;
		else
			status = status - missing;
	}
	mutex_unlock(&spidev->buf_lock);

	return status;
}

/* Write-only message with current device setup */
static ssize_t
spidev_write(struct file *filp, const char __user *buf,
		size_t count, loff_t *f_pos)
{
	struct spidev_data	*spidev;
	ssize_t			status;
	unsigned long		missing;
#ifdef ESAM_DEBUG
	int i;
#endif

	/* chipselect only toggles at start or end of operation */
	if (count > bufsiz)
		return -EMSGSIZE;

	spidev = filp->private_data;

	mutex_lock(&spidev->buf_lock);
	missing = copy_from_user(spidev->tx_buffer, buf, count);
	if (missing == 0) {
		esam_ssn_control(spidev->cs_gpio, 0);
		udelay(10);
		status = spidev_sync_write(spidev, count);
#ifdef ESAM_DEBUG
		pr_info("esam write:");
		for (i = 0; i < count; i++)
			pr_info("0x%x ", spidev->tx_buffer[i]);
		pr_info("\n");
#endif
		udelay(10);
		esam_ssn_control(spidev->cs_gpio, 1);
	} else
		status = -EFAULT;
	spidev->tx_buffer[0] = 0x00;
	mutex_unlock(&spidev->buf_lock);

	return status;
}

static int spidev_message(struct spidev_data *spidev,
		struct spi_ioc_transfer *u_xfers, unsigned n_xfers)
{
	struct spi_message	msg;
	struct spi_transfer	*k_xfers;
	struct spi_transfer	*k_tmp;
	struct spi_ioc_transfer *u_tmp;
	unsigned		n, total, tx_total, rx_total;
	u8			*tx_buf, *rx_buf;
	int			status = -EFAULT;

	spi_message_init(&msg);
	k_xfers = kcalloc(n_xfers, sizeof(*k_tmp), GFP_KERNEL);
	if (k_xfers == NULL)
		return -ENOMEM;

	/* Construct spi_message, copying any tx data to bounce buffer.
	 * We walk the array of user-provided transfers, using each one
	 * to initialize a kernel version of the same transfer.
	 */
	tx_buf = spidev->tx_buffer;
	rx_buf = spidev->rx_buffer;
	total = 0;
	tx_total = 0;
	rx_total = 0;
	for (n = n_xfers, k_tmp = k_xfers, u_tmp = u_xfers;
			n;
			n--, k_tmp++, u_tmp++) {
		/* Ensure that also following allocations from rx_buf/tx_buf will meet
		 * DMA alignment requirements.
		 */
		unsigned int len_aligned = ALIGN(u_tmp->len, ARCH_KMALLOC_MINALIGN);

		k_tmp->len = u_tmp->len;

		total += k_tmp->len;
		/* Since the function returns the total length of transfers
		 * on success, restrict the total to positive int values to
		 * avoid the return value looking like an error.  Also check
		 * each transfer length to avoid arithmetic overflow.
		 */
		if (total > INT_MAX || k_tmp->len > INT_MAX) {
			status = -EMSGSIZE;
			goto done;
		}

		if (u_tmp->rx_buf) {
			/* this transfer needs space in RX bounce buffer */
			rx_total += len_aligned;
			if (rx_total > bufsiz) {
				status = -EMSGSIZE;
				goto done;
			}
			k_tmp->rx_buf = rx_buf;
			rx_buf += len_aligned;
		}
		if (u_tmp->tx_buf) {
			/* this transfer needs space in TX bounce buffer */
			tx_total += len_aligned;
			if (tx_total > bufsiz) {
				status = -EMSGSIZE;
				goto done;
			}
			k_tmp->tx_buf = tx_buf;
			if (copy_from_user(tx_buf, (const u8 __user *)
						(uintptr_t) u_tmp->tx_buf,
					u_tmp->len))
				goto done;
			tx_buf += len_aligned;
		}

		k_tmp->cs_change = !!u_tmp->cs_change;
		k_tmp->tx_nbits = u_tmp->tx_nbits;
		k_tmp->rx_nbits = u_tmp->rx_nbits;
		k_tmp->bits_per_word = u_tmp->bits_per_word;
		k_tmp->delay.value = u_tmp->delay_usecs;
		k_tmp->delay.unit = SPI_DELAY_UNIT_USECS;
		k_tmp->speed_hz = u_tmp->speed_hz;
		k_tmp->word_delay.value = u_tmp->word_delay_usecs;
		k_tmp->word_delay.unit = SPI_DELAY_UNIT_USECS;
		if (!k_tmp->speed_hz)
			k_tmp->speed_hz = spidev->speed_hz;
#ifdef VERBOSE
		dev_dbg(&spidev->spi->dev,
			"  xfer len %u %s%s%s%dbits %u usec %u usec %uHz\n",
			k_tmp->len,
			k_tmp->rx_buf ? "rx " : "",
			k_tmp->tx_buf ? "tx " : "",
			k_tmp->cs_change ? "cs " : "",
			k_tmp->bits_per_word ? : spidev->spi->bits_per_word,
			k_tmp->delay.value,
			k_tmp->word_delay.value,
			k_tmp->speed_hz ? : spidev->spi->max_speed_hz);
#endif
		spi_message_add_tail(k_tmp, &msg);
	}

	status = spidev_sync(spidev, &msg);
	if (status < 0)
		goto done;

	/* copy any rx data out of bounce buffer */
	for (n = n_xfers, k_tmp = k_xfers, u_tmp = u_xfers;
			n;
			n--, k_tmp++, u_tmp++) {
		if (u_tmp->rx_buf) {
			if (copy_to_user((u8 __user *)
					(uintptr_t) u_tmp->rx_buf, k_tmp->rx_buf,
					u_tmp->len)) {
				status = -EFAULT;
				goto done;
			}
		}
	}
	status = total;

done:
	kfree(k_xfers);
	return status;
}

static struct spi_ioc_transfer *
spidev_get_ioc_message(unsigned int cmd, struct spi_ioc_transfer __user *u_ioc,
		unsigned *n_ioc)
{
	u32	tmp;

	/* Check type, command number and direction */
	if (_IOC_TYPE(cmd) != SPI_IOC_MAGIC
			|| _IOC_NR(cmd) != _IOC_NR(SPI_IOC_MESSAGE(0))
			|| _IOC_DIR(cmd) != _IOC_WRITE)
		return ERR_PTR(-ENOTTY);

	tmp = _IOC_SIZE(cmd);
	if ((tmp % sizeof(struct spi_ioc_transfer)) != 0)
		return ERR_PTR(-EINVAL);
	*n_ioc = tmp / sizeof(struct spi_ioc_transfer);
	if (*n_ioc == 0)
		return NULL;

	/* copy into scratch area */
	return memdup_user(u_ioc, tmp);
}

static long
spidev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int			retval = 0;
	struct spidev_data	*spidev;
	struct spi_device	*spi;
	u32			tmp;
	unsigned		n_ioc;
	struct spi_ioc_transfer	*ioc;

	/* Check type and command number */
	if (_IOC_TYPE(cmd) != SPI_IOC_MAGIC)
		return -ENOTTY;

	/* guard against device removal before, or while,
	 * we issue this ioctl.
	 */
	spidev = filp->private_data;
	spin_lock_irq(&spidev->spi_lock);
	spi = spi_dev_get(spidev->spi);
	spin_unlock_irq(&spidev->spi_lock);

	if (spi == NULL)
		return -ESHUTDOWN;

	/* use the buffer lock here for triple duty:
	 *  - prevent I/O (from us) so calling spi_setup() is safe;
	 *  - prevent concurrent SPI_IOC_WR_* from morphing
	 *    data fields while SPI_IOC_RD_* reads them;
	 *  - SPI_IOC_MESSAGE needs the buffer locked "normally".
	 */
	mutex_lock(&spidev->buf_lock);

	switch (cmd) {
	case RESET_ESAM:
		esam_reset(spidev->pwctl);
		break;
	/* read requests */
	case SPI_IOC_RD_MODE:
		retval = put_user(spi->mode & SPI_MODE_MASK,
					(__u8 __user *)arg);
		break;
	case SPI_IOC_RD_MODE32:
		retval = put_user(spi->mode & SPI_MODE_MASK,
					(__u32 __user *)arg);
		break;
	case SPI_IOC_RD_LSB_FIRST:
		retval = put_user((spi->mode & SPI_LSB_FIRST) ?  1 : 0,
					(__u8 __user *)arg);
		break;
	case SPI_IOC_RD_BITS_PER_WORD:
		retval = put_user(spi->bits_per_word, (__u8 __user *)arg);
		break;
	case SPI_IOC_RD_MAX_SPEED_HZ:
		retval = put_user(spidev->speed_hz, (__u32 __user *)arg);
		break;

	/* write requests */
	case SPI_IOC_WR_MODE:
	case SPI_IOC_WR_MODE32:
		if (cmd == SPI_IOC_WR_MODE)
			retval = get_user(tmp, (u8 __user *)arg);
		else
			retval = get_user(tmp, (u32 __user *)arg);
		if (retval == 0) {
			struct spi_controller *ctlr = spi->controller;
			u32	save = spi->mode;

			if (tmp & ~SPI_MODE_MASK) {
				retval = -EINVAL;
				break;
			}

			if (ctlr->use_gpio_descriptors && ctlr->cs_gpiods &&
			    ctlr->cs_gpiods[spi->chip_select])
				tmp |= SPI_CS_HIGH;

			tmp |= spi->mode & ~SPI_MODE_MASK;
			spi->mode = (u16)tmp;
			retval = spi_setup(spi);
			if (retval < 0)
				spi->mode = save;
			else
				dev_dbg(&spi->dev, "spi mode %x\n", tmp);
		}
		break;
	case SPI_IOC_WR_LSB_FIRST:
		retval = get_user(tmp, (__u8 __user *)arg);
		if (retval == 0) {
			u32	save = spi->mode;

			if (tmp)
				spi->mode |= SPI_LSB_FIRST;
			else
				spi->mode &= ~SPI_LSB_FIRST;
			retval = spi_setup(spi);
			if (retval < 0)
				spi->mode = save;
			else
				dev_dbg(&spi->dev, "%csb first\n",
						tmp ? 'l' : 'm');
		}
		break;
	case SPI_IOC_WR_BITS_PER_WORD:
		retval = get_user(tmp, (__u8 __user *)arg);
		if (retval == 0) {
			u8	save = spi->bits_per_word;

			spi->bits_per_word = tmp;
			retval = spi_setup(spi);
			if (retval < 0)
				spi->bits_per_word = save;
			else
				dev_dbg(&spi->dev, "%d bits per word\n", tmp);
		}
		break;
	case SPI_IOC_WR_MAX_SPEED_HZ:
		retval = get_user(tmp, (__u32 __user *)arg);
		if (retval == 0) {
			u32	save = spi->max_speed_hz;

			spi->max_speed_hz = tmp;
			retval = spi_setup(spi);
			if (retval == 0) {
				spidev->speed_hz = tmp;
				dev_dbg(&spi->dev, "%d Hz (max)\n",
					spidev->speed_hz);
			}
			spi->max_speed_hz = save;
		}
		break;

	default:
		/* segmented and/or full-duplex I/O request */
		/* Check message and copy into scratch area */
		ioc = spidev_get_ioc_message(cmd,
				(struct spi_ioc_transfer __user *)arg, &n_ioc);
		if (IS_ERR(ioc)) {
			retval = PTR_ERR(ioc);
			break;
		}
		if (!ioc)
			break;	/* n_ioc is also 0 */

		/* translate to spi_message, execute */
		retval = spidev_message(spidev, ioc, n_ioc);
		kfree(ioc);
		break;
	}

	mutex_unlock(&spidev->buf_lock);
	spi_dev_put(spi);
	return retval;
}

#ifdef CONFIG_COMPAT
static long
spidev_compat_ioc_message(struct file *filp, unsigned int cmd,
		unsigned long arg)
{
	struct spi_ioc_transfer __user	*u_ioc;
	int				retval = 0;
	struct spidev_data		*spidev;
	struct spi_device		*spi;
	unsigned			n_ioc, n;
	struct spi_ioc_transfer		*ioc;

	u_ioc = (struct spi_ioc_transfer __user *) compat_ptr(arg);

	/* guard against device removal before, or while,
	 * we issue this ioctl.
	 */
	spidev = filp->private_data;
	spin_lock_irq(&spidev->spi_lock);
	spi = spi_dev_get(spidev->spi);
	spin_unlock_irq(&spidev->spi_lock);

	if (spi == NULL)
		return -ESHUTDOWN;

	/* SPI_IOC_MESSAGE needs the buffer locked "normally" */
	mutex_lock(&spidev->buf_lock);

	/* Check message and copy into scratch area */
	ioc = spidev_get_ioc_message(cmd, u_ioc, &n_ioc);
	if (IS_ERR(ioc)) {
		retval = PTR_ERR(ioc);
		goto done;
	}
	if (!ioc)
		goto done;	/* n_ioc is also 0 */

	/* Convert buffer pointers */
	for (n = 0; n < n_ioc; n++) {
		ioc[n].rx_buf = (uintptr_t) compat_ptr(ioc[n].rx_buf);
		ioc[n].tx_buf = (uintptr_t) compat_ptr(ioc[n].tx_buf);
	}

	/* translate to spi_message, execute */
	retval = spidev_message(spidev, ioc, n_ioc);
	kfree(ioc);

done:
	mutex_unlock(&spidev->buf_lock);
	spi_dev_put(spi);
	return retval;
}

static long
spidev_compat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	if (_IOC_TYPE(cmd) == SPI_IOC_MAGIC
			&& _IOC_NR(cmd) == _IOC_NR(SPI_IOC_MESSAGE(0))
			&& _IOC_DIR(cmd) == _IOC_WRITE)
		return spidev_compat_ioc_message(filp, cmd, arg);

	return spidev_ioctl(filp, cmd, (unsigned long)compat_ptr(arg));
}
#else
#define spidev_compat_ioctl NULL
#endif /* CONFIG_COMPAT */

static int spidev_open(struct inode *inode, struct file *filp)
{
	struct spidev_data	*spidev;
	int			status = -ENXIO;

	mutex_lock(&device_list_lock);

	list_for_each_entry(spidev, &device_list, device_entry) {
		if (spidev->devt == inode->i_rdev) {
			status = 0;
			break;
		}
	}

	if (status) {
		pr_debug("spidev: nothing for minor %d\n", iminor(inode));
		goto err_find_dev;
	}

	if (!spidev->tx_buffer) {
		spidev->tx_buffer = kmalloc(bufsiz, GFP_KERNEL);
		if (!spidev->tx_buffer) {
			dev_dbg(&spidev->spi->dev, "open/ENOMEM\n");
			status = -ENOMEM;
			goto err_find_dev;
		}
	}

	if (!spidev->rx_buffer) {
		spidev->rx_buffer = kmalloc(bufsiz, GFP_KERNEL);
		if (!spidev->rx_buffer) {
			dev_dbg(&spidev->spi->dev, "open/ENOMEM\n");
			status = -ENOMEM;
			goto err_alloc_rx_buf;
		}
	}

	spidev->users++;
	filp->private_data = spidev;
	stream_open(inode, filp);

	if (spidev->users == 1) {
		if (spidev->pwctl && spidev->pwctl->power_init)
			status = spidev->pwctl->power_init(spidev->pwctl);
		if (status)
			pr_err("ESAM:power ctrl init failed with %d\n", status);
		else if (spidev->pwctl && spidev->pwctl->power_on)
			spidev->pwctl->power_on(spidev->pwctl);
	}
	esam_reset(spidev->pwctl);

	mutex_unlock(&device_list_lock);
	return 0;

err_alloc_rx_buf:
	kfree(spidev->tx_buffer);
	spidev->tx_buffer = NULL;
err_find_dev:
	mutex_unlock(&device_list_lock);
	return status;
}

static int spidev_release(struct inode *inode, struct file *filp)
{
	struct spidev_data	*spidev;
	int			dofree;

	mutex_lock(&device_list_lock);
	spidev = filp->private_data;
	filp->private_data = NULL;

	spin_lock_irq(&spidev->spi_lock);
	/* ... after we unbound from the underlying device? */
	dofree = (spidev->spi == NULL);
	spin_unlock_irq(&spidev->spi_lock);

	/* last close? */
	spidev->users--;
	if (!spidev->users) {

		kfree(spidev->tx_buffer);
		spidev->tx_buffer = NULL;

		kfree(spidev->rx_buffer);
		spidev->rx_buffer = NULL;

		if (spidev->pwctl && spidev->pwctl->power_off)
			spidev->pwctl->power_off(spidev->pwctl);
		if (spidev->pwctl && spidev->pwctl->power_exit)
			spidev->pwctl->power_exit(spidev->pwctl);

		if (dofree)
			kfree(spidev);
		else
			spidev->speed_hz = spidev->spi->max_speed_hz;
	}
#ifdef CONFIG_SPI_SLAVE
	if (!dofree)
		spi_slave_abort(spidev->spi);
#endif
	mutex_unlock(&device_list_lock);

	return 0;
}

static const struct file_operations spidev_fops = {
	.owner =	THIS_MODULE,
	/* REVISIT switch to aio primitives, so that userspace
	 * gets more complete API coverage.  It'll simplify things
	 * too, except for the locking.
	 */
	.write =	spidev_write,
	.read =		spidev_read,
	.unlocked_ioctl = spidev_ioctl,
	.compat_ioctl = spidev_compat_ioctl,
	.open =		spidev_open,
	.release =	spidev_release,
	.llseek =	no_llseek,
};

/*-------------------------------------------------------------------------*/

/* The main reason to have this class is to make mdev/udev create the
 * /dev/spidevB.C character device nodes exposing our userspace API.
 * It also simplifies memory management.
 */

static struct class *spidev_class;

/*-------------------------------------------------------------------------*/
static void esam_power_on(struct power_ctrl *pwctl)
{
	if (!gpio_is_valid(pwctl->gpio)) {
		pr_err("ESAM:power ctrl gpio [%d] is not valid\n", pwctl->gpio);
		return;
	}

	if (pwctl->active_low)
		gpio_set_value(pwctl->gpio, 0);
	else
		gpio_set_value(pwctl->gpio, 1);
}

static void esam_power_off(struct power_ctrl *pwctl)
{
	if (!gpio_is_valid(pwctl->gpio)) {
		pr_err("ESAM:power ctrl gpio [%d] is not valid\n", pwctl->gpio);
		return;
	}

	if (pwctl->active_low)
		gpio_set_value(pwctl->gpio, 1);
	else
		gpio_set_value(pwctl->gpio, 0);
}

static int esam_power_init(struct power_ctrl *pwctl)
{
	int err;

	if (!gpio_is_valid(pwctl->gpio)) {
		pr_err("ESAM:power ctrl gpio [%d] is not valid\n", pwctl->gpio);
		return -EINVAL;
	}

	err = gpio_request(pwctl->gpio, "esam_pwctl");
	if (err) {
		pr_err("ESAM:unable to request gpio %d, err = %d\n", pwctl->gpio, err);
		return err;
	}

	if (pwctl->active_low)
		gpio_direction_output(pwctl->gpio, 0);
	else
		gpio_direction_output(pwctl->gpio, 1);

	return 0;
}

static void esam_power_exit(struct power_ctrl *pwctl)
{
	if (!gpio_is_valid(pwctl->gpio)) {
		pr_err("ESAM:power ctrl gpio [%d] is not valid\n", pwctl->gpio);
		return;
	}

	if (pwctl->active_low)
		gpio_direction_output(pwctl->gpio, 0);
	else
		gpio_direction_output(pwctl->gpio, 1);

	gpio_free(pwctl->gpio);
}

static struct power_ctrl esam_pwctl = {
	.gpio = 273,  /* PI17  (32 * 8 + 17)*/
	.active_low = 1,
	.power_on = esam_power_on,
	.power_off = esam_power_off,
	.power_init = esam_power_init,
	.power_exit = esam_power_exit,
};

#ifdef CONFIG_OF
static const struct of_device_id spidev_dt_ids[] = {
	{ .compatible = "esam,sc1168y", .data = &esam_pwctl },
	{ .compatible = "esam,sc1161y", .data = &esam_pwctl },
	{},
};
MODULE_DEVICE_TABLE(of, spidev_dt_ids);
#endif

/*-------------------------------------------------------------------------*/

static int spidev_probe(struct spi_device *spi)
{
	struct spidev_data	*spidev;
	int			status;
	unsigned long		minor;
	const struct of_device_id	*match = &spidev_dt_ids[0];
	unsigned int            flag;
	int                     gpio;

	/*
	 * spidev should never be referenced in DT without a specific
	 * compatible string, it is a Linux implementation thing
	 * rather than a description of the hardware.
	 */
	WARN(spi->dev.of_node &&
	     of_device_is_compatible(spi->dev.of_node, "spidev"),
	     "%pOF: buggy DT: spidev listed directly in DT\n", spi->dev.of_node);

	/* Allocate driver data */
	spidev = kzalloc(sizeof(*spidev), GFP_KERNEL);
	if (!spidev)
		return -ENOMEM;

	/* Initialize the driver data */
	spidev->pwctl = (struct power_ctrl *)match->data;
	spidev->spi = spi;
	spin_lock_init(&spidev->spi_lock);
	mutex_init(&spidev->buf_lock);

	INIT_LIST_HEAD(&spidev->device_entry);

	/* If we can allocate a minor number, hook up this device.
	 * Reusing minors is fine so long as udev or mdev is working.
	 */
	mutex_lock(&device_list_lock);
	minor = find_first_zero_bit(minors, N_SPI_MINORS);
	if (minor < N_SPI_MINORS) {
		struct device *dev;

		spidev->devt = MKDEV(SPIDEV_MAJOR, minor);
		dev = device_create(spidev_class, &spi->dev, spidev->devt,
				    spidev, "encryption.%d", spi->chip_select);
		status = PTR_ERR_OR_ZERO(dev);
	} else {
		dev_dbg(&spi->dev, "no minor number available!\n");
		status = -ENODEV;
	}
	if (status == 0) {
		set_bit(minor, minors);
		list_add(&spidev->device_entry, &device_list);
	}
	mutex_unlock(&device_list_lock);

	spidev->cs_gpio = of_get_named_gpio_flags(spi->dev.of_node, "ssn_gpio", 0,
			(enum of_gpio_flags *)&flag);
	if (gpio_is_valid(spidev->cs_gpio)) {
		status = gpio_request(spidev->cs_gpio, "esam-cs");
		if (status < 0) {
			pr_err("request esam cs gpio failed, status = %d\n", status);
			status = 0;
		} else {
			gpio_direction_output(spidev->cs_gpio, 1);
		}
	} else {
		status = -EINVAL;
	}

	gpio = of_get_named_gpio_flags(spi->dev.of_node, "power_gpio", 0,
			(enum of_gpio_flags *)&flag);
	if (gpio > 0 && gpio_is_valid(gpio)) {
		spidev->pwctl->gpio = gpio;
		spidev->pwctl->active_low = (flag & OF_GPIO_ACTIVE_LOW ? 1 : 0);
	}

	spidev->speed_hz = spi->max_speed_hz;

	if (status == 0)
		spi_set_drvdata(spi, spidev);
	else
		kfree(spidev);

	return status;
}

static int spidev_remove(struct spi_device *spi)
{
	struct spidev_data	*spidev = spi_get_drvdata(spi);

	if (gpio_is_valid(spidev->cs_gpio))
		gpio_free(spidev->cs_gpio);
	/* prevent new opens */
	mutex_lock(&device_list_lock);
	/* make sure ops on existing fds can abort cleanly */
	spin_lock_irq(&spidev->spi_lock);
	spidev->spi = NULL;
	spin_unlock_irq(&spidev->spi_lock);

	list_del(&spidev->device_entry);
	device_destroy(spidev_class, spidev->devt);
	clear_bit(MINOR(spidev->devt), minors);
	if (spidev->users == 0)
		kfree(spidev);
	mutex_unlock(&device_list_lock);

	return 0;
}

static struct spi_driver spidev_spi_driver = {
	.driver = {
		.name =		"esamdev",
		.of_match_table = of_match_ptr(spidev_dt_ids),
		.acpi_match_table = ACPI_PTR(spidev_acpi_ids),
	},
	.probe =	spidev_probe,
	.remove =	spidev_remove,

	/* NOTE:  suspend/resume methods are not necessary here.
	 * We don't do anything except pass the requests to/from
	 * the underlying controller.  The refrigerator handles
	 * most issues; the controller driver handles the rest.
	 */
};

/*-------------------------------------------------------------------------*/

static int __init spidev_init(void)
{
	int status;

	/* Claim our 256 reserved device numbers.  Then register a class
	 * that will key udev/mdev to add/remove /dev nodes.  Last, register
	 * the driver which manages those device numbers.
	 */
	BUILD_BUG_ON(N_SPI_MINORS > 256);
	status = register_chrdev(SPIDEV_MAJOR, "esam", &spidev_fops);
	if (status < 0)
		return status;

	spidev_class = class_create(THIS_MODULE, "spi-esam");
	if (IS_ERR(spidev_class)) {
		unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver.driver.name);
		return PTR_ERR(spidev_class);
	}

	status = spi_register_driver(&spidev_spi_driver);
	if (status < 0) {
		class_destroy(spidev_class);
		unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver.driver.name);
	}
	return status;
}
module_init(spidev_init);

static void __exit spidev_exit(void)
{
	spi_unregister_driver(&spidev_spi_driver);
	class_destroy(spidev_class);
	unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver.driver.name);
}
module_exit(spidev_exit);

MODULE_AUTHOR("Andrea Paterniani, <a.paterniani@swapp-eng.it>, <chenxd@hzhytech.com>");
MODULE_DESCRIPTION("User mode SPI esam device interface");
MODULE_LICENSE("GPL");
