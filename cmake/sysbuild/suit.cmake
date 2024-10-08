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

  if(SB_CONFIG_SUIT_BUILD_RECOVERY)
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
  get_property(
    additional_script_params
    GLOBAL PROPERTY
    SUIT_DFU_ZIP_ADDITIONAL_SCRIPT_PARAMS
  )

  set(root_name "${SB_CONFIG_SUIT_ENVELOPE_ROOT_ARTIFACT_NAME}.suit")
  set(script_params "${root_name}type=suit-envelope;${additional_script_params}")

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

function(suit_generate_recovery_dfu_zip)
  get_property(
    dfu_artifacts
    GLOBAL PROPERTY
    SUIT_RECOVERY_DFU_ARTIFACTS
  )
  get_property(
    additional_script_params
    GLOBAL PROPERTY
    SUIT_RECOVERY_DFU_ZIP_ADDITIONAL_SCRIPT_PARAMS
  )

  set(root_name "${SB_CONFIG_SUIT_ENVELOPE_APP_RECOVERY_ARTIFACT_NAME}.suit")
  set(script_params "${root_name}type=suit-envelope;${additional_script_params}")

  include(${ZEPHYR_NRF_MODULE_DIR}/cmake/fw_zip.cmake)

  generate_dfu_zip(
    OUTPUT ${PROJECT_BINARY_DIR}/dfu_suit_recovery.zip
    BIN_FILES ${dfu_artifacts}
    TYPE bin
    IMAGE ${DEFAULT_IMAGE}
    DEPENDS ${create_suit_artifacts}
    SCRIPT_PARAMS "${script_params}"
  )
endfunction()

# Get path to the manifest template in the base manifest template directory.
#
# Usage:
#   suit_make_base_manifest_path(template_name output_variable)
#
# Parameters:
#   template_name - Name of the base manifest template
#   output_variable - CMake variable where the path to the base manifest template will be stored.
#
function(suit_make_base_manifest_path template_name output_variable)
  set(${output_variable} "${SB_CONFIG_SUIT_BASE_MANIFEST_TEMPLATE_DIR}/${SB_CONFIG_SOC}/${SB_CONFIG_SUIT_BASE_MANIFEST_VARIANT}/${template_name}" PARENT_SCOPE)
endfunction()

# Get path to the manifest template in the application manifest template directory.
#
# Usage:
#   suit_make_custom_manifest_path(template_name output_variable)
#
# Parameters:
#   template_name - Name of the base manifest template
#   output_variable - CMake variable where the path to the base manifest template will be stored.
#
function(suit_make_custom_manifest_path template_name output_variable)
  sysbuild_get(app_config_dir IMAGE ${DEFAULT_IMAGE} VAR APPLICATION_CONFIG_DIR CACHE)
  set(${output_variable} "${app_config_dir}/suit/${SB_CONFIG_SOC}/${template_name}" PARENT_SCOPE)
endfunction()

# Get path to the manifest template in the recovery application manifest template directory.
#
# Usage:
#   suit_make_custom_recovery_manifest_path(template_name output_variable)
#
# Parameters:
#   template_name - Name of the base manifest template
#   output_variable - CMake variable where the path to the base manifest template will be stored.
#
function(suit_make_custom_recovery_manifest_path template_name output_variable)
  set(${output_variable} "${SB_CONFIG_SUIT_RECOVERY_APPLICATION_PATH}/suit/${SB_CONFIG_SOC}/${template_name}" PARENT_SCOPE)
endfunction()

# Get the path to the manifest template.
#
# The function searches for the manifest templates in the application manifest template directory.
# If such template does not exist, it searches for the template in the base manifest template
# directory.
#
# Usage:
#   suit_get_manifest(template_name output_variable)
#
# Parameters:
#   template_name - Name of the base manifest template
#   output_variable - CMake variable where the path to the base manifest template will be stored.
#
function(suit_get_manifest template_name output_variable)
  suit_make_custom_manifest_path(${template_name} custom_path)
  suit_make_custom_recovery_manifest_path(${template_name} custom_recovery_path)
  suit_make_base_manifest_path(${template_name} base_path)
  if(EXISTS ${custom_path})
    set(${output_variable} ${custom_path} PARENT_SCOPE)
  elseif(SB_CONFIG_SUIT_BUILD_RECOVERY AND EXISTS ${custom_recovery_path})
    set(${output_variable} ${custom_recovery_path} PARENT_SCOPE)
  else()
    set(${output_variable} ${base_path} PARENT_SCOPE)
  endif()
endfunction()

# Find a VERSION file path.
#
# Usage:
#   suit_get_version_file(<output_variable>)
#
# Parameters:
#   'output_variable' - variable to store path
function(suit_get_version_file output_variable)
  sysbuild_get(PROJECT_DIR IMAGE ${DEFAULT_IMAGE} VAR APPLICATION_SOURCE_DIR CACHE)

  if(DEFINED VERSION_FILE)
    set(${output_variable} "${VERSION_FILE}" PARENT_SCOPE)
    message(STATUS "Using version: ${VERSION_FILE}")
  elseif((DEFINED PROJECT_DIR) AND (EXISTS ${PROJECT_DIR}/VERSION))
    set(${output_variable} "${PROJECT_DIR}/VERSION" PARENT_SCOPE)
    message(STATUS "Using version: ${PROJECT_DIR}/VERSION")
  else()
    message(STATUS "Unable to find a VERSION file. Using default values (seq: 1, no semver)")
  endif()
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

    unset(CONFIG_SUIT_RECOVERY)
    sysbuild_get(CONFIG_SUIT_RECOVERY IMAGE ${image} VAR CONFIG_SUIT_RECOVERY KCONFIG)
    if(CONFIG_SUIT_RECOVERY)
      set_property(GLOBAL APPEND PROPERTY SUIT_RECOVERY_DFU_ARTIFACTS ${SUIT_ROOT_DIRECTORY}${image}.bin)
    else()
      set_property(GLOBAL APPEND PROPERTY SUIT_DFU_ARTIFACTS ${SUIT_ROOT_DIRECTORY}${image}.bin)
    endif()
  endforeach()

  set(TEMPLATE_ARGS ${CORE_ARGS})
  suit_get_version_file(VERSION_PATH)
  if(DEFINED VERSION_PATH)
    list(APPEND TEMPLATE_ARGS
      --version_file ${VERSION_PATH}
    )
  endif()

  foreach(image ${IMAGES})
    unset(GENERATE_LOCAL_ENVELOPE)
    sysbuild_get(GENERATE_LOCAL_ENVELOPE IMAGE ${image} VAR CONFIG_SUIT_LOCAL_ENVELOPE_GENERATE KCONFIG)
    if(NOT DEFINED GENERATE_LOCAL_ENVELOPE)
      continue()
    endif()

    sysbuild_get(INPUT_ENVELOPE_JINJA_FILE IMAGE ${image} VAR CONFIG_SUIT_ENVELOPE_TEMPLATE_FILENAME KCONFIG)
    string(CONFIGURE "${INPUT_ENVELOPE_JINJA_FILE}" INPUT_ENVELOPE_JINJA_FILE)
    suit_get_manifest(${INPUT_ENVELOPE_JINJA_FILE} INPUT_ENVELOPE_JINJA_FILE)

    message(STATUS "Found ${image} manifest template: ${INPUT_ENVELOPE_JINJA_FILE}")

    suit_set_absolute_or_relative_path(${INPUT_ENVELOPE_JINJA_FILE} ${app_config_dir} INPUT_ENVELOPE_JINJA_FILE)
    sysbuild_get(target IMAGE ${image} VAR CONFIG_SUIT_ENVELOPE_TARGET KCONFIG)
    sysbuild_get(BINARY_DIR IMAGE ${image} VAR APPLICATION_BINARY_DIR CACHE)
    sysbuild_get(BINARY_FILE IMAGE ${image} VAR CONFIG_KERNEL_BIN_NAME KCONFIG)
    if(NOT DEFINED INPUT_ENVELOPE_JINJA_FILE OR INPUT_ENVELOPE_JINJA_FILE STREQUAL "")
      message(STATUS "DFU: Input SUIT template for ${image} is not defined. Skipping.")
      continue()
    endif()
    if(NOT DEFINED INPUT_ENVELOPE_JINJA_FILE)
      message(SEND_ERROR "DFU: Creation of SUIT artifacts failed.")
      return()
    endif()

    set(ENVELOPE_YAML_FILE ${SUIT_ROOT_DIRECTORY}${target}.yaml)
    set(ENVELOPE_SUIT_FILE ${SUIT_ROOT_DIRECTORY}${target}.suit)

    suit_render_template(${INPUT_ENVELOPE_JINJA_FILE} ${ENVELOPE_YAML_FILE} "${TEMPLATE_ARGS}")
    suit_create_envelope(${ENVELOPE_YAML_FILE} ${ENVELOPE_SUIT_FILE} ${SB_CONFIG_SUIT_ENVELOPE_SIGN})

    unset(CONFIG_SUIT_RECOVERY)
    sysbuild_get(CONFIG_SUIT_RECOVERY IMAGE ${image} VAR CONFIG_SUIT_RECOVERY KCONFIG)
    if(CONFIG_SUIT_RECOVERY)
      set_property(GLOBAL APPEND PROPERTY SUIT_RECOVERY_DFU_ARTIFACTS ${ENVELOPE_SUIT_FILE})
    else()
      set_property(GLOBAL APPEND PROPERTY SUIT_DFU_ARTIFACTS ${ENVELOPE_SUIT_FILE})
    endif()

    list(APPEND STORAGE_BOOT_ARGS
      --input-envelope ${ENVELOPE_SUIT_FILE}
    )
  endforeach()

  # First parse which images should be extracted to which cache partitions
  set(DFU_CACHE_PARTITIONS_USED "")
  set(RECOVERY_DFU_CACHE_PARTITIONS_USED "")

  foreach(image ${IMAGES})
    sysbuild_get(EXTRACT_TO_CACHE IMAGE ${image} VAR CONFIG_SUIT_DFU_CACHE_EXTRACT_IMAGE KCONFIG)
    if(EXTRACT_TO_CACHE)
      sysbuild_get(CACHE_PARTITION_NUM IMAGE ${image} VAR CONFIG_SUIT_DFU_CACHE_EXTRACT_IMAGE_PARTITION KCONFIG)

      unset(CONFIG_SUIT_RECOVERY)
      sysbuild_get(CONFIG_SUIT_RECOVERY IMAGE ${image} VAR CONFIG_SUIT_RECOVERY KCONFIG)

      if(CONFIG_SUIT_RECOVERY)
        list(APPEND RECOVERY_DFU_CACHE_PARTITIONS_USED ${CACHE_PARTITION_NUM})
        list(APPEND SUIT_RECOVERY_CACHE_PARTITION_${CACHE_PARTITION_NUM} ${image})
      else()
        list(APPEND DFU_CACHE_PARTITIONS_USED ${CACHE_PARTITION_NUM})
        list(APPEND SUIT_CACHE_PARTITION_${CACHE_PARTITION_NUM} ${image})
      endif()
    endif()
  endforeach()

  list(REMOVE_DUPLICATES DFU_CACHE_PARTITIONS_USED)
  list(REMOVE_DUPLICATES RECOVERY_DFU_CACHE_PARTITIONS_USED)

  # Then create the cache partitions
  foreach(CACHE_PARTITION_NUM ${DFU_CACHE_PARTITIONS_USED})
    set(CACHE_CREATE_ARGS "")
    foreach(image ${SUIT_CACHE_PARTITION_${CACHE_PARTITION_NUM}})
      sysbuild_get(BINARY_DIR IMAGE ${image} VAR APPLICATION_BINARY_DIR CACHE)
      sysbuild_get(BINARY_FILE IMAGE ${image} VAR CONFIG_KERNEL_BIN_NAME KCONFIG)
      sysbuild_get(IMAGE_CACHE_URI IMAGE ${image} VAR CONFIG_SUIT_DFU_CACHE_EXTRACT_IMAGE_URI KCONFIG)
      list(APPEND CACHE_CREATE_ARGS
        "--input" "\"${IMAGE_CACHE_URI},${BINARY_DIR}/zephyr/${BINARY_FILE}.bin\""
      )
    endforeach()

    if(SUIT_DFU_CACHE_PARTITION_${CACHE_PARTITION_NUM}_EB_SIZE)
      list(APPEND CACHE_CREATE_ARGS "--eb-size" "${SUIT_DFU_CACHE_PARTITION_${CACHE_PARTITION_NUM}_EB_SIZE}")
    endif()

    suit_create_cache_partition(
      "${CACHE_CREATE_ARGS}"
      "${SUIT_ROOT_DIRECTORY}dfu_cache_partition_${CACHE_PARTITION_NUM}.bin"
      ${CACHE_PARTITION_NUM}
      FALSE
    )
  endforeach()

  if(SB_CONFIG_SUIT_BUILD_RECOVERY)

    # Create cache partitions for the recovery images
    foreach(CACHE_PARTITION_NUM ${RECOVERY_DFU_CACHE_PARTITIONS_USED})
      set(CACHE_CREATE_ARGS "")
      foreach(image ${SUIT_RECOVERY_CACHE_PARTITION_${CACHE_PARTITION_NUM}})
        sysbuild_get(BINARY_DIR IMAGE ${image} VAR APPLICATION_BINARY_DIR CACHE)
        sysbuild_get(BINARY_FILE IMAGE ${image} VAR CONFIG_KERNEL_BIN_NAME KCONFIG)
        sysbuild_get(IMAGE_CACHE_URI IMAGE ${image} VAR CONFIG_SUIT_DFU_CACHE_EXTRACT_IMAGE_URI KCONFIG)
        list(APPEND CACHE_CREATE_ARGS
          "--input" "\"${IMAGE_CACHE_URI},${BINARY_DIR}/zephyr/${BINARY_FILE}.bin\""
        )
      endforeach()

      if(SUIT_DFU_CACHE_PARTITION_${CACHE_PARTITION_NUM}_EB_SIZE)
        list(APPEND CACHE_CREATE_ARGS "--eb-size" "${SUIT_DFU_CACHE_PARTITION_${CACHE_PARTITION_NUM}_EB_SIZE}")
      endif()

      suit_create_cache_partition(
        "${CACHE_CREATE_ARGS}"
        "${SUIT_ROOT_DIRECTORY}dfu_cache_partition_recovery_${CACHE_PARTITION_NUM}.bin"
        ${CACHE_PARTITION_NUM}
        TRUE
      )
    endforeach()

    suit_get_manifest(${SB_CONFIG_SUIT_ENVELOPE_APP_RECOVERY_TEMPLATE_FILENAME} INPUT_APP_RECOVERY_ENVELOPE_JINJA_FILE)

    # create app recovery envelope if defined
    if(DEFINED INPUT_APP_RECOVERY_ENVELOPE_JINJA_FILE AND NOT INPUT_APP_RECOVERY_ENVELOPE_JINJA_FILE STREQUAL "")
      message(STATUS "Found app_recovery manifest template: ${INPUT_APP_RECOVERY_ENVELOPE_JINJA_FILE}")
      set(APP_RECOVERY_NAME ${SB_CONFIG_SUIT_ENVELOPE_APP_RECOVERY_ARTIFACT_NAME})
      string(CONFIGURE "${INPUT_APP_RECOVERY_ENVELOPE_JINJA_FILE}" INPUT_APP_RECOVERY_ENVELOPE_JINJA_FILE)
      suit_set_absolute_or_relative_path(${INPUT_APP_RECOVERY_ENVELOPE_JINJA_FILE} ${app_config_dir} INPUT_APP_RECOVERY_ENVELOPE_JINJA_FILE)
      set(APP_RECOVERY_ENVELOPE_YAML_FILE ${SUIT_ROOT_DIRECTORY}${APP_RECOVERY_NAME}.yaml)
      set(APP_RECOVERY_ENVELOPE_SUIT_FILE ${SUIT_ROOT_DIRECTORY}${APP_RECOVERY_NAME}.suit)
      suit_render_template(${INPUT_APP_RECOVERY_ENVELOPE_JINJA_FILE} ${APP_RECOVERY_ENVELOPE_YAML_FILE} "${TEMPLATE_ARGS}")
      suit_create_envelope(${APP_RECOVERY_ENVELOPE_YAML_FILE} ${APP_RECOVERY_ENVELOPE_SUIT_FILE} ${SB_CONFIG_SUIT_ENVELOPE_SIGN})
        list(APPEND STORAGE_BOOT_ARGS
          --input-envelope ${APP_RECOVERY_ENVELOPE_SUIT_FILE}
        )
      set_property(GLOBAL APPEND PROPERTY SUIT_RECOVERY_DFU_ARTIFACTS ${APP_RECOVERY_ENVELOPE_SUIT_FILE})
    endif()
  endif()

  if(SB_CONFIG_SUIT_MULTI_IMAGE_PACKAGE_BUILD)
    include(${ZEPHYR_NRF_MODULE_DIR}/cmake/fw_zip.cmake)
    include(${ZEPHYR_NRF_MODULE_DIR}/cmake/dfu_multi_image.cmake)

    set(suit_multi_image_ids)
    set(suit_multi_image_paths)
    set(suit_multi_image_targets)

    list(APPEND suit_multi_image_ids 0)
    # Include the suit Envelope to the multi image package to store it in dfu_partition
    list(APPEND suit_multi_image_paths "${SUIT_ROOT_DIRECTORY}root.suit")
    list(APPEND suit_multi_image_targets "${SUIT_ROOT_DIRECTORY}root.suit")
    # Include cache partition to the multi image package to store it in cache_partition
    foreach(cache_partition_num ${DFU_CACHE_PARTITIONS_USED})
      math(EXPR dfu_image_id "${cache_partition_num} + 1")
      list(APPEND suit_multi_image_ids ${dfu_image_id})
      list(APPEND suit_multi_image_paths "${SUIT_ROOT_DIRECTORY}dfu_cache_partition_${cache_partition_num}.bin")
    endforeach()

    dfu_multi_image_package(
      dfu_multi_image_pkg
      IMAGE_IDS ${suit_multi_image_ids}
      IMAGE_PATHS ${suit_multi_image_paths}
      OUTPUT ${CMAKE_BINARY_DIR}/dfu_multi_image.bin
      DEPENDS ${suit_multi_image_targets}
    )
  endif() # SB_CONFIG_SUIT_MULTI_IMAGE_PACKAGE_BUILD

  suit_get_manifest(${SB_CONFIG_SUIT_ENVELOPE_ROOT_TEMPLATE_FILENAME} INPUT_ROOT_ENVELOPE_JINJA_FILE)
  message(STATUS "Found root manifest template: ${INPUT_ROOT_ENVELOPE_JINJA_FILE}")

  # create root envelope if defined
  if(DEFINED INPUT_ROOT_ENVELOPE_JINJA_FILE AND NOT INPUT_ROOT_ENVELOPE_JINJA_FILE STREQUAL "")
    set(ROOT_NAME ${SB_CONFIG_SUIT_ENVELOPE_ROOT_ARTIFACT_NAME})
    string(CONFIGURE "${INPUT_ROOT_ENVELOPE_JINJA_FILE}" INPUT_ROOT_ENVELOPE_JINJA_FILE)
    suit_set_absolute_or_relative_path(${INPUT_ROOT_ENVELOPE_JINJA_FILE} ${app_config_dir} INPUT_ROOT_ENVELOPE_JINJA_FILE)
    set(ROOT_ENVELOPE_YAML_FILE ${SUIT_ROOT_DIRECTORY}${ROOT_NAME}.yaml)
    set(ROOT_ENVELOPE_SUIT_FILE ${SUIT_ROOT_DIRECTORY}${ROOT_NAME}.suit)
    suit_render_template(${INPUT_ROOT_ENVELOPE_JINJA_FILE} ${ROOT_ENVELOPE_YAML_FILE} "${TEMPLATE_ARGS}")
    suit_create_envelope(${ROOT_ENVELOPE_YAML_FILE} ${ROOT_ENVELOPE_SUIT_FILE} ${SB_CONFIG_SUIT_ENVELOPE_SIGN})
      list(APPEND STORAGE_BOOT_ARGS
        --input-envelope ${ROOT_ENVELOPE_SUIT_FILE}
      )
    set_property(GLOBAL APPEND PROPERTY SUIT_DFU_ARTIFACTS ${ROOT_ENVELOPE_SUIT_FILE})
  endif()

  sysbuild_get(DEFAULT_BINARY_DIR IMAGE ${DEFAULT_IMAGE} VAR APPLICATION_BINARY_DIR CACHE)

  # Read SUIT storage addresses, set during MPI generation
  sysbuild_get(SUIT_STORAGE_ADDRESS IMAGE ${DEFAULT_IMAGE} VAR SUIT_STORAGE_ADDRESS CACHE)
  if(DEFINED SUIT_STORAGE_ADDRESS)
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

    unset(CONFIG_NRF_REGTOOL_GENERATE_UICR)
    sysbuild_get(CONFIG_NRF_REGTOOL_GENERATE_UICR IMAGE ${image} VAR CONFIG_NRF_REGTOOL_GENERATE_UICR KCONFIG)
    if(NOT DEFINED CONFIG_NRF_REGTOOL_GENERATE_UICR)
      continue()
    endif()

    unset(IMAGE_TARGET_NAME)
    sysbuild_get(IMAGE_TARGET_NAME IMAGE ${image} VAR CONFIG_SUIT_ENVELOPE_TARGET KCONFIG)
    if(NOT DEFINED IMAGE_TARGET_NAME OR IMAGE_TARGET_NAME STREQUAL "")
      message(STATUS "DFU: Target name for ${image} is not defined. Skipping.")
      continue()
    endif()

    sysbuild_get(IMAGE_BINARY_DIR IMAGE ${image} VAR APPLICATION_BINARY_DIR CACHE)
    sysbuild_get(IMAGE_BINARY_FILE IMAGE ${image} VAR CONFIG_KERNEL_BIN_NAME KCONFIG)
    sysbuild_get(CONFIG_SUIT_ENVELOPE_OUTPUT_ARTIFACT IMAGE ${image} VAR CONFIG_SUIT_ENVELOPE_OUTPUT_ARTIFACT KCONFIG)

    set(OUTPUT_HEX_FILE "${IMAGE_BINARY_DIR}/zephyr/${CONFIG_SUIT_ENVELOPE_OUTPUT_ARTIFACT}")

    list(APPEND ARTIFACTS_TO_MERGE ${BINARY_DIR}/zephyr/suit_installed_envelopes_${IMAGE_TARGET_NAME}_merged.hex)
    list(APPEND ARTIFACTS_TO_MERGE ${IMAGE_BINARY_DIR}/zephyr/${IMAGE_BINARY_FILE}.hex)
    list(APPEND ARTIFACTS_TO_MERGE ${IMAGE_BINARY_DIR}/zephyr/uicr.hex)

    unset(CONFIG_SUIT_ENVELOPE_OUTPUT_MPI_MERGE)
    sysbuild_get(CONFIG_SUIT_ENVELOPE_OUTPUT_MPI_MERGE IMAGE ${image} VAR CONFIG_SUIT_ENVELOPE_OUTPUT_MPI_MERGE KCONFIG)
    if(CONFIG_SUIT_ENVELOPE_OUTPUT_MPI_MERGE)
      list(APPEND ARTIFACTS_TO_MERGE ${BINARY_DIR}/zephyr/suit_mpi_${IMAGE_TARGET_NAME}_merged.hex)
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

if(SB_CONFIG_SUIT_ENVELOPE)
  suit_create_package()
  suit_generate_dfu_zip()
  if(SB_CONFIG_SUIT_BUILD_RECOVERY)
    suit_generate_recovery_dfu_zip()
  endif()
endif()
