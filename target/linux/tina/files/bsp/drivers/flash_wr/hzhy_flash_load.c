#include <linux/module.h>
#include <linux/mtd/mtd.h>
#include <linux/firmware.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/sysfs.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <crypto/hash.h>

#define FLASH_OFFSET        0x0      // 写入起始偏移（从Flash起始地址开始）
#define FLASH_INFO_OFFSET   1024  

#define GPIO_ACT_HIGH 1
#define GPIO_ACT_LOW 0

// MTD设备最大数量定义
#ifndef MAX_MTD_DEVICES
#define MAX_MTD_DEVICES 32
#endif

#define MAX_BUF_SIZE 128
#define MAX_FILE_LENS 32
#define MAX_MD5_LENS 32

static struct mtd_info *mtd_dev;
static struct fpga_load_data *g_fpga_data;
static struct class *fpga_class;
static const struct attribute_group fpga_attr_group;


struct fpga_load_data {
    struct platform_device *pdev;
    int sw_gpio_pin;           // flash-sw-gpio引脚号
    int gpio_sw_polarity; // flash-sw-gpio极性 0:低电平 1:高电平

    int rst_gpio_pin;           // fpga-rst-gpio引脚号
    int gpio_rst_polarity; // fpga-rst-gpio极性 0:低电平 1:高电平

    struct device *sysfs_dev;  // sysfs设备节点
    char firmware_name[MAX_BUF_SIZE];    // 固件文件名，最大32字符+1个'\0'
    char spi_flash_ctrl[16];   // SPI控制器名称，从设备树获取

    unsigned long last_update_time; // 最后一次固件更新时间（jiffies）
    unsigned int min_update_interval; // 最小更新间隔（秒），默认10秒
    char file_name[MAX_BUF_SIZE]; // 文件名
    char md5_check_code[MAX_BUF_SIZE]; // md5校验码
};


// 获取SPI设备信息的通用函数
static int get_spi_device_info(struct device *dev, char *device_name, size_t name_len)
{
    if (!g_fpga_data || !device_name) {
        dev_err(dev, "Invalid parameters for SPI device info\n");
        return -EINVAL;
    }
    
    if (strlen(g_fpga_data->spi_flash_ctrl) == 0) {
        dev_err(dev, "SPI flash controller name not available\n");
        return -EINVAL;
    }
    
    snprintf(device_name, name_len, "%s.0", g_fpga_data->spi_flash_ctrl);
    dev_info(dev, "Generated SPI device name: %s\n", device_name);
    
    return 0;
}

// SPI设备重新绑定函数
static int rebind_spi_nor_device(struct device *dev)
{
    struct file *unbind_file, *bind_file;
    loff_t pos = 0;
    int ret = 0;
    char device_name[MAX_BUF_SIZE];
    const char *unbind_path = "/sys/bus/spi/drivers/spi-nor/unbind";
    const char *bind_path = "/sys/bus/spi/drivers/spi-nor/bind";
    
    // 获取SPI设备名称
    ret = get_spi_device_info(dev, device_name, sizeof(device_name));
    if (ret) {
        return ret;
    }
    
    unbind_file = filp_open(unbind_path, O_WRONLY, 0);
    if (IS_ERR(unbind_file)) {
        dev_err(dev, "Failed to open unbind file %s: %ld\n", unbind_path, PTR_ERR(unbind_file));
    } else {
        pos = 0;
        kernel_write(unbind_file, device_name, strlen(device_name), &pos);
        filp_close(unbind_file, NULL);
        
        msleep(100);
    }
    
    bind_file = filp_open(bind_path, O_WRONLY, 0);
    if (IS_ERR(bind_file)) {
        dev_err(dev, "Failed to open bind file %s: %ld\n", bind_path, PTR_ERR(bind_file));
        ret = PTR_ERR(bind_file);
    } else {
        pos = 0;
        kernel_write(bind_file, device_name, strlen(device_name), &pos);
        filp_close(bind_file, NULL);
    }
    
    msleep(100);
    return ret;
}


static int get_mtd_device_flexible(struct fpga_load_data *data)
{
    const char *try_names[] = {
        "jedec,spi-nor",
        "spi-nor",
        "nor-flash", 
        NULL
    };
    int i;
    struct mtd_info *mtd;
    char spi_device_name[MAX_BUF_SIZE];
    
    // 构建动态SPI设备名称
    if (data && strlen(data->spi_flash_ctrl) > 0) {
        snprintf(spi_device_name, sizeof(spi_device_name), "%s.0", data->spi_flash_ctrl);
        mtd = get_mtd_device_nm(spi_device_name);
        if (!IS_ERR(mtd)) {
            mtd_dev = mtd;
            return 0;
        }
    }
    
    // 按通用名称查找
    for (i = 0; try_names[i] != NULL; i++) {
        mtd = get_mtd_device_nm(try_names[i]);
        if (!IS_ERR(mtd)) {
            mtd_dev = mtd;
            return 0;
        }
    }
    
    dev_err(&data->pdev->dev, "No suitable MTD device found\n");
    return -ENODEV;
}


static int reacquire_mtd_device(struct device *dev)
{
    int ret;
    bool need_rebind = false;
    
    if (!g_fpga_data) {
        dev_err(dev, "FPGA data not available\n");
        return -EINVAL;
    }
    
    // 检查当前MTD设备状态
    if (!mtd_dev) {
        need_rebind = true;
    } else {
        if (!mtd_dev->name || mtd_dev->size == 0) {
            need_rebind = true;
            put_mtd_device(mtd_dev);
            mtd_dev = NULL;
        } 
    }
    
    if (need_rebind) {
        ret = rebind_spi_nor_device(dev);
        if (ret) {
            dev_warn(dev, "SPI NOR rebind failed: %d, continuing anyway\n", ret);
        }
    }
    
    if (!mtd_dev) {
        ret = get_mtd_device_flexible(g_fpga_data);
        if (ret) {
            dev_err(dev, "Failed to acquire MTD device after SPI rebind: %d\n", ret);
            return ret;
        }
        dev_info(dev, "Successfully acquired MTD device: %s\n", mtd_dev->name);
    } else {
        bool mtd_invalid = false;
        
        if (!mtd_dev->name || strlen(mtd_dev->name) == 0) {
            dev_warn(dev, "MTD device name is invalid (name=%p)\n", mtd_dev->name);
            mtd_invalid = true;
        } else if (mtd_dev->size == 0 || mtd_dev->erasesize == 0) {
            dev_warn(dev, "MTD device parameters invalid (size=%llu, erasesize=%u)\n", 
                     mtd_dev->size, mtd_dev->erasesize);
            mtd_invalid = true;
        }
        
        if (mtd_invalid) {
            dev_warn(dev, "MTD device appears invalid, re-acquiring\n");
            put_mtd_device(mtd_dev);
            mtd_dev = NULL;
            ret = get_mtd_device_flexible(g_fpga_data);
            if (ret) {
                dev_err(dev, "Failed to re-acquire MTD device: %d\n", ret);
                return ret;
            }
        } else {
            dev_info(dev, "MTD device validation passed: %s (size=%lluMB, erasesize=%uKB)\n",
                     mtd_dev->name, mtd_dev->size >> 20, mtd_dev->erasesize >> 10);
        }
    }
    
    return 0;
}


// 解析设备树中的GPIO配置
static int parse_gpio_from_dt(struct platform_device *pdev, struct fpga_load_data *data)

{
    int  gpio, ret;
    enum of_gpio_flags flags;
    const char *spi_ctrl_name;
    struct device_node *np = pdev->dev.of_node;
    
    if (!np) {
        dev_err(&pdev->dev, "No device tree node found\n");
        return -ENODEV;
    }
    
    // 获取 flash-sw-gpios 配置
    gpio = of_get_named_gpio_flags(np, "flash-sw-gpios", 0, &flags);
    if (!gpio_is_valid(gpio)) {
        dev_err(&pdev->dev, "Failed to get flash-sw-gpios from device tree: %d\n", gpio);
        return gpio < 0 ? gpio : -EINVAL;
    }
    
    data->sw_gpio_pin = gpio;
    data->gpio_sw_polarity = (flags & OF_GPIO_ACTIVE_LOW) ? GPIO_ACT_LOW : GPIO_ACT_HIGH;
    
    dev_info(&pdev->dev, "Flash SW GPIO %d Active: %s\n", 
             gpio, data->gpio_sw_polarity == GPIO_ACT_HIGH ? "GPIO_ACTIVE_HIGH" : "GPIO_ACTIVE_LOW");
    
    if (gpio_request(gpio, "flash-sw-gpio")) {
        dev_err(&pdev->dev, "Failed to request Flash SW GPIO %d\n", gpio);
        return -EBUSY;
    }
    
    gpio_direction_output(gpio, !data->gpio_sw_polarity);
    dev_info(&pdev->dev, "Flash SW GPIO %d direction output: %d\n", gpio, !data->gpio_sw_polarity);


    // 获取 fpga-rst-gpios 配置
    gpio = of_get_named_gpio_flags(np, "fpga-rst-gpios", 0, &flags);
    if (!gpio_is_valid(gpio)) {
        dev_err(&pdev->dev, "Failed to get fpga-rst-gpios from device tree: %d\n", gpio);
        return gpio < 0 ? gpio : -EINVAL;
    }
    
    data->rst_gpio_pin = gpio;
    data->gpio_rst_polarity = (flags & OF_GPIO_ACTIVE_LOW) ? GPIO_ACT_LOW : GPIO_ACT_HIGH;

    dev_info(&pdev->dev, "FPGA RST GPIO %d Active: %s\n", 
             gpio, data->gpio_rst_polarity == GPIO_ACT_HIGH ? "GPIO_ACTIVE_HIGH" : "GPIO_ACTIVE_LOW");
    
    if (gpio_request(gpio, "fpga-rst-gpio")) {
        dev_err(&pdev->dev, "Failed to request FPGA RS T GPIO %d\n", gpio);
        return -EBUSY;
    }
    
    gpio_direction_output(gpio, !data->gpio_rst_polarity);
    dev_info(&pdev->dev, "FPGA RST GPIO %d direction output: %d\n", gpio, !data->gpio_rst_polarity);
    
    // 获取 spi-flash-ctrl 配置
    ret = of_property_read_string(np, "spi-flash-ctrl", &spi_ctrl_name);
    if (ret) {
        dev_warn(&pdev->dev, "Failed to get spi-flash-ctrl from device tree, using default 'spi4'\n");
        strcpy(data->spi_flash_ctrl, "spi4");
    } else {
        strncpy(data->spi_flash_ctrl, spi_ctrl_name, sizeof(data->spi_flash_ctrl) - 1);
        data->spi_flash_ctrl[sizeof(data->spi_flash_ctrl) - 1] = '\0';
        dev_info(&pdev->dev, "SPI Flash Controller: %s\n", data->spi_flash_ctrl);
    }
    
    return 0;
}

// 获取当前MTD设备信息的通用函数
static int get_current_mtd_info(struct device *dev, char *info_buf, size_t buf_len)
{
    if (!mtd_dev || !info_buf) {
        dev_err(dev, "MTD device not available or invalid buffer\n");
        return -EINVAL;
    }
    
    snprintf(info_buf, buf_len, "MTD: %s, size=%lluMB, erasesize=%uKB, type=%d", 
             mtd_dev->name ? mtd_dev->name : "unnamed",
             mtd_dev->size >> 20, 
             mtd_dev->erasesize >> 10,
             mtd_dev->type);
    
    return 0;
}



// 创建sysfs接口
static int create_sysfs_interface(struct fpga_load_data *data)
{
    int ret;
    
    // 创建设备类
    if (!fpga_class) {
        fpga_class = class_create(THIS_MODULE, "hzhy-fpga");
        if (IS_ERR(fpga_class)) {
            dev_err(&data->pdev->dev, "Failed to create device class\n");
            return PTR_ERR(fpga_class);
        }
    }
    
    // 创建设备节点
    data->sysfs_dev = device_create(fpga_class, &data->pdev->dev, 
                                   MKDEV(0, 0), data, "fpga_load");
    if (IS_ERR(data->sysfs_dev)) {
        dev_err(&data->pdev->dev, "Failed to create device node\n");
        ret = PTR_ERR(data->sysfs_dev);
        goto err_class;
    }
    
    // 创建属性文件
    ret = sysfs_create_group(&data->sysfs_dev->kobj, &fpga_attr_group);
    if (ret) {
        dev_err(&data->pdev->dev, "Failed to create sysfs attributes\n");
        goto err_device;
    }
    
    // dev_info(&data->pdev->dev, "Created sysfs interface: /sys/class/hzhy-fpga/fpga_load/sw-gpio\n");
    return 0;
    
err_device:
    device_destroy(fpga_class, MKDEV(0, 0));
    data->sysfs_dev = NULL;
err_class:
    class_destroy(fpga_class);
    fpga_class = NULL;
    return ret;
}

// 销毁sysfs接口
static void destroy_sysfs_interface(struct fpga_load_data *data)
{
    if (data->sysfs_dev) {
        sysfs_remove_group(&data->sysfs_dev->kobj, &fpga_attr_group);
        device_destroy(fpga_class, MKDEV(0, 0));
        data->sysfs_dev = NULL;
    }
    
    if (fpga_class) {
        class_destroy(fpga_class);
        fpga_class = NULL;
    }
    
    dev_info(&data->pdev->dev, "Destroyed sysfs interface\n");
}

// 从 /lib/firmware/ 加载固件
static int find_firmware(const char *name, u8 **buf, size_t *size)
{
    const struct firmware *fw;
    int ret;

    ret = request_firmware(&fw, name, NULL);
    if (ret) {
        pr_err("Failed to load firmware '%s': %d\n", name, ret);
        return ret;
    }

    *buf = kmemdup(fw->data, fw->size, GFP_KERNEL);
    *size = fw->size;
    release_firmware(fw);

    if (!*buf) {
        pr_err("Failed to allocate firmware buffer\n");
        return -ENOMEM;
    }

    return 0;
}

// 擦除 Flash 区域（4KB 对齐）
static int erase_flash(loff_t offset, size_t len)
{
    struct erase_info erase = {
        .addr = offset,
        .len = len,
    };
    int ret;

    ret = mtd_erase(mtd_dev, &erase);
    if (ret) {
        pr_err("Erase failed at 0x%llx: %d\n", offset, ret);
        return ret;
    }
    return 0;
}

// 写入数据到 Flash
static int write_flash(loff_t offset, const u8 *buf, size_t len)
{
    size_t retlen;
    int ret;

    ret = mtd_write(mtd_dev, offset, len, &retlen, buf);
    if (ret || retlen != len) {
        pr_err("Write failed: ret=%d, retlen=%zu\n", ret, retlen);
        return -EIO;
    }
    return 0;
}

// 读取数据从 Flash
static int read_flash(loff_t offset, u8 *buf, size_t len)
{
    size_t retlen;
    int ret;
    ret = mtd_read(mtd_dev, offset, len, &retlen, buf);
    if (ret || retlen != len) {
        pr_err("Read failed: ret=%d, retlen=%zu\n", ret, retlen);
        return -EIO;
    }

    return ret;
}

// 计算MD5校验码
static int calculate_md5_check_code(const u8 *data, size_t len, char *md5_str)
{
    struct crypto_shash *tfm;
    struct shash_desc *desc;
    u8 hash[16];
    int i, ret = 0;
    
    tfm = crypto_alloc_shash("md5", 0, 0);
    if (IS_ERR(tfm)) {
        pr_err("Failed to allocate MD5 hash algorithm\n");
        return PTR_ERR(tfm);
    }
    
    desc = kmalloc(sizeof(*desc) + crypto_shash_descsize(tfm), GFP_KERNEL);
    if (!desc) {
        ret = -ENOMEM;
        goto free_tfm;
    }
    
    desc->tfm = tfm;
    
    ret = crypto_shash_init(desc);
    if (ret)
        goto free_desc;
    
    ret = crypto_shash_update(desc, data, len);
    if (ret)
        goto free_desc;
    
    ret = crypto_shash_final(desc, hash);
    if (ret)
        goto free_desc;
    
    // 转换为十六进制字符串
    for (i = 0; i < 16; i++) {
        sprintf(md5_str + i * 2, "%02x", hash[i]);
    }
    md5_str[32] = '\0';
    
free_desc:
    kfree(desc);
free_tfm:
    crypto_free_shash(tfm);
    return ret;
}

static int update_fpga_firmware(void)
{
    int ret;
    size_t fw_size;
    u8 *fw_buf = NULL;
    loff_t offset = FLASH_OFFSET;
    char fw_name[MAX_BUF_SIZE] = {0};
    char read_md5[MAX_BUF_SIZE] = {0};
    size_t erase_len;
    char *load_file_data = NULL;
    char verify_buf[MAX_BUF_SIZE] = {0};
    size_t erase_size;
    loff_t info_offset;
    loff_t erase_offset;

    if (!g_fpga_data) {
        pr_err("FPGA data not available\n");
        return -EINVAL;
    }
    
    if (!mtd_dev) {
        pr_err("MTD device not available\n");
        return -ENODEV;
    }
    
    if (!mtd_dev->name || strlen(mtd_dev->name) == 0) {
        pr_err("MTD device appears to be invalid (name is NULL or empty)\n");
        return -EINVAL;
    }
    
    if (mtd_dev->size == 0 || mtd_dev->erasesize == 0) {
        pr_err("MTD device has invalid size or erasesize\n");
        return -EINVAL;
    }
    
    pr_info("Using MTD device: %s (size=%lluMB, erasesize=%uKB)\n",
            mtd_dev->name, mtd_dev->size >> 20, mtd_dev->erasesize >> 10);
    
    strcpy(g_fpga_data->file_name, g_fpga_data->firmware_name);
    
    // pr_info("find firmware: %s\n", fw_name);
    ret = find_firmware(g_fpga_data->file_name, &fw_buf, &fw_size);
    if (ret != 0) {
        pr_err("Failed to find firmware: %s\n", g_fpga_data->file_name);
        return ret;
    }
    
    pr_info("Successfully find firmware: %s (size: %zu bytes)\n", g_fpga_data->file_name, fw_size);

    ret = calculate_md5_check_code(fw_buf, fw_size, g_fpga_data->md5_check_code);
    if (ret) {
        pr_err("Failed to calculate MD5 check code: %d\n", ret);
        kfree(fw_buf);
        return ret;
    }

    if (offset % mtd_dev->erasesize != 0) {
        pr_err("Offset 0x%llx must be aligned to 0x%x\n", 
               offset, mtd_dev->erasesize);
        kfree(fw_buf);
        return -EINVAL;
    }

    erase_len = ALIGN(fw_size, mtd_dev->erasesize);

    ret = erase_flash(offset, erase_len);
    if (ret) goto out;

    ret = write_flash(offset, fw_buf, fw_size);
    if (ret) {
        pr_err("Failed to write firmware to flash\n");
        goto out;
    } 
    msleep(100);

    load_file_data = kmalloc(fw_size, GFP_KERNEL);
    if (!load_file_data) {
        pr_err("Failed to allocate memory for firmware verification\n");
        goto out;
    }

    ret = read_flash(offset, load_file_data, fw_size);
    if (ret) {
        pr_err("Failed to read firmware from flash for verification\n");
        kfree(load_file_data);
        goto out;
    }
 
    ret = calculate_md5_check_code(load_file_data, fw_size, read_md5);
    if (ret) {
        pr_err("Failed to calculate MD5 for read firmware\n");
        kfree(load_file_data);
        goto out;
    }
    
    if (strcmp(g_fpga_data->md5_check_code, read_md5) == 0) {
        pr_info("Firmware verification PASSED - MD5 match: %s\n", read_md5);
    } else {
        pr_err("Firmware verification FAILED - MD5 mismatch!\n");
        pr_err("  Original: %s\n", g_fpga_data->md5_check_code);
        pr_err("  Read:     %s\n", read_md5);
        ret = -EIO;
        kfree(load_file_data);
        goto out;
    }
    
    kfree(load_file_data);
    load_file_data = NULL;
    
    strcpy(fw_name, g_fpga_data->file_name);
    strcat(fw_name, ":");
    strcat(fw_name, g_fpga_data->md5_check_code);

   
    info_offset = mtd_dev->size - FLASH_INFO_OFFSET;
    erase_size = mtd_dev->erasesize; // 4KB
    
    // 擦除地址对齐到扇区边界
    erase_offset = (info_offset / erase_size) * erase_size;
    
    pr_info("Erasing info area: offset=0x%llx, size=0x%zx\n", erase_offset, erase_size);
    ret = erase_flash(erase_offset, erase_size);
    if (ret) {
        pr_err("Failed to erase info area: %d\n", ret);
        goto out;
    }

    ret = write_flash(info_offset, fw_name, strlen(fw_name));
    if (ret) {
        pr_err("Failed to write file name and md5 check code to flash\n");
        goto out;
    }
    
    // 立即读取验证写入是否成功
    
    msleep(100); 
    ret = read_flash(info_offset, (u8*)verify_buf, MAX_BUF_SIZE);
    if (ret) {
        pr_err("Failed to read back for verification\n");
    } else {
        verify_buf[MAX_BUF_SIZE - 1] = '\0';
        // pr_info("Verification read: '%s'\n", verify_buf);
        if (strncmp(fw_name, verify_buf, strlen(fw_name)) == 0) {
            pr_info("Write verification PASSED\n");
        } else {
            pr_err("Write verification FAILED\n");
            pr_err("  Expected: '%s'\n", fw_name);
            pr_err("  Read:     '%s'\n", verify_buf);
        }
    }

out:
    kfree(fw_buf);
    kfree(load_file_data);
    return ret;
}


// sysfs属性：读取SW-GPIO状态
static ssize_t sw_gpio_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int gpio_value;
    
    if (!g_fpga_data || !gpio_is_valid(g_fpga_data->sw_gpio_pin)) {
        return sprintf(buf, "error: Flash SW GPIO not configured\n");
    }
    
    gpio_value = gpio_get_value(g_fpga_data->sw_gpio_pin);
    
    return sprintf(buf, "Flash_SW_GPIO%d:%d\n", g_fpga_data->sw_gpio_pin, gpio_value);
}

// sysfs属性：设置 SW-GPIO状态
static ssize_t sw_gpio_store(struct device *dev, struct device_attribute *attr, 
                            const char *buf, size_t count)
{
    int value;
    int ret;
    
    if (!g_fpga_data || !gpio_is_valid(g_fpga_data->sw_gpio_pin)) {
        dev_err(dev, "Flash SW GPIO not configured\n");
        return -EINVAL;
    }
    
    ret = kstrtoint(buf, 10, &value);
    if (ret) {
        dev_err(dev, "Invalid input, use 0 or 1\n");
        return ret;
    }
    
    if (value != 0 && value != 1) {
        dev_err(dev, "Invalid value %d, use 0 or 1\n", value);
        return -EINVAL;
    }
    
    // 设置GPIO电平
    gpio_set_value(g_fpga_data->sw_gpio_pin, value);
    
    dev_info(dev, "Flash SW GPIO%d set to %d (%s)\n", 
             g_fpga_data->sw_gpio_pin, value, value ? "HIGH" : "LOW");
    
    return count;
}

// sysfs属性：读取固件名称
static ssize_t firmware_name_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    if (!g_fpga_data) {
        dev_err(dev, "FPGA data not available\n");
        return -EINVAL;
    }
    
    return sprintf(buf, "%s\n", g_fpga_data->firmware_name);
}

// sysfs属性：设置固件名称
static ssize_t firmware_name_store(struct device *dev, struct device_attribute *attr,
                                   const char *buf, size_t count)
{
    char temp_name[MAX_BUF_SIZE];
    size_t len;
    
    if (!g_fpga_data) {
        dev_err(dev, "FPGA data not available\n");
        return -EINVAL;
    }
    
    len = count;
    if (len > 0 && buf[len - 1] == '\n') {
        len--; 
    }
    
    if (len == 0) {
        dev_err(dev, "Firmware name cannot be empty\n");
        return -EINVAL;
    }
    
    if (len > 32) {
        dev_err(dev, "Firmware name too long (max 32 characters)\n");
        return -EINVAL;
    }
    
    memcpy(temp_name, buf, len);
    temp_name[len] = '\0';
    
    strcpy(g_fpga_data->firmware_name, temp_name);
    
    dev_info(dev, "Firmware name updated to: %s\n", g_fpga_data->firmware_name);
    
    return count;
}

// sysfs属性：读取/控制RST GPIO状态
static ssize_t rst_gpio_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int gpio_value;
    
    if (!g_fpga_data || !gpio_is_valid(g_fpga_data->rst_gpio_pin)) {
        return sprintf(buf, "error: FPGA RST GPIO not configured\n");
    }
    
    gpio_value = gpio_get_value(g_fpga_data->rst_gpio_pin);
    
    return sprintf(buf, "FPGA_RST_GPIO%d:%d\n", g_fpga_data->rst_gpio_pin, gpio_value);
}

static ssize_t rst_gpio_store(struct device *dev, struct device_attribute *attr, 
                             const char *buf, size_t count)
{
    int value;
    int ret;
    
    if (!g_fpga_data || !gpio_is_valid(g_fpga_data->rst_gpio_pin)) {
        dev_err(dev, "FPGA RST GPIO not configured\n");
        return -EINVAL;
    }
    
    ret = kstrtoint(buf, 10, &value);
    if (ret) {
        dev_err(dev, "Invalid value for RST GPIO (must be 0 or 1)\n");
        return ret;
    }
    
    if (value != 0 && value != 1) {
        dev_err(dev, "Invalid value: %d (must be 0 or 1)\n", value);
        return -EINVAL;
    }
    
    // 设置GPIO电平
    gpio_set_value(g_fpga_data->rst_gpio_pin, value);
    
    dev_info(dev, "FPGA RST GPIO%d set to %d (%s)\n", 
             g_fpga_data->rst_gpio_pin, value, value ? "HIGH" : "LOW");
    
    return count;
}

// sysfs属性：读取/设置固件更新最小时间间隔（10-300秒）
static ssize_t update_interval_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    if (!g_fpga_data) {
        return sprintf(buf, "error: FPGA data not available\n");
    }
    
    return sprintf(buf, "%u\n", g_fpga_data->min_update_interval);
}

static ssize_t update_interval_store(struct device *dev, struct device_attribute *attr, 
                                    const char *buf, size_t count)
{
    unsigned int interval;
    int ret;
    
    if (!g_fpga_data) {
        dev_err(dev, "FPGA data not available\n");
        return -EINVAL;
    }
    
    ret = kstrtouint(buf, 10, &interval);
    if (ret) {
        dev_err(dev, "Invalid interval value\n");
        return ret;
    }
    
    // 限制间隔范围：10秒到300秒（5分钟）
    if (interval < 10 || interval > 300) {
        dev_err(dev, "Invalid interval: %u (must be 10-300 seconds)\n", interval);
        return -EINVAL;
    }
    
    g_fpga_data->min_update_interval = interval;
    
    dev_info(dev, "Minimum update interval set to %u seconds\n", interval);
    
    return count;
}

// sysfs属性：读取Flash中的固件信息（文件名和MD5）
static ssize_t firmware_md5_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int ret;
    int filename_len;
    char *colon_pos;
    char md5[MAX_BUF_SIZE] = {0};
    char filename[MAX_BUF_SIZE] = {0};
    char flash_buf[MAX_BUF_SIZE] = {0};
    
    if (!g_fpga_data) {
        return sprintf(buf, "ERROR: FPGA data not available\n");
    }
    
    gpio_set_value(g_fpga_data->sw_gpio_pin, g_fpga_data->gpio_sw_polarity);
    msleep(100);
    
    ret = reacquire_mtd_device(dev);
    if (ret) {
        gpio_set_value(g_fpga_data->sw_gpio_pin, !g_fpga_data->gpio_sw_polarity);
        return sprintf(buf, "ERROR: Failed to acquire MTD device (%d)\n", ret);
    }
    
    if (!mtd_dev || mtd_dev->size < FLASH_INFO_OFFSET) {
        gpio_set_value(g_fpga_data->sw_gpio_pin, !g_fpga_data->gpio_sw_polarity);
        return sprintf(buf, "ERROR: MTD device invalid or too small\n");
    }
    
    ret = read_flash(mtd_dev->size - FLASH_INFO_OFFSET, (u8*)flash_buf, MAX_BUF_SIZE);
    if (ret) {
        gpio_set_value(g_fpga_data->sw_gpio_pin, !g_fpga_data->gpio_sw_polarity);
        return sprintf(buf, "ERROR: Failed to read from flash (%d)\n", ret);
    }
    
    flash_buf[MAX_BUF_SIZE - 1] = '\0';
   
    colon_pos = strchr(flash_buf, ':');
    if (!colon_pos) {
        gpio_set_value(g_fpga_data->sw_gpio_pin, !g_fpga_data->gpio_sw_polarity);
        return sprintf(buf, "ERROR: Cannot find ':' separator, unable to get MD5 checksum\n");
    }
    
    filename_len = colon_pos - flash_buf;
    if (filename_len > MAX_FILE_LENS - 1) filename_len = MAX_FILE_LENS - 1;
    strncpy(filename, flash_buf, filename_len);
    filename[filename_len] = '\0';
    
    strncpy(md5, colon_pos + 1, MAX_MD5_LENS);
    md5[MAX_MD5_LENS] = '\0';
    
    gpio_set_value(g_fpga_data->sw_gpio_pin, !g_fpga_data->gpio_sw_polarity);
    
    // 返回结果
    return sprintf(buf, "Filename: %s  MD5: %s\n", filename, md5);
}

// sysfs属性：触发固件更新
static ssize_t firmware_update_store(struct device *dev, struct device_attribute *attr,
                                    const char *buf, size_t count)
{
    int value;
    int ret;
    
    if (!g_fpga_data) {
        dev_err(dev, "FPGA data not available\n");
        return -EINVAL;
    }
    
    if (!gpio_is_valid(g_fpga_data->sw_gpio_pin)) {
        dev_err(dev, "Flash SW GPIO not configured\n");
        return -EINVAL;
    }
    
    if (g_fpga_data->last_update_time != 0) {
        unsigned long time_diff = jiffies - g_fpga_data->last_update_time;
        unsigned long min_interval = g_fpga_data->min_update_interval * HZ;
        
        if (time_diff < min_interval) {
            unsigned long remaining = (min_interval - time_diff) / HZ;
            dev_warn(dev, "Firmware update too frequent! Please wait %lu more seconds\n", remaining + 1);
            dev_warn(dev, "Last update was %lu seconds ago, minimum interval is %u seconds\n", 
                     time_diff / HZ, g_fpga_data->min_update_interval);
            return -EAGAIN;
        }
    }
    
    ret = kstrtoint(buf, 10, &value);
    if (ret) {
        dev_err(dev, "Invalid input, use 1 to trigger firmware update\n");
        return ret;
    }
    
    if (value != 1) {
        dev_warn(dev, "Invalid value %d, use 1 to trigger firmware update\n", value);
        return -EINVAL;
    }
    
    dev_info(dev, "Activating Flash SW GPIO%d value: %d for firmware update\n", g_fpga_data->sw_gpio_pin, g_fpga_data->gpio_sw_polarity);
    
    gpio_set_value(g_fpga_data->sw_gpio_pin, g_fpga_data->gpio_sw_polarity);
    msleep(100);
    
    ret = reacquire_mtd_device(dev);
    if (ret) {
        dev_err(dev, "Failed to acquire/validate MTD device: %d\n", ret);
        return ret;
    }

    ret = update_fpga_firmware();
    if (ret) {
        dev_err(dev, "Firmware update failed: %d\n", ret);
        if (mtd_dev) {
            dev_warn(dev, "Releasing MTD device due to firmware update failure\n");
            put_mtd_device(mtd_dev);
            mtd_dev = NULL;
        }
        return ret;
    }

    msleep(100);
    gpio_set_value(g_fpga_data->sw_gpio_pin, !g_fpga_data->gpio_sw_polarity);


    // 触发FPGA复位
    gpio_set_value(g_fpga_data->rst_gpio_pin, !g_fpga_data->gpio_rst_polarity);
    msleep(10);
    gpio_set_value(g_fpga_data->rst_gpio_pin, g_fpga_data->gpio_rst_polarity);
    msleep(100);
    gpio_set_value(g_fpga_data->rst_gpio_pin, !g_fpga_data->gpio_rst_polarity);


    dev_info(dev, "Firmware update completed successfully\n");
    
    g_fpga_data->last_update_time = jiffies;
    
    return count;
}


static DEVICE_ATTR_RW(sw_gpio);
static DEVICE_ATTR_RW(firmware_name);
static DEVICE_ATTR_RW(rst_gpio);
static DEVICE_ATTR_RW(update_interval);
static DEVICE_ATTR_WO(firmware_update);
static DEVICE_ATTR_RO(firmware_md5);


static struct attribute *fpga_attrs[] = {
    &dev_attr_sw_gpio.attr,
    &dev_attr_firmware_name.attr,
    &dev_attr_rst_gpio.attr,
    &dev_attr_update_interval.attr,
    &dev_attr_firmware_update.attr,
    &dev_attr_firmware_md5.attr,
    NULL,
};

static const struct attribute_group fpga_attr_group = {
    .attrs = fpga_attrs,
};



static int fpga_load_probe(struct platform_device *pdev)
{
    struct fpga_load_data *data;
    int ret;

    dev_info(&pdev->dev, "FPGA load driver probing\n");

    data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    data->pdev = pdev;
    g_fpga_data = data;
    platform_set_drvdata(pdev, data);
    
    strcpy(data->firmware_name, "fpga_top.sfc");
    
    data->last_update_time = 0;
    data->min_update_interval = 10;
    
    // 解析设备树GPIO配置
    ret = parse_gpio_from_dt(pdev, data);
    if (ret) {
        dev_err(&pdev->dev, "Failed to parse GPIO from device tree: %d\n", ret);
        return ret;
    }
    
    // 先创建sysfs接口
    ret = create_sysfs_interface(data);
    if (ret) {
        dev_err(&pdev->dev, "Failed to create sysfs interface: %d\n", ret);
        goto err_free_gpio;
    }
    
    return 0;

err_free_gpio:
    if (gpio_is_valid(data->sw_gpio_pin)) {
        gpio_free(data->sw_gpio_pin);
    }
    if (gpio_is_valid(data->rst_gpio_pin)) {
        gpio_free(data->rst_gpio_pin);
    }
    g_fpga_data = NULL;
    return ret;
}

// 平台驱动remove函数
static int fpga_load_remove(struct platform_device *pdev)
{
    struct fpga_load_data *data = platform_get_drvdata(pdev);

    if (data) {
        destroy_sysfs_interface(data);
    }

    if (mtd_dev) {
        put_mtd_device(mtd_dev);
        mtd_dev = NULL;
    }

    if (data && gpio_is_valid(data->sw_gpio_pin)) {
        gpio_set_value(data->sw_gpio_pin, !data->gpio_sw_polarity);
        gpio_free(data->sw_gpio_pin);
        dev_info(&pdev->dev, "Flash SW GPIO %d reset to inactive and freed\n", data->sw_gpio_pin);
    }
    
    if (data && gpio_is_valid(data->rst_gpio_pin)) {
        gpio_set_value(data->rst_gpio_pin, !data->gpio_rst_polarity); 
        gpio_free(data->rst_gpio_pin);
        dev_info(&pdev->dev, "FPGA RST GPIO %d reset to inactive and freed\n", data->rst_gpio_pin);
    }

    g_fpga_data = NULL;
    dev_info(&pdev->dev, "FPGA load driver removed\n");

    return 0;
}

// 设备树匹配表
static const struct of_device_id fpga_load_of_match[] = {
    { .compatible = "hzhy,fpga_load" },
    { }
};
MODULE_DEVICE_TABLE(of, fpga_load_of_match);

// 平台驱动结构
static struct platform_driver fpga_load_driver = {
    .probe = fpga_load_probe,
    .remove = fpga_load_remove,
    .driver = {
        .name = "hzhy-fpga-load",
        .of_match_table = fpga_load_of_match,
    },
};

module_platform_driver(fpga_load_driver);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("mawenxuan_xa@hzhytech.com");
MODULE_DESCRIPTION("FPGA Firmware Updater for SPI Flash");
