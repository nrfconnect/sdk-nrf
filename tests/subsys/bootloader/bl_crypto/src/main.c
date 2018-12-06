/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <ztest.h>

#include "bl_crypto.h"
#include "test_vector.c"


void test_crypto_root_of_trust(void)
{
	/* Success. */
	int retval = crypto_root_of_trust(pk, pk_hash, sig, firmware,
			sizeof(firmware));

	zassert_equal(0, retval, "retval was %d", retval);

	/* pk doesn't match pk_hash. */
	pk[1]++;
	retval = crypto_root_of_trust(pk, pk_hash, sig, firmware,
			sizeof(firmware));
	pk[1]--;

	zassert_equal(-EPKHASHINV, retval, "retval was %d", retval);

	/* metadata doesn't match signature */
	firmware[0]++;
	retval = crypto_root_of_trust(pk, pk_hash, sig, firmware,
			sizeof(firmware));
	firmware[0]--;

	zassert_equal(-ESIGINV, retval, "retval was %d", retval);
}

void test_main(void)
{
	ztest_test_suite(test_bl_crypto,
			 ztest_unit_test(test_crypto_root_of_trust));
	ztest_run_test_suite(test_bl_crypto);
}
