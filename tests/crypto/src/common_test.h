/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include "common.h"
#include <zephyr/toolchain.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/entropy.h>

#if defined(CONFIG_ZTEST)
#include <zephyr/ztest.h>
#endif

#include <mbedtls/platform.h>
#include <mbedtls/cipher.h>

/* Found in nrfxlib/nrf_security/mbedtls/mbedtls_heap.c
 * Used for reallocating the heap between suites.
 */
extern void _heap_init(void);
extern void _heap_free(void);

/* Points to either CTR or HMAC drbg random depending on what's compiled in */
extern int (*drbg_random)(void *, unsigned char *, size_t);

/**@brief Function for initializing deterministic random byte generator.
 *
 * @details Should only be called once.
 *
 * @param[in]		p_optional_seed		Pointer to a seed. Can be NULL. In that case uses internal seed.
 * @param[in]      	len					Length of seed in bytes. Ignored if seed is NULL.
 *
 * @return								0 on success, else an error code.
 */
int init_drbg(const unsigned char *p_optional_seed, size_t len);

/**@brief Wrapper function for hex2bin that makes sure that the input pointer is valid.
 *
 * @details Will return 0 if given param hex is NULL.
 *
 * @param hex     The hexadecimal string to convert
 * @param buf     Address of where to store the binary data
 * @param buflen  Size of the storage area for binary data
 *
 * @return     The length of the binary array, or 0 if an error occurred.
 */
size_t hex2bin_safe(const char *hex, uint8_t *buf, size_t buflen);

#if defined(MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG)
#include "psa/crypto.h"
#include "nrf_cc3xx_platform_ctr_drbg.h"
extern char drbg_ctx;

#else
#error "No RNG is enabled, MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG needs to be defined in nrf-config.h"

#endif

#if defined(CONFIG_ENTROPY_GENERATOR)
#include <zephyr/drivers/entropy.h>
#endif

#if defined CONFIG_TV_ASSERT_USER_OVERRIDE
#include <tv_assert_user_override.h>
#endif

/**@brief Test vector expected result.
 *  Used to verify invalid behavior test cases.
 */
typedef enum {
	EXPECTED_TO_PASS = 0, /**< Test vector is expected to pass. */
	EXPECTED_TO_FAIL = 1 /**< Test vector is expected to fail. */
} expected_results_t;

/**@brief Types of memory operations in sha test cases.
 *  Used to measure the time cost of memcpy operation between hash updates.
 */
typedef enum {
	NO_MODE = 0, /**< No memcpy operation of test vector. */
	DO_MEMCPY = 1 /**< Do a memcpy operation of test vector. */
} hash_mem_mode_t;

/**@brief General test suite information.
 */
typedef struct {
	uint32_t current_id;
	char *p_test_case_name;
	uint16_t tests_passed;
	uint16_t tests_failed;
} test_info_t;

/**@brief Test case setup function.
 */
typedef void (*test_setup_fn_t)(void);

/**@brief Test case execute function.
 *
 * @param[in] p_test_info    Pointer to global test info structure.
 */
typedef void (*test_exec_fn_t)(void);

/**@brief Test case teardown function.
 */
typedef void (*test_teardown_fn_t)(void);

/**@brief Empty passing teardown function.
 */
static inline void teardown_pass(void)
{
}

enum test_vector_t {
	TV_AES = 0,
	TV_AEAD,
	TV_ECDSA_VERIFY,
	TV_ECDSA_SIGN,
	TV_ECDSA_RANDOM,
	TV_ECDH,
	TV_HASH,
	TV_HMAC,
	TV_HKDF,
	TV_ECJPAKE,
};

/**@brief General test case information.
 */
typedef const struct {
	const char *p_test_case_name;
	test_setup_fn_t setup; /**< Setup function for test case. */
	test_exec_fn_t exec; /**< Test case function. */
	test_teardown_fn_t teardown; /**< Teardown function for test case. */
	enum test_vector_t vector_type; /**< Test vector differentiating type. */
	const void *vectors_start; /**< Base test vector pointer. */
	const void *vectors_stop; /**< End test vector pointer. */
} test_case_t;

extern test_case_t __start_test_case_sha_256_data[];
extern test_case_t __stop_test_case_sha_256_data[];

extern test_case_t __start_test_case_sha_512_data[];
extern test_case_t __stop_test_case_sha_512_data[];

extern test_case_t __start_test_case_hmac_data[];
extern test_case_t __stop_test_case_hmac_data[];

extern test_case_t __start_test_case_hkdf_data[];
extern test_case_t __stop_test_case_hkdf_data[];

extern test_case_t __start_test_case_ecdh_data[];
extern test_case_t __stop_test_case_ecdh_data[];

extern test_case_t __start_test_case_ecdsa_data[];
extern test_case_t __stop_test_case_ecdsa_data[];

extern test_case_t __start_test_case_aes_cbc_data[];
extern test_case_t __stop_test_case_aes_cbc_data[];

extern test_case_t __start_test_case_aes_cbc_mac_data[];
extern test_case_t __stop_test_case_aes_cbc_mac_data[];

extern test_case_t __start_test_case_aes_ecb_mac_data[];
extern test_case_t __stop_test_case_aes_ecb_mac_data[];

extern test_case_t __start_test_case_aes_ecb_data[];
extern test_case_t __stop_test_case_aes_ecb_data[];

extern test_case_t __start_test_case_aes_ctr_data[];
extern test_case_t __stop_test_case_aes_ctr_data[];

extern test_case_t __start_test_case_aead_ccm_data[];
extern test_case_t __stop_test_case_aead_ccm_data[];

extern test_case_t __start_test_case_aead_ccm_simple_data[];
extern test_case_t __stop_test_case_aead_ccm_simple_data[];

extern test_case_t __start_test_case_aead_gcm_data[];
extern test_case_t __stop_test_case_aead_gcm_data[];

extern test_case_t __start_test_case_aead_gcm_simple_data[];
extern test_case_t __stop_test_case_aead_gcm_simple_data[];

extern test_case_t __start_test_case_aead_chachapoly_data[];
extern test_case_t __stop_test_case_aead_chachapoly_data[];

extern test_case_t __start_test_case_aead_chachapoly_simple_data[];
extern test_case_t __stop_test_case_aead_chachapoly_simple_data[];

extern test_case_t __start_test_case_ecjpake_data[];
extern test_case_t __stop_test_case_ecjpake_data[];

/**@brief ECJPAKE test vector information.
 */
typedef const struct {
	const int expected_err_code; /**< Expected error code of operation. */
	const uint8_t expected_result; /**< Expected result of operation. */
	const char
		*p_test_vector_name; /**< Pointer to ECJPAKE test vector name in
									  hex string format. */
	const char *p_password; /**< Pointer to ECJPAKE password. */
	const char
		*p_priv_key_client_1; /**< Pointer to ECJPAKE client private key
									  point component 1. */
	const char
		*p_priv_key_client_2; /**< Pointer to ECJPAKE client private key
									  point component 2. */
	const char
		*p_priv_key_server_1; /**< Pointer to ECJPAKE server private key
									  point component 1. */
	const char
		*p_priv_key_server_2; /**< Pointer to ECJPAKE server private key
									  point component 2. */
	const char *
		p_round_message_client_1; /**< Pointer to ECJPAKE round message. */
	const char *
		p_round_message_client_2; /**< Pointer to ECJPAKE round message. */
	const char *
		p_round_message_server_1; /**< Pointer to ECJPAKE round message. */
	const char *
		p_round_message_server_2; /**< Pointer to ECJPAKE round message. */
	const char *
		p_expected_shared_secret; /**< Pointer to ECJPAKE expected shared
										   secret. */
} test_vector_ecjpake_t;

/**@brief AES test vector information.
 */
typedef const struct {
	const mbedtls_cipher_mode_t mode; /**< Mode, e.g. CBC, ECB, GCM, etc. */
	const mbedtls_cipher_padding_t
		padding; /**< Padding, e.g. ZEROS, PKCS7, etc. */
	const int expected_err_code; /**< Expected error code from AES operation. */
	const uint8_t expected_result; /**< Expected result of AES operation. */
	const mbedtls_operation_t direction; /**< Encrypt or decrypt. */
	const char
		*p_test_vector_name; /**< Pointer to AES test vector name in hex
									 string format. */
	const char *
		p_plaintext; /**< Pointer to AES plaintext in hex string format. */
	const char *
		p_ciphertext; /**< Pointer to AES ciphertext in hex string format. */
	const char *p_key; /**< Pointer to AES key in hex string format. */
	const char *
		p_iv; /**< Pointer to AES initialization vector in hex string format. */
	const char *
		p_ad; /**< Pointer to AES additional Data in hex string format. */
} test_vector_aes_t;

/**@brief AEAD test vector information.
 */
typedef const struct {
	const mbedtls_cipher_mode_t
		mode; /**< Cipher mode, e.g. CCM. See mbedtls/cipher.h. */
	const mbedtls_cipher_id_t
		id; /**< Cipher mode, e.g. CCM. See mbedtls/cipher.h. */
	const bool
		ccm_star; /**< There is no mode for CCM STAR. Thus CCM && this bool
						  == CCM STAR. */
	const int expected_err_code; /**< Expected error code from AEAD operation. */
	const uint8_t
		crypt_expected_result; /**< Expected result of AEAD crypt operation. */
	const uint8_t
		mac_expected_result; /**< Expected result of AEAD MAC operation. */
	const mbedtls_operation_t direction; /**< Encrypt or decrypt. */
	const char *p_test_vector_name; /**< Pointer to AEAD test vector name. */
	const char *
		p_plaintext; /**< Pointer to AEAD plaintext in hex string format. */
	const char *
		p_ciphertext; /**< Pointer to AEAD ciphertext in hex string format. */
	const char *p_key; /**< Pointer to AEAD key in hex string format. */
	const char *
		p_ad; /**< Pointer to AEAD additional Data in hex string format. */
	const char *p_nonce; /**< Pointer to AEAD nonce in hex string format. */
	const char *p_mac; /**< Pointer to AEAD message Authentication Code in hex
						  string format. */
} test_vector_aead_t;

/**@brief ECDSA Verify test vector information.
 */
typedef const struct {
	const uint32_t src_line_num; /**< Test vector source file line number. */
	const uint32_t curve_type; /**< Curve type for test vector. */
	const int expected_err_code; /**< Expected error code from ECDSA verify
									 operation. */
	const char *p_test_vector_name; /**< Pointer to ECDSA test vector name. */
	const char *
		p_input; /**< Pointer to ECDSA hash input in hex string format. */
	const char *
		p_qx; /**< Pointer to ECDSA public Key X component in hex string
					   format. */
	const char *
		p_qy; /**< Pointer to ECDSA public Key Y component in hex string
					   format. */
	const char *
		p_r; /**< Pointer to ECDSA signature R component in hex string format. */
	const char *
		p_s; /**< Pointer to ECDSA signature S component in hex string format. */
} test_vector_ecdsa_verify_t;

/**@brief ECDSA Sign test vector information.
 */
typedef const struct {
	const uint32_t src_line_num; /**< Test vector source file line number. */
	const uint32_t curve_type; /**< Curve type for test vector. */
	const int expected_sign_err_code; /**< Expected error code from ECDSA sign
									   operation. */
	const int expected_verify_err_code; /**< Expected result of following ECDSA
										 verify operation. */
	const char *p_test_vector_name; /**< Pointer to ECDSA test vector name. */
	const char *
		p_input; /**< Pointer to ECDSA hash input in hex string format. */
	const char *
		p_qx; /**< Pointer to ECDSA public key X component in hex string
					   format. */
	const char *
		p_qy; /**< Pointer to ECDSA public key Y component in hex string
					   format. */
	const char *
		p_x; /**< Pointer to ECDSA private key component in hex string format. */
} test_vector_ecdsa_sign_t;

/**@brief ECDSA Random test vector information.
 */
typedef const struct {
	const uint32_t src_line_num; /**< Test vector source file line number. */
	const uint32_t curve_type; /**< Curve type for test vector. */
	const char *p_test_vector_name; /**< Pointer to ECDSA test vector name. */
	const char *
		p_input; /**< Pointer to ECDSA hash input in hex string format. */
	const size_t sig_len; /**< Length of the signature. */
} test_vector_ecdsa_random_t;

/**@brief ECDH test vector information.
 */
typedef const struct {
	const uint32_t curve_type; /**< Curve type for test vector. */
	const int expected_err_code; /**< Expected error code from ECDH operation. */
	const uint8_t expected_result; /**< Expected result of ECDH operation. */
	const char *p_test_vector_name; /**< Pointer to ECDH test vector name. */
	const char
		*p_initiator_priv; /**< Pointer to ECDH initiator private key in
									 hex string format. */
	const char
		*p_responder_priv; /**< Pointer to ECDH responder private key in
									 hex string format. */
	const char
		*p_initiator_publ_x; /**< Pointer to ECDH initiator public key X
									 component in hex string format. */
	const char
		*p_initiator_publ_y; /**< Pointer to ECDH initiator public key Y
									 component in hex string format. */
	const char
		*p_responder_publ_x; /**< Pointer to ECDH responder public key X
									 component in hex string format. */
	const char
		*p_responder_publ_y; /**< Pointer to ECDH responder public key Y
									 component in hex string format. */
	const char *p_expected_shared_secret; /**< Pointer to ECDH expected Shared
										   Secret in hex string format. */
} test_vector_ecdh_t;

/**@brief Hash test vector information.
 */
typedef const struct {
	const uint32_t digest_type; /**< Digest type of hash operation. */
	const int expected_err_code; /**< Expected error code from hash operation. */
	const uint8_t expected_result; /**< Expected result of hash operation. */
	const hash_mem_mode_t mode; /**< Hash memory operation. */
	const uint32_t
		chunk_length; /**< Size of input chunks to hash function in bytes. */
	const uint32_t
		update_iterations; /**< Number of update iterations of input. */
	const char *p_test_vector_name; /**< Pointer to hash test vector name. */
	const char
		*p_input; /**< Pointer to input message in hex string format. */
	const char *
		p_expected_output; /**< Pointer to expected message digest in hex
									string format. */
} test_vector_hash_t;

/**@brief hmac test vector information.
 */
typedef const struct {
	const uint32_t digest_type; /**< Digest type of hmac operation. */
	const int expected_err_code; /**< Expected error code from hmac operation. */
	const uint8_t expected_result; /**< Expected result of hmac operation. */
	const char *p_test_vector_name; /**< Pointer to hmac test vector name. */
	const char
		*p_input; /**< Pointer to input message in hex string format. */
	const char *p_key; /**< Pointer to hmac key in hex string format. */
	const char *p_expected_output; /**< Pointer to expected hmac digest in hex
									string format. */
} test_vector_hmac_t;

/**@brief hkdf test vector information.
 */
typedef const struct {
	const uint32_t digest_type; /**< Digest type of hkdf operation. */
	const int expected_err_code; /**< Expected error code from hkdf operation. */
	const int expected_err_code_expand; /**< Expected error code from hkdf expand
										 operation. */
	const uint8_t expected_result; /**< Expected result of hkdf operation. */
	const uint8_t
		expected_result_expand; /**< Expected result of hkdf expand operation. */
	const char *p_test_vector_name; /**< Pointer to hkdf test vector name. */
	const char *
		p_ikm; /**< Pointer to hkdf Input Key Material in hex string format. */
	const char *
		p_okm; /**< Pointer to hkdf Output Key Material in hex string format. */
	const char *
		p_prk; /**< Pointer to hkdf PseudoRandom Key in hex string format. */
	const char *p_salt; /**< Pointer to hkdf salt in hex string format. */
	const char *p_info; /**< Pointer to hkdf optional application specific
						 information in hex string format. */
} test_vector_hkdf_t;

const char *get_vector_name(const test_case_t *tc, uint32_t v);
uint32_t get_vector_count(const test_case_t *tc);

/* Placeholder for time measurements.
 * Is called from various tests, but implementation is currently a no-op.
 */
void start_time_measurement(void);
void stop_time_measurement(void);

/**@brief Macro(s) for decorating test vector names with file and line information.
 */
#ifndef TV_NAME
#define TV_NAME(name) name " -- [" __FILE__ ":" STRINGIFY(__LINE__) "]"
#endif

/**@brief   Macro for obtaining the address of the beginning of a section.
 *
 * param[in]    section_name    Name of the section.
 * @hideinitializer
 */
#ifndef SECTION_START_ADDR
#define SECTION_START_ADDR(section_name) (&UTIL_CAT(__start_, section_name))
#endif

/**@brief    Macro for obtaining the address of the end of a section.
 *
 * @param[in]   section_name    Name of the section.
 * @hideinitializer
 */
#ifndef SECTION_END_ADDR
#define SECTION_END_ADDR(section_name) (&UTIL_CAT(__stop_, section_name))
#endif

/**@brief   Macro for retrieving the length of a given section, in bytes.
 *
 * @param[in]   section_name    Name of the section.
 * @hideinitializer
 */
#ifndef SECTION_LENGTH
#define SECTION_LENGTH(section_name)                                           \
	((size_t)SECTION_END_ADDR(section_name) -                              \
	 (size_t)SECTION_START_ADDR(section_name))
#endif

/**@brief   Macro for declaring a variable and registering it in a section.
 *
 * @details Declares a variable and registers it in a named section. This macro ensures that the
 *          variable is not stripped away when using optimizations.
 *
 * @note The order in which variables are placed in a section is dependent on the order in
 *       which the linker script encounters the variables during linking.
 *
 * @param[in]   section_name    Name of the section.
 * @param[in]   section_var     Variable to register in the given section.
 * @hideinitializer
 */
#ifndef ITEM_REGISTER
#define ITEM_REGISTER(section_name, section_var)                               \
	Z_GENERIC_SECTION(section_name) __attribute__((used)) section_var
#endif

/**@brief   Macro for retrieving a variable from a section.
 *
 * @note     The stored symbol can only be resolved using this macro if the
 *           type of the data is word aligned. The operation of acquiring
 *           the stored symbol relies on the size of the stored type. No
 *           padding can exist in the named section in between individual
 *           stored items or this macro will fail.
 *
 * @param[in]   section_name    Name of the section.
 * @param[in]   data_type       Data type of the variable.
 * @param[in]   i               Index of the variable in section.
 * @hideinitializer
 */
#ifndef ITEM_GET
#define ITEM_GET(section_name, data_type, i)                                   \
	((data_type *)SECTION_START_ADDR(section_name) + (i))
#endif

/**@brief   Macro for getting the number of variables in a section.
 *
 * @param[in]   section_name    Name of the section.
 * @param[in]   data_type       Data type of the variables in the section.
 * @hideinitializer
 */
#ifndef ITEM_COUNT
#define ITEM_COUNT(section_name, data_type)                                    \
	(SECTION_LENGTH(section_name) / sizeof(data_type))
#endif

/**@brief Macro for comparing two data buffers.
 *
 * @details Equal to a memcmp, except that it returns a 1 if memory areas are different.
 *
 * @param[in] x         First Memory pointer.
 * @param[in] y         Second Memory pointer.
 * @param[in] size      Number of bytes to compare.
 *
 * @retval    1         If buffers does not match.
 * @retval    0         If buffers match.
 */
#ifndef TEST_MEMCMP
#define TEST_MEMCMP(x, y, size) ((memcmp(x, y, size) == 0) ? 0 : 1)
#endif

/**@brief Macro asserting equality.
 */
#ifndef TEST_VECTOR_ASSERT_EQUAL
#define TEST_VECTOR_ASSERT_EQUAL(expected, actual)                             \
	zassert_equal((expected), (actual),                                    \
		      "\tAssert values: 0x%04X != -0x%04X", (expected),        \
		      (-actual))
#endif

/**@brief Macro asserting inequality.
 */
#ifndef TEST_VECTOR_ASSERT_NOT_EQUAL
#define TEST_VECTOR_ASSERT_NOT_EQUAL(expected, actual)                         \
	zassert_not_equal((expected), (actual),                                \
			  "\tAssert values: 0x%04X == -0x%04X", (expected),    \
			  (-actual))
#endif

/**@brief Macro for checking buffer overflow for a given buffer. Requires that the two following
 *  bytes after the buffer are set to 0xFF.
 *
 * @details Requires a label statement "exit_test_vector" to jump to if condition is false.
 *
 * @param[in] p_buffer   Pointer to buffer to check for overflow.
 * @param[in] length     Length of buffer (Not including overflow bytes).
 * @param[in] tc_info    Additional information to log if condition is false.
 */
#ifndef TEST_VECTOR_OVERFLOW_ASSERT
#define TEST_VECTOR_OVERFLOW_ASSERT(p_buffer, length, tc_info)                 \
	do {                                                                   \
		TEST_VECTOR_ASSERT_EQUAL(0xFF, p_buffer[length]);              \
		TEST_VECTOR_ASSERT_EQUAL(0xFF, p_buffer[length + 1]);          \
	} while (0)
#endif

/**@brief Macro for verifying a memcmp inside a test vector test, and logging the result.
 *
 * @details Requires a label statement "exit_test_vector" to jump to if condition is false.
 *
 * @param[in] buf1              First buffer.
 * @param[in] buf2              Second buffer.
 * @param[in] len               Length to compare in bytes.
 * @param[in] required_result   Required memcmp result to pass (EXPECTED_TO_PASS, EXPECTED_TO_FAIL).
 * @param[in] tc_info           Additional information to log if condition is false.
 */
#ifndef TEST_VECTOR_MEMCMP_ASSERT
#define TEST_VECTOR_MEMCMP_ASSERT(buf1, buf2, len, expected_result, tc_info)   \
	TEST_VECTOR_ASSERT_EQUAL(expected_result, TEST_MEMCMP(buf1, buf2, len))
#endif
