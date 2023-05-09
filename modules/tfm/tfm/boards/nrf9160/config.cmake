#
# Copyright (c) 2021 - 2023, Nordic Semiconductor ASA.
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

include(${CMAKE_CURRENT_LIST_DIR}/../common/config.cmake)

set(PLATFORM_PATH platform/ext/target/nordic_nrf/)
include(${PLATFORM_PATH}/common/nrf9160/config.cmake)

# Override the AEAD algorithm configuration since nRF9160 supports only AES_CCM
set(PS_CRYPTO_AEAD_ALG                  PSA_ALG_CCM CACHE STRING    "The AEAD algorithm to use for authenticated encryption in Protected Storage")
