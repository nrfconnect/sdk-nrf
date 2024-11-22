#-------------------------------------------------------------------------------
# Copyright (c) 2024, Nordic Semiconductor ASA.
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
#-------------------------------------------------------------------------------

include(${CMAKE_CURRENT_LIST_DIR}/../common/config.cmake)

set(NRF_SOC_VARIANT nrf54l15 CACHE STRING "nRF SoC Variant")

include(${PLATFORM_PATH}/common/${NRF_SOC_VARIANT}/config.cmake)

# Override PS_CRYPTO_KDF_ALG
set(PS_CRYPTO_KDF_ALG                  PSA_ALG_SP800_108_COUNTER_CMAC CACHE STRING    "KDF Algorithm to use")

set(CONFIG_NRFX_RRAMC                ON          CACHE BOOL      "Enable nrfx drivers for RRAMC")
add_compile_definitions(CONFIG_NRFX_RRAMC)

# Override attestation to sign message instead of hash, because CRACEN drivers can not sign a hash.
set(ATTEST_SIGN_MESSAGE               ON         CACHE BOOL      "Sign message instead of hash")
