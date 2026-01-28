#ifndef __SOFT_UART_H__
#define __SOFT_UART_H__
#include <linux/tty.h>
#include <linux/kfifo.h>

#define MAX_NUM_PORT 4

struct suart_port {
	struct tty_port port;
	int minor;
	struct device *dev;
	struct suart_operations *ops;
	unsigned long flag;
};

struct gpio_data {
	const char *label;
	unsigned int dir;
	unsigned int gpio;
	unsigned int vdef;
};

struct gpio_tty {
	struct hrtimer timer;
	unsigned char character;
	int bit_index;
	int gpio;
	int flag;
	int databits;
	int high;
};

struct gpio_uart {
	struct suart_port gport;
	struct gpio_data gdata[2];
	spinlock_t tty_lock;
	struct mutex tty_mutex;
	ktime_t period;
	struct tty_struct *tty;
	DECLARE_KFIFO(fifo, char, 256);
	struct gpio_tty rx;
	struct gpio_tty tx;
};

struct suart_operations {
	int (*open)(struct tty_struct *, struct file *, struct gpio_uart *);
	void (*close)(struct tty_struct *, struct file *, struct gpio_uart *);
	int (*write)(struct tty_struct *, const unsigned char *, int,
			struct gpio_uart *);
	int (*write_room)(struct tty_struct *tty, struct gpio_uart *);
	int (*chars_in_buffer)(struct tty_struct *, struct gpio_uart *);
	int (*ioctl)(struct tty_struct *, unsigned int, unsigned int long,
			struct gpio_uart *);
	void (*set_termios)(struct tty_struct *tty, struct ktermios *,
			struct gpio_uart *);
};

int gpio_serial_init(void);
void gpio_serial_exit(void);
int gpio_serial_add_port(struct suart_port *gpio_port);
void gpio_serial_remove_port(struct suart_port *gpio_port);

#endif
