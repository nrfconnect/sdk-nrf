/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOCK_SUIT_SERVICE_H__
#define MOCK_SUIT_SERVICE_H__

#include <zephyr/fff.h>
#include <mock_globals.h>

#include <sdfw/sdfw_services/suit_service.h>

/* suit_update.c */
FAKE_VALUE_FUNC(suit_ssf_err_t, suit_get_installed_manifest_info, suit_manifest_class_id_t *,
		unsigned int *, suit_semver_raw_t *, suit_digest_status_t *, int *,
		suit_plat_mreg_t *);

static inline void mock_suit_service_reset(void)
{
	RESET_FAKE(suit_get_installed_manifest_info);
}

#endif /* MOCK_SUIT_SERVICE_H__ */
