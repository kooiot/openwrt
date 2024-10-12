REQUIRE_IMAGE_METADATA=1

tlink_get_type_magic() {
	# 0x800943
	local skip_base=8390979
	local skip_offset=$(($skip_base))
	local name_len=$2
	get_image_dd "$1" bs=1 count=$name_len skip=$skip_offset 2>/dev/null | hexdump -v -n $name_len -e '/1 "%c"'
}

tlink_check_image() {
	local cur_name=$(board_name)
	local name_len=${#cur_name}
	local name_len_s=$((name_len-7))
	local cur_name_s=${cur_name:7}
	local typemagic="$(tlink_get_type_magic "$1" $name_len_s)"
	[ "${typemagic}"x != "${cur_name:7}"x ] && {
		echo "Invalid image, bad type:${typemagic}!=${cur_name:7}."
		return 1
	}
	return 0
}

platform_check_image() {
	local diskdev partdev diff

	export_bootdevice && export_partdevice diskdev 0 || {
		echo "Unable to determine upgrade device"
		return 1
	}

	get_partitions "/dev/$diskdev" bootdisk

	#extract the boot sector from the image
	get_image "$@" | dd of=/tmp/image.bs count=1 bs=512b 2>/dev/null

	get_partitions /tmp/image.bs image

	#compare tables
	diff="$(grep -F -x -v -f /tmp/partmap.bootdisk /tmp/partmap.image)"

	rm -f /tmp/image.bs /tmp/partmap.bootdisk /tmp/partmap.image

	if [ -n "$diff" ]; then
		echo "Partition layout has changed. Full image will be written."
		#ask_bool 0 "Abort" && exit 1
		#return 0
		return 1
	fi

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
	local diskdev partdev diff

	export_bootdevice && export_partdevice diskdev 0 || {
		echo "Unable to determine upgrade device"
		return 1
	}

	sync

	if [ "$UPGRADE_OPT_SAVE_PARTITIONS" = "1" ]; then
		get_partitions "/dev/$diskdev" bootdisk

		#extract the boot sector from the image
		get_image_dd "$1" of=/tmp/image.bs count=1 bs=512b

		get_partitions /tmp/image.bs image

		#compare tables
		diff="$(grep -F -x -v -f /tmp/partmap.bootdisk /tmp/partmap.image)"
	else
		diff=1
	fi

	if [ -n "$diff" ]; then
		get_image_dd "$1" of="/dev/$diskdev" bs=4096 conv=fsync

		# Separate removal and addtion is necessary; otherwise, partition 1
		# will be missing if it overlaps with the old partition 2
		partx -d - "/dev/$diskdev"
		partx -a - "/dev/$diskdev"

		return 0
	fi

	#iterate over each partition from the image and write it to the boot disk
	while read part start size; do
		if export_partdevice partdev $part; then
			echo "Writing image to /dev/$partdev..."
			get_image_dd "$1" of="/dev/$partdev" ibs="512" obs=1M skip="$start" count="$size" conv=fsync
		else
			echo "Unable to find partition $part device, skipped."
		fi
	done < /tmp/partmap.image

	#copy partition uuid
	# Do not copy partition uuid
	# echo "Writing new UUID to /dev/$diskdev..."
	# get_image_dd "$1" of="/dev/$diskdev" bs=1 skip=440 count=4 seek=440 conv=fsync
}
