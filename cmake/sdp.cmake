#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

function(sdp_assembly_generate)
  set(hrt_dir ${PROJECT_SOURCE_DIR}/src/hrt)
  set(hrt_src ${hrt_dir}/hrt.c)
  set(hrt_msg "Generating ASM file for Hard Real Time files.")
  set(hrt_opts -g0 -P -fno-ident -fno-verbose-asm)

  # The external static library that we are linking with does not know
  # how to build for this platform so we export all the flags used in
  # this zephyr build to the external build system.
  zephyr_get_compile_options_for_lang(C options)
  zephyr_get_compile_definitions_for_lang(C defines)
  zephyr_get_include_directories_for_lang(C includes)
  zephyr_get_system_include_directories_for_lang(C sys_includes)
  # Replace "-I" with "-isystem" to treat all Zephyr headers as system headers
  # that do not trigger -Werror.
  string(REPLACE "-I" "-isystem" includes "${includes}")

  set(compiler_options ${defines} ${options} ${includes} ${sys_includes})

  zephyr_include_directories(include)

  add_custom_target(asm_gen
    BYPRODUCTS ${hrt_dir}/hrt.s
    COMMAND ${CMAKE_C_COMPILER} ${compiler_options} ${hrt_opts} -S ${hrt_src}
    COMMAND ${PYTHON_EXECUTABLE} ${ZEPHYR_NRF_MODULE_DIR}/scripts/sdp/clean_asm.py ${hrt_dir}
    WORKING_DIRECTORY "${hrt_dir}"
    COMMAND_EXPAND_LISTS
    COMMENT ${hrt_msg}
  )

  add_dependencies(asm_gen syscall_list_h_target)

endfunction()

sdp_assembly_generate()
