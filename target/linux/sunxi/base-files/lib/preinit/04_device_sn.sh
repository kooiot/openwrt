do_device_sn_generic() {
	mmc_disk="/dev/mmcblk1"
	if [ -b "$mmc_disk" -o -f "$mmc_disk" ]; then

		device_sn=$(dd if=/dev/mmcblk1 \
			bs=1 count=8 skip=410112 2>/dev/null | \
			hexdump -v -n 32 -e '/1 "%02x"' 2>/dev/null)

		mkdir -p /tmp/sysinfo
		[ -e /tmp/sysinfo/device_sn ] || \
			echo ${device_sn} > /tmp/sysinfo/device_sn

		[ -e /tmp/sysinfo/cloud] || \
			echo "cloud.thingsroot.com" > /tmp/sysinfo/cloud
	fi
}

boot_hook_add preinit_main do_device_sn_generic
