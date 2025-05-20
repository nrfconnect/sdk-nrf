#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(adc_accuracy)

target_sources(app PRIVATE ${ZEPHYR_BASE}/tests/drivers/adc/adc_accuracy_test/src/main.c)
target_sources_ifdef(CONFIG_REFERENCE_VOLTAGE_TEST app PRIVATE ${ZEPHYR_BASE}/tests/drivers/adc/adc_accuracy_test/src/ref_volt.c)
