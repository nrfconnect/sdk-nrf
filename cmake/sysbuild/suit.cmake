#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
include(${CMAKE_CURRENT_LIST_DIR}/suit_utilities.cmake)

# Resolve passed absolute or relative path to real path.
#
# Usage:
#   suit_set_absolute_or_relative_path(<path> <relative_root> <output_variable>)
#
# Parameters:
#   'path' - path to resolve
#   'relative_root' - root folder used in case of relative path
#   'output_variable' - variable to store results
function(suit_set_absolute_or_relative_path path relative_root output_variable)
  if(NOT IS_ABSOLUTE ${path})
    file(REAL_PATH "${relative_root}/${path}" path)
  endif()
  set(${output_variable} "${path}" PARENT_SCOPE)
endfunction()

# Sign an envelope using SIGN_SCRIPT.
#
# Usage:
#   suit_sign_envelope(<input_file> <output_file>)
#
# Parameters:
#   'input_file' - path to input unsigned envelope
#   'output_file' - path to output signed envelope
function(suit_sign_envelope input_file output_file)
  cmake_path(GET ZEPHYR_NRF_MODULE_DIR PARENT_PATH NRF_DIR_PARENT)
  suit_set_absolute_or_relative_path(${SB_CONFIG_SUIT_ENVELOPE_SIGN_SCRIPT} ${NRF_DIR_PARENT} SIGN_SCRIPT)
  if(NOT EXISTS ${SIGN_SCRIPT})
    message(SEND_ERROR "DFU: ${SB_CONFIG_SUIT_ENVELOPE_SIGN_SCRIPT} does not exist. Corrupted configuration?")
    return()
  endif()
  set_property(
    GLOBAL APPEND PROPERTY SUIT_POST_BUILD_COMMANDS
    COMMAND ${PYTHON_EXECUTABLE} ${SIGN_SCRIPT}
    --input-file ${input_file}
    --output-file ${output_file}
  )
endfunction()

# Register SUIT post build commands.
#
# Usage:
#   suit_register_post_build_commands()
#
function(suit_register_post_build_commands)
  get_property(
    post_build_commands
    GLOBAL PROPERTY
    SUIT_POST_BUILD_COMMANDS
  )

  foreach(image ${IMAGES})
    sysbuild_get(BINARY_DIR IMAGE ${image} VAR APPLICATION_BINARY_DIR CACHE)
    sysbuild_get(BINARY_FILE IMAGE ${image} VAR CONFIG_KERNEL_BIN_NAME KCONFIG)
    list(APPEND dependencies "${BINARY_DIR}/zephyr/${BINARY_FILE}.bin")
  endforeach()

  if (SB_CONFIG_SUIT_BUILD_RECOVERY)
    list(APPEND dependencies recovery)
  endif()

  add_custom_target(
    create_suit_artifacts
    ALL
    ${post_build_commands}
    DEPENDS
    ${dependencies}
    COMMAND_EXPAND_LISTS
    COMMENT "Create SUIT artifacts"
  )
endfunction()

function(suit_generate_dfu_zip)
  get_property(
    dfu_artifacts
    GLOBAL PROPERTY
    SUIT_DFU_ARTIFACTS
  )

  set(root_name "${SB_CONFIG_SUIT_ENVELOPE_ROOT_ARTIFACT_NAME}.suit")
  set(script_params "${root_name}type=suit-envelope")

  include(${ZEPHYR_NRF_MODULE_DIR}/cmake/fw_zip.cmake)

  generate_dfu_zip(
    OUTPUT ${PROJECT_BINARY_DIR}/dfu_suit.zip
    BIN_FILES ${dfu_artifacts}
    TYPE bin
    IMAGE ${DEFAULT_IMAGE}
    DEPENDS ${create_suit_artifacts}
    SCRIPT_PARAMS "${script_params}"
  )
endfunction()

# Create DFU package/main envelope.
#
# Usage:
#   suit_create_package()
#
function(suit_create_package)
  add_custom_target(
    suit_prepare_output_folder
    ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${SUIT_ROOT_DIRECTORY}
    COMMENT
    "Create DFU output directory"
  )
  set(SUIT_OUTPUT_ARTIFACTS_ITEMS)
  set(SUIT_OUTPUT_ARTIFACTS_TARGETS)
  set(CORE_ARGS)
  set(STORAGE_BOOT_ARGS)
  sysbuild_get(app_config_dir IMAGE ${DEFAULT_IMAGE} VAR APPLICATION_CONFIG_DIR CACHE)

  if(NOT DEFINED SB_CONFIG_SUIT_ENVELOPE_SIGN)
    set(SB_CONFIG_SUIT_ENVELOPE_SIGN FALSE)
  endif()

  list(APPEND CORE_ARGS
    --core sysbuild,,,${PROJECT_BINARY_DIR}/.config
  )

  foreach(image ${IMAGES})
    unset(target)
    sysbuild_get(BINARY_DIR IMAGE ${image} VAR APPLICATION_BINARY_DIR CACHE)
    sysbuild_get(BINARY_FILE IMAGE ${image} VAR CONFIG_KERNEL_BIN_NAME KCONFIG)
    sysbuild_get(target IMAGE ${image} VAR CONFIG_SUIT_ENVELOPE_TARGET KCONFIG)

    set(BINARY_FILE "${BINARY_FILE}.bin")

    list(APPEND CORE_ARGS
      --core ${image},${SUIT_ROOT_DIRECTORY}${image}.bin,${BINARY_DIR}/zephyr/edt.pickle,${BINARY_DIR}/zephyr/.config
    )

    if(DEFINED target AND NOT target STREQUAL "")
      list(APPEND CORE_ARGS
      --core ${target},${SUIT_ROOT_DIRECTORY}${image}.bin,${BINARY_DIR}/zephyr/edt.pickle,${BINARY_DIR}/zephyr/.config
      )
    endif()
    suit_copy_artifact_to_output_directory(${image} ${BINARY_DIR}/zephyr/${BINARY_FILE})
  endforeach()

  foreach(image ${IMAGES})
    unset(GENERATE_LOCAL_ENVELOPE)
    sysbuild_get(GENERATE_LOCAL_ENVELOPE IMAGE ${image} VAR CONFIG_SUIT_LOCAL_ENVELOPE_GENERATE KCONFIG)
    if(NOT DEFINED GENERATE_LOCAL_ENVELOPE)
      continue()
    endif()

    sysbuild_get(INPUT_ENVELOPE_JINJA_FILE IMAGE ${image} VAR CONFIG_SUIT_ENVELOPE_TEMPLATE KCONFIG)
    suit_set_absolute_or_relative_path(${INPUT_ENVELOPE_JINJA_FILE} ${app_config_dir} INPUT_ENVELOPE_JINJA_FILE)
    sysbuild_get(target IMAGE ${image} VAR CONFIG_SUIT_ENVELOPE_TARGET KCONFIG)
    sysbuild_get(BINARY_DIR IMAGE ${image} VAR APPLICATION_BINARY_DIR CACHE)
    sysbuild_get(BINARY_FILE IMAGE ${image} VAR CONFIG_KERNEL_BIN_NAME KCONFIG)
    if (NOT DEFINED INPUT_ENVELOPE_JINJA_FILE OR INPUT_ENVELOPE_JINJA_FILE STREQUAL "")
      message(STATUS "DFU: Input SUIT template for ${image} is not defined. Skipping.")
      continue()
    endif()
    if(NOT DEFINED INPUT_ENVELOPE_JINJA_FILE)
      message(SEND_ERROR "DFU: Creation of SUIT artifacts failed.")
      return()
    endif()

    set(ENVELOPE_YAML_FILE ${SUIT_ROOT_DIRECTORY}${target}.yaml)
    set(ENVELOPE_SUIT_FILE ${SUIT_ROOT_DIRECTORY}${target}.suit)

    suit_render_template(${INPUT_ENVELOPE_JINJA_FILE} ${ENVELOPE_YAML_FILE} "${CORE_ARGS}")
    suit_create_envelope(${ENVELOPE_YAML_FILE} ${ENVELOPE_SUIT_FILE} ${SB_CONFIG_SUIT_ENVELOPE_SIGN})
    list(APPEND STORAGE_BOOT_ARGS
      --input-envelope ${ENVELOPE_SUIT_FILE}
    )
  endforeach()

  set(INPUT_ROOT_ENVELOPE_JINJA_FILE ${SB_CONFIG_SUIT_ENVELOPE_ROOT_TEMPLATE})

  # create root envelope if defined
  if(DEFINED INPUT_ROOT_ENVELOPE_JINJA_FILE AND NOT INPUT_ROOT_ENVELOPE_JINJA_FILE STREQUAL "")
    set(ROOT_NAME ${SB_CONFIG_SUIT_ENVELOPE_ROOT_ARTIFACT_NAME})
    suit_set_absolute_or_relative_path(${INPUT_ROOT_ENVELOPE_JINJA_FILE} ${app_config_dir} INPUT_ROOT_ENVELOPE_JINJA_FILE)
    set(ROOT_ENVELOPE_YAML_FILE ${SUIT_ROOT_DIRECTORY}${ROOT_NAME}.yaml)
    set(ROOT_ENVELOPE_SUIT_FILE ${SUIT_ROOT_DIRECTORY}${ROOT_NAME}.suit)
    suit_render_template(${INPUT_ROOT_ENVELOPE_JINJA_FILE} ${ROOT_ENVELOPE_YAML_FILE} "${CORE_ARGS}")
    suit_create_envelope(${ROOT_ENVELOPE_YAML_FILE} ${ROOT_ENVELOPE_SUIT_FILE} ${SB_CONFIG_SUIT_ENVELOPE_SIGN})
      list(APPEND STORAGE_BOOT_ARGS
        --input-envelope ${ROOT_ENVELOPE_SUIT_FILE}
      )
  endif()

  sysbuild_get(DEFAULT_BINARY_DIR IMAGE ${DEFAULT_IMAGE} VAR APPLICATION_BINARY_DIR CACHE)

  # Read SUIT storage addresses, set during MPI generation
  sysbuild_get(SUIT_STORAGE_ADDRESS IMAGE ${DEFAULT_IMAGE} VAR SUIT_STORAGE_ADDRESS CACHE)
  if (DEFINED SUIT_STORAGE_ADDRESS)
    list(APPEND STORAGE_BOOT_ARGS --storage-address ${SUIT_STORAGE_ADDRESS})
  else()
    message(WARNING "Using default value of the SUIT storage address")
  endif()

  # create all storages in the DEFAULT_IMAGE output directory
  list(APPEND STORAGE_BOOT_ARGS
    --storage-output-directory
    "${DEFAULT_BINARY_DIR}/zephyr"
    --zephyr-base ${ZEPHYR_BASE}
    --config-file "${DEFAULT_BINARY_DIR}/zephyr/.config"
    ${CORE_ARGS}
  )
  set_property(
    GLOBAL APPEND PROPERTY SUIT_POST_BUILD_COMMANDS
    COMMAND ${PYTHON_EXECUTABLE}
    ${SUIT_GENERATOR_BUILD_SCRIPT}
    storage
    ${STORAGE_BOOT_ARGS}
  )
  suit_setup_merge()
  suit_register_post_build_commands()
endfunction()

# Setup task to create final and merged artifact.
#
# Usage:
#   suit_setup_merge()
#
function(suit_setup_merge)
  sysbuild_get(BINARY_DIR IMAGE ${DEFAULT_IMAGE} VAR APPLICATION_BINARY_DIR CACHE)
  foreach(image ${IMAGES})
    set(ARTIFACTS_TO_MERGE)

    unset(GENERATE_LOCAL_ENVELOPE)
    sysbuild_get(GENERATE_LOCAL_ENVELOPE IMAGE ${image} VAR CONFIG_SUIT_LOCAL_ENVELOPE_GENERATE KCONFIG)
    if(NOT DEFINED GENERATE_LOCAL_ENVELOPE)
      continue()
    endif()

    sysbuild_get(IMAGE_BINARY_DIR IMAGE ${image} VAR APPLICATION_BINARY_DIR CACHE)
    sysbuild_get(IMAGE_BINARY_FILE IMAGE ${image} VAR CONFIG_KERNEL_BIN_NAME KCONFIG)
    sysbuild_get(IMAGE_TARGET_NAME IMAGE ${image} VAR CONFIG_SUIT_ENVELOPE_TARGET KCONFIG)
    if (NOT DEFINED IMAGE_TARGET_NAME OR IMAGE_TARGET_NAME STREQUAL "")
      message(STATUS "DFU: Target name for ${image} is not defined. Skipping.")
      continue()
    endif()

    sysbuild_get(CONFIG_SUIT_ENVELOPE_OUTPUT_ARTIFACT IMAGE ${image} VAR CONFIG_SUIT_ENVELOPE_OUTPUT_ARTIFACT KCONFIG)
    sysbuild_get(CONFIG_NRF_REGTOOL_GENERATE_UICR IMAGE ${image} VAR CONFIG_NRF_REGTOOL_GENERATE_UICR KCONFIG)
    sysbuild_get(CONFIG_SUIT_ENVELOPE_OUTPUT_MPI_MERGE IMAGE ${image} VAR CONFIG_SUIT_ENVELOPE_OUTPUT_MPI_MERGE KCONFIG)

    set(OUTPUT_HEX_FILE "${IMAGE_BINARY_DIR}/zephyr/${CONFIG_SUIT_ENVELOPE_OUTPUT_ARTIFACT}")

    list(APPEND ARTIFACTS_TO_MERGE ${BINARY_DIR}/zephyr/suit_installed_envelopes_${IMAGE_TARGET_NAME}_merged.hex)
    list(APPEND ARTIFACTS_TO_MERGE ${IMAGE_BINARY_DIR}/zephyr/${IMAGE_BINARY_FILE}.hex)
    if(CONFIG_SUIT_ENVELOPE_OUTPUT_MPI_MERGE)
      list(APPEND ARTIFACTS_TO_MERGE ${BINARY_DIR}/zephyr/suit_mpi_${IMAGE_TARGET_NAME}_merged.hex)
    endif()
    if(CONFIG_NRF_REGTOOL_GENERATE_UICR)
      list(APPEND ARTIFACTS_TO_MERGE ${IMAGE_BINARY_DIR}/zephyr/uicr.hex)
    endif()

    if(SB_CONFIG_SUIT_BUILD_RECOVERY)
      get_property(
        recovery_artifacts
        GLOBAL PROPERTY
        SUIT_RECOVERY_ARTIFACTS_TO_MERGE_${IMAGE_TARGET_NAME}
      )

      list(APPEND ARTIFACTS_TO_MERGE ${recovery_artifacts})
    endif()

    set_property(
      GLOBAL APPEND PROPERTY SUIT_POST_BUILD_COMMANDS
      COMMAND ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/scripts/build/mergehex.py
      --overlap replace
      -o ${OUTPUT_HEX_FILE}
       ${ARTIFACTS_TO_MERGE}
      # fixme: uicr_merged is overwritten by new content, runners_yaml_props_target could be used to define
      #     what shall be flashed, but not sure where to set this! Remove --overlap if ready!
      #     example usage: set_property(TARGET runners_yaml_props_target PROPERTY hex_file ${merged_hex_file})
       COMMAND ${CMAKE_COMMAND} -E copy ${OUTPUT_HEX_FILE} ${IMAGE_BINARY_DIR}/zephyr/uicr_merged.hex
    )
  endforeach()
endfunction()

# Build recovery image.
#
# Usage:
#   suit_build_recovery()
#
function(suit_build_recovery)
  include(ExternalProject)

  set(sysbuild_root_path "${ZEPHYR_NRF_MODULE_DIR}/../zephyr/share/sysbuild")
  set(board_target "${SB_CONFIG_BOARD}/${target_soc}/cpuapp")

  # Get class and vendor ID-s to pass to recovery application
  sysbuild_get(APP_RECOVERY_VENDOR_NAME IMAGE ${DEFAULT_IMAGE} VAR CONFIG_SUIT_MPI_APP_RECOVERY_VENDOR_NAME KCONFIG)
  sysbuild_get(APP_RECOVERY_CLASS_NAME IMAGE ${DEFAULT_IMAGE} VAR CONFIG_SUIT_MPI_APP_RECOVERY_CLASS_NAME KCONFIG)
  sysbuild_get(RAD_RECOVERY_VENDOR_NAME IMAGE ${DEFAULT_IMAGE} VAR CONFIG_SUIT_MPI_RAD_RECOVERY_VENDOR_NAME KCONFIG)
  sysbuild_get(RAD_RECOVERY_CLASS_NAME IMAGE ${DEFAULT_IMAGE} VAR CONFIG_SUIT_MPI_RAD_RECOVERY_CLASS_NAME KCONFIG)

  ExternalProject_Add(
    recovery
    SOURCE_DIR ${sysbuild_root_path}
    PREFIX ${CMAKE_BINARY_DIR}/recovery
    INSTALL_COMMAND ""
    CMAKE_CACHE_ARGS
      "-DAPP_DIR:PATH=${ZEPHYR_NRF_MODULE_DIR}/samples/suit/recovery"
      "-DBOARD:STRING=${board_target}"
      "-DEXTRA_DTC_OVERLAY_FILE:STRING=${APP_DIR}/sysbuild/recovery.overlay"
      "-Dhci_ipc_EXTRA_DTC_OVERLAY_FILE:STRING=${APP_DIR}/sysbuild/recovery_hci_ipc.overlay"
      "-DSB_CONFIG_SUIT_ENVELOPE_ROOT_TEMPLATE:STRING=\\\"${ZEPHYR_NRF_MODULE_DIR}/samples/suit/recovery/app_recovery_envelope.yaml.jinja2\\\""
      "-Dhci_ipc_CONFIG_SUIT_ENVELOPE_TEMPLATE:STRING=\\\"${ZEPHYR_NRF_MODULE_DIR}/samples/suit/recovery/rad_recovery_envelope.yaml.jinja2\\\""
      "-DCONFIG_SUIT_MPI_APP_RECOVERY_VENDOR_NAME:STRING=\\\"${APP_RECOVERY_VENDOR_NAME}\\\""
      "-DCONFIG_SUIT_MPI_APP_RECOVERY_CLASS_NAME:STRING=\\\"${APP_RECOVERY_CLASS_NAME}\\\""
      "-DCONFIG_SUIT_MPI_RAD_RECOVERY_VENDOR_NAME:STRING=\\\"${RAD_RECOVERY_VENDOR_NAME}\\\""
      "-DCONFIG_SUIT_MPI_RAD_RECOVERY_CLASS_NAME:STRING=\\\"${RAD_RECOVERY_CLASS_NAME}\\\""
    BUILD_ALWAYS True
)
  ExternalProject_Get_property(recovery BINARY_DIR)

  set_property(
    GLOBAL APPEND PROPERTY SUIT_RECOVERY_ARTIFACTS_TO_MERGE_application
    ${BINARY_DIR}/recovery/zephyr/suit_installed_envelopes_application_merged.hex)
  set_property(
    GLOBAL APPEND PROPERTY SUIT_RECOVERY_ARTIFACTS_TO_MERGE_application ${BINARY_DIR}/recovery/zephyr/zephyr.hex)

  set_property(
    GLOBAL APPEND PROPERTY SUIT_RECOVERY_ARTIFACTS_TO_MERGE_radio
    ${BINARY_DIR}/recovery/zephyr/suit_installed_envelopes_radio_merged.hex)
  set_property(
    GLOBAL APPEND PROPERTY SUIT_RECOVERY_ARTIFACTS_TO_MERGE_radio ${BINARY_DIR}/hci_ipc/zephyr/zephyr.hex)

endfunction()

if(SB_CONFIG_SUIT_BUILD_RECOVERY)
  suit_build_recovery()
endif()

if(SB_CONFIG_SUIT_ENVELOPE)
  suit_create_package()
  suit_generate_dfu_zip()
endif()
