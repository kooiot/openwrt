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
	local diskdev partdev diff

	export_bootdevice && export_partdevice diskdev 0 || {
		echo "Unable to determine upgrade device"
		return 1
	}

	get_partitions "/dev/$diskdev" bootdisk

	#extract the boot sector from the image
	get_image_dd "$1" of=/tmp/image.bs count=1 bs=512b 2>/dev/null

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
	*)
		return 0
		;;
	esac
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
	"kooiot,tlink-ok-a40i"|\
	"kooiot,tlink-dj-a40i-e"|\
	"kooiot,tlink-dr4-a40i"|\
	"kooiot,tlink-nano-a40i"|\
	"kooiot,tlink-qh-x40"|\
	"kooiot,tlink-m408"|\
	"kooiot,tlink-m416"|\
	"kooiot,tlink-r1")
		platform_kooiot_pre_upgrade
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

	#write uboot image
	get_image_dd "$1" of="$diskdev" bs=1024 skip=8 seek=8 count=1016 conv=fsync
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
	# Do not copy parition uuid
	# echo "Writing new UUID to /dev/$diskdev..."
	# get_image_dd "$1" of="/dev/$diskdev" bs=1 skip=440 count=4 seek=440 conv=fsync
}
