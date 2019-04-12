do_product_sn_kooiot() {
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

		[ -e /tmp/sysinfo/cloud] || \
			echo "cloud.thingsroot.com" > /tmp/sysinfo/cloud
	fi
}

do_kooiot_freeioe_generic() {
	. /lib/functions.sh

	case "$(board_name)" in
	"kooiot,tlink-x1"|\
	"kooiot,tlink-r1")
		do_product_sn_kooiot
		;;
	esac

}

boot_hook_add preinit_main do_kooiot_freeioe_generic
