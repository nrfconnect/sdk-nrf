#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)

# Propagate shields to the Remote Shell child image, we want to build for nRF53 series
# in the same way like for nRF52 series.
# Use following command to build for nRF5340 with nRF21540 EK shield:
# west build nrf5340dk_nrf5340_cpunet -- -DSHIELD nrf21540_ek
# The shield will added to child image automatically.
# Yoy can also use standard command:
# west build -b nrf5340dk_nrf5340_cpunet -- -DSHIELD=nrf21540_ek -Dremote_shell_SHIELD=nrf21540ek
if (DEFINED SHIELD)
  # Do this only if user did not specify shield for this child image.
  if (NOT DEFINED remote_shell_SHIELD)
    message(STATUS "Set shields: ${SHIELD} for the child image")
    set(remote_shell_SHIELD ${remote_shell_SHIELD};${SHIELD})
  endif()
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(NONE)

# Application sources
add_subdirectory(src/fem)

# NORDIC SDK APP START
target_sources(app PRIVATE
  src/main.c
  src/dtm.c
  src/dtm_hw.c
)
# NORDIC SDK APP END
