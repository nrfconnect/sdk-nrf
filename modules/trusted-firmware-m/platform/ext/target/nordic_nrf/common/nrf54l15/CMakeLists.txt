#-------------------------------------------------------------------------------
# Copyright (c) 2020-2022, Arm Limited. All rights reserved.
# Copyright (c) 2020, Nordic Semiconductor ASA.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

cmake_policy(SET CMP0076 NEW)
set(CMAKE_CURRENT_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR})

set(OOT_PLATFORM_PATH ${PSA_ARCH_TESTS_PATH}/../trusted-firmware-m/platform/ext/target/nordic_nrf)

set(target nrf54l15)

add_subdirectory(${OOT_PLATFORM_PATH}/common/core nrf_common)

#========================= Platform common defs ===============================#

#========================= Platform Secure ====================================#

target_include_directories(platform_s
    PUBLIC
        .
)

set(mdk_system_source_file ${HAL_NORDIC_PATH}/nrfx/mdk/system_nrf54l.c)

target_sources(platform_s PRIVATE ${mdk_system_source_file})

target_compile_definitions(platform_s
    PUBLIC
        NRF_SKIP_FICR_NS_COPY_TO_RAM
        $<$<BOOL:${TFM_ITS_ENCRYPTED}>:TFM_ITS_ENCRYPTED>
        $<$<BOOL:${TFM_ITS_ENCRYPTED}>:TFM_ITS_ENC_NONCE_LENGTH=${TFM_ITS_ENC_NONCE_LENGTH}>
        $<$<BOOL:${TFM_ITS_ENCRYPTED}>:TFM_ITS_AUTH_TAG_LENGTH=${TFM_ITS_AUTH_TAG_LENGTH}>
)

#========================= Platform Non-Secure ================================#

target_include_directories(platform_ns
    PUBLIC
        .
)

target_sources(platform_ns
    PRIVATE
        ${mdk_system_source_file}
)

target_compile_definitions(platform_ns
    PUBLIC
	    # TODO: Remove this CMake code when MLT-8314 is fixed in the MDK
        NRF_SKIP_GLITCHDETECTOR_DISABLE

        NRF_TRUSTZONE_NONSECURE
        DOMAIN_NS=1
)

#========================= Platform BL2 =======================================#

if(BL2)
    target_include_directories(platform_bl2
        PRIVATE
            .
    )

    target_sources(platform_bl2
        PRIVATE
            ${mdk_system_source_file}
    )
endif()

#========================= Platform region defs ===============================#

target_compile_definitions(platform_region_defs
    INTERFACE
        $<$<BOOL:${NRF_NS_STORAGE}>:NRF_NS_STORAGE>
        $<$<BOOL:${NRF_NS_SECONDARY}>:NRF_NS_SECONDARY>
)

#========================= tfm_spm ============================================#

target_sources(tfm_spm
    PRIVATE
        $<$<OR:$<BOOL:${CONFIG_TFM_FLIH_API}>,$<BOOL:${CONFIG_TFM_SLIH_API}>>:${CMAKE_CURRENT_SOURCE_DIR}/tfm_interrupts.c>
)
