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
function(asm_check)
  if(NOT DEFINED soc)
    message(FATAL_ERROR "asm_check missing soc argument.")
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
      message(WARNING "${asm_filename}-${soc}.s ASM file content has changed.\
              If you want to include the new ASM in build, \
              please run `ninja asm_install` in FLPR build directory and build again")
    else()
      message("Something went wrong when comparing ${asm_filename}-${soc}.s and ${asm_filename}-${soc}-temp.s")
    endif()
  endforeach()

endfunction(asm_check)

asm_check()
