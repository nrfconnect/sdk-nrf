/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stddef.h>

#include "common_test.h"
#include <mbedtls/ecp.h>

/**@brief ECDSA test vectors can be found on NIST web pages.
 *
 * https://csrc.nist.gov/Projects/Cryptographic-Algorithm-Validation-Program/Component-Testing
 */
#if defined(MBEDTLS_ECP_DP_SECP256R1_ENABLED)
/* ECDSA random - NIST P-256, SHA-256 - first test case */
ITEM_REGISTER(test_vector_ecdsa_random_data,
	      test_vector_ecdsa_random_t
		      test_vector_ecdsa_random_secp256r1_SHA256_1) = {
	.curve_type = MBEDTLS_ECP_DP_SECP256R1,
	.p_test_vector_name = TV_NAME("secp256r1 random SHA256 1"),
	.p_input =
		"44acf6b7e36c1342c2c5897204fe09504e1e2efb1a900377dbc4e7a6a133ec56",
};
#endif /* MBEDTLS_ECP_DP_SECP256R1_ENABLED */
