#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

function(sdp_assembly_generate hrt_srcs)
  set(hrt_msg "Generating ASM files for Hard Real Time files.")
  set(hrt_opts -g0 -fno-ident -fno-verbose-asm)

  # Compiler does not know how to build for this platform so we export
  # all the flags used in this zephyr build to the external build system.
  zephyr_get_compile_options_for_lang(C options)
  zephyr_get_compile_definitions_for_lang(C defines)
  zephyr_get_include_directories_for_lang(C includes)
  zephyr_get_system_include_directories_for_lang(C sys_includes)
  # Replace "-I" with "-isystem" to treat all Zephyr headers as system headers
  # that do not trigger -Werror.
  string(REPLACE "-I" "-isystem" includes "${includes}")

  set(compiler_options ${defines} ${options} ${includes} ${sys_includes})

  if(TARGET asm_gen)
    message(FATAL_ERROR "sdp_assembly_generate() already called, please note that
      sdp_assembly_generate() must be called only once with a list of all source files."
    )
  endif()

  # Define the asm_gen target that depends on all generated assembly files
  add_custom_target(asm_gen
    COMMENT ${hrt_msg}
  )

  foreach(hrt_src ${hrt_srcs})
    if(IS_DIRECTORY ${hrt_src})
      message(FATAL_ERROR "sdp_assembly_generate() was called on a directory")
    endif()
    get_filename_component(src_filename ${hrt_src} NAME_WE)  # filename without extension
    add_custom_command(TARGET asm_gen
      BYPRODUCTS ${src_filename}-temp.s
      COMMAND ${CMAKE_C_COMPILER} ${compiler_options} ${hrt_opts} -S ${hrt_src} -o ${src_filename}-temp.s
      COMMAND ${PYTHON_EXECUTABLE} ${ZEPHYR_NRF_MODULE_DIR}/scripts/sdp/remove_comments.py ${src_filename}-temp.s
      DEPENDS ${hrt_src}
      COMMAND_EXPAND_LISTS
      COMMENT "Generating ASM file for ${hrt_src}"
    )
  endforeach()

  add_dependencies(asm_gen syscall_list_h_target)

endfunction()

function(sdp_assembly_check hrt_srcs)
  set(asm_check_msg "Checking if ASM files for Hard Real Time have changed.")

  if(TARGET asm_check)
    message(FATAL_ERROR "sdp_assembly_check() already called, please note that
      sdp_assembly_check() must be called only once with a list of all source files."
    )
  endif()

  string(REPLACE ";" "$<SEMICOLON>" hrt_srcs_semi "${hrt_srcs}")

  add_custom_target(asm_check
    COMMAND ${CMAKE_COMMAND} -D hrt_srcs="${hrt_srcs_semi}" -P ${ZEPHYR_NRF_MODULE_DIR}/cmake/sdp_asm_check.cmake
    COMMENT ${asm_check_msg}
  )

  add_dependencies(asm_check asm_gen)

endfunction()

function(sdp_assembly_prepare_install hrt_srcs)
  set(asm_install_msg "Updating ASM files for Hard Real Time.")

  if(TARGET asm_install)
    message(FATAL_ERROR "sdp_assembly_prepare_install() already called, please note that
      sdp_assembly_prepare_install() must be called only once with a list of all source files."
    )
  endif()

  string(REPLACE ";" "$<SEMICOLON>" hrt_srcs_semi "${hrt_srcs}")

  add_custom_target(asm_install
    COMMAND ${CMAKE_COMMAND} -D hrt_srcs="${hrt_srcs_semi}" -P ${ZEPHYR_NRF_MODULE_DIR}/cmake/sdp_asm_install.cmake
    COMMENT ${asm_install_msg}
  )

endfunction()
