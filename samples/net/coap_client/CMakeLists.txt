#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(coap-client)

if(CONFIG_COAP_SAMPLE_DTLS)
  set(gen_dir "${ZEPHYR_BINARY_DIR}/include/generated")

  generate_inc_file_for_target(
    app
    ${CONFIG_COAP_SAMPLE_CA_CERT_FILE}
    "${gen_dir}/coap_sample_ca_cert.inc"
  )

  generate_inc_file_for_target(
    app
    ${CONFIG_COAP_SAMPLE_CLIENT_CERT_FILE}
    "${gen_dir}/coap_sample_client_cert.inc"
  )

  generate_inc_file_for_target(
    app
    ${CONFIG_COAP_SAMPLE_CLIENT_KEY_FILE}
    "${gen_dir}/coap_sample_client_key.inc"
  )
endif()

# NORDIC SDK APP START
target_sources(app PRIVATE src/main.c)
# NORDIC SDK APP END
