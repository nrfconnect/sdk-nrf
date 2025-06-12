#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Collects all Matter-related #define statements from generated header files
# and writes them to a single output file (.config_matter) in the build directory.
#
# This function:
#   - Searches recursively for all header files in the generated Matter include directory.
#   - Extracts lines starting with '#define' from these files.
#   - Writes the collected defines to ${APPLICATION_BINARY_DIR}/zephyr/.config_matter.
#
# The resulting .config_matter file can be used for diagnostics, reproducibility,
# or as input to other build steps.
function(collect_matter_defines)
    set(MATTER_DEFINES_OUTPUT "${APPLICATION_BINARY_DIR}/zephyr/.config_matter")
    set(MATTER_TARGET_DIR "${APPLICATION_BINARY_DIR}/modules/connectedhomeip/gen/include")

    add_custom_command(
        OUTPUT ${MATTER_DEFINES_OUTPUT}
        COMMAND ${CMAKE_COMMAND} -Doutput_file=${MATTER_DEFINES_OUTPUT} -Dtarget_dir=${MATTER_TARGET_DIR} -P ${CMAKE_CURRENT_LIST_DIR}/collect_config.cmake
        DEPENDS chip
    )
    add_custom_target(collect_matter_defines_target ALL
        DEPENDS ${MATTER_DEFINES_OUTPUT}
    )
    add_dependencies(collect_matter_defines_target chip)
endfunction()

if(DEFINED output_file AND DEFINED target_dir)
    file(GLOB_RECURSE all_files "${target_dir}/*")
    file(REMOVE "${output_file}")
    foreach(file_path IN LISTS all_files)
        execute_process(
            COMMAND grep "^#define" "${file_path}"
            OUTPUT_VARIABLE defines
            RESULT_VARIABLE grep_result
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        if(grep_result EQUAL 0)
            file(APPEND "${output_file}" "${defines}\n")
        endif()
    endforeach()
    message(STATUS "Collected Matter configuration: ${output_file}")
endif()
