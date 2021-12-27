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

zephyr_sources(test_sha_256.c)
zephyr_sources(test_sha_512.c)
zephyr_sources(test_hmac.c)
zephyr_sources(test_hkdf.c)
zephyr_sources(test_ecdh.c)
zephyr_sources(test_ecdsa.c)
zephyr_sources(test_aes_ecb.c)
zephyr_sources(test_aead.c)
zephyr_sources(test_ecjpake.c)
zephyr_sources(test_aes_cbc.c)
zephyr_sources(test_aes_cbc_mac.c)
zephyr_sources(test_aes_ecb_mac.c)
zephyr_sources(test_aes_ctr.c)

if(CONFIG_REDUCED_TEST_SUITE)
    # Quick reduced case: Run only a selection of test vectors,
    # found in the single file test_vectors.c
    zephyr_sources(                                     test_vectors.c)
else()
    zephyr_sources(test_vectors_aead_chachapoly.c)
    zephyr_sources(test_vectors_aead_gcm.c)
    zephyr_sources(test_vectors_aes_cbc_mac.c)
    zephyr_sources(test_vectors_aes_cbc.c)
    zephyr_sources(test_vectors_aes_ctr.c)
    zephyr_sources(test_vectors_aes_ecb.c)
    zephyr_sources(test_vectors_ecdh.c)
    zephyr_sources(test_vectors_ecdsa_random.c)
    zephyr_sources(test_vectors_ecdsa_sign.c)
    zephyr_sources(test_vectors_ecdsa_verify.c)
    zephyr_sources(test_vectors_ecjpake.c)
    zephyr_sources(test_vectors_hkdf.c)
    zephyr_sources(test_vectors_hmac.c)
    zephyr_sources(test_vectors_sha_256.c)
    zephyr_sources(test_vectors_sha_512.c)
    zephyr_sources(test_vectors_aes_ecb_mac.c)
    zephyr_sources(test_vectors_aead_ccm.c)
endif()
