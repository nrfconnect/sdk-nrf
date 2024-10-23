#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nRF_Cloud_REST_FOTA_Sample)
zephyr_compile_definitions(PROJECT_NAME=${PROJECT_NAME})

# NORDIC SDK APP START
target_sources(app PRIVATE src/main.c)

if(CONFIG_NRF_CLOUD_FOTA_SMP AND CONFIG_BOARD_NRF9160DK_NRF9160_NS)
  target_sources(app PRIVATE src/smp_reset.c)
endif()
# NORDIC SDK APP END
