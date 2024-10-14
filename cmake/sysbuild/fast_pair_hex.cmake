#
# Copyright (c) 2022-2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

function(fast_pair_hex_pm)
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

function(fast_pair_hex_dts)
  if(NOT SB_CONFIG_SOC_SERIES_NRF54HX)
    message(FATAL_ERROR "Fast Pair data provisioning using DTS partitions is only supported"
                        "for nRF54H series.")
  endif()

  set(fp_partition_name bt_fast_pair_partition)

  sysbuild_dt_nodelabel(
    bt_fast_pair_partition_nodelabel
    IMAGE
    ${DEFAULT_IMAGE}
    NODELABEL
    "${fp_partition_name}"
    )
  sysbuild_dt_reg_addr(
    bt_fast_pair_partition_relative_address
    IMAGE
    ${DEFAULT_IMAGE}
    PATH
    "${bt_fast_pair_partition_nodelabel}"
    )

  # This part assumes that the Fast Pair partition resides in MRAM1x.
  # TODO: Rewrite it to be more generic and support other memory regions.
  sysbuild_dt_nodelabel(mram1x_nodelabel IMAGE ${DEFAULT_IMAGE} NODELABEL "mram1x")
  sysbuild_dt_reg_addr(mram1x_address IMAGE ${DEFAULT_IMAGE} PATH "${mram1x_nodelabel}")

  math(
    EXPR
    bt_fast_pair_partition_address
    "${mram1x_address} + ${bt_fast_pair_partition_relative_address}"
    OUTPUT_FORMAT
    HEXADECIMAL)

  set(
    fp_provisioning_data_hex
    ${CMAKE_BINARY_DIR}/modules/nrf/subsys/bluetooth/services/fast_pair/fp_provisioning_data.hex
    )

  set(fp_provisioning_data_address ${bt_fast_pair_partition_address})

  add_custom_command(
    OUTPUT
    ${fp_provisioning_data_hex}
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
    ALL
    DEPENDS
    "${fp_provisioning_data_hex}"
    )

  # TODO: Integrate the generated hex file into the merged hex file chain using a dedicated
  #       CMake function like the one suggested in the following draft PR:
  #       https://github.com/nrfconnect/sdk-nrf/pull/16425.
  #       Code below is highly experimental and needs to be improved.
  set(OUTPUT_FILE_PATH ${APPLICATION_BINARY_DIR}/${DEFAULT_IMAGE}/zephyr)

  add_custom_command(
    OUTPUT
    ${OUTPUT_FILE_PATH}/uicr_merged_fast_pair.hex
    COMMAND
    ${PYTHON_EXECUTABLE}
    ${ZEPHYR_BASE}/scripts/build/mergehex.py
    -o ${OUTPUT_FILE_PATH}/uicr_merged_fast_pair.hex
    ${OUTPUT_FILE_PATH}/uicr_merged.hex
    ${fp_provisioning_data_hex}
    DEPENDS
    ${fp_partition_name}_target
    create_suit_artifacts
    ${fp_provisioning_data_hex}
    COMMENT
    "Merging Fast Pair provisioning data hex file"
    USES_TERMINAL
    )

  add_custom_target(
    ${fp_partition_name}_hex_target
    ALL
    COMMAND ${CMAKE_COMMAND} -E copy ${OUTPUT_FILE_PATH}/uicr_merged_fast_pair.hex ${OUTPUT_FILE_PATH}/uicr_merged.hex
    DEPENDS
    ${OUTPUT_FILE_PATH}/uicr_merged_fast_pair.hex
    )
endfunction()

function(fast_pair_hex)
  if(SB_CONFIG_PARTITION_MANAGER)
    fast_pair_hex_pm()
  else()
    fast_pair_hex_dts()
  endif()
endfunction()

fast_pair_hex()
