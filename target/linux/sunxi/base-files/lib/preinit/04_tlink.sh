do_product_sn_kooiot_nvmem() {
	NVMEM_PATH="/sys/devices/platform/soc/1c2ac00.i2c/i2c-0/0-0050/0-00500/nvmem"
	if [ -b "${NVMEM_PATH}" -o -f "${NVMEM_PATH}" ]; then
		product_sn=$(dd if=${NVMEM_PATH} \
			bs=1 count=16 skip=0 2>/dev/null | \
			hexdump -v -e '/1 "%c"' 2>/dev/null)
		product_sn_dp=$(dd if=${NVMEM_PATH} \
			bs=1 count=16 skip=16 2>/dev/null | \
			hexdump -v -e '/1 "%c"' 2>/dev/null)

		mkdir -p /tmp/sysinfo

		if [ "${product_sn}"x = "${product_sn_dp}"x ]; then
			[ -e /tmp/sysinfo/product_sn ] || \
				echo ${product_sn} > /tmp/sysinfo/product_sn
		else
			[ -e /tmp/sysinfo/product_sn ] || \
				echo "UNKNOWN" > /tmp/sysinfo/product_sn
		fi

		[ -e /tmp/sysinfo/cloud ] || \
			echo "ioe.thingsroot.com" > /tmp/sysinfo/cloud

	fi
}

do_product_sn_kooiot_emmc() {
	mmc_disk="/dev/mmcblk1"
	if [ -b "$mmc_disk" -o -f "$mmc_disk" ]; then

		product_sn=$(dd if=/dev/mmcblk1 \
			bs=1 count=16 skip=410112 2>/dev/null | \
			hexdump -v -e '/1 "%c"' 2>/dev/null)
		product_sn_dp=$(dd if=/dev/mmcblk1 \
			bs=1 count=16 skip=410128 2>/dev/null | \
			hexdump -v -e '/1 "%c"' 2>/dev/null)

		mkdir -p /tmp/sysinfo

		if [ "${product_sn}"x = "${product_sn_dp}"x ]; then
			[ -e /tmp/sysinfo/product_sn ] || \
				echo ${product_sn} > /tmp/sysinfo/product_sn
		else
			[ -e /tmp/sysinfo/product_sn ] || \
				echo "UNKNOWN" > /tmp/sysinfo/product_sn
		fi

		[ -e /tmp/sysinfo/cloud ] || \
			echo "ioe.thingsroot.com" > /tmp/sysinfo/cloud
	fi
}

do_product_sn_kooiot() {
	NVMEM_PATH="/sys/devices/platform/soc/1c2ac00.i2c/i2c-0/0-0050/0-00500/nvmem"
	if [ -b "${NVMEM_PATH}" -o -f "${NVMEM_PATH}" ]; then
		do_product_sn_kooiot_nvmem
	else
		do_product_sn_kooiot_emmc
	fi
}

do_kooiot_tlink_generic() {
	. /lib/functions.sh

	case "$(board_name)" in
	"kooiot,tlink-x1"|\
	"kooiot,tlink-x2"|\
	"kooiot,tlink-r1")
		do_product_sn_kooiot
		;;
	esac

}

boot_hook_add preinit_main do_kooiot_tlink_generic