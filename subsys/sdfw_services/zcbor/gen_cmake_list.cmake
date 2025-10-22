#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

set(cddl_name ${CDDL_NAME})
set(install_dir ${INSTALL_DIR})

set(CMAKE_LISTS_FILE ${install_dir}/CMakeLists.txt)
string(TIMESTAMP current_year "%Y" UTC)
set(CMAKE_LISTS_CONTENT "\
#
# Copyright (c) ${current_year} Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

#
# Generated using cmake macro generate_and_add_cbor_files.
#

zephyr_sources(
	${cddl_name}_decode.c
	${cddl_name}_encode.c
)

zephyr_include_directories(.)
")

file(WRITE ${CMAKE_LISTS_FILE} "${CMAKE_LISTS_CONTENT}")
