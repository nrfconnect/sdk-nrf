#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

function(sdp_assembly_generate)
  set(hrt_dir ${PROJECT_SOURCE_DIR}/src/hrt)
  set(hrt_msg "Generating ASM files for Hard Real Time files.")
  set(hrt_opts -g0 -P -fno-ident -fno-verbose-asm)

  # We get all the .c files from the hrt_dir directory
  file(GLOB hrt_srcs ${hrt_dir}/*.c)

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

  # List of .s files that will be generated
  set(asm_outputs "")

  # Loop through each source file and generate an assembly file
  foreach(hrt_src ${hrt_srcs})
    get_filename_component(src_filename ${hrt_src} NAME_WE)  # filename without extension
    add_custom_command(
      OUTPUT ${hrt_dir}/${src_filename}-temp.s
      COMMAND ${CMAKE_C_COMPILER} ${compiler_options} ${hrt_opts} -S ${hrt_src} -o ${hrt_dir}/${src_filename}-temp.s
      DEPENDS ${hrt_dir}/${src_filename}.c
      COMMAND_EXPAND_LISTS
      COMMENT "Generating ASM file for ${hrt_src}"
    )
    list(APPEND asm_outputs ${hrt_dir}/${src_filename}-temp.s)
  endforeach()

  # Define the clean_asm target that will always run the clean_asm.py script
  # after generating all assembly files
  add_custom_target(clean_asm
    COMMAND ${PYTHON_EXECUTABLE} ${ZEPHYR_NRF_MODULE_DIR}/scripts/sdp/clean_asm.py ${hrt_dir}
    DEPENDS ${asm_outputs}  # The script will be run after all assembly files are generated
    COMMENT "Cleaning ASM files"
    VERBATIM
  )

  # Define the asm_gen target that depends on all generated assembly files and the clean_asm target
  add_custom_target(asm_gen
    DEPENDS ${asm_outputs} clean_asm
    COMMENT ${hrt_msg}
  )

  add_dependencies(clean_asm syscall_list_h_target)

endfunction()

sdp_assembly_generate()
