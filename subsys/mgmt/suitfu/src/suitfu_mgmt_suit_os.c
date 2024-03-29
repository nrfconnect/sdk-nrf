/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zcbor_common.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>
#include <mgmt/mcumgr/util/zcbor_bulk.h>

#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/mgmt/handlers.h>
#include "suitfu_mgmt_priv.h"

int suitfu_mgmt_suit_bootloader_info_read(struct smp_streamer *ctxt)
{
	zcbor_state_t *zse = ctxt->writer->zs;
	zcbor_state_t *zsd = ctxt->reader->zs;
	struct zcbor_string query = { 0 };
	size_t decoded = 0;
	bool ok;

	struct zcbor_map_decode_key_val bootloader_info[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("query", zcbor_tstr_decode, &query),
	};

	if (zcbor_map_decode_bulk(zsd, bootloader_info, ARRAY_SIZE(bootloader_info), &decoded)) {
		return MGMT_ERR_EINVAL;
	}

	/* If no parameter is recognized then just introduce the bootloader. */
	if (decoded == 0) {
		ok = zcbor_tstr_put_lit(zse, "bootloader") &&
		     zcbor_tstr_put_lit(zse, "SUIT");
	} else {
		return MGMT_ERR_ENOTSUP;
	}

	return ok ? MGMT_ERR_EOK : MGMT_ERR_EMSGSIZE;
}
