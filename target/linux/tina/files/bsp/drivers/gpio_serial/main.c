#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/serial.h>
#include <linux/serial_core.h>
#include <linux/tty_driver.h>
#include <linux/version.h>
#include <linux/tty.h>
#include <asm/io.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/of_gpio.h>
#include "gpio_serial.h"
#include "tty_gpio.h"

static struct gpio_uart *gpio_uart;
static int serial_count = 0;

static int gpio_uart_of_parse(struct gpio_uart *gport, struct device_node *node)
{
	struct device_node *child;
	struct gpio_data *gdata;
	int index = 0;
	const char *dir;

	for_each_available_child_of_node(node, child) {
		gdata = &gport->gdata[index++];
		gdata->label = of_get_property(child, "label", NULL) ? : child->name;
		dir = of_get_property(child, "direction", NULL);
		if (!strcmp(dir, "in"))
			gdata->dir = 0;
		else
			gdata->dir = 1;
		gdata->gpio = of_get_gpio_flags(child, 0, &gdata->vdef);
	}

	return 0;
}

static struct suart_operations guart_ops = {
	.open = tty_gpio_open,
	.close = tty_gpio_close,
	.write = tty_gpio_send_string,
	.write_room = tty_gpio_get_fifo_avail,
	.chars_in_buffer = tty_gpio_get_fifo_len,
	.set_termios = tty_gpio_set_termios,
};

static int gpio_port_init(int index, struct gpio_uart *guart, struct device *dev)
{
	struct gpio_data *gdata;
	int i;
	int ret = 0;

	guart->gport.dev = dev;
	guart->gport.minor = index;
	//guart->gport.ops = xxx_ops;
	guart->gport.flag = 0;
	guart->gport.ops = &guart_ops;

	for (i = 0; i < 2; i++) {
		gdata = &guart->gdata[i];

		if (!gpio_is_valid(gdata->gpio)) {
			printk("gpio [%d] is not valid\n", gdata->gpio);
			ret = -EINVAL;
			goto out;
		}
		ret = gpio_request(gdata->gpio, gdata->label);
		if (ret < 0) {
			printk("request gpio [%d] failed\n", gdata->gpio);
			ret = -ENOMEM;
			goto out;
		}
		if (gdata->dir == 0) {
			gpio_direction_input(gdata->gpio);
			guart->rx.gpio = gdata->gpio;
		} else {
			gpio_direction_output(gdata->gpio, gdata->vdef);
			guart->tx.gpio = gdata->gpio;
		}

		gpio_export(gdata->gpio, false);
	}

out:
	return ret;
}

static void gpio_port_finalize(struct gpio_uart *guart)
{
	struct gpio_data *gdata;
	int i;
	for (i = 0; i < 2; i++) {
		gdata = &guart->gdata[i];
		gpio_unexport(gdata->gpio);
		gpio_free(gdata->gpio);
	}
}

static int gpio_uart_probe(struct platform_device *pdev)
{
	int index;
	int ret = 0;
	struct device_node *node = pdev->dev.of_node, *child;
	struct gpio_uart *gport;
	printk("%s\n", __func__);

	//serial_count = of_get_available_child_count(node);
	serial_count = of_get_child_count(node);
	if (!serial_count)
		return -ENODEV;

	gpio_uart = devm_kzalloc(&pdev->dev, sizeof *gpio_uart, GFP_KERNEL);
	if (!gpio_uart)
		return -ENOMEM;
	memset(gpio_uart, 0x0, sizeof *gpio_uart);
	gport = gpio_uart;

	index = 0;
	for_each_available_child_of_node(node, child) {
		gport = gpio_uart + index;
		ret = gpio_uart_of_parse(gport, child);
		if (ret)
			goto gpio_uart_free;
		ret = gpio_port_init(index, gport, &pdev->dev);
		if (ret)
			goto gpio_uart_free;
		ret = tty_gpio_init(gport);
		if (ret)
			goto gpio_port_free;
		ret = gpio_serial_add_port(&gport->gport);
		if (ret)
			goto tty_gpio_free;
		index++;
	}

	return 0;

tty_gpio_free:
	tty_gpio_over(gport);
gpio_port_free:
	gpio_port_finalize(gport);
gpio_uart_free:
	kfree(gpio_uart);
	return ret;
}

static int gpio_uart_remove(struct platform_device *dev)
{
	struct gpio_uart *gport;
	int index = 0;
	int i;
	printk("%s\n", __func__);

	for (i = 0; i < serial_count; i++) {
		gport = gpio_uart + index;
		gpio_serial_remove_port(&gport->gport);
		tty_gpio_over(gport);
		gpio_port_finalize(gport);
	}
	return 0;
}

static const struct of_device_id gpio_uart_of_match[] = {
	{ .compatible = "hzhy,gpio_uart" },
	{},
};

static struct platform_driver gpio_uart_driver = {
	.probe = gpio_uart_probe,
	.remove = gpio_uart_remove,
	.driver = {
		.name = "gpio_uart",
		.of_match_table = of_match_ptr(gpio_uart_of_match),
	},
};

static int __init gpio_uart_init(void)
{
	int ret = 0;
	printk("%s\n", __func__);

	ret = gpio_serial_init();
	if (ret) {
		printk("Cannot init gpio serial!\n");
		return ret;
	}

	return platform_driver_register(&gpio_uart_driver);
}

static void __exit gpio_uart_exit(void)
{
	printk("%s\n", __func__);
	platform_driver_unregister(&gpio_uart_driver);
	gpio_serial_exit();
}

module_init(gpio_uart_init);
module_exit(gpio_uart_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ChexXiaodong");
MODULE_VERSION("0.0.1");
MODULE_DESCRIPTION("Genery gpio-serial driver");

