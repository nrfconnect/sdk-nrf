#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(NONE)

# Application sources
target_sources_ifdef(CONFIG_FEM app
			PRIVATE ../../bluetooth/direct_test_mode/src/fem/fem.c)
target_sources_ifdef(CONFIG_NRF21540_FEM app
			PRIVATE ../../bluetooth/direct_test_mode/src/fem/nrf21540.c)
target_sources_ifdef(CONFIG_GENERIC_FEM app
			PRIVATE ../../bluetooth/direct_test_mode/src/fem/generic_fem.c)
zephyr_include_directories(../../bluetooth/direct_test_mode/src/fem/)

FILE(GLOB app_sources src/*.c)
# NORDIC SDK APP START
target_sources(app PRIVATE
	${app_sources}
)
# NORDIC SDK APP END
