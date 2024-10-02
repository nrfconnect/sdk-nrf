#-------------------------------------------------------------------------------
# Copyright (c) 2024, Arm Limited. All rights reserved.
# Copyright (c) 2024, Nordic Semiconductor ASA.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

#=================== Build logic for external PSA core build ==================#

# Break out of this if PSA_CRYPTO_EXTERNAL_CORE is not set
if(NOT PSA_CRYPTO_EXTERNAL_CORE)
    return()
endif()

# Note that 

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
                    ${OBERON_PSA_CORE_PATH}/include
                    ${INTERFACE_INC_DIR}/crypto_keys
    )
endif()

# Duplicates that can be removed
#set(TFM_MBEDCRYPTO_CONFIG_PATH ${MBEDTLS_CFG_FILE})
#set(TFM_MBEDCRYPTO_PSA_CRYPTO_CONFIG_PATH ${MBEDTLS_PSA_CRYPTO_CONFIG_FILE})
#set(TFM_MBEDCRYPTO_PSA_CRYPTO_USER_CONFIG_PATH ${MBEDTLS_PSA_CRYPTO_USER_CONFIG_FILE})

# Note: This is a duplicate from nrf_security/CMakeLists.txt
#       with additions of the install-target for Oberon-psa-core includes
if(TARGET psa_interface)
    set(EXTERNAL_CRYPTO_CORE_HANDLED_PSA_INTERFACE True)
    target_include_directories(psa_interface
        INTERFACE
            $<BUILD_INTERFACE:${OBERON_PSA_CORE_PATH}/include>
            # Oberon library
            ${OBERON_PSA_CORE_PATH}/library
            # Mbed TLS (mbedcrypto) PSA headers
            ${ARM_MBEDTLS_PATH}/library
            ${ARM_MBEDTLS_PATH}/include
            ${ARM_MBEDTLS_PATH}/include/library 
    )
endif()

# Constructing config libraries in partition/crypto/CMakeLists.txt

# Configurations files for the users of the client interface
if(TARGET psa_crypto_config)
    set(EXTERNAL_CRYPTO_CORE_HANDLED_PSA_CRYPTO_CONFIG True)
    target_compile_definitions(psa_crypto_config
        INTERFACE
            MBEDTLS_CONFIG_FILE="${MBEDTLS_CONFIG_FILE}"
            MBEDTLS_PSA_CRYPTO_CONFIG_FILE="${MBEDTLS_PSA_CRYPTO_CONFIG_FILE}"
            # Give a signal that we are inside TF-M build to prevent check_config.h
            # complaining about lacking legacy features for Mbed TLS wrapper APIs, TLS/DTLS and X.509.
            INSIDE_TFM_BUILD
    )

    target_include_directories(psa_crypto_config
        INTERFACE
            ${PSA_CRYPTO_CONFIG_INTERFACE_PATH}
            ${NRF_SECURITY_ROOT}/include
            ${OBERON_PSA_CORE_PATH}/include
            ${NRF_DIR}/include/tfm
    )
endif()

# This defines the configuration files for the users of the library directly
if(TARGET psa_crypto_library_config)
    set(EXTERNAL_CRYPTO_CORE_HANDLED_PSA_CRYPTO_LIBRARY_CONFIG True)
    target_compile_definitions(psa_crypto_library_config
        INTERFACE
            MBEDTLS_CONFIG_FILE="${MBEDTLS_CONFIG_FILE}"
            MBEDTLS_PSA_CRYPTO_CONFIG_FILE="${MBEDTLS_PSA_CRYPTO_CONFIG_FILE}"
            MBEDTLS_PSA_CRYPTO_USER_CONFIG_FILE="${MBEDTLS_PSA_CRYPTO_USER_CONFIG_FILE}"
    )

    target_include_directories(psa_crypto_library_config
        INTERFACE
            ${PSA_CRYPTO_CONFIG_LIBRARY_PATH}
            ${NRF_SECURITY_ROOT}/include
            ${OBERON_PSA_CORE_PATH}/include
            ${NRF_DIR}/include/tfm
    )

    target_compile_definitions(psa_crypto_library_config
        INTERFACE
            MBEDTLS_PSA_CRYPTO_DRIVERS
            MBEDTLS_PSA_CRYPTO_BUILTIN_KEYS
            $<$<BOOL:${CRYPTO_TFM_BUILTIN_KEYS_DRIVER}>:PSA_CRYPTO_DRIVER_TFM_BUILTIN_KEY PSA_CRYPTO_DRIVER_TFM_BUILTIN_KEY_LOADER>
    )
endif()

if(TARGET tfm_psa_rot_partition_crypto)
    set(EXTERNAL_CRYPTO_CORE_HANDLED_TFM_PSA_ROT_PARTITION_CRYPTO True)
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
            $<BUILD_INTERFACE:${OBERON_PSA_CORE_PATH}/include>
    )
endif()

if(TARGET tfm_sprt)
    set(EXTERNAL_CRYPTO_CORE_HANDLED_TFM_SPRT True)
    target_compile_definitions(tfm_sprt
        PRIVATE
            MBEDTLS_CONFIG_FILE="${MBEDTLS_CONFIG_FILE}"
            MBEDTLS_PSA_CRYPTO_CONFIG_FILE="${MBEDTLS_PSA_CRYPTO_CONFIG_FILE}"
            INSIDE_TFM_BUILD
    )

    target_link_libraries(tfm_sprt
        PRIVATE
            psa_crypto_config
            psa_interface
    )
endif()
