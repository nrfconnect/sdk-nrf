#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# This file creates a target which can be used to re-generate and install
# new CDDL decoder files. This is ONLY needed if the modem_update.cddl file is
# modified or if the argument passe to the parser generator is changed.
# The option 'CONFIG_FMFU_CDDL_DECODER_GENERATE' must be set for this file to
# be executed. Since this is a promptless option, you can set it by adding
# a 'Kconfig' file in the sample directory and create a duplicate
# declaration of the option with 'default y', or by changing the default value
# directly in the Kconfig definition. Once that is done, run the
# 'fmfu_cddl_modem_update_install' target to install the new files.

# Output directories inside build dir
set(src_out ${ZEPHYR_BINARY_DIR}/source/generated)
set(include_out ${ZEPHYR_BINARY_DIR}/include/generated)

# Make sure that output directory for *.c files exist
file(MAKE_DIRECTORY ${src_out})

# This file is used as the source for the parser generator
set(cddl_file ${CMAKE_CURRENT_LIST_DIR}/modem_update.cddl)

# These are the entry types needed by the source code
set(entry_types Wrapper Sig_structure1 Segments)

set(decode_c_name modem_update_decode.c)
set(decode_h_name modem_update_decode.h)
set(types_h_name modem_update_types.h)
set(decode_c ${src_out}/${decode_c_name})
set(decode_h ${include_out}/${decode_h_name})
set(types_h ${include_out}/${types_h_name})
set(src_install_dir ${ZEPHYR_NRF_MODULE_DIR}/subsys/dfu/fmfu_fdev/src)
set(include_install_dir ${ZEPHYR_NRF_MODULE_DIR}/subsys/dfu/fmfu_fdev/include)
set(license ${CMAKE_CURRENT_LIST_DIR}/license.cmake)

# The .clang-format in this repo states support for >= version 4
find_program(
  CLANG_FORMAT
  NAMES
  clang-format
  clang-format-10
  clang-format-9
  clang-format-8
  clang-format-7
  clang-format-6
  clang-format-5
  clang-format-4
  )

if(NOT CLANG_FORMAT)
  message(WARNING
    "'clang-format' not found, generated code will not be formatted")
else()
  set(CLANG_FORMAT_COMMAND
   COMMAND ${CLANG_FORMAT} -i "${decode_c};${decode_h};${types_h}")
endif()

add_custom_command(
  OUTPUT ${decode_c} ${decode_h} ${types_h}
  COMMAND
  ${PYTHON_EXECUTABLE}
  ${ZEPHYR_ZCBOR_MODULE_DIR}/zcbor/zcbor.py
  -c ${cddl_file}
  --default-max-qty 128 # TNSW-33521
  code
  --oc ${decode_c}
  --oh ${decode_h}
  --oht ${types_h}
  -t ${entry_types}
  -d # Decode
  COMMAND
  ${CMAKE_COMMAND} -DFILES="${decode_c}\;${decode_h}\;${types_h}" -P ${license}
  ${CLANG_FORMAT_COMMAND}
  COMMENT
  "Generating files based on ${cddl_file}"
  DEPENDS ${license} ${cddl_file}
  )

zephyr_library()
zephyr_library_sources(${decode_c})
zephyr_include_directories(${include_out})

# Create install target which allows the user to 'install' the generated
# parser files into the working tree.
add_custom_target(
  fmfu_cddl_modem_update_install
  COMMAND ${CMAKE_COMMAND} -E copy ${decode_c} ${src_install_dir}/${decode_c_name}
  COMMAND ${CMAKE_COMMAND} -E copy ${decode_h} ${include_install_dir}/${decode_h_name}
  COMMAND ${CMAKE_COMMAND} -E copy ${types_h}  ${include_install_dir}/${types_h_name}
  DEPENDS
  ${decode_c} ${decode_h} ${types_h}
  COMMENT
  "Installing Modem Update CDDL decoder files"
  )
