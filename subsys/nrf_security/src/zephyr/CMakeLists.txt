#
# Copyright (c) 2021-2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Additional build logic for Zephyr builds

# Add mbedcrypto common includes and definitions to zephyr build
target_link_libraries(zephyr_interface INTERFACE mbedcrypto_common)

if (CONFIG_MBEDTLS_DEBUG)
  # Add Zephyr's mbedlts module include path because we need rely on their headers for debug logs.
  target_include_directories(mbedcrypto_common
      INTERFACE
        ${ZEPHYR_BASE}/modules/mbedtls/include
  )
  zephyr_library_sources(${ZEPHYR_BASE}/modules/mbedtls/debug.c)
  zephyr_library_sources(debug_init.c)
endif()

# Add mbed TLS heap library - Not for TF-M build
if(CONFIG_MBEDTLS_ENABLE_HEAP)
  list(APPEND src_zephyr
    mbedtls_heap.c
  )
endif()

# Add entropy_poll only if NRF_CC3XX_PLATFORM is not added
# This file is not useful for TF-M build where entropy should come
# from SPE image, using psa_generate_random
if(CONFIG_MBEDTLS_ENTROPY_POLL)
  list(APPEND src_zephyr
    ${NRF_SECURITY_ROOT}/src/zephyr/entropy_poll.c
  )
endif()

if(CONFIG_BUILD_WITH_TFM)
  # For some reason $<TARGET_PROPERTY:tfm,TFM_BINARY_DIR> does not work here
  set(TFM_BINARY_DIR ${CMAKE_BINARY_DIR}/tfm)

  # Add the TF-M interface to zephyr includes
  zephyr_include_directories(
    ${TFM_BINARY_DIR}/install/interface/include
  )
endif()

# Add the zephyr-specific sources necessary for nrf_security
# This library contains files that require knowledge of zephyr
if(DEFINED src_zephyr)
  zephyr_library_named(mbedtls_zephyr)

  zephyr_library_sources(${src_zephyr})

  zephyr_library_include_directories(
    $<TARGET_PROPERTY:${mbedcrypto_target},INTERFACE_INCLUDE_DIRECTORIES>
  )

  zephyr_library_compile_definitions(
    $<TARGET_PROPERTY:${mbedcrypto_target},INTERFACE_COMPILE_DEFINITIONS>
  )

  # We need the k_mbedtls_partition when userspace is
  # used and nrf_security is enabled. However when CONFIG_MBEDTLS
  # is enabled the partition is defined by Zephyr and therefore
  # we cannot redefine it here.
  # (see zephyr/doc/kernel/usermode/memory_domain.rst)
  if((NOT CONFIG_MBEDTLS) AND CONFIG_USERSPACE)
    zephyr_library_sources(mbedtls_partition.c)
  endif()

  zephyr_library_app_memory(k_mbedtls_partition)

endif()

# Add configuration/options from zephyr interface libraries
nrf_security_add_zephyr_options(${mbedcrypto_target})
nrf_security_add_zephyr_options(mbedcrypto_base)

# Add the generic mbedtls_external library to interface with zephyr
add_library(mbedtls_external INTERFACE)

# Link to the NSPE versions of the libraries providing TLS/DTLS and x509 APIs
if(TARGET mbedtls)
  target_link_libraries(mbedtls_external INTERFACE mbedtls)
endif()

if(TARGET mbedx509)
  target_link_libraries(mbedtls_external INTERFACE mbedx509)
endif()

# Link with mbedcrypto library (in NS build or by default)
target_link_libraries(mbedtls_external INTERFACE ${mbedcrypto_target})

zephyr_link_libraries(mbedtls_external)
