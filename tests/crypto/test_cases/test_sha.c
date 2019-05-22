/*$$$LICENCE_NORDIC_STANDARD<2018>$$$*/

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stddef.h>
#include "nrf_gpio.h"
#include "common_test.h"
#include <sha256.h>

#if 1//CONFIG_MBEDTLS

extern test_vector_hash_t __start_test_vector_hash_data[];
extern test_vector_hash_t __stop_test_vector_hash_data[];
extern test_vector_hash_t __start_test_vector_hash_long_data[];
extern test_vector_hash_t __stop_test_vector_hash_long_data[];

#define CONTEXT_SIZE                (240)                                      /**< Temporary context size define. */
#define INPUT_BUF_SIZE              (4125)                                     /**< Input buffer size for SHA. */

/**< Get number of the SHA test vectors. */
#define TEST_VECTOR_SHA_GET(i)      \
    &__start_test_vector_hash_data[i]

/**< Get the test vector reference from the array of test vectors. */
#define TEST_VECTOR_SHA_COUNT       \
    ITEM_COUNT(test_vector_hash_data, test_vector_hash_t)

/**< Get the number of long SHA test vectors. */
#define TEST_VECTOR_LONG_SHA_GET(i) \
    &__start_test_vector_hash_long_data[i]

/**< Get the test vector reference from the array of long test vectors. */
#define TEST_VECTOR_LONG_SHA_COUNT  \
    ITEM_COUNT(test_vector_hash_long_data, test_vector_hash_t)

static mbedtls_sha256_context hash_context;                                 /**< Hash context. */

static uint8_t m_sha_input_buf[INPUT_BUF_SIZE];                                /**< Buffer for storing the unhexified m_sha_input_buf data. */
static uint8_t m_sha_output_buf[64];                  /**< Buffer for holding the calculated hash. */
static uint8_t m_sha_expected_output_buf[64];         /**< Buffer for storing the unhexified expected ouput data. */


/**@brief Function for running the test setup.
 */
void setup_test_case_sha(void)
{
    return;
}


extern test_info_t test_info;

/**@brief Function for the test execution.
 */
void exec_test_case_sha(void)
{
    uint32_t i;
    uint32_t in_len;
    int err_code;
    uint32_t expected_out_len;
    uint32_t sha_test_vector_count = TEST_VECTOR_SHA_COUNT;
    size_t   out_len;

    for (i = 0; i < sha_test_vector_count; i++)
    {
        test_vector_hash_t * p_test_vector = TEST_VECTOR_SHA_GET(i);
        test_info.current_id++;

        memset(m_sha_input_buf, 0x00, sizeof(m_sha_input_buf));
        memset(m_sha_output_buf, 0x00, sizeof(m_sha_output_buf));
        memset(m_sha_expected_output_buf, 0x00, sizeof(m_sha_expected_output_buf));

        // Fetch and unhexify test vectors.
        in_len           = unhexify(m_sha_input_buf, p_test_vector->p_input);
        expected_out_len = unhexify(m_sha_expected_output_buf, p_test_vector->p_expected_output);
        out_len          = expected_out_len;

        // Initialize the hash.
        start_time_measurement();
        mbedtls_sha256_init(&hash_context);
        err_code = mbedtls_sha256_starts_ret(&hash_context, false);
        TEST_VECTOR_ASSERT_ERR_CODE((err_code == 0), "mbedtls_sha256_starts_ret");

        // Update the hash.
        err_code = mbedtls_sha256_update_ret(&hash_context, m_sha_input_buf, in_len);
        TEST_VECTOR_ASSERT_ERR_CODE((err_code == p_test_vector->expected_err_code),
                                    "mbedtls_sha256_update_ret");

        // Finalize the hash.
        err_code = mbedtls_sha256_finish_ret(&hash_context, m_sha_output_buf);
        stop_time_measurement();

        // Verify the mbedtls_sha256_finish_ret err_code.
        TEST_VECTOR_ASSERT_ERR_CODE((err_code == p_test_vector->expected_err_code),
                                    "mbedtls_sha256_finish_ret");

        // Verify the generated digest.
        TEST_VECTOR_ASSERT((expected_out_len == out_len), "Incorrect length");

        TEST_VECTOR_MEMCMP_ASSERT(m_sha_output_buf,
                                  m_sha_expected_output_buf,
                                  expected_out_len,
                                  p_test_vector->expected_result,
                                  "Incorrect hash");

        printk("#%04d Test vector passed: %s %s\n",
                     test_info.current_id,
                     test_info.p_test_case_name,
                     p_test_vector->p_test_vector_name);

        test_info.tests_passed++;

    }
    return;
}

/**@brief Function for the test execution.
 */
void exec_test_case_sha_combined(void)
{
    uint32_t i;
    uint32_t in_len;
    int err_code;
    uint32_t expected_out_len;
    uint32_t sha_test_vector_count = TEST_VECTOR_SHA_COUNT;
    size_t   out_len;

    for (i = 0; i < sha_test_vector_count; i++)
    {
        test_vector_hash_t * p_test_vector = TEST_VECTOR_SHA_GET(i);
        test_info.current_id++;

        memset(m_sha_input_buf, 0x00, sizeof(m_sha_input_buf));
        memset(m_sha_output_buf, 0x00, sizeof(m_sha_output_buf));
        memset(m_sha_expected_output_buf, 0x00, sizeof(m_sha_expected_output_buf));

        // Fetch message to the hash.
        in_len           = unhexify(m_sha_input_buf, p_test_vector->p_input);
        expected_out_len = unhexify(m_sha_expected_output_buf, p_test_vector->p_expected_output);
        out_len          = expected_out_len;

        // Execute the hash method.
        start_time_measurement();
        err_code = mbedtls_sha256_ret(m_sha_input_buf,
                                      in_len,
                                      m_sha_output_buf,
                                      false);
        stop_time_measurement();

        // Verify the mbedtls_sha256_ret err_code.
        TEST_VECTOR_ASSERT_ERR_CODE((err_code == p_test_vector->expected_err_code),
                                    "mbedtls_sha256_ret");

        // Verify the generated digest.
        TEST_VECTOR_ASSERT((expected_out_len == out_len), "Incorrect length");
        TEST_VECTOR_MEMCMP_ASSERT(m_sha_output_buf,
                                  m_sha_expected_output_buf,
                                  expected_out_len,
                                  p_test_vector->expected_result,
                                  "Incorrect hash");

        printk("#%04d Test vector passed: %s %s\n",
                     test_info.current_id,
                     test_info.p_test_case_name,
                     p_test_vector->p_test_vector_name);

        test_info.tests_passed++;

    }
    return;
}


/**@brief Function for verifying the SHA digest of long messages.
 */
void exec_test_case_sha_long(void)
{
    uint32_t i;
    uint32_t j;
    uint32_t in_len;
    int err_code;
    uint32_t expected_out_len;
    uint32_t sha_test_vector_count = TEST_VECTOR_LONG_SHA_COUNT;
    size_t   out_len;

    for (i = 0; i < sha_test_vector_count; i++)
    {
        test_vector_hash_t * p_test_vector = TEST_VECTOR_LONG_SHA_GET(i);
        test_info.current_id++;

        memset(m_sha_input_buf, 0x00, sizeof(m_sha_input_buf));
        memset(m_sha_output_buf, 0x00, sizeof(m_sha_output_buf));
        memset(m_sha_expected_output_buf, 0x00, sizeof(m_sha_expected_output_buf));

        // Fetch and unhexify test vectors.
        in_len           = p_test_vector->chunk_length;
        expected_out_len = unhexify(m_sha_expected_output_buf, p_test_vector->p_expected_output);
        out_len          = expected_out_len;

        memcpy(m_sha_input_buf, p_test_vector->p_input, in_len);

        // Initialize the hash.
        start_time_measurement();
        mbedtls_sha256_init(&hash_context);
        err_code = mbedtls_sha256_starts_ret(&hash_context, false);
        TEST_VECTOR_ASSERT_ERR_CODE((err_code == p_test_vector->expected_err_code),
                                    "mbedtls_sha256_starts_ret");

        // Update the hash until all input data is processed.
        for (j = 0; j < p_test_vector->update_iterations; j++)
        {
            // Test mode for measuring the memcpy from the flash in SHA.
            if (p_test_vector->mode == DO_MEMCPY)
            {
                memcpy(m_sha_input_buf, p_test_vector->p_input, 4096);
            }

            err_code = mbedtls_sha256_update_ret(&hash_context, m_sha_input_buf, in_len);
            TEST_VECTOR_ASSERT_ERR_CODE((err_code == p_test_vector->expected_err_code),
                                        "mbedtls_sha256_update_ret");
        }

        // Finalize the hash.
        err_code = mbedtls_sha256_finish_ret(&hash_context, m_sha_output_buf);
        stop_time_measurement();

        // Verify the mbedtls_sha256_finish_ret err_code.
        TEST_VECTOR_ASSERT_ERR_CODE((err_code == p_test_vector->expected_err_code),
                                    "mbedtls_sha256_finish_ret");

        // Verify the generated digest.
        TEST_VECTOR_ASSERT((expected_out_len == out_len), "Incorrect length");
        TEST_VECTOR_MEMCMP_ASSERT(m_sha_output_buf,
                                  m_sha_expected_output_buf,
                                  expected_out_len,
                                  p_test_vector->expected_result,
                                  "Incorrect hash");

        printk("#%04d Test vector passed: %s %s\n",
                     test_info.current_id,
                     test_info.p_test_case_name,
                     p_test_vector->p_test_vector_name);

        test_info.tests_passed++;

    }
    return;
}


/**@brief Function for running the test teardown.
 */
void teardown_test_case_sha(void)
{
    return;
}


/** @brief  Macro for registering the SHA test case by using section variables.
 *
 * @details     This macro places a variable in a section named "test_case_data",
 *              which is initialized by main.
 */
ITEM_REGISTER(test_case_data, test_case_t test_sha) =
{
    .p_test_case_name = "SHA",
    .setup = setup_test_case_sha,
    .exec = exec_test_case_sha,
    .teardown = teardown_test_case_sha
};


/** @brief  Macro for registering the SHA test case by using section variables.
 *
 * @details     This macro places a variable in a section named "test_case_data",
 *              which is initialized by main.
 */
ITEM_REGISTER(test_case_data, test_case_t test_sha_combined) =
{
    .p_test_case_name = "SHA combined",
    .setup = setup_test_case_sha,
    .exec = exec_test_case_sha_combined,
    .teardown = teardown_test_case_sha
};


/** @brief  Macro for registering the SHA test case by using section variables.
 *
 * @details     This macro places a variable in a section named "test_case_data",
 *              which is initialized by main.
 */
ITEM_REGISTER(test_case_data, test_case_t test_sha_long) =
{
    .p_test_case_name = "SHA long",
    .setup = setup_test_case_sha,
    .exec = exec_test_case_sha_long,
    .teardown = teardown_test_case_sha
};

#endif
