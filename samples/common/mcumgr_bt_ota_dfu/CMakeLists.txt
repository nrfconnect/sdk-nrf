#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if (CONFIG_NCS_SAMPLE_MCUMGR_BT_OTA_DFU_VALIDATION)

  if (CONFIG_NCS_SAMPLE_MCUMGR_BT_OTA_DFU_SPEEDUP AND CONFIG_BT_CTLR)
    if ((NOT (CONFIG_BT_BUF_ACL_TX_SIZE EQUAL 251)) OR
        (NOT (CONFIG_BT_BUF_ACL_RX_SIZE EQUAL 502)) OR
        (NOT (CONFIG_BT_CTLR_DATA_LENGTH_MAX EQUAL 251))
      )
      message(WARNING "MCUmgr Bluetooth OTA DFU: suboptimal configuration "
        "of Bluetooth Controller buffers. Consider using optimal values "
        "defined in the ${CMAKE_CURRENT_LIST_DIR}/Kconfig file for "
        "CONFIG_BT_BUF_ACL_TX_SIZE, CONFIG_BT_BUF_ACL_RX_SIZE and "
        "CONFIG_BT_CTLR_DATA_LENGTH_MAX options")
    endif()
  endif()

  if (CONFIG_NCS_SAMPLE_MCUMGR_BT_OTA_DFU_SPEEDUP AND CONFIG_BT_HCI_HOST)
    if ((NOT (CONFIG_BT_BUF_ACL_TX_SIZE EQUAL 251)) OR
        (NOT (CONFIG_BT_BUF_ACL_RX_SIZE EQUAL 502)) OR
        (NOT (CONFIG_BT_L2CAP_TX_MTU EQUAL 498))
      )
      message(WARNING "MCUmgr Bluetooth OTA DFU: suboptimal configuration "
        "of Bluetooth Host buffers. Consider using optimal values "
        "defined in the ${CMAKE_CURRENT_LIST_DIR}/Kconfig file for "
        "CONFIG_BT_BUF_ACL_TX_SIZE, CONFIG_BT_BUF_ACL_RX_SIZE and "
        "CONFIG_BT_L2CAP_TX_MTU options")
    endif()
  endif()

  if (CONFIG_NCS_SAMPLE_MCUMGR_BT_OTA_DFU)
    if (CONFIG_MCUMGR_SMP_REASSEMBLY_BT)
      if (NOT (CONFIG_MCUMGR_BUF_SIZE EQUAL 2475))
        message(WARNING "MCUmgr Bluetooth OTA DFU: suboptimal configuration "
          "of the MCUmgr buffer with the Reassembly feature support. Consider using"
          "the optimal value defined in the ${CMAKE_CURRENT_LIST_DIR}/Kconfig "
          "file for the CONFIG_MCUMGR_BUF_SIZE option")
      endif()
      if (NOT CONFIG_OS_MGMT_MCUMGR_PARAMS)
        message(WARNING "MCUmgr Bluetooth OTA DFU: suboptimal configuration "
          "of the MCUmgr library with the Reassembly feature support. Consider "
          "enabling the CONFIG_OS_MGMT_MCUMGR_PARAMS option")
      endif()
    else()
      message(WARNING "MCUmgr Bluetooth OTA DFU: suboptimal configuration "
        "of the MCUmgr library without the Reassembly feature support. Consider "
        "enabling this feature using the CONFIG_MCUMGR_SMP_REASSEMBLY_BT option")
    endif()
  endif()

endif()
