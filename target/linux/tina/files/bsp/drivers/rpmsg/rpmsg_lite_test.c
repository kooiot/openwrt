#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/rpmsg.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#include "rpmsg_master.h"

#define RPMSG_DEV_NAME "rpmsg_test"
#define RPMSG_CLASS_NAME "rpmsg"
#define RPMSG_BUFFER_SIZE 256


static struct class *rpmsg_class;
static struct rpmsg_device *rpmsg_dev;
static char rpmsg_rx_buf[RPMSG_BUFFER_SIZE];
static char rpmsg_tx_buf[RPMSG_BUFFER_SIZE];
// static struct mutex rpmsg_mutex;
static struct cdev rpmsg_cdev;
static dev_t rpmsg_devno;
static DECLARE_WAIT_QUEUE_HEAD(rpmsg_wait_queue);
static int rpmsg_data_available;

static int rpmsg_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int rpmsg_release(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t rpmsg_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	if (copy_from_user(rpmsg_tx_buf, buf, count))
		return -EFAULT;

	rpmsg_send(rpmsg_dev->ept, rpmsg_tx_buf, count);
	return count;
}

static ssize_t rpmsg_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	if (!rpmsg_data_available) {
		wait_event(rpmsg_wait_queue, rpmsg_data_available);
	}

	if (copy_to_user(buf, rpmsg_rx_buf, RPMSG_BUFFER_SIZE))
		return -EFAULT;
	rpmsg_data_available = 0;
     pr_info("Data read from rpmsg: %s\n", rpmsg_rx_buf);
	return RPMSG_BUFFER_SIZE;
}

static struct file_operations rpmsg_fops = {
	.owner = THIS_MODULE,
	.open = rpmsg_open,
	.release = rpmsg_release,
	.write = rpmsg_write,
	.read = rpmsg_read,
};

static int rpmsg_test_cb(struct rpmsg_device *rpmsg_dev, void *data, int len,
						void *priv, u32 src)
{
	memcpy(rpmsg_rx_buf, data, len);
	rpmsg_data_available = 1;
	wake_up_interruptible(&rpmsg_wait_queue);
    return 0;
}

static int rpmsg_test_probe(struct rpmsg_device *dev)
{
	struct device *rpmsg_device;
	dev_t devno;
	int ret;

	rpmsg_dev = dev;


	ret = alloc_chrdev_region(&devno, 0, 1, RPMSG_DEV_NAME);
	if (ret < 0) {
		pr_err("Failed to allocate character device region\n");
		class_destroy(rpmsg_class);
		return ret;
	}
	rpmsg_devno = devno;

	rpmsg_device = device_create(g_aw_rpmsg_class, NULL, devno, NULL, RPMSG_DEV_NAME);
	if (IS_ERR(rpmsg_device)) {
		pr_err("Failed to create rpmsg device\n");
		unregister_chrdev_region(devno, 1);
		class_destroy(rpmsg_class);
		return PTR_ERR(rpmsg_device);
	}

	cdev_init(&rpmsg_cdev, &rpmsg_fops);
	ret = cdev_add(&rpmsg_cdev, devno, 1);
	if (ret < 0) {
		pr_err("Failed to add character device\n");
		device_destroy(rpmsg_class, devno);
		unregister_chrdev_region(devno, 1);
		class_destroy(rpmsg_class);
		return ret;
	}

	pr_info("RPMsg driver probed\n");
	return 0;
}

static void rpmsg_test_remove(struct rpmsg_device *dev)
{
	cdev_del(&rpmsg_cdev);
	device_destroy(rpmsg_class, rpmsg_devno);
	unregister_chrdev_region(rpmsg_devno, 1);
	class_destroy(rpmsg_class);
	pr_info("RPMsg driver removed\n");
}


static struct rpmsg_device_id rpmsg_driver_test_id_table[] = {
	{ .name = "sunxi,rpmsg_test_bare" },
	{ .name = "sunxi,rpmsg_test_rv" },
	{ },
};
MODULE_DEVICE_TABLE(rpmsg, rpmsg_driver_test_id_table);

static struct rpmsg_driver rpmsg_test_driver = {
	.probe = rpmsg_test_probe,
	.remove = rpmsg_test_remove,
	.callback = rpmsg_test_cb,
	.drv = {
		.name = "sunxi_rpmsg_lite",
	},
	.id_table = rpmsg_driver_test_id_table,
};

module_rpmsg_driver(rpmsg_test_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("RPMsg Driver");