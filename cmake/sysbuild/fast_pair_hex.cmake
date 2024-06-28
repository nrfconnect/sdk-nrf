#
# Copyright (c) 2022-2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

set(FP_PARTITION_NAME bt_fast_pair)

set(
  FP_PROVISIONING_DATA_HEX
  ${CMAKE_BINARY_DIR}/modules/nrf/subsys/bluetooth/services/fast_pair/fp_provisioning_data.hex
  )

set(FP_PROVISIONING_DATA_ADDRESS $<TARGET_PROPERTY:partition_manager,PM_BT_FAST_PAIR_ADDRESS>)

add_custom_command(
  OUTPUT
  ${FP_PROVISIONING_DATA_HEX}
  DEPENDS
  "${APPLICATION_BINARY_DIR}/pm.config"
  COMMAND
  ${PYTHON_EXECUTABLE} ${ZEPHYR_NRF_MODULE_DIR}/scripts/nrf_provision/fast_pair/fp_provision_cli.py
                       -o ${FP_PROVISIONING_DATA_HEX} -a ${FP_PROVISIONING_DATA_ADDRESS}
                       -m ${FP_MODEL_ID} -k ${FP_ANTI_SPOOFING_KEY}
  COMMENT
  "Generating Fast Pair provisioning data hex file"
  USES_TERMINAL
  )

add_custom_target(
  ${FP_PARTITION_NAME}_target
  DEPENDS
  "${FP_PROVISIONING_DATA_HEX}"
  )

set_property(
  GLOBAL PROPERTY
  ${FP_PARTITION_NAME}_PM_HEX_FILE
  "${FP_PROVISIONING_DATA_HEX}"
  )

set_property(
  GLOBAL PROPERTY
  ${FP_PARTITION_NAME}_PM_TARGET
  ${FP_PARTITION_NAME}_target
  )
