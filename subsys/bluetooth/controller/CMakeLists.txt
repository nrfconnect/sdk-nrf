#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()

zephyr_library_sources(
  hci_driver.c
  hci_internal.c
)

zephyr_library_sources_ifdef(
  CONFIG_BT_CTLR_CRYPTO
  crypto.c
)

zephyr_library_sources_ifdef(
  CONFIG_BT_CTLR_ECDH
  ecdh.c
)

zephyr_library_link_libraries_ifdef(
  CONFIG_BT_CTLR_ECDH_LIB_OBERON
  nrfxlib_crypto
)

zephyr_library_link_libraries(subsys__bluetooth)

zephyr_include_directories(.)

math(EXPR req_conns "${CONFIG_BT_CTLR_SDC_CENTRAL_COUNT} + ${CONFIG_BT_CTLR_SDC_PERIPHERAL_COUNT}")

if (CONFIG_BT_MAX_CONN LESS req_conns)

  message(WARNING "There are not enough host connection contexts to be able to \
establish ${CONFIG_BT_CTLR_SDC_CENTRAL_COUNT} central and \
${CONFIG_BT_CTLR_SDC_PERIPHERAL_COUNT} peripheral connections simultaneously. \
For this to be possible, CONFIG_BT_MAX_CONN must be set to ${req_conns}.")

endif()
