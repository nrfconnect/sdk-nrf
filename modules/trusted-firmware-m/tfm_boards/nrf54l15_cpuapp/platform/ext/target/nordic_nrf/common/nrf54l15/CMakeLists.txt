#-------------------------------------------------------------------------------
# Copyright (c) 2024, Nordic Semiconductor ASA.
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
#-------------------------------------------------------------------------------

cmake_policy(SET CMP0076 NEW)
set(CMAKE_CURRENT_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR})

set(target nrf54l15)

add_subdirectory(${PLATFORM_PATH}/common/core nrf_common)

#========================= Platform Secure ====================================#

target_include_directories(platform_s
    PUBLIC
        .
)

set(mdk_system_source_file ${HAL_NORDIC_PATH}/nrfx/mdk/system_nrf54l.c)

target_sources(platform_s PRIVATE ${mdk_system_source_file})

target_compile_definitions(platform_s
    PUBLIC
        $<$<BOOL:${TFM_ITS_ENCRYPTED}>:TFM_ITS_ENCRYPTED>
        $<$<BOOL:${TFM_ITS_ENCRYPTED}>:TFM_ITS_ENC_NONCE_LENGTH=${TFM_ITS_ENC_NONCE_LENGTH}>
        $<$<BOOL:${TFM_ITS_ENCRYPTED}>:TFM_ITS_AUTH_TAG_LENGTH=${TFM_ITS_AUTH_TAG_LENGTH}>
)

#========================= tfm_spm ============================================#

target_sources(tfm_spm
    PRIVATE
        $<$<OR:$<BOOL:${CONFIG_TFM_FLIH_API}>,$<BOOL:${CONFIG_TFM_SLIH_API}>>:${CMAKE_CURRENT_SOURCE_DIR}/tfm_interrupts.c>
)
