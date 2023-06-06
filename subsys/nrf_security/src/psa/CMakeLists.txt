#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

#
# Create a zephyr library adding the EITS
#
zephyr_library_named(mbedcrypto_eits)
zephyr_library_sources(
  ${CMAKE_CURRENT_LIST_DIR}/psa_eits.c
  ${CMAKE_CURRENT_LIST_DIR}/psa_eits_zephyr_settings.c
)

zephyr_library_include_directories(.)

target_include_directories(${mbedcrypto_target} INTERFACE
  ${NRF_SECURITY_ROOT}/include/native_its
)

zephyr_library_link_libraries(
  ${mbedcrypto_target}
)

nrf_security_debug_list_target_files(mbedcrypto_eits)
