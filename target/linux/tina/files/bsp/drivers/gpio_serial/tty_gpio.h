#ifndef __TTY_GPIO_H__
#define __TTY_GPIO_H__
#include <linux/tty.h>
#include <linux/fs.h>
#include "gpio_serial.h"

void tty_gpio_over(struct gpio_uart *guart);
int tty_gpio_init(struct gpio_uart *guart);
void tty_gpio_set_termios(struct tty_struct *tty, struct ktermios *termios,
		struct gpio_uart *guart);
int tty_gpio_get_fifo_avail(struct tty_struct *tty, struct gpio_uart *guart);
int tty_gpio_get_fifo_len(struct tty_struct *tty, struct gpio_uart *guart);
int tty_gpio_send_string(struct tty_struct *tty,const unsigned char *string,
		int size, struct gpio_uart *guart);
int tty_gpio_open(struct tty_struct *tty, struct file *file,
		struct gpio_uart *guart);
void tty_gpio_close(struct tty_struct *tty, struct file *file,
		struct gpio_uart *guart);
#endif
