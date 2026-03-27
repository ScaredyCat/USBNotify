#!/bin/bash

#
# If you're using this on a mac change the shebang to: 
#
#      #!/bin/zsh
#

# Configuration
VID="1ed5"
PID="aced"

# Color Map (R G B)
declare -A COLOURS=( 
    [red]="255 0 0" [green]="0 255 0" [blue]="0 0 255"
    [yellow]="255 255 0" [magenta]="255 0 255" [cyan]="0 255 255"
    [white]="255 255 255" [orange]="255 165 0" [purple]="157 0 255"
    [amber]="220 166 8" [indigo]="75 0 130" [off]="0 0 0"
)

OS_TYPE=$(uname -s)

list_devices() {
    echo "Scanning for Waveshare RP2040-One devices..."
    if [[ "$OS_TYPE" == "Linux" ]]; then
        printf "%-12s | %-20s\n" "Node" "Serial Number"
	for dev in /dev/hidraw*; do
	    [ -e "$dev" ] || continue
	    INFO=$(udevadm info --attribute-walk --name="$dev" 2>/dev/null)
	    echo "$INFO" | grep -qi "ATTRS{idVendor}==\"$VID\"" || continue
	    echo "$INFO" | grep -qi "ATTRS{idProduct}==\"$PID\"" || continue
	    SN=$(echo "$INFO" | grep -m1 'ATTRS{serial}=="' | cut -d'"' -f2)
	    [[ -n "$SN" ]] && printf "%-12s | %-20s\n" "$dev" "$SN"
	done
    elif [[ "$OS_TYPE" == "Darwin" ]]; then
        hidapitester --vidpid "$VID/$PID" --list-detail
    fi
}

send_cmd() {
    local TARGET_SN=$1
    local COLOUR_NAME=$2
    local MODE_NAME=$3
    local BRIGHT_PERC=${4:-100}
    local SAVE_FLAG=$5   # pass --save to persist settings to flash

    # Parse colour
    read R G B <<< "${COLOURS[$COLOUR_NAME]}"
    case "$MODE_NAME" in flash) M=1 ;; pulse) M=2 ;; blip) M=3 ;; rainbow) M=4 ;; *) M=0 ;; esac

    B_VAL=$(( BRIGHT_PERC * 255 / 100 ))

    # Byte 6 (index 5): 1 = save to flash, 0 = don't save
    local SAVE_BYTE=0
    [[ "$SAVE_FLAG" == "--save" ]] && SAVE_BYTE=1

    if [[ "$OS_TYPE" == "Linux" ]]; then
        DEV_NODE=$(for dev in /dev/hidraw*; do
            SN=$(udevadm info --attribute-walk --name="$dev" 2>/dev/null | grep -m1 'ATTRS{serial}=="' | cut -d'"' -f2)
            [[ "$SN" == "$TARGET_SN" ]] && echo "$dev" && break
        done)
        [[ -z "$DEV_NODE" ]] && echo "Device $TARGET_SN not found" && exit 1
        echo -ne "\x00$(printf '\\x%02x\\x%02x\\x%02x\\x%02x\\x%02x\\x%02x' $R $G $B $M $B_VAL $SAVE_BYTE)" > "$DEV_NODE"
        [[ "$SAVE_BYTE" == "1" ]] && echo "Settings saved to flash."

    elif [[ "$OS_TYPE" == "Darwin" ]]; then
        hidapitester --vidpid "$VID/$PID" --serial "$TARGET_SN" --open \
            --send-output "0,$R,$G,$B,$M,$B_VAL,$SAVE_BYTE"
        [[ "$SAVE_BYTE" == "1" ]] && echo "Settings saved to flash."
    fi
}

usage() {
    echo "Usage:"
    echo "  $0 list"
    echo "  $0 <serial> <colour> [mode] [brightness%] [--save]"
    echo ""
    echo "  mode    : solid (default), flash, pulse, blip, rainbow"
    echo "  --save  : persist colour/mode/brightness to flash (restored on power-on)"
    echo ""
    echo "Examples:"
    echo "  $0 list"
    echo "  $0 ABC123 red"
    echo "  $0 ABC123 blue pulse 75"
    echo "  $0 ABC123 green flash 100 --save"
}

case "$1" in
    list) list_devices ;;
    help|--help|-h) usage ;;
    *) send_cmd "$@" ;;
esac
