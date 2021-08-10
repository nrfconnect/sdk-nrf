#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(sms_test)

# generate runner for the test
test_runner_generate(src/sms_test.c)

cmock_handle(../../../include/modem/at_cmd.h)

# add test file
target_sources(app PRIVATE src/sms_test.c)
