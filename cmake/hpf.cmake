#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# This function takes as an argument target to be built and path to .c source file(s)
# whose .s output must be verified against .s file tracked in repository. It then:
# - adds a pre-build dependency that generates .s files in build directory
# - adds a dependency on specified target to call hpf_asm_check.cmake with appropriate arguments
# - adds a custom target that calls hpf_asm_install.cmake with appropriate arguments
# - adds .s files from build directory to target sources
#
# Arguments:
# target   - target to which the dependencies are to be applied
# hrt_srcs - path to the .c source file(s) to verify
function(hpf_assembly_install target hrt_srcs)
  hpf_assembly_generate(${CONFIG_SOC} "${hrt_srcs}")
  hpf_assembly_check(${CONFIG_SOC} "${hrt_srcs}")
  hpf_assembly_prepare_install(${CONFIG_SOC} "${hrt_srcs}")
  hpf_assembly_target_sources(${CONFIG_SOC} ${target} "${hrt_srcs}")

  add_dependencies(${target} asm_check)
endfunction()

function(hpf_assembly_target_sources soc target hrt_srcs)
  foreach(hrt_src ${hrt_srcs})
    get_filename_component(hrt_dir ${hrt_src} DIRECTORY)  # directory
    get_filename_component(hrt_src_file ${hrt_src} NAME_WE)  # filename without extension
    if(CONFIG_HPF_DEVELOPER_MODE)
      set(hrt_s_file "${hrt_src_file}-${soc}-temp.s")
    else()
      set(hrt_s_file "${hrt_dir}/${hrt_src_file}-${soc}.s")
    endif()
    target_sources(${target} PRIVATE ${hrt_s_file})
  endforeach()
endfunction()

function(hpf_assembly_generate soc hrt_srcs)
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
    message(FATAL_ERROR "hpf_assembly_generate() already called, please note that
      hpf_assembly_generate() must be called only once with a list of all source files."
    )
  endif()

  # Define the asm_gen target that depends on all generated assembly files
  add_custom_target(asm_gen
    COMMENT ${hrt_msg}
  )

  foreach(hrt_src ${hrt_srcs})
    if(IS_DIRECTORY ${hrt_src})
      message(FATAL_ERROR "hpf_assembly_generate() was called on a directory")
    endif()
    get_filename_component(src_filename ${hrt_src} NAME_WE)  # filename without extension
    add_custom_command(TARGET asm_gen
      PRE_BUILD
      BYPRODUCTS ${src_filename}-${soc}-temp.s
      COMMAND ${CMAKE_C_COMPILER} ${compiler_options} ${hrt_opts} -S ${hrt_src} -o ${src_filename}-${soc}-temp.s
      COMMAND ${PYTHON_EXECUTABLE} ${ZEPHYR_NRF_MODULE_DIR}/scripts/hpf/remove_comments.py ${src_filename}-${soc}-temp.s
      COMMAND_EXPAND_LISTS
      COMMENT "Generating ASM file for ${hrt_src}"
    )
  endforeach()

  add_dependencies(asm_gen syscall_list_h_target kobj_types_h_target)

endfunction()

function(hpf_assembly_check soc hrt_srcs)
  set(asm_check_msg "Checking if ASM files for Hard Real Time have changed.")

  if(TARGET asm_check)
    message(FATAL_ERROR "hpf_assembly_check() already called, please note that
      hpf_assembly_check() must be called only once with a list of all source files."
    )
  endif()

  if(CONFIG_HPF_DEVELOPER_MODE)
    set(dev_mode TRUE)
  else()
    set(dev_mode FALSE)
  endif()

  string(REPLACE ";" "$<SEMICOLON>" hrt_srcs_semi "${hrt_srcs}")

  add_custom_target(asm_check
    COMMAND ${CMAKE_COMMAND} -D hrt_srcs="${hrt_srcs_semi}" -D soc="${soc}" -D dev_mode=${dev_mode} -P ${ZEPHYR_NRF_MODULE_DIR}/cmake/hpf_asm_check.cmake
    COMMENT ${asm_check_msg}
  )

  add_dependencies(asm_check asm_gen)

endfunction()

function(hpf_assembly_prepare_install soc hrt_srcs)
  set(asm_install_msg "Updating ASM files for Hard Real Time.")

  if(TARGET asm_install)
    message(FATAL_ERROR "hpf_assembly_prepare_install() already called, please note that
      hpf_assembly_prepare_install() must be called only once with a list of all source files."
    )
  endif()

  string(REPLACE ";" "$<SEMICOLON>" hrt_srcs_semi "${hrt_srcs}")

  add_custom_target(asm_install
    COMMAND ${CMAKE_COMMAND} -D hrt_srcs="${hrt_srcs_semi}" -D soc="${soc}" -P ${ZEPHYR_NRF_MODULE_DIR}/cmake/hpf_asm_install.cmake
    COMMENT ${asm_install_msg}
  )

endfunction()
