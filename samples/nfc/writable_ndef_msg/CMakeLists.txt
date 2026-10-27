#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(NONE)

# NORDIC SDK APP START
if(CONFIG_SOC_NRF9251_CPUAPP)
target_sources(app PRIVATE src/main_cpuapp_idle.c)
else()
target_sources(app PRIVATE src/main.c src/ndef_file_m.c)
endif()
# NORDIC SDK APP END
