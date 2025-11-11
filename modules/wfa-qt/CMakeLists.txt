#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(NOT CONFIG_WFA_QT_CONTROL_APP)
  return()
endif()

zephyr_library()

# Enable of same options as upstream points to issues with Zephyr
# headers, so, for now, we disable them.
zephyr_library_compile_options(
  -Werror
  # False positive, not going away even with null check
  -Wno-format-overflow
)

zephyr_library_compile_definitions(CONFIG_CTRL_IFACE_ZEPHYR)

set(WFA_QT_BASE ${ZEPHYR_NRF_MODULE_DIR}/../modules/lib/wfa-qt-control-app)
zephyr_include_directories(${WFA_QT_BASE})

zephyr_library_sources(
  # Zephyr's port of the Indigo API
  src/wpas_events.c
  src/main.c
  src/indigo_api_callback_dut.c
  src/vendor_specific_dut.c
  src/utils.c
  ${WFA_QT_BASE}/eloop.c
  ${WFA_QT_BASE}/indigo_api.c
  ${WFA_QT_BASE}/indigo_packet.c
  ${WFA_QT_BASE}/qt_client.c
)
