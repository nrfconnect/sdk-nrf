#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library_named(fp_crypto)

if(CONFIG_BT_FAST_PAIR_CRYPTO)
  target_sources(fp_crypto PRIVATE fp_crypto_common.c)
endif()
if(CONFIG_BT_FAST_PAIR_CRYPTO_MBEDTLS)
  target_sources(fp_crypto PRIVATE fp_crypto_mbedtls.c)
endif()
if(CONFIG_BT_FAST_PAIR_CRYPTO_OBERON)
  target_sources(fp_crypto PRIVATE fp_crypto_oberon.c)
  target_link_libraries(fp_crypto PRIVATE nrfxlib_crypto)
endif()
if(CONFIG_BT_FAST_PAIR_CRYPTO_TINYCRYPT)
  target_sources(fp_crypto PRIVATE fp_crypto_tinycrypt.c)
endif()

target_include_directories(fp_crypto PUBLIC include)
target_include_directories(fp_crypto PUBLIC ../include/common)
