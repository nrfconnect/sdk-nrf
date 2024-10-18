#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

function(asm_update)
  # We get all the .c files from the hrt_dir directory
  file(GLOB hrt_srcs ${hrt_dir}/*.c)

  foreach(hrt_src ${hrt_srcs})
    get_filename_component(asm_filename ${hrt_src} NAME_WE)  # filename without extension

    file(RENAME ${asm_filename}-temp.s ${asm_filename}.s RESULT rename_result)

    if (rename_result EQUAL 0)
        message("Updated ${asm_filename}.s ASM file.")
    else()
        message(WARNING "${asm_filename}.s cannot be updated, new ASM does not exists. Please run ninja asm_gen to create one.")
    endif()

  endforeach()

endfunction(asm_update)

asm_update()
