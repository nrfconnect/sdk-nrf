#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)

if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}")
  message(FATAL_ERROR
          "Board ${BOARD} is not supported.\n"
          "Please make sure board specific configuration files are added to "
          "${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}")
endif()

################################################################################

# The application uses the configuration/<board> scheme for configuration files.
# Configuring here to ensure BOARD contains BOARD_REVISION
set(APPLICATION_CONFIG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/configuration/\${NORMALIZED_BOARD_TARGET}")

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project("Peripheral Sensor Node"
        VERSION 0.1)

################################################################################

# NORDIC SDK APP START
target_sources(app PRIVATE
  src/main.c
  )
# NORDIC SDK APP END

# Include application events and configuration headers
target_include_directories(app PRIVATE
  src/events
  src/modules
)

zephyr_include_directories(
  configuration/common
  ${APPLICATION_CONFIG_DIR}
)

add_subdirectory(src/events)
add_subdirectory(src/modules)
