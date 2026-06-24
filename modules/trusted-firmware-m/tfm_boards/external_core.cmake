#-------------------------------------------------------------------------------
# Copyright (c) 2024, Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
#-------------------------------------------------------------------------------

#=================== Build logic for external PSA core build ==================#

# Break out of this if PSA_CRYPTO_EXTERNAL_CORE is not set
if(NOT PSA_CRYPTO_EXTERNAL_CORE)
    return()
endif()

# Adjusting includes from spe-CMakeLists.txt which has the following heading:
#
# This CMake script is prepard by TF-M for building the non-secure side
# application and not used in secure build a tree being for export only.
# This file is renamed to spe/CMakeList.txt during installation phase
#
if(TARGET tfm_api_ns)
    set(EXTERNAL_CRYPTO_CORE_HANDLED_TFM_API_NS True)
    target_include_directories(tfm_api_ns
            PUBLIC
                    ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/include
                    ${INTERFACE_INC_DIR}/crypto_keys
    )
endif()

if(TARGET psa_interface)
    set(EXTERNAL_CRYPTO_CORE_HANDLED_PSA_INTERFACE True)
    include(${NRF_SECURITY_DIR}/cmake/psa_interface_include_directories.cmake)
endif()

# Constructing config libraries in partition/crypto/CMakeLists.txt

# Configurations files for the users of the client interface
if(TARGET psa_crypto_config)
    set(EXTERNAL_CRYPTO_CORE_HANDLED_PSA_CRYPTO_CONFIG True)
    target_compile_definitions(psa_crypto_config
        INTERFACE
            TF_PSA_CRYPTO_CONFIG_FILE="${CONFIG_TF_PSA_CRYPTO_CONFIG_FILE}"
            # Give a signal that we are inside TF-M build to prevent check_config.h
            # complaining about lacking legacy features for Mbed TLS wrapper APIs, TLS/DTLS and X.509.
            INSIDE_TFM_BUILD
    )

    target_include_directories(psa_crypto_config
        INTERFACE
            ${PSA_CRYPTO_CONFIG_INTERFACE_PATH}
            ${NRF_SECURITY_DIR}/include
            ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/include
            ${NRF_DIR}/include/tfm
    )
endif()

# This defines the configuration files for the users of the library directly
if(TARGET psa_crypto_library_config)
    set(EXTERNAL_CRYPTO_CORE_HANDLED_PSA_CRYPTO_LIBRARY_CONFIG True)
    target_compile_definitions(psa_crypto_library_config
        INTERFACE
            TF_PSA_CRYPTO_CONFIG_FILE="${CONFIG_TF_PSA_CRYPTO_CONFIG_FILE}"
            TF_PSA_CRYPTO_USER_CONFIG_FILE="${CONFIG_TF_PSA_CRYPTO_USER_CONFIG_FILE}"
    )

    target_include_directories(psa_crypto_library_config
        INTERFACE
            ${PSA_CRYPTO_CONFIG_LIBRARY_PATH}
            ${NRF_SECURITY_DIR}/include
            ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/include
            ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/oberon/drivers
            ${NRF_DIR}/include/tfm
    )

    target_compile_definitions(psa_crypto_library_config
        INTERFACE
            MBEDTLS_PSA_CRYPTO_DRIVERS
            $<$<BOOL:${CRYPTO_TFM_BUILTIN_KEYS_DRIVER}>:PSA_CRYPTO_DRIVER_TFM_BUILTIN_KEY PSA_CRYPTO_DRIVER_TFM_BUILTIN_KEY_LOADER>
    )
endif()

if(TARGET tfm_psa_rot_partition_crypto)
    set(EXTERNAL_CRYPTO_CORE_HANDLED_TFM_PSA_ROT_PARTITION_CRYPTO True)
    target_compile_definitions(tfm_psa_rot_partition_crypto
        PRIVATE
            PSA_CRYPTO_EXTERNAL_CORE
    )
    target_link_libraries(tfm_psa_rot_partition_crypto
        PRIVATE
            psa_crypto_library_config
            psa_interface
    )
endif()

if(TARGET ${MBEDTLS_TARGET_PREFIX}mbedcrypto)
    # Can't store the state as there might be multiple calls depending on MBEDTLS_TARGET_PREFIX
    target_include_directories(${MBEDTLS_TARGET_PREFIX}mbedcrypto
        PUBLIC
            # The following is required for psa/error.h
            $<BUILD_INTERFACE:${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/include>
    )
endif()

if(TARGET tfm_sprt)
    set(EXTERNAL_CRYPTO_CORE_HANDLED_TFM_SPRT True)
    target_compile_definitions(tfm_sprt
        PRIVATE
            INSIDE_TFM_BUILD
    )

    target_link_libraries(tfm_sprt
        PRIVATE
            psa_crypto_config
            psa_interface
    )
endif()

if(TARGET platform_s)
    target_compile_definitions(platform_s
        PRIVATE
            # Required for system_nrfxx_approtect.h.
            $<$<BOOL:${CONFIG_NRF_APPROTECT_LOCK}>:ENABLE_APPROTECT>
            $<$<BOOL:${CONFIG_NRF_APPROTECT_USER_HANDLING}>:ENABLE_APPROTECT_USER_HANDLING>
            $<$<BOOL:${CONFIG_NRF_APPROTECT_USER_HANDLING}>:ENABLE_AUTHENTICATED_APPROTECT> # nRF54L15/nRF54L10
            $<$<BOOL:${CONFIG_NRF_SECURE_APPROTECT_LOCK}>:ENABLE_SECURE_APPROTECT>
            $<$<BOOL:${CONFIG_NRF_SECURE_APPROTECT_LOCK}>:ENABLE_SECUREAPPROTECT> # nRF54L15/nRF54L10
            $<$<BOOL:${CONFIG_NRF_SECURE_APPROTECT_USER_HANDLING}>:ENABLE_SECURE_APPROTECT_USER_HANDLING>
            $<$<BOOL:${CONFIG_NRF_SECURE_APPROTECT_USER_HANDLING}>:ENABLE_AUTHENTICATED_SECUREAPPROTECT> # nRF54L15/nRF54L10
            $<$<BOOL:${CONFIG_NRF91_ANOMALY_36_WORKAROUND}>:NRF91_ERRATA_36_ENABLE_WORKAROUND> #nRF91
    )
endif()

if(NOT EXTERNAL_CRYPTO_CORE_HANDLED_NRF_SECURITY_TFM)
    set(EXTERNAL_CRYPTO_CORE_HANDLED_NRF_SECURITY_TFM True CACHE BOOL "" FORCE)
    if(NOT TF_PSA_CRYPTO_TARGET_PREFIX)
        set(TF_PSA_CRYPTO_TARGET_PREFIX crypto_service_)
    endif()
    add_subdirectory(${NRF_SECURITY_DIR}/tfm ${CMAKE_BINARY_DIR}/nrf_security_tfm)

    if(TARGET ${TF_PSA_CRYPTO_TARGET_PREFIX}tfpsacrypto)
        set(_tfm_crypto_target ${TF_PSA_CRYPTO_TARGET_PREFIX}tfpsacrypto)
        target_include_directories(${_tfm_crypto_target}
            PUBLIC
                ${CMAKE_SOURCE_DIR}/secure_fw/partitions/crypto
                ${CMAKE_SOURCE_DIR}/secure_fw/partitions/crypto/psa_driver_api
                $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/interface/include>
        )
        set_target_properties(${_tfm_crypto_target} PROPERTIES LINK_INTERFACE_MULTIPLICITY 3)
        target_link_libraries(${_tfm_crypto_target}
            PRIVATE
                platform_s
            PUBLIC
                crypto_service_tfpsacrypto_config
            INTERFACE
                platform_common_interface
        )
    endif()
endif()
