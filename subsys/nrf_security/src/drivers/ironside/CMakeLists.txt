# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

# Driver sources are expected to be added externally
add_library(ironside_psa_driver STATIC)

target_include_directories(psa_crypto_library_config
  INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
    $<TARGET_PROPERTY:ironside_psa_driver,INTERFACE_INCLUDE_DIRECTORIES>
)

# Link psa_core with this driver
target_link_libraries(psa_core
  PRIVATE
    ironside_psa_driver
)

nrf_security_add_zephyr_options_library(ironside_psa_driver)
