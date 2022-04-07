#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_include_directories(include)
zephyr_library_sources(
	src/zigbee_shell.c
	src/zigbee_shell_utils.c
	src/zigbee_shell_cmd.c
	src/zigbee_shell_cmd_attr.c
	src/zigbee_shell_cmd_zcl.c
	src/zigbee_shell_cmd_attr_report.c
	src/zigbee_shell_cmd_ping.c
	src/zigbee_shell_cmd_bdb.c
	src/zigbee_shell_cmd_generic_cmd.c
	src/zigbee_shell_cmd_zdo.c
	src/zigbee_shell_cmd_zscheduler.c
	src/zigbee_shell_cmd_nvram.c
	src/zigbee_shell_cmd_zcl_groups.c
	src/zigbee_shell_ctx_mgr.c
	src/zigbee_shell_cmd_nbr.c
)

zephyr_library_link_libraries(zboss)
zephyr_library_link_libraries(zigbee)
