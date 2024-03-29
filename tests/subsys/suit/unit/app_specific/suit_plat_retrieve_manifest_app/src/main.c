/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <suit_platform.h>
#include <suit_plat_retrieve_manifest_domain_specific.h>

ZTEST_SUITE(suit_platform_retrieve_manifest_tests, NULL, NULL, NULL, NULL, NULL);

ZTEST(suit_platform_retrieve_manifest_tests, test_retrieve_manifest_app_specific)
{
	struct zcbor_string dummy_component_id = {0};
	uint8_t *envelope = NULL;
	size_t envelope_len = 0;

	/* Simply return SUIT_ERR_UNSUPPORTED_COMPONENT_ID unconditionally */
	zassert_equal(suit_plat_retrieve_manifest_domain_specific(&dummy_component_id,
								  SUIT_COMPONENT_TYPE_INSTLD_MFST,
								  envelope, envelope_len),
		      SUIT_ERR_UNSUPPORTED_COMPONENT_ID,
		      "suit_plat_retrieve_manifest_domain_specific returned incorrect error code");
}
