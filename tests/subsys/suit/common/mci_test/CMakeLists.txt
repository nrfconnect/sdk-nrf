# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if (CONFIG_SUIT_MCI_IMPL_CUSTOM)
  zephyr_library_named(mci_test)
  zephyr_library_sources(mci_test.c)
  zephyr_library_link_libraries(suit_mci)
  zephyr_library_link_libraries(suit_metadata)
  zephyr_library_link_libraries(suit_execution_mode)

  if (CONFIG_MBEDTLS)
    # Link MCI (incl. crypto) module with mbedTLS library, that provides PSA crypto APIs.
    zephyr_library_link_libraries(mbedTLS)
  endif() # CONFIG_MBEDTLS

  target_link_libraries(app PUBLIC mci_test)
endif()
