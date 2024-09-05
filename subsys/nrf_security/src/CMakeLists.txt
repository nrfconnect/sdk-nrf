#
# Copyright (c) 2021 - 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

#
# File including instructions to build PSA APIs.
#
# Can be executed by both the NCS and TF-M build systems.
#
# The variable BUILD_INSIDE_TFM can be used to distinguish between being
# within the TF-M build system or the NCS build system.
#
# For TF-M the nrf_security/tfm/CMakeLists.txt file will execute this file. BUILD_INSIDE_TFM=True
# For NCS  the nrf_security/CMakeLists.txt     file will execute this file. BUILD_INSIDE_TFM=False
#


# Library for mbedcrypto includes and configurations
add_library(mbedcrypto_common INTERFACE)

# Add the config-file entry point
target_compile_definitions(mbedcrypto_common
  INTERFACE
    -DMBEDTLS_CONFIG_FILE="${CONFIG_MBEDTLS_CFG_FILE}"
    -DMBEDTLS_PSA_CRYPTO_CONFIG_FILE="${CONFIG_MBEDTLS_PSA_CRYPTO_CONFIG_FILE}"
    -DMBEDTLS_PSA_CRYPTO_USER_CONFIG_FILE="${CONFIG_MBEDTLS_PSA_CRYPTO_USER_CONFIG_FILE}"
    -DMBEDTLS_PSA_CRYPTO_CONFIG
)

# Add the user config only if it is set
if(NOT ${CONFIG_MBEDTLS_USER_CONFIG_FILE} STREQUAL "nrf-config-user-empty.h" )
  target_compile_definitions(mbedcrypto_common
    INTERFACE
      -DMBEDTLS_USER_CONFIG_FILE="${CONFIG_MBEDTLS_USER_CONFIG_FILE}"
  )
endif()

if(BUILD_INSIDE_TFM)
  # In order to support FPU in TF-M the following is documented by the TF-M
  # build system:
  #
  #   Secure and non-secure libraries are compiled with COMPILER_CP_FLAG and
  #   linked with LINKER_CP_OPTION for different FP ABI types. All those
  #   libraries shall be built with COMPILER_CP_FLAG
  target_compile_options(mbedcrypto_common
    INTERFACE
      ${COMPILER_CP_FLAG}
  )

  target_link_options(mbedcrypto_common
    INTERFACE
      ${LINKER_CP_OPTION}
  )
endif()

set(generated_include_path ${CMAKE_CURRENT_BINARY_DIR}/include/generated)

# Empty out previous versions of config-files
file(REMOVE_RECURSE ${generated_include_path})

# Generate PSA WANT configurations
include(${NRF_SECURITY_ROOT}/cmake/psa_crypto_want_config.cmake)

# Generate mbed TLS configurations
if(CONFIG_MBEDTLS_LEGACY_CRYPTO_C OR NOT COMPILE_PSA_APIS)
  include(${NRF_SECURITY_ROOT}/cmake/legacy_crypto_config.cmake)
else()
  include(${NRF_SECURITY_ROOT}/cmake/psa_crypto_config.cmake)
endif()

if(CONFIG_GENERATE_MBEDTLS_CFG_FILE)
  # Add the generated CONFIG_MBEDTLS_CFG_FILE file to the include path
  target_include_directories(mbedcrypto_common
    INTERFACE
    ${generated_include_path} # From cmake/*_crypto_config.cmake
)
endif()

if(CONFIG_MBEDTLS_ENABLE_HEAP)
  if (CONFIG_BUILD_WITH_TFM)
    # Add replacement for memory_buffer_alloc.c for NS build
    list(APPEND src_crypto ${NRF_SECURITY_ROOT}/src/legacy/memory_buffer_alloc.c)
  else()
    # Both secure only builds and TF-M builds use the implementation from MbedTLS.
    # This currently doesn't do a mutex on heap access.
    list(APPEND src_crypto ${ARM_MBEDTLS_PATH}/library/memory_buffer_alloc.c)
  endif()
endif()

if (COMPILE_PSA_APIS)
  # Add the local version of psa_crypto_driver_wrappers.c
  list(APPEND src_crypto
    psa_crypto_driver_wrappers.c
  )
endif()

append_with_prefix(src_crypto ${ARM_MBEDTLS_PATH}/library
  pem.c
  pkcs12.c
  pkcs5.c
  pkparse.c
  pkwrite.c
)

# Library for mbed TLS crypto toolbox functionality
add_library(${mbedcrypto_target}
  STATIC
    ${src_crypto}
)

if(CONFIG_PSA_SSF_CRYPTO_CLIENT)
  add_subdirectory(ssf_secdom)
endif()

nrf_security_add_zephyr_options(${mbedcrypto_target})

# Base mbed TLS files (not in drivers or builtin's)
append_with_prefix(src_crypto_base ${ARM_MBEDTLS_PATH}/library
  asn1parse.c
  asn1write.c
  base64.c
  bignum.c
  bignum_core.c
  nist_kw.c
  oid.c
  padlock.c
  version.c
  constant_time.c
)

# Add base library with files required by all drivers/backends.
add_library(mbedcrypto_base
  STATIC
    ${src_crypto_base}
)

target_link_libraries(mbedcrypto_base
  PRIVATE
    mbedcrypto_common
)

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

target_link_libraries(${mbedcrypto_target}
  PRIVATE
    mbedcrypto_base
)

# Add regular mbedcrypto common includes and configurations
target_link_libraries(${mbedcrypto_target}
  PUBLIC
    mbedcrypto_common
)

target_compile_options(${mbedcrypto_target} PRIVATE
  # Don't be too strict with warnings on mbedtls code. This code is not
  # ours, so it's not necessary to do too much static analysis on
  # it. And any warnings that do pop up are difficult to fix as they
  # require changes far upstream.
  -Wno-unused-function
  -Wno-unused-variable
  # Disable warnings showing up in Oberon PSA crypto code.
  -Wno-stringop-overflow
  -Wno-stringop-overread
  -Wno-strict-aliasing
  )

add_subdirectory(core)

# Add drivers if enabled
add_subdirectory(drivers)

# Add legacy mbed TLS APIs
add_subdirectory(legacy)

# Print all files in the mbedcrypto_target (with optional legacy-additions)
nrf_security_debug_list_target_files(${mbedcrypto_target})

# In TF-M build, a psa_interface lib is created and we must inherit its settings
set_property(TARGET ${mbedcrypto_target}
  APPEND PROPERTY
    PRIVATE_LINK_LIBRARIES
      $<TARGET_NAME_IF_EXISTS:psa_interface>
)

# NEWLIB requires linking with the C library
if (CONFIG_NEWLIB_LIBC)
  target_link_libraries(${mbedcrypto_target} PRIVATE -lc)
  target_link_libraries(mbedcrypto_base PRIVATE -lc)
endif()

if (CONFIG_MBEDTLS_X509_LIBRARY)
  append_with_prefix(src_x509 ${ARM_MBEDTLS_PATH}/library
    x509.c
    x509_create.c
    x509_crl.c
    x509_crt.c
    x509_csr.c
    x509write.c
    x509write_crt.c
    x509write_csr.c
  )

  add_library(${mbedx509_target} STATIC
    ${src_x509}
  )

  # Add options from Zephyr build
  nrf_security_add_zephyr_options(${mbedx509_target})

  target_link_libraries(${mbedx509_target}
    PRIVATE
      ${mbedcrypto_target}
      mbedcrypto_base
    PUBLIC
      mbedcrypto_common
  )
endif()

# Library for TLS protocol support
if (CONFIG_MBEDTLS_TLS_LIBRARY)
  # TLS files
  append_with_prefix(src_tls ${ARM_MBEDTLS_PATH}/library
    debug.c
    net_sockets.c
    ssl_cache.c
    ssl_ciphersuites.c
    ssl_client.c
    ssl_cookie.c
    ssl_msg.c
    ssl_ticket.c
    ssl_tls.c
    ssl_tls12_client.c
    ssl_tls12_server.c
    ssl_tls13_keys.c
    ssl_tls13_server.c
    ssl_tls13_client.c
    ssl_tls13_generic.c
  )

  if (CONFIG_MBEDTLS_DEBUG)
	list(APPEND src_tls
	  ${ARM_MBEDTLS_PATH}/library/ssl_debug_helpers_generated.c
	)
  endif()

  add_library(${mbedtls_target} STATIC
    ${src_tls}
  )

  # Add options from Zephyr build
  nrf_security_add_zephyr_options(${mbedtls_target})

  target_link_libraries(${mbedtls_target}
    PRIVATE
      ${mbedx509_target}
      ${mbedcrypto_target}
      mbedcrypto_base
    PUBLIC
      mbedcrypto_common
  )
endif()

if (NOT BUILD_INSIDE_TFM)
  # Add zephyr-specific code
  add_subdirectory(zephyr)
endif()
