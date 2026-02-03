#
# Copyright (c) 2022-2025 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

function(fast_pair_hex_pm)
  set(fp_partition_name bt_fast_pair)

  set(fp_partition_pm_config_name $<TARGET_PROPERTY:partition_manager,PM_BT_FAST_PAIR_NAME>)

  add_custom_target(
    fast_pair_hex_pm_partition_validation
    ALL
    DEPENDS
    ${APPLICATION_BINARY_DIR}/pm.config
    COMMAND
    ${CMAKE_COMMAND} -D FP_PARTITION_NAME=${fp_partition_name} -D FP_PARTITION_PM_CONFIG_NAME=${fp_partition_pm_config_name} -P ${ZEPHYR_NRF_MODULE_DIR}/cmake/sysbuild/fast_pair/pm_partition_validation.cmake
    WORKING_DIRECTORY
    ${APPLICATION_BINARY_DIR}
    COMMENT
    "Validating Fast Pair partition configuration for the provisioning data hex file"
  )

  set(
    fp_provisioning_data_hex
    ${CMAKE_BINARY_DIR}/modules/nrf/subsys/bluetooth/services/fast_pair/fp_provisioning_data.hex
    )

  set(fp_provisioning_data_address $<TARGET_PROPERTY:partition_manager,PM_BT_FAST_PAIR_ADDRESS>)

  add_custom_command(
    OUTPUT
    ${fp_provisioning_data_hex}
    DEPENDS
    fast_pair_hex_pm_partition_validation
    COMMAND
    ${PYTHON_EXECUTABLE} ${ZEPHYR_NRF_MODULE_DIR}/scripts/nrf_provision/fast_pair/fp_provision_cli.py
                         -o ${fp_provisioning_data_hex}
                         -a ${fp_provisioning_data_address}
                         -m ${SB_CONFIG_BT_FAST_PAIR_MODEL_ID}
                         -k ${SB_CONFIG_BT_FAST_PAIR_ANTI_SPOOFING_PRIVATE_KEY}
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

function(fast_pair_hex_dts)
  set(fp_partition_name bt_fast_pair_partition)

  dt_nodelabel(
    fp_partition_node_full_path
    TARGET
    ${DEFAULT_IMAGE}
    NODELABEL
    "${fp_partition_name}"
    )

  if(NOT fp_partition_node_full_path)
    message(FATAL_ERROR
            "The `bt_fast_pair_partition` partition node is not defined in DTS. "
            "Please define the Fast Pair partition to use the automatic provisioning "
            "data generation with the SB_CONFIG_BT_FAST_PAIR_PROV_DATA Kconfig.")
  endif()

  dt_reg_addr(
    fp_provisioning_data_address
    TARGET
    ${DEFAULT_IMAGE}
    PATH
    "${fp_partition_node_full_path}"
    )

  set(
    fp_provisioning_data_hex
    ${CMAKE_BINARY_DIR}/modules/nrf/subsys/bluetooth/services/fast_pair/fp_provisioning_data.hex
    )

  add_custom_command(
    OUTPUT
    ${fp_provisioning_data_hex}
    COMMAND
    ${PYTHON_EXECUTABLE} ${ZEPHYR_NRF_MODULE_DIR}/scripts/nrf_provision/fast_pair/fp_provision_cli.py
                         -o ${fp_provisioning_data_hex}
                         -a ${fp_provisioning_data_address}
                         -m ${SB_CONFIG_BT_FAST_PAIR_MODEL_ID}
                         -k ${SB_CONFIG_BT_FAST_PAIR_ANTI_SPOOFING_PRIVATE_KEY}
    COMMENT
    "Generating Fast Pair provisioning data hex file"
    USES_TERMINAL
    )

  add_custom_target(
    ${fp_partition_name}_target
    ALL
    DEPENDS
    "${fp_provisioning_data_hex}"
    )
endfunction()

function(fast_pair_device_model_warning)
  # Emit a warning when using a Nordic owned demo Fast Pair device Model ID.
  # These Model IDs are used by Fast Pair applications/samples in the nRF Connect SDK.
  if(SB_CONFIG_BT_FAST_PAIR_MODEL_ID EQUAL 0x2A410B OR
     SB_CONFIG_BT_FAST_PAIR_MODEL_ID EQUAL 0x4A436B OR
     SB_CONFIG_BT_FAST_PAIR_MODEL_ID EQUAL 0x52FF02 OR
     SB_CONFIG_BT_FAST_PAIR_MODEL_ID EQUAL 0x8E717D)
    message(WARNING "
    -------------------------------------------------------
    --- WARNING: Using demo Fast Pair Model ID and Fast ---
    --- Pair Anti-Spoofing Key, it should not be used   ---
    --- for production.                                 ---
    -------------------------------------------------------
    \n"
    )
  endif()
endfunction()

fast_pair_device_model_warning()
if(SB_CONFIG_PARTITION_MANAGER)
  fast_pair_hex_pm()
else()
  fast_pair_hex_dts()
endif()
