#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)

# The application uses the conf/<board> scheme for configuration files.
set(APPLICATION_CONFIG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/conf/\${BOARD}")

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(peripheral_esl)

# NORDIC SDK APP START
target_sources(app PRIVATE
  src/main.c
  hw/esl_hw_impl.c
)

# Display HW implementation

if (CONFIG_DISPLAY)
  target_sources(app PRIVATE hw/display/epd_shield_display.c)
endif()

if (CONFIG_ESL_SIMULATE_DISPLAY)
  target_sources(app PRIVATE hw/display/led_display.c)
endif()

if (CONFIG_BT_ESL_DISPLAY_MAX EQUAL 0)
  target_sources(app PRIVATE hw/display/dummy_display.c)
endif()

# Sensor HW implementation
target_sources(app PRIVATE hw/sensor/sensor.c)

# LED HW implementation
target_sources(app PRIVATE hw/led/led.c)

# Storage HW implementation
if (CONFIG_BT_ESL_IMAGE_AVAILABLE)
  if (CONFIG_ESL_OTS_NVS)
    target_sources(app PRIVATE hw/storage/nvs.c)
  elseif (CONFIG_ESL_OTS_LFS)
    target_sources(app PRIVATE hw/storage/lfs.c)
  endif()
else()
  target_sources(app PRIVATE hw/storage/dummy.c)
endif()
# NORDIC SDK APP END

target_include_directories(app PRIVATE hw)
