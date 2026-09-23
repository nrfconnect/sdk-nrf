#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(sap_demo)

target_sources(app PRIVATE
  src/main.c
  src/demo_credentials.c
)

target_sources_ifdef(CONFIG_SAMPLE_BT_SAP_ROLE_CENTRAL app PRIVATE src/sap_central.c)
target_sources_ifdef(CONFIG_SAMPLE_BT_SAP_ROLE_PERIPHERAL app PRIVATE src/sap_peripheral.c)
target_include_directories(app PRIVATE src)

if(CONFIG_SAMPLE_BT_SAP_USE_CUSTOM_CREDENTIALS)
  set(gen_dir "${ZEPHYR_BINARY_DIR}/include/generated")

  generate_inc_file_for_target(
    app
    "${CONFIG_SAMPLE_BT_SAP_PRIVATE_KEY_FILE}"
    "${gen_dir}/sap_private_key.inc"
  )
  generate_inc_file_for_target(
    app
    "${CONFIG_SAMPLE_BT_SAP_CERTIFICATE_FILE}"
    "${gen_dir}/sap_certificate.inc"
  )
  generate_inc_file_for_target(
    app
    "${CONFIG_SAMPLE_BT_SAP_CA_PUBLIC_KEY_FILE}"
    "${gen_dir}/sap_ca_public_key.inc"
  )

  message(WARNING
    "The SAP sample is embedding raw credential bytes from Kconfig-selected files. "
    "This path is for demos and tests only; production firmware must use "
    "product provisioning and secure credential storage."
  )
else()
  message(WARNING
    "The SAP sample is using preset demonstration credentials. "
    "Do not use these keys in production."
  )
endif()
