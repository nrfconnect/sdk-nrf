# SPDX-License-Identifier: Apache-2.0

zephyr_library_amend(${ZEPHYR_BASE}/subsys/bluetooth)

target_include_directories(subsys__bluetooth INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(common)
add_subdirectory_ifdef(CONFIG_BT_HCI host)
add_subdirectory_ifdef(CONFIG_BT_CRYPTO crypto)

zephyr_include_directories(${ZEPHYR_NRF_MODULE_DIR}/subsys/bluetooth/host)

add_subdirectory(lib)
