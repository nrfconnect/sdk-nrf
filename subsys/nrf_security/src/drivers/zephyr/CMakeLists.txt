#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if (PSA_NEED_ZEPHYR_ENTROPY_DRIVER)
  # Add nrf_cc3xx_platform includes for mbedcrypto
  target_include_directories(${mbedcrypto_target}
    INTERFACE
      include
  )

  target_sources(${mbedcrypto_target}
    PRIVATE
      zephyr_entropy.c
  )
endif()
