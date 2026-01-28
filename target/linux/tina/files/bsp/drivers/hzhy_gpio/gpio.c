#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/kdev_t.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/ioctl.h>
#include <linux/poll.h>
#include "hzhy_gpio.h"

struct hzhy_gpios_desc {
	char *name;
	struct list_head list;
	struct device *gpio_dev;
	struct cdev cdev;
	struct fasync_struct *fasync;
	struct gpio_desc *gpio;
	int cansleep;
	int changed;
	int irq;
	refcount_t ref;
};

static struct class *hzhy_gdev_cls;
static dev_t hzhy_gdev_devno;
static LIST_HEAD(gpio_desc_head);
static DECLARE_WAIT_QUEUE_HEAD(wq_head);

static irqreturn_t hzhy_gpios_irq_handler(unsigned int irq,
		void *device, struct pt_regs *registers)
{
	struct hzhy_gpios_desc *desc;
	desc = (struct hzhy_gpios_desc *)device;

	kill_fasync(&desc->fasync, SIGIO, POLL_IN);

	if (waitqueue_active(&wq_head)) {
		desc->changed = 1;
		wake_up_interruptible(&wq_head);
	}

	return IRQ_HANDLED;
}

static int hzhy_gpios_open(struct inode *inode, struct file *filp)
{
	int ret = 0;
	struct hzhy_gpios_desc *desc;

	desc = container_of(inode->i_cdev, struct hzhy_gpios_desc, cdev);

	if (refcount_read(&desc->ref))
		return -EBUSY;

	if (desc->irq > 0) {
		ret = devm_request_irq(desc->gpio_dev, gpiod_to_irq(desc->gpio),
				(irq_handler_t)hzhy_gpios_irq_handler,
				IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
				desc->name, desc);
		if (ret) {
			dev_err(desc->gpio_dev, "%s:request irq failed with:%d gpio:%s\n",
					__func__, ret, desc->name);
			return ret;
		}
	}

	filp->private_data = desc;
	refcount_inc(&desc->ref);

	return ret;
}

static int hzhy_gpios_close(struct inode *inode, struct file *filp)
{
	struct hzhy_gpios_desc *desc;

	desc = container_of(inode->i_cdev, struct hzhy_gpios_desc, cdev);

	if (!refcount_read(&desc->ref))
		return -ENODEV;

	if (desc->irq > 0) {
		/*TODO :
		 * free desc->fasync ?
		 */
		devm_free_irq(desc->gpio_dev, gpiod_to_irq(desc->gpio), desc);
	}

	refcount_dec(&desc->ref);

	return 0;
}

static ssize_t hzhy_gpios_read(struct file *file,
		char __user *ptr, size_t size, loff_t *off)
{
	struct hzhy_gpios_desc *desc;
	unsigned char value = 0;

	desc = (struct hzhy_gpios_desc *)file->private_data;
	if (!desc)
		return -EINVAL;

	if (gpiod_get_value(desc->gpio))
		value = '1';
	else
		value = '0';

	if (copy_to_user(ptr, &value, sizeof(value)))
		return -EFAULT;

	return sizeof(value);
}

static ssize_t hzhy_gpios_write(struct file *file,
		const char __user *ptr, size_t size, loff_t *off)
{
	struct hzhy_gpios_desc *desc;
	int value = 0;

	desc = (struct hzhy_gpios_desc *)file->private_data;
	if (!desc)
		return -EINVAL;

	if (copy_from_user(&value, ptr, 1))
		return -EFAULT;
	if (value == '0')
		value = 0;
	else
		value = 1;
	if (!desc->cansleep)
		gpiod_set_value(desc->gpio, value);
	else
		gpiod_set_value_cansleep(desc->gpio, value);

	return size;
}

static unsigned int hzhy_gpios_poll(struct file *file, struct poll_table_struct *poll_table)
{
	struct hzhy_gpios_desc *desc;
	unsigned int mask = 0;

	desc = (struct hzhy_gpios_desc *)file->private_data;
	if (!desc)
		return -EINVAL;

	if (desc->irq < 0)
		return -EINVAL;

	if (!desc->changed)
		poll_wait(file, &wq_head, poll_table);
	else {
		desc->changed = 0;
		mask = POLLIN | POLLRDNORM;
	}

	return mask;
}

#define HZGPIO_IOC_MAGIC	'X'
#define HZGPIO_DIR_OUTPUT	_IOW(HZGPIO_IOC_MAGIC, 1, int)
#define HZGPIO_DIR_INPUT	_IOW(HZGPIO_IOC_MAGIC, 2, int)

static long hzhy_gpios_ioctl(struct file *file,
		unsigned int cmd, unsigned long data)
{
	struct hzhy_gpios_desc *desc;

	desc = (struct hzhy_gpios_desc *)file->private_data;
	if (!desc)
		return -EINVAL;

	if (desc->irq > 0) { /* not a input/output gpio */
		return -EINVAL;
	}

	if (_IOC_TYPE(cmd) != HZGPIO_IOC_MAGIC)
		return -ENOTTY;

	switch (cmd) {
	case HZGPIO_DIR_OUTPUT:
		gpiod_direction_output(desc->gpio, data);
		break;
	case HZGPIO_DIR_INPUT:
		gpiod_direction_input(desc->gpio);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int hzhy_gpios_async(int fd, struct file *file, int on)
{
	struct hzhy_gpios_desc *desc;

	desc = (struct hzhy_gpios_desc *)file->private_data;
	if (!desc)
		return -EINVAL;

	if (desc->irq < 0)
		return -EINVAL;

	return fasync_helper(fd, file, on, &desc->fasync);
}

static struct file_operations hzhy_gpios_fops = {
	.owner = THIS_MODULE,
	.read = hzhy_gpios_read,
	.write = hzhy_gpios_write,
	.unlocked_ioctl = hzhy_gpios_ioctl,
	.open = hzhy_gpios_open,
	.release = hzhy_gpios_close,
	.fasync = hzhy_gpios_async,
	.poll = hzhy_gpios_poll,
};

static int init_gpio_chardev(void)
{
	int ret;

	ret = alloc_chrdev_region(&hzhy_gdev_devno, 0, 255, "gpiodev");
	if (ret) {
		pr_err("%s: alloc chrdev region failed!ret=%d\n", __func__, ret);
		return ret;
	}

	hzhy_gdev_cls = class_create(THIS_MODULE, "gpiodev");
	if (IS_ERR(hzhy_gdev_cls)) {
		pr_err("%s:class create failed!\n", __func__);
		unregister_chrdev_region(hzhy_gdev_devno, 255);
		return PTR_ERR(hzhy_gdev_cls);
	}

	return 0;
}

static int hzhy_gpios_probe(struct hzhy_gpio_device *hdev)
{
	int ret;
	struct device_node *node = hdev->dev.of_node, *child;
	struct device *dev = &hdev->dev;
	struct hzhy_gpios_desc *desc;
	static int idx = 0;
	dev_t major = MAJOR(hzhy_gdev_devno);

	if (!node)
		return -ENODEV;

	if (of_get_child_count(node) == 0)
		return -ENODEV;


	for_each_available_child_of_node(node, child) {
		desc = devm_kzalloc(&hdev->dev, sizeof *desc, GFP_KERNEL);
		if (!desc) {
			pr_err("%s:Error while allocate memory for gpio-desc\n", __func__);
			continue;
		}

		refcount_set(&desc->ref, 0);
		desc->name = (char *)of_get_property(child, "label", NULL) ? : (char *)child->name;
		if (of_device_is_compatible(node, "hzhy,gpios-output")) {
			desc->gpio = devm_gpiod_get(dev, NULL, GPIOD_OUT_HIGH);
		} else if (of_device_is_compatible(node, "hzhy,gpios-input")) {
			desc->gpio = devm_gpiod_get(dev, NULL, GPIOD_IN);
			desc->irq = gpiod_to_irq(desc->gpio);
		}
		if (IS_ERR(desc->gpio)) {
			pr_err("%s:Invalid gpio desc for %s ret is %d\n", __func__, desc->name, ret);
			devm_kfree(dev, desc);
			continue;
		}

		desc->gpio_dev = device_create(hzhy_gdev_cls, dev, MKDEV(major, idx++), desc, desc->name);
		if (IS_ERR(desc->gpio_dev)) {
			ret = PTR_ERR(desc->gpio_dev);
			pr_err("%s device create failed!ret=%d\n", __func__, ret);
			devm_kfree(dev, desc);
			continue;
		}

		cdev_init(&desc->cdev, &hzhy_gpios_fops);
		desc->cdev.owner = THIS_MODULE;
		ret = cdev_add(&desc->cdev, MKDEV(major, idx - 1), 1);
		if (ret) {
			pr_err("%s:cdev add failed, ret=%d\n", __func__, ret);
			device_destroy(hzhy_gdev_cls, MKDEV(major, idx - 1));
			devm_kfree(dev, desc);
			continue;
		}

		list_add(&desc->list, &gpio_desc_head);
	}

	return 0;
}

static int hzhy_gpios_remove(struct hzhy_gpio_device *hdev)
{
	struct list_head *pos, *n;
	struct hzhy_gpios_desc *desc;
	int idx = 0;
	dev_t major = MAJOR(hzhy_gdev_devno);

	list_for_each_safe(pos, n, &gpio_desc_head) {
		desc = list_entry(pos, struct hzhy_gpios_desc, list);
		list_del(&desc->list);
		cdev_del(&desc->cdev);
		device_destroy(hzhy_gdev_cls, MKDEV(major, idx++));
		devm_kfree(&hdev->dev, desc);
	}

	return 0;
}

static const struct of_device_id hzhy_gpios_of_match[] = {
	{ .compatible = "hzhy,gpios-input", .data = NULL },
	{ .compatible = "hzhy,gpios-output", .data = NULL },
	{ },
};

static struct hzhy_gpio_driver hzhy_gpios = {
	.probe = hzhy_gpios_probe,
	.remove = hzhy_gpios_remove,
	.driver = {
		.name = "hzhy_gpios",
		.of_match_table = hzhy_gpios_of_match,
	},
};

int hzhy_gpios_init(void)
{
	int ret;
	ret = init_gpio_chardev();
	if (ret)
		return ret;
	return hzhy_gpio_driver_register(&hzhy_gpios);
}

void hzhy_gpios_exit(void)
{
	class_destroy(hzhy_gdev_cls);
	unregister_chrdev_region(hzhy_gdev_devno, 255);
	hzhy_gpio_driver_unregister(&hzhy_gpios);
}

module_init(hzhy_gpios_init);
module_exit(hzhy_gpios_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("chenxd@hzhytech.com");
