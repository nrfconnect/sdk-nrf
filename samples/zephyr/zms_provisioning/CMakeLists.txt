# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(zms_provisioning)

target_sources(app PRIVATE src/main.c)

# Extract configuration from device tree
# Storage partition node (labeled as storage_partition)
dt_nodelabel(storage_partition_node NODELABEL "storage_partition")

if(DEFINED storage_partition_node)
    # Get storage partition address and size
    dt_reg_addr(storage_base PATH ${storage_partition_node})
    dt_reg_size(storage_size PATH ${storage_partition_node})
    
    # Get flash device from chosen node
    dt_chosen(flash_node PROPERTY "zephyr,flash")
    
    if(NOT DEFINED flash_node)
        # Fallback: try to get flash device by label
        dt_nodelabel(flash_node NODELABEL "mram1x")
    endif()
    
    if(DEFINED flash_node)
        # Get NVM base address and size from flash device
        dt_reg_addr(nvm_address PATH ${flash_node})
        dt_reg_size(nvm_size PATH ${flash_node})
        
        # Get write block size from flash device
        dt_prop(write_block_size PATH ${flash_node} PROPERTY "write-block-size")
    else()
        message(WARNING "Flash device node not found. Using defaults for NVM settings.")
        set(nvm_address "0x0")
        set(nvm_size "0x0")
        set(write_block_size "16")
    endif()
    
    # Convert to hex format for the provisioning script
    math(EXPR storage_base_dec "${storage_base}")
    math(EXPR storage_size_dec "${storage_size}")
    math(EXPR nvm_address_dec "${nvm_address}")
    math(EXPR nvm_size_dec "${nvm_size}")
    math(EXPR write_block_size_dec "${write_block_size}")
    
    set(ZMS_STORAGE_BASE "${storage_base}")
    set(ZMS_STORAGE_SIZE "${storage_size}")
    set(ZMS_NVM_ADDRESS "${nvm_address}")
    set(ZMS_NVM_SIZE "${nvm_size}")
    set(ZMS_WRITE_BLOCK_SIZE "${write_block_size}")
    
    message(STATUS "Device tree configuration found:")
    message(STATUS "  Storage partition: ${storage_partition_node}")
    if(DEFINED flash_node)
        message(STATUS "  Flash device: ${flash_node}")
    endif()
else()
    message(WARNING "Device tree node 'storage_partition' not found. Using default values.")
    # Fallback to CMake cache variables
    set(ZMS_STORAGE_BASE "0x0" CACHE STRING "Storage base address in hex format")
    set(ZMS_STORAGE_SIZE "0x1000" CACHE STRING "Storage size in bytes in hex format")
    set(ZMS_NVM_ADDRESS "0x0" CACHE STRING "NVM base address offset in hex format")
    set(ZMS_NVM_SIZE "0x0" CACHE STRING "NVM size in bytes in hex format")
    set(ZMS_WRITE_BLOCK_SIZE "16" CACHE STRING "Write block size in bytes")
endif()

# Other configuration variables
set(ZMS_SECTOR_SIZE "0x1000" CACHE STRING "Sector size in bytes in hex format")
set(ZMS_PROVISION_DATA_FILE "example_data.txt" CACHE STRING "Optional input file with ID-data pairs")
set(ZMS_PROVISION_DATA "" CACHE STRING "Optional ID-data pairs (semicolon-separated)")
set(ZMS_OUTPUT_NAME "provisioned_raw.hex" CACHE STRING "Output hex file name")

# Path to the provisioning script
set(PROVISION_SCRIPT ${ZEPHYR_BASE}/scripts/build/gen_zms_provision_data.py)

# Output directory and file
set(PROVISION_OUTPUT_DIR ${CMAKE_BINARY_DIR}/zephyr)
set(PROVISION_OUTPUT_FILE ${PROVISION_OUTPUT_DIR}/${ZMS_OUTPUT_NAME})

# Build the command line arguments for the provisioning script
set(PROVISION_ARGS
    -f ${ZMS_STORAGE_BASE}
    -s ${ZMS_STORAGE_SIZE}
    -n ${ZMS_NVM_ADDRESS}
    -z ${ZMS_NVM_SIZE}
    -w ${ZMS_WRITE_BLOCK_SIZE}
    -c ${ZMS_SECTOR_SIZE}
)

# Add data file if specified
if(ZMS_PROVISION_DATA_FILE)
    list(APPEND PROVISION_ARGS -i ${ZMS_PROVISION_DATA_FILE})
endif()

# Add individual data pairs if specified
if(ZMS_PROVISION_DATA)
    string(REPLACE ";" " -d " DATA_ARGS "${ZMS_PROVISION_DATA}")
    list(APPEND PROVISION_ARGS -d ${DATA_ARGS})
endif()

# Validate that at least one data source is provided
if(NOT ZMS_PROVISION_DATA_FILE AND NOT ZMS_PROVISION_DATA)
    message(WARNING "No provisioning data specified. Set ZMS_PROVISION_DATA_FILE or ZMS_PROVISION_DATA.")
endif()

# Create custom command to generate provisioned hex file
add_custom_command(
    OUTPUT ${PROVISION_OUTPUT_FILE}
    COMMAND ${CMAKE_COMMAND} -E remove -f ${PROVISION_OUTPUT_FILE}
    COMMAND ${PYTHON_EXECUTABLE} ${PROVISION_SCRIPT} ${PROVISION_ARGS} -o ${PROVISION_OUTPUT_FILE}
    DEPENDS ${PROVISION_SCRIPT}
            ${ZEPHYR_BASE}/scripts/build/fs_raw_instance.py
            ${ZEPHYR_BASE}/scripts/build/fs_class.py
            ${ZEPHYR_BASE}/scripts/build/zms.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating ZMS provisioning data: ${ZMS_OUTPUT_NAME}"
    VERBATIM
)

# Create custom target for the provisioning
add_custom_target(provision_raw_data ALL
    DEPENDS ${PROVISION_OUTPUT_FILE}
)

message(STATUS "ZMS Provisioning Configuration:")
message(STATUS "  Storage Base: ${ZMS_STORAGE_BASE}")
message(STATUS "  Storage Size: ${ZMS_STORAGE_SIZE}")
message(STATUS "  NVM Address: ${ZMS_NVM_ADDRESS}")
message(STATUS "  NVM Size: ${ZMS_NVM_SIZE}")
message(STATUS "  Write Block Size: ${ZMS_WRITE_BLOCK_SIZE}")
message(STATUS "  Sector Size: ${ZMS_SECTOR_SIZE}")
message(STATUS "  Merged Output File: ${MERGED_HEX}")
