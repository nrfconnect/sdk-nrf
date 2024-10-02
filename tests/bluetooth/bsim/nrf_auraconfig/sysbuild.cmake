#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(NOT("${SB_CONFIG_NET_CORE_BOARD}" STREQUAL ""))
	set(NET_APP ipc_radio)
	set(NET_APP_SRC_DIR ${ZEPHYR_BASE}/../nrf/applications/${NET_APP})

	ExternalZephyrProject_Add(
		APPLICATION ${NET_APP}
		SOURCE_DIR  ${NET_APP_SRC_DIR}
		BOARD       ${SB_CONFIG_NET_CORE_BOARD}
	)

	set(${NET_APP}_CONF_FILE
	${NET_APP_SRC_DIR}/overlay-bt_hci_ipc.conf
	prj.conf
	CACHE INTERNAL "")

	native_simulator_set_primary_mcu_index(${DEFAULT_IMAGE} ${NET_APP})

	native_simulator_set_child_images(${DEFAULT_IMAGE} ${NET_APP})
endif()

native_simulator_set_final_executable(${DEFAULT_IMAGE})
