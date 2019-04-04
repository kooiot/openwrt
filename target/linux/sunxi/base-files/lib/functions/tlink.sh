#!/bin/sh

export_tlink_efuse_mac() {
	local var="$1"
	local offset="$2"
	local nvmem_file="/sys/devices/platform/soc/1c14000.eeprom/sunxi-sid0/nvmem"
	local mac_val oem nv1 mac_base

	[ -z "$var" -o -z "$offset" ] && return -1
	[ -b ${nvmem_file} ] && return -2

	sid=$( hexdump -v -n 24 -e '/1 "%02x"' ${nvmem_file} )

	oem="${sid:32:8}"
	oem=$((0x$oem))

	nv1="${sid:40:8}"
	nv1=$((0x${nv1}))

	mac_base=${nv1}
	if [ $mac_base -eq 0 ]; then
		mac_base=${oem}
	fi

	mac_val=$(printf '%08x' $(( ${mac_base} + ${offset} )))
	mac_val="1E:4B:${mac_val:0:2}:${mac_val:2:2}:${mac_val:4:2}:${mac_val:6:2}"

	export "$var=${mac_val}"
	return 0
}

#test() {
#	local mac
#	read_tlink_efuse_mac mac 1
#	echo "MAC ${mac}"
#}
#
#test
