#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

function(sdp_assembly_generate)
  set(hrt_dir ${PROJECT_SOURCE_DIR}/src/hrt)
  set(hrt_src ${hrt_dir}/hrt.c)
  set(hrt_msg "Generating ASM file for Hard Real Time files.")

  # The external static library that we are linking with does not know
  # how to build for this platform so we export all the flags used in
  # this zephyr build to the external build system.
  zephyr_get_compile_options_for_lang(C hrt_opt)
  zephyr_get_compile_definitions_for_lang(C hrt_def)
  zephyr_get_include_directories_for_lang(C hrt_inc)
  zephyr_get_system_include_directories_for_lang(C hrt_sys)
  # Replace "-I" with "-isystem" to treat all Zephyr headers as system headers
  # that do not trigger -Werror.
  string(REPLACE "-I" "-isystem" hrt_inc "${hrt_inc}")

  zephyr_include_directories(include)

  add_custom_target(asm_gen
    COMMAND ${CMAKE_C_COMPILER} ${hrt_def} ${hrt_opt} ${hrt_inc} ${hrt_sys} -g0 -P -fno-ident -fno-verbose-asm  -S ${hrt_src}
    COMMAND python3 ${ZEPHYR_NRF_MODULE_DIR}/scripts/sdp/clean_asm.py ${hrt_dir}
    WORKING_DIRECTORY "${hrt_dir}"
    COMMAND_EXPAND_LISTS
    COMMENT ${hrt_msg}
  )
endfunction()

sdp_assembly_generate()
