#
# Copyright (c) 2023-2024, Nordic Semiconductor ASA.
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

include(${CMAKE_CURRENT_LIST_DIR}/../common/config.cmake)

set(NRF_SOC_VARIANT nrf91 CACHE STRING "nRF SoC Variant")

include(${PLATFORM_PATH}/common/${NRF_SOC_VARIANT}/config.cmake)

# Override the AEAD algorithm configuration since nRF91 series supports only AES_CCM
set(PS_CRYPTO_AEAD_ALG                  PSA_ALG_CCM CACHE STRING    "The AEAD algorithm to use for authenticated encryption in Protected Storage")
