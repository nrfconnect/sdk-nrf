#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Function that replaces .s file tracked in repository with updated version from build
#
# Arguments:
# hrt_srcs - path to the .c file(s) to verify
# soc      - name of the SoC the code is being built for
function(asm_install)
  if(NOT DEFINED soc)
    message(FATAL_ERROR "asm_install missing soc argument.")
  endif()

  foreach(hrt_src ${hrt_srcs})
    get_filename_component(asm_filename ${hrt_src} NAME_WE)  # filename without extension
    get_filename_component(src_dir ${hrt_src} DIRECTORY)

    file(RENAME ${asm_filename}-${soc}-temp.s ${src_dir}/${asm_filename}-${soc}.s RESULT rename_result)

    if(rename_result EQUAL 0)
        message("Updated ${asm_filename}-${soc}.s ASM file.")
    else()
        message(WARNING "${asm_filename}-${soc}.s cannot be updated, new ASM does not exist. Please run ninja asm_gen to create one.")
    endif()
  endforeach()
endfunction(asm_install)

asm_install()
