#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

add_overlay_dts(recovery ${CMAKE_CURRENT_LIST_DIR}/boards/nrf54h20dk_nrf54h20_cpuapp.overlay)

ExternalZephyrProject_Add(
	APPLICATION recovery_hci_ipc
	SOURCE_DIR  "${ZEPHYR_BASE}/samples/bluetooth/hci_ipc"
	BOARD       ${BOARD}/${SB_CONFIG_SOC}/${SB_CONFIG_NETCORE_REMOTE_BOARD_TARGET_CPUCLUSTER}
	BOARD_REVISION ${BOARD_REVISION}
)

add_overlay_config(recovery_hci_ipc ${CMAKE_CURRENT_LIST_DIR}/sysbuild/hci_ipc.conf)

add_overlay_dts(recovery_hci_ipc ${CMAKE_CURRENT_LIST_DIR}/sysbuild/hci_ipc.overlay)
