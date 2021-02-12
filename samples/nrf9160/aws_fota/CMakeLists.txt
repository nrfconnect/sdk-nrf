#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
if(CONFIG_PROVISION_CERTIFICATES)
  message(WARNING "
      ------------------------------------------------------------
      --- WARNING: Provisioning certificates is ENABLED. Do    ---
      --- not use this binary in production or share it with   ---
      --- anyone. It has certificates stored in readable flash,---
      --- the binary, and the modem traces. Only use this      ---
      --- binary once to provision certificates for development---
      --- to reduce flash tear. After the certificates are     ---
      --- provisioned, disable this option and rebuild the     ---
      --- sample.                                              ---
      ------------------------------------------------------------")
endif()
project(mqtt-aws-jobs-fota)

# NORDIC SDK APP START
target_sources(app PRIVATE
	src/main.c
	)

target_include_directories(app PRIVATE src)
# NORDIC SDK APP END
