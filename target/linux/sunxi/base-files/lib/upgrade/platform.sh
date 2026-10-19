REQUIRE_IMAGE_METADATA=1

RAMFS_COPY_BIN='fw_printenv fw_setenv'
RAMFS_COPY_DATA='/etc/fw_env.config /var/lock/fw_printenv.lock'

tlink_get_type_magic() {
	local skip_base=8236
	local skip_offset=$(($1+$skip_base+1))
	local name_len=$(($3-7))
	get_image "$2" | dd bs=1 count=$name_len skip=$skip_offset 2>/dev/null | hexdump -v -n $name_len -e '/1 "%c"'
}

tlink_check_image() {
	local img_arch=$1
	local cur_name=$(board_name)
	local skip_offset=${#img_arch}
	local name_len=${#cur_name}
	local typemagic="$(tlink_get_type_magic $skip_offset "$2" $name_len)"
	[ "kooiot,${typemagic}" != "$(board_name)" ] && {
		echo "Invalid image, bad type: $typemagic"
		return 1
	}
	return 0
}

platform_check_image() {
	case "$(board_name)" in
		"kooiot,tlink-e2-spinand"|\
		"kooiot,tlink-ex-spinand"|\
		"kooiot,tlink-dly-e102-spinand")
			nand_do_platform_check "$(board_name)" "$1"
			return $?
			;;
	esac

	case "$(board_name)" in
	"kooiot,tlink-x1"|\
	"kooiot,tlink-x1s"|\
	"kooiot,tlink-x2"|\
	"kooiot,tlink-r1")
		tlink_check_image "sun8i-h3" "$1" && return 0
		return 1
		;;
	"kooiot,tlink-ok-a40i"|\
	"kooiot,tlink-dj-a40i-e"|\
	"kooiot,tlink-dr4-a40i"|\
	"kooiot,tlink-nano-a40i"|\
	"kooiot,tlink-qh-x40"|\
	"kooiot,tlink-k1"|\
	"kooiot,tlink-k2"|\
	"kooiot,tlink-k2x"|\
	"kooiot,tlink-k4a"|\
	"kooiot,tlink-k4g")
		tlink_check_image "sun8i-r40" "$1" && return 0
		return 1
		;;
	"kooiot,tlink-m408"|\
	"kooiot,tlink-m416")
		tlink_check_image "sun8i-t3" "$1" && return 0
		return 1
		;;
	"kooiot,tlink-s1"|\
	"sipeed,lichee-zero-plus")
		tlink_check_image "sun8i-s3" "$1" && return 0
		return 1
		;;
	"kooiot,tlink-x3")
		tlink_check_image "sun8i-x3" "$1" && return 0
		return 1
		;;
	"kooiot,tlink-e1-v0"|\
	"kooiot,tlink-e1"|\
	"kooiot,tlink-e2"|\
	"kooiot,tlink-ex"|\
	"kooiot,tlink-dly-e102"|\
	"kooiot,tlink-dly-e204")
		tlink_check_image "sun8i-t113s" "$1" && return 0
		return 1
		;;
	*)
		return 0
		;;
	esac

	legacy_sdcard_check_image "$1"
}

platform_kooiot_pre_upgrade() {
	if [ -f /etc/init.d/symlink ]; then
		echo "Wait SymLink App quited..."
		sleep 10
	fi

	case "$(rootfs_type)" in
		"overlay")
			echo "Erasing overlay....."
			umount /overlay
			dd if=/dev/zero of=/dev/loop0 bs=1M count=1
			;;
	esac
}

platform_pre_upgrade() {
	case "$(board_name)" in
	"sipeed,lichee-zero-plus"|\
	"kooiot,tlink-x1"|\
	"kooiot,tlink-x1s"|\
	"kooiot,tlink-x2"|\
	"kooiot,tlink-x3"|\
	"kooiot,tlink-k1"|\
	"kooiot,tlink-k2"|\
	"kooiot,tlink-k2x"|\
	"kooiot,tlink-k4a"|\
	"kooiot,tlink-k4g"|\
	"kooiot,tlink-s1"|\
	"kooiot,tlink-e1-v0"|\
	"kooiot,tlink-e1"|\
	"kooiot,tlink-e2"|\
	"kooiot,tlink-e2-spinand"|\
	"kooiot,tlink-ex"|\
	"kooiot,tlink-ex-spinand"|\
	"kooiot,tlink-ok-a40i"|\
	"kooiot,tlink-dj-a40i-e"|\
	"kooiot,tlink-dr4-a40i"|\
	"kooiot,tlink-nano-a40i"|\
	"kooiot,tlink-qh-x40"|\
	"kooiot,tlink-m408"|\
	"kooiot,tlink-m416"|\
	"kooiot,tlink-dly-e102"|\
	"kooiot,tlink-dly-e102-spinand"|\
	"kooiot,tlink-dly-e204"|\
	"kooiot,tlink-r1")
		platform_kooiot_pre_upgrade
		;;
	esac
}

platform_copy_config() {
	local partdev

	case "$(board_name)" in
		"kooiot,tlink-e2-spinand"|\
		"kooiot,tlink-ex-spinand"|\
		"kooiot,tlink-dly-e102-spinand")
			echo "Backup configuration is not supported!!!!"
			return 0;
			;;
	esac

	if export_partdevice partdev 1; then
		mount -o rw,noatime "/dev/$partdev" /mnt
		cp -af "$UPGRADE_BACKUP" "/mnt/$BACKUP_FILE"
		umount /mnt
	fi
}

platform_do_upgrade() {
	case "$(board_name)" in
		"kooiot,tlink-e2-spinand"|\
		"kooiot,tlink-ex-spinand"|\
		"kooiot,tlink-dly-e102-spinand")
			echo "Do SPI-NAND sysupgrade!!!"
			CI_KERNPART="kernel"
			CI_UBIPART="ubi"
			nand_do_upgrade "$1"
			return $?
			;;
	esac

	legacy_sdcard_do_upgrade "$1"
}
