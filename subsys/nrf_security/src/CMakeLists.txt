#
# Copyright (c) 2021 - 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

#
# File including instructions to build PSA crypto APIs.
#
# This is executed both in NCS (NS, secure only) build of Mbed TLS as
# well as TF-M build
#
# The Mbed TLS configuration MBEDTLS_PSA_CRYPTO_SPM used to check if this is
# used inside a TF-M build.

# The interface is only generated in the NS image if TF-M is used
include(${NRF_SECURITY_DIR}/cmake/generate_configs.cmake)
generate_mbedcrypto_interface_configs()
generate_mbedcrypto_library_configs()

# This configuration is used for regex matching of the PSA crypto config interface path
# This regex is used by nrf_security_add_zephyr_options when adding includes from
# the zephyr_interface to nrf_security libraries
string(REPLACE "/" "\\/" PSA_CRYPTO_CONFIG_INTERFACE_PATH_REGEX ${PSA_CRYPTO_CONFIG_INTERFACE_PATH})

# Add include folders for psa_crypto_config (interface builds)
target_include_directories(psa_crypto_config
  INTERFACE
    ${PSA_CRYPTO_CONFIG_INTERFACE_PATH}
    ${NRF_SECURITY_DIR}/include
)

# Add fallback include folders from Mbed TLS for driver context structures
# This is only necessary when building with legacy code which will include <psa/crypto.h>
if(NOT (CMAKE_BUILD_WITH_TFM OR CONFIG_MBEDTLS_PSA_CRYPTO_C))
target_include_directories(psa_crypto_config
  INTERFACE
    ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/include
)
endif()

# Add a meta-target of the PSA config interface to avoid using generator expressions
add_library(psa_crypto_config_chosen INTERFACE)

if(CONFIG_BUILD_WITH_TFM OR CONFIG_PSA_SSF_CRYPTO_CLIENT)
  target_link_libraries(psa_crypto_config_chosen INTERFACE psa_crypto_config)
else()
  target_link_libraries(psa_crypto_config_chosen INTERFACE psa_crypto_library_config)
endif()


# Add include folders for psa_crypto_library_config (library build in S-only and inside TF-M)
target_include_directories(psa_crypto_library_config
  INTERFACE
    ${PSA_CRYPTO_CONFIG_LIBRARY_PATH}
    ${NRF_SECURITY_DIR}/include
    # This is needed for the oberon_check_unsupported.h
    ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/oberon/drivers
)

add_library(${tfpsacrypto_target}
  STATIC
)

add_subdirectory(tf-psa-crypto)

target_link_libraries(${tfpsacrypto_target}
  PUBLIC
    psa_crypto_config_chosen
    psa_interface
  PRIVATE
    $<TARGET_NAME_IF_EXISTS:platform_s>
    nrf_security_utils
)

if(CONFIG_PSA_SSF_CRYPTO_CLIENT)
  add_subdirectory(ssf_secdom)
endif()

nrf_security_add_zephyr_options(${tfpsacrypto_target})

# Add threading support for PSA core (if enabled)
include(${CMAKE_CURRENT_LIST_DIR}/threading/threading.cmake)
include(${NRF_SECURITY_DIR}/src/utils/nrf_security_utils.cmake)

# Misusing the psa_crypto_config and psa_crypto_library_config awareness to
# set compiler-flags to ensure right floating-point types are set
if(COMPILER_CP_FLAG)
  target_compile_options(psa_crypto_config
    INTERFACE
      ${COMPILER_CP_FLAG}
  )

  target_compile_options(psa_crypto_library_config
    INTERFACE
      ${COMPILER_CP_FLAG}
  )
endif()

# Misusing the psa_crypto_config and psa_crypto_library_config awareness to
# set linker-options to ensure right floating-point types are set
if(LINKER_CP_OPTION)
  target_link_options(psa_crypto_config
    INTERFACE
      ${LINKER_CP_OPTION}
  )

  target_link_options(psa_crypto_library_config
  INTERFACE
    ${LINKER_CP_OPTION}
)
endif()

# Add PSA core
if(CONFIG_MBEDTLS_PSA_CRYPTO_C)
  add_subdirectory(core)
endif()

# Add drivers (for legacy and PSA crypto build)
add_subdirectory(drivers)

# NEWLIB requires linking with the C library
if(CONFIG_NEWLIB_LIBC)
  target_link_libraries(${tfpsacrypto_target} PRIVATE -lc)
endif()

if(NOT CONFIG_MBEDTLS_PSA_CRYPTO_SPM)
  # Add zephyr-specific code
  add_subdirectory(zephyr)
endif()
