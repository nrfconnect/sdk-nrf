#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if (PSA_NEED_NRF_RNG_ENTROPY_DRIVER)
  target_sources(${mbedcrypto_target}
    PRIVATE
      nrf_rng_entropy.c
  )
endif()
