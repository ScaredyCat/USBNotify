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
            SN=$(udevadm info --attribute-walk --name="$dev" 2>/dev/null | grep -m1 'ATTRS{serial}=="' | cut -d'"' -f2)
            [[ -n "$SN" ]] && printf "%-12s | %-20s\n" "$dev" "$SN"
        done
    elif [[ "$OS_TYPE" == "Darwin" ]]; then
        # macOS: List matching VID/PID with details
        hidapitester --vidpid "$VID/$PID" --list-detail
    fi
}

send_cmd() {
    local TARGET_SN=$1
    local COLOUR_NAME=$2
    local MODE_NAME=$3
    local BRIGHT_PERC=${4:-100}

    # Parse Color & Mode
    read R G B <<< "${COLOURS[$COLOUR_NAME]}"
#    M=0; [[ "$MODE_NAME" == "flash" ]] && M=1; [[ "$MODE_NAME" == "pulse" ]] && M=2
    case "$MODE_NAME" in flash) M=1 ;; pulse) M=2 ;; blip) M=3 ;; rainbow) M=4 ;; *) M=0 ;; esac


    B_VAL=$(( BRIGHT_PERC * 255 / 100 ))

    if [[ "$OS_TYPE" == "Linux" ]]; then
        DEV_NODE=$(for dev in /dev/hidraw*; do
            SN=$(udevadm info --attribute-walk --name="$dev" 2>/dev/null | grep -m1 'ATTRS{serial}=="' | cut -d'"' -f2)
            [[ "$SN" == "$TARGET_SN" ]] && echo "$dev" && break
        done)
        [[ -z "$DEV_NODE" ]] && echo "Device $TARGET_SN not found" && exit 1
        echo -ne "\x00$(printf '\\x%02x\\x%02x\\x%02x\\x%02x\\x%02x' $R $G $B $M $B_VAL)" > "$DEV_NODE"

    elif [[ "$OS_TYPE" == "Darwin" ]]; then
        # macOS: Use --serial to target specific board
        # Packet format: 0 is Report ID, followed by R, G, B, Mode, Bright
        hidapitester --vidpid "$VID/$PID" --serial "$TARGET_SN" --open --send-output "0,$R,$G,$B,$M,$B_VAL"
    fi
}

case "$1" in
    list) list_devices ;;
    *) send_cmd "$@" ;;
esac


