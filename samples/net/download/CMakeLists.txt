#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(download)

set(gen_dir "${ZEPHYR_BINARY_DIR}/include/generated")

# Generate hex file from pem file
if(CONFIG_SAMPLE_PROVISION_CERT)
  generate_inc_file_for_target(
    app
    ${CONFIG_SAMPLE_CERT_FILE}
    "${gen_dir}/sample_cert.inc"
  )
endif()

if(CONFIG_SAMPLE_PROVISION_CLIENT_CERT)
  generate_inc_file_for_target(
    app
    ${CONFIG_SAMPLE_CLIENT_CERT_FILE}
    "${gen_dir}/sample_client_cert.inc"
  )

  generate_inc_file_for_target(
    app
    ${CONFIG_SAMPLE_CLIENT_KEY_FILE}
    "${gen_dir}/sample_client_key.inc"
  )
endif()

# NORDIC SDK APP START
target_sources(app PRIVATE src/main.c)
# NORDIC SDK APP END
