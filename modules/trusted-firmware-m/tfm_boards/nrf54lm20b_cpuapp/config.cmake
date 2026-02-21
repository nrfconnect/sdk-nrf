#-------------------------------------------------------------------------------
# Copyright (c) 2026, Nordic Semiconductor ASA.
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
#-------------------------------------------------------------------------------

include(${CMAKE_CURRENT_LIST_DIR}/../common/config.cmake)

set(NRF_SOC_VARIANT nrf54lm20b CACHE STRING "nRF SoC Variant")

include(${PLATFORM_PATH}/common/${NRF_SOC_VARIANT}/config.cmake)

# Override PS_CRYPTO_KDF_ALG
set(PS_CRYPTO_KDF_ALG                  PSA_ALG_SP800_108_COUNTER_CMAC CACHE STRING    "KDF Algorithm to use")

# attest_hal.c includes bl_storage.h, which needs CONFIG_NRFX_RRAMC to be defined.
# This is because bl_storage is a lib intended to be run from either the bootloader (Zephyr) or from TF-M.
# This is independent from the NS image's CONFIG_NRFX_RRAMC, which must be disabled, so we can not inherit
# this from app Kconfig.
if(TFM_PARTITION_INITIAL_ATTESTATION)
  add_compile_definitions(CONFIG_NRFX_RRAMC)
endif()
