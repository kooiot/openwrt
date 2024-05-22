#!/bin/sh

do_product_sn_kooiot_nvmem() {
	NVMEM_PATH="/sys/bus/i2c/devices/0-0050/eeprom"
	if [ -b "${NVMEM_PATH}" -o -f "${NVMEM_PATH}" ]; then
		echo "old eeprom path"
	else
		NVMEM_PATH="/sys/bus/nvmem/devices/0-00501/nvmem"
	fi

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

do_product_sn_kooiot_spi_flash() {
	local spi_part="/dev/unknown"
	local offset=65504		#0xFFE0
	local offset_dp=65520	#0xFFF0

	spi_part=$(find_mtd_part "u-boot-env")

	if [ -b "${spi_part}" -o -f "${spi_part}" ]; then

		product_sn=$(dd if=${spi_part} \
			bs=1 count=16 skip=${offset} 2>/dev/null | \
			hexdump -v -e '/1 "%c"' 2>/dev/null)
		product_sn_dp=$(dd if=${spi_part} \
			bs=1 count=16 skip=${offset_dp} 2>/dev/null | \
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
	NVMEM_PATH="/sys/bus/i2c/devices/0-0050/eeprom"
	if [ -b "${NVMEM_PATH}" -o -f "${NVMEM_PATH}" ]; then
		do_product_sn_kooiot_nvmem
	else
		do_product_sn_kooiot_spi_flash
	fi
}

do_kooiot_tlink_generic() {
	. /lib/functions.sh

	case "$(board_name)" in
	"widora,neo-16m" | \
	"widora,neo-32m" | \
	"kooiot,tlink-c1-16m" | \
	"kooiot,tlink-c1-32m" | \
	"kooiot,tlink-c2-16m" | \
	"kooiot,tlink-c2-32m" | \
	"kooiot,tlink-c2-v2" | \
	"kooiot,tlink-c3-16m" | \
	"kooiot,tlink-c3-32m" | \
	"kooiot,tlink-c35-16m" | \
	"kooiot,tlink-c35-32m" | \
	"kooiot,tlink-c4-16m" | \
	"kooiot,tlink-c4-32m")
		do_product_sn_kooiot
		;;
	esac

}

boot_hook_add preinit_main do_kooiot_tlink_generic
