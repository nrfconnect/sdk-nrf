# CMakeLists.txt for generating protobuf definitions
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

# Define protobuf source files
set(PROTO_FILES
    request.proto
    common.proto
    response.proto
    result.proto
    version.proto
)

# Generate Python protobuf files for the configuration script
if(PROTOBUF_PROTOC_EXECUTABLE)
    add_custom_target(generate_python_proto_files ALL
        COMMAND ${PROTOBUF_PROTOC_EXECUTABLE}
            --python_out=${CMAKE_CURRENT_SOURCE_DIR}
            --proto_path=${CMAKE_CURRENT_SOURCE_DIR}
            ${PROTO_FILES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating Python protobuf files"
        VERBATIM
    )
else()
    message(WARNING "protoc not found - Python protobuf files will not be generated")
endif()

# Create custom target for cleaning
add_custom_target(clean_proto_files
    COMMAND ${CMAKE_COMMAND} -E echo "Cleaning generated files..."
    COMMAND ${CMAKE_COMMAND} -E remove *.pb.c *.pb.h *_pb2.py
    COMMAND ${CMAKE_COMMAND} -E echo "Cleaned!"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Cleaning generated protobuf files"
)

# Create custom target for testing
add_custom_target(test_proto_files
    COMMAND ${CMAKE_COMMAND} -E echo "Running auth mode validation tests..."
    COMMAND python3 test_auth_modes.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running protobuf validation tests"
    DEPENDS generate_python_proto_files
)
