#!/bin/bash
#
# configure_fscc_range.sh
# Silently configures a range of FSCC tty devices.
# Exits with 0 on complete success, non-zero if any device fails.
#

set -u

# ====================== CONFIGURATION ======================
# Edit this section as needed
DEVICES=(/dev/ttyS12 /dev/ttyS13 /dev/ttyS14 /dev/ttyS15 /dev/ttyS16 /dev/ttyS17 /dev/ttyS18 /dev/ttyS19)
BAUD_RATE=921600
CONFIG_PROG="./fscc_config"          # Change to /usr/local/bin/fscc_config if installed
# ===========================================================

success_count=0
failure_count=0
failed_devices=()

for device in "${DEVICES[@]}"; do
    if [ ! -e "$device" ]; then
        failed_devices+=("$device (not found)")
        ((failure_count++))
        continue
    fi

    # Run silently and capture only the exit code
    if "$CONFIG_PROG" "$device" "$BAUD_RATE" >/dev/null 2>&1; then
        ((success_count++))
    else
        failed_devices+=("$device")
        ((failure_count++))
    fi
done

# Final exit status only (no output)
if [ $failure_count -eq 0 ]; then
    exit 0
else
    exit 1
fi