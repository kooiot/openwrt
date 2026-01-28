#include <linux/gpio.h>
#include <linux/hrtimer.h>
#include <linux/interrupt.h>
#include <linux/ktime.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include "gpio_serial.h"

static enum hrtimer_restart handle_tx(struct hrtimer* timer)
{
	struct gpio_tty *gtty =
		container_of(timer, struct gpio_tty, timer);
	struct gpio_uart *guart =
		container_of(gtty, struct gpio_uart, tx);
	ktime_t current_time = ktime_get();
	enum hrtimer_restart result = HRTIMER_NORESTART;
	bool must_restart_timer = false;
	int value;

	// Start bit.
	if (gtty->bit_index == -1) {
		if (kfifo_out(&(guart->fifo), &gtty->character, 1) == 1) {
			gpio_set_value(gtty->gpio, 0);
			gtty->bit_index++;
			gtty->high = 0;
			must_restart_timer = true;
		}
	// Data bits.
	} else if (0 <= gtty->bit_index && gtty->bit_index < gtty->databits) {
		if (gtty->bit_index < 8) {
			value =  1 & (gtty->character >> gtty->bit_index);
			if (value)
				gtty->high++;
		} else { // parity
			if (gtty->flag & PARODD) {
				value = gtty->high % 2 ? 0 : 1;
			} else {
				value = gtty->high % 2 ? 1 : 0;
			}
		}
		gpio_set_value(gtty->gpio, value);
		gtty->bit_index++;
		must_restart_timer = true;
	// Stop bit.
	} else if (gtty->bit_index == gtty->databits) {
		gpio_set_value(gtty->gpio, 1);
		gtty->character = 0;
		gtty->bit_index = -1;
		must_restart_timer = (kfifo_len(&(guart->fifo)) > 0);
	}

	// Restarts the TX timer.
	if (must_restart_timer) {
		hrtimer_forward(&gtty->timer, current_time, guart->period);
		result = HRTIMER_RESTART;
	}

	return result;
}

static int tty_gpio_set_baudrate(const int baudrate, struct gpio_uart *guart)
{
	guart->period = ktime_set(0, 1000000000/baudrate);
	gpio_set_debounce(guart->rx.gpio, 1000/baudrate/2);
	return 0;
}

int tty_gpio_open(struct tty_struct *tty, struct file *file,
		struct gpio_uart *guart)
{
	int ret = -1;
	mutex_lock(&guart->tty_mutex);
	if (guart->tty == NULL) {
		guart->tty = tty;
		INIT_KFIFO(guart->fifo);
		if (!kfifo_initialized(&(guart->fifo))) {
			ret = ENOMEM;
			goto out;
		}
		enable_irq(gpio_to_irq(guart->rx.gpio));
		ret = 0;
	}
	guart->tx.databits = 8;
	guart->rx.databits = 8;
out:
	mutex_unlock(&guart->tty_mutex);
	return ret;
}

void tty_gpio_close(struct tty_struct *tty, struct file *file,
		struct gpio_uart *guart)
{
	mutex_lock(&guart->tty_mutex);
	if (guart->tty) {
		disable_irq(gpio_to_irq(guart->rx.gpio));
		hrtimer_cancel(&guart->tx.timer);
		hrtimer_cancel(&guart->rx.timer);
		kfifo_free(&(guart->fifo));
		guart->tty = NULL;
	}
	mutex_unlock(&guart->tty_mutex);
}

int tty_gpio_send_string(struct tty_struct *tty,const unsigned char *string,
		int size, struct gpio_uart *guart)
{
	int result = 0;

	result = kfifo_in(&(guart->fifo), string, size);

	if (!hrtimer_active(&guart->tx.timer))
		hrtimer_start(&guart->tx.timer, guart->period, HRTIMER_MODE_REL);

	return result;
}

int tty_gpio_get_fifo_avail(struct tty_struct *tty, struct gpio_uart *guart)
{
	return kfifo_avail(&(guart->fifo));
}

int tty_gpio_get_fifo_len(struct tty_struct *tty, struct gpio_uart *guart)
{
	return kfifo_len(&(guart->fifo));
}

static void receive_character(unsigned char character,
		struct gpio_uart *guart)
{
	spin_lock(&guart->tty_lock);
	if (guart->tty != NULL && guart->tty->port != NULL) {
		tty_insert_flip_char(guart->tty->port, character, TTY_NORMAL);
		tty_flip_buffer_push(guart->tty->port);
	}
	spin_unlock(&guart->tty_lock);
}

void tty_gpio_set_termios(struct tty_struct *tty, struct ktermios *termios,
		struct gpio_uart *guart)
{
	int cflag = 0;
	speed_t baudrate = tty_get_baud_rate(tty);

	cflag = tty->termios.c_cflag;

	if ((cflag & CSIZE) != CS8)
		printk(KERN_ALERT "data bits not valid.\n");
	guart->tx.databits = 8;
	guart->rx.databits = 8;

	if (cflag & CSTOPB)
		printk(KERN_ALERT "stop bits not valid.\n");

	if(cflag & PARENB) {
		guart->tx.flag |= PARENB;
		guart->rx.flag |= PARENB;
		if (cflag & PARODD) {
			guart->tx.flag |= PARODD;
			guart->rx.flag |= PARODD;
		}
		guart->tx.databits = 9;
		guart->rx.databits = 9;
	}

	if (tty_gpio_set_baudrate(baudrate, guart))
		printk(KERN_ALERT "baudrate not valid.\n");
}

static enum hrtimer_restart handle_rx(struct hrtimer *timer)
{
	struct gpio_tty *gtty =
		container_of(timer, struct gpio_tty, timer);
	struct gpio_uart *guart =
		container_of(gtty, struct gpio_uart, rx);
	ktime_t current_time = ktime_get();
	enum hrtimer_restart result = HRTIMER_NORESTART;
	bool must_restart_timer = false;

	int bit_value = gpio_get_value(gtty->gpio);

	// Start bit. 判断是否为低电平
	if (gtty->bit_index == -1) {
		gtty->bit_index++;
		gtty->character = 0xff;
		must_restart_timer = true;
	// Data bits.
	} else if (0 <= gtty->bit_index && gtty->bit_index < gtty->databits) {
		if (gtty->bit_index < 8) {
			if (bit_value == 0)
				gtty->character &= ~(1 << gtty->bit_index);
		}
		gtty->bit_index++;
		must_restart_timer = true;
	// Stop bit.
	} else if (gtty->bit_index == gtty->databits) {
		receive_character(gtty->character, guart);
		gtty->character = 0xff;
		gtty->bit_index = -1;
	}

	// Restarts the RX timer.
	if (must_restart_timer) {
		hrtimer_forward(&gtty->timer, current_time, guart->period);
		result = HRTIMER_RESTART;
	}

	return result;
}

static irqreturn_t handle_rx_start(unsigned int irq, void *device,
		struct pt_regs *registers)
{
	struct gpio_uart *guart = (struct gpio_uart *)device;
	unsigned long value = ktime_to_ns(guart->period);

	if (guart->rx.bit_index == -1)
		hrtimer_start(&guart->rx.timer,
				ktime_set(0, value / 2), HRTIMER_MODE_REL);
	return IRQ_HANDLED;
}

int tty_gpio_init(struct gpio_uart *guart)
{
	int ret = 0;
	char irq_name[32] = {0x0};

	sprintf(irq_name, "gpio_uart%d", guart->gport.minor);

	mutex_init(&guart->tty_mutex);
	spin_lock_init(&guart->tty_lock);

	hrtimer_init(&guart->tx.timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	guart->tx.timer.function = &handle_tx;
	hrtimer_init(&guart->rx.timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	guart->rx.timer.function = &handle_rx;

	guart->rx.bit_index = -1;
	guart->tx.bit_index = -1;
	guart->rx.character = 0xff;
	guart->tx.character = 0;

	ret = request_irq(gpio_to_irq(guart->rx.gpio),
			(irq_handler_t)handle_rx_start, IRQF_TRIGGER_FALLING,
			irq_name, guart);
	if (ret) {
		printk("Unable to request irq for gpio %d\n", guart->rx.gpio);
		return ret;
	}
	disable_irq(gpio_to_irq(guart->rx.gpio));

	return 0;
}

void tty_gpio_over(struct gpio_uart *guart)
{
	free_irq(gpio_to_irq(guart->rx.gpio), guart);
	hrtimer_cancel(&guart->tx.timer);
	hrtimer_cancel(&guart->rx.timer);
}

