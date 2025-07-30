# CMakeLists.txt for generating nanopb C definitions
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

# Create custom target for nanopb generation (for embedded code)
add_custom_target(generate_nanopb ALL
    COMMAND ${CMAKE_COMMAND} -E echo "Generating nanopb C definitions..."
    COMMAND protoc --nanopb_out=. ${PROTO_FILES}
    COMMAND ${CMAKE_COMMAND} -E echo "Generated nanopb files:"
    COMMAND ${CMAKE_COMMAND} -E ls -la *.pb.c *.pb.h
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating nanopb C definitions"
    VERBATIM
)

# Create custom target for cleaning
add_custom_target(clean_nanopb
    COMMAND ${CMAKE_COMMAND} -E echo "Cleaning generated files..."
    COMMAND ${CMAKE_COMMAND} -E remove *.pb.c *.pb.h
    COMMAND ${CMAKE_COMMAND} -E echo "Cleaned!"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Cleaning generated nanopb files"
)

# Create custom target for testing
add_custom_target(test_nanopb
    COMMAND ${CMAKE_COMMAND} -E echo "Running auth mode validation tests..."
    COMMAND python3 test_auth_modes.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running nanopb validation tests"
)

# Add dependencies to the main project if it exists
if(TARGET ${PROJECT_NAME})
    add_dependencies(${PROJECT_NAME} generate_nanopb)
endif()
