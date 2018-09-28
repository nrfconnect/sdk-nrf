#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

zephyr_include_directories(include)

add_subdirectory(lib)
add_subdirectory(subsys)

# Add nrfxlib repository, if found
set(NRFXLIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../nrfxlib)
if(EXISTS ${NRFXLIB_DIR})
	add_subdirectory(${NRFXLIB_DIR} ${PROJECT_BINARY_DIR}/nrfxlib)
endif()
