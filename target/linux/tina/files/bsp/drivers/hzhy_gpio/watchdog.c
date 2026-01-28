#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/workqueue.h>
#include <linux/of_gpio.h>
#include <linux/watchdog.h>
#include "hzhy_gpio.h"

struct hzhy_gpio_wdt_dev {
	struct watchdog_device wdog;
	struct delayed_work work;
	int auto_sec;
	int gpio;
	int gpio_en;
	int enable_active_low;
	bool used;
	bool auto_on_release;
	/* kernel feed dog seconds counter */
	unsigned int seconds;
};

static void gpio_set_value_compat(unsigned int gpio, int value)
{
	gpio_set_value_cansleep(gpio, value);
}

static void _feed_dog(int gpio)
{
	static unsigned int val = 0;
	//gpio_direction_output(gpio, val);
	gpio_set_value_compat(gpio, val);
	if (val == 0)
		val = 1;
	else
		val = 0;
}

static void auto_feed_dog(struct work_struct *work)
{
	struct hzhy_gpio_wdt_dev *wdev;
	static unsigned int time_ms = 0;

	wdev = container_of((struct delayed_work *)work, struct hzhy_gpio_wdt_dev, work);
	pr_debug("used:%d seconds:%d auto_sec:%d\n", wdev->used ? 1 : 0,
			wdev->seconds, wdev->auto_sec);
	if (!wdev->used && ((wdev->auto_sec < 0) || (wdev->seconds < wdev->auto_sec))) {
		_feed_dog(wdev->gpio);
		time_ms += 500;

		if (time_ms >= 1000) {
			wdev->seconds++;
			time_ms = 0;
		}

		schedule_delayed_work(&wdev->work, msecs_to_jiffies(500));
		pr_debug("hzhy auto feed dog\n");
	}
}

static const struct watchdog_info hzhy_wdt_info = {
	.options = WDIOF_KEEPALIVEPING,
	.identity = "HZHY GPIO Watchdog",
};

static int hzhy_wdt_ping(struct watchdog_device *wdog)
{
	struct hzhy_gpio_wdt_dev *wdev = watchdog_get_drvdata(wdog);

	pr_debug("%s\n", __func__);
	_feed_dog(wdev->gpio);

	return 0;
}

static int hzhy_wdt_start(struct watchdog_device *wdog)
{
	struct hzhy_gpio_wdt_dev *wdev = watchdog_get_drvdata(wdog);
	pr_debug("%s\n", __func__);
	
	if (wdev->gpio_en)
		gpio_set_value_compat(wdev->gpio_en, !wdev->enable_active_low);

	set_bit(WDOG_HW_RUNNING, &wdog->status);
	wdev->used = true;

	return hzhy_wdt_ping(wdog);
}

static int hzhy_wdt_stop(struct watchdog_device *wdog)
{
	struct hzhy_gpio_wdt_dev *wdev = watchdog_get_drvdata(wdog);
	pr_debug("%s\n", __func__);

	wdev->used = false;
	wdev->seconds = 0;

	if (wdev->auto_on_release)
		schedule_delayed_work(&wdev->work, msecs_to_jiffies(500));
	else if (wdev->gpio_en)
		gpio_set_value_compat(wdev->gpio_en, wdev->enable_active_low);

	return 0;
}

static const struct watchdog_ops hzhy_wdt_ops = {
	.owner = THIS_MODULE,
	.start = hzhy_wdt_start,
	.stop  = hzhy_wdt_stop,
	.ping  = hzhy_wdt_ping,
};

static int hzhy_gpio_wdt_probe(struct hzhy_gpio_device *hdev)
{
	int ret;
	struct device_node *node = hdev->dev.of_node;
	struct hzhy_gpio_wdt_dev *wdev;
	enum of_gpio_flags flag;

	if (!node)
		return -ENODEV;

	wdev = devm_kzalloc(&hdev->dev, sizeof(*wdev), GFP_KERNEL);
	if (!wdev)
		return -ENOMEM;

	wdev->used = false;
	wdev->wdog.info = &hzhy_wdt_info;
	wdev->wdog.ops = &hzhy_wdt_ops;
	wdev->wdog.min_timeout = 1;
	wdev->wdog.max_timeout = 120;
	wdev->wdog.parent = &hdev->dev;

	if (watchdog_init_timeout(&wdev->wdog, 0, &hdev->dev) < 0)
		wdev->wdog.timeout = 60;

	watchdog_set_drvdata(&wdev->wdog, wdev);
	hdev->private_data = wdev;
	
	if (of_property_read_u32(node, "auto_seconds", &wdev->auto_sec))
		wdev->auto_sec = -1;

	ret = of_get_named_gpio_flags(node, "wdt_gpio", 0, &flag);
	if (ret <= 0) {
		pr_err("%s:Invalid gpio desc\n", __func__);
		goto free_wdev;
	}
	wdev->gpio = ret;

	ret = devm_gpio_request(&hdev->dev, wdev->gpio, "wdt_gpio");
	if (ret) {
		pr_err("%s:failed to request wdt gpio\n", __func__);
		goto free_wdev;
	}
	gpio_export(wdev->gpio, true);

	gpio_direction_output(wdev->gpio, 0);

	ret = of_get_named_gpio_flags(node, "en_gpio", 0, &flag);
	if (ret > 0) {
		wdev->gpio_en = ret;
		ret = devm_gpio_request(&hdev->dev, wdev->gpio_en, "wdt_gpio_enable");
		if (ret) {
			pr_err("%s:failed to request wdt gpio\n", __func__);
			wdev->gpio_en = 0;
			goto free_gpio;
		}
		wdev->enable_active_low = (flag == OF_GPIO_ACTIVE_LOW);
		if (of_property_read_bool(node, "enable_active_low"))
			wdev->enable_active_low = 1;

		/* disable hardware watchdog */
		gpio_direction_output(wdev->gpio_en, wdev->enable_active_low ? 1 : 0);
		gpio_export(wdev->gpio_en, true);
	}

	wdev->auto_on_release = of_property_read_bool(node, "auto-feed-on-release");

	if (of_property_read_bool(node, "nowayout"))
		watchdog_set_nowayout(&wdev->wdog, true);
	else
		watchdog_set_nowayout(&wdev->wdog, false);

	ret = watchdog_register_device(&wdev->wdog);
	if (ret) {
		pr_err("%s:Error register gpio watchdog:%d\n", __func__, ret);
		goto free_gpio;
	}

	INIT_DELAYED_WORK(&wdev->work, auto_feed_dog);
	if (of_property_read_bool(node, "enable-on-boot")) {
		wdev->seconds = 0;
		schedule_delayed_work(&wdev->work, msecs_to_jiffies(500));

		if (gpio_is_valid(wdev->gpio_en))
			gpio_direction_output(wdev->gpio_en, wdev->enable_active_low ? 0 : 1);
	}


	return 0;

free_gpio:
	devm_gpio_free(&hdev->dev, wdev->gpio);
	if (wdev->gpio_en)
		devm_gpio_free(&hdev->dev, wdev->gpio_en);
free_wdev:
	devm_kfree(&hdev->dev, wdev);

	return ret;
}

static int hzhy_gpio_wdt_remove(struct hzhy_gpio_device *hdev)
{
	struct hzhy_gpio_wdt_dev *wdev;
	wdev = hdev->private_data;

	cancel_delayed_work(&wdev->work);
	watchdog_unregister_device(&wdev->wdog);
	gpio_unexport(wdev->gpio);
	devm_gpio_free(&hdev->dev, wdev->gpio);
	if (wdev->gpio_en) {
		gpio_unexport(wdev->gpio_en);
		devm_gpio_free(&hdev->dev, wdev->gpio_en);
	}
	devm_kfree(&hdev->dev, wdev);

	return 0;
}

static struct hzhy_gpio_driver hzhy_gpio_wdt = {
	.probe = hzhy_gpio_wdt_probe,
	.remove = hzhy_gpio_wdt_remove,
	.driver = {
		.name = "hzhy_gpio_wdt",
	},
};

static int hzhy_gpio_wdt_init(void)
{
	return hzhy_gpio_driver_register(&hzhy_gpio_wdt);
}

static void hzhy_gpio_wdt_exit(void)
{
	hzhy_gpio_driver_unregister(&hzhy_gpio_wdt);
}

module_init(hzhy_gpio_wdt_init);
module_exit(hzhy_gpio_wdt_exit);

MODULE_VERSION("V1.0.2");
MODULE_AUTHOR("chenxd@hzhytech.com");
MODULE_LICENSE("GPL");

