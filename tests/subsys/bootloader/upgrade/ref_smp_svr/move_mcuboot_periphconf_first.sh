#!/bin/bash

# Exit immediately if a command exits with a non-zero status
set -e

# Function to process periphconf for a given build directory
move_mcuboot_periphconf_first() {
    local build_dir="$1"

    # Create a bin file with the periphconf for MCUBoot
    objcopy --set-section-flags uicr_periphconf_entry=alloc,load,readonly -I elf32-little -O binary \
    -j uicr_periphconf_entry "$build_dir/mcuboot/zephyr/zephyr.elf" "$build_dir/mcuboot_periphconf.bin"

    # Convert the full periphconf.hex to bin...
    objcopy -I ihex -O binary "$build_dir/uicr/zephyr/periphconf.hex" "$build_dir/periphconf.bin"

    # ... and concatenate with MCUBoot, placing MCUBoot first
    cat "$build_dir/mcuboot_periphconf.bin" "$build_dir/periphconf.bin" > "$build_dir/periphconf_cat.bin"

    # Remove duplicates
    bash -c "./remove_periphconf_duplicates.py $build_dir/periphconf_cat.bin > $build_dir/periphconf_cat_nodups.bin"

    # Verify that the processed periphconf matches the original when sorted, this works since the
    # original is sorted by regptr address in the first place.
    local sorted_digest
    local original_digest

    sorted_digest=$(xxd -e -g 4 -c 8 "$build_dir/periphconf_cat_nodups.bin" | awk '{print $2, $3}' | sort | md5sum)
    original_digest=$(xxd -e -g 4 -c 8 "$build_dir/periphconf.bin" | awk '{print $2, $3}' | md5sum)

    if [ "$sorted_digest" != "$original_digest" ]; then
        echo "ERROR: Digests don't match for $build_dir!"
        exit 1
    fi

    # Create a hex file we can program to replace the default periphconf.hex
    objcopy -I binary -O ihex --change-address 0x0E1AE000 \
    "$build_dir/periphconf_cat_nodups.bin" "$build_dir/periphconf_final.hex"

    echo "Generated new periphconf with mcuboot first:  $build_dir/periphconf_final.hex"
}

# Build initial image, enable radio, disable UART for MCUBoot
west build -p -b nrf54h20dk/nrf54h20/cpuapp -d build_no_mcuboot_uart -T no_mcuboot_uart .

# Process periphconf for the initial build (includes digest comparison)
move_mcuboot_periphconf_first "build_no_mcuboot_uart"

# Build of image with swapped UART, used to get new periphconf
west build -p -b nrf54h20dk/nrf54h20/cpuapp -d build_swapped_uart_v1 -T swapped_uart_v1 .

# Process periphconf for the swapped UART build
move_mcuboot_periphconf_first "build_swapped_uart_v1"

# Generate the periphconf that will be contained in the update image
objcopy --input-target=ihex --output-target=binary build_swapped_uart_v1/periphconf_final.hex \
build_swapped_uart_v1/periphconf_swapped_uart.bin

# Re-build the swapped image with integrated PERIPHCONF and bumped firmware version
west build -p -b nrf54h20dk/nrf54h20/cpuapp -d build_swapped_uart_v2 -T swapped_uart_v2 . -- \
-DPERIPHCONF_BIN=build_swapped_uart_v1/periphconf_swapped_uart.bin
