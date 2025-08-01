#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# BLE transport layer for WiFi provisioning
zephyr_library_named(wifi_prov_ble)

# Add include path to generated .pb.h header files
zephyr_library_include_directories(${CMAKE_CURRENT_BINARY_DIR})
zephyr_library_include_directories(${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/wifi_prov_core)
zephyr_library_include_directories($<TARGET_PROPERTY:wifi_prov_core,BINARY_DIR>)

zephyr_library_sources(
    wifi_prov_ble.c
)

zephyr_library_link_libraries(wifi_prov_core)
# Ensure wifi_prov_ble depends on wifi_prov_core
add_dependencies(wifi_prov_ble wifi_prov_core)
