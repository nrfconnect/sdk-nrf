#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(dfu_multi_image)

set(dfu_multi_image_sample_dir ${ZEPHYR_NRF_MODULE_DIR}/samples/dfu/dfu_multi_image)

target_sources(app PRIVATE
  ${dfu_multi_image_sample_dir}/src/main.c
  ${dfu_multi_image_sample_dir}/src/dfu_multi_image_shell.c
)
