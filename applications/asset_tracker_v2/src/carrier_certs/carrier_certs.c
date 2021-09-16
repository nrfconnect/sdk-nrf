/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <string.h>
#include <toolchain.h>
#include <sys/util.h>
#include <modem/modem_key_mgmt.h>

#include "carrier_certs.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(carrier_certs, CONFIG_MODEM_MODULE_LOG_LEVEL);

/* The order of the tags in this array specifies which tag is used first */
static int tags[] = { 411, 412 };

static const char ca411[] = {
	/* VzW and Motive, AT&T */
	#include "../certs/DigiCertGlobalRootG2.pem"
};
static const char ca412[] = {
	/* AT&T Interop */
	#include "../certs/DSTRootCA-X3.pem"
};

BUILD_ASSERT(sizeof(ca411) < KB(4), "Cert is too large");
BUILD_ASSERT(sizeof(ca412) < KB(4), "Cert is too large");

static const struct {
	uint16_t tag;
	const char *data;
} certs[] = {
	{
		.tag = 411,
		.data = ca411,
	},
	{
		.tag = 412,
		.data = ca412,
	}
};

static const ca_cert_tags_t ca_tags = {
	.tags = tags,
	.count = ARRAY_SIZE(tags),
};

int carrier_certs_provision(ca_cert_tags_t * const tags)
{
	int err;
	bool mismatch = 0;
	bool provisioned;

	if (tags == NULL) {
		LOG_ERR("Invalid input argument");
		return -1;
	}

	for (int i = 0; i < ARRAY_SIZE(certs); i++) {
		err = modem_key_mgmt_exists(
			certs[i].tag, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN, &provisioned);
		if (err) {
			goto cert_exit_empty;
		}

		if (provisioned) {
			err = modem_key_mgmt_cmp(
				certs[i].tag, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
				certs[i].data, strlen(certs[i].data));

			/* 0 on match, 1 otherwise; like memcmp() */
			mismatch = err;

			LOG_DBG("Certificate found, tag %d: %s", certs[i].tag,
				mismatch ? "mismatch" : "match");
		} else {
			LOG_DBG("Certificate tag %d not found", certs[i].tag);
		}

		if (mismatch || !provisioned) {
			/* overwrite the certificate */
			err = modem_key_mgmt_write(
				certs[i].tag, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
				certs[i].data, strlen(certs[i].data));
			if (err) {
				LOG_ERR("Unable to provision certificate, error: %d", err);
				goto cert_exit_empty;
			}

			LOG_DBG("Certificate provisioned, tag %d", certs[i].tag);
		}
	}

	*tags = ca_tags;

	return 0;

cert_exit_empty:
	tags->count = 0;
	tags->tags = NULL;

	return -1;
}
