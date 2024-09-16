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
include(${NRF_SECURITY_ROOT}/cmake/generate_configs.cmake)
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
    ${NRF_SECURITY_ROOT}/include
)

# Add include folders for psa_crypto_library_config (library build in S-only and inside TF-M)
target_include_directories(psa_crypto_library_config
  INTERFACE
    ${PSA_CRYPTO_CONFIG_LIBRARY_PATH}
    ${NRF_SECURITY_ROOT}/include
)

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

if(NOT CONFIG_MBEDTLS_PSA_CRYPTO_SPM AND NOT PSA_CRYPTO_EXTERNAL_CORE)
  append_with_prefix(src_crypto ${ARM_MBEDTLS_PATH}/library
    pem.c
    pkcs12.c
    pkcs5.c
    pkparse.c
    pkwrite.c
    dhm.c
    cipher.c
    cipher_wrap.c
    md.c
    pk.c
    pk_ecc.c
    pk_wrap.c
    pkwrite.c
    psa_util.c
  )
endif()

# Library for mbed TLS crypto toolbox functionality
add_library(${mbedcrypto_target}
  STATIC
    ${src_crypto}
)

target_link_libraries(${mbedcrypto_target}
  INTERFACE
    psa_crypto_config
    psa_interface
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

# Legacy APIs were missing files added by Oberon PSA core (not built in
# certain instances. This adds the same platform support as the Oberon PSA core)
if(NOT CONFIG_MBEDTLS_PSA_CRYPTO_C)
  append_with_prefix(src_crypto_base  ${OBERON_PSA_CORE_PATH}/library/
    platform.c
    platform_util.c
  )
endif()


# Add threading support for PSA core (if enabled)
include(${CMAKE_CURRENT_LIST_DIR}/threading/threading.cmake)
include(${NRF_SECURITY_ROOT}/src/utils/nrf_security_utils.cmake)

# Add base library with files required by all drivers/backends.
add_library(mbedcrypto_base
  STATIC
    ${src_crypto_base}
)

target_link_libraries(mbedcrypto_base
  PRIVATE
    psa_crypto_config
    $<TARGET_NAME_IF_EXISTS:platform_s>
  PUBLIC
    psa_interface
)

nrf_security_add_zephyr_options(mbedcrypto_base)

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

# Disable compile warnings for Mbed TLS sources for files that are 
# added to the build for legacy APIs but are not enabled due to
# configurations not being set. 
#
# These can be removed when NCSDK-28901 is addressed.
target_compile_options(${mbedcrypto_target}
  PRIVATE
    -Wno-unused-function
    -Wno-unused-variable
)

# Add PSA core
if(CONFIG_MBEDTLS_PSA_CRYPTO_C)
  add_subdirectory(core)
endif()

# Add drivers (for legacy and PSA crypto build)
add_subdirectory(drivers)

# Add legacy Mbed TLS APIs
if(CONFIG_MBEDTLS_LEGACY_CRYPTO_C)
  add_subdirectory(legacy)
endif()

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

  add_library(mbedx509 STATIC
    ${src_x509}
  )

  # Link with 
  target_link_libraries(mbedx509
    PRIVATE
      psa_crypto_config
      ${mbedcrypto_target}
      mbedcrypto_base
  )

  nrf_security_add_zephyr_options(mbedx509)

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

  add_library(mbedtls STATIC
    ${src_tls}
  )

  target_link_libraries(mbedtls
    PRIVATE
      psa_crypto_config
      mbedx509
      ${mbedcrypto_target}
      mbedcrypto_base
  )

  nrf_security_add_zephyr_options(mbedtls)
endif()

if (NOT CONFIG_MBEDTLS_PSA_CRYPTO_SPM)
  # Add zephyr-specific code
  add_subdirectory(zephyr)
endif()
