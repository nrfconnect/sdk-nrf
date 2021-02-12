#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
zephyr_include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../include_override
)
zephyr_linker_sources(RODATA
    ${CMAKE_CURRENT_SOURCE_DIR}/custom-rodata.ld
)

if(CONFIG_CRYPTO_TEST_HASH)
    zephyr_sources_ifdef(CONFIG_MBEDTLS_SHA256_C        test_sha_256.c)
    zephyr_sources_ifdef(CONFIG_MBEDTLS_SHA512_C        test_sha_512.c)
    zephyr_sources_ifdef(CONFIG_MBEDTLS_SHA256_C        test_hmac.c)
    zephyr_sources_ifdef(CONFIG_MBEDTLS_SHA256_C        test_hkdf.c)
endif()
zephyr_sources_ifdef(CONFIG_MBEDTLS_ECDH_C              test_ecdh.c)
zephyr_sources_ifdef(CONFIG_MBEDTLS_ECDSA_C             test_ecdsa.c)
zephyr_sources_ifdef(CONFIG_MBEDTLS_AES_C               test_aes_ecb.c)
zephyr_sources_ifdef(CONFIG_MBEDTLS_AES_C               test_aead.c)
zephyr_sources_ifdef(CONFIG_MBEDTLS_ECJPAKE_C           test_ecjpake.c)
zephyr_sources_ifdef(CONFIG_MBEDTLS_CIPHER_MODE_CBC     test_aes_cbc.c)
zephyr_sources_ifdef(CONFIG_MBEDTLS_CIPHER_MODE_CBC     test_aes_cbc_mac.c)
zephyr_sources_ifdef(CONFIG_MBEDTLS_CIPHER_MODE_CTR     test_aes_ctr.c)

if(CONFIG_REDUCED_TEST_SUITE)
    # Quick reduced case: Run only a selection of test vectors,
    # found in the single file test_vectors.c
    zephyr_sources(                                     test_vectors.c)
else()
    zephyr_sources_ifdef(CONFIG_MBEDTLS_CHACHAPOLY_C    test_vectors_aead_chachapoly.c)
    zephyr_sources_ifdef(CONFIG_MBEDTLS_GCM_C           test_vectors_aead_gcm.c)
    zephyr_sources_ifdef(CONFIG_MBEDTLS_CIPHER_MODE_CBC test_vectors_aes_cbc_mac.c)
    zephyr_sources_ifdef(CONFIG_MBEDTLS_CIPHER_MODE_CBC test_vectors_aes_cbc.c)
    zephyr_sources_ifdef(CONFIG_MBEDTLS_CIPHER_MODE_CTR test_vectors_aes_ctr.c)
    zephyr_sources_ifdef(CONFIG_MBEDTLS_AES_C           test_vectors_aes_ecb.c)
    zephyr_sources_ifdef(CONFIG_MBEDTLS_ECDH_C          test_vectors_ecdh.c)
    zephyr_sources_ifdef(CONFIG_MBEDTLS_ECDSA_C         test_vectors_ecdsa_random.c)
    zephyr_sources_ifdef(CONFIG_MBEDTLS_ECDSA_C         test_vectors_ecdsa_sign.c)
    zephyr_sources_ifdef(CONFIG_MBEDTLS_ECDSA_C         test_vectors_ecdsa_verify.c)
    zephyr_sources_ifdef(CONFIG_MBEDTLS_ECJPAKE_C       test_vectors_ecjpake.c)
    if(CONFIG_CRYPTO_TEST_HASH)
        zephyr_sources_ifdef(CONFIG_MBEDTLS_SHA256_C    test_vectors_hkdf.c)
        zephyr_sources_ifdef(CONFIG_MBEDTLS_SHA256_C    test_vectors_hmac.c)
        zephyr_sources_ifdef(CONFIG_MBEDTLS_SHA256_C    test_vectors_sha_256.c)
        zephyr_sources_ifdef(CONFIG_MBEDTLS_SHA512_C    test_vectors_sha_512.c)
    endif()

    if(CONFIG_MBEDTLS_CMAC_C OR CONFIG_MBEDTLS_CMAC_ALT)
        zephyr_sources(                                 test_vectors_aes_ecb_mac.c)
        zephyr_sources_ifdef(CONFIG_MBEDTLS_AES_C       test_aes_ecb_mac.c)
    endif()

    # This is set differently based on backend
    if(CONFIG_CC310_MBEDTLS_CCM_C OR CONFIG_VANILLA_MBEDTLS_CCM_C OR CONFIG_MBEDTLS_CCM_C)
        zephyr_sources(                                 test_vectors_aead_ccm.c)
    endif()
endif()
