#!/bin/sh

. /lib/functions.sh

SHOW=$1
CSQ=$2

LED_PATH="/sys/class/leds/"
LED_ORG_PATH="/sys/class/leds/kooiot:"

tlink_led_echo()
{
	local name=$1
	local value=$2

	if [ -e ${LED_ORG_PATH}${name}/brightness ]
	then
		echo ${value} > ${LED_ORG_PATH}${name}/brightness
	fi

	if [ -e ${LED_PATH}${name}/brightness ]
	then
		echo ${value} > ${LED_PATH}${name}/brightness
	fi
}

tlink_csq_leds_default()
{
	if [ ${SHOW} -le 0 ]
	then
		tlink_led_echo "green:bbs" 0
		tlink_led_echo "green:bs" 0
		tlink_led_echo "green:gs" 0
		tlink_led_echo "green:ggs" 0
		return 0
	fi

	if [ $CSQ -ge 17 ]
	then
		tlink_led_echo "green:bbs" 255
		tlink_led_echo "green:bs" 255
		tlink_led_echo "green:gs" 255
		if [ $CSQ -ge 26 ]
		then
			tlink_led_echo "green:ggs" 255
		else
			tlink_led_echo "green:ggs" 0
		fi
	else
		tlink_led_echo "green:ggs" 0
		tlink_led_echo "green:gs" 0
		tlink_led_echo "green:bbs" 255
		if [ $CSQ -ge 10 ]
		then
			tlink_led_echo "green:bs" 255
		else
			tlink_led_echo "green:bs" 0
		fi
	fi
}

tlink_csq_leds_mixed()
{
	if [ ${SHOW} -le 0 ]
	then
		tlink_led_echo "green:gs" 0
		tlink_led_echo "red:bs" 0
		return 0
	fi

	if [ $CSQ -ge 17 ]
	then
		tlink_led_echo "green:gs" 255
		tlink_led_echo "red:bs" 0
	else
		tlink_led_echo "red:bs" 255
		if [ $CSQ -ge 10 ]
		then
			tlink_led_echo "green:gs" 255
		else
			tlink_led_echo "green:bs" 0
		fi
	fi
}


tlink_csq_leds_single()
{
	if [ ${SHOW} -le 0 ]
	then
		echo "none" > /sys/class/leds/kooiot:green:csq/trigger
		echo 0 > /sys/class/leds/kooiot:green:csq/brightness
		return 0
	fi

	if [ $CSQ -ge 17 ]
	then
		echo "none" > /sys/class/leds/kooiot:green:csq/trigger
		echo 255 > /sys/class/leds/kooiot:green:csq/brightness
	else
		echo "timer" > /sys/class/leds/kooiot:green:csq/trigger
		echo 500 > /sys/class/leds/kooiot:green:csq/delay_on
		if [ $CSQ -ge 10 ]
		then
			echo 500 > /sys/class/leds/kooiot:green:csq/delay_off
		else
			echo 2000 > /sys/class/leds/kooiot:green:csq/delay_off
		fi
	fi
}

case $(board_name) in
	kooiot,tlink-m408|\
	kooiot,tlink-m416)
		tlink_csq_leds_mixed
		;;
	kooiot,tlink-k2|\
	kooiot,tlink-k2x|\
	kooiot,tlink-k4x)
		tlink_csq_leds_single
		;;
	*)
		tlink_csq_leds_default
		;;
esac

exit 0
