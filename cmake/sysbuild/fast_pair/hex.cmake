#
# Copyright (c) 2022-2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

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
    ${CMAKE_BINARY_DIR}/modules/nrf/subsys/bluetooth/fast_pair/fp_provisioning_data.hex
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

    if(SB_CONFIG_MERGED_HEX_FILES)
      set(board_target)
      sysbuild_get(board_target IMAGE ${DEFAULT_IMAGE} VAR CONFIG_BOARD_TARGET KCONFIG)
      string(REPLACE "/" "_" board_target ${board_target})
      string(REPLACE "@" "_" board_target ${board_target})

      set_property(GLOBAL APPEND
        PROPERTY sysbuild_merged_hex_dependencies_${board_target} ${fp_partition_name}_target
      )
    endif()
#message(WARNING "added ${fp_partition_name}_target to sysbuild_merged_hex_dependencies_${board_target}")
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
fast_pair_hex_dts()
