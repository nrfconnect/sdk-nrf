# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if (CONFIG_SUIT_VALIDATOR_IMPL_CUSTOM)
  zephyr_library_named(validator_test)
  zephyr_library_sources(validator_test.c)
  zephyr_library_link_libraries(suit_validator)

  target_link_libraries(app PUBLIC validator_test)
endif()
