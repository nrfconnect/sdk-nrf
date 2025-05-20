#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(CONFIG_APP_SMP_CLIENT_FOTA_EXTERNAL)
  zephyr_sources(${CMAKE_CURRENT_SOURCE_DIR}/client.c)
  zephyr_include_directories(include)
  zephyr_library_link_libraries(MCUBOOT_BOOTUTIL)
endif()
