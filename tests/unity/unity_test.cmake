#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)

set(UNITY_PRODUCTS_DIR "${APPLICATION_BINARY_DIR}/unity")
set(CMOCK_PRODUCTS_DIR "${UNITY_PRODUCTS_DIR}/mocks")
set(CMOCK_DIR "${ZEPHYR_BASE}/../test/cmock")
file(MAKE_DIRECTORY "${UNITY_PRODUCTS_DIR}")

find_program(
  RUBY_EXECUTABLE
  ruby
)
if(${RUBY_EXECUTABLE} STREQUAL RUBY-NOTFOUND)
    message(FATAL_ERROR "Unable to find ruby")
endif()

# Generate cmock for provided header file.
function(cmock_generate header_path dst_path)
    file(MAKE_DIRECTORY "${dst_path}")

    execute_process(
      COMMAND ${RUBY_EXECUTABLE}
      ${CMOCK_DIR}/lib/cmock.rb
      --mock_prefix=mock_
      --plugins=return_thru_ptr\;ignore_arg\;ignore\;callback\;array
      --mock_path=${dst_path}
      ${header_path}
      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
      RESULT_VARIABLE op_result
      OUTPUT_VARIABLE output_result
    )

    if (NOT ${op_result} EQUAL 0)
          message(SEND_ERROR "${output_result}")
          message(FATAL_ERROR "Failed to generate runner ${output_file}")
      endif()
    
    target_include_directories(app PRIVATE ${CMOCK_PRODUCTS_DIR})
    #target_include_directories(app PRIVATE ${dst_path})
    file(GLOB app_sources ${dst_path}/*.c)
    target_sources(app PRIVATE ${app_sources})
endfunction()

# Generate test runner file.
function(test_runner_generate test_file_path)
    get_filename_component(test_file_name "${test_file_path}" NAME)
    set(output_file "${UNITY_PRODUCTS_DIR}/runner/runner_${test_file_name}")
    file(MAKE_DIRECTORY "${UNITY_PRODUCTS_DIR}/runner")

    execute_process(
      COMMAND ${RUBY_EXECUTABLE}
      ${CMOCK_DIR}/vendor/unity/auto/generate_test_runner.rb
      ${test_file_path} ${output_file}
      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
      RESULT_VARIABLE op_result
    )

    if (NOT ${op_result} EQUAL 0)
        message(FATAL_ERROR "Failed to generate runner ${output_file}")
    endif()

    file(GLOB app_sources ${UNITY_PRODUCTS_DIR}/runner/*.c)
    target_sources(app PRIVATE ${app_sources})

    message(STATUS "Generating test runner ${unity_runner_arg3}")
endfunction()

# Add --wrap linker option for each function listed in the input file.
function(cmock_linker_trick func_name_path)
    file(READ "${func_name_path}" contents)
    string(REGEX REPLACE "\n" ";" contents "${contents}")
    set(linker_str "-Wl")
    foreach(src ${contents})
        set(linker_str "${linker_str},--wrap=${src}")
    endforeach()
    zephyr_link_libraries(${linker_str})
endfunction()


# Handle wrapping functions from mocked file.
# Function takes header file and generates a file containing list of functions.
# File is then passed to 'cmock_linker_trick' which adds linker option for each
# function listed in the file.
function(cmock_linker_wrap_trick header_file_path)
    set(flist_file "${header_file_path}.flist")

    execute_process(
      COMMAND
      ${PYTHON_EXECUTABLE}
      ${ZEPHYR_BASE}/../nrf/scripts/unity/func_name_list.py
      --input ${header_file_path}
      --output ${flist_file}
      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
      RESULT_VARIABLE op_result
      OUTPUT_VARIABLE output_result
    )

    if (NOT ${op_result} EQUAL 0)
        message(SEND_ERROR "${output_result}")
        message(FATAL_ERROR "Failed to parse header ${header_file_path}")
    endif()
    cmock_linker_trick(${flist_file})
endfunction()

# Function takes original header and prepares two version
# - version with system calls removed and static inline functions
#   converted to standard function declarations
# - version with addtional __wrap_ prefix for all functions that
#   is used to generate cmock
function(cmock_headers_prepare in_header out_header wrap_header)
    execute_process(
      COMMAND
      ${PYTHON_EXECUTABLE}
      ${ZEPHYR_BASE}/../nrf/scripts/unity/header_prepare.py
      "--input" ${in_header}
      "--output" ${out_header}
      "--wrap" ${wrap_header}
      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
      RESULT_VARIABLE op_result
      OUTPUT_VARIABLE output_result
    )

    if (NOT ${op_result} EQUAL 0)
        message(SEND_ERROR "${output_result}")
        message(FATAL_ERROR "Failed to parse header ${in_header}")
    endif()
endfunction()

#function for handling usage of mock
#optional second argument can contain offset that include should be placed in
#for example if file under test is include mocked header as <foo/header.h> then
# mock and replaced header should be placed in <mock_path>/foo with <mock_path>
# added as include path.
function(cmock_handle header_file)
    #get optional offset macro
    set (extra_macro_args ${ARGN})
    list(LENGTH extra_macro_args num_extra_args)
    if (${num_extra_args} EQUAL 1)
        list(GET extra_macro_args 0 optional_offset)
        set(dst_path "${CMOCK_PRODUCTS_DIR}/${optional_offset}")
    else()
        set(dst_path "${CMOCK_PRODUCTS_DIR}")
    endif()

    file(MAKE_DIRECTORY "${dst_path}/internal")

    get_filename_component(header_name "${header_file}" NAME)
    set(mod_header_path "${dst_path}/${header_name}")
    set(wrap_header "${dst_path}/internal/${header_name}")

    cmock_headers_prepare(${header_file} ${mod_header_path} ${wrap_header})
    cmock_generate(${wrap_header} ${dst_path})

    cmock_linker_wrap_trick(${mod_header_path})
    message(STATUS "Generating cmock for header ${header_file}")
endfunction()

