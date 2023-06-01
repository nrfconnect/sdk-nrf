#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()

set(WFA_QT_BASE ${CMAKE_CURRENT_SOURCE_DIR}/../../../modules/lib/wfa-qt-control-app)
set(SOURCES_BASE ${CMAKE_CURRENT_SOURCE_DIR}/)

# Enable of same options as upstream points to issues with Zephyr
# headers, so, for now, we disable them.
zephyr_library_compile_options_ifdef(CONFIG_WFA_QT_CONTROL_APP
	-Werror
	# False positive, not going away even with null check
	-Wno-format-overflow
)

zephyr_library_compile_definitions_ifdef(CONFIG_WFA_QT_CONTROL_APP
	CONFIG_ZEPHYR
)

zephyr_include_directories_ifdef(CONFIG_WFA_QT_CONTROL_APP
	${WFA_QT_BASE}
)

zephyr_library_sources_ifdef(CONFIG_WFA_QT_CONTROL_APP
	# Zephyr's port of the Indigo API
	${SOURCES_BASE}/src/main.c
	${SOURCES_BASE}/src/indigo_api_callback_dut.c
	${SOURCES_BASE}/src/vendor_specific_dut.c
	${SOURCES_BASE}/src/utils.c
	${WFA_QT_BASE}/eloop.c
	${WFA_QT_BASE}/indigo_api.c
	${WFA_QT_BASE}/indigo_packet.c
	${WFA_QT_BASE}/qt_client.c
)
