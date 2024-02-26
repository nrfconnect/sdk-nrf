#-------------------------------------------------------------------------------
# Copyright (c) 2024, Nordic Semiconductor ASA.
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
#-------------------------------------------------------------------------------

include(${PLATFORM_PATH}/common/core/config.cmake)

set(SECURE_UART1                        ON         CACHE BOOL      "Enable secure UART1")
set(PSA_API_TEST_TARGET                 "nrf54l15" CACHE STRING    "PSA API test target")
set(NRF_NS_STORAGE                      OFF        CACHE BOOL      "Enable non-secure storage partition")
set(BL2                                 ON         CACHE BOOL      "Whether to build BL2")
set(NRF_NS_SECONDARY                    ${BL2}     CACHE BOOL      "Enable non-secure secondary partition")
set(TFM_ITS_ENC_NONCE_LENGTH            "12"       CACHE STRING    "The size of the nonce used in ITS encryption in bytes")
set(TFM_ITS_AUTH_TAG_LENGTH             "16"       CACHE STRING    "The size of the tag used in ITS encryption in bytes")
