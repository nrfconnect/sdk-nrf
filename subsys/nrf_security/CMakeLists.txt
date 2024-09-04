#
# Copyright (c) 2019-2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# This file is nrf_security's entry-point for the NCS build system
set(BUILD_INSIDE_TFM False)


# Include generic macros
include(cmake/extensions.cmake)

set(mbedcrypto_target mbedcrypto)
set(mbedx509_target   mbedx509)
set(mbedtls_target    mbedtls)

# Generate the file containing all static paths and configurations used both in
# TF-M and non TF-M builds
configure_file(${CMAKE_CURRENT_LIST_DIR}/configs/config_extra.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/cmake/config_extra.cmake
)

# Include files with standard paths like oberon-psa-core and nrf_security root
include(${CMAKE_CURRENT_BINARY_DIR}/cmake/config_extra.cmake)

if(CONFIG_SSF_SERVER_PSA_CRYPTO_SERVICE_ENABLED)
# Inside the SSF server, when the PSA crypto service is enabled we need to
# use the OWNER id to give key ownership to the differrent domains.
set(CONFIG_MBEDTLS_PSA_CRYPTO_KEY_ID_ENCODES_OWNER     True)
else()
# MBEDTLS_PSA_CRYPTO_KEY_ID_ENCODES_OWNER must be disabled for Zephyr
# builds or when MBEDTLS_USE_PSA_CRYPTO is enabled (e.g. for TLS/DTLS
# and x.509 support) Note: This configuration is internal and may be
# removed with a new mbed TLS version
set(CONFIG_MBEDTLS_PSA_CRYPTO_KEY_ID_ENCODES_OWNER     False)
endif()

if(CONFIG_BUILD_WITH_TFM)
  # Execute Cmake logic to forward configurations to TF-M build
  include(${NRF_SECURITY_ROOT}/cmake/config_to_tf-m.cmake)
endif()

if(CONFIG_BUILD_WITH_TFM OR CONFIG_PSA_SSF_CRYPTO_CLIENT)
  # We enable either TF-M or the SSF client PSA crypto interface but we are
  # not in the secure image build

  # NS-build: PSA APIs are already compiled in the secure image and is
  # exposed as a service
  set(COMPILE_PSA_APIS                          False)

  # Add replacement platform.c for NS build
  list(APPEND src_zephyr
    ${ARM_MBEDTLS_PATH}/library/platform.c
  )

  # The current version of the mbed TLS deliverables requires mbedcrypto built
  # and linked in the NS image (e.g. for mbedtls and mbedx509 library).
  # If CC3XX_BACKEND is enabled, configurations need to be converted to
  # OBERON_BACKEND (enabled by default).

  get_cmake_property(all_vars VARIABLES)

  # 1. Non-secure should not build the PSA core or drivers
  set(CONFIG_MBEDTLS_PSA_CRYPTO_C               False)

  # 2. Enable OBERON_BACKEND, disable CC3XX_BACKEND
  set(CONFIG_NRF_OBERON                         True)
  set(CONFIG_OBERON_BACKEND                     True)
  set(CONFIG_CC3XX_BACKEND                      False)
  set(CONFIG_CC310_BACKEND                      False)
  set(CONFIG_CC312_BACKEND                      False)
  set(CONFIG_NRF_CC3XX_PLATFORM                 False)
  set(CONFIG_PSA_CRYPTO_DRIVER_CC3XX            False)

  # 3. Special case: _ALT in CC3XX, not in OBERON (set  to False)
  set(CONFIG_MBEDTLS_AES_ALT                    False)
  set(CONFIG_MBEDTLS_CCM_ALT                    False)
  set(CONFIG_MBEDTLS_CHACHAPOLY_ALT             False)
  set(CONFIG_MBEDTLS_CMAC_ALT                   False)
  set(CONFIG_MBEDTLS_ECP_ALT                    False)
  set(CONFIG_MBEDTLS_GCM_ALT                    False)
  set(CONFIG_MBEDTLS_DHM_ALT                    False)
  set(CONFIG_MBEDTLS_RSA_ALT                    False)

  # 4. Special case: _ALT in ECJPAKE (only in OBERON, set to True)
  #    Only has effect if ECJPAKE is enabled
  set(CONFIG_MBEDTLS_ECJPAKE_ALT                True)

  # 5. Special case: Handle platform specific configurations
  set(CONFIG_MBEDTLS_PLATFORM_EXIT_ALT                 False)
  set(CONFIG_MBEDTLS_PLATFORM_SETUP_TEARDOWN_ALT       False)

  # 6. Threading is not used for TF-M NS build
  # Force this to false to disable it.
  set(CONFIG_MBEDTLS_THREADING_ALT                     False)
  set(CONFIG_MBEDTLS_THREADING_C                       False)
else()
  nrf_security_debug("Building for pure Zephyr")

  if(CONFIG_MBEDTLS_PSA_CRYPTO_C)
    # Build PSA APIs if requested
    set(COMPILE_PSA_APIS                        True)
  endif()
endif()

set(CONFIG_MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG  True)

# Finally adding the crypto lib
add_subdirectory(${NRFXLIB_DIR}/crypto crypto_copy)

# Add mbed TLS Libraries
add_subdirectory(src)
