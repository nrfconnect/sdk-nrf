/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOCK_SUIT_PLATFORM_H__
#define MOCK_SUIT_PLATFORM_H__

#include <zephyr/fff.h>
#include <mock_globals.h>

#include <suit_platform.h>

FAKE_VALUE_FUNC(int, suit_plat_check_digest, enum suit_cose_alg, struct zcbor_string *,
		struct zcbor_string *);
FAKE_VALUE_FUNC(int, suit_plat_authenticate_manifest, struct zcbor_string *, enum suit_cose_alg,
		struct zcbor_string *, struct zcbor_string *, struct zcbor_string *);
FAKE_VALUE_FUNC(int, suit_plat_authorize_unsigned_manifest, struct zcbor_string *);
FAKE_VALUE_FUNC(int, suit_plat_create_component_handle, struct zcbor_string *, suit_component_t *);
FAKE_VALUE_FUNC(int, suit_plat_release_component_handle, suit_component_t);

#ifdef CONFIG_CHECK_IMAGE_MATCH_TEST
FAKE_VALUE_FUNC(int, suit_plat_check_image_match, suit_component_t, enum suit_cose_alg,
		struct zcbor_string *);
#endif /* CONFIG_CHECK_IMAGE_MATCH_TEST */

FAKE_VALUE_FUNC(int, suit_plat_check_content, suit_component_t, struct zcbor_string *);
FAKE_VALUE_FUNC(int, suit_plat_check_slot, suit_component_t, unsigned int);
FAKE_VALUE_FUNC(int, suit_plat_check_vid, suit_component_t, struct zcbor_string *);
FAKE_VALUE_FUNC(int, suit_plat_check_cid, suit_component_t, struct zcbor_string *);
FAKE_VALUE_FUNC(int, suit_plat_check_did, suit_component_t, struct zcbor_string *);
FAKE_VALUE_FUNC(int, suit_plat_authorize_sequence_num, enum suit_command_sequence,
		struct zcbor_string *, unsigned int);
FAKE_VALUE_FUNC(int, suit_plat_authorize_component_id, struct zcbor_string *,
		struct zcbor_string *);
FAKE_VALUE_FUNC(int, suit_plat_fetch, suit_component_t, struct zcbor_string *);
FAKE_VALUE_FUNC(int, suit_plat_fetch_integrated, suit_component_t, struct zcbor_string *);
FAKE_VALUE_FUNC(int, suit_plat_copy, suit_component_t, suit_component_t);
FAKE_VALUE_FUNC(int, suit_plat_swap, suit_component_t, suit_component_t);
FAKE_VALUE_FUNC(int, suit_plat_write, suit_component_t, struct zcbor_string *);
FAKE_VALUE_FUNC(int, suit_plat_invoke, suit_component_t, struct zcbor_string *);
FAKE_VALUE_FUNC(int, suit_plat_report, unsigned int, struct suit_report *);
FAKE_VALUE_FUNC(int, suit_plat_sequence_completed, enum suit_command_sequence,
		struct zcbor_string *, const uint8_t *, size_t);
FAKE_VALUE_FUNC(int, suit_plat_retrieve_manifest, suit_component_t, const uint8_t **, size_t *);
FAKE_VALUE_FUNC(int, suit_plat_override_image_size, suit_component_t, size_t);

#ifdef CONFIG_SUIT_PLATFORM_DRY_RUN_SUPPORT
FAKE_VALUE_FUNC(int, suit_plat_check_fetch, suit_component_t, struct zcbor_string *);
FAKE_VALUE_FUNC(int, suit_plat_check_fetch_integrated, suit_component_t, struct zcbor_string *);
FAKE_VALUE_FUNC(int, suit_plat_check_copy, suit_component_t, suit_component_t);
FAKE_VALUE_FUNC(int, suit_plat_check_swap, suit_component_t, suit_component_t);
FAKE_VALUE_FUNC(int, suit_plat_check_write, suit_component_t, struct zcbor_string *);
FAKE_VALUE_FUNC(int, suit_plat_check_invoke, suit_component_t, struct zcbor_string *);
#endif /* CONFIG_SUIT_PLATFORM_DRY_RUN_SUPPORT */

static inline void mock_suit_platform_reset(void)
{
	RESET_FAKE(suit_plat_check_digest);
	RESET_FAKE(suit_plat_authenticate_manifest);
	RESET_FAKE(suit_plat_authorize_unsigned_manifest);
	RESET_FAKE(suit_plat_create_component_handle);
	RESET_FAKE(suit_plat_release_component_handle);

#ifdef CONFIG_CHECK_IMAGE_MATCH_TEST
	RESET_FAKE(suit_plat_check_image_match);
#endif /* CONFIG_CHECK_IMAGE_MATCH_TEST */

	RESET_FAKE(suit_plat_check_content);
	RESET_FAKE(suit_plat_check_slot);
	RESET_FAKE(suit_plat_check_vid);
	RESET_FAKE(suit_plat_check_cid);
	RESET_FAKE(suit_plat_check_did);
	RESET_FAKE(suit_plat_authorize_sequence_num);
	RESET_FAKE(suit_plat_authorize_component_id);
	RESET_FAKE(suit_plat_fetch);
	RESET_FAKE(suit_plat_fetch_integrated);
	RESET_FAKE(suit_plat_copy);
	RESET_FAKE(suit_plat_swap);
	RESET_FAKE(suit_plat_write);
	RESET_FAKE(suit_plat_invoke);
	RESET_FAKE(suit_plat_report);
	RESET_FAKE(suit_plat_sequence_completed);
	RESET_FAKE(suit_plat_retrieve_manifest);
	RESET_FAKE(suit_plat_override_image_size);

#ifdef CONFIG_SUIT_PLATFORM_DRY_RUN_SUPPORT
	RESET_FAKE(suit_plat_check_fetch);
	RESET_FAKE(suit_plat_check_fetch_integrated);
	RESET_FAKE(suit_plat_check_copy);
	RESET_FAKE(suit_plat_check_swap);
	RESET_FAKE(suit_plat_check_write);
	RESET_FAKE(suit_plat_check_invoke);
#endif /* CONFIG_SUIT_PLATFORM_DRY_RUN_SUPPORT */
}
#endif /* MOCK_SUIT_PLATFORM_H__ */
