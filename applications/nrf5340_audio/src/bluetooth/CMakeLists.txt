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

if (CONFIG_TRANSPORT_CIS)
	target_sources(app PRIVATE
		       ${CMAKE_CURRENT_SOURCE_DIR}/le_audio_cis_gateway.c
		       ${CMAKE_CURRENT_SOURCE_DIR}/le_audio_cis_headset.c)
elseif (CONFIG_TRANSPORT_BIS)
	target_sources(app PRIVATE
		       ${CMAKE_CURRENT_SOURCE_DIR}/le_audio_bis_gateway.c
		       ${CMAKE_CURRENT_SOURCE_DIR}/le_audio_bis_headset.c)
else()
	message(FATAL_ERROR "CIS or BIS mode must be chosen")
endif()
