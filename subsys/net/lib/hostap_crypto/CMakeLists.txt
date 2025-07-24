#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# This is for use in NCS with nRF security which works only on nRF platforms
if(NOT DEFINED CONFIG_SOC_FAMILY_NORDIC_NRF)
  return()
endif()

zephyr_interface_library_named(hostap_crypto)

set(HOSTAP_BASE ${ZEPHYR_HOSTAP_MODULE_DIR})
set(WIFI_NM_WPA_SUPPLICANT_BASE ${HOSTAP_BASE}/wpa_supplicant)
set(HOSTAP_SRC_BASE ${HOSTAP_BASE}/src)
set(WIFI_NM_HOSTAPD_BASE ${HOSTAP_BASE}/hostapd)

set(CMAKE_EXE_LINKER_FLAGS "--specs=nosys.specs -lnosys")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DMISSING_SYSCALL_NAMES")

# For src/utils includes
target_link_libraries(hostap_crypto INTERFACE hostap)

if(DEFINED CONFIG_HOSTAP_CRYPTO_ALT_LEGACY_PSA)
  zephyr_include_directories(
    ${HOSTAP_BASE}/port/mbedtls
  )

  zephyr_library_sources(
    ${HOSTAP_SRC_BASE}/crypto/crypto_mbedtls_alt.c
    ${HOSTAP_SRC_BASE}/crypto/rc4.c
    ${HOSTAP_SRC_BASE}/crypto/aes-wrap.c
    ${HOSTAP_SRC_BASE}/crypto/aes-unwrap.c
    ${HOSTAP_SRC_BASE}/crypto/aes-internal-dec.c
    ${HOSTAP_SRC_BASE}/crypto/aes-internal.c
    ${HOSTAP_SRC_BASE}/crypto/aes-internal-enc.c
  )

  zephyr_library_sources_ifdef(CONFIG_HOSTAP_CRYPTO_ALT_PSA
    ${HOSTAP_BASE}/port/mbedtls/supp_psa_api.c
  )

  zephyr_library_sources_ifdef(CONFIG_HOSTAP_CRYPTO_ENTERPRISE
    ${HOSTAP_SRC_BASE}/crypto/des-internal.c
    ${HOSTAP_SRC_BASE}/crypto/ms_funcs.c
    ${HOSTAP_SRC_BASE}/crypto/aes-eax.c
    ${HOSTAP_SRC_BASE}/crypto/md4-internal.c
    ${HOSTAP_SRC_BASE}/crypto/sha1-internal.c
    ${HOSTAP_SRC_BASE}/crypto/fips_prf_internal.c
    ${HOSTAP_SRC_BASE}/crypto/milenage.c
    ${HOSTAP_SRC_BASE}/crypto/tls_mbedtls_alt.c
  )

  zephyr_library_sources_ifndef(CONFIG_HOSTAP_CRYPTO_ENTERPRISE
    ${HOSTAP_SRC_BASE}/crypto/tls_none.c
  )
endif()

if(DEFINED CONFIG_HOSTAP_CRYPTO_ALT_PSA)
  # Source code still uses the original symbol
  zephyr_compile_definitions(
    CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_MBEDTLS_PSA
  )

  zephyr_include_directories(
    ${HOSTAP_BASE}/port/mbedtls
  )

  zephyr_library_sources(
    ${HOSTAP_SRC_BASE}/crypto/aes-wrap.c
    ${HOSTAP_SRC_BASE}/crypto/aes-unwrap.c
    ${HOSTAP_SRC_BASE}/crypto/aes-internal-dec.c
    ${HOSTAP_SRC_BASE}/crypto/aes-internal.c
    ${HOSTAP_SRC_BASE}/crypto/aes-internal-enc.c
    ${HOSTAP_SRC_BASE}/crypto/rc4.c
    ${HOSTAP_SRC_BASE}/crypto/crypto_mbedtls_alt.c
    ${HOSTAP_SRC_BASE}/crypto/sha256-kdf.c
    ${HOSTAP_BASE}/port/mbedtls/supp_psa_api.c
    ${HOSTAP_SRC_BASE}/crypto/tls_none.c
  )

  if(CONFIG_HOSTAP_CRYPTO_WPA3_PSA)
    zephyr_library_sources(
      wpa3_psa.c
    )
    zephyr_library_compile_definitions(
      CONFIG_SAE
      CONFIG_ECC
    )
  endif()
endif()
