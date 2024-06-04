#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

set(SUIT_GENERATOR_BUILD_SCRIPT "${ZEPHYR_SUIT_GENERATOR_MODULE_DIR}/ncs/build.py")
set(SUIT_GENERATOR_CLI_SCRIPT "${ZEPHYR_SUIT_GENERATOR_MODULE_DIR}/suit_generator/cli.py")
set(SUIT_OUTPUT_ARTIFACTS_DIRECTORY "DFU")

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

  set_property(GLOBAL APPEND PROPERTY SUIT_DFU_ARTIFACTS ${SUIT_ROOT_DIRECTORY}${target}.bin)
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

  set_property(GLOBAL APPEND PROPERTY SUIT_DFU_ARTIFACTS ${output_file})

  if (create_signature AND SB_CONFIG_SUIT_ENVELOPE_SIGN_SCRIPT)
    suit_sign_envelope(${output_file} ${output_file})
  endif()
endfunction()
