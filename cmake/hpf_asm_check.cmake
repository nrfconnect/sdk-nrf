#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Function checking if .s file generated during build is identical to one tracked in repository
#
# Arguments:
# hrt_srcs - path to the .c file(s) to verify
# soc      - name of the SoC the code is being built for
# dev_mode - variable indicating if developer mode is enabled
function(asm_check)
  if(NOT DEFINED soc)
    message(FATAL_ERROR "asm_check missing soc argument.")
  endif()

  if(NOT DEFINED dev_mode)
    set(dev_mode FALSE)
  endif()

  foreach(hrt_src ${hrt_srcs})
    get_filename_component(asm_filename ${hrt_src} NAME_WE)  # filename without extension
    get_filename_component(src_dir ${hrt_src} DIRECTORY)

    execute_process(COMMAND ${CMAKE_COMMAND} -E compare_files --ignore-eol
                    ${src_dir}/${asm_filename}-${soc}.s ${asm_filename}-${soc}-temp.s
                    RESULT_VARIABLE compare_result)

    if( compare_result EQUAL 0)
      message("File ${asm_filename}-${soc}.s has not changed.")
    elseif( compare_result EQUAL 1)
      if(dev_mode)
        message(WARNING "${asm_filename}-${soc}.s ASM file content has changed.\
                The new version will be included in build, but it will not be moved to the source \
                directory. If you want to update ASM file in source directory with the new one \
                run `ninja asm_install` in FLPR build directory.")
      else()
        message(FATAL_ERROR "${asm_filename}-${soc}.s ASM file content has changed.\
                If you want to include the new ASM in build, \
                please run `ninja asm_install` in FLPR build directory and build again. \
                If you want to disable this error and include new ASM in build every time,\
                enable SB_CONFIG_HPF_DEVELOPER_MODE option (when building for cpuapp) \
                or CONFIG_HPF_DEVELOPER_MODE option (when building for cpuflpr).")
      endif()
    else()
      message("Something went wrong when comparing ${asm_filename}-${soc}.s and ${asm_filename}-${soc}-temp.s")
    endif()
  endforeach()

endfunction(asm_check)

asm_check()
