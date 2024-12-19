/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <psa/crypto.h>

int mac_create_hmac(const struct sxhashalg *hashalg, struct sxhash *hashopctx, const char *key,
		    size_t keysz, char *workmem, size_t workmemsz);

int hmac_produce(struct sxhash *hashctx, const struct sxhashalg *hashalg, char *out, size_t sz,
		 char *workmem);
