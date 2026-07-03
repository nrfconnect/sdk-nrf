#
# Copyright (c) 2019-2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# This file is nrf_security's entry-point for the NCS build system
# Set SPM to false as we are not in a secure image build
set(CONFIG_MBEDTLS_PSA_CRYPTO_SPM False)

# Include generic macros
include(cmake/extensions.cmake)

set(tfpsacrypto_target mbedcrypto)

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
  # builds or e.g. TLS/DTLS and x.509 support)
  # Note: This configuration is internal and may be removed with a new mbed TLS version
  set(CONFIG_MBEDTLS_PSA_CRYPTO_KEY_ID_ENCODES_OWNER     False)
endif()

if(CONFIG_BUILD_WITH_TFM)
  # Execute Cmake logic to forward configurations to TF-M build
  include(${NRF_SECURITY_DIR}/cmake/config_to_tf-m.cmake)
endif()

if(CONFIG_BUILD_WITH_TFM OR CONFIG_PSA_SSF_CRYPTO_CLIENT)
  # We enable either TF-M or the SSF client PSA crypto interface but we are
  # not in the secure image build

  # Add replacement platform.c for NS build
  list(APPEND src_zephyr
    ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/platform/platform.c
  )

  # The current version of the mbed TLS deliverables requires mbedcrypto built
  # and linked in the NS image (e.g. for mbedtls and mbedx509 library).

  get_cmake_property(all_vars VARIABLES)

  set(CONFIG_NRF_OBERON                         True)
  set(CONFIG_NRF_CC3XX_PLATFORM                 False)
  set(CONFIG_PSA_CRYPTO_DRIVER_CC3XX            False)

else()
  nrf_security_debug("Building for pure Zephyr")
endif()

# Add library for crypto configs (NS/S-only build)
# The name and intent of this comes from TF-M distribution
add_library(psa_crypto_config INTERFACE)

# Add config files required for PSA crypto interface
target_compile_definitions(psa_crypto_config
  INTERFACE
    -DTF_PSA_CRYPTO_CONFIG_FILE="${CONFIG_TF_PSA_CRYPTO_CONFIG_FILE}"
)

# Add library for crypto configs (S-only or Secure image build)
# The name and intent of this comes from TF-M distribution
add_library(psa_crypto_library_config INTERFACE)

# Add config files required for PSA core
target_compile_definitions(psa_crypto_library_config
  INTERFACE
    -DTF_PSA_CRYPTO_CONFIG_FILE="${CONFIG_TF_PSA_CRYPTO_CONFIG_FILE}"
    -DTF_PSA_CRYPTO_USER_CONFIG_FILE="${CONFIG_TF_PSA_CRYPTO_USER_CONFIG_FILE}"
)

# Add a library for crypto includes for the PSA interface (NS, S-only and TF-M)
# The name and intent of this comes from TF-M distribution
add_library(psa_interface INTERFACE)

include(cmake/psa_interface_include_directories.cmake)

# Finally adding the crypto lib
add_subdirectory(${ZEPHYR_NRFXLIB_MODULE_DIR}/crypto crypto_copy)

# Add mbed TLS Libraries
add_subdirectory(src)
