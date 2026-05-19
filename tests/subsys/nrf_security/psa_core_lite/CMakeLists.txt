#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project("PSA core lite unit tests")

# Add test sources

# Choose either KMU supported or import-based (RSA)
if(CONFIG_PSA_CORE_LITE_HAS_RSA OR CONFIG_PSA_CRYPTO_DRIVER_OBERON)
  target_sources(app PRIVATE src/main_rsa.c)
elseif(CONFIG_PSA_NEED_CRACEN_KMU_DRIVER)
  target_sources(app PRIVATE src/main_kmu.c)
endif()

# Hashing is not dependent on keys
if(CONFIG_PSA_CORE_LITE_HAS_HASH OR CONFIG_PSA_CRYPTO_DRIVER_OBERON)
  target_sources(app PRIVATE src/hash.c)
endif()
