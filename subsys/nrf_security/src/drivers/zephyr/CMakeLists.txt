#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Add the RNG driver to the Oberon PSA core (if needed)
if (PSA_NEED_NRF_RNG_ENTROPY_DRIVER)
  target_sources(${oberon_psa_core}
    PRIVATE
      nrf_rng_entropy.c
  )
endif()
