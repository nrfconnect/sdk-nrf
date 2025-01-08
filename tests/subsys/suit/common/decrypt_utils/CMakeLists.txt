# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(CONFIG_SUIT_STREAM_FILTER_DECRYPT)
  zephyr_library_named(suit_decrypt_test_utils)
  zephyr_include_directories(.)
  zephyr_library_sources(decrypt_test_utils.c)
  zephyr_library_link_libraries(suit_metadata)

  if(CONFIG_MBEDTLS)
    # Link MCI (incl. crypto) module with mbedTLS library, that provides PSA crypto APIs.
    zephyr_library_link_libraries(mbedTLS)
  endif() # CONFIG_MBEDTLS
endif()
