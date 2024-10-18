#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

function(asm_check)
  # We get all the .c files from the hrt_dir directory
  file(GLOB hrt_srcs ${hrt_dir}/*.c)

  foreach(hrt_src ${hrt_srcs})
    get_filename_component(asm_filename ${hrt_src} NAME_WE)  # filename without extension

    execute_process(COMMAND ${CMAKE_COMMAND} -E compare_files ${asm_filename}.s ${asm_filename}-temp.s
                    RESULT_VARIABLE compare_result)

    if( compare_result EQUAL 0)
      file(REMOVE ${asm_filename}-temp.s)
      message("File ${asm_filename}.s has not changed.")
    elseif( compare_result EQUAL 1)
      message(WARNING "${asm_filename}.s ASM file content has changed.\
              If you want to include the new ASM in build, \
              please run `west sdp_asm_update` and build again")
    else()
      message("Something went wrong when comparing ${asm_filename}.s and ${asm_filename}-temp.s")
    endif()
  endforeach()

endfunction(asm_check)

asm_check()
