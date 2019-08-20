. = ALIGN(4);
__start_test_vector_aes_mac_data = .;
KEEP(*(test_vector_aes_mac_data))
__stop_test_vector_aes_mac_data = .;

. = ALIGN(4);
__start_test_vector_aes_func_data = .;
KEEP(*(test_vector_aes_func_data))
__stop_test_vector_aes_func_data = .;

. = ALIGN(4);
__start_test_vector_aes_monte_carlo_data = .;
KEEP(*(test_vector_aes_monte_carlo_data))
__stop_test_vector_aes_monte_carlo_data = .;

. = ALIGN(4);
__start_test_vector_aes_cbc_data = .;
KEEP(*(test_vector_aes_cbc_data))
__stop_test_vector_aes_cbc_data = .;

. = ALIGN(4);
__start_test_vector_aes_cbc_func_data = .;
KEEP(*(test_vector_aes_cbc_func_data))
__stop_test_vector_aes_cbc_func_data = .;

. = ALIGN(4);
__start_test_vector_aes_cbc_monte_carlo_data = .;
KEEP(*(test_vector_aes_cbc_monte_carlo_data))
__stop_test_vector_aes_cbc_monte_carlo_data = .;

. = ALIGN(4);
__start_test_vector_aes_cbc_mac_data = .;
KEEP(*(test_vector_aes_cbc_mac_data))
__stop_test_vector_aes_cbc_mac_data = .;

. = ALIGN(4);
__start_test_vector_aes_ecb_mac_data = .;
KEEP(*(test_vector_aes_ecb_mac_data))
__stop_test_vector_aes_ecb_mac_data = .;

. = ALIGN(4);
__start_test_vector_aes_ecb_data = .;
KEEP(*(test_vector_aes_ecb_data))
__stop_test_vector_aes_ecb_data = .;

. = ALIGN(4);
__start_test_vector_aes_ecb_func_data = .;
KEEP(*(test_vector_aes_ecb_func_data))
__stop_test_vector_aes_ecb_func_data = .;

. = ALIGN(4);
__start_test_vector_aes_ecb_monte_carlo_data = .;
KEEP(*(test_vector_aes_ecb_monte_carlo_data))
__stop_test_vector_aes_ecb_monte_carlo_data = .;

. = ALIGN(4);
__start_test_vector_aes_ctr_data = .;
KEEP(*(test_vector_aes_ctr_data))
__stop_test_vector_aes_ctr_data = .;

. = ALIGN(4);
__start_test_vector_aes_ctr_func_data = .;
KEEP(*(test_vector_aes_ctr_func_data))
__stop_test_vector_aes_ctr_func_data = .;

. = ALIGN(4);
__start_test_vector_ecdsa_random_data = .;
KEEP(*(test_vector_ecdsa_random_data))
__stop_test_vector_ecdsa_random_data = .;

. = ALIGN(4);
__start_test_vector_ecdsa_verify_data = .;
KEEP(*(test_vector_ecdsa_verify_data))
__stop_test_vector_ecdsa_verify_data = .;

. = ALIGN(4);
__start_test_vector_ecdsa_sign_data = .;
KEEP(*(test_vector_ecdsa_sign_data))
__stop_test_vector_ecdsa_sign_data = .;

. = ALIGN(4);
__start_test_vector_ecdh_data_random = .;
KEEP(*(test_vector_ecdh_data_random))
__stop_test_vector_ecdh_data_random = .;

. = ALIGN(4);
__start_test_vector_ecdh_data_deterministic_simple = .;
KEEP(*(test_vector_ecdh_data_deterministic_simple))
__stop_test_vector_ecdh_data_deterministic_simple = .;

. = ALIGN(4);
__start_test_vector_ecdh_data_deterministic_full = .;
KEEP(*(test_vector_ecdh_data_deterministic_full))
__stop_test_vector_ecdh_data_deterministic_full = .;

. = ALIGN(4);
__start_test_vector_aead_ccm_data = .;
KEEP(*(test_vector_aead_ccm_data))
__stop_test_vector_aead_ccm_data = .;

. = ALIGN(4);
__start_test_vector_aead_ccm_simple_data = .;
KEEP(*(test_vector_aead_ccm_simple_data))
__stop_test_vector_aead_ccm_simple_data = .;

. = ALIGN(4);
__start_test_vector_aead_gcm_data = .;
KEEP(*(test_vector_aead_gcm_data))
__stop_test_vector_aead_gcm_data = .;

. = ALIGN(4);
__start_test_vector_aead_gcm_simple_data = .;
KEEP(*(test_vector_aead_gcm_simple_data))
__stop_test_vector_aead_gcm_simple_data = .;

. = ALIGN(4);
__start_test_vector_aead_chachapoly_data = .;
KEEP(*(test_vector_aead_chachapoly_data))
__stop_test_vector_aead_chachapoly_data = .;

. = ALIGN(4);
__start_test_vector_aead_chachapoly_simple_data = .;
KEEP(*(test_vector_aead_chachapoly_simple_data))
__stop_test_vector_aead_chachapoly_simple_data = .;

. = ALIGN(4);
__start_test_vector_hkdf_data = .;
KEEP(*(test_vector_hkdf_data))
__stop_test_vector_hkdf_data = .;

. = ALIGN(4);
__start_test_vector_hmac_data = .;
KEEP(*(test_vector_hmac_data))
__stop_test_vector_hmac_data = .;

. = ALIGN(4);
__start_test_vector_hash_256_data = .;
KEEP(*(test_vector_hash_256_data))
__stop_test_vector_hash_256_data = .;

. = ALIGN(4);
__start_test_vector_hash_512_data = .;
KEEP(*(test_vector_hash_512_data))
__stop_test_vector_hash_512_data = .;

. = ALIGN(4);
__start_test_vector_hash_512_long_data = .;
KEEP(*(test_vector_hash_512_long_data))
__stop_test_vector_hash_512_long_data = .;

. = ALIGN(4);
__start_test_vector_hash_256_long_data = .;
KEEP(*(test_vector_hash_256_long_data))
__stop_test_vector_hash_256_long_data = .;

. = ALIGN(4);
__start_test_vector_ecjpake_given_data = .;
KEEP(*(test_vector_ecjpake_given_data))
__stop_test_vector_ecjpake_given_data = .;

. = ALIGN(4);
__start_test_vector_ecjpake_random_data = .;
KEEP(*(test_vector_ecjpake_random_data))
__stop_test_vector_ecjpake_random_data = .;

. = ALIGN(4);
__start_test_case_sha_256_data = .;
KEEP(*(test_case_sha_256_data))
__stop_test_case_sha_256_data = .;

. = ALIGN(4);
__start_test_case_sha_512_data = .;
KEEP(*(test_case_sha_512_data))
__stop_test_case_sha_512_data = .;

. = ALIGN(4);
__start_test_case_hmac_data = .;
KEEP(*(test_case_hmac_data))
__stop_test_case_hmac_data = .;

. = ALIGN(4);
__start_test_case_hkdf_data = .;
KEEP(*(test_case_hkdf_data))
__stop_test_case_hkdf_data = .;

. = ALIGN(4);
__start_test_case_ecdh_data = .;
KEEP(*(test_case_ecdh_data))
__stop_test_case_ecdh_data = .;

. = ALIGN(4);
__start_test_case_ecdsa_data = .;
KEEP(*(test_case_ecdsa_data))
__stop_test_case_ecdsa_data = .;

. = ALIGN(4);
__start_test_case_aes_ctr_data = .;
KEEP(*(test_case_aes_ctr_data))
__stop_test_case_aes_ctr_data = .;

. = ALIGN(4);
__start_test_case_aes_cbc_data = .;
KEEP(*(test_case_aes_cbc_data))
__stop_test_case_aes_cbc_data = .;

. = ALIGN(4);
__start_test_case_aes_cbc_mac_data = .;
KEEP(*(test_case_aes_cbc_mac_data))
__stop_test_case_aes_cbc_mac_data = .;

. = ALIGN(4);
__start_test_case_aes_ecb_mac_data = .;
KEEP(*(test_case_aes_ecb_mac_data))
__stop_test_case_aes_ecb_mac_data = .;

. = ALIGN(4);
__start_test_case_aes_ecb_data = .;
KEEP(*(test_case_aes_ecb_data))
__stop_test_case_aes_ecb_data = .;

. = ALIGN(4);
__start_test_case_aead_ccm_data = .;
KEEP(*(test_case_aead_ccm_data))
__stop_test_case_aead_ccm_data = .;

. = ALIGN(4);
__start_test_case_aead_gcm_data = .;
KEEP(*(test_case_aead_gcm_data))
__stop_test_case_aead_gcm_data = .;

. = ALIGN(4);
__start_test_case_aead_chachapoly_data = .;
KEEP(*(test_case_aead_chachapoly_data))
__stop_test_case_aead_chachapoly_data = .;

. = ALIGN(4);
__start_test_case_aead_ccm_simple_data = .;
KEEP(*(test_case_aead_ccm_simple_data))
__stop_test_case_aead_ccm_simple_data = .;

. = ALIGN(4);
__start_test_case_aead_gcm_simple_data = .;
KEEP(*(test_case_aead_gcm_simple_data))
__stop_test_case_aead_gcm_simple_data = .;

. = ALIGN(4);
__start_test_case_aead_chachapoly_simple_data = .;
KEEP(*(test_case_aead_chachapoly_simple_data))
__stop_test_case_aead_chachapoly_simple_data = .;

. = ALIGN(4);
__start_test_case_ecjpake_data = .;
KEEP(*(test_case_ecjpake_data))
__stop_test_case_ecjpake_data = .;
