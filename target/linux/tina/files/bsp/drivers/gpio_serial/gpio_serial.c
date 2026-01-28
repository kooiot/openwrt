#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/tty_driver.h>
#include <linux/device.h>
#include <linux/serial.h>
#include <linux/serial_core.h>
#include "gpio_serial.h"


static int gpio_serial_open(struct tty_struct *tty, struct file *filp)
{
	//struct gpio_uart *guart = tty->dev->driver_data;
	struct gpio_uart *guart = dev_get_drvdata(tty->dev);
	struct suart_port *gpio_port = &guart->gport;

	if (gpio_port && gpio_port->ops->open)
		return gpio_port->ops->open(tty, filp, guart);

	return -ENODEV;
}

static void gpio_serial_close(struct tty_struct *tty, struct file *filp)
{
	struct gpio_uart *guart = dev_get_drvdata(tty->dev);
	struct suart_port *gpio_port = &guart->gport;

	printk("%s()\n", __func__);

	if (gpio_port && gpio_port->ops->close)
		gpio_port->ops->close(tty, filp, guart);
}

static int gpio_serial_write(struct tty_struct *tty, const unsigned char *buf,
		int buf_size)
{
	struct gpio_uart *guart = dev_get_drvdata(tty->dev);
	struct suart_port *gpio_port = &guart->gport;

	if (gpio_port && gpio_port->ops->write)
		return gpio_port->ops->write(tty, buf, buf_size, guart);

	return 0;
}

static unsigned int gpio_serial_write_room(struct tty_struct *tty)
{
	struct gpio_uart *guart = dev_get_drvdata(tty->dev);
	struct suart_port *gpio_port = &guart->gport;

	if (gpio_port && gpio_port->ops->write_room)
		return gpio_port->ops->write_room(tty, guart);

	return 0;
}

static unsigned int gpio_serial_chars_in_buffer(struct tty_struct *tty)
{
	struct gpio_uart *guart = dev_get_drvdata(tty->dev);
	struct suart_port *gpio_port = &guart->gport;

	if (gpio_port && gpio_port->ops->chars_in_buffer)
		return gpio_port->ops->chars_in_buffer(tty, guart);

	return 0;
}

static int gpio_serial_ioctl(struct tty_struct *tty, unsigned int cmd,
		unsigned int long parameter)
{
	int ret = 0;

	switch (cmd) {
	case TIOCMSET:
		break;
	case TIOCMGET:
		break;
	default:
		ret = -ENOIOCTLCMD;
		break;
	}

	return ret;
}

static void gpio_serial_set_termios(struct tty_struct *tty, struct ktermios
		*termios)
{
	struct gpio_uart *guart = dev_get_drvdata(tty->dev);
	struct suart_port *gpio_port = &guart->gport;

	printk("%s()\n", __func__);
	if (gpio_port && gpio_port->ops->set_termios)
		gpio_port->ops->set_termios(tty, termios, guart);
}

static void gpio_serial_stop(struct tty_struct* tty)
{
	printk(KERN_DEBUG "%s()\n", __func__);
}

static void gpio_serial_start(struct tty_struct *tty)
{
	printk(KERN_DEBUG "%s()\n", __func__);
}

static void gpio_serial_hangup(struct tty_struct *tty)
{
	printk(KERN_DEBUG "%s()\n", __func__);
}

static int gpio_serial_tiocmget(struct tty_struct *tty)
{
	printk(KERN_DEBUG "%s()\n", __func__);
	return 0;
}

static int gpio_serial_tiocmset(struct tty_struct *tty, unsigned int set,
		unsigned int clear)
{
	printk(KERN_DEBUG "%s()\n", __func__);
	return 0;
}

static void gpio_serial_throttle(struct tty_struct *tty)
{
	printk(KERN_DEBUG "%s()\n", __func__);
}

static void gpio_serial_unthrottle(struct tty_struct *tty)
{
	printk(KERN_DEBUG "%s()\n", __func__);
}

static const struct tty_operations serial_ops = {
	.open =				gpio_serial_open,
	.close =			gpio_serial_close,
	.write =			gpio_serial_write,
	.hangup =			gpio_serial_hangup,
	.write_room =		gpio_serial_write_room,
	.ioctl =			gpio_serial_ioctl,
	.set_termios =		gpio_serial_set_termios,
	.throttle =			gpio_serial_throttle,
	.unthrottle =		gpio_serial_unthrottle,
	.start =			gpio_serial_start,
	.stop =				gpio_serial_stop,
	//.break_ctl =		gpio_serial_break,
	.chars_in_buffer =	gpio_serial_chars_in_buffer,
	//.wait_until_sent =	gpio_serial_wait_until_sent,
	.tiocmget =			gpio_serial_tiocmget,
	.tiocmset =			gpio_serial_tiocmset,
	//.cleanup =			gpio_serial_cleanup,
	//.install =			gpio_serial_cleanup,
	//.proc_fops =		&serial_proc_fops,
};

static struct tty_driver *gpio_serial_tty_driver;

int gpio_serial_init(void)
{
	int result;

	gpio_serial_tty_driver = tty_alloc_driver(10, 0);
	if (!gpio_serial_tty_driver)
		return -ENOMEM;

	gpio_serial_tty_driver->driver_name = "gpio-uart";
	gpio_serial_tty_driver->name = "stty";
	gpio_serial_tty_driver->major = 501;
	gpio_serial_tty_driver->minor_start = 0;
	gpio_serial_tty_driver->type = TTY_DRIVER_TYPE_SERIAL;
	gpio_serial_tty_driver->subtype = SERIAL_TYPE_NORMAL;
	gpio_serial_tty_driver->flags = TTY_DRIVER_REAL_RAW |
		TTY_DRIVER_DYNAMIC_DEV;
	gpio_serial_tty_driver->init_termios = tty_std_termios;
	gpio_serial_tty_driver->init_termios.c_cflag = B4800 | CS8 | CREAD
		| HUPCL | CLOCAL;
	gpio_serial_tty_driver->init_termios.c_ispeed = 4800;
	gpio_serial_tty_driver->init_termios.c_ospeed = 4800;
	tty_set_operations(gpio_serial_tty_driver, &serial_ops);

	result = tty_register_driver(gpio_serial_tty_driver);
	if (result) {
		pr_err("%s - tty_register_driver failed\n", __func__);
		goto exit_reg_driver;
	}

exit_reg_driver:
	return result;
}

void gpio_serial_exit(void)
{
	tty_unregister_driver(gpio_serial_tty_driver);
}

int gpio_serial_add_port(struct suart_port *gpio_port)
{
	struct device *tty_dev;
	struct tty_port *port;
	int ret = 0;

	if (!gpio_port)
		return -EINVAL;

	if (gpio_port->minor > MAX_NUM_PORT) {
		return -EINVAL;
	}

	/* port init*/
	port = &gpio_port->port;
	tty_port_init(port);
	//port->ops = &gpio_port_ops;
	mutex_lock(&port->mutex);

	tty_dev = tty_port_register_device(port, gpio_serial_tty_driver,
			gpio_port->minor, gpio_port->dev);
	if (unlikely(IS_ERR(tty_dev))) {
		dev_err(gpio_port->dev, "Cannot register tty device on line %d\n",
				gpio_port->minor);
		ret = PTR_ERR(tty_dev);
		goto out;
	}
	dev_set_drvdata(tty_dev, gpio_port);

out:
	mutex_unlock(&port->mutex);
	return ret;
}

void gpio_serial_remove_port(struct suart_port *gpio_port)
{
	struct tty_port *port = &gpio_port->port;
	struct tty_struct *tty;

	/*
	 *  Remove the device from the tty layer
	 */
	tty_unregister_device(gpio_serial_tty_driver, gpio_port->minor);

	tty = tty_port_tty_get(port);
	if (tty) {
		tty_vhangup(port->tty);
		tty_kref_put(tty);
	}
}

