#!/bin/sh

if [ "$(id -u)" != "0" ]; then
   echo "Script must be run as root !"
   exit 0
fi

BOARD_NAME=`cat /tmp/sysinfo/board_name  | sed "s/.*\,\(.*\)$/\1/"`


echo ""
date
echo -e "\033[36m==============================="
echo "Generate eMMC data partition"
echo -e "===============================\033[37m"
echo ""

EMMC_DISK="/dev/mmcblk1"

# If blk1 not there try blk2 (in R40 borads)
if [ ! -b ${EMMC_DISK} ]; then
	EMMC_DISK="/dev/mmcblk2"
fi

EMMC_FIRST_SECTOR=65536
EMMC_BOOTFS_SIZE=$((30 * 1024 * 1024))
EMMC_ROOTFS_SIZE=$((256 * 1024 * 1024))
EMMC_DATA_FIRST_SECTOR=$(( 800 * 1024 ))

if [ ! -b ${EMMC_DISK}boot0 ]; then
    echo "Error: EMMC not found."
    exit 1
fi
if [ -b ${EMMC_DISK}p3 ]; then
    echo "Error: EMMC data partition found."
    exit 1
fi

#----------------------------------------------------------
echo ""
echo -n "WARNING: EMMC DATA PARTITION WILL BE ERASED !, Continue (y/N)?  "
read -n 1 ANSWER

if [ ! "${ANSWER}" = "y" ] ; then
	echo "."
	echo "Canceled.."
	exit 0
fi
echo ""
#----------------------------------------------------------

echo "Partitioning EMMC ..."

sbootfs=$(( $EMMC_FIRST_SECTOR ))
ebootfs=$(( $EMMC_BOOTFS_SIZE / 512 + $sbootfs - 1 ))
srootfs=$(( $ebootfs + 1 + 4096 ))
erootfs=$(( $EMMC_ROOTFS_SIZE / 512 + $srootfs -1 ))
sdatafs=$(( $EMMC_DATA_FIRST_SECTOR ))
edatafs=""

# echo "BOOTFS: "$sbootfs" - "$ebootfs
# echo "ROOTFS: "$srootfs" - "$erootfs
echo "DATAFS: "$sdatafs" - "$edatafs

echo "  Creating eMMC partitions"
echo -e "n\np\n3\n$sdatafs\n$edatafs\nt\n3\n83\nw" | fdisk ${EMMC_DISK} > /dev/null 2>&1
echo $?
echo "  OK."
sync
sleep 2
partx -s ${EMMC_DISK} > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "ERROR: Failed to create data partition"
    exit 1
fi
sleep 1

echo "Formating data partition (ext4), please wait ..."
dd if=/dev/zero of=${EMMC_DISK}p3 bs=1M count=1 oflag=direct > /dev/null 2>&1
sync
sleep 1

mkfs.ext4 -L data ${EMMC_DISK}p3 > /dev/null 2>&1
if [ $? -ne 0 ]; then
	echo "ERROR formating ext4 partition."
	exit 1
fi
echo "  linux partition formated."

# -------------------------------------------------------------------

echo ""
echo -e "\033[36m*******************************"
echo "Data partition written to EMMC."
echo -e "*******************************\033[37m"
echo ""


tlink_mount_data() {
	data_blk_p="/dev/mmcblk1p3"
	rule_name=$(uci add fstab mount)
	uci batch <<EOF
set fstab.$rule_name.target='/mnt/data'
set fstab.$rule_name.device='$data_blk_p'
set fstab.$rule_name.fstype='ext4'
set fstab.$rule_name.options='rw,async'
set fstab.$rule_name.enabled='1'
set fstab.$rule_name.enabled_fsck='1'
EOF
	uci commit fstab
}

tlink_mount_data

echo ""
echo -e "\033[36m*******************************"
echo "Data partition mount info added into /etc/config/fstab."
echo -e "*******************************\033[37m"
echo ""


exit 0
