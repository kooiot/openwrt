// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright 2025 allwnner
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>
#include <linux/smp.h>
#include <linux/arm-smccc.h>
#include <linux/cpu.h>
#include <linux/slab.h>
#include <linux/param.h>

#define SMC_GET_CACHE_REG 0x8000ff51
#define SMC_SET_CACHE_REG 0x8000ff52

#define PARTCR		0
#define THREADSID	1
#define VERSION		0XFF
#define WAY		4
#define ID		8

#define VERSION_KEY	0x19283746

static bool support_cache_partition;

static ssize_t register_read(struct kobject *kobj,
				struct kobj_attribute *attr,
				char *buf);
static ssize_t register_write(struct kobject *kobj,
				struct kobj_attribute *attr,
				const char *buf,
				size_t count);

static struct kobject *cache_partition;
static struct kobj_attribute partcr = __ATTR(val, 0664,
				register_read, register_write);
static int num_cpus;

static unsigned int l3_cache;
core_param(l3_cache, l3_cache, int, 0644);

static void get_registers(void *p)
{
	u64 *regs = p;
	struct arm_smccc_res smc_res;

	arm_smccc_smc(SMC_GET_CACHE_REG, *regs, 0, 0, 0, 0, 0, 0, &smc_res);

	*regs = smc_res.a0;
}

static ssize_t register_read(struct kobject *kobj,
				struct kobj_attribute *attr, char *buf)
{
	u64 val;
	int len;

	val = PARTCR;
	get_registers(&val);

	printk("L3 Cache val is 0x%llx \n", val);

	len = sprintf(buf, "0x%llX\n", val);

	return len;
}


static int check(void *p)
{
	int i;
	u64 *params = p;
	u32 chunk = 0, ret = 0;

	if (!params[1])
		return 0;

	if (params[1] >> num_cpus * WAY) {
		pr_err("error arg (%llx), please check \n", params[1]);
		return -1;
	}

	for (i = 0; i < num_cpus; i++) {
		chunk = (params[1] >> (WAY * i)) & ((1U << WAY) - 1);
		ret |= chunk;
	}

	if (ret != ((1u << WAY) - 1)) {
		pr_err("error arg (%llx), please check \n", params[1]);
		return -1;
	}

	return 0;
}

static void set_registers(void *p)
{
	struct arm_smccc_res res;
	u64 *params = p;

	if (check(p))
		return;

	printk("L3 Cache val is 0x%llx \n", params[1]);

	arm_smccc_smc(SMC_SET_CACHE_REG, params[0], params[1], 0, 0, 0, 0, 0, &res);
}

static ssize_t register_write(struct kobject *kobj, struct kobj_attribute *attr,
				const char *buf, size_t count)
{
	u64 params[2];

	params[0] = PARTCR;
	sscanf(buf, "%llX", &params[1]);

	set_registers(params);

	return count;
}


static int check_l3_cache_support(void)
{
	u64 val = VERSION;
	get_registers(&val);

	if (val == VERSION_KEY)
		return 0;
	else
		return -1;
}

static int __init sunxi_l3_cache_partition_init(void)
{
	int ret;
	u64 params[2];

	if (check_l3_cache_support()) {
		pr_warn("ATF no support L3 cache partition, please check or update\n");
		return 0;
	}

	pr_info("[%s] %s", __FILE__, __func__);

	support_cache_partition = true;

	params[0] = PARTCR;
	params[1] = l3_cache;

	num_cpus = num_possible_cpus();

	if (l3_cache)
		set_registers(params);

	cache_partition = kobject_create_and_add("cache_partition", kernel_kobj);
	if (!cache_partition) {
		pr_err("Failed to create kobject\n");
		return -ENOMEM;
	}

	ret = sysfs_create_file(cache_partition, &partcr.attr);
	if (ret) {
		kobject_put(cache_partition);
		pr_err("Failed to create sysfs partcr file\n");
		return ret;
	}

	pr_info("sunxi cache partition module initialized\n");

	return 0;
}

static void __exit sunxi_l3_cache_partition_exit(void)
{
	u64 params[2];

	if (!support_cache_partition)
		return;

	params[0] = PARTCR;
	params[1] = 0;

	set_registers(params);

	sysfs_remove_file(cache_partition, &partcr.attr);
	kobject_put(cache_partition);

	pr_info("sunxi cache partition module exited\n");
}

module_init(sunxi_l3_cache_partition_init);
module_exit(sunxi_l3_cache_partition_exit);

MODULE_DESCRIPTION("Cache partition registers access through sysfs");
MODULE_AUTHOR("allwinner");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.0");
