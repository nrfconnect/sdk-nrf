#
# Copyright (c) 2022-2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

function(fast_pair_hex)
  set(fp_partition_name bt_fast_pair)

  set(
    fp_provisioning_data_hex
    ${CMAKE_BINARY_DIR}/modules/nrf/subsys/bluetooth/services/fast_pair/fp_provisioning_data.hex
    )

  set(fp_provisioning_data_address $<TARGET_PROPERTY:partition_manager,PM_BT_FAST_PAIR_ADDRESS>)

  add_custom_command(
    OUTPUT
    ${fp_provisioning_data_hex}
    DEPENDS
    "${APPLICATION_BINARY_DIR}/pm.config"
    COMMAND
    ${PYTHON_EXECUTABLE} ${ZEPHYR_NRF_MODULE_DIR}/scripts/nrf_provision/fast_pair/fp_provision_cli.py
                         -o ${fp_provisioning_data_hex} -a ${fp_provisioning_data_address}
                         -m ${FP_MODEL_ID} -k ${FP_ANTI_SPOOFING_KEY}
    COMMENT
    "Generating Fast Pair provisioning data hex file"
    USES_TERMINAL
    )

  add_custom_target(
    ${fp_partition_name}_target
    DEPENDS
    "${fp_provisioning_data_hex}"
    )

  set_property(
    GLOBAL PROPERTY
    ${fp_partition_name}_PM_HEX_FILE
    "${fp_provisioning_data_hex}"
    )

  set_property(
    GLOBAL PROPERTY
    ${fp_partition_name}_PM_TARGET
    ${fp_partition_name}_target
    )
endfunction()

fast_pair_hex()
