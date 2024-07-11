/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_decrypt_filter.h>

suit_plat_err_t suit_decrypt_filter_get(struct stream_sink *dec_sink,
					struct suit_encryption_info *enc_info,
					struct stream_sink *enc_sink)
{
	if ((enc_info == NULL) || (enc_sink == NULL) || (dec_sink == NULL)) {
		return SUIT_PLAT_ERR_INVAL;
	}

	dec_sink->ctx = enc_sink->ctx;
	dec_sink->erase = enc_sink->erase;
	dec_sink->flush = enc_sink->flush;
	dec_sink->release = enc_sink->release;
	/* Seeking is not possible on encrypted payload. */
	dec_sink->seek = NULL;
	dec_sink->used_storage = enc_sink->used_storage;
	dec_sink->write = enc_sink->write;

	return SUIT_PLAT_SUCCESS;
}
