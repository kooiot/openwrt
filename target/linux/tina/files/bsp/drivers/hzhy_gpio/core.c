#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include "hzhy_gpio.h"

struct device hzhy_gpio = {
	.init_name = "hzhy-gpio",
};
EXPORT_SYMBOL_GPL(hzhy_gpio);

static int hzhy_gpio_device_match(struct device *dev,
		struct device_driver *drv)
{
	struct hzhy_gpio_device *hdev;

	if (!dev || !drv)
		return 0;

	hdev = to_hzhy_gpio_device(dev);
	if (!hdev)
		return 0;
	
	/* Attempt an OF style match */
	if (of_driver_match_device(dev, drv))
		return 1;

	return (strcmp(hdev->name, drv->name) == 0);
}

static struct bus_type hzhy_gpio_bus = {
	.name = "hzhy_gpio_bus",
	.match = hzhy_gpio_device_match,
	//.probe = hzhy_gpio_device_probe,
	//.remove = hzhy_gpio_device_remove,
};

struct drv_dev_data {
	void *fn;
	void *data;
};

typedef int (*dev_fun)(struct hzhy_gpio_device *, void *);
typedef int (*drv_fun)(struct hzhy_gpio_driver *, void *);

static int hzhy_gpio_drv_iterate(struct device_driver *_drv, void *data)
{
	struct hzhy_gpio_driver *drv;
	struct drv_dev_data *drv_fn;
	drv_fun fn;

	drv = to_hzhy_gpio_driver(_drv);
	drv_fn = (struct drv_dev_data *)data;

	fn = (drv_fun)(drv_fn->fn);
	return fn(drv, drv_fn->data);
}

static int hzhy_gpio_dev_iterate(struct device *_dev, void *data)
{
	struct hzhy_gpio_device *hdev;
	struct drv_dev_data *dev_fn;
	dev_fun fn;

	hdev = to_hzhy_gpio_device(_dev);
	dev_fn = (struct drv_dev_data *)data;

	fn = (dev_fun)(dev_fn->fn);
	return fn(hdev, dev_fn->data);
}

int for_each_hzhy_gpio_dev(void *data,
		int (*fn)(struct hzhy_gpio_device *, void *))
{
	struct drv_dev_data private;

	private.fn = fn;
	private.data = data;

	return bus_for_each_dev(&hzhy_gpio_bus, NULL,
			&private, hzhy_gpio_dev_iterate);
}

int for_each_hzhy_gpio_drv(void *data,
		int (*fn)(struct hzhy_gpio_driver *, void *))
{
	struct drv_dev_data private;

	private.fn = fn;
	private.data = data;

	return bus_for_each_drv(&hzhy_gpio_bus, NULL,
			&private, hzhy_gpio_drv_iterate);
}

/**
 * add a hzhy specific gpio device.
 * @hdev: hzhy gpio device we're adding
 */
int hzhy_gpio_device_register(struct hzhy_gpio_device *hdev)
{
	if (!hdev)
		return -EINVAL;

	device_initialize(&hdev->dev);
	if (!hdev->dev.parent)
		hdev->dev.parent = &hzhy_gpio;
	hdev->dev.bus = &hzhy_gpio_bus;

	dev_set_name(&hdev->dev, "%s", hdev->name);
	
	pr_debug("hzhy-debug:Register hzhy-gpio device '%s'. Parent at %s \n",
			dev_name(&hdev->dev), dev_name(hdev->dev.parent));

	return device_add(&hdev->dev);
}
EXPORT_SYMBOL_GPL(hzhy_gpio_device_register);

void hzhy_gpio_device_unregister(struct hzhy_gpio_device *hdev)
{
	if (hdev) {
		device_del(&hdev->dev);
		put_device(&hdev->dev);
	}
}
EXPORT_SYMBOL_GPL(hzhy_gpio_device_unregister);

static int hzhy_gpio_drv_probe(struct device *_dev)
{
	struct hzhy_gpio_driver *drv;
	struct hzhy_gpio_device *dev;

	drv = to_hzhy_gpio_driver(_dev->driver);
	dev = to_hzhy_gpio_device(_dev);

	if (drv->probe)
		return drv->probe(dev);

	return 0;
}

int hzhy_gpio_driver_register(struct hzhy_gpio_driver *drv)
{
	drv->driver.owner = THIS_MODULE;
	drv->driver.bus = &hzhy_gpio_bus;
	drv->driver.probe = hzhy_gpio_drv_probe;

	return driver_register(&drv->driver);
}
EXPORT_SYMBOL_GPL(hzhy_gpio_driver_register);

void hzhy_gpio_driver_unregister(struct hzhy_gpio_driver *drv)
{
	driver_unregister(&drv->driver);
}
EXPORT_SYMBOL_GPL(hzhy_gpio_driver_unregister);

static int hzhy_gpio_probe(struct platform_device *pdev)
{
	int ret;
	struct hzhy_gpio_device *hdev;
	struct device_node *node = pdev->dev.of_node, *child;

	for_each_available_child_of_node(node, child) {
		hdev = devm_kzalloc(&pdev->dev, sizeof *hdev, GFP_KERNEL);
		if (!hdev) {
			pr_err("hzhy-debug:%s error allocate memory for hzhy gpio key device",
					__func__);
			continue;
		}

		hdev->name = (char *)of_get_property(
				child, "compatible", NULL) ? :  (char *)child->name;
		hdev->dev.of_node = child;
		hdev->dev.parent = &pdev->dev;

		ret = hzhy_gpio_device_register(hdev);
		if (ret) {
			pr_err("hzhy-debug:%s error register gpio key device\n", __func__);
			goto free_hzhy_gpio_key;
		}
	}

	return 0;

free_hzhy_gpio_key:
	devm_kfree(&pdev->dev, hdev);

	return ret;
}

int _hzhy_gpio_dev_remove(struct hzhy_gpio_device *hdev, void *data)
{
	struct device *dev = (struct device *)data;
	hzhy_gpio_device_unregister(hdev);
	devm_kfree(dev, hdev);
	return 0;
}

static int hzhy_gpio_remove(struct platform_device *pdev)
{
	return for_each_hzhy_gpio_dev(&pdev->dev, _hzhy_gpio_dev_remove);
}

static const struct of_device_id hzhy_gpio_of_match[] = {
	{.compatible = "hzhy,gpio-specific"},
	{},
};

static struct platform_driver hzhy_gpio_platform_drv = {
	.probe = hzhy_gpio_probe,
	.remove = hzhy_gpio_remove,
	.driver = {
		.name = "hzhy-gpio",
		.of_match_table = of_match_ptr(hzhy_gpio_of_match),
	},
};

static int __init hzhy_gpio_core_init(void)
{
	int ret;

	ret = bus_register(&hzhy_gpio_bus);
	if (ret) {
		pr_err("hzhy-debug:%s - register hzhy_gpio_bus failed\n", __func__);
		return ret;
	}

	ret = platform_driver_register(&hzhy_gpio_platform_drv);
	if (ret) {
		pr_err("hzhy-debug:%s - register hzhy gpio platform drv failed\n", __func__);
		goto bus_unregister;
	}

	pr_info("hzhy_gpio_core_init completed!\n");

	return 0;

bus_unregister:
	bus_unregister(&hzhy_gpio_bus);

	return ret;
}

static void __exit hzhy_gpio_core_exit(void)
{
	pr_info("hzhy-debug:hzhy_gpio_spec_exit\n");
	platform_driver_unregister(&hzhy_gpio_platform_drv);
	bus_unregister(&hzhy_gpio_bus);
}

subsys_initcall(hzhy_gpio_core_init);
module_exit(hzhy_gpio_core_exit);

MODULE_LICENSE("GPL");
MODULE_VERSION("V0.0.1");
MODULE_AUTHOR("chenxd@hzhytech.com");
MODULE_DESCRIPTION("hzhy gpio system");

