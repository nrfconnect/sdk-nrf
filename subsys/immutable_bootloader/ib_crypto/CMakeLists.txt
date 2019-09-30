#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

zephyr_library()
zephyr_library_sources(ib_crypto.c)
zephyr_library_link_libraries(${IMAGE}nrfxlib_crypto)

if (CONFIG_IB_CRYPTO_OBERON_ECDSA_SECP256R1)
  zephyr_library_sources(ib_crypto_oberon_ecdsa.c)
elseif (CONFIG_IB_CRYPTO_CC310_ECDSA_SECP256R1)
  zephyr_library_sources(
    ib_crypto_cc310_ecdsa.c
    ib_crypto_cc310_common.c
    )
else()
  message(FATAL_ERROR "No signature implementation chosen for bootloader.")
endif()

if (CONFIG_IB_CRYPTO_OBERON_SHA256)
  zephyr_library_sources(ib_crypto_oberon_hash.c)
elseif (CONFIG_IB_CRYPTO_CC310_SHA256)
  zephyr_library_sources(
    ib_crypto_cc310_hash.c
    ib_crypto_cc310_common.c
    )
else()
  message(FATAL_ERROR "No hash implementation chosen for bootloader.")
endif()

zephyr_link_libraries(nrfxlib_crypto)
