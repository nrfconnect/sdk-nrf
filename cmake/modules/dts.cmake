# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

if((BOARD MATCHES "dk" OR BOARD MATCHES "ek") AND BOARD_QUALIFIERS MATCHES "nrf91")
  list(PREPEND DTC_OVERLAY_FILE
    ${ZEPHYR_NRF_MODULE_DIR}/dts/common/nordic/nrf91_partitions.dtsi
  )
endif()

list(REMOVE_ITEM CMAKE_MODULE_PATH ${ZEPHYR_NRF_MODULE_DIR}/cmake/modules)
include(dts)
list(PREPEND CMAKE_MODULE_PATH ${ZEPHYR_NRF_MODULE_DIR}/cmake/modules)
