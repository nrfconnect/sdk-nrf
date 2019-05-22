/*$$$LICENCE_NORDIC_STANDARD<2018>$$$*/

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stddef.h>
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrf_error.h"
#include "app_util.h"
#include "nrf_section.h"
#include "nrf_crypto.h"
#include "common_test.h"

/**@brief ECDSA test vectors can be found on NIST web pages.
 *
 * https://csrc.nist.gov/Projects/Cryptographic-Algorithm-Validation-Program/Component-Testing
 */

/*lint -save -e91 */

#if NRF_CRYPTO_ECC_ENABLED && NRF_MODULE_ENABLED(NRF_CRYPTO_BACKEND_OPTIGA)

#if NRF_MODULE_ENABLED(NRF_CRYPTO_ECC_SECP160R1)
#endif // NRF_MODULE_ENABLED(NRF_CRYPTO_ECC_SECP160R1)
#if NRF_MODULE_ENABLED(NRF_CRYPTO_ECC_SECP160R2)
#endif // NRF_MODULE_ENABLED(NRF_CRYPTO_ECC_SECP160R2)
#if NRF_MODULE_ENABLED(NRF_CRYPTO_ECC_SECP192R1)
#endif // NRF_MODULE_ENABLED(NRF_CRYPTO_ECC_SECP192R1)

#if NRF_MODULE_ENABLED(NRF_CRYPTO_ECC_SECP256R1)

// ECDSA random - NIST P-256, SHA-256 - first test case
NRF_SECTION_ITEM_REGISTER(test_vector_ecdsa_random_data, test_vector_ecdsa_random_t test_vector_ecdsa_random_secp256r1_sha256_1) =
{
    .p_curve_info               = &g_nrf_crypto_ecc_secp256r1_curve_info,
    .p_test_vector_name         = "secp256r1 random sha256 1",
    .p_input                    = "44acf6b7e36c1342c2c5897204fe09504e1e2efb1a900377dbc4e7a6a133ec56",
    .sig_len                    = NRF_CRYPTO_ECDSA_SECP256R1_SIGNATURE_SIZE,
};

#endif // NRF_MODULE_ENABLED(NRF_CRYPTO_ECC_SECP256R1)

#if NRF_MODULE_ENABLED(NRF_CRYPTO_ECC_SECP384R1)
#endif // NRF_MODULE_ENABLED(NRF_CRYPTO_ECC_SECP384R1)

#if NRF_MODULE_ENABLED(NRF_CRYPTO_ECC_SECP521R1)
#endif // NRF_MODULE_ENABLED(NRF_CRYPTO_ECC_SECP521R1)

#if NRF_MODULE_ENABLED(NRF_CRYPTO_ECC_SECP160K1)
#endif // NRF_MODULE_ENABLED(NRF_CRYPTO_ECC_SECP160K1)
#if NRF_MODULE_ENABLED(NRF_CRYPTO_ECC_SECP192K1)
#endif // NRF_MODULE_ENABLED(NRF_CRYPTO_ECC_SECP192K1)
#if NRF_MODULE_ENABLED(NRF_CRYPTO_ECC_SECP224K1)
#endif // NRF_MODULE_ENABLED(NRF_CRYPTO_ECC_SECP224K1)
#if NRF_MODULE_ENABLED(NRF_CRYPTO_ECC_SECP256K1)
#endif // NRF_MODULE_ENABLED(NRF_CRYPTO_ECC_SECP256K1)

#endif // NRF_CRYPTO_ECC_ENABLED && !NRF_MODULE_ENABLED(NRF_CRYPTO_BACKEND_CC310_BL)

/*lint -restore */
