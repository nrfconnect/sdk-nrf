#
# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

add_subdirectory(lib)

zephyr_interface_library_named(zigbee)

target_include_directories(zigbee INTERFACE osif)
target_include_directories_ifdef(CONFIG_ZIGBEE_SHELL zigbee INTERFACE cli)

zephyr_library()

zephyr_library_link_libraries(zigbee)

# Source files
zephyr_library_sources(osif/zb_nrf_platform.c)
zephyr_library_sources(osif/zb_nrf_nvram.c)
zephyr_library_sources(osif/zb_nrf_timer.c)
zephyr_library_sources(osif/zb_nrf_led_button.c)
zephyr_library_sources(osif/zb_nrf_transceiver.c)
zephyr_library_sources(osif/zb_nrf_crypto.c)
zephyr_library_sources(osif/zb_nrf_pwr_mgmt.c)
zephyr_library_sources_ifdef(CONFIG_ZIGBEE_HAVE_SERIAL osif/zb_nrf_serial.c)
zephyr_library_sources_ifndef(CONFIG_ZIGBEE_HAVE_SERIAL osif/zb_nrf_logger.c)
zephyr_library_sources_ifdef(
	CONFIG_ZIGBEE_SHELL
	cli/zigbee_cli.c
	cli/zigbee_cli_utils.c
	cli/zigbee_cli_cmd.c
	cli/zigbee_cli_cmd_zcl.c
	cli/zigbee_cli_cmd_ping.c
	cli/zigbee_cli_cmd_bdb.c
	cli/zigbee_cli_cmd_zdo.c
	cli/zigbee_cli_cmd_zscheduler.c
	cli/zigbee_cli_ctx_mgr.c
)

zephyr_library_link_libraries(zboss)
