#!/bin/sh

export_tlink_efuse_mac() {
	local var="$1"
	local offset="$2"
	local nvmem_file="/sys/bus/nvmem/devices/sunxi-sid0/nvmem"
	local mac_val oem nv1 mac_base mac1 mac2 sid chip0 chip3

	[ -z "$var" -o -z "$offset" ] && return -1
	[ -b ${nvmem_file} ] && return -2

	sid=$( hexdump -v -n 24 -e '/1 "%02x"' ${nvmem_file} )

	chip0="${sid:6:2}"
	chip0=$((0x${offset}2${chip0}))

	chip3="${sid:24:8}"
	chip3=$((0x${chip3}))

	if [ $chip3 -eq 0 ]; then
		chip3=$((0x800000))
	fi

#	oem="${sid:32:8}"
#	oem=$((0x$oem))
#	nv1="${sid:40:8}"
#	nv1=$((0x${nv1}))

#	mac_base=${nv1}
#	if [ $mac_base -eq 0 ]; then
#		mac_base=${oem}
#	fi

#	mac_val=$(printf '%08x' $(( ${mac_base} + ${offset} )))
#	mac_val="1E:4B:${mac_val:0:2}:${mac_val:2:2}:${mac_val:4:2}:${mac_val:6:2}"

#	mac1=$(printf '%04x' ${chip0})
	mac1="B0C9"
	mac2=$(printf '%08x' ${chip3})
	mac_val="${mac1:0:2}:${mac1:2:2}:${mac2:0:2}:${mac2:2:2}:${mac2:4:2}:${mac2:6:2}"

	export "$var=${mac_val}"
	return 0
}

tlink_gen_mac_emmc_serial() {
	local offset="$1"
	local serial_file="/sys/class/block/mmcblk1/device/serial"
	local mac_base="b0c9"
	local mac_val mac0_num mac0_val uid mac0 mac1 mac2 mac3

	[ -z "$offset" ] && return 0
	[ -b ${serial_file} ] && return 0

	uid=$(cat ${serial_file})

	mac0="${uid:2:2}"
	mac1="${uid:4:2}"
	mac2="${uid:6:2}"
	mac3="${uid:8:2}"

	mac0_num=$(printf %d 0x${mac0})
	mac0_val=`expr ${mac0_num} + ${offset}`
	mac0_val=$(( ${mac0_val} % 256 ))
	mac0=$(printf '%02x' ${mac0_val})

	mac_val="${mac_base:0:2}:${mac_base:2:2}:${mac3}:${mac2}:${mac1}:${mac0}"

	echo "${mac_val}"
	return 0
}

gpio_out() {
	local gpio_pin
	local value

	gpio_pin="$1"
	value="$2"

	local gpio_path="/sys/class/gpio/gpio${gpio_pin}"
	# export GPIO pin for access
	[ -d "$gpio_path" ] || {
		echo "$gpio_pin" >/sys/class/gpio/export
		# we need to wait a bit until the GPIO appears
		[ -d "$gpio_path" ] || sleep 1
		echo out >"$gpio_path/direction"
	}
	# write 0 or 1 to the "value" field
	{ [ "$value" = "0" ] && echo "0" || echo "1"; } >"$gpio_path/value"
}

gpio_in() {
	local gpio_pin

	gpio_pin="$1"

	local gpio_path="/sys/class/gpio/gpio${gpio_pin}"
	# export GPIO pin for access
	[ -d "$gpio_path" ] || {
		echo "$gpio_pin" >/sys/class/gpio/export
		# we need to wait a bit until the GPIO appears
		[ -d "$gpio_path" ] || sleep 1
		echo in >"$gpio_path/direction"
	}
	# read the "value" field
	return $(cat $gpio_path/value)
}

product_sn() {
	[ -e /tmp/sysinfo/product_sn ] && cat /tmp/sysinfo/product_sn || echo "UNKNOWN_DEVICE_SN"
}

ioe_cloud() {
	[ -e /tmp/sysinfo/cloud ] && cat /tmp/sysinfo/cloud || echo "iot.kooiot.com"
}

avoid_empty_passwd() {
	if ( grep -qs '^root::' /etc/shadow && \
		 [ -z "$FAILSAFE" ] )
	then
		local default_pw='root:$1$pMlMRXYW$H06ycPKhVzpVfSsy2zc1P0:17997'
		sed -i "s/^root::0:/${default_pw}/g" /etc/shadow
cat << EOF
=== WARNING! =====================================
The default root password created on this device!
--------------------------------------------------
EOF
	fi
}


#test() {
#	local mac
#	export_tlink_efuse_mac mac 1
#	echo "MAC ${mac}"
#}
#
#test
