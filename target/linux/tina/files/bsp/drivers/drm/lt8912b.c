/*
 * Copyright (c) 2025 HZHY Technologies Co., Ltd. All rights reserved.
 * Copyright (c) 2025 ChenXiaodong <chenxd@hzhytech.com>.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY,OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 */

#include <linux/acpi.h>
#include <linux/bitmap.h>
#include <linux/gpio/driver.h>
#include <linux/gpio/consumer.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/platform_data/pca953x.h>
#include <linux/regmap.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>

#include <asm/unaligned.h>

enum {
	ADDR_48 = 0,
	ADDR_49,
	ADDR_4a,
	ADDR_MAX,
};

enum {
	MODE_1080P = 0,
	MODE_720P,
	MODE_480P,
};

struct lt8912_client {
	struct regmap *regmap;
	struct i2c_client *client;
};

struct lt8912_dev {
	struct lt8912_client *clients[ADDR_MAX];
	struct gpio_desc *reset_gpio;
	struct mutex mutex;
	unsigned char Hsync_L_last;
	unsigned char Hsync_H_last;
	unsigned char Vsync_L_last;
	unsigned char Vsync_H_last;
	int suspend;
	int mode;
	int opened;
};

struct reg_key_value {
	struct lt8912_client *cli;
	u32 reg;
	u32 val;
};

struct video_timing {
	unsigned short hfp;
	unsigned short hs;
	unsigned short hbp;
	unsigned short hact;
	unsigned short htotal;
	unsigned short vfp;
	unsigned short vs;
	unsigned short vbp;
	unsigned short vact;
	unsigned short vtotal;
	unsigned int pclk_khz;
};

static struct lt8912_dev lt8912_device;

static struct video_timing video_720x480_60Hz = {16, 62, 60, 720, 858, 9, 6, 30, 480, 525};
static struct video_timing video_1280x720_60Hz = {110, 40, 220, 1280, 1650, 5, 5, 20, 720, 750};
static struct video_timing video_1920x1080_60Hz = {88, 44, 148, 1920, 2200, 4, 5, 36, 1080, 1125};
static struct video_timing video_1280x800_60Hz = {64, 136, 200, 1280, 1680, 1, 3, 24, 800, 828, 74250};
static struct video_timing video_800x1280_60Hz = {80, 20, 20, 800, 920, 15, 6, 8, 1280, 1309, 67200};
static struct video_timing video_1024x600_60Hz = {50,20,50,1024,1144,9,3,13,600,625,42500};
static struct video_timing video_lvds_60Hz = {24, 136, 160, 1024, 1344, 3, 6, 29, 768, 806, 65000};

static int lt8912_reg_write(struct reg_key_value *reg)
{
	return regmap_write(reg->cli->regmap, reg->reg, reg->val);
}

static int lt8912_clk_enable(struct lt8912_dev *ltdev)
{
	int i;
	int ret;
	struct reg_key_value regs[] = {
		{ ltdev->clients[ADDR_48], 0x02, 0xf7 },
		{ ltdev->clients[ADDR_48], 0x08, 0xff },
		{ ltdev->clients[ADDR_48], 0x09, 0xff },
		{ ltdev->clients[ADDR_48], 0x0a, 0xff },
		{ ltdev->clients[ADDR_48], 0x0b, 0x7c },
		{ ltdev->clients[ADDR_48], 0x0c, 0xff }
	};

	for (i = 0; i < ARRAY_SIZE(regs); i++) {
		ret = lt8912_reg_write(&regs[i]);
		if (ret)
			return ret;
	}

	return 0;
}

static int lt8912_tx_config(struct lt8912_dev *ltdev)
{
	int i;
	int ret;
	struct reg_key_value regs[] = {
		{ ltdev->clients[ADDR_48], 0x31, 0xE1 },
		{ ltdev->clients[ADDR_48], 0x32, 0xE1 },
		{ ltdev->clients[ADDR_48], 0x33, 0x0c },
		{ ltdev->clients[ADDR_48], 0x37, 0x00 },
		{ ltdev->clients[ADDR_48], 0x38, 0x22 },
		{ ltdev->clients[ADDR_48], 0x60, 0x82 }
	};

	for (i = 0; i < ARRAY_SIZE(regs); i++) {
		ret = lt8912_reg_write(&regs[i]);
		if (ret)
			return ret;
	}

	return 0;
}

static int lt8912_cbus_config(struct lt8912_dev *ltdev)
{
	int i;
	int ret;
	struct reg_key_value regs[] = {
		{ ltdev->clients[ADDR_48], 0x39, 0x45 },
		{ ltdev->clients[ADDR_48], 0x3a, 0x00 },
		{ ltdev->clients[ADDR_48], 0x3b, 0x00 },
	};

	for (i = 0; i < ARRAY_SIZE(regs); i++) {
		ret = lt8912_reg_write(&regs[i]);
		if (ret)
			return ret;
	}

	return 0;
}

static int lt8912_pll_config(struct lt8912_dev *ltdev)
{
	int i;
	int ret;
	struct reg_key_value regs[] = {
		{ ltdev->clients[ADDR_48], 0x44, 0x31 },
		{ ltdev->clients[ADDR_48], 0x55, 0x44 },
		{ ltdev->clients[ADDR_48], 0x57, 0x01 },
		{ ltdev->clients[ADDR_48], 0x5a, 0x02 },
	};

	for (i = 0; i < ARRAY_SIZE(regs); i++) {
		ret = lt8912_reg_write(&regs[i]);
		if (ret)
			return ret;
	}

	return 0;
}

static int lt8912_audio_config(struct lt8912_dev *ltdev)
{
	int i;
	int ret;
	struct reg_key_value regs[] = {
		{ ltdev->clients[ADDR_48], 0xB2, 0x01 },
		{ ltdev->clients[ADDR_4a], 0x06, 0x08 },
		{ ltdev->clients[ADDR_4a], 0x07, 0xF0 },
		{ ltdev->clients[ADDR_4a], 0x34, 0xD2 },
	};

	for (i = 0; i < ARRAY_SIZE(regs); i++) {
		ret = lt8912_reg_write(&regs[i]);
		if (ret)
			return ret;
	}

	return 0;
}

static int lt8912_avi_config(struct lt8912_dev *ltdev)
{
	int i;
	int ret;
	struct reg_key_value regs[7] = {
		{ ltdev->clients[ADDR_4a], 0x3C, 0x41 },
		{ ltdev->clients[ADDR_48], 0xAB, 0x03 },
		{ ltdev->clients[ADDR_4a], 0x43, 0x27 },
		{ ltdev->clients[ADDR_4a], 0x44, 0x10 },
		{ ltdev->clients[ADDR_4a], 0x45, 0x28 },
		{ ltdev->clients[ADDR_4a], 0x46, 0x00 },
		{ ltdev->clients[ADDR_4a], 0x47, 0x10 },
	};

	for (i = 0; i < ARRAY_SIZE(regs); i++) {
		ret = lt8912_reg_write(&regs[i]);
		if (ret)
			return ret;
	}

	switch (ltdev->mode) {
	case MODE_1080P:
		regs[1] = (struct reg_key_value) { .cli = ltdev->clients[ADDR_48], .reg = 0xAB, .val = 0x03 };
		regs[2] = (struct reg_key_value) { .cli = ltdev->clients[ADDR_4a], .reg = 0x43, .val = 0x27 };
		regs[3] = (struct reg_key_value) { .cli = ltdev->clients[ADDR_4a], .reg = 0x44, .val = 0x10 };
		regs[4] = (struct reg_key_value) { .cli = ltdev->clients[ADDR_4a], .reg = 0x45, .val = 0x28 };
		regs[5] = (struct reg_key_value) { .cli = ltdev->clients[ADDR_4a], .reg = 0x46, .val = 0x00 };
		regs[6] = (struct reg_key_value) { .cli = ltdev->clients[ADDR_4a], .reg = 0x47, .val = 0x10 };
		break;
	case MODE_720P:
		regs[1] = (struct reg_key_value) { .cli = ltdev->clients[ADDR_48], .reg = 0xAB, .val = 0x03 };
		regs[2] = (struct reg_key_value) { .cli = ltdev->clients[ADDR_4a], .reg = 0x43, .val = 0x33 };
		regs[3] = (struct reg_key_value) { .cli = ltdev->clients[ADDR_4a], .reg = 0x44, .val = 0x10 };
		regs[4] = (struct reg_key_value) { .cli = ltdev->clients[ADDR_4a], .reg = 0x45, .val = 0x28 };
		regs[5] = (struct reg_key_value) { .cli = ltdev->clients[ADDR_4a], .reg = 0x46, .val = 0x00 };
		regs[6] = (struct reg_key_value) { .cli = ltdev->clients[ADDR_4a], .reg = 0x47, .val = 0x04 };
		break;
	case MODE_480P:
		regs[1] = (struct reg_key_value) { .cli = ltdev->clients[ADDR_48], .reg = 0xAB, .val = 0x0c };
		regs[2] = (struct reg_key_value) { .cli = ltdev->clients[ADDR_4a], .reg = 0x43, .val = 0x45 };
		regs[3] = (struct reg_key_value) { .cli = ltdev->clients[ADDR_4a], .reg = 0x44, .val = 0x10 };
		regs[4] = (struct reg_key_value) { .cli = ltdev->clients[ADDR_4a], .reg = 0x45, .val = 0x18 };
		regs[5] = (struct reg_key_value) { .cli = ltdev->clients[ADDR_4a], .reg = 0x46, .val = 0x00 };
		regs[6] = (struct reg_key_value) { .cli = ltdev->clients[ADDR_4a], .reg = 0x47, .val = 0x02 };
		break;
	default:
		pr_warn("Unrecognized mode %d\n", ltdev->mode);
		break;
	}

	for (i = 1; i < ARRAY_SIZE(regs); i++) {
		ret = lt8912_reg_write(&regs[i]);
		if (ret)
			return ret;
	}

	return 0;
}

static int lt8912_mipi_config(struct lt8912_dev *ltdev)
{
	int i;
	int ret;
	struct reg_key_value regs[] = {
		{ ltdev->clients[ADDR_48], 0x3e, 0xd6 },
		{ ltdev->clients[ADDR_48], 0x3f, 0xd4 },
		{ ltdev->clients[ADDR_48], 0x41, 0x3c },
	};

	for (i = 0; i < ARRAY_SIZE(regs); i++) {
		ret = lt8912_reg_write(&regs[i]);
		if (ret)
			return ret;
	}

	return 0;
}

static int lt8912_rx_reset(struct lt8912_dev *ltdev)
{
	int ret;
	struct reg_key_value regs[] = {
		{ ltdev->clients[ADDR_48], 0x03, 0x7f },
		{ ltdev->clients[ADDR_48], 0x03, 0xff },
		{ ltdev->clients[ADDR_48], 0x05, 0xfb },
		{ ltdev->clients[ADDR_48], 0x05, 0xff },
	};
 
	ret = lt8912_reg_write(&regs[0]);
	if (ret)
		return ret;
	mdelay(20);
	ret = lt8912_reg_write(&regs[1]);
	if (ret)
		return ret;

	ret = lt8912_reg_write(&regs[2]);
	if (ret)
		return ret;
	mdelay(20);
	ret = lt8912_reg_write(&regs[3]);
	if (ret)
		return ret;

	return 0;
}

static int lt8912_mipi_basic_config(struct lt8912_dev *ltdev)
{
	int i;
	int ret;
	struct reg_key_value regs[] = {
		{ ltdev->clients[ADDR_49], 0x10, 0x01 },
		{ ltdev->clients[ADDR_49], 0x11, 0x08 },
		{ ltdev->clients[ADDR_49], 0x13, 0x00 }, //00-4lane 01-1lane 02-2lane 03-3lane 
		{ ltdev->clients[ADDR_49], 0x14, 0x00 },
		{ ltdev->clients[ADDR_49], 0x15, 0x00 },
		{ ltdev->clients[ADDR_49], 0x1a, 0x03 },
		{ ltdev->clients[ADDR_49], 0x1b, 0x03 },
	};

	if (ltdev->mode != MODE_1080P)
		regs[1].val = 0x05;

	for (i = 0; i < ARRAY_SIZE(regs); i++) {
		ret = lt8912_reg_write(&regs[i]);
		if (ret)
			return ret;
	}

	return 0;
}

static int lt8912_start(struct lt8912_dev *ltdev, int on)
{
	struct reg_key_value regs[] = {
		{ ltdev->clients[ADDR_48], 0x33, 0x0e },
		{ ltdev->clients[ADDR_48], 0x33, 0x0c },
	};

	if (on)
		return lt8912_reg_write(&regs[0]);
	else
		return lt8912_reg_write(&regs[1]);
}

static int lt8912_suspend(struct lt8912_dev *ltdev, int on)
{
	int i;
	int ret = 0;
	struct reg_key_value suspend_regs[] = {
		{ ltdev->clients[ADDR_48], 0x54, 0x1d },
		{ ltdev->clients[ADDR_48], 0x51, 0x15 },
		{ ltdev->clients[ADDR_48], 0x44, 0x31 },
		{ ltdev->clients[ADDR_48], 0x41, 0xbd },
		{ ltdev->clients[ADDR_48], 0x5c, 0x11 },
	};
	struct reg_key_value resume_regs[] = {
		{ ltdev->clients[ADDR_48], 0x5c, 0x10 },
		{ ltdev->clients[ADDR_48], 0x54, 0x1c },
		{ ltdev->clients[ADDR_48], 0x51, 0x2d },
		{ ltdev->clients[ADDR_48], 0x44, 0x30 },
		{ ltdev->clients[ADDR_48], 0x41, 0xbc },
	}; 

	mutex_lock(&ltdev->mutex);
	if (on && !(ltdev->suspend)) {
		for (i = 0; i < ARRAY_SIZE(suspend_regs); i++) {
			ret = lt8912_reg_write(&suspend_regs[i]);
			if (ret)
				goto out;
			ltdev->suspend = 1;
		}
	} else if (!on && ltdev->suspend) {
		for (i = 0; i < ARRAY_SIZE(resume_regs); i++) {
			ret = lt8912_reg_write(&resume_regs[i]);
			if (ret)
				goto out;
			ltdev->suspend = 0;
		}
		mdelay(10);
		ret = lt8912_rx_reset(ltdev);
	}
out:
	mutex_unlock(&ltdev->mutex);

	return ret;
}

static int lt8912_video_format_config(struct lt8912_dev *ltdev, struct video_timing *video_format)
{
	int i;
	int ret;

	struct reg_key_value regs[] = {
		{ ltdev->clients[ADDR_49], 0x18, (unsigned char)(video_format->hs%256) }, /* hwidth */
		{ ltdev->clients[ADDR_49], 0x19, (unsigned char)(video_format->vs%256) }, /* vwidth 6 */
		{ ltdev->clients[ADDR_49], 0x1c, (unsigned char)(video_format->hact%256) }, /* H_active[7:0] */
		{ ltdev->clients[ADDR_49], 0x1d, (unsigned char)(video_format->hact/256) }, /* H_active[15:8] */
		{ ltdev->clients[ADDR_49], 0x2f, 0x0c }, /* fifo_buff_length 12 */
		{ ltdev->clients[ADDR_49], 0x34, (unsigned char)(video_format->htotal%256) }, /* H_total[7:0] */
		{ ltdev->clients[ADDR_49], 0x35, (unsigned char)(video_format->htotal/256) }, /* H_total[15:8] */
		{ ltdev->clients[ADDR_49], 0x36, (unsigned char)(video_format->vtotal%256) }, /* V_total[7:0] */
		{ ltdev->clients[ADDR_49], 0x37, (unsigned char)(video_format->vtotal/256) }, /* V_total[15:8] */
		{ ltdev->clients[ADDR_49], 0x38, (unsigned char)(video_format->vbp%256) }, /* VBP[7:0] */
		{ ltdev->clients[ADDR_49], 0x39, (unsigned char)(video_format->vbp/256) }, /* VBP[15:8] */
		{ ltdev->clients[ADDR_49], 0x3a, (unsigned char)(video_format->vfp%256) }, /* VFP[7:0] */
		{ ltdev->clients[ADDR_49], 0x3b, (unsigned char)(video_format->vfp/256) }, /* VFP[15:8] */
		{ ltdev->clients[ADDR_49], 0x3c, (unsigned char)(video_format->hbp%256) }, /* HBP[7:0] */
		{ ltdev->clients[ADDR_49], 0x3d, (unsigned char)(video_format->hbp/256) }, /* HBP[15:8] */
		{ ltdev->clients[ADDR_49], 0x3e, (unsigned char)(video_format->hfp%256) }, /* HFP[7:0] */
		{ ltdev->clients[ADDR_49], 0x3f, (unsigned char)(video_format->hfp/256) }, /* HFP[15:8] */
	};

	for (i = 0; i < ARRAY_SIZE(regs); i++) {
		ret = lt8912_reg_write(&regs[i]);
		if (ret)
			return ret;
	}

	return 0;
}

static int lt8912_video_det(struct lt8912_dev *ltdev, struct video_timing *default_format)
{
	unsigned char Hsync_L, Hsync_H, Vsync_L, Vsync_H;
	struct lt8912_client *cli = ltdev->clients[ADDR_48];
	struct video_timing *timing = default_format;
	int ret;

	ret = regmap_read(cli->regmap, 0x9c, (u32 *)&Hsync_L);
	if (ret)
		return ret;
	ret = regmap_read(cli->regmap, 0x9d, (u32 *)&Hsync_H);
	if (ret)
		return ret;
	ret = regmap_read(cli->regmap, 0x9e, (u32 *)&Vsync_L);
	if (ret)
		return ret;
	ret = regmap_read(cli->regmap, 0x9f, (u32 *)&Vsync_H);
	if (ret)
		return ret;

	if((Hsync_H != ltdev->Hsync_H_last) || (Vsync_H != ltdev->Vsync_H_last)) {
		pr_info("LT8912 0x9c~9f = %x, %x, %x, %x\n", Hsync_H, Hsync_L, Vsync_H, Vsync_L);

		if(Vsync_H == 0x02 && Vsync_L <= 0x0f && Vsync_L >= 0x0b) {
			timing = &video_720x480_60Hz;
			pr_info("videoformat = VESA_720x480_60\n");
		}
		else if(Vsync_H == 0x02 && Vsync_L == 0x71) {
			timing = &video_1024x600_60Hz;
			pr_info("videoformat = VESA_1024x600_60\n");
		}
		else if(Vsync_H==0x02 && Vsync_L <= 0xef && Vsync_L >= 0xec) {
			timing = &video_1280x720_60Hz;
			pr_info("nvideoformat = VESA_1280x720_60\n");
		}
		else if(Vsync_H == 0x03 && Vsync_L <= 0x3a && Vsync_L >= 0x34) {
			timing = &video_1280x800_60Hz;
			pr_info("videoformat = VESA_1280x800_60\n");
		}
		else if(Vsync_H == 0x04 && Vsync_L <= 0x67 && Vsync_L >= 0x63) {
			timing = &video_1920x1080_60Hz;
			pr_info("videoformat = VESA_1920x1080_60\n");
		}
		else if(Vsync_H == 0x03 && Vsync_L <= 0x23 && Vsync_L >= 0x1d) {
			timing = &video_lvds_60Hz;
			pr_info("videoformat = VESA_1366x768_60\n");
		}
		else if(Vsync_H == 0x1d && Vsync_L <= 0x05 && Vsync_L >= 0x1d) {
			timing = &video_800x1280_60Hz;
			pr_info("videoformat = VESA_800x1280_60\n");
		}
		else {
			timing = default_format;
		}

		ltdev->Hsync_L_last = Hsync_L;
		ltdev->Hsync_H_last = Hsync_H;
		ltdev->Vsync_L_last = Vsync_L;
		ltdev->Vsync_H_last = Vsync_H;
	}

	ret = lt8912_video_format_config(ltdev, timing);
	if (ret)
		return ret;

	return lt8912_rx_reset(ltdev);
}

static int lt8912_video_config(struct lt8912_dev *ltdev)
{
	struct video_timing *video_format;
	int ret;

	video_format = &video_1920x1080_60Hz;
	switch (ltdev->mode) {
	case MODE_1080P:
		video_format = &video_1920x1080_60Hz;
		break;
	case MODE_720P:
		video_format = &video_1280x720_60Hz;
		break;
	case MODE_480P:
		video_format = &video_720x480_60Hz;
		break;
	}

	ret = lt8912_video_format_config(ltdev, video_format);
	if (ret)
		return ret;

	return lt8912_video_det(ltdev, video_format);
}

static int lt8912_dds_config(struct lt8912_dev *ltdev)
{
	int i;
	int ret;
	struct reg_key_value regs[] = {
		{ ltdev->clients[ADDR_49], 0x4e, 0xaa },
		{ ltdev->clients[ADDR_49], 0x4f, 0xaa },
		{ ltdev->clients[ADDR_49], 0x50, 0x6a },
		{ ltdev->clients[ADDR_49], 0x51, 0x80 },
		{ ltdev->clients[ADDR_49], 0x1e, 0x4f },
		{ ltdev->clients[ADDR_49], 0x1f, 0x5e },
		{ ltdev->clients[ADDR_49], 0x20, 0x01 },
		{ ltdev->clients[ADDR_49], 0x21, 0x2c }, /* full_value1 416 */
		{ ltdev->clients[ADDR_49], 0x22, 0x01 },
		{ ltdev->clients[ADDR_49], 0x23, 0xfa }, /* full_value2 400 */
		{ ltdev->clients[ADDR_49], 0x24, 0x00 },
		{ ltdev->clients[ADDR_49], 0x25, 0xc8 }, /* full_value3 384 */
		{ ltdev->clients[ADDR_49], 0x26, 0x00 },
		{ ltdev->clients[ADDR_49], 0x27, 0x5e }, /* empty_value 464 */
		{ ltdev->clients[ADDR_49], 0x28, 0x01 },
		{ ltdev->clients[ADDR_49], 0x29, 0x2c }, /* empty_value1 416 */
		{ ltdev->clients[ADDR_49], 0x2a, 0x01 },
		{ ltdev->clients[ADDR_49], 0x2b, 0xfa }, /* empty_value2 400 */
		{ ltdev->clients[ADDR_49], 0x2c, 0x00 },
		{ ltdev->clients[ADDR_49], 0x2d, 0xc8 }, /* empty_value3 384 */
		{ ltdev->clients[ADDR_49], 0x2e, 0x00 },
		{ ltdev->clients[ADDR_49], 0x42, 0x64 }, /* tmr_set[ 7:0]:100us */
		{ ltdev->clients[ADDR_49], 0x43, 0x00 }, /* tmr_set[15:8]:100us */
		{ ltdev->clients[ADDR_49], 0x44, 0x04 }, /* timer step */
		{ ltdev->clients[ADDR_49], 0x45, 0x00 },
		{ ltdev->clients[ADDR_49], 0x46, 0x59 },
		{ ltdev->clients[ADDR_49], 0x47, 0x00 },
		{ ltdev->clients[ADDR_49], 0x48, 0xf2 },
		{ ltdev->clients[ADDR_49], 0x49, 0x06 },
		{ ltdev->clients[ADDR_49], 0x4a, 0x00 },
		{ ltdev->clients[ADDR_49], 0x4b, 0x72 },
		{ ltdev->clients[ADDR_49], 0x4c, 0x45 },
		{ ltdev->clients[ADDR_49], 0x4d, 0x00 },
		{ ltdev->clients[ADDR_49], 0x52, 0x08 }, /* trend step */
		{ ltdev->clients[ADDR_49], 0x53, 0x00 },
		{ ltdev->clients[ADDR_49], 0x54, 0xb2 },
		{ ltdev->clients[ADDR_49], 0x55, 0x00 },
		{ ltdev->clients[ADDR_49], 0x56, 0xe4 },
		{ ltdev->clients[ADDR_49], 0x57, 0x0d },
		{ ltdev->clients[ADDR_49], 0x58, 0x00 },
		{ ltdev->clients[ADDR_49], 0x59, 0xe4 },
		{ ltdev->clients[ADDR_49], 0x5a, 0x8a },
		{ ltdev->clients[ADDR_49], 0x5b, 0x00 },
		{ ltdev->clients[ADDR_49], 0x5c, 0x34 },
		{ ltdev->clients[ADDR_49], 0x51, 0x00 },
	};

	for (i = 0; i < ARRAY_SIZE(regs); i++) {
		ret = lt8912_reg_write(&regs[i]);
		if (ret)
			return ret;
	}

	return 0;
}

static void print_lt8912_chipid(struct lt8912_dev *ltdev)
{
	int ret;
	u32 reg_val1;
	u32 reg_val2;

	ret = regmap_read(ltdev->clients[ADDR_48]->regmap, 0x0, &reg_val1);
	if (ret)
		return;
	ret = regmap_read(ltdev->clients[ADDR_48]->regmap, 0x1, &reg_val2);
	if (ret)
		return;

	dev_info(&(ltdev->clients[ADDR_48]->client->dev), "lt8912b chip ID:0x%x, 0x%x\n",
			reg_val1, reg_val2);
}

typedef int (*func)(struct lt8912_dev *);
static int lt8912b_setup(struct lt8912_dev *ltdev)
{
	int i;
	int ret;

	static func funcs[] = {
		&lt8912_clk_enable, 
		&lt8912_tx_config,
		&lt8912_cbus_config,
		&lt8912_pll_config,
		&lt8912_audio_config,
		&lt8912_avi_config,
		&lt8912_clk_enable,
		&lt8912_tx_config,
		&lt8912_cbus_config,
		&lt8912_pll_config,
		&lt8912_mipi_config,
		&lt8912_mipi_basic_config,
		&lt8912_dds_config,
		&lt8912_video_config,
		&lt8912_audio_config,
		&lt8912_avi_config,
		&lt8912_rx_reset,
	};

	for (i = 0; i < ARRAY_SIZE(funcs); i++) {
		ret = funcs[i](ltdev);
		if (ret) {
			pr_err("call setup function %d failed, ret=%d\n", i, ret);
			return ret;
		}
	}

	return 0;
}

static int lt8912b_test(struct lt8912_dev *ltdev)
{
	int i;
	int ret;
	struct reg_key_value regs1[] = {
		{ ltdev->clients[ADDR_48], 0x08, 0xff },
		{ ltdev->clients[ADDR_48], 0x09, 0xff },
		{ ltdev->clients[ADDR_48], 0x0a, 0xff },
		{ ltdev->clients[ADDR_48], 0x0b, 0xff },
		{ ltdev->clients[ADDR_48], 0x0c, 0xff },
		{ ltdev->clients[ADDR_48], 0x31, 0xa1 },
		{ ltdev->clients[ADDR_48], 0x32, 0xa1 },
		{ ltdev->clients[ADDR_48], 0x33, 0x03 },
		{ ltdev->clients[ADDR_48], 0x37, 0x00 },
		{ ltdev->clients[ADDR_48], 0x38, 0x22 },
		{ ltdev->clients[ADDR_48], 0x60, 0x82 },
		{ ltdev->clients[ADDR_48], 0x39, 0x45 },
		{ ltdev->clients[ADDR_48], 0x3b, 0x00 },
		{ ltdev->clients[ADDR_48], 0x44, 0x31 },
		{ ltdev->clients[ADDR_48], 0x55, 0x44 },
		{ ltdev->clients[ADDR_48], 0x57, 0x01 },
		{ ltdev->clients[ADDR_48], 0x5a, 0x02 },
	};

	struct reg_key_value regs2[] = {
		{ ltdev->clients[ADDR_49], 0x10, 0x00 }, /* term en  To analog phy for trans lp mode to hs mode */
		{ ltdev->clients[ADDR_49], 0x11, 0x04 }, /* settle Set timing for dphy trans state from PRPR to SOT state */
		{ ltdev->clients[ADDR_49], 0x12, 0x04 }, /* trail */
		{ ltdev->clients[ADDR_49], 0x13, 0x00 }, /* 4 lane  // 01 lane // 02 2 lane //03 3lane */
		{ ltdev->clients[ADDR_49], 0x14, 0x00 }, /* debug mux */
		{ ltdev->clients[ADDR_49], 0x15, 0x00 },
		{ ltdev->clients[ADDR_49], 0x1a, 0x03 }, /* hshift 3 */
		{ ltdev->clients[ADDR_49], 0x1b, 0x03 }, /* vshift 3 */
		{ ltdev->clients[ADDR_49], 0x18, 0x28 }, /* hwidth 62 */
		{ ltdev->clients[ADDR_49], 0x19, 0x05 }, /* vwidth 6 */
		{ ltdev->clients[ADDR_49], 0x1c, 0x00 }, /* pix num hactive */
		{ ltdev->clients[ADDR_49], 0x1d, 0x05 },
		{ ltdev->clients[ADDR_49], 0x1e, 0x67 }, /* h v d pol hdmi sel pll sel */
		{ ltdev->clients[ADDR_49], 0x2f, 0x0c }, /* fifo_buff_length 12 */
		{ ltdev->clients[ADDR_49], 0x34, 0x72 }, /* htotal */
		{ ltdev->clients[ADDR_49], 0x35, 0x06 }, /* htotal */
		{ ltdev->clients[ADDR_49], 0x36, 0xee }, /* vtotal */
		{ ltdev->clients[ADDR_49], 0x37, 0x02 }, /* vtotal */
		{ ltdev->clients[ADDR_49], 0x38, 0x14 }, /* vbp */
		{ ltdev->clients[ADDR_49], 0x39, 0x00 }, /* vbp */
		{ ltdev->clients[ADDR_49], 0x3a, 0x05 }, /* vfp */
		{ ltdev->clients[ADDR_49], 0x3b, 0x00 }, /* vfp */
		{ ltdev->clients[ADDR_49], 0x3c, 0xdc }, /* hbp */
		{ ltdev->clients[ADDR_49], 0x3d, 0x00 }, /* hbp */
		{ ltdev->clients[ADDR_49], 0x3e, 0x6e }, /* hfp */
		{ ltdev->clients[ADDR_49], 0x3f, 0x00 }, /* hfp */
		{ ltdev->clients[ADDR_49], 0x72, 0x12 },
		{ ltdev->clients[ADDR_49], 0x73, 0x04 }, /* RGD_PTN_DE_DLY[7:0] */
		{ ltdev->clients[ADDR_49], 0x74, 0x01 }, /* RGD_PTN_DE_DLY[11:8]  260 */
		{ ltdev->clients[ADDR_49], 0x75, 0x19 }, /* RGD_PTN_DE_TOP[6:0]   150 */
		{ ltdev->clients[ADDR_49], 0x76, 0x00 }, /* RGD_PTN_DE_CNT[7:0] */
		{ ltdev->clients[ADDR_49], 0x77, 0xd0 }, /* RGD_PTN_DE_LIN[7:0] */
		{ ltdev->clients[ADDR_49], 0x78, 0x25 }, /* RGD_PTN_DE_LIN[10:8], RGD_PTN_DE_CNT[11:8] */
		{ ltdev->clients[ADDR_49], 0x79, 0x72 }, /* RGD_PTN_H_TOTAL[7:0] */
		{ ltdev->clients[ADDR_49], 0x7a, 0xee }, /* RGD_PTN_V_TOTAL[7:0] */
		{ ltdev->clients[ADDR_49], 0x7b, 0x26 }, /* RGD_PTN_V_TOTAL[10:8], RGD_PTN_H_TOTAL[11:8] */
		{ ltdev->clients[ADDR_49], 0x7c, 0x28 }, /* RGD_PTN_HWIDTH[7:0] */
		{ ltdev->clients[ADDR_49], 0x7d, 0x05 }, /* RGD_PTN_HWIDTH[9:8], RGD_PTN_VWIDTH[5:0] */
		{ ltdev->clients[ADDR_49], 0x70, 0x80 }, /* pattern enable */
		{ ltdev->clients[ADDR_49], 0x71, 0x51 },
		{ ltdev->clients[ADDR_49], 0x42, 0x12 },
		{ ltdev->clients[ADDR_49], 0x4e, 0xAA }, /* strm_sw_freq_word[ 7: 0] */
		{ ltdev->clients[ADDR_49], 0x4f, 0xAA }, /* strm_sw_freq_word[15: 8] */
		{ ltdev->clients[ADDR_49], 0x50, 0x6A }, /* strm_sw_freq_word[23:16] */
		{ ltdev->clients[ADDR_49], 0x51, 0x80 }, /* pattern en */
	};

	for (i = 0; i < ARRAY_SIZE(regs1); i++) {
		ret = lt8912_reg_write(&regs1[i]);
		if (ret)
			return ret;
	}
	ret = lt8912_clk_enable(ltdev);
	if (ret)
		return ret;
	ret = lt8912_tx_config(ltdev);
	if (ret)
		return ret;

	ret = lt8912_cbus_config(ltdev);
	if (ret)
		return ret;

	ret = lt8912_pll_config(ltdev);
	if (ret)
		return ret;

	ret = lt8912_audio_config(ltdev);
	if (ret)
		return ret;

	ret = lt8912_avi_config(ltdev);
	if (ret)
		return ret;

	for (i = 0; i < ARRAY_SIZE(regs2); i++) {
		ret = lt8912_reg_write(&regs2[i]);
		if (ret)
			return ret;
	}

	return lt8912_start(ltdev, 1);
}

static int lt8912_open(struct inode *inode, struct file *file)
{
	struct lt8912_dev *ltdev = &lt8912_device;

	if (!ltdev->clients[0])
		return -ENODEV;

	mutex_lock(&ltdev->mutex);
	if (ltdev->opened == 1) {
		mutex_unlock(&ltdev->mutex);
		return -EBUSY;
	}
	ltdev->opened = 1;
	mutex_unlock(&ltdev->mutex);

	if (ltdev->reset_gpio) {
		gpiod_set_value(ltdev->reset_gpio, 0);
		udelay(100);
		gpiod_set_value(ltdev->reset_gpio, 1);
		udelay(1000);
	}

	file->private_data = ltdev;
	print_lt8912_chipid(ltdev);

	return 0;
}

static int lt8912_close(struct inode *inode, struct file *file)
{
	struct lt8912_dev *ltdev = file->private_data;

	mutex_lock(&ltdev->mutex);
	if (ltdev->opened == 1) {
		ltdev->opened = 0;
	}
	mutex_unlock(&ltdev->mutex);
	//lt8912_suspend(ltdev, 1);
	//lt8912_start(ltdev, 0);
	return 0;
}

#define LT8912B_IOC_MAGIC	'M'
#define LT8912B_START		_IOW(LT8912B_IOC_MAGIC, 1, int)
#define LT8912B_STOP		_IOW(LT8912B_IOC_MAGIC, 2, int)
#define LT8912B_TEST		_IOW(LT8912B_IOC_MAGIC, 3, int)

static long int lt8912_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret;
	struct lt8912_dev *ltdev = file->private_data;

	if (_IOC_TYPE(cmd) != LT8912B_IOC_MAGIC)
		return -ENOTTY;

	switch (cmd) {
	case LT8912B_START:
		pr_info("hzhy-debug: startting hdmi\n");
		ret = lt8912b_setup(ltdev);
		if (ret)
			return ret;
		ret = lt8912_start(ltdev, 1);
		if (ret)
			return ret;
		ret = lt8912_suspend(ltdev, 0);
		if (ret)
			return ret;
		break;
	case LT8912B_STOP:
		pr_info("hzhy-debug: stopping hdmi\n");
		ret = lt8912_suspend(ltdev, 1);
		if (ret)
			return ret;
		ret = lt8912_start(ltdev, 0);
		if (ret)
			return ret;
		break;
	case LT8912B_TEST:
		pr_info("hzhy-debug: testting hdmi\n");
		ret = lt8912b_test(ltdev);
		if (ret)
			return ret;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static const struct file_operations lt8912_misc_fops = {
	.owner		= THIS_MODULE,
	.open		= lt8912_open,
	.release	= lt8912_close,
	.unlocked_ioctl = lt8912_ioctl,
};

static const struct regmap_config lt8912b_regmap = {
	.reg_bits = 8,
	.val_bits = 8,
	
	.use_single_read = true,
	.use_single_write = true,
};

static int lt8912b_probe(struct i2c_client *client,
		const struct i2c_device_id *i2c_id)
{
	struct lt8912_dev *ltdev = &lt8912_device;
	struct lt8912_client *cli;
	int ret;
	const struct regmap_config *regmap_config;

	cli = devm_kzalloc(&client->dev, sizeof(*cli), GFP_KERNEL);
	if (cli == NULL)
		return -ENOMEM;
	
	cli->client = client;

	regmap_config = &lt8912b_regmap;
	cli->regmap = devm_regmap_init_i2c(client, regmap_config);
	if (IS_ERR(cli->regmap)) {
		ret = PTR_ERR(cli->regmap);
		return ret;
	}
	regcache_mark_dirty(cli->regmap);

	pr_info("hzhy-debug: client->addr = 0x%x\n", client->addr);
	switch (client->addr) {
	case 0x48:
		ltdev->clients[ADDR_48] = cli;
		break;
	case 0x49:
		ltdev->clients[ADDR_49] = cli;
		break;
	case 0x4a:
		ltdev->clients[ADDR_4a] = cli;
		break;
	default:
		pr_err("Unrecognized client id %x\n", client->addr);
		return -EINVAL;
	}

	ltdev->reset_gpio = devm_gpiod_get_optional(&client->dev, "reset",
					     GPIOD_OUT_LOW);
	if (IS_ERR(ltdev->reset_gpio))
		return PTR_ERR(ltdev->reset_gpio);

	i2c_set_clientdata(client, cli);

	return 0;
}

static int lt8912b_remove(struct i2c_client *client)
{
	struct lt8912_client *cli = i2c_get_clientdata(client);

	switch (client->addr) {
	case 0x48:
		lt8912_device.clients[ADDR_48] = NULL;
		break;
	case 0x49:
		lt8912_device.clients[ADDR_49] = NULL;
		break;
	case 0x4a:
		lt8912_device.clients[ADDR_4a] = NULL;
		break;
	}

	devm_kfree(&client->dev, cli);
	return 0;
}

static const struct i2c_device_id lt8912b_id[] = {
	{ "lt8912b", 0 },
	{}
};
MODULE_DEVICE_TABLE(i2c, lt8912b_id);

static const struct of_device_id lt8912b_dt_ids[] = {
	{ .compatible = "lontium,lt8912b", .data = NULL },
	{ }
};
MODULE_DEVICE_TABLE(of, lt8912b_dt_ids);

static struct i2c_driver lt8912b_driver = {
	.driver = {
		.name = "lt8912b",
		.of_match_table = lt8912b_dt_ids,
	},
	.probe = lt8912b_probe,
	.remove = lt8912b_remove,
	.id_table = lt8912b_id,
};

static struct miscdevice lt8912_miscdev = {
	.name  = "lt8912b",
	.fops  = &lt8912_misc_fops,
};

static int __init lt8912b_init(void)
{
	int ret;

	memset(&lt8912_device, 0x0, sizeof(lt8912_device));
	mutex_init(&lt8912_device.mutex);
	lt8912_device.Hsync_L_last = 0;
	lt8912_device.Hsync_H_last = 0;
	lt8912_device.Vsync_L_last = 0;
	lt8912_device.Vsync_H_last = 0;
	lt8912_device.suspend = 0;
	lt8912_device.mode = MODE_1080P;
	lt8912_device.opened = 0;

	ret = misc_register(&lt8912_miscdev);
	if (ret) {
		pr_err("Cannot register miscdev for lt8912b(err=%d)\n", ret);
		return ret;
	}

	return i2c_add_driver(&lt8912b_driver);
}

static void __exit lt8912b_exit(void)
{
	i2c_del_driver(&lt8912b_driver);
	misc_deregister(&lt8912_miscdev);
}

module_init(lt8912b_init);
module_exit(lt8912b_exit);

MODULE_AUTHOR("chenxiaodong <chenxd@hzhytech.com>");
MODULE_DESCRIPTION("lt8912b mipi to hdmi out driver");
MODULE_LICENSE("GPL");

