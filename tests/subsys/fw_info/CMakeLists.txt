#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

cmake_minimum_required(VERSION 3.8.2)

# Cannot choose via CONFIG_SOC_NRF9160 because this has to be run before
# boilerplate.cmake.
get_filename_component(provision_path provision.hex ABSOLUTE)
#get_filename_component(provision_path provision_nrf9160.hex ABSOLUTE)

# Modified provision(_nrf9160).hex generated with command
# nRF9160:
#   python3 ../../../scripts/bootloader/provision.py --s0-addr 0x8000
#	  --s1-addr 0x20000 --provision-addr 0xff8108 --public-key-files
#	  build/zephyr/nrf/subsys/bootloader/generated/public.pem,
#	  test_public_0.pem,test_public_1.pem --output provision_nrf9160.hex
# Other:
#   python3 ../../../scripts/bootloader/provision.py --s0-addr 0x8000
#	  --s1-addr 0x20000 --provision-addr 0x7000 --public-key-files
#	  build/zephyr/nrf/subsys/bootloader/generated/public.pem,
#	  test_public_0.pem,test_public_1.pem --output provision.hex

set_property(
  GLOBAL PROPERTY
  provision_PM_HEX_FILE
  ${provision_path})

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)

project(NONE)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
