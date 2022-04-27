#
# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

add_subdirectory(lib)

zephyr_interface_library_named(zigbee)

target_include_directories(zigbee INTERFACE osif)

zephyr_library()

zephyr_library_link_libraries(zigbee)

zephyr_link_libraries_ifdef(CONFIG_ZIGBEE_USE_SOFTWARE_AES nrfxlib_crypto)

# Source files
zephyr_library_sources(osif/zb_nrf_platform.c)
zephyr_library_sources(osif/zb_nrf_transceiver.c)
zephyr_library_sources(osif/zb_nrf_crypto.c)
zephyr_library_sources(osif/zb_nrf_pwr_mgmt.c)
zephyr_library_sources_ifdef(CONFIG_DK_LIBRARY osif/zb_nrf_led_button.c)
zephyr_library_sources_ifdef(CONFIG_ZIGBEE_HAVE_SERIAL osif/zb_nrf_serial.c)
zephyr_library_sources_ifdef(CONFIG_ZIGBEE_HAVE_ASYNC_SERIAL osif/zb_nrf_async_serial.c)
zephyr_library_sources_ifdef(CONFIG_ZBOSS_TRACE_HEXDUMP_LOGGING osif/zb_nrf_logger.c)
zephyr_library_sources_ifdef(CONFIG_ZBOSS_TRACE_BINARY_LOGGING osif/zb_nrf_serial_logger.c)

zephyr_library_sources_ifdef(CONFIG_FLASH_MAP osif/zb_nrf_nvram.c)
zephyr_library_sources_ifndef(CONFIG_FLASH_MAP osif/zb_nrf_nvram_none.c)

zephyr_library_sources_ifdef(CONFIG_ZIGBEE_TIME_COUNTER osif/zb_nrf_timer_counter.c)
zephyr_library_sources_ifdef(CONFIG_ZIGBEE_TIME_KTIMER osif/zb_nrf_timer_ktimer.c)

zephyr_library_link_libraries(zboss)

zephyr_compile_definitions(CUSTOM_IEEE802154_L2=ZIGBEE_L2)
zephyr_compile_definitions(ZIGBEE_L2=ZIGBEE)
