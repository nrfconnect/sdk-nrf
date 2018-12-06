#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

add_library(bl_crypto STATIC bl_crypto.c)
target_link_libraries(bl_crypto PRIVATE zephyr_interface nrfxlib_crypto)
target_include_directories(bl_crypto PUBLIC ../include)
target_link_libraries_ifdef(CONFIG_SECURE_BOOT_DEBUG bl_crypto PRIVATE debuglib)

if (CONFIG_SB_CRYPTO_OBERON_ECDSA_SECP256R1)
  target_sources(bl_crypto PRIVATE bl_crypto_oberon_ecdsa.c)
elseif (CONFIG_SB_CRYPTO_CC310_ECDSA_SECP256R1)
  target_sources(bl_crypto PRIVATE bl_crypto_cc310_ecdsa.c)
  target_sources(bl_crypto PRIVATE bl_crypto_cc310_common.c)
else()
  message(FATAL_ERROR "No signature implementation chosen for bootloader.")
endif()

if (CONFIG_SB_CRYPTO_OBERON_SHA256)
  target_sources(bl_crypto PRIVATE bl_crypto_oberon_hash.c)
elseif (CONFIG_SB_CRYPTO_CC310_SHA256)
  target_sources(bl_crypto PRIVATE bl_crypto_cc310_hash.c)
  target_sources(bl_crypto PRIVATE bl_crypto_cc310_common.c)
else()
  message(FATAL_ERROR "No hash implementation chosen for bootloader.")
endif()
