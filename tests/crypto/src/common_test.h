/*$$$LICENCE_NORDIC_STANDARD<2018>$$$*/

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <toolchain/common.h>
#include <ztest.h>

#define TEST_CASE_COUNT     ITEM_COUNT(test_case_data, test_case_t)     /**< Get number of different test cases. */
#define TEST_CASE_GET(i)    ITEM_GET(test_case_data, test_case_t, (i))  /**< Get test case reference from array of test cases. */


/**@brief Test vector expected result.
 *  Used to verify invalid behavior test cases.
 */
typedef enum
{
    EXPECTED_TO_PASS    = 0,    /**< Test vector is expected to pass. */
    EXPECTED_TO_FAIL    = 1     /**< Test vector is expected to fail. */
} expected_results_t;


/**@brief Types of memory operations in sha test cases.
 *  Used to measure the time cost of memcpy operation between hash updates.
 */
typedef enum
{
    NO_MODE     = 0,            /**< No memcpy operation of test vector. */
    DO_MEMCPY   = 1             /**< Do a memcpy operation of test vector. */
} hash_mem_mode_t;


/**@brief General test suite information.
 */
typedef struct
{
    uint32_t    current_id;
    char      * p_test_case_name;
    uint16_t    tests_passed;
    uint16_t    tests_failed;
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


/**@brief General test case information.
 */
typedef const struct
{
    char                              * p_test_case_name;           /**< Pointer to test case name. */
    test_setup_fn_t                     setup;                      /**< Setup function for test case. */
    test_exec_fn_t                      exec;                       /**< Test case function. */
    test_teardown_fn_t                  teardown;                   /**< Teardown function for test case. */
} test_case_t;

extern test_case_t __start_test_case_data[];
extern test_case_t __stop_test_case_data[];

#if 0
/**@brief AES test vector information.
 */
typedef const struct
{
    const nrf_crypto_aes_info_t       * p_aes_info;                 /**< Pointer to AES info type. */
    const ret_code_t                    expected_err_code;          /**< Expected error code from AES operation. */
    const uint8_t                       expected_result;            /**< Expected result of AES operation. */
    const nrf_crypto_operation_t        direction;                  /**< Encrypt or decrypt. */
    const char                        * p_test_vector_name;         /**< Pointer to AES test vector name in hex string format. */
    const char                        * p_plaintext;                /**< Pointer to AES plaintext in hex string format. */
    const char                        * p_ciphertext;               /**< Pointer to AES ciphertext in hex string format. */
    const char                        * p_key;                      /**< Pointer to AES key in hex string format. */
    const char                        * p_iv;                       /**< Pointer to AES initialization vector in hex string format. */
    const char                        * p_ad;                       /**< Pointer to AES additional Data in hex string format. */
}test_vector_aes_t;


/**@brief AEAD test vector information.
 */
typedef const struct
{
    const nrf_crypto_aead_info_t      * p_aead_info;                /**< Pointer to AEAD info type. */
    const ret_code_t                    expected_err_code;          /**< Expected error code from AEAD operation. */
    const uint8_t                       crypt_expected_result;      /**< Expected result of AEAD crypt operation. */
    const uint8_t                       mac_expected_result;        /**< Expected result of AEAD MAC operation. */
    const nrf_crypto_operation_t        direction;                  /**< Encrypt or decrypt. */
    const char                        * p_test_vector_name;         /**< Pointer to AEAD test vector name. */
    const char                        * p_plaintext;                /**< Pointer to AEAD plaintext in hex string format. */
    const char                        * p_ciphertext;               /**< Pointer to AEAD ciphertext in hex string format. */
    const char                        * p_key;                      /**< Pointer to AEAD key in hex string format. */
    const char                        * p_iv;                       /**< Pointer to AEAD initialization vector in hex string format. */
    const char                        * p_ad;                       /**< Pointer to AEAD additional Data in hex string format. */
    const char                        * p_tag;                      /**< Pointer to AEAD tag in hex string format. */
    const char                        * p_nonce;                    /**< Pointer to AEAD nonce in hex string format. */
    const char                        * p_mac;                      /**< Pointer to AEAD message Authentication Code in hex string format. */
}test_vector_aead_t;


/**@brief ECDSA Verify test vector information.
 */
typedef const struct
{
    const nrf_crypto_ecc_curve_info_t * p_curve_info;               /**< Pointer to ECDSA curve info type. */
    const ret_code_t                    expected_err_code;          /**< Expected error code from ECDSA verify operation. */
    const char                        * p_test_vector_name;         /**< Pointer to ECDSA test vector name. */
    const char                        * p_input;                    /**< Pointer to ECDSA hash input in hex string format. */
    const char                        * p_qx;                       /**< Pointer to ECDSA public Key X component in hex string format. */
    const char                        * p_qy;                       /**< Pointer to ECDSA public Key Y component in hex string format. */
    const char                        * p_r;                        /**< Pointer to ECDSA signature R component in hex string format. */
    const char                        * p_s;                        /**< Pointer to ECDSA signature S component in hex string format. */
}test_vector_ecdsa_verify_t;


/**@brief ECDSA Sign test vector information.
 */
typedef const struct
{
    const nrf_crypto_ecc_curve_info_t * p_curve_info;               /**< Pointer to ECDSA curve info type. */
    const ret_code_t                    expected_sign_err_code;     /**< Expected error code from ECDSA sign operation. */
    const ret_code_t                    expected_verify_err_code;   /**< Expected result of following ECDSA verify operation. */
    const char                        * p_test_vector_name;         /**< Pointer to ECDSA test vector name. */
    const char                        * p_input;                    /**< Pointer to ECDSA hash input in hex string format. */
    const char                        * p_qx;                       /**< Pointer to ECDSA public key X component in hex string format. */
    const char                        * p_qy;                       /**< Pointer to ECDSA public key Y component in hex string format. */
    const char                        * p_x;                        /**< Pointer to ECDSA private key component in hex string format. */
}test_vector_ecdsa_sign_t;

/**@brief ECDSA Random test vector information.
 */
typedef const struct
{
    const nrf_crypto_ecc_curve_info_t * p_curve_info;               /**< Pointer to ECDSA curve info type. */
    const char                        * p_test_vector_name;         /**< Pointer to ECDSA test vector name. */
    const char                        * p_input;                    /**< Pointer to ECDSA hash input in hex string format. */
    const size_t                        sig_len;                    /**< Length of the signature. */
}test_vector_ecdsa_random_t;


/**@brief ECDH test vector information.
 */
typedef const struct
{
    const nrf_crypto_ecc_curve_info_t * p_curve_info;               /**< Pointer to ECDH curve info type. */
    const ret_code_t                    expected_err_code;          /**< Expected error code from ECDH operation. */
    const uint8_t                       expected_result;            /**< Expected result of ECDH operation. */
    const char                        * p_test_vector_name;         /**< Pointer to ECDH test vector name. */
    const char                        * p_initiater_priv;           /**< Pointer to ECDH initiater private key in hex string format. */
    const char                        * p_responder_priv;           /**< Pointer to ECDH responder private key in hex string format. */
    const char                        * p_initiater_publ_x;         /**< Pointer to ECDH initiater public key X component in hex string format. */
    const char                        * p_initiater_publ_y;         /**< Pointer to ECDH initiater public key Y component in hex string format. */
    const char                        * p_responder_publ_x;         /**< Pointer to ECDH responder public key X component in hex string format. */
    const char                        * p_responder_publ_y;         /**< Pointer to ECDH responder public key Y component in hex string format. */
    const char                        * p_expected_shared_secret;   /**< Pointer to ECDH expected Shared Secret in hex string format. */
}test_vector_ecdh_t;
#endif

/**@brief Hash test vector information.
 */
typedef const struct
{
    const int                         expected_err_code;            /**< Expected error code from hash operation. */
    const uint8_t                     expected_result;              /**< Expected result of hash operation. */
    const hash_mem_mode_t             mode;                         /**< Hash memory operation. */
    const uint32_t                    chunk_length;                 /**< Size of input chunks to hash function in bytes. */
    const uint32_t                    update_iterations;            /**< Number of update iterations of input. */
    const char                      * p_test_vector_name;           /**< Pointer to hash test vector name. */
    const char                      * p_input;                      /**< Pointer to input message in hex string format. */
    const char                      * p_expected_output;            /**< Pointer to expected message digest in hex string format. */
} test_vector_hash_t;


#if 0
/**@brief hmac test vector information.
 */
typedef const struct
{
    const nrf_crypto_hmac_info_t    * p_hmac_info;                  /**< Pointer to hmac info type. */
    const ret_code_t                  expected_err_code;            /**< Expected error code from hmac operation. */
    const uint8_t                     expected_result;              /**< Expected result of hmac operation. */
    const char                      * p_test_vector_name;           /**< Pointer to hmac test vector name. */
    const char                      * p_input;                      /**< Pointer to input message in hex string format. */
    const char                      * p_key;                        /**< Pointer to hmac key in hex string format. */
    const char                      * p_expected_output;            /**< Pointer to expected hmac digest in hex string format. */
}test_vector_hmac_t;


/**@brief hkdf test vector information.
 */
typedef const struct
{
    const nrf_crypto_hmac_info_t    * p_hmac_info;                  /**< Pointer to hmac info type. */
    const ret_code_t                  expected_err_code;            /**< Expected error code from hkdf operation. */
    const uint8_t                     expected_result;              /**< Expected result of hkdf operation. */
    const char                      * p_test_vector_name;           /**< Pointer to hkdf test vector name. */
    const char                      * p_ikm;                        /**< Pointer to hkdf Input Key Material in hex string format. */
    const char                      * p_okm;                        /**< Pointer to hkdf Output Key Material in hex string format. */
    const char                      * p_prk;                        /**< Pointer to hkdf PseudoRandom Key in hex string format. */
    const char                      * p_salt;                       /**< Pointer to hkdf salt in hex string format. */
    const char                      * p_info;                       /**< Pointer to hkdf optional application specific information in hex string format. */
}test_vector_hkdf_t;
#endif

/**@brief Function for converting hex string to bytes.
 *
 * @param[in,out]  p_output     Pointer to the output buffer where the bytes will be stored.
 * @param[in]      p_input      Pointer to the input hexstring.
 *
 * @return                    The number of bytes that was read and converted.
 */
uint32_t unhexify(uint8_t * p_output, char const * p_input);


/**@brief Function for setting LED_1 to indicate a start of time measurement.
 *
 */
void start_time_measurement(void);


/**@brief Function for clearing LED_1 to indicate a stop of time measurement.
 *
 */
void stop_time_measurement(void);

/**
 * @brief Macro for concatenating two tokens in macro expansion.
 *
 * @note This macro is expanded in two steps so that tokens given as macros
 *       themselves are fully expanded before they are merged.
 *
 * @param[in] p1 First token.
 * @param[in] p2 Second token.
 *
 * @return The two tokens merged into one, unless they cannot together form
 *         a valid token (in such case, the preprocessor issues a warning and
 *         does not perform the concatenation).
 *
 * @sa NRFX_CONCAT_3
 */
#define CONCAT_2(p1, p2)       CONCAT_2_(p1, p2)


/** @brief Internal macro used by @ref CONCAT_2 to perform the expansion in two steps. */
#define CONCAT_2_(p1, p2)      p1 ## p2


/**@brief   Macro for obtaining the address of the beginning of a section.
 *
 * param[in]    section_name    Name of the section.
 * @hideinitializer
 */
#define SECTION_START_ADDR(section_name)       &CONCAT_2(__start_, section_name)


/**@brief    Macro for obtaining the address of the end of a section.
 *
 * @param[in]   section_name    Name of the section.
 * @hideinitializer
 */
#define SECTION_END_ADDR(section_name)         &CONCAT_2(__stop_, section_name)


/**@brief   Macro for retrieving the length of a given section, in bytes.
 *
 * @param[in]   section_name    Name of the section.
 * @hideinitializer
 */
#define SECTION_LENGTH(section_name)                        \
    ((size_t)SECTION_END_ADDR(section_name) -               \
     (size_t)SECTION_START_ADDR(section_name))


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
#define ITEM_REGISTER(section_name, section_var) \
    Z_GENERIC_SECTION(section_name) __attribute__((used)) section_var


/**@brief   Macro for retrieving a variable from a section.
 *
 * @warning     The stored symbol can only be resolved using this macro if the
 *              type of the data is word aligned. The operation of acquiring
 *              the stored symbol relies on the size of the stored type. No
 *              padding can exist in the named section in between individual
 *              stored items or this macro will fail.
 *
 * @param[in]   section_name    Name of the section.
 * @param[in]   data_type       Data type of the variable.
 * @param[in]   i               Index of the variable in section.
 * @hideinitializer
 */
#define ITEM_GET(section_name, data_type, i) \
    ((data_type*)SECTION_START_ADDR(section_name) + (i))


/**@brief   Macro for getting the number of variables in a section.
 *
 * @param[in]   section_name    Name of the section.
 * @param[in]   data_type       Data type of the variables in the section.
 * @hideinitializer
 */
#define ITEM_COUNT(section_name, data_type) \
    SECTION_LENGTH(section_name) / sizeof(data_type)

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
#define TEST_MEMCMP(x, y, size) ( (memcmp(x, y, size) == 0) ? 0 : 1 )


/**@brief Macro for checking return code of test case, and logging the result.
 *
 * @param[in] err_code  Error code from the test case.
 * @param[in] tc_name   Name of test case to log.
 * @param[in] tc_state  Current test state (setup, exec, teardown).
 */
#define TEST_CASE_ASSERT(err_code, tc_name, tc_state)                                                           \
    do                                                                                                          \
    {                                                                                                           \
        const uint32_t local_err_code = (err_code);                                                             \
        if (local_err_code != NRF_SUCCESS)                                                                      \
        {                                                                                                       \
            NRF_LOG_INFO("%s - %s - failed err: 0x%x", tc_name, tc_state, err_code);                            \
        }                                                                                                       \
    } while (0)


/**@brief Macro for verifying a condition inside a test vector test, and logging the result.
 *
 * @details Requires a label statement "exit_test_vector" to jump to if condition is false.
 *
 * @param[in] condition     The condition to assert.
 * @param[in] tc_info       Additional information to log if condition is false.
 */

#define TEST_VECTOR_ASSERT(condition, tc_info) zassert(condition, "passed",      \
    "#%04d Test vector failed: %s %s, %s", test_info.current_id,            \
    test_info.p_test_case_name, p_test_vector->p_test_vector_name, tc_info)


/**@brief Macro for checking buffer overflow for a given buffer. Requires that the two following
 *  bytes after the buffer are set to 0xFF.
 *
 * @details Requires a label statement "exit_test_vector" to jump to if condition is false.
 *
 * @param[in] p_buffer   Pointer to buffer to check for overflow.
 * @param[in] length     Length of buffer (Not including overflow bytes).
 * @param[in] tc_info    Additional information to log if condition is false.
 */
#define TEST_VECTOR_OVERFLOW_ASSERT(p_buffer, length, tc_info) \
    TEST_VECTOR_ASSERT(((p_buffer[length] != 0xFF) || (p_buffer[length + 1] != 0xFF)), tc_info)


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
#define TEST_VECTOR_MEMCMP_ASSERT(buf1, buf2, len, expected_result, tc_info) \
    TEST_VECTOR_ASSERT((TEST_MEMCMP(buf1, buf2, len) == expected_result) , tc_info)
    


/**@brief Macro for verifying a nrf_crypto return value, and logging the result.
 *
 * @details Requires a label statement "exit_test_vector" to jump to if condition is false.
 *
 * @param[in] condition     The condition to assert.
 * @param[in] tc_info       Additional information to log if condition is false.
 */
#define TEST_VECTOR_ASSERT_ERR_CODE(condition, tc_info) \
    TEST_VECTOR_ASSERT((condition), tc_info)

