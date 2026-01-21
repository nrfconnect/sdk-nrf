#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library_named(wifi_prov_core)

set(PROTOC_OPTIONS "-I${CMAKE_CURRENT_SOURCE_DIR}")
set(nanopb_BUILD_RUNTIME OFF)
set(CMAKE_MODULE_PATH ${ZEPHYR_NANOPB_MODULE_DIR}/extra)
find_package(Nanopb REQUIRED)
set(NANOPB_GENERATE_CPP_STANDALONE FALSE)

# Generate nanopb C files for embedded code
nanopb_generate_cpp(proto_sources proto_headers
  proto/common.proto
  proto/version.proto
  proto/result.proto
  proto/request.proto
  proto/response.proto
)

# Generate Python protobuf files for the configuration script
if(PROTOBUF_PROTOC_EXECUTABLE)
  set(PYTHON_PROTO_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/proto/common.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/proto/version.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/proto/result.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/proto/request.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/proto/response.proto
  )

  add_custom_target(generate_python_proto ALL
    COMMAND ${PROTOBUF_PROTOC_EXECUTABLE}
      --python_out=${CMAKE_CURRENT_SOURCE_DIR}/proto
      --proto_path=${CMAKE_CURRENT_SOURCE_DIR}/proto
      ${PYTHON_PROTO_FILES}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/proto
    COMMENT "Generating Python protobuf files"
    VERBATIM
  )

  # Make sure Python proto files are generated before the library
  add_dependencies(wifi_prov_core generate_python_proto)
endif()

# Add include path to generated .pb.h header files
zephyr_library_include_directories(${CMAKE_CURRENT_BINARY_DIR})

zephyr_library_sources(wifi_prov_handler.c ${proto_sources})
