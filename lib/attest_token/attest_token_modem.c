/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <nrf_modem_at.h>
#include <attest_token.h>

#include "attest_token_modem.h"

LOG_MODULE_DECLARE(attest_token, CONFIG_ATTEST_TOKEN_LOG_LEVEL);

#define AT_ATTEST_CMD "AT%ATTESTTOKEN"

int attest_token_modem_get(struct nrf_attestation_token *const token)
{
	int ret = 0;
	size_t attest_sz;
	size_t cose_sz;
	bool attest_alloc = false;
	/* Maximum response length is 200 bytes */
	char attest[128] = {0};
	char cose[128] = {0};

	if (!token) {
		return -EINVAL;
	} else if ((token->attest && !token->attest_sz) ||
		   (token->cose && !token->cose_sz)) {
		return -EBADF;
	}

	/* Execute AT command to get attestation token */
	ret = nrf_modem_at_scanf(AT_ATTEST_CMD,
		"%%ATTESTTOKEN: \"%127[^.].%127[^\"]\"", attest, cose);
	if (ret != 2) {
		LOG_ERR("Failed to parse %%ATTESTTOKEN response (ret %d)", ret);
		return -EBADMSG;
	}

	ret = 0;

	attest_sz = strlen(attest) + 1;
	cose_sz = strlen(cose) + 1;

	/* Ensure provided buffers are large enough */
	if (((token->attest) && (token->attest_sz < attest_sz)) ||
	    ((token->cose) && (token->cose_sz < cose_sz))) {
		LOG_ERR("Provided buffers are not large enough (attest_sz %d, cose_sz %d)",
			attest_sz, cose_sz);
		return -EMSGSIZE;
	}

	/* Allocate if not provided */
	if (!token->attest) {
		token->attest = k_calloc(attest_sz, 1);
		if (!token->attest) {
			ret = -ENOMEM;
			goto cleanup;
		}
		attest_alloc = true;
		token->attest_sz = attest_sz;
	}
	if (!token->cose) {
		token->cose = k_calloc(cose_sz, 1);
		if (!token->cose) {
			ret = -ENOMEM;
			goto cleanup;
		}
		token->cose_sz = cose_sz;
	}

	/* Copy token contents */
	memcpy(token->attest, attest, attest_sz);
	memcpy(token->cose, cose, cose_sz);

cleanup:
	if (ret && attest_alloc) {
		k_free(token->attest);
		token->attest = NULL;
		token->attest_sz = 0;
	}

	return ret;
}
