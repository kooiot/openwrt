#ifndef __HZHY__GPIO__H__
#define __HZHY__GPIO__H__
#include <linux/device.h>

#define HZHY_GPIO_KEY_DEBUG 1

struct hzhy_gpio_device {
	char *name;
	struct device dev;
	void *private_data;
};

struct hzhy_gpio_driver {
	int (*probe)(struct hzhy_gpio_device *);
	int (*remove)(struct hzhy_gpio_device *);
	struct device_driver driver;
};

#define to_hzhy_gpio_device(x) container_of((x), struct hzhy_gpio_device, dev)
#define to_hzhy_gpio_driver(x) container_of((x), struct hzhy_gpio_driver, driver)

int hzhy_gpio_device_register(struct hzhy_gpio_device *hdev);
void hzhy_gpio_device_unregister(struct hzhy_gpio_device *hdev);
int hzhy_gpio_driver_register(struct hzhy_gpio_driver *drv);
void hzhy_gpio_driver_unregister(struct hzhy_gpio_driver *drv);

int for_each_hzhy_gpio_dev(void *data, 
		int (*fn)(struct hzhy_gpio_device *, void *));
int for_each_hzhy_gpio_drv(void *data,
		int (*fn)(struct hzhy_gpio_driver *, void *));

int hzhy_gpio_key_drv_register(void);
void hzhy_gpio_key_drv_unregister(void);
int hzhy_gpios_init(void);
void hzhy_gpios_exit(void);
int hzhy_gpio_usb_rst_init(void);
void hzhy_gpio_usb_rst_exit(void);

#endif
