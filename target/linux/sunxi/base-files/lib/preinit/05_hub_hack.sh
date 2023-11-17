do_tlink_m416_hack() {
	. /lib/functions.sh

	# hack: enable switch on Lamobo R1 and reset counters
	case $(board_name) in
	kooiot,tlink-m416)
		echo "M416 hub reset hack" > /dev/kmsg
		echo 1 > /sys/class/gpio/usb_reset/value
		sleep 1
		echo 0 > /sys/class/gpio/usb_reset/value
		;;
	esac
}

boot_hook_add preinit_main do_tlink_m416_hack
