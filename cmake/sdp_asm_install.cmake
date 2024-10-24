#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

function(asm_install)

  foreach(hrt_src ${hrt_srcs})
    get_filename_component(asm_filename ${hrt_src} NAME_WE)  # filename without extension
    get_filename_component(src_dir ${hrt_src} DIRECTORY)

    file(RENAME ${asm_filename}-temp.s ${src_dir}/${asm_filename}.s RESULT rename_result)

    if (rename_result EQUAL 0)
        message("Updated ${asm_filename}.s ASM file.")
    else()
        message(WARNING "${asm_filename}.s cannot be updated, new ASM does not exist. Please run ninja asm_gen to create one.")
    endif()

  endforeach()

endfunction(asm_install)

asm_install()
