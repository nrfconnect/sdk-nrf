#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(download)

# Generate hex file from pem file
get_filename_component(FILE_NAME ${CONFIG_SAMPLE_CERT_FILE} NAME)
set(OUTPUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/certs/${FILE_NAME}.inc")
add_definitions(-DSAMPLE_CERT_FILE_INC="${OUTPUT_FILE}")
generate_inc_file_for_target(
    app
    ${CONFIG_SAMPLE_CERT_FILE}
    ${OUTPUT_FILE}
)

# NORDIC SDK APP START
target_sources(app PRIVATE src/main.c)
# NORDIC SDK APP END
