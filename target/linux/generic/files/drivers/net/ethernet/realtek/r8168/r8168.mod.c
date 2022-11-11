#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

MODULE_INFO(intree, "Y");

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x8496402d, "module_layout" },
	{ 0x609f1c7e, "synchronize_net" },
	{ 0x3ce4ca6f, "disable_irq" },
	{ 0x2d3385d3, "system_wq" },
	{ 0xb6392789, "netdev_info" },
	{ 0x95b1413b, "pci_write_config_dword" },
	{ 0xeb233a45, "__kmalloc" },
	{ 0x9c1a098e, "ethtool_op_get_ts_info" },
	{ 0xb364c9a1, "__skb_gso_segment" },
	{ 0xa9301969, "pci_write_config_word" },
	{ 0xaa1a5147, "single_open" },
	{ 0x8ae4cd24, "param_ops_int" },
	{ 0x265303b5, "napi_disable" },
	{ 0x987e48d1, "pci_read_config_byte" },
	{ 0x37f5e79e, "napi_schedule_prep" },
	{ 0x8f996a30, "ethtool_convert_legacy_u32_to_link_mode" },
	{ 0x79aa04a2, "get_random_bytes" },
	{ 0xca07421e, "dma_set_mask" },
	{ 0xf1a83f7a, "dev_printk" },
	{ 0xeddb2ef3, "single_release" },
	{ 0x86e26ecd, "pci_get_slot" },
	{ 0xefd0960e, "seq_puts" },
	{ 0x5820db03, "pci_disable_device" },
	{ 0x5cb2527d, "netif_carrier_on" },
	{ 0x12a4e128, "__arch_copy_from_user" },
	{ 0x22b531b3, "delayed_work_timer_fn" },
	{ 0x7f995d6a, "seq_printf" },
	{ 0xd8ebe89b, "netif_carrier_off" },
	{ 0x56470118, "__warn_printk" },
	{ 0xe6c07f3a, "pci_write_config_byte" },
	{ 0xeeb8b154, "__dev_kfree_skb_any" },
	{ 0xeae3dfd6, "__const_udelay" },
	{ 0x77e74c61, "pci_release_regions" },
	{ 0xd7425285, "init_timer_key" },
	{ 0xe3389d48, "cancel_delayed_work_sync" },
	{ 0x22bf32bd, "pci_enable_wake" },
	{ 0xe57985fa, "dma_free_attrs" },
	{ 0xdd64e639, "strscpy" },
	{ 0x20f2cb64, "seq_read" },
	{ 0xe4e59303, "dma_set_coherent_mask" },
	{ 0x15ba50a6, "jiffies" },
	{ 0x4b252609, "__netdev_alloc_skb" },
	{ 0x4d65cbd5, "csum_ipv6_magic" },
	{ 0x49b533ba, "PDE_DATA" },
	{ 0x24c0cc7, "pci_set_master" },
	{ 0x7d29e492, "del_timer_sync" },
	{ 0xdcb764ad, "memset" },
	{ 0xd46f74b0, "dma_sync_single_for_cpu" },
	{ 0x449d262d, "proc_mkdir" },
	{ 0xce768c0c, "netif_tx_wake_queue" },
	{ 0xf74c9728, "pci_restore_state" },
	{ 0x4b0a3f52, "gic_nonsecure_priorities" },
	{ 0x24e0d9d9, "_raw_spin_unlock_irqrestore" },
	{ 0xc5850110, "printk" },
	{ 0x449ad0a7, "memcmp" },
	{ 0xc15b106f, "register_netdev" },
	{ 0xeb52af21, "seq_putc" },
	{ 0xdf989d8d, "pci_read_config_word" },
	{ 0x5792f848, "strlcpy" },
	{ 0xa5938501, "dma_alloc_attrs" },
	{ 0x69dd3b5b, "crc32_le" },
	{ 0x93838873, "proc_mkdir_data" },
	{ 0x89e101cf, "mod_timer" },
	{ 0x20085936, "netif_napi_add" },
	{ 0x92d5838e, "request_threaded_irq" },
	{ 0x6091797f, "synchronize_rcu" },
	{ 0x6b4b2933, "__ioremap" },
	{ 0x53dd2d44, "init_net" },
	{ 0x7d3a794b, "_dev_err" },
	{ 0xdd9e79dc, "pci_enable_msi" },
	{ 0x564ba4e1, "pci_clear_master" },
	{ 0xe523ad75, "synchronize_irq" },
	{ 0xac8e35c7, "pci_find_capability" },
	{ 0xead102cb, "pci_set_mwi" },
	{ 0x55e31703, "ethtool_convert_link_mode_to_legacy_u32" },
	{ 0xc6cbbc89, "capable" },
	{ 0xba00a596, "netif_device_attach" },
	{ 0x281ea14a, "napi_gro_receive" },
	{ 0x6e13e9e9, "_dev_info" },
	{ 0x14fdc5b6, "pci_disable_link_state" },
	{ 0xd3746946, "netif_device_detach" },
	{ 0x176f4557, "__napi_schedule" },
	{ 0x6cbbfc54, "__arch_copy_to_user" },
	{ 0x33c8e066, "queue_delayed_work_on" },
	{ 0x86332725, "__stack_chk_fail" },
	{ 0xd048aca, "skb_checksum_help" },
	{ 0xb9cbd6ad, "kfree_skb" },
	{ 0x8b9f70c7, "cpu_hwcaps" },
	{ 0xec68d0dd, "napi_complete_done" },
	{ 0x1769015f, "dma_map_page_attrs" },
	{ 0xe8858c3f, "__raw_spin_lock_init" },
	{ 0xb0df128d, "pci_read_config_dword" },
	{ 0x35d9a91e, "eth_type_trans" },
	{ 0x5bfa16f6, "proc_get_parent_data" },
	{ 0xec2fc692, "cpu_hwcap_keys" },
	{ 0x18bd32b5, "dev_driver_string" },
	{ 0x8b0e7fef, "pskb_expand_head" },
	{ 0xfaa05b85, "netdev_err" },
	{ 0x585daef7, "pci_unregister_driver" },
	{ 0x68a2c71a, "_raw_spin_lock_irqsave" },
	{ 0x52edd6a9, "__netif_napi_del" },
	{ 0xf6ebc03b, "net_ratelimit" },
	{ 0x17f4c67d, "pci_set_power_state" },
	{ 0x67b0c359, "remove_proc_subtree" },
	{ 0x65c6e4a1, "proc_create_data" },
	{ 0x73853064, "pci_clear_mwi" },
	{ 0xc5314756, "seq_lseek" },
	{ 0xfcec0987, "enable_irq" },
	{ 0x37a0cba, "kfree" },
	{ 0x4829a47e, "memcpy" },
	{ 0x1d1dee0f, "pci_request_regions" },
	{ 0xd5fd377a, "pci_disable_msi" },
	{ 0xaf56600a, "arm64_use_ng_mappings" },
	{ 0xedc03953, "iounmap" },
	{ 0xcafd787c, "dma_sync_single_for_device" },
	{ 0x6975fe3e, "__pci_register_driver" },
	{ 0x15af7f4, "system_state" },
	{ 0x78e9d398, "dma_unmap_page_attrs" },
	{ 0x940b1eb0, "unregister_netdev" },
	{ 0x1b6e97c1, "pci_choose_state" },
	{ 0xd82ffb7, "netdev_update_features" },
	{ 0x85670f1d, "rtnl_is_locked" },
	{ 0x92689d8d, "__napi_alloc_skb" },
	{ 0x3b148c11, "skb_tstamp_tx" },
	{ 0x8317c2cf, "skb_put" },
	{ 0xb12863f7, "pci_enable_device" },
	{ 0x98cecd07, "pci_wake_from_d3" },
	{ 0xb7fefbcc, "param_ops_ulong" },
	{ 0x22443395, "param_ops_uint" },
	{ 0x14b89635, "arm64_const_caps_ready" },
	{ 0x3aa46338, "skb_copy_bits" },
	{ 0x29b734ea, "__skb_pad" },
	{ 0x23f420f0, "device_set_wakeup_enable" },
	{ 0xc31db0ce, "is_vmalloc_addr" },
	{ 0xc1514a3b, "free_irq" },
	{ 0x9588d48e, "pci_save_state" },
	{ 0x2e042d6b, "alloc_etherdev_mqs" },
};

MODULE_INFO(depends, "");

MODULE_ALIAS("pci:v000010ECd00008168sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010ECd00008161sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010ECd00002502sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010ECd00002600sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00001186d00004300sv00001186sd00004B10bc*sc*i*");

MODULE_INFO(srcversion, "17BCF7A3B9B2AF984D91D03");
