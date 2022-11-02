#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(random_hw_unique_key)

# NORDIC SDK APP START
target_sources(app PRIVATE src/main.c)
if (DEFINED CONFIG_BUILD_WITH_TFM)
  target_sources(app PRIVATE src/derive_key_tfm.c)
else ()
  target_sources(app PRIVATE src/derive_key.c)
endif()
# NORDIC SDK APP END
