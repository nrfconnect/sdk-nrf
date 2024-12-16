#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

set(SUIT_GENERATOR_BUILD_SCRIPT "${ZEPHYR_SUIT_GENERATOR_MODULE_DIR}/ncs/build.py")
set(SUIT_GENERATOR_CLI_SCRIPT "${ZEPHYR_SUIT_GENERATOR_MODULE_DIR}/suit_generator/cli.py")
set(SUIT_OUTPUT_ARTIFACTS_DIRECTORY "DFU")

if(WIN32)
  set(SEP $<SEMICOLON>)
else()
  set(SEP :)
endif()

if(NOT DEFINED SUIT_ROOT_DIRECTORY)
  set(SUIT_ROOT_DIRECTORY ${APPLICATION_BINARY_DIR}/${SUIT_OUTPUT_ARTIFACTS_DIRECTORY}/)
endif()

# Copy artifact to the common SUIT output directory.
#
# Usage:
#   suit_copy_artifact_to_output_directory(<target> <artifact>)
#
# Parameters:
#   'target' - target name
#   'artifact' - path to artifact
function(suit_copy_artifact_to_output_directory target artifact)
  cmake_path(GET artifact FILENAME artifact_filename)
  cmake_path(GET artifact EXTENSION artifact_extension)
  set(destination_name "${target}${artifact_extension}")
  set_property(
    GLOBAL APPEND PROPERTY SUIT_POST_BUILD_COMMANDS
    COMMAND ${CMAKE_COMMAND} -E copy ${artifact} ${SUIT_ROOT_DIRECTORY}${target}.bin
    BYPRODUCTS ${SUIT_ROOT_DIRECTORY}${target}.bin
  )
endfunction()

# Render jinja templates using passed arguments.
# Function uses core_arguments which is list of folowing entries:
#   --core <target_name>,<locatino_of_firmware_binary_file>,<location_of_edt_file_representing_dts>
#
# Usage:
#   suit_render_template(<input_file> <output_file> <core_arguments>)
#
# Parameters:
#   'input_file' - path to input jinja template
#   'output_file' - path to output yaml file
#   'core_arguments' - list of arguments for registered cores
function(suit_render_template input_file output_file core_arguments)
  set(TEMPLATE_ARGS)
  list(APPEND TEMPLATE_ARGS
    --template-suit ${input_file}
    --output-suit ${output_file}
    --zephyr-base ${ZEPHYR_BASE}
  )
  list(APPEND TEMPLATE_ARGS ${core_arguments})
  list(APPEND TEMPLATE_ARGS --artifacts-folder "${SUIT_ROOT_DIRECTORY}")
   set_property(
    GLOBAL APPEND PROPERTY SUIT_POST_BUILD_COMMANDS
    COMMAND ${PYTHON_EXECUTABLE} ${SUIT_GENERATOR_BUILD_SCRIPT}
    template
    ${TEMPLATE_ARGS}
    BYPRODUCTS ${output_file}
  )
endfunction()

# Create binary envelope from input yaml file.
#
# Usage:
#   suit_create_envelope(<input_file> <output_file> <create_signature>)
#
# Parameters:
#   'input_file' - path to input yaml configuration
#   'output_file' - path to output binary suit envelope
#   'create_signature' - sign the envelope if set to true
function(suit_create_envelope input_file output_file create_signature)
  set_property(
    GLOBAL APPEND PROPERTY SUIT_POST_BUILD_COMMANDS
    COMMAND ${PYTHON_EXECUTABLE} ${SUIT_GENERATOR_CLI_SCRIPT}
    create
    --input-file ${input_file}
    --output-file ${output_file}
    BYPRODUCTS ${output_file}
  )

  if (create_signature AND SB_CONFIG_SUIT_ENVELOPE_SIGN_SCRIPT)
    suit_sign_envelope(${output_file} ${output_file})
  endif()
endfunction()

# Create a SUIT DFU cache partition file from a list of payloads.
#
# Usage:
#  suit_create_cache_partition(<args> <output_file> <partition_num> <recovery>)
#
# Parameters:
#   'args' - list of arguments for the cache_create command
#   'output_file' - path to output cache partition file
#   'partition_num' - partition number
#   'recovery' - if set to true, the cache partition contains recovery firmware payloads
function(suit_create_cache_partition args output_file partition_num recovery)
  list(APPEND args "--output-file" "${output_file}")

  set_property(
    GLOBAL APPEND PROPERTY SUIT_POST_BUILD_COMMANDS
    COMMAND ${PYTHON_EXECUTABLE} ${SUIT_GENERATOR_CLI_SCRIPT}
    cache_create
    from_payloads
    ${args}
    BYPRODUCTS ${output_file}
  )

  get_filename_component(output_file_name ${output_file} NAME)

  if (recovery)
    set_property(GLOBAL APPEND PROPERTY SUIT_RECOVERY_DFU_ARTIFACTS ${output_file})
    set_property(GLOBAL APPEND PROPERTY SUIT_RECOVERY_DFU_ZIP_ADDITIONAL_SCRIPT_PARAMS
                 "${output_file_name}type=cache;${output_file_name}partition=${partition_num};")
  else()
    set_property(GLOBAL APPEND PROPERTY SUIT_DFU_ARTIFACTS ${output_file})
    set_property(GLOBAL APPEND PROPERTY SUIT_DFU_ZIP_ADDITIONAL_SCRIPT_PARAMS
                 "${output_file_name}type=cache;${output_file_name}partition=${partition_num};")
  endif()
endfunction()

# Create a SUIT DFU cache partition with Nordic proprietary payloads
# extracted from the top-level SUIT envelope.
#
# Usage:
# suit_create_nordic_cache_partition(<args> <output_file>)
#
# Parameters:
#   'args' - list of arguments for the cache_create command
#   'output_file' - path to output cache partition file
function(suit_create_nordic_cache_partition args output_file)
  list(APPEND args "--output-file" "${output_file}")
  list(APPEND args "--omit-payload-regex" "'(?!.*secdom.*\.bin|.*sysctl_v.*\.bin).*'")
  list(APPEND args "--dependency-regex" "'(#top|#secdom|#sysctrl)'")

  set_property(
    GLOBAL APPEND PROPERTY SUIT_POST_BUILD_COMMANDS
    COMMAND ${PYTHON_EXECUTABLE} ${SUIT_GENERATOR_CLI_SCRIPT}
    cache_create
    from_envelope
    ${args}
    BYPRODUCTS ${output_file}
  )
endfunction()

# Usage:
#   suit_add_merge_hex_file(FILES <files> [DEPENDENCIES <dependencies>] [TARGET <sysbuild_target>]
#
# Add files which should be merged into the uicr_merged.hex output file, respecting any
# dependencies that need to be generated before hand. This will overwrite existing data if it is
# present in other files
function(suit_add_merge_hex_file)
  cmake_parse_arguments(arg "" "TARGET" "FILES;DEPENDENCIES" ${ARGN})
  zephyr_check_arguments_required(${CMAKE_CURRENT_FUNCTION} arg FILES)

  if(DEFINED arg_TARGET)
    set_property(GLOBAL APPEND PROPERTY SUIT_MERGE_${arg_TARGET}_FILE ${arg_FILES})
    set_property(GLOBAL APPEND PROPERTY SUIT_MERGE_${arg_TARGET}_DEPENDENCIES ${arg_DEPENDENCIES})
  else()
    set_property(GLOBAL APPEND PROPERTY SUIT_MERGE_application_FILE ${arg_FILES})
    set_property(GLOBAL APPEND PROPERTY SUIT_MERGE_application_DEPENDENCIES ${arg_DEPENDENCIES})
endif()
endfunction()

# Create SUIT encryption artifacts for a given image.
#
# Usage:
# suit_encrypt_image(<args> <output_directory>)
#
# Parameters:
#   'args' - list of arguments for the encryption script
#   'output_directory' - path to a directory where the encryption artifacts will be stored
function(suit_encrypt_image args output_directory)
  get_property(
    encrypt_script
    GLOBAL PROPERTY
    SUIT_ENCRYPT_SCRIPT
  )
  # If the user has not provided the path to the encrypt script, use the default one.
  if(NOT encrypt_script)
    set(encrypt_script "${ZEPHYR_SUIT_GENERATOR_MODULE_DIR}/ncs/encrypt_script.py")
  endif()

  if(NOT EXISTS ${encrypt_script})
    message(SEND_ERROR "DFU: ${encrypt_script} does not exist. Corrupted configuration?")
    return()
  endif()

  list(APPEND args --output-dir ${output_directory})

  set_property(
    GLOBAL APPEND PROPERTY SUIT_POST_BUILD_COMMANDS
    COMMAND ${CMAKE_COMMAND} -E make_directory ${output_directory}
  )
  set_property(
    GLOBAL APPEND PROPERTY SUIT_POST_BUILD_COMMANDS
    COMMAND
    PYTHONPATH=${ZEPHYR_SUIT_GENERATOR_MODULE_DIR}${SEP}$ENV{PYTHONPATH}
    ${PYTHON_EXECUTABLE}
    ${encrypt_script} encrypt-and-generate
    ${args}
  )
endfunction()
