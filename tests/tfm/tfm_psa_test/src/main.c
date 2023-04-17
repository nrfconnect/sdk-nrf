/*
 * Copyright (c) 2022 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifdef CONFIG_TFM_PSA_TEST_NONE
#error "No PSA test suite set. See "Building and Running" in README."
#endif

/* Run the PSA test suite */
void psa_test(void);

int main(void)
{
	psa_test();

	return 0;
}
