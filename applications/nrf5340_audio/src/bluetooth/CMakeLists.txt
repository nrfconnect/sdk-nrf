#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_sources(app PRIVATE
	       ${CMAKE_CURRENT_SOURCE_DIR}/ble_audio_services.c
	       ${CMAKE_CURRENT_SOURCE_DIR}/ble_core.c
	       ${CMAKE_CURRENT_SOURCE_DIR}/ble_hci_vsc.c
)
