#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

cmake_minimum_required(VERSION 3.8.2)

include(../../../cmake/boilerplate.cmake NO_POLICY_SCOPE)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(mqtt-aws-jobs-fota)

# NORDIC SDK APP START
target_sources(app PRIVATE
	src/main.c
	)

target_include_directories(app PRIVATE src)
# NORDIC SDK APP END
