/*
 * drivers/misc/awmem/awmem.c
 *
 * Copyright (c) 2022-2027 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "awmem.h"

#define AWMEM_DRIVER_NAME "awmem"
struct awmem_priv *awmem_glo;

static struct awmem_buffer *awmem_alloc_impl(struct awmem_priv *awmem, size_t size, int id)
{
	unsigned long offset, viraddr;
	struct vm_area_struct *vma;
	struct awmem_buffer *buffer;

	buffer = kzalloc(sizeof(struct awmem_buffer), GFP_KERNEL);
	if (!buffer) {
		pr_err("%s alloc buffer failed!\n", __func__);
		return NULL;
	}
	offset = gen_pool_alloc(awmem->pool, size);
	if (!offset) {
		pr_err("memory alloc failed, please check the remain space\n");
		kfree(buffer);
		return NULL;
	}
	viraddr = vm_mmap(NULL, 0, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_NORESERVE, 0);
	vma = find_vma(current->mm, viraddr);
	remap_pfn_range(vma, vma->vm_start, __phys_to_pfn(offset), size, vma->vm_page_prot);
	buffer->viraddr = (void *)viraddr;
	buffer->phys_addr = offset;
	buffer->phys_ptr = (void *)(buffer->phys_addr);
	buffer->size = size;
	buffer->id = idr_alloc(&awmem->idr, buffer->phys_ptr, id, id + 1, GFP_KERNEL);
	if (buffer->id < 0) {
		pr_err("%s idr_alloc failed\n", __func__);
		kfree(buffer);
		return NULL;
	}
	pr_info("[%s] iommu map buffer%d:0x%08x\n", __func__, buffer->id, buffer->phys_addr);
	iommu_map(awmem->domain, buffer->phys_addr, buffer->phys_addr, AWMEM_ALIGN(buffer->size, AWMEM_ALIGN_SIZE), IOMMU_READ | IOMMU_WRITE);
	list_add(&buffer->list, &awmem->list);
	buffer->count = 1;
	pr_info("[%s] alloc buffer phys addr:0x%08x, vir addr:0x%08x, id:%d, size:0x%x\n", __func__, buffer->phys_addr, buffer->viraddr, buffer->id, buffer->size);

	return buffer;
}

static int awmem_alloc(struct awmem_priv *awmem, struct awmem_buf_request *request)
{
	struct awmem_buffer *buffer, *tmp_buf;
	void *ptr;
	unsigned long new_viraddr;
	struct vm_area_struct *new_vma;
	unsigned long tmp_phys_addr;

	if (request->id < 0) {
		pr_err("invalid request id\n");
		return -EFAULT;
	}
	ptr = idr_find(&awmem->idr, request->id);
	if (ptr) {
		list_for_each_entry(tmp_buf, &awmem->list, list) {
			if (tmp_buf->id == request->id) {
				if (tmp_buf->size != request->size) {
					pr_err("size not match! request size:0x%x, id match buffer size:0x%x\n", request->size, tmp_buf->size);
					request->status = AWMEM_REQUEST_FAILURE;
					return -EFAULT;
				}
				tmp_buf->count++;
			}
		}
		tmp_phys_addr = (unsigned long)ptr;
		new_viraddr = vm_mmap(NULL, 0, request->size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_NORESERVE, 0);
		new_vma = find_vma(current->mm, new_viraddr);
		remap_pfn_range(new_vma, new_vma->vm_start, __phys_to_pfn(tmp_phys_addr), request->size, new_vma->vm_page_prot);
		request->addr = new_viraddr;
		request->status = AWMEM_REQUEST_SUCCESS;
	} else {
		buffer = awmem_alloc_impl(awmem, request->size, request->id);
		if (!buffer) {
			pr_err("alloc implementaion failed\n");
			request->status = AWMEM_ALLOC_FAILURE;
			return -EFAULT;
		}
		request->addr = buffer->viraddr;
		request->status = AWMEM_ALLOC_SUCCESS;
	}

	return 0;
}

static void awmem_free(struct awmem_priv *awmem, struct awmem_buf_free_arg *free_arg)
{
	struct awmem_buffer *buffer, *tmp_buf;
	void *ptr;

	ptr = idr_find(&awmem->idr, free_arg->id);
	if (ptr) {
		list_for_each_entry(tmp_buf, &awmem->list, list) {
			if (tmp_buf->id == free_arg->id) {
				buffer = tmp_buf;
			}
		}
		if (!buffer) {
			pr_err("cannot find match buffer, failed to free\n");
			return;
		}
		if (buffer->count <= 0) {
			pr_err("buffer%d used count is is %d, cannot be free\n", buffer->count);
			return;
		} else {
			if (free_arg->viraddr) {
				pr_info("[%s] ummap viraddr is 0x%lx\n", __func__, free_arg->viraddr);
				vm_munmap((unsigned long)(free_arg->viraddr), buffer->size);
			} else {
				pr_err("no valid viraddr, failed to free\n");
				return;
			}
			buffer->count--;
			if (buffer->count == 0) {
				/* implement free operation when count is zero*/
				pr_info("[%s] buffer%d count is zero, implementing buffer free\n", __func__, buffer->id);
				iommu_unmap(awmem->domain, buffer->phys_addr, AWMEM_ALIGN(buffer->size, AWMEM_ALIGN_SIZE));
				list_del(&buffer->list);
				idr_remove(&awmem->idr, buffer->id);
				gen_pool_free(awmem->pool, buffer->phys_addr, buffer->size);
				kfree(buffer);
			}
		}
	} else {
		pr_err("no vailable buffer with id %d, failed to free buffer\n", free_arg->id);
	}
}

static int awmem_get_phys_addr(struct awmem_priv *awmem, struct awmem_get_phys_addr_arg *phys_addr_arg)
{
	void *ptr;
	unsigned long phys_addr;
	struct awmem_buffer *tmp_buf;

	if (phys_addr_arg->virt < phys_addr_arg->virt_start) {
		pr_err("[%s] invalid argument, virt:0x%08x, virt_start:0x%08x\n", __func__, phys_addr_arg->virt, phys_addr_arg->virt_start);
		return -EFAULT;
	}

	ptr = idr_find(&awmem->idr, phys_addr_arg->id);
	if (ptr) {
		list_for_each_entry(tmp_buf, &awmem->list, list) {
			if (tmp_buf->id == phys_addr_arg->id) {
				if ((phys_addr_arg->virt - phys_addr_arg->virt_start + 1) > tmp_buf->size) {
					pr_err("[%s] request physical address out of bounds, virt:0x%08x, virt_start:0x%08x, buffer%d:0x%08x, size:0x%08x\n", __func__, phys_addr_arg->virt, phys_addr_arg->virt_start, phys_addr_arg->id, tmp_buf->phys_addr, tmp_buf->size);
					return -EFAULT;
				}
			}
		}
		phys_addr_arg->phys_addr = (unsigned long)ptr + (phys_addr_arg->virt - phys_addr_arg->virt_start);
		/*pr_info("[%s] find phys addr:0x%x for buffer%d, virt:0x%08x, virt_start:0x%08x\n", __func__, phys_addr_arg->phys_addr, phys_addr_arg->id, phys_addr_arg->virt, phys_addr_arg->virt_start);*/
	} else {
		pr_err("%s cannot match buffer for id %d\n", __func__, phys_addr_arg->id);
		return -EFAULT;
	}

	return 0;
}

static int awmem_flush_range(struct awmem_priv *awmem, struct awmem_flush_range_arg *flush_arg)
{
	void *ptr_start, *ptr_end;

	ptr_start = (void *)flush_arg->start_addr;
	ptr_end = (void *)(flush_arg->start_addr + flush_arg->size - 1);

    /*pr_info("[%s] start:%p, size:0x%x start\n", __func__, ptr_start, flush_arg->size);*/
#ifdef CONFIG_ARM64
	dcache_clean_inval_poc((unsigned long)ptr_start, (unsigned long)(ptr_end));
#else
	dmac_flush_range(ptr_start, ptr_end);
#endif
	/*pr_info("[%s]over\n", __func__);*/

	return 0;
}

static int awmem_open(struct inode *inode, struct file *file)
{
	file->private_data = awmem_glo;

	return 0;
}

static int awmem_release(struct inode *inode, struct file *file)
{
	struct awmem_priv *awmem;
	struct awmem_buffer *tmp_buf, *buffer;

	awmem = file->private_data;
	list_for_each_entry_safe(tmp_buf, buffer, &awmem->list, list) {
		pr_info("[%s] force to free buffer%d:0x%08x\n", __func__, tmp_buf->id, tmp_buf->phys_addr);
		iommu_unmap(awmem->domain, tmp_buf->phys_addr, AWMEM_ALIGN(tmp_buf->size, AWMEM_ALIGN_SIZE));
		idr_remove(&awmem->idr, tmp_buf->id);
		gen_pool_free(awmem->pool, tmp_buf->phys_addr, tmp_buf->size);
		list_del(&tmp_buf->list);
		kfree(tmp_buf);
	}

	return 0;
}

static ssize_t awmem_write(struct file *file, const char __user *buf, size_t size, loff_t *ppos)
{
	return 0;
}

static long awmem_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct awmem_priv *awmem;
	struct awmem_buf_request request;
	struct awmem_buf_free_arg free_arg;
	struct awmem_get_phys_addr_arg phys_addr_arg;
	struct awmem_flush_range_arg flush_arg;
	int ret;

	awmem = filp->private_data;
	switch (cmd) {
	case CMD_AWMEM_ALLOC:
		memset(&request, 0, sizeof(struct awmem_buf_request));
		if (copy_from_user(&request, (struct awmem_buf_request *)arg, sizeof(struct awmem_buf_request)) != 0) {
			pr_err("get request arg failed\n");
			return -EFAULT;
		}

		ret = awmem_alloc(awmem, &request);
		if (ret) {
			pr_err("AWMEM_ALLOC ioctl failed\n");
			return -EFAULT;
		}

		if (copy_to_user(arg, (void *)&request, sizeof(struct awmem_buf_request)) != 0) {
			return -EFAULT;
		}
		break;
	case CMD_AWMEM_FREE:
		memset(&free_arg, 0, sizeof(struct awmem_buf_free_arg));
		if (copy_from_user(&free_arg, (struct awmem_buf_free_arg *)arg, sizeof(struct awmem_buf_free_arg)) != 0) {
			pr_err("get free arg failed\n");
			return -EFAULT;
		}
		awmem_free(awmem, &free_arg);
		break;
	case CMD_AWMEM_GET_PHYS_ADDR:
		memset(&phys_addr_arg, 0, sizeof(struct awmem_get_phys_addr_arg));
		if (copy_from_user(&phys_addr_arg, (struct awmem_get_phys_addr_arg *)arg, sizeof(struct awmem_get_phys_addr_arg)) != 0) {
			pr_err("get phys addr id failed\n");
			return -EFAULT;
		}
		ret = awmem_get_phys_addr(awmem, &phys_addr_arg);
		if (ret) {
			pr_err("CMD_AWMEM_GET_PHYS_ADDR ioctl failed\n");
			return -EFAULT;
		}
		if (copy_to_user(arg, (void *)&phys_addr_arg, sizeof(struct awmem_get_phys_addr_arg)) != 0) {
			return -EFAULT;
		}
		break;
	case CMD_AWMEM_FLUSH_RANGE:
		memset(&flush_arg, 0, sizeof(struct awmem_flush_range_arg));
		if (copy_from_user(&flush_arg, (struct awmem_flush_range_arg *)arg, sizeof(struct awmem_flush_range_arg)) != 0) {
			pr_err("get flush id failed\n");
			return -EFAULT;
		}
		ret = awmem_flush_range(awmem, &flush_arg);
		if (ret) {
			pr_err("CMD_AWMEM_FLUSH_RANGE ioctl failed\n");
			return -EFAULT;
		}
		break;
	default:
		break;
	}

	return 0;
}

static const struct file_operations awmem_fops = {
	.owner = THIS_MODULE,
	.write = awmem_write,
	.release = awmem_release,
	.open = awmem_open,
	.unlocked_ioctl = awmem_ioctl
};

static struct miscdevice awmem_miscdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = AWMEM_DRIVER_NAME,
	.fops = &awmem_fops,
};

static int awmem_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct device_node *tmp_np;
	struct awmem_priv *awmem;
	struct resource res;
	int ret;

	awmem = devm_kzalloc(dev, sizeof(struct awmem_priv), GFP_KERNEL);
	if (!awmem) {
		dev_err(dev, "kzalloc for struct awmem failed\n");
		ret = -ENOMEM;
		goto err_out;
	}
	awmem->pool = gen_pool_create(PAGE_SHIFT, -1);
    if (!awmem->pool) {
		dev_err(dev, "failed to create gen pool\n");
		ret = -EFAULT;
		goto err_free_awmem;
	}

	tmp_np = of_parse_phandle(np, "memory-region", 0);
	if (tmp_np) {
		ret = of_address_to_resource(tmp_np, 0, &res);
		if (ret) {
			dev_err(dev, "failed to parse reserve memory node\n");
			ret = -EFAULT;
			goto err_destroy_gen_pool;
		}
		awmem->pool_start = res.start;
		awmem->pool_length = (res.end - res.start + 1);
		pr_info("[%s]  get gen pool start:0x%x, length:0x%x\n", __func__, awmem->pool_start, awmem->pool_length);
	}

	ret = gen_pool_add_virt(awmem->pool, awmem->pool_start, awmem->pool_start, awmem->pool_length, -1);
	if (ret) {
		dev_err(dev, "add gen pool failed\n");
		ret = -EFAULT;
		goto err_destroy_gen_pool;
	}

	awmem->domain = iommu_domain_alloc(dev->bus);
	if (!awmem->domain) {
		pr_err("awmem alloc domain failed!\n");
		return -EFAULT;
	}

	awmem->domain->type = IOMMU_DOMAIN_DMA;

	INIT_LIST_HEAD(&awmem->list);
	idr_init(&awmem->idr);
	dev_set_drvdata(dev, awmem);
	awmem_glo = awmem;

	ret = misc_register(&awmem_miscdev);
	if (ret) {
		dev_err(dev, "%s: cannot register miscdev on minor=%d (%d)\n",
				__func__, MISC_DYNAMIC_MINOR, ret);
		ret = -EFAULT;
		goto err_destroy_idr;
	}

	return 0;
err_destroy_idr:
	idr_destroy(&awmem->idr);
err_destroy_gen_pool:
	gen_pool_destroy(awmem->pool);
err_free_awmem:
	devm_kfree(dev, awmem);
err_out:
	return ret;
}

static int awmem_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct awmem_priv *awmem = dev_get_drvdata(dev);

	misc_deregister(&awmem_miscdev);
	idr_destroy(&awmem->idr);
	gen_pool_destroy(awmem->pool);
	devm_kfree(dev, awmem);

	return 0;
}

static const struct of_device_id awmem_match[] = {
	{ .compatible = "awmem", },
	{ },
};
MODULE_DEVICE_TABLE(of, awmem_match);

static struct platform_driver awmem_driver = {
	.driver = {
		.name	= AWMEM_DRIVER_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = awmem_match,
	},
	.probe  = awmem_probe,
	.remove = awmem_remove,
};

static int awmem_init(void)
{
	platform_driver_register(&awmem_driver);

	return 0;
}

static void awmem_exit(void)
{
	platform_driver_unregister(&awmem_driver);
}

module_init(awmem_init);
module_exit(awmem_exit);

MODULE_DESCRIPTION("allwinner memory alloc driver");
MODULE_AUTHOR("allwinner");
MODULE_LICENSE("GPL");
