#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

cmake_minimum_required(VERSION 3.8.2)

# Modified provision(_nrf9160).hex generated with command
# nRF9160:
#   python3 ../../../scripts/bootloader/provision.py --s0-addr 0x8000
#	  --s1-addr 0x20000 --provision-addr 0xff8108 --public-key-files
#	  build/zephyr/nrf/subsys/bootloader/generated/public.pem,
#	  test_public_0.pem,test_public_1.pem --output provision_nrf9160.hex
# nRF5340_cpuapp:
#   python3 ../../../scripts/bootloader/provision.py --s0-addr 0x8000
#	  --s1-addr 0x20000 --provision-addr 0xff8100 --public-key-files
#	  build/zephyr/nrf/subsys/bootloader/generated/public.pem,
#	  test_public_0.pem,test_public_1.pem --output provision_nrf5340_cpuapp.hex
# Other:
#   python3 ../../../scripts/bootloader/provision.py --s0-addr 0x8000
#	  --s1-addr 0x20000 --provision-addr 0x7000 --public-key-files
#	  build/zephyr/nrf/subsys/bootloader/generated/public.pem,
#	  test_public_0.pem,test_public_1.pem --output provision.hex

# Cannot choose via CONFIG_SOC_* because provision_PM_HEX_FILE must be set
# before boilerplate.cmake is run.
if (NOT DEFINED provision_hex)
  if (BOARD MATCHES "nrf9160")
    set(provision_hex provision_nrf9160.hex)
  elseif (BOARD MATCHES "nrf5340")
    set(provision_hex provision_nrf5340_cpuapp.hex)
  else()
    set(provision_hex provision.hex)
  endif()
endif()

get_filename_component(provision_path ${provision_hex} ABSOLUTE)

set_property(
  GLOBAL PROPERTY
  provision_PM_HEX_FILE
  ${provision_path})

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)

if (NOT ((DEFINED CONFIG_SOC_NRF9160
              AND provision_hex STREQUAL "provision_nrf9160.hex")
          OR (DEFINED CONFIG_SOC_NRF5340_CPUAPP
              AND provision_hex STREQUAL "provision_nrf5340_cpuapp.hex")
          OR (NOT DEFINED CONFIG_SOC_NRF9160
              AND NOT DEFINED CONFIG_SOC_NRF5340_CPUAPP
              AND provision_hex STREQUAL "provision.hex")))
  message(FATAL_ERROR
    "This SoC needs a different provision hex file. Pass to Cmake with "
    "-Dprovision_hex=[hex file]"
    )
endif()

project(NONE)

FILE(GLOB app_sources ${CMAKE_CURRENT_LIST_DIR}/src/*.c)
target_sources(app PRIVATE ${app_sources})
