#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

#
# This file adds the macro `generate_and_add_cbor_files()` for
# generating CBOR parsers from CDDL files with zcbor, or
# adding already generated and installed CBOR parsers to the build.
#
# To (re-)generate CBOR parser files, and use the generated files,
# add -DCONFIG_SSF_ZCBOR_GENERATE=y when building.
# This config is needed for the generation process to run.
#
# To install the generated CBOR parser files, build/run target
# 'ssf_parser_files_install'. CBOR parser files will be installed
# into relative directory specified by the second argument of
# `generate_and_add_cbor_files()`.
#

find_program(
  CLANG_FORMAT_EXECUTABLE
  NAMES
  clang-format
  clang-format-15
  clang-format-14
)

# Empty target that will depend on other install targets.
add_custom_target(ssf_parser_files_install)

function(generate_cbor_files cddl_file install_dir)
  set(license ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/license.cmake)
  set(gen_cmake_list ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/gen_cmake_list.cmake)

  cmake_path(GET install_dir FILENAME bin_dir)
  cmake_path(REMOVE_EXTENSION bin_dir)

  set(src_out ${ZEPHYR_BINARY_DIR}/source/${bin_dir})
  set(include_out ${ZEPHYR_BINARY_DIR}/include/${bin_dir})
  file(MAKE_DIRECTORY ${src_out})

  cmake_path(GET cddl_file FILENAME cddl_name)
  cmake_path(REMOVE_EXTENSION cddl_name)

  set(encode_c_name ${cddl_name}_encode.c)
  set(decode_c_name ${cddl_name}_decode.c)
  set(encode_h_name ${cddl_name}_encode.h)
  set(decode_h_name ${cddl_name}_decode.h)
  set(types_h_name ${cddl_name}_types.h)

  set(encode_c ${src_out}/${encode_c_name})
  set(decode_c ${src_out}/${decode_c_name})
  set(encode_h ${include_out}/${encode_h_name})
  set(decode_h ${include_out}/${decode_h_name})
  set(types_h ${include_out}/${types_h_name})

  set(out_files "")
  list(APPEND out_files ${encode_c} ${decode_c} ${encode_h} ${decode_h} ${types_h})

  set(entry_types ${ARGN})

  # Ensures correct passing of list argument to ${license} file.
  string(REPLACE ";" "\;" out_files_cmd_arg "${out_files}")

  if (CLANG_FORMAT_EXECUTABLE)
    set(CLANG_FORMAT_COMMAND COMMAND ${CLANG_FORMAT_EXECUTABLE} -i "${out_files}")
  endif()

  add_custom_command(
    OUTPUT ${out_files}
    COMMAND ${CMAKE_COMMAND} -E echo "Generating files from ${cddl_file}"
    COMMAND ${PYTHON_EXECUTABLE} ${ZEPHYR_ZCBOR_MODULE_DIR}/zcbor/zcbor.py code
      --decode --encode
      --cddl ${cddl_file}
      --entry-types ${entry_types}
      --output-c ${src_out}/${cddl_name}.c # When both '-d' and '-e' are specified, zcbor automatically
      --output-h ${include_out}/${cddl_name}.h # adds '_encode' and '_decode' to filenames.
      --output-h-types ${include_out}/${types_h_name}
    COMMAND ${CMAKE_COMMAND} -DFILES=${out_files_cmd_arg} -P ${license}
    ${CLANG_FORMAT_COMMAND}
    DEPENDS ${license} ${cddl_file}
    VERBATIM
  )

  zephyr_library_sources(${encode_c} ${decode_c})
  zephyr_include_directories(${include_out})

  # Create install target which installs the generated ${cddl_name}
  # parser files into the working tree when target
  # 'ssf_parser_files_install' is built.
  set(parser_files_install parser_files_install_${cddl_name})
  add_custom_target(${parser_files_install}
    COMMAND ${CMAKE_COMMAND} -E echo "Installing files generated from ${cddl_file} in ${install_dir}"
    COMMAND ${CMAKE_COMMAND} -E copy ${encode_c} ${install_dir}/${encode_c_name}
    COMMAND ${CMAKE_COMMAND} -E copy ${decode_c} ${install_dir}/${decode_c_name}
    COMMAND ${CMAKE_COMMAND} -E copy ${encode_h} ${install_dir}/${encode_h_name}
    COMMAND ${CMAKE_COMMAND} -E copy ${decode_h} ${install_dir}/${decode_h_name}
    COMMAND ${CMAKE_COMMAND} -E copy ${types_h} ${install_dir}/${types_h_name}
    COMMAND ${CMAKE_COMMAND} -DCDDL_NAME=${cddl_name} -DINSTALL_DIR=${install_dir} -P ${gen_cmake_list}
    DEPENDS ${out_files}
  )
  add_dependencies(ssf_parser_files_install ${parser_files_install})

endfunction()

#
# Function for generating CBOR parsers from CDDL files with zcbor, or
# adding already generated and installed CBOR parsers to the build.
#
# `cddl_file`     Path to a .cddl file.
# `install_dir `  Path of the directory where installed CBOR parser
#                 files can be found. Or, in case target `ssf_parser_files_install`
#                 is run, the directory to install the CBOR parsers.
# OPTIONAL        List of entry types to create parsers for.
#
function(generate_and_add_cbor_files cddl_file install_dir)
  cmake_path(ABSOLUTE_PATH cddl_file
    BASE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} NORMALIZE
    OUTPUT_VARIABLE cddl_file
  )
  cmake_path(ABSOLUTE_PATH install_dir
    BASE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} NORMALIZE
    OUTPUT_VARIABLE install_dir
  )

  cmake_path(GET install_dir FILENAME bin_dir)
  cmake_path(REMOVE_EXTENSION bin_dir)

  if (CONFIG_SSF_ZCBOR_GENERATE)
    zephyr_library_get_current_dir_lib_name(${ZEPHYR_BASE} lib_name)
    if (NOT TARGET ${lib_name})
      zephyr_library()
    endif()
    generate_cbor_files(${cddl_file} ${install_dir} ${ARGN})
  else()
    add_subdirectory(${install_dir} ${bin_dir})
  endif()

endfunction()
