#!/usr/bin/bash

if [ -z "$1" ]; then
	echo "Usage: $0 <output_hex_path>"
	exit 1
fi

out_hex_path=$1
thread_cal_dir=$(dirname "$0")
raw_thread_cal_path="${thread_cal_dir}/ThreadCal.hex"
thread_cal_hex="$thread_cal_dir/PsemiThreadCal_nrf52840.hex"
power_map_address=0xF7000

echo "Looking for power map in directory: ${thread_cal_dir}"
rm -rf "${thread_cal_dir}"/*ThreadCal_nrf*

echo "Using address of power map partition: $power_map_address"
if ! arm-none-eabi-objcopy --change-addresses $power_map_address "$raw_thread_cal_path" "$thread_cal_hex"; then
	echo "Error: Failed to change addresses in $raw_thread_cal_path"
	exit 1
fi

echo "Merging power map table $thread_cal_hex with $out_hex_path starting from address $power_map_address"
if ! mergehex -m "$out_hex_path" "$thread_cal_hex" -o "$out_hex_path"; then
	echo "Error: Failed to merge hex files"
	exit 1
fi

rm -rf -v "${thread_cal_dir}"/*ThreadCal_nrf*