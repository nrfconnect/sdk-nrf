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
# Same layout as Zephyr MBEDTLS_REMOVED_MODULES_PATH (modules/mbedtls/legacy_support.cmake);
# TF-PSA / Oberon drop DES from the core tree; hostap still needs mbedtls/des.h and des.c for
# crypto_mbedtls_alt.c (e.g. des_encrypt) even without Enterprise.
set(MBEDTLS_REMOVED_MODULES_PATH "${HOSTAP_BASE}/port/mbedtls/removed")

set(CMAKE_EXE_LINKER_FLAGS "--specs=nosys.specs -lnosys")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DMISSING_SYSCALL_NAMES")

# For src/utils includes
target_link_libraries(hostap_crypto INTERFACE hostap)

if(DEFINED CONFIG_HOSTAP_CRYPTO_ALT_PSA)
  # Source code still uses the original symbol
  zephyr_compile_definitions(
    CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_MBEDTLS_PSA
  )

  zephyr_include_directories(
    ${MBEDTLS_REMOVED_MODULES_PATH}
    ${HOSTAP_BASE}/port/mbedtls
  )

  zephyr_library_sources(
    ${MBEDTLS_REMOVED_MODULES_PATH}/des.c
    ${MBEDTLS_REMOVED_MODULES_PATH}/dhm.c
    ${HOSTAP_SRC_BASE}/crypto/aes-wrap.c
    ${HOSTAP_SRC_BASE}/crypto/aes-unwrap.c
    ${HOSTAP_SRC_BASE}/crypto/aes-internal-dec.c
    ${HOSTAP_SRC_BASE}/crypto/aes-internal.c
    ${HOSTAP_SRC_BASE}/crypto/aes-internal-enc.c
    ${HOSTAP_SRC_BASE}/crypto/rc4.c
    ${HOSTAP_SRC_BASE}/crypto/crypto_mbedtls_alt.c
    ${HOSTAP_BASE}/port/mbedtls/supp_psa_api.c
  )

  # Enterprise EAP needs the mbedtls software TLS stack and extra helpers.
  zephyr_library_sources_ifdef(CONFIG_HOSTAP_CRYPTO_ENTERPRISE
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

  zephyr_library_sources_ifndef(CONFIG_PSA_WANT_ALG_HKDF
    ${HOSTAP_SRC_BASE}/crypto/sha256-kdf.c
    ${HOSTAP_SRC_BASE}/crypto/sha384-kdf.c
    ${HOSTAP_SRC_BASE}/crypto/sha512-kdf.c
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

  zephyr_library_compile_definitions(
    MBEDTLS_DES_C
    MBEDTLS_DHM_C
  )
  zephyr_include_directories(${ZEPHYR_MBEDTLS_MODULE_DIR}/include)
endif()
