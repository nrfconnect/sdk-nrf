#
# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

set(EDGE_IMPULSE_DIR        ${CMAKE_BINARY_DIR}/edge_impulse)
set(EDGE_IMPULSE_SOURCE_DIR ${EDGE_IMPULSE_DIR}/src/edge_impulse_project)
set(EDGE_IMPULSE_BINARY_DIR ${EDGE_IMPULSE_DIR}/src/edge_impulse_project-build)
set(EDGE_IMPULSE_STAMP_DIR  ${EDGE_IMPULSE_DIR}/src/edge_impulse_project-stamp)
set(EDGE_IMPULSE_LIBRARY    ${EDGE_IMPULSE_BINARY_DIR}/libedge_impulse.a)

# Override Zephyr's Wdouble-promotion and unused variable warnings, as Edge Impulse gives warnings.
zephyr_compile_options(-Wno-double-promotion -Wno-unused)

file(GENERATE OUTPUT ${EDGE_IMPULSE_DIR}/compile_options.$<COMPILE_LANGUAGE>.cmake CONTENT
"set(EI_$<COMPILE_LANGUAGE>_COMPILE_OPTIONS \"$<TARGET_PROPERTY:zephyr_interface,INTERFACE_COMPILE_OPTIONS>\")"
)

if(CONFIG_EDGE_IMPULSE_URI STREQUAL "")
  message(FATAL_ERROR "CONFIG_EDGE_IMPULSE_URI must be specified")
endif()

# Match first and any URI in the middle of input string.
# URI can be separated by space, new line or hyphen.
string(REGEX MATCHALL ".+[ \r\n;]" EI_URI_PREPARE_LIST ${CONFIG_EDGE_IMPULSE_URI})
# Match the last URI in input string
string(REGEX MATCH "[^ \n\r;].+$" EI_URI_LIST_END ${CONFIG_EDGE_IMPULSE_URI})

list(APPEND EI_URI_PREPARE_LIST ${EI_URI_LIST_END})

foreach(EI_URI IN LISTS EI_URI_PREPARE_LIST)
  # Remove trailing spaces
  string(STRIP ${EI_URI} EI_URI_STRING)

  if(NOT ${EI_URI_STRING} MATCHES "^[a-z]+://")
    string(CONFIGURE ${EI_URI_STRING} EI_URI_STRING)
    if(NOT IS_ABSOLUTE ${EI_URI_STRING})
      # Using application source directory as base directory for relative path.
      set(EI_URI_STRING ${APPLICATION_SOURCE_DIR}/${EI_URI_STRING})
    endif()
  endif()

  list(APPEND EI_URI_LIST ${EI_URI_STRING})
endforeach()

# Remove duplicated URIs from list
list(REMOVE_DUPLICATES EI_URI_LIST)

file(GLOB_RECURSE  edge_impulse_all_headers "${EDGE_IMPULSE_SOURCE_DIR}/*.h")

if(SYSBUILD)
  zephyr_get(EI_API_KEY_HEADER SYSBUILD GLOBAL)
endif()

include(ExternalProject)
ExternalProject_Add(edge_impulse_project
    URL              ${EI_URI_LIST}
    HTTP_HEADER      "Accept: application/zip"
    DOWNLOAD_EXTRACT_TIMESTAMP True
                     ${EI_API_KEY_HEADER}
    PREFIX           ${EDGE_IMPULSE_DIR}
    SOURCE_DIR       ${EDGE_IMPULSE_SOURCE_DIR}
    BINARY_DIR       ${EDGE_IMPULSE_BINARY_DIR}
    STAMP_DIR        ${EDGE_IMPULSE_STAMP_DIR}
    DOWNLOAD_NAME    edge_impulse_src.zip
    BUILD_BYPRODUCTS ${EDGE_IMPULSE_LIBRARY}
                     ${edge_impulse_all_headers}
    PATCH_COMMAND    ${CMAKE_COMMAND} -E copy
                     ${CMAKE_CURRENT_LIST_DIR}/CMakeLists.ei.template
                     ${EDGE_IMPULSE_SOURCE_DIR}/CMakeLists.txt
    DEPENDS          zephyr_interface
    CMAKE_ARGS -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
               -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
               -DCMAKE_AR=${CMAKE_AR}
               -DCMAKE_RANLIB=${CMAKE_RANLIB}
               -DEI_COMPILE_DEFINITIONS=$<TARGET_PROPERTY:zephyr_interface,INTERFACE_COMPILE_DEFINITIONS>
               -DEI_INCLUDE_DIRECTORIES=$<TARGET_PROPERTY:zephyr_interface,INTERFACE_INCLUDE_DIRECTORIES>
               -DEI_SYSTEM_INCLUDE_DIRECTORIES=$<TARGET_PROPERTY:zephyr_interface,INTERFACE_SYSTEM_INCLUDE_DIRECTORIES>
               -DEI_LIBRARY_NAME=edge_impulse
    INSTALL_COMMAND ""
    BUILD_ALWAYS True
    USES_TERMINAL_BUILD True
)
add_dependencies(edge_impulse_project zephyr_generated_headers)
add_library(edge_impulse_imported STATIC IMPORTED)
set_target_properties(edge_impulse_imported PROPERTIES IMPORTED_LOCATION ${EDGE_IMPULSE_LIBRARY})
target_link_libraries(edge_impulse_imported INTERFACE kernel)

# This targets remove the `edge_impulse_project-download` stamp file, which
# causes the Edge impulse library to be fetched on each build invocation.
# Note: This also results in the `ALL` target to always be considered out-of-date.
if(CONFIG_EDGE_IMPULSE_DOWNLOAD_ALWAYS)
  if(${CMAKE_VERSION} VERSION_LESS "3.17")
    set(REMOVE_COMMAND remove)
  else()
    set(REMOVE_COMMAND rm)
  endif()

  add_custom_target(edge_impulse_project_download
                    COMMAND ${CMAKE_COMMAND} -E ${REMOVE_COMMAND} -f
                            ${EDGE_IMPULSE_STAMP_DIR}/edge_impulse_project-download
                    DEPENDS edge_impulse_project
  )
endif()

zephyr_interface_library_named(edge_impulse)
target_include_directories(edge_impulse INTERFACE
                           ${EDGE_IMPULSE_SOURCE_DIR}
                           ${EDGE_IMPULSE_SOURCE_DIR}/tflite-model
                           ${EDGE_IMPULSE_SOURCE_DIR}/model-parameters
                           ${EDGE_IMPULSE_SOURCE_DIR}/edge-impulse-sdk/
                           ${EDGE_IMPULSE_SOURCE_DIR}/edge-impulse-sdk/third_party/ruy
                           ${EDGE_IMPULSE_SOURCE_DIR}/edge-impulse-sdk/third_party/gemmlowp
                           ${EDGE_IMPULSE_SOURCE_DIR}/edge-impulse-sdk/third_party/flatbuffers/include
                           ${EDGE_IMPULSE_SOURCE_DIR}/edge-impulse-sdk/third_party
                           ${EDGE_IMPULSE_SOURCE_DIR}/edge-impulse-sdk/tensorflow
                           ${EDGE_IMPULSE_SOURCE_DIR}/edge-impulse-sdk/dsp
                           ${EDGE_IMPULSE_SOURCE_DIR}/edge-impulse-sdk/classifier
                           ${EDGE_IMPULSE_SOURCE_DIR}/edge-impulse-sdk/anomaly
                           ${EDGE_IMPULSE_SOURCE_DIR}/edge-impulse-sdk/CMSIS/NN/Include
                           ${EDGE_IMPULSE_SOURCE_DIR}/edge-impulse-sdk/CMSIS/DSP/PrivateInclude
                           ${EDGE_IMPULSE_SOURCE_DIR}/edge-impulse-sdk/CMSIS/DSP/Include
                           ${EDGE_IMPULSE_SOURCE_DIR}/edge-impulse-sdk/CMSIS/Core/Include
)

target_link_libraries(edge_impulse INTERFACE edge_impulse_imported)

add_dependencies(edge_impulse
                 zephyr_interface
                 edge_impulse_project
                 edge_impulse_project_download
)

if(CONFIG_EI_WRAPPER)
  zephyr_library_named(ei_wrapper)
  zephyr_library_sources(ei_wrapper.cpp)
  zephyr_library_link_libraries(edge_impulse)
endif()
