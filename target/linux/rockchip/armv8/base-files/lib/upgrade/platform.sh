REQUIRE_IMAGE_METADATA=1

tlink_get_type_magic() {
	# 0x2dcf3
	local skip_base=187635
	local skip_offset=$(($skip_base))
	local name_len=$2
	get_image_dd "$1" bs=1 count=$name_len skip=$skip_offset 2>/dev/null | hexdump -v -n $name_len -e '/1 "%c"'
}

tlink_check_image() {
	local cur_name=$(board_name)
	local cur_name_s=${cur_name:7}
	local name_len_s=${#cur_name_s}
	local typemagic="$(tlink_get_type_magic "$1" $name_len_s)"
	[ "${typemagic}"x != "${cur_name_s}"x ] && {
		echo "Invalid image, bad type:${typemagic}!=${cur_name_s}."
		return 1
	}
	return 0
}

platform_check_image() {
	case "$(board_name)" in
		"kooiot,tlink-r4x" | \
		"kooiot,tlink-r7")
			tlink_check_image "$1" && return 0
			return 1
			;;
		*)
			return 0
			;;
	esac

	legacy_sdcard_check_image "$1"
}

platform_copy_config() {
	local partdev

	if export_partdevice partdev 1; then
		mount -o rw,noatime "/dev/$partdev" /mnt
		cp -af "$UPGRADE_BACKUP" "/mnt/$BACKUP_FILE"
		umount /mnt
	fi
}

platform_pre_upgrade() {
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

platform_do_upgrade() {
	legacy_sdcard_do_upgrade "$1"
}
