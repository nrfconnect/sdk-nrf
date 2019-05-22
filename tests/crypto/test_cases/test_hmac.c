/*$$$LICENCE_NORDIC_STANDARD<2018>$$$*/

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stddef.h>
#include "boards.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrf_error.h"
#include "app_util.h"
#include "nrf_section.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "common_test.h"
#include "nrf_crypto.h"

#if NRF_MODULE_ENABLED(NRF_CRYPTO_HMAC)

NRF_SECTION_DEF(test_vector_hmac_data, test_vector_hmac_t);

#define CONTEXT_SIZE                        (240)                              /**< Temporary context size define. */
#define INPUT_BUF_SIZE                      (256)                              /**< Input buffer size for SHA. */
#define NRF_CRYPTO_HMAC_SHA512_RESULT_SIZE  (64)

/**< Get number of SHA test vectors. */
#define TEST_VECTOR_HMAC_GET(i)             \
  NRF_SECTION_ITEM_GET(test_vector_hmac_data, test_vector_hmac_t, (i))

/**< Get test vector reference from array of test vectors. */
#define TEST_VECTOR_HMAC_COUNT              \
    NRF_SECTION_ITEM_COUNT(test_vector_hmac_data, test_vector_hmac_t)

static nrf_crypto_hmac_context_t hmac_context;                                 /**< HMAC context. */

static uint8_t m_hmac_key_buf[INPUT_BUF_SIZE];                                 /**< Buffer for storing the unhexified m_hmac_input_buf data. */
static uint8_t m_hmac_input_buf[INPUT_BUF_SIZE];                               /**< Buffer for storing the unhexified m_hmac_input_buf data. */
static uint8_t m_hmac_output_buf[NRF_CRYPTO_HMAC_SHA512_RESULT_SIZE];          /**< Buffer for holding the calculated hash. */
static uint8_t m_hmac_expected_output_buf[NRF_CRYPTO_HMAC_SHA512_RESULT_SIZE]; /**< Buffer for storing the unhexified expected output data. */


/**@brief Function for running the test setup.
 */
ret_code_t setup_test_case_hmac(void)
{
    return NRF_SUCCESS;
}


/**@brief Function for the test execution.
 */
ret_code_t exec_test_case_hmac(test_info_t * p_test_info)
{
    uint32_t i;
    ret_code_t err_code;
    uint32_t hmac_test_vector_count = TEST_VECTOR_HMAC_COUNT;
    size_t   in_len;
    size_t   key_len;
    size_t   hmac_len;
    size_t   expected_hmac_len;

    for (i = 0; i < hmac_test_vector_count; i++)
    {
        test_vector_hmac_t * p_test_vector = TEST_VECTOR_HMAC_GET(i);
        p_test_info->current_id++;

        memset(m_hmac_key_buf, 0x00, sizeof(m_hmac_key_buf));
        memset(m_hmac_input_buf, 0x00, sizeof(m_hmac_input_buf));
        memset(m_hmac_output_buf, 0x00, sizeof(m_hmac_output_buf));
        memset(m_hmac_expected_output_buf, 0x00, sizeof(m_hmac_expected_output_buf));

        // Fetch and unhexify test vectors.
        key_len           = unhexify(m_hmac_key_buf, p_test_vector->p_key);
        in_len            = unhexify(m_hmac_input_buf, p_test_vector->p_input);
        expected_hmac_len = unhexify(m_hmac_expected_output_buf,
                                     p_test_vector->p_expected_output);

        hmac_len = expected_hmac_len;
        hmac_context.hmac_sha512_context.header.p_info = p_test_vector->p_hmac_info;

        // Initialize the HMAC module.
        start_time_measurement();
        err_code = nrf_crypto_hmac_init(&hmac_context,
                                        p_test_vector->p_hmac_info,
                                        m_hmac_key_buf,
                                        key_len);

        // Verify the nrf_crypto_hmac_init err_code.
        TEST_VECTOR_ASSERT_ERR_CODE((err_code == p_test_vector->expected_err_code),
                                    "nrf_crypto_hmac_init");

        // Update the HMAC with message.
        err_code = nrf_crypto_hmac_update(&hmac_context,
                                          m_hmac_input_buf,
                                          in_len);

        // Verify the nrf_crypto_hmac_update err_code.
        TEST_VECTOR_ASSERT_ERR_CODE((err_code == p_test_vector->expected_err_code),
                                    "nrf_crypto_hmac_update");

        // Finalize the HMAC computation.
        err_code = nrf_crypto_hmac_finalize(&hmac_context, m_hmac_output_buf, &hmac_len);
        stop_time_measurement();
        
        // Verify the nrf_crypto_hmac_finalize err_code.
        TEST_VECTOR_ASSERT_ERR_CODE((err_code == p_test_vector->expected_err_code),
                                    "nrf_crypto_hmac_finalize");

        // Verify the generated HMAC.
        TEST_VECTOR_ASSERT((expected_hmac_len == hmac_len), "Incorrect hmac length");
        TEST_VECTOR_MEMCMP_ASSERT(m_hmac_output_buf,
                                  m_hmac_expected_output_buf,
                                  expected_hmac_len,
                                  p_test_vector->expected_result,
                                  "Incorrect hmac");

        NRF_LOG_INFO("#%04d Test vector passed: %s %s",
                     p_test_info->current_id,
                     p_test_info->p_test_case_name,
                     p_test_vector->p_test_vector_name);

        p_test_info->tests_passed++;

exit_test_vector:

        while (NRF_LOG_PROCESS());

    }
    return NRF_SUCCESS;
}


/**@brief Function for the test execution.
 */
ret_code_t exec_test_case_hmac_combined(test_info_t * p_test_info)
{
    uint32_t i;
    ret_code_t err_code;
    uint32_t hmac_test_vector_count = TEST_VECTOR_HMAC_COUNT;
    size_t   in_len;
    size_t   key_len;
    size_t   hmac_len;
    size_t   expected_hmac_len;

    for (i = 0; i < hmac_test_vector_count; i++)
    {
        test_vector_hmac_t * p_test_vector = TEST_VECTOR_HMAC_GET(i);
        p_test_info->current_id++;

        memset(m_hmac_key_buf, 0x00, sizeof(m_hmac_key_buf));
        memset(m_hmac_input_buf, 0x00, sizeof(m_hmac_input_buf));
        memset(m_hmac_output_buf, 0x00, sizeof(m_hmac_output_buf));
        memset(m_hmac_expected_output_buf, 0x00, sizeof(m_hmac_expected_output_buf));

        // Fetch and unhexify test vectors.
        key_len           = unhexify(m_hmac_key_buf, p_test_vector->p_key);
        in_len            = unhexify(m_hmac_input_buf, p_test_vector->p_input);
        expected_hmac_len = unhexify(m_hmac_expected_output_buf,
                                     p_test_vector->p_expected_output);

        hmac_len = expected_hmac_len;
        hmac_context.hmac_sha512_context.header.p_info = p_test_vector->p_hmac_info;

        // Generate the HMAC using the combined method.
        start_time_measurement();
        err_code = nrf_crypto_hmac_calculate(&hmac_context,
                                             p_test_vector->p_hmac_info,
                                             m_hmac_output_buf, &hmac_len,
                                             m_hmac_key_buf,
                                             key_len,
                                             m_hmac_input_buf,
                                             in_len);
        stop_time_measurement();

        // Verify the nrf_crypto_hmac_calculate err_code.
        TEST_VECTOR_ASSERT_ERR_CODE((err_code == p_test_vector->expected_err_code),
                                    "nrf_crypto_hmac_calculate");

        // Verify the generated HMAC.
        TEST_VECTOR_ASSERT((expected_hmac_len == hmac_len), "Incorrect hmac length");
        TEST_VECTOR_MEMCMP_ASSERT(m_hmac_output_buf,
                                  m_hmac_expected_output_buf,
                                  expected_hmac_len,
                                  p_test_vector->expected_result,
                                  "Incorrect hmac");

        NRF_LOG_INFO("#%04d Test vector passed: %s %s",
                     p_test_info->current_id,
                     p_test_info->p_test_case_name,
                     p_test_vector->p_test_vector_name);

        p_test_info->tests_passed++;

exit_test_vector: ;

    }
    return NRF_SUCCESS;
}


/**@brief Function for running the test teardown.
 */
ret_code_t teardown_test_case_hmac(void)
{
    return NRF_SUCCESS;
}


/** @brief  Macro for registering the ECDH test case by using section variables.
 *
 * @details     This macro places a variable in a section named "test_case_data",
 *              which is initialized by main.
 */
NRF_SECTION_ITEM_REGISTER(test_case_data, test_case_t test_hmac) =
{
    .p_test_case_name = "HMAC",
    .setup = setup_test_case_hmac,
    .exec = exec_test_case_hmac,
    .teardown = teardown_test_case_hmac
};


/** @brief  Macro for registering the ECDH test case by using section variables.
 *
 * @details     This macro places a variable in a section named "test_case_data",
 *              which is initialized by main.
 */
NRF_SECTION_ITEM_REGISTER(test_case_data, test_case_t test_hmac_combined) =
{
    .p_test_case_name = "HMAC combined",
    .setup = setup_test_case_hmac,
    .exec = exec_test_case_hmac_combined,
    .teardown = teardown_test_case_hmac
};

#endif // NRF_CRYPTO_HMAC_ENABLED
