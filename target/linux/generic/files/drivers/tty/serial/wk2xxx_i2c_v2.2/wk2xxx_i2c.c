/*
*   FILE NAME  : wk2xxx_i2c.c
*
*   WKIC Ltd.
*   By  Xu XunWei Tech  
*   DEMO Version :2.2 Data:2021-5-28
*
*   DESCRIPTION: Implements an interface for the wk2xxx of i2c interface
*
*   1. compiler warnings all changes
*   2. v2.2(port changes from wk2xxx_spi V2.2 driver by dirk@kooiot.com)
*/




#include <linux/init.h>                        
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/console.h>
#include <linux/device.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial_core.h>
#include <linux/serial.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/freezer.h>
#include <linux/i2c.h>
#include <linux/timer.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>
#include <linux/gpio.h>


#include <linux/io.h>
#include <linux/workqueue.h>
#include <linux/platform_device.h>
//#include <asm/mach/map.h>
#include <asm/irq.h>
#include <asm/io.h>
// #include <linux/platform_data/spi-rockchip.h>
#include <uapi/linux/tty_flags.h>
#include "wk2xxx.h"



MODULE_LICENSE("Dual BSD/GPL");

//#define _DEBUG_WK_FUNCTION
//#define _DEBUG_WK_RX
//#define _DEBUG_WK_TX
//#define _DEBUG_WK_IRQ
//#define _DEBUG_WK_VALUE
//#define _DEBUG_WK_FIFO
//#define _DEBUG_WK_TEST

//#define WK_RS485_FUNCTION
#define WK_FIFO_FUNCTION
#define WK_RSTGPIO_FUNCTION

#define WK2XXX_STATUS_PE    1
#define WK2XXX_STATUS_FE    2
#define WK2XXX_STATUS_BRK   4
#define WK2XXX_STATUS_OE    8

#define I2C_LEN_LIMIT       255    //MAX<=255


static DEFINE_MUTEX(wk2xxxs_lock);                /* race on probe */
static DEFINE_MUTEX(wk2xxxs_reg_lock);
static DEFINE_MUTEX(wk2xxs_work_lock);                /* work on probe */
static DEFINE_MUTEX(wk2xxxs_global_lock);

struct wk2xxx_quirks {
	int nr_ports;
};

struct wk2xxx_priv_data {
	int rst_gpio;
};

struct wk2xxx_port 
{
	//struct timer_list mytimer;
	struct uart_port port;//[NR_PORTS];
	struct i2c_client *wk2xxx_i2c_client;
	spinlock_t conf_lock;   /* shared data */
	struct workqueue_struct *workqueue;
	struct work_struct work;
	int suspending;
	void (*wk2xxx_hw_suspend) (int suspend);
	int tx_done;

	int force_end_work;
	int irq;
	int minor;      /* minor number */
	int tx_empty; 
	int tx_empty_flag;


	int start_tx_flag;
	int stop_tx_flag;
	int stop_rx_flag; 
	int irq_flag;
	int conf_flag;

	int tx_empty_fail;
	int start_tx_fail;
	int stop_tx_fail;
	int stop_rx_fail;
	int irq_fail;
	int conf_fail;


	uint8_t new_lcr;
	uint8_t new_fwcr;
	uint8_t new_scr; 
	/*set baud 0f register*/
	uint8_t new_baud1;
	uint8_t new_baud0;
	uint8_t new_pres;

};

static struct wk2xxx_port wk2xxxs[NR_PORTS]; /* the chips */
/*
 * This function read wk2xxx of Global register:
 */
static int wk2xxx_read_global_reg(struct i2c_client *client,uint8_t greg,uint8_t *dat)
{
	struct i2c_msg msg[2];
	uint8_t cmd_addr,wk_addr = 0;
	uint8_t ret, status = 0;
	uint8_t wk_reg[1],wk_dat[1];
	cmd_addr=((wk_addr<<6)|0xE0|(greg&0x30)>>2)>>1;
	mutex_lock(&wk2xxxs_reg_lock);
	/***********************************************/
	wk_reg[0] = greg&0x0f;
	msg[0].addr = cmd_addr;
	msg[0].flags = 0;//write flags
	msg[0].len = 1;
	msg[0].buf = wk_reg;
	ret= i2c_transfer(client->adapter, &msg[0], 1);
	// printk(KERN_ALERT "wk2xxx_read_global_reg12(i2c) w_error,ret:%d!\n",ret);
	if ( ret< 0) {
		printk(KERN_ALERT "wk2xxx_read_global_reg1(i2c) w_error,ret:%d!\n",ret);
		status = 1;
		mutex_unlock(&wk2xxxs_reg_lock);
		return status;
	}

	/**********************************************/
	msg[1].addr = cmd_addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = wk_dat;
	ret=i2c_transfer(client->adapter, &msg[1], 1);
	// printk(KERN_ALERT "wk2xxx_read_global_reg2(i2c) w_error!ret:%d!\n",ret);
	if(ret!=1){
		printk(KERN_ALERT "wk2xxx_read_global_reg3(i2c) w_error!ret:%d!\n",ret);
		*dat=0x0;
		status = 1;
		mutex_unlock(&wk2xxxs_reg_lock);
		return status;
	}

	*dat=wk_dat[0];
	mutex_unlock(&wk2xxxs_reg_lock);
	return status;

}
/*
 * This function write wk2xxx of Global register:
 */
static int wk2xxx_write_global_reg(struct i2c_client *client,uint8_t greg,uint8_t dat)
{
	struct i2c_msg msg;
	uint8_t cmd_addr,wk_addr = 0;
	uint8_t status = 0;
	uint8_t wk_buf[2];
	cmd_addr=((wk_addr<<6)|0xE0|(greg&0x30)>>2)>>1;
	mutex_lock(&wk2xxxs_reg_lock);
	wk_buf[0] = greg&0x0f;
	wk_buf[1] = dat;
	msg.addr = cmd_addr;
	msg.flags = 0;
	msg.len = 2;
	msg.buf = wk_buf;
	if (i2c_transfer(client->adapter, &msg, 1) < 0) {
		printk(KERN_ALERT "wk2xxx_write_global_reg1(i2c) w_error!\n");
		status = 1;
		mutex_unlock(&wk2xxxs_reg_lock);
		return status;
	}
	mutex_unlock(&wk2xxxs_reg_lock);
	return status;
}
/*
 * This function read wk2xxx of slave register:
 */
static int wk2xxx_read_slave_reg(struct i2c_client *client,uint8_t port,uint8_t sreg,uint8_t *dat)
{        struct i2c_msg msg[2];
	uint8_t cmd_addr,wk_addr = 0;
	uint8_t ret, status = 0;
	uint8_t wk_reg[1],wk_dat[1];
	cmd_addr=((wk_addr<<6)|0xE0|(port-1)<<2)>>1;
	mutex_lock(&wk2xxxs_reg_lock);
	/***********************************************/
	wk_reg[0] = sreg&0x0f;
	msg[0].addr = cmd_addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = wk_reg;
	if (i2c_transfer(client->adapter, &msg[0], 1) < 0) {
		printk(KERN_ALERT "wk2xxx_read_slave_reg1(i2c) w_error!\n");
		status = 1;
		mutex_unlock(&wk2xxxs_reg_lock);
		return status;
	}

	/**********************************************/
	msg[1].addr = cmd_addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = wk_dat;
	ret=i2c_transfer(client->adapter, &msg[1], 1);
	if(ret!=1){
		printk(KERN_ALERT "wk2xxx_read_slave_reg2(i2c) w_error!\n");
		*dat=0x0;
		status = 1;
		mutex_unlock(&wk2xxxs_reg_lock);
		return status;
	}
	*dat=wk_dat[0];
	mutex_unlock(&wk2xxxs_reg_lock);
	return status;

}
/*
 * This function write wk2xxx of Slave register:
 */
static int wk2xxx_write_slave_reg(struct i2c_client *client,uint8_t port,uint8_t sreg,uint8_t dat)
{
	struct i2c_msg msg;
	uint8_t cmd_addr,wk_addr = 0;
	uint8_t status = 0;
	uint8_t wk_buf[2];
	cmd_addr=((wk_addr<<6)|0xE0|(port-1)<<2)>>1;
	mutex_lock(&wk2xxxs_reg_lock);
	wk_buf[0] = sreg&0x0f;
	wk_buf[1] = dat;
	msg.addr = cmd_addr;
	msg.flags = 0;
	msg.len = 2;
	msg.buf = wk_buf;
	if (i2c_transfer(client->adapter, &msg, 1) < 0) {
		printk(KERN_ALERT "wk2xxx_write_slave_reg1(i2c) w_error!\n");
		status = 1;
		mutex_unlock(&wk2xxxs_reg_lock);
		return status;
	}
	mutex_unlock(&wk2xxxs_reg_lock);
	return status;
}

#define MAX_RFCOUNT_SIZE 256

/*
 * This function read wk2xxx of fifo:
 */
static int wk2xxx_read_fifo(struct i2c_client *client,uint8_t port,uint8_t fifolen,uint8_t *dat)
{
	struct i2c_msg msg;
	uint8_t cmd_addr,wk_addr = 0;
	int i, status = 0;
	uint8_t fifo_data[256] = {0};
	cmd_addr = ((wk_addr<<6)|0xE0|(port-1)<<2|0x03) >> 1;
	if(!(fifolen>0)){
		printk(KERN_ERR "%s,read fifolen error!!\n", __func__);
		return 1;
	}
	mutex_lock(&wk2xxxs_reg_lock);
	msg.addr = cmd_addr;
	msg.flags = I2C_M_RD;
	msg.len = fifolen;
	msg.buf = fifo_data;
	if (i2c_transfer(client->adapter, &msg, 1) < 0) {
		printk(KERN_ALERT " wk2xxx_read_fifo1(i2c) w_error!\n");
		status = 1;
		mutex_unlock(&wk2xxxs_reg_lock);
		return status;
	}
	for (i = 0; i < fifolen; i++)
		*(dat + i) = fifo_data[i];
	mutex_unlock(&wk2xxxs_reg_lock);
	return status;

}
/*
 * This function write wk2xxx of fifo:
 */
static int wk2xxx_write_fifo(struct i2c_client *client,uint8_t port,uint8_t fifolen,uint8_t *dat)
{
	struct i2c_msg msg;
	uint8_t cmd_addr,wk_addr = 0;
	int i, status = 0;
	uint8_t fifo_data[256] = {0};
	cmd_addr = ((wk_addr<<6)|0xE0|(port-1)<<2|0x02) >> 1;
	if(!(fifolen>0)){
		printk(KERN_ERR "%s,wrire fifolen error!!\n", __func__);
		return 1;
	}
	for (i = 0; i < fifolen; i++)
		fifo_data[i] = *(dat + i);

	mutex_lock(&wk2xxxs_reg_lock);
	msg.addr = cmd_addr;
	msg.flags = 0;
	msg.len = fifolen;
	msg.buf = fifo_data;
	if (i2c_transfer(client->adapter, &msg, 1) < 0) {
		printk(KERN_ALERT " wk2xxx_write_fifo1(i2c) w_error!\n");
		status = 1;
		mutex_unlock(&wk2xxxs_reg_lock);
		return status;
	}
	mutex_unlock(&wk2xxxs_reg_lock);
	return status;

}

static void wk2xxxirq_app(struct uart_port *port);
static void conf_wk2xxx_subport(struct uart_port *port);
static void wk2xxx_work(struct work_struct *w);
static void wk2xxx_stop_tx(struct uart_port *port);
static u_int wk2xxx_tx_empty(struct uart_port *port);// or query the tx fifo is not empty?

static int wk2xxx_dowork(struct wk2xxx_port *s)
{    
#ifdef _DEBUG_WK_FUNCTION
	printk(KERN_ALERT "%s!!-port:%ld;--in--\n", __func__,s->port.iobase);
#endif

	if (!s->force_end_work && !work_pending(&s->work) && !freezing(current) && !s->suspending)
	{
		queue_work(s->workqueue, &s->work);//
#ifdef _DEBUG_WK_FUNCTION
		printk( "%s!!--queue_work---ok!---\n", __func__);
		//printk("work_pending =: %d s->force_end_work  = : %d freezing(current) = :%d s->suspending= :%d\n" ,work_pending(&s->work),s->force_end_work ,freezing(current),s->suspending);
#endif
		return 1;
	}
	else
	{ 
#ifdef _DEBUG_WK_FUNCTION
		printk( "%s!!--queue_work---error!---\n", __func__);
		printk("work_pending =: %d s->force_end_work  = : %d freezing(current) = :%d s->suspending= :%d\n" ,work_pending(&s->work),s->force_end_work ,freezing(current),s->suspending);
#endif
		//printk("work_pending =: %d s->force_end_work  = : %d freezing(current) = :%d s->suspending= :%d\n" ,work_pending(&s->work),s->force_end_work ,freezing(current),s->suspending);
		return 0;
	}

}

static void wk2xxx_work(struct work_struct *w)
{  


	struct wk2xxx_port *s = container_of(w, struct wk2xxx_port, work);
	uint8_t rx;
	int work_start_tx_flag; 
	int work_stop_rx_flag;
	int work_irq_flag;
	// int work_conf_flag;
#ifdef _DEBUG_WK_FUNCTION
	printk(KERN_ALERT "%s!!-port:%ld;--in--\n", __func__,s->port.iobase);
#endif
	do {

		mutex_lock(&wk2xxs_work_lock);

		work_start_tx_flag = s->start_tx_flag;
		if(work_start_tx_flag)
			s->start_tx_flag = 0;

		work_stop_rx_flag = s->stop_rx_flag;
		if(work_stop_rx_flag)
			s->stop_rx_flag = 0;

		//work_conf_flag = s->conf_flag;
		//if( work_conf_flag)
		//   s->conf_flag = 0;

		work_irq_flag = s->irq_flag;
		if(work_irq_flag)
			s->irq_flag = 0;
		mutex_unlock(&wk2xxs_work_lock);

		if(work_start_tx_flag)
		{
			wk2xxx_read_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_SIER,&rx);
			rx |= WK2XXX_TFTRIG_IEN|WK2XXX_RFTRIG_IEN|WK2XXX_RXOUT_IEN;
			wk2xxx_write_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_SIER,rx);
		}

		if(work_stop_rx_flag)
		{
			wk2xxx_read_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_SIER,&rx);
			rx &=~WK2XXX_RFTRIG_IEN;
			rx &=~WK2XXX_RXOUT_IEN;
			wk2xxx_write_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_SIER,rx);
		}

		if(work_irq_flag)
		{
			wk2xxxirq_app(&s->port);
			s->irq_fail = 1;
		}



	}while (!s->force_end_work && !freezing(current) && \
			(work_irq_flag || work_stop_rx_flag ));


	if(s->start_tx_fail)
	{
		wk2xxx_read_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_SIER,&rx);
		rx |= WK2XXX_TFTRIG_IEN|WK2XXX_RFTRIG_IEN|WK2XXX_RXOUT_IEN;
		wk2xxx_write_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_SIER,rx);
		s->start_tx_fail =0;
	}


	if(s->stop_rx_fail)
	{
		wk2xxx_read_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_SIER,&rx);
		rx &=~WK2XXX_RFTRIG_IEN;
		rx &=~WK2XXX_RXOUT_IEN;
		wk2xxx_write_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_SIER,rx);
		s->stop_rx_fail =0;
	}
	if(s->irq_fail)
	{
		s->irq_fail = 0;
		enable_irq(s->port.irq);
	}

#ifdef _DEBUG_WK_FUNCTION
	printk(KERN_ALERT "%s!!-port:%ld;--exit--\n", __func__,s->port.iobase);
#endif
}


static void wk2xxx_rx_chars(struct uart_port *port)
{


	struct wk2xxx_port *s = container_of(port,struct wk2xxx_port,port);
	uint8_t fsr,lsr,dat[1],rx_dat[256]={0};
	unsigned int ch,flg,sifr, ignored=0,status = 0,rx_count=0;
	int rfcnt=0,rfcnt2=0,rx_num=0;
	int len_rfcnt,len_limit,len_p=0;
	len_limit=I2C_LEN_LIMIT;
#ifdef _DEBUG_WK_FUNCTION
	printk(KERN_ALERT "%s!!-port:%ld;--in--\n", __func__,s->port.iobase);
#endif
	wk2xxx_write_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_SPAGE,WK2XXX_PAGE0);//set register in page0
	wk2xxx_read_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_FSR,dat);
	fsr = dat[0];
	wk2xxx_read_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_LSR,dat);
	lsr = dat[0];
	wk2xxx_read_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_SIFR,dat);
	sifr=dat[0];
#ifdef _DEBUG_WK_RX
	printk(KERN_ALERT "rx_chars()-port:%lx--fsr:0x%x--lsr:0x%x--\n",s->port.iobase,fsr,lsr);
#endif
	if(!(sifr&0x80))//no error
	{  
		flg = TTY_NORMAL;
		if (fsr& WK2XXX_RDAT)
		{
			wk2xxx_read_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_RFCNT,dat);
			rfcnt=dat[0];
			wk2xxx_read_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_RFCNT,dat);
			rfcnt2=dat[0];

			if(!(rfcnt2>=rfcnt))
			{
				rfcnt=rfcnt2;
			}
			rfcnt=(rfcnt==0)?256:rfcnt;	
#ifdef  _DEBUG_WK_RX
			printk(KERN_ALERT "rx_chars()--port:%ld--rfcnt:0x%x-\n",s->port.iobase,rfcnt);
#endif

#ifdef WK_FIFO_FUNCTION  
			//wk2xxx_read_fifo(s->spi_wk,s->port.iobase, rfcnt,rx_dat);
			len_rfcnt=rfcnt;
			while(len_rfcnt)
			{
				if(len_rfcnt>len_limit)
				{
					wk2xxx_read_fifo(s->wk2xxx_i2c_client,s->port.iobase,len_limit,rx_dat+len_p);
					len_rfcnt=len_rfcnt-len_limit;
					len_p=len_p+len_limit;
				}
				else
				{
					wk2xxx_read_fifo(s->wk2xxx_i2c_client,s->port.iobase,len_rfcnt,rx_dat+len_p);//
					len_rfcnt=0;
				}
			}


#else
			for(rx_num=0;rx_num<rfcnt;rx_num++)
			{
				wk2xxx_read_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_FDAT,dat);
				rx_dat[rx_num]=dat[0];
			}
#endif

			s->port.icount.rx+=rfcnt;
			for(rx_num=0;rx_num<rfcnt;rx_num++)
			{
				if (uart_handle_sysrq_char(&s->port,rx_dat[rx_num]))//.state, ch))
					break;//

#ifdef _DEBUG_WK_RX
				printk(KERN_ALERT "rx_chars:0x%x----\n",rx_dat[rx_num]);
#endif
				uart_insert_char(&s->port, status, WK2XXX_STATUS_OE, rx_dat[rx_num], flg);
				rx_count++;
				if ((rx_count >= 64 ) && (s->port.state->port.tty->port != NULL))
				{
					tty_flip_buffer_push(s->port.state->port.tty->port);
					rx_count = 0;
				}
			}//for
			if((rx_count > 0)&&(s->port.state->port.tty->port!= NULL))
			{
#ifdef _DEBUG_WK_RX
				printk(KERN_ALERT  "push buffer tty flip port = :%lx count =:%d\n",s->port.iobase,rx_count);
#endif
				tty_flip_buffer_push(s->port.state->port.tty->port);
				rx_count = 0;
			}

		}
	}//ifm
	else//error
	{
		while (fsr& WK2XXX_RDAT)/**/
		{
			wk2xxx_read_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_FDAT,dat);
			ch = (int)dat[0];

#ifdef _DEBUG_WK_RX
			printk(KERN_ALERT "wk2xxx_rx_chars()----port:%lx--RXDAT:0x%x----\n",s->port.iobase,ch);
#endif

			s->port.icount.rx++;
#ifdef _DEBUG_WK_RX
			printk(KERN_ALERT "wk2xxx_rx_chars()----port:%lx error\n",s->port.iobase);
#endif
			flg = TTY_NORMAL;
			if (lsr&(WK2XXX_OE |WK2XXX_FE|WK2XXX_PE|WK2XXX_BI))
			{
				printk(KERN_ALERT "wk2xxx_rx_chars()----port:%lx error,lsr:%x!!!!!!!!!!!!!!!!!\n",s->port.iobase,lsr);
				//goto handle_error;
				if (lsr & WK2XXX_PE)
				{
					s->port.icount.parity++;
					status |= WK2XXX_STATUS_PE;
					flg = TTY_PARITY;
				}
				if (lsr & WK2XXX_FE)
				{
					s->port.icount.frame++;
					status |= WK2XXX_STATUS_FE;
					flg = TTY_FRAME;
				}
				if (lsr & WK2XXX_OE)
				{
					s->port.icount.overrun++;
					status |= WK2XXX_STATUS_OE;
					flg = TTY_OVERRUN;
				}
				if(lsr&fsr & WK2XXX_BI)
				{
					s->port.icount.brk++;
					status |= WK2XXX_STATUS_BRK;
					flg = TTY_BREAK;
				}

				if (++ignored > 100) 
					goto out;

				goto ignore_char;       
			}

error_return:
			if (uart_handle_sysrq_char(&s->port,ch))//.state, ch
			{
				goto ignore_char;
			}

			uart_insert_char(&s->port, status, WK2XXX_STATUS_OE, ch, flg);
			rx_count++;

			if ((rx_count >= 64 ) && (s->port.state->port.tty->port != NULL)) 
			{
				tty_flip_buffer_push(s->port.state->port.tty->port);
				rx_count = 0;
			} 
#ifdef _DEBUG_WK_RX
			printk(KERN_ALERT  " s->port.icount.rx = 0x%x char = 0x%x flg = 0x%X port = %lx rx_count = %d\n",s->port.icount.rx,ch,flg,s->port.iobase,rx_count);
#endif
ignore_char:

			wk2xxx_read_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_FSR,dat);
			fsr = dat[0];
			wk2xxx_read_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_LSR,dat);
			lsr = dat[0];
		}
out:
		if((rx_count > 0)&&(s->port.state->port.tty->port != NULL))
		{
#ifdef _DEBUG_WK_FUNCTION
			printk(KERN_ALERT  "push buffer tty flip port = :%lx count = :%d\n",s->port.iobase,rx_count);
#endif
			tty_flip_buffer_push(s->port.state->port.tty->port);
			rx_count = 0;
		}

	}//if()else

#ifdef _DEBUG_WK_FUNCTION
	printk(KERN_ALERT "%s!!-port:%ld;--exit--\n", __func__,s->port.iobase);
#endif
	return;
#ifdef SUPPORT_SYSRQ
	s->port.state->sysrq = 0;
#endif
	goto error_return;
#ifdef _DEBUG_WK_FUNCTION
	printk(KERN_ALERT "%s!!-port:%ld;--exit--\n", __func__,s->port.iobase);
#endif

}

static void wk2xxx_tx_chars(struct uart_port *port)
{


	struct wk2xxx_port *s = container_of(port,struct wk2xxx_port,port);
	uint8_t fsr,tfcnt,dat[1],txbuf[256]={0};
	int count,tx_count,i;
	int len_tfcnt,len_limit,len_p=0;
	len_limit=I2C_LEN_LIMIT;
#ifdef _DEBUG_WK_FUNCTION
	printk(KERN_ALERT "%s!!-port:%ld;--in--\n", __func__,s->port.iobase);
#endif
	if (s->port.x_char) 
	{   
#ifdef _DEBUG_WK_TX
		printk(KERN_ALERT "wk2xxx_tx_chars   s->port.x_char:%x,port = %ld\n",s->port.x_char,s->port.iobase);
#endif
		wk2xxx_write_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_FDAT,s->port.x_char);
		s->port.icount.tx++;
		s->port.x_char = 0;
		goto out;
	}

	if(uart_circ_empty(&s->port.state->xmit) || uart_tx_stopped(&s->port))
	{
		goto out;

	}

	wk2xxx_read_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_FSR,dat);
	fsr = dat[0];

	wk2xxx_read_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_TFCNT,dat); 
	tfcnt= dat[0];
#ifdef _DEBUG_WK_TX
	printk(KERN_ALERT "wk2xxx_tx_chars   fsr:0x%x,tfcnt:0x%x,port = %ld\n",fsr,tfcnt,s->port.iobase);
#endif
	if(tfcnt==0)
	{
		tx_count=(fsr & WK2XXX_TFULL)?0:256;
#ifdef _DEBUG_WK_TX
		printk(KERN_ALERT "wk2xxx_tx_chars2   tx_count:%x,port = %ld\n",tx_count,s->port.iobase);
#endif
	}
	else
	{
		tx_count=256-tfcnt;
#ifdef _DEBUG_WK_TX
		printk(KERN_ALERT "wk2xxx_tx_chars2   tx_count:%x,port = %ld\n",tx_count,s->port.iobase);
#endif 
	}

	count = tx_count;
	i=0;
	while(count)
	{
		if(uart_circ_empty(&s->port.state->xmit))
			break;
		txbuf[i]=s->port.state->xmit.buf[s->port.state->xmit.tail];
		s->port.state->xmit.tail = (s->port.state->xmit.tail + 1) & (UART_XMIT_SIZE - 1);
		s->port.icount.tx++;
		i++;
		count=count-1;
#ifdef _DEBUG_WK_TX
		printk(KERN_ALERT "tx_chars:0x%x--\n",txbuf[i-1]);
#endif

	};

#ifdef WK_FIFO_FUNCTION 

	//wk2xxx_write_fifo(s->wk2xxx_i2c_client,s->port.iobase,i,txbuf);
	len_tfcnt=i;    
	while(len_tfcnt)
	{
		if(len_tfcnt>len_limit)
		{


			wk2xxx_write_fifo(s->wk2xxx_i2c_client,s->port.iobase,len_limit,txbuf+len_p);	
			len_p=len_p+len_limit;
			len_tfcnt=len_tfcnt-len_limit;

		}
		else
		{

			wk2xxx_write_fifo(s->wk2xxx_i2c_client,s->port.iobase,len_tfcnt,txbuf+len_p);
			len_p=len_p+len_tfcnt;
			len_tfcnt=0;
		}
	}
#else
	for(count=0;count<i;count++)
	{
		wk2xxx_write_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_FDAT,txbuf[count]);	
	}
#endif


#ifdef _DEBUG_WK_VALUE
	printk(KERN_ALERT "icount.tx:%d,xmit.head1:%d,xmit.tail:%d,UART_XMIT_SIZE::%lx,char:%x,fsr:0x%x,port = %ld\n",s->port.icount.tx,s->port.state->xmit.head,s->port.state->xmit.tail,UART_XMIT_SIZE,s->port.state->xmit.buf[s->port.state->xmit.tail],fsr,s->port.iobase);
#endif
out:wk2xxx_read_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_FSR,dat);
	fsr = dat[0];
	if(((fsr&WK2XXX_TDAT)==0)&&((fsr&WK2XXX_TBUSY)==0))
	{
		if (uart_circ_chars_pending(&s->port.state->xmit) < WAKEUP_CHARS)
			uart_write_wakeup(&s->port); 

		if (uart_circ_empty(&s->port.state->xmit))
		{
			wk2xxx_stop_tx(&s->port);
		}
	}
#ifdef _DEBUG_WK_FUNCTION
	printk(KERN_ALERT "%s!!-port:%ld;--exit--\n", __func__,s->port.iobase);
#endif


}

static irqreturn_t wk2xxx_irq(int irq, void *dev_id)//
{
	struct wk2xxx_port *s = dev_id;
	disable_irq_nosync(s->port.irq);
#ifdef _DEBUG_WK_FUNCTION
	printk(KERN_ALERT "%s!!-port:%ld;--in--\n", __func__,s->port.iobase);
#endif       
	s->irq_flag = 1;
	if(wk2xxx_dowork(s))
	{
		;
	}
	else
	{
		s->irq_flag = 0;
		s->irq_fail = 1;    
	}

#ifdef _DEBUG_WK_FUNCTION
	printk(KERN_ALERT "%s!!-port:%ld;--exit--\n", __func__,s->port.iobase);
#endif

	return IRQ_HANDLED;
}

static void wk2xxxirq_app(struct uart_port *port)//
{
	struct wk2xxx_port *s = container_of(port,struct wk2xxx_port,port);
	unsigned int  pass_counter = 0;
	uint8_t sifr,gifr,sier,dat[1];

#ifdef _DEBUG_WK_IRQ
	uint8_t gier,sifr0,sifr1,sifr2,sifr3,sier1,sier0,sier2,sier3;
#endif

#ifdef _DEBUG_WK_FUNCTION
	printk(KERN_ALERT "%s!!-port:%ld;--in--\n", __func__,s->port.iobase);
#endif

	wk2xxx_read_global_reg(s->wk2xxx_i2c_client,WK2XXX_GIFR ,dat);
	gifr = dat[0];
#ifdef _DEBUG_WK_IRQ
	wk2xxx_read_global_reg(s->wk2xxx_i2c_client,WK2XXX_GIER ,dat);
	gier = dat[0];
	wk2xxx_read_slave_reg(s->wk2xxx_i2c_client,1,WK2XXX_SIFR,&sifr0);
	wk2xxx_read_slave_reg(s->wk2xxx_i2c_client,2,WK2XXX_SIFR,&sifr1);
	wk2xxx_read_slave_reg(s->wk2xxx_i2c_client,3,WK2XXX_SIFR,&sifr2);
	wk2xxx_read_slave_reg(s->wk2xxx_i2c_client,4,WK2XXX_SIFR,&sifr3);
	wk2xxx_read_slave_reg(s->wk2xxx_i2c_client,1,WK2XXX_SIER,&sier0);
	wk2xxx_read_slave_reg(s->wk2xxx_i2c_client,2,WK2XXX_SIER,&sier1);
	wk2xxx_read_slave_reg(s->wk2xxx_i2c_client,3,WK2XXX_SIER,&sier2);
	wk2xxx_read_slave_reg(s->wk2xxx_i2c_client,4,WK2XXX_SIER,&sier3);
	printk(KERN_ALERT "irq_app....gifr:%x  gier:%x  sier1:%x  sier2:%x sier3:%x sier4:%x   sifr1:%x sifr2:%x sifr3:%x sifr4:%x \n",gifr,gier,sier0,sier1,sier2,sier3,sifr0,sifr1,sifr2,sifr3);
#endif      



	switch(s->port.iobase)
	{
		case 1 :
			if(!(gifr & WK2XXX_UT1INT))
			{
				return;
			}
			break;
		case 2 :
			if(!(gifr & WK2XXX_UT2INT))
			{            
				return;
			}                                     
			break;
		case 3 :
			if(!(gifr & WK2XXX_UT3INT))
			{            
				return;
			}
			break;
		case 4 :
			if(!(gifr & WK2XXX_UT4INT))
			{               
				return;
			}
			break;
		default:
			break;

	}

	wk2xxx_read_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_SIFR,dat);
	sifr = dat[0];
	wk2xxx_read_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_SIER,dat);
	sier = dat[0];
#ifdef _DEBUG_WK_IRQ
	printk(KERN_ALERT "irq_app....port:%ld......sifr:%x sier:%x \n",s->port.iobase,sifr,sier);
#endif
	do {
		if ((sifr&WK2XXX_RFTRIG_INT)||(sifr&WK2XXX_RXOVT_INT))
		{
			wk2xxx_rx_chars(&s->port);
		}

		if ((sifr & WK2XXX_TFTRIG_INT)&&(sier & WK2XXX_TFTRIG_IEN ))
		{
			wk2xxx_tx_chars(&s->port);
			return;
		}
		if (pass_counter++ > WK2XXX_ISR_PASS_LIMIT)
			break;
		wk2xxx_read_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_SIFR,dat);
		sifr = dat[0];
		wk2xxx_read_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_SIER,dat);
		sier = dat[0];
#ifdef _DEBUG_WK_VALUE
		printk(KERN_ALERT "irq_app...........rx............tx  sifr:%x sier:%x port:%ld\n",sifr,sier,s->port.iobase);
#endif
	} while ((sifr&WK2XXX_RXOVT_INT)||(sifr & WK2XXX_RFTRIG_INT)||((sifr & WK2XXX_TFTRIG_INT)&&(sier & WK2XXX_TFTRIG_IEN)));
#ifdef _DEBUG_WK_FUNCTION
	printk(KERN_ALERT "%s!!-port:%ld;--exit--\n", __func__,s->port.iobase);
#endif

}


/*
 *   Return TIOCSER_TEMT when transmitter is not busy.
 */

static u_int wk2xxx_tx_empty(struct uart_port *port)// or query the tx fifo is not empty?
{
	uint8_t tx;
	struct wk2xxx_port *s = container_of(port,struct wk2xxx_port,port);
#ifdef _DEBUG_WK_FUNCTION
	printk(KERN_ALERT "%s!!-port:%ld;--in--\n", __func__,s->port.iobase);
#endif
	mutex_lock(&wk2xxxs_lock);
	if(!(s->tx_empty_flag || s->tx_empty_fail))
	{
		wk2xxx_read_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_FSR,&tx);
		while((tx & WK2XXX_TDAT)|(tx&WK2XXX_TBUSY))
		{
			wk2xxx_read_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_FSR,&tx);
		}
		s->tx_empty = ((tx & WK2XXX_TDAT)|(tx&WK2XXX_TBUSY))<=0;
		if(s->tx_empty)
		{
			s->tx_empty_flag =0;
			s->tx_empty_fail=0;
		}
		else
		{
			s->tx_empty_fail=0;
			s->tx_empty_flag =0;
		}
	}
	mutex_unlock(&wk2xxxs_lock);

#ifdef _DEBUG_WK_VALUE
	printk(KERN_ALERT "%s!!----FSR:%d--s->tx_empty:%d--\n",__func__,tx,s->tx_empty);
#endif

#ifdef _DEBUG_WK_FUNCTION
	printk(KERN_ALERT "%s!!-port:%ld;--exit--\n", __func__,s->port.iobase);
#endif
	return s->tx_empty;


}

static void wk2xxx_set_mctrl(struct uart_port *port, u_int mctrl)//nothing
{
#ifdef _DEBUG_WK_FUNCTION
	printk(KERN_ALERT "%s!!---in--\n", __func__);
#endif
}
static u_int wk2xxx_get_mctrl(struct uart_port *port)// since no modem control line
{       
#ifdef _DEBUG_WK_FUNCTION
	printk(KERN_ALERT "%s!!---in--\n", __func__);
#endif
	return TIOCM_CTS | TIOCM_DSR | TIOCM_CAR;
}


/*
 *  interrupts disabled on entry
 */

static void wk2xxx_stop_tx(struct uart_port *port)//
{
	uint8_t dat[1],sier,sifr;
	struct wk2xxx_port *s = container_of(port,struct wk2xxx_port,port);
#ifdef _DEBUG_WK_FUNCTION
	printk(KERN_ALERT "%s!!-port:%ld;--in--\n", __func__,s->port.iobase);
#endif

	mutex_lock(&wk2xxxs_lock);
	if(!(s->stop_tx_flag||s->stop_tx_fail))
	{
		wk2xxx_read_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_SIER,dat);
		sier=dat[0];
		s->stop_tx_fail=(sier&WK2XXX_TFTRIG_IEN)>0;
		if(s->stop_tx_fail)
		{
			wk2xxx_read_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_SIER,dat);
			sier=dat[0];
			sier&=~WK2XXX_TFTRIG_IEN;
			wk2xxx_write_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_SIER,sier);

			wk2xxx_read_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_SIFR,dat);
			sifr=dat[0];
			sifr&= ~WK2XXX_TFTRIG_INT;
			wk2xxx_write_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_SIFR,sifr);
			s->stop_tx_fail =0;
			s->stop_tx_flag=0;
		}
		else
		{
			s->stop_tx_fail =0;
			s->stop_tx_flag=0;
		}
	}
	mutex_unlock(&wk2xxxs_lock); 
#ifdef _DEBUG_WK_FUNCTION
	printk(KERN_ALERT "%s!!-port:%ld;--exit--\n", __func__,s->port.iobase);
#endif
}

/*
 *  * interrupts may not be disabled on entry
 */
static void wk2xxx_start_tx(struct uart_port *port)
{

	struct wk2xxx_port *s = container_of(port,struct wk2xxx_port,port);
#ifdef _DEBUG_WK_FUNCTION
	printk(KERN_ALERT "%s!!-port:%ld;--in--\n", __func__,s->port.iobase);
#endif
	if(!(s->start_tx_flag||s->start_tx_fail))
	{    
		s->start_tx_flag = 1;
		if(wk2xxx_dowork(s))
		{
			;
		}
		else
		{
			s->start_tx_fail = 1;
			s->start_tx_flag = 0;
		}
	}

#ifdef _DEBUG_WK_FUNCTION
	printk(KERN_ALERT "%s!!-port:%ld;--exit--\n", __func__,s->port.iobase);
#endif



}

/*
 *  * Interrupts enabled
 */

static void wk2xxx_stop_rx(struct uart_port *port)
{
	struct wk2xxx_port *s = container_of(port,struct wk2xxx_port,port);
#ifdef _DEBUG_WK_FUNCTION
	printk(KERN_ALERT "%s!!-port:%ld;--in--\n", __func__,s->port.iobase);
#endif
	if(!(s->stop_rx_flag ||s->stop_rx_fail ))
	{
		s->stop_rx_flag = 1;
		if(wk2xxx_dowork(s))
		{
			;
		}
		else
		{
			s->stop_rx_flag = 0;
			s->stop_rx_fail = 1;
		}
	}


#ifdef _DEBUG_WK_FUNCTION
	printk(KERN_ALERT "%s!!-port:%ld;--exit--\n", __func__,s->port.iobase);
#endif


}


/*
 *  * No modem control lines
 *   */
static void wk2xxx_enable_ms(struct uart_port *port)    //nothing
{
#ifdef _DEBUG_WK_FUNCTION
	printk(KERN_ALERT "%s!!---in--\n", __func__);
#endif


}
/*
 *  * Interrupts always disabled.
 */   
static void wk2xxx_break_ctl(struct uart_port *port, int break_state)
{
#ifdef _DEBUG_WK_FUNCTION
	printk(KERN_ALERT "%s!!---in--\n", __func__);
#endif
}


static int wk2xxx_startup(struct uart_port *port)//i
{

	uint8_t gena,grst,gier,sier,scr,dat[1];
	struct wk2xxx_port *s = container_of(port,struct wk2xxx_port,port);
	char b[12];
#ifdef _DEBUG_WK_FUNCTION
	printk(KERN_ALERT "%s!!-port:%ld;--in--\n", __func__,s->port.iobase);
#endif
	if (s->suspending)
		return 0;

	s->force_end_work = 0;
	sprintf(b, "wk2xxx-%d", (uint8_t)s->port.iobase);
	s->workqueue = create_workqueue(b);

	if (!s->workqueue) 
	{
		dev_warn(&s->wk2xxx_i2c_client->dev, "cannot create workqueue\n");
		return -EBUSY;
	}

	INIT_WORK(&s->work, wk2xxx_work);

	if (s->wk2xxx_hw_suspend)
		s->wk2xxx_hw_suspend(0);
	mutex_lock(&wk2xxxs_global_lock);
	wk2xxx_read_global_reg(s->wk2xxx_i2c_client,WK2XXX_GENA,dat);
	gena=dat[0];
	switch (s->port.iobase)
	{
		case 1:
			gena|=WK2XXX_UT1EN;
			wk2xxx_write_global_reg(s->wk2xxx_i2c_client,WK2XXX_GENA,gena);
			break;
		case 2:
			gena|=WK2XXX_UT2EN;
			wk2xxx_write_global_reg(s->wk2xxx_i2c_client,WK2XXX_GENA,gena);
			break;
		case 3:
			gena|=WK2XXX_UT3EN;
			wk2xxx_write_global_reg(s->wk2xxx_i2c_client,WK2XXX_GENA,gena);
			break;
		case 4:
			gena|=WK2XXX_UT4EN;
			wk2xxx_write_global_reg(s->wk2xxx_i2c_client,WK2XXX_GENA,gena);
			break;
		default:
			printk(KERN_ALERT ":con_wk2xxx_subport bad iobase %d\n", (uint8_t)s->port.iobase);
			break;
	}

	wk2xxx_read_global_reg(s->wk2xxx_i2c_client,WK2XXX_GRST,dat);
	grst=dat[0];
	switch (s->port.iobase)
	{
		case 1:
			grst|=WK2XXX_UT1RST;
			wk2xxx_write_global_reg(s->wk2xxx_i2c_client,WK2XXX_GRST,grst);
			break;
		case 2:
			grst|=WK2XXX_UT2RST;
			wk2xxx_write_global_reg(s->wk2xxx_i2c_client,WK2XXX_GRST,grst);
			break;
		case 3:
			grst|=WK2XXX_UT3RST;
			wk2xxx_write_global_reg(s->wk2xxx_i2c_client,WK2XXX_GRST,grst);
			break;
		case 4:
			grst|=WK2XXX_UT4RST;
			wk2xxx_write_global_reg(s->wk2xxx_i2c_client,WK2XXX_GRST,grst);
			break;
		default:
			printk(KERN_ALERT ":con_wk2xxx_subport bad iobase %d\n", (uint8_t)s->port.iobase);
			break;
	}

	//enable the sub port interrupt
	wk2xxx_read_global_reg(s->wk2xxx_i2c_client,WK2XXX_GIER,dat);
	gier = dat[0];
	switch (s->port.iobase)
	{
		case 1:
			gier|=WK2XXX_UT1IE;
			wk2xxx_write_global_reg(s->wk2xxx_i2c_client,WK2XXX_GIER,gier);
			break;
		case 2:
			gier|=WK2XXX_UT2IE;
			wk2xxx_write_global_reg(s->wk2xxx_i2c_client,WK2XXX_GIER,gier);
			break;
		case 3:
			gier|=WK2XXX_UT3IE;
			wk2xxx_write_global_reg(s->wk2xxx_i2c_client,WK2XXX_GIER,gier);
			break;
		case 4:
			gier|=WK2XXX_UT4IE;
			wk2xxx_write_global_reg(s->wk2xxx_i2c_client,WK2XXX_GIER,gier);
			break;
		default:
			printk(KERN_ALERT ": bad iobase %d\n", (uint8_t)s->port.iobase);
			break;
	}


	mutex_unlock(&wk2xxxs_global_lock);

	wk2xxx_read_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_SIER,dat);
	sier = dat[0];
	sier &= ~WK2XXX_TFTRIG_IEN;
	sier |= WK2XXX_RFTRIG_IEN;
	sier |= WK2XXX_RXOUT_IEN;
	wk2xxx_write_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_SIER,sier);

	wk2xxx_read_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_SCR,dat);
	scr = dat[0] | WK2XXX_TXEN|WK2XXX_RXEN;
	wk2xxx_write_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_SCR,scr);

	//initiate the fifos
	wk2xxx_write_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_FCR,0xff);//initiate the fifos
	wk2xxx_write_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_FCR,0xfc);
	//set rx/tx interrupt 
	wk2xxx_write_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_SPAGE,1);  
	wk2xxx_write_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_RFTL,0x40);
	wk2xxx_write_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_TFTL,0X20);
	wk2xxx_write_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_SPAGE,0);  
	/*enable rs485*/
#ifdef WK_RS485_FUNCTION
	wk2xxx_write_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_RS485,0X02);//default  high
	//wk2xxx_write_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_RS485,0X03);//default low
	wk2xxx_write_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_SPAGE,0X01);
	wk2xxx_write_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_RRSDLY,0X10);
	wk2xxx_write_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_SPAGE,0X00);
#endif
	/*****************************test**************************************/
#ifdef _DEBUG_WK_TEST
	wk2xxx_read_global_reg(s->wk2xxx_i2c_client,WK2XXX_GENA,&gena);
	wk2xxx_read_global_reg(s->wk2xxx_i2c_client,WK2XXX_GIER,&gier);
	wk2xxx_read_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_SIER,&sier);
	wk2xxx_read_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_SCR,&scr);
	wk2xxx_read_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_FCR,dat);
	printk(KERN_ALERT "%s!!-port:%ld;gena:0x%x;gier:0x%x;sier:0x%x;scr:0x%x;fcr:0x%x----\n", __func__,s->port.iobase,gena,gier,sier,scr,dat[0]);	
#endif
	/**********************************************************************/
	if (s->wk2xxx_hw_suspend)
		s->wk2xxx_hw_suspend(0);
	msleep(50);


	uart_circ_clear(&s->port.state->xmit);
	wk2xxx_enable_ms(&s->port);

	// request irq
	if(request_irq(s->port.irq, wk2xxx_irq,IRQF_SHARED|IRQF_TRIGGER_LOW, "wk2xxx_irq_gpio", s) < 0)
	{
		dev_warn(&s->wk2xxx_i2c_client->dev, "cannot allocate irq %d\n", s->irq);
		s->port.irq = 0;
		destroy_workqueue(s->workqueue);
		s->workqueue = NULL;
		return -EBUSY;
	} else {
		dev_err(&s->wk2xxx_i2c_client->dev, "allocate irq %d\n", s->irq);
	}
      	udelay(100);
	udelay(100);
#ifdef _DEBUG_WK_FUNCTION
	printk(KERN_ALERT "%s!!-port:%ld;--exit--\n", __func__,s->port.iobase);
#endif

	return 0;
}
//* Power down all displays on reboot, poweroff or halt *

static void wk2xxx_shutdown(struct uart_port *port)
{

	uint8_t gena,dat[1];
	struct wk2xxx_port *s = container_of(port,struct wk2xxx_port,port);
#ifdef _DEBUG_WK_FUNCTION
	printk(KERN_ALERT "%s!!-port:%ld;--in--\n", __func__,s->port.iobase);
#endif
	if (s->suspending)
		return;
	s->force_end_work = 1;
	if (s->workqueue) 
	{
		flush_workqueue(s->workqueue);
		destroy_workqueue(s->workqueue);
		s->workqueue = NULL;
	}

	if (s->port.irq)
	{    
		free_irq(s->port.irq, s);//释放中断
	}
	mutex_lock(&wk2xxxs_global_lock);
	wk2xxx_read_global_reg(s->wk2xxx_i2c_client,WK2XXX_GENA,dat);
	gena=dat[0];
	switch (s->port.iobase)
	{
		case 1:
			gena&=~WK2XXX_UT1EN;
			wk2xxx_write_global_reg(s->wk2xxx_i2c_client,WK2XXX_GENA,gena);
			break;
		case 2:
			gena&=~WK2XXX_UT2EN;
			wk2xxx_write_global_reg(s->wk2xxx_i2c_client,WK2XXX_GENA,gena);
			break;
		case 3:
			gena&=~WK2XXX_UT3EN;
			wk2xxx_write_global_reg(s->wk2xxx_i2c_client,WK2XXX_GENA,gena);
			break;
		case 4:
			gena&=~WK2XXX_UT4EN;
			wk2xxx_write_global_reg(s->wk2xxx_i2c_client,WK2XXX_GENA,gena);
			break;
		default:
			printk(KERN_ALERT ":con_wk2xxx_subport bad iobase %d\n", (uint8_t)s->port.iobase);
			break;
	} 

	mutex_unlock(&wk2xxxs_global_lock);

#ifdef _DEBUG_WK_FUNCTION
	printk(KERN_ALERT "%s!!-port:%ld;--exit--\n", __func__,s->port.iobase);
#endif

}

static void conf_wk2xxx_subport(struct uart_port *port)//i
{   


	struct wk2xxx_port *s = container_of(port,struct wk2xxx_port,port);
	uint8_t old_sier,fwcr,lcr,scr,scr_ss,dat[1],baud0_ss,baud1_ss,pres_ss;
#ifdef _DEBUG_WK_FUNCTION
	printk(KERN_ALERT "%s!!-port:%ld;--in--\n", __func__,s->port.iobase);
#endif
	lcr = s->new_lcr;
	scr_ss = s->new_scr;
	baud0_ss=s->new_baud0;
	baud1_ss=s->new_baud1;
	pres_ss=s->new_pres;
	fwcr=s->new_fwcr;
	wk2xxx_read_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_SIER ,dat);
	old_sier = dat[0];
	wk2xxx_write_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_SIER ,old_sier&(~(WK2XXX_TFTRIG_IEN | WK2XXX_RFTRIG_IEN | WK2XXX_RXOUT_IEN)));
	//local_irq_restore(flags);
	do{
		wk2xxx_read_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_FSR,dat);
	} while (dat[0] & WK2XXX_TBUSY);
	// then, disable tx and rx
	wk2xxx_read_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_SCR,dat);
	scr = dat[0];
	wk2xxx_write_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_SCR ,scr&(~(WK2XXX_RXEN|WK2XXX_TXEN)));
	// set the parity, stop bits and data size //
	wk2xxx_write_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_LCR ,lcr);
	/*set cts  and rst*/
	if(fwcr>0){  
#ifdef _DEBUG_WK2XXX
		printk(KERN_ALERT "-conf_wk2xxx_subport-set ctsrts--fwcr=0x%X\n",fwcr);
#endif
		wk2xxx_write_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_FWCR,fwcr);
		wk2xxx_write_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_SPAGE ,1);
		wk2xxx_write_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_FWTH,0XF0);
		wk2xxx_write_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_FWTL,0X80);
		wk2xxx_write_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_SPAGE ,0);
	}

	wk2xxx_write_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_SIER ,old_sier);
	// set the baud rate //
	wk2xxx_write_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_SPAGE ,1);
	wk2xxx_write_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_BAUD0 ,baud0_ss);
	wk2xxx_write_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_BAUD1 ,baud1_ss);
	wk2xxx_write_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_PRES ,pres_ss);
#ifdef _DEBUG_WK_FUNCTION
	wk2xxx_read_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_BAUD0,dat);
	printk(KERN_ALERT ":WK2XXX_BAUD0=0x%X\n", dat[0]);
	wk2xxx_read_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_BAUD1,dat);
	printk(KERN_ALERT ":WK2XXX_BAUD1=0x%X\n", dat[0]);
	wk2xxx_read_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_PRES,dat);
	printk(KERN_ALERT ":WK2XXX_PRES=0x%X\n", dat[0]);
#endif
	wk2xxx_write_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_SPAGE ,0);
	wk2xxx_write_slave_reg(s->wk2xxx_i2c_client,s->port.iobase,WK2XXX_SCR ,scr|(WK2XXX_RXEN|WK2XXX_TXEN));


#ifdef _DEBUG_WK_FUNCTION
	printk(KERN_ALERT "%s!!-port:%ld;--exit--\n", __func__,s->port.iobase);
#endif

}


// change speed
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
static void wk2xxx_termios( struct uart_port *port, struct ktermios *termios, const struct ktermios *old)
#else
static void wk2xxx_termios( struct uart_port *port, struct ktermios *termios, struct ktermios *old)
#endif
{

	struct wk2xxx_port *s = container_of(port,struct wk2xxx_port,port);
	int baud = 0;
	uint32_t temp,freq;
	uint8_t lcr=0,fwcr,baud1,baud0,pres;
	unsigned short cflag;
	unsigned short lflag;
#ifdef _DEBUG_WK_FUNCTION
	printk(KERN_ALERT "%s!!-port:%ld;--in--\n", __func__,s->port.iobase);
#endif

	cflag = termios->c_cflag;
	lflag = termios->c_lflag;
#ifdef _DEBUG_WK_VALUE
	printk(KERN_ALERT "cflag := 0x%X  lflag : = 0x%X\n",cflag,lflag);
#endif
	baud1=0;
	baud0=0;
	pres=0;
	baud = tty_termios_baud_rate(termios);
#if 1
	freq=s->port.uartclk;
	if(freq>=(baud*16))
	{
		temp=(freq)/(baud*16);
		temp=temp-1;
		baud1=(uint8_t)((temp>>8)&0xff);
		baud0=(uint8_t)(temp&0xff);
		temp=(((freq%(baud*16))*100)/(baud));
		pres=(temp+100/2)/100;
		printk(KERN_ALERT "baudrate---freq:%d,baudrate:%d\n",freq,baud);
		printk(KERN_ALERT "baudrate---baud1:%x,baud0:%x,pres:%x\n",baud1,baud0,pres);
	}
	else
	{
		printk(KERN_ALERT "the baud rate:%d is too high！ \n",baud);
	}
#else
	switch (baud) {
		case 600:
			baud1=0x4;
			baud0=0x7f;
			pres=0;
			break;
		case 1200:
			baud1=0x2;
			baud0=0x3F;
			pres=0;
			break;
		case 2400:
			baud1=0x1;
			baud0=0x1f;
			pres=0;
			break;
		case 4800:
			baud1=0x00;
			baud0=0x8f;
			pres=0;
			break;
		case 9600:
			baud1=0x00;
			baud0=0x47;
			pres=0;
			break;
		case 19200:
			baud1=0x00;
			baud0=0x23;
			pres=0;
			break;
		case 38400:
			baud1=0x00;
			baud0=0x11;
			pres=0;
			break;
		case 76800:
			baud1=0x00;
			baud0=0x08;
			pres=0;
			break;  
		case 1800:
			baud1=0x01;
			baud0=0x7f;
			pres=0;
			break;
		case 3600:
			baud1=0x00;
			baud0=0xbf;
			pres=0;
			break;
		case 7200:
			baud1=0x00;
			baud0=0x5f;
			pres=0;
			break;
		case 14400:
			baud1=0x00;
			baud0=0x2f;
			pres=0;
			break;
		case 28800:
			baud1=0x00;
			baud0=0x17;
			pres=0;
			break;
		case 57600:
			baud1=0x00;
			baud0=0x0b;
			pres=0;
			break;
		case 115200:
			baud1=0x00;
			baud0=0x05;
			pres=0;
			break;
		case 230400:
			baud1=0x00;
			baud0=0x02;
			pres=0;
			break;
		default:  
			baud1=0x00;
			baud0=0x00;
			pres=0;
			break;
	}
#endif

	tty_termios_encode_baud_rate(termios, baud, baud);

	/* we are sending char from a workqueue so enable */


#ifdef _DEBUG_WK_VALUE
	printk(KERN_ALERT "wk2xxx_termios()----port:%lx--lcr:0x%x- cflag:0x%x-CSTOPB:0x%x,PARENB:0x%x,PARODD:0x%x--\n",s->port.iobase,lcr,cflag,CSTOPB,PARENB,PARODD);
#endif

	lcr =0;
	if (cflag & CSTOPB)
		lcr|=WK2XXX_STPL;//two  stop_bits
	else
		lcr&=~WK2XXX_STPL;//one  stop_bits

	if (cflag & PARENB) {
		lcr|=WK2XXX_PAEN;//enbale spa
		if (!(cflag & PARODD)){
			lcr |= WK2XXX_PAM1;
			lcr &= ~WK2XXX_PAM0;
		}
		else{
			lcr |= WK2XXX_PAM0;//PAM0=1
			lcr &= ~WK2XXX_PAM1;//PAM1=0
		}
	}
	else{
		lcr&=~WK2XXX_PAEN;
	}

	/*set rts and cts*/
	fwcr=(termios->c_cflag&CRTSCTS)?0X30:0;


#ifdef _DEBUG_WK_VALUE
	printk(KERN_ALERT "wk2xxx_termios()----port:%ld--lcr:0x%x- cflag:0x%x-CSTOPB:0x%x,PARENB:0x%x,PARODD:0x%x--\n",s->port.iobase,lcr,cflag,CSTOPB,PARENB,PARODD);
#endif

	s->new_baud1=baud1;
	s->new_baud0=baud0;	
	s->new_pres=pres;
	s->new_lcr = lcr;
	s->new_fwcr = fwcr;

#ifdef _DEBUG_WK_VALUE
	printk(KERN_ALERT "wk2xxx_termios()----port:%ld,--NEW_FWCR:%x-\n",s->port.iobase,s->new_fwcr);
#endif

	conf_wk2xxx_subport(&s->port);
#ifdef _DEBUG_WK_FUNCTION
	printk(KERN_ALERT "%s!!-port:%ld;--exit--\n", __func__,s->port.iobase);
#endif
}


static const char *wk2xxx_type(struct uart_port *port)
{


#ifdef _DEBUG_WK_FUNCTION
	printk(KERN_ALERT "%s!!---in--\n", __func__);
#endif
	return port->type == PORT_WK2XXX ? "wk2xxx" : NULL;//this is defined in serial_core.h
}


static void wk2xxx_release_port(struct uart_port *port)
{
#ifdef _DEBUG_WK_FUNCTION
	printk(KERN_ALERT "%s!!---in--\n", __func__);
#endif

}


static int wk2xxx_request_port(struct uart_port *port)//no such memory region needed for wk2xxx
{
#ifdef _DEBUG_WK_FUNCTION
	printk(KERN_ALERT "%s!!---in--\n", __func__);
#endif
	return 0;
}


static void wk2xxx_config_port(struct uart_port *port, int flags)
{
	struct wk2xxx_port *s = container_of(port,struct wk2xxx_port,port);

#ifdef _DEBUG_WK_FUNCTION
	printk(KERN_ALERT "%s!!\n", __func__);
#endif

	if (flags & UART_CONFIG_TYPE && wk2xxx_request_port(port) == 0)
		s->port.type = PORT_WK2XXX;
}


static int wk2xxx_verify_port(struct uart_port *port, struct serial_struct *ser)
{

	int ret = 0;
#ifdef _DEBUG_WK_FUNCTION
	printk(KERN_ALERT "%s!!---in--\n", __func__);
#endif

	if (ser->type != PORT_UNKNOWN && ser->type != PORT_WK2XXX)
		ret = -EINVAL;
	if (port->irq != ser->irq)
		ret = -EINVAL;
	if (ser->io_type != SERIAL_IO_PORT)
		ret = -EINVAL;
	//if (port->uartclk / 16 != ser->baud_base)
	//      ret = -EINVAL;
	if (port->iobase != ser->port)
		ret = -EINVAL;
	if (ser->hub6 != 0)
		ret = -EINVAL;
	return ret;
}




static struct uart_ops wk2xxx_pops = {
	tx_empty:       wk2xxx_tx_empty,
	set_mctrl:      wk2xxx_set_mctrl,
	get_mctrl:      wk2xxx_get_mctrl,
	stop_tx:        wk2xxx_stop_tx,
	start_tx:       wk2xxx_start_tx,
	stop_rx:        wk2xxx_stop_rx,
	enable_ms:      wk2xxx_enable_ms,
	break_ctl:      wk2xxx_break_ctl,
	startup:        wk2xxx_startup,
	shutdown:       wk2xxx_shutdown,
	set_termios:    wk2xxx_termios,
	type:           wk2xxx_type,
	release_port:   wk2xxx_release_port,
	request_port:   wk2xxx_request_port,
	config_port:    wk2xxx_config_port,
	verify_port:    wk2xxx_verify_port,
};
static struct uart_driver wk2xxx_uart_driver = {
	owner:                  THIS_MODULE,
	major:               	SERIAL_WK2XXX_MAJOR,
#ifdef CONFIG_DEVFS_FS
	driver_name:            "ttySWK",
	dev_name:               "ttySWK",
#else
	driver_name:            "ttySWK",
	dev_name:               "ttySWK",
#endif
	minor:                  MINOR_START,
	nr:                     NR_PORTS,
	cons:                   NULL
};

static int uart_driver_registered;


/*
 *解析设备\E6\A0?
 */
//#ifdef CONFIG_OF
static int rockchip_i2c_parse_dt(struct device *dev)
{

	int irq_gpio, irq_flags, irq;
#ifdef _DEBUG_WK_FUNCTION
	printk(KERN_ALERT "%s!!--in--\n", __func__);
#endif
	//从设备树获取IRQ——GPIO
	irq_gpio = of_get_named_gpio_flags(dev->of_node, "irq_gpio", 0,(enum of_gpio_flags *)&irq_flags);
	if (!gpio_is_valid(irq_gpio))
	{
		printk(KERN_ERR"invalid wk2xxx_irq_gpio: %d\n", irq_gpio);
		return -1;
	}

	irq = gpio_to_irq(irq_gpio);
#if 0
	if(!irq)
	{
		printk(KERN_ERR"wk2xxx_irqGPIO: %d get irq failed!\n", irq);
		//gpio_free(s->irq_gpio);
		return -1;
	}
#else
	if(irq)
	{

		if (gpio_request(irq_gpio , "irq-gpio"))
		{
			printk(KERN_ERR"gpio_request failed!! irq_gpio: %d!\n", irq);
			gpio_free(irq_gpio);
			return  IRQ_NONE;
		}
	}
	else
	{
		printk(KERN_ERR"gpio_to_irq failed! irq: %d !\n", irq);
		return -ENODEV;
	}
#endif
	printk(KERN_ERR"wk2xxx_irq_gpio: %d, irq: %d", irq_gpio, irq);
#ifdef _DEBUG_WK_FUNCTION
	printk(KERN_ALERT "%s!!--exit--\n", __func__);
#endif
	return irq;
}

#ifdef WK_RSTGPIO_FUNCTION
static int wk2xxx_i2c_rstgpio_parse_dt(struct device *dev,int *rst_gpio)
{

	enum of_gpio_flags rst_flags; 
#ifdef _DEBUG_WK_FUNCTION
	printk(KERN_ALERT "%s!!--in--\n", __func__);
#endif
	*rst_gpio = of_get_named_gpio_flags(dev->of_node, "reset_gpio", 0,&rst_flags);
	if (!gpio_is_valid(*rst_gpio)){
		printk(KERN_ERR"invalid wk2xxx_rst_gpio: %d\n", *rst_gpio);
		return -1;
	}

	if(	*rst_gpio){
		if (gpio_request(*rst_gpio , "rst_gpio")){
			printk(KERN_ERR"gpio_request failed!! rst_gpio: %d!\n",*rst_gpio);
			gpio_free(*rst_gpio);
			return  IRQ_NONE;
		}
	}
	gpio_direction_output(*rst_gpio,1);// output high
	printk(KERN_ERR"wk2xxx_rst_gpio: %d", *rst_gpio);
#ifdef _DEBUG_WK_FUNCTION
	printk(KERN_ALERT "%s!!--exit--\n", __func__);
#endif
	return 0;
}

#endif


static int wk2xxx_probe(struct i2c_client *client,const struct i2c_device_id *dev_id)
{
	uint8_t i;
	int status, irq, ret;
	uint8_t dat[1];
	const struct wk2xxx_quirks *quirks;
	struct wk2xxx_priv_data *priv;
#ifdef _DEBUG_WK_FUNCTION
	printk(KERN_ALERT "%s!!--in--\n", __func__);
#endif

	/* Alloc port structure */
	priv = devm_kzalloc(&client->dev, sizeof(*priv),GFP_KERNEL);
	if (!priv) {
	    printk(KERN_ALERT "wk2xxx_probe(devm_kzalloc) fail.\n");
		return -ENOMEM;
	}
	dev_set_drvdata(&client->dev, priv);

	if (client->irq <= 0) {
		irq = rockchip_i2c_parse_dt(&client->dev);
		if(irq<0)
			return -EINVAL;
	} else {
		irq = client->irq;
		printk(KERN_ALERT "wk2xxx_probe(client->irq)  irq = 0x%d\n",irq);
	}

#ifdef WK_RSTGPIO_FUNCTION
	//Obtain the GPIO number of RST signal
	ret=wk2xxx_i2c_rstgpio_parse_dt(&client->dev,&priv->rst_gpio);
	if(ret!=0){
		printk(KERN_ALERT "wk2xxx_probe(rst_gpio)  rst_gpio= 0x%d\n",priv->rst_gpio);
		ret=priv->rst_gpio;
		return 1;
	}
	/*reset wk2xxx*/
	mdelay(10);
	gpio_set_value(priv->rst_gpio, 0);
	mdelay(10);
	gpio_set_value(priv->rst_gpio, 1);
	mdelay(10);
#endif

	do
	{
		wk2xxx_read_global_reg(client,WK2XXX_GENA,dat);
		wk2xxx_read_global_reg(client,WK2XXX_GENA,dat);
		printk(KERN_ERR "wk2xxx_probe(0xB0)  GENA = 0x%X\n",dat[0]);//GENA=0XB0
		wk2xxx_write_global_reg(client,WK2XXX_GENA,0xf5);
		wk2xxx_read_global_reg(client,WK2XXX_GENA,dat);
		printk(KERN_ERR "wk2xxx_probe(0xB5)  GENA = 0x%X\n",dat[0]);//GENA=0XB5
		wk2xxx_write_global_reg(client,WK2XXX_GENA,0xf0);
		wk2xxx_read_global_reg(client,WK2XXX_GENA,dat);
		printk(KERN_ERR "wk2xxx_probe(0xBF)  GENA = 0x%X\n",dat[0]);//GENA=0XBF
        wk2xxx_write_global_reg(client,WK2XXX_GENA,0xB0);
	}while(0);
	/////////////////////test i2c//////////////////////////
	wk2xxx_write_global_reg(client,WK2XXX_GENA,0x0);
	wk2xxx_read_global_reg(client,WK2XXX_GENA,dat);
	if((dat[0]&0xf0)!=0xb0)
	{ 
		printk(KERN_ALERT "wk2xxx_probe()  GENA = 0x%X\n",dat[0]);
		printk(KERN_ERR "i2c driver error!!!!\n");
		return 1;
	}
	mutex_lock(&wk2xxxs_lock);
	if(!uart_driver_registered)
	{
		uart_driver_registered = 1;
		status = uart_register_driver(&wk2xxx_uart_driver);
		if (status)
		{
			printk(KERN_ERR "Couldn't register wk2xxx uart driver\n");
			mutex_unlock(&wk2xxxs_lock);
			return status;
		}
	}
	printk(KERN_ALERT "wk2xxx_serial_init.\n");

	quirks = device_get_match_data(&client->dev);
	if (!quirks) {
		dev_err(&client->dev, "Failed to determine the quirks to use\n");
		return -ENODEV;
	}

	for(i =0;i<NR_PORTS;i++)
	{
		struct wk2xxx_port *s = &wk2xxxs[i];//container_of(port,struct wk2xxx_port,port);
		s->tx_done       = 0;
		s->wk2xxx_i2c_client        = client;
		s->port.line     = i;
		s->port.dev      = &client->dev;
		s->port.ops      = &wk2xxx_pops;
		s->port.uartclk  = WK_CRASTAL_CLK;
		s->port.fifosize = 256;
		s->port.iobase   = i+1;
		s->port.irq      = irq;
		s->port.iotype   = SERIAL_IO_PORT;
		s->port.flags    = UPF_BOOT_AUTOCONF;
		status = uart_add_one_port(&wk2xxx_uart_driver, &s->port);
		if(status<0)
		{
			printk(KERN_ALERT "uart_add_one_port failed for line i:= %d with error %d\n",i,status);
			mutex_unlock(&wk2xxxs_lock);
			return status;
		}
	}
	printk(KERN_ALERT "uart_add_one_port status= 0x%d\n",status);
	mutex_unlock(&wk2xxxs_lock);

	return status;
}


static int wk2xxx_remove(struct i2c_client *i2c_client)
{

	int i;
	const struct wk2xxx_quirks *quirks;
	struct wk2xxx_priv_data *priv = dev_get_drvdata(&i2c_client->dev);
#ifdef _DEBUG_WK_FUNCTION
	printk(KERN_ALERT "%s!!--in--\n", __func__);
#endif

	mutex_lock(&wk2xxxs_lock);
	quirks = device_get_match_data(&i2c_client->dev);

	for(i = 0; i < quirks->nr_ports; i++)
	{
		struct wk2xxx_port *s = &wk2xxxs[i];
		uart_remove_one_port(&wk2xxx_uart_driver, &s->port);
	}
	printk( KERN_ERR"removing wk2xxx driver\n");
	uart_unregister_driver(&wk2xxx_uart_driver);
	mutex_unlock(&wk2xxxs_lock);

	if (priv->rst_gpio > 0 )
	{
		gpio_free(priv->rst_gpio);
	}
	devm_kfree(&i2c_client->dev,priv);
#ifdef _DEBUG_WK_FUNCTION
	printk(KERN_ALERT "%s!!--exit--\n", __func__);
#endif
	return 0;
}
/*
static int wk2xxx_resume(struct i2c_client *i2c_client)
{
#ifdef _DEBUG_WK_FUNCTION
	printk(KERN_ALERT "%s!!--in--\n", __func__);
#endif
	return 0;
}
*/

static const struct wk2xxx_quirks wk2xxx_wk2124_quirks = {
	.nr_ports = 4
};

static const struct wk2xxx_quirks wk2xxx_wk2132_quirks = {
	.nr_ports = 2
};

static const struct i2c_device_id wk2xxx_i2c_id_table[]={
	{"wk2xxx_i2c",0},
	{"wk2124_i2c",0},
	{"wk2132_i2c",0},
	{}
};

static struct of_device_id wk2xxx_i2c_dt_ids[] = {
	{ .compatible = "wkmic,wk2xxx_i2c", .data = &wk2xxx_wk2124_quirks },
	{ .compatible = "wkmic,wk2124_i2c", .data = &wk2xxx_wk2124_quirks },
	{ .compatible = "wkmic,wk2132_i2c", .data = &wk2xxx_wk2132_quirks },
	{}
};

MODULE_DEVICE_TABLE(of, wk2xxx_i2c_dt_ids);

static struct i2c_driver wk2xxx_i2c_driver = {
	.driver = {
		.name       = "wk2xxx_i2c",
		.owner      = THIS_MODULE,
		.of_match_table = of_match_ptr(wk2xxx_i2c_dt_ids),
	},

	.probe          = wk2xxx_probe,
	.remove         = wk2xxx_remove,
	//.resume         = wk2xxx_resume,
	.id_table       = wk2xxx_i2c_id_table,
};
//module_i2c_driver(wk2xxx_i2c_driver);
static int __init wk2xxx_init(void)
{
	int retval;
	retval = i2c_add_driver(&wk2xxx_i2c_driver);
	printk(KERN_ALERT "rgister i2c_driver return v = :%d\n",retval);
	return retval;
}



static void __exit wk2xxx_exit(void)
{
	printk(KERN_ERR "%s, wk2xxx_i2c_driver:quit\n", __func__);
	return i2c_del_driver(&wk2xxx_i2c_driver);
}


module_init(wk2xxx_init);
module_exit(wk2xxx_exit);

MODULE_AUTHOR("WKMIC Ltd");
MODULE_DESCRIPTION("wk2xxx generic serial port driver");
MODULE_LICENSE("GPL");






