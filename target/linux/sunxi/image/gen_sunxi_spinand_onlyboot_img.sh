#!/usr/bin/env bash
#
# Copyright (C) 2019 Benedikt-Alexander Mokroß (iCOGNIZE GmbH)
# Copyright (C) 2024 Dirk Chang <dirk@kooiot.com>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

set -ex
[ $# -eq 4 ] || [ $# -eq 6 ] || {
    echo "SYNTAX: $0 <outputfile> <u-boot image> <nand page size> <nand block size in KiB>"
    echo "Given: $@"
    exit 1
}

OUTPUT="$1"
UBOOT="$2"
PAGESIZE="$3"
BLOCKSIZE="$4"

# SPL-Size is an uint32 at 16 bytes offset contained in the SPL header
SPLSIZE=$(od -An -t u4 -j16 -N4 "$UBOOT" | xargs)

ALIGNCHECK=$(($PAGESIZE%1024))
if [ "$ALIGNCHECK" -ne "0" ]; then
	echo "Page-size is not 1k alignable and thus not supported by EGON"
	exit -1
fi

KPAGESIZE=$(($PAGESIZE/1024))
SPLBLOCKS=$(($SPLSIZE/1024))
LOOPSPLBLOCKS=$(($SPLBLOCKS-1))
UBOOTOFFSET=$(($SPLBLOCKS * $KPAGESIZE))

echo "$@" > $OUTPUT.imgmeta
echo "BLOCK-SIZE: $BLOCKSIZE KiB" >> $OUTPUT.imgmeta
echo "PAGE-SIZE: $KPAGESIZE KiB">> $OUTPUT.imgmeta
echo "SPL-SIZE: $SPLSIZE">> $OUTPUT.imgmeta
echo "SPL-BLOCKS: $SPLBLOCKS">> $OUTPUT.imgmeta
echo "U-BOOT-OFFSET: $UBOOTOFFSET">> $OUTPUT.imgmeta
echo "## Layout ##">> $OUTPUT.imgmeta

echo "Generating 0-image for boot part of size $SPLSIZE ($SPLBLOCKS blocks)"
dd if="/dev/zero" of="$OUTPUT" bs=1024 count=$SPLBLOCKS

echo "Copying block 0 to 0"
dd if="$UBOOT" of="$OUTPUT" bs=1024 count=2 seek=0 skip=0 conv=notrunc

for from in `seq 1 $LOOPSPLBLOCKS`;
do
	to=$(($from*$KPAGESIZE))
	echo "Copying block $from to $to" 
	dd if="$UBOOT" of="$OUTPUT" bs=1024 count=1 seek=$to skip=$from conv=notrunc
done

echo "Appending u-boot to chunked SPL at block $UBOOTOFFSET (origin: $SPLBLOCKS)"
dd if="$UBOOT" of="$OUTPUT" bs=1024 seek=$UBOOTOFFSET skip=$SPLBLOCKS conv=notrunc

