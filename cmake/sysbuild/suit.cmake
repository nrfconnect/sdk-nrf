#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
include(${CMAKE_CURRENT_LIST_DIR}/suit_utilities.cmake)

# Copy input template into destination directory.
#
# Usage:
#   suit_copy_input_templates(<destination_template_directory> <soure_template_path> <output_variable>)
#
# Parameters:
#   'destination_template_directory' - destination directory
#   'source_template_path' - path to the source template
#   'output_variable' - variable to store new path to the copied template
function(suit_copy_input_template destination_template_directory source_template_path output_variable)
  cmake_path(GET source_template_path FILENAME source_filename)
  set(destination_template_path "${destination_template_directory}/${source_filename}")
  if(NOT EXISTS ${source_template_path})
    message(SEND_ERROR "DFU: Could not find default SUIT template: '${source_template_path}'. Corrupted configuration?")
    return()
  endif()
  if(NOT EXISTS ${destination_template_path})
    # copy default template and create digest
    configure_file(${source_template_path} ${destination_template_path} COPYONLY)
    file(SHA256 ${destination_template_path} checksum_variable)
    file(WRITE "${destination_template_path}.digest" ${checksum_variable})
  endif()
  if(NOT EXISTS "${destination_template_path}.digest")
    # restore digest removed by user to discard warning about changes in the source template
    file(SHA256 ${source_template_path} checksum_variable)
    file(WRITE "${destination_template_path}.digest" ${checksum_variable})
  endif()
  cmake_path(GET source_template_path FILENAME copied_filename)
  set(${output_variable} "${destination_template_directory}/${copied_filename}" PARENT_SCOPE)
endfunction()

# Check digests for template.
#
# Usage:
#   suit_check_template_digest(<destination_template_directory> <template_path>)
#
# Parameters:
#   'destination_template_directory' - destination directory
#   'template_path' - path to the source template
function(suit_check_template_digest destination_template_directory source_template_path)
    suit_set_absolute_or_relative_path(${source_template_path} ${PROJECT_BINARY_DIR} source_template_path)
    if(NOT EXISTS ${source_template_path})
      message(SEND_ERROR "DFU: Could not find default SUIT template: '${source_template_path}'. Corrupted configuration?")
      return()
    endif()
    cmake_path(GET source_template_path FILENAME source_filename)
    set(input_file "${destination_template_directory}/${source_filename}")

    file(SHA256 ${source_template_path} CHECKSUM_DEFAULT)
    set(DIGEST_STORAGE "${input_file}.digest")
    file(STRINGS ${DIGEST_STORAGE} CHECKSUM_STORED)
    if(NOT ${CHECKSUM_DEFAULT} STREQUAL ${CHECKSUM_STORED})
      message(SEND_ERROR "DFU: Outdated input SUIT template detected - consider update.\n"
      "Some changes has been done to the SUIT_ENVELOPE_DEFAULT_TEMPLATE which was used to create your ${input_file}.\n"
      "Please review these changes and remove ${input_file}.digest file to bypass this error.\n"
      )
    endif()
endfunction()

# Create digest for input file.
#
# Usage:
#   suit_create_digest(<input_file> <output_file>)
#
# Parameters:
#   'input_file' - input file to calculate digest on
#   'output_file' - output file to store calculated digest
function(suit_create_digest input_file output_file)
  file(SHA256 ${input_file} CHECKSUM_VARIABLE)
  file(WRITE ${output_file} ${CHECKSUM_VARIABLE})
endfunction()

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

  suit_set_absolute_or_relative_path(${SB_CONFIG_SUIT_ENVELOPE_EDITABLE_TEMPLATES_LOCATION} ${PROJECT_BINARY_DIR} INPUT_TEMPLATES_DIRECTORY)
  set(ENVELOPE_SHALL_BE_SIGNED ${SB_CONFIG_SUIT_ENVELOPE_SIGN})
  if(NOT DEFINED ENVELOPE_SHALL_BE_SIGNED)
    set(ENVELOPE_SHALL_BE_SIGNED FALSE)
  endif()

  list(APPEND CORE_ARGS
    --core sysbuild,,,${CMAKE_BINARY_DIR}/zephyr/.config
  )

  foreach(image ${IMAGES})
    sysbuild_get(GENERATE_LOCAL_ENVELOPE IMAGE ${image} VAR CONFIG_SUIT_LOCAL_ENVELOPE_GENERATE KCONFIG)
    if(NOT DEFINED GENERATE_LOCAL_ENVELOPE)
      continue()
    endif()
    unset(GENERATE_LOCAL_ENVELOPE)

    sysbuild_get(INPUT_ENVELOPE_JINJA_FILE IMAGE ${image} VAR CONFIG_SUIT_ENVELOPE_TEMPLATE KCONFIG)
    sysbuild_get(target IMAGE ${image} VAR CONFIG_SUIT_ENVELOPE_TARGET KCONFIG)
    sysbuild_get(BINARY_DIR IMAGE ${image} VAR APPLICATION_BINARY_DIR CACHE)
    sysbuild_get(BINARY_FILE IMAGE ${image} VAR CONFIG_KERNEL_BIN_NAME KCONFIG)
    suit_copy_input_template(${INPUT_TEMPLATES_DIRECTORY} "${INPUT_ENVELOPE_JINJA_FILE}" ENVELOPE_JINJA_FILE)
    if(NOT DEFINED ENVELOPE_JINJA_FILE)
      message(SEND_ERROR "DFU: Creation of SUIT artifacts failed.")
      return()
    endif()
    suit_check_template_digest(${INPUT_TEMPLATES_DIRECTORY} "${INPUT_ENVELOPE_JINJA_FILE}")
    set(BINARY_FILE "${BINARY_FILE}.bin")

    list(APPEND CORE_ARGS
      --core ${target},${SUIT_ROOT_DIRECTORY}${target}.bin,${BINARY_DIR}/zephyr/edt.pickle,${BINARY_DIR}/zephyr/.config
    )

    set(ENVELOPE_YAML_FILE ${SUIT_ROOT_DIRECTORY}${target}.yaml)
    set(ENVELOPE_SUIT_FILE ${SUIT_ROOT_DIRECTORY}${target}.suit)

    suit_copy_artifact_to_output_directory(${target} ${BINARY_DIR}/zephyr/${BINARY_FILE})
    suit_render_template(${ENVELOPE_JINJA_FILE} ${ENVELOPE_YAML_FILE} "${CORE_ARGS}")
    suit_create_envelope(${ENVELOPE_YAML_FILE} ${ENVELOPE_SUIT_FILE} ${ENVELOPE_SHALL_BE_SIGNED})
    list(APPEND STORAGE_BOOT_ARGS
      --input-envelope ${ENVELOPE_SUIT_FILE}
    )
  endforeach()

  set(INPUT_ROOT_ENVELOPE_JINJA_FILE ${SB_CONFIG_SUIT_ENVELOPE_ROOT_TEMPLATE})

  # create root envelope if defined
  if(DEFINED INPUT_ROOT_ENVELOPE_JINJA_FILE AND NOT INPUT_ROOT_ENVELOPE_JINJA_FILE STREQUAL "")
    set(ROOT_NAME ${SB_CONFIG_SUIT_ENVELOPE_ROOT_ARTIFACT_NAME})
    suit_copy_input_template(${INPUT_TEMPLATES_DIRECTORY} "${INPUT_ROOT_ENVELOPE_JINJA_FILE}" ROOT_ENVELOPE_JINJA_FILE)
    suit_check_template_digest(${INPUT_TEMPLATES_DIRECTORY} "${INPUT_ROOT_ENVELOPE_JINJA_FILE}")
    set(ROOT_ENVELOPE_YAML_FILE ${SUIT_ROOT_DIRECTORY}${ROOT_NAME}.yaml)
    set(ROOT_ENVELOPE_SUIT_FILE ${SUIT_ROOT_DIRECTORY}${ROOT_NAME}.suit)
    suit_render_template(${ROOT_ENVELOPE_JINJA_FILE} ${ROOT_ENVELOPE_YAML_FILE} "${CORE_ARGS}")
    suit_create_envelope(${ROOT_ENVELOPE_YAML_FILE} ${ROOT_ENVELOPE_SUIT_FILE} ${ENVELOPE_SHALL_BE_SIGNED})
      list(APPEND STORAGE_BOOT_ARGS
        --input-envelope ${ROOT_ENVELOPE_SUIT_FILE}
      )
  endif()

  sysbuild_get(DEFAULT_BINARY_DIR IMAGE ${DEFAULT_IMAGE} VAR APPLICATION_BINARY_DIR CACHE)
  # create all storages in the DEFAULT_IMAGE output directory
  list(APPEND STORAGE_BOOT_ARGS --storage-output-directory "${DEFAULT_BINARY_DIR}/zephyr" --zephyr-base ${ZEPHYR_BASE} ${CORE_ARGS})
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

    sysbuild_get(GENERATE_LOCAL_ENVELOPE IMAGE ${image} VAR CONFIG_SUIT_LOCAL_ENVELOPE_GENERATE KCONFIG)
    if(NOT DEFINED GENERATE_LOCAL_ENVELOPE)
      continue()
    endif()
    unset(GENERATE_LOCAL_ENVELOPE)

    sysbuild_get(IMAGE_BINARY_DIR IMAGE ${image} VAR APPLICATION_BINARY_DIR CACHE)
    sysbuild_get(IMAGE_BINARY_FILE IMAGE ${image} VAR CONFIG_KERNEL_BIN_NAME KCONFIG)
    sysbuild_get(IMAGE_TARGET_NAME IMAGE ${image} VAR CONFIG_SUIT_ENVELOPE_TARGET KCONFIG)

    sysbuild_get(CONFIG_SUIT_ENVELOPE_OUTPUT_ARTIFACT IMAGE ${image} VAR CONFIG_SUIT_ENVELOPE_OUTPUT_ARTIFACT KCONFIG)
    sysbuild_get(CONFIG_NRF_REGTOOL_GENERATE_UICR IMAGE ${image} VAR CONFIG_NRF_REGTOOL_GENERATE_UICR KCONFIG)
    sysbuild_get(CONFIG_SUIT_ENVELOPE_OUTPUT_MPI_MERGE IMAGE ${image} VAR CONFIG_SUIT_ENVELOPE_OUTPUT_MPI_MERGE KCONFIG)

    set(OUTPUT_HEX_FILE "${IMAGE_BINARY_DIR}/zephyr/${CONFIG_SUIT_ENVELOPE_OUTPUT_ARTIFACT}")

    list(APPEND ARTIFACTS_TO_MERGE ${BINARY_DIR}/zephyr/storage_${IMAGE_TARGET_NAME}.hex)
    list(APPEND ARTIFACTS_TO_MERGE ${IMAGE_BINARY_DIR}/zephyr/${IMAGE_BINARY_FILE}.hex)
    if(CONFIG_SUIT_ENVELOPE_OUTPUT_MPI_MERGE)
      list(APPEND ARTIFACTS_TO_MERGE ${BINARY_DIR}/zephyr/suit_mpi_${IMAGE_TARGET_NAME}_merged.hex)
    endif()
    if(CONFIG_NRF_REGTOOL_GENERATE_UICR)
      list(APPEND ARTIFACTS_TO_MERGE ${IMAGE_BINARY_DIR}/zephyr/uicr.hex)
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
endif()
