# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

if(SB_CONFIG_FLPRCORE_HELLO_WORLD_CPUFLPR)
  add_dependencies(${DEFAULT_IMAGE} hello_world_cpuflpr)
  sysbuild_add_dependencies(FLASH ${DEFAULT_IMAGE} hello_world_cpuflpr)
endif()
