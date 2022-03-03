#
# Copyright (c) 2021 Nordic Semiconductor
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
set(APPLICATION_CONFIG_DIR "configuration/\${BOARD}")


find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project("Machine learning"
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
  src/util
)

zephyr_include_directories(
  configuration/common
  configuration/${BOARD}
)

# Application sources
add_subdirectory(src/events)
add_subdirectory(src/modules)
add_subdirectory(src/util)

if(CONFIG_BT_RPMSG)
  assert_exists(hci_rpmsg_CONF_FILE)
endif()
