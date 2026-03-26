#!/bin/bash

# Usage: ./led.sh <SERIAL> <COLOR> [flash/pulse]
TARGET_SN=$1
COLOR_NAME=$2
MODE_NAME=$3

# Find device by checking its parent's serial attribute in sysfs
DEV_NODE=""
for dev in /sys/class/hidraw/hidraw*; do
    # Get serial from the parent USB device
    SN=$(cat "$dev/device/../serial" 2>/dev/null)
    if [[ "$SN" == "$TARGET_SN" ]]; then
        DEV_NODE=$(basename "$dev")
        break
    fi
done

if [[ -z "$DEV_NODE" ]]; then
    echo "Device with Serial $TARGET_SN not found."
    exit 1
fi

# ... (Previous RGB parsing logic here) ...

# Send data
echo -ne "\x00${HEX_R}${HEX_G}${HEX_B}${HEX_M}" | sudo tee /dev/$DEV_NODE > /dev/null

