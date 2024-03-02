#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_interface_library_named(curl)

target_include_directories(curl INTERFACE include/curl)

zephyr_library()
zephyr_library_include_directories(lib)

zephyr_library_link_libraries(curl)

add_subdirectory_ifdef(CONFIG_NRF_CURL_LIB lib)
add_subdirectory_ifdef(CONFIG_NRF_CURL_TOOL tool)
