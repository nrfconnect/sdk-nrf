#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(pm_test)

# Generate runner for the test
test_runner_generate(pm_test.c)

# Create mocks for pm module.
cmock_handle(${ZEPHYR_BASE}/include/zephyr/pm/policy.h)
cmock_handle(${ZEPHYR_NRFXLIB_MODULE_DIR}/mpsl/include/mpsl_pm.h)
cmock_handle(${ZEPHYR_NRFXLIB_MODULE_DIR}/mpsl/include/mpsl_pm_config.h)
cmock_handle(${ZEPHYR_NRF_MODULE_DIR}/include/mpsl/mpsl_work.h)

# Add Unit Under Test source files
target_sources(app PRIVATE ${ZEPHYR_NRF_MODULE_DIR}/subsys/mpsl/pm/mpsl_pm_utils.c)

# Add test source file
target_sources(app PRIVATE pm_test.c)

# Include paths
target_include_directories(app PRIVATE src)

# Preinclude file to the UUT to redefine mpsl_work_schedule().
set_property(SOURCE ${ZEPHYR_NRF_MODULE_DIR}/subsys/mpsl/pm/mpsl_pm_utils.c
        PROPERTY COMPILE_FLAGS "-include mocks/mpsl_work.h")

# Options that cannot be passed through Kconfig fragments.
target_compile_options(app PRIVATE
	-DCONFIG_PM=y
	-DCONFIG_MPSL_USE_ZEPHYR_PM=y
)
