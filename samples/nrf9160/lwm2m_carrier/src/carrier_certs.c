/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <string.h>
#include <toolchain.h>
#include <sys/util.h>
#include <sys/printk.h>
#include <modem/modem_key_mgmt.h>
#include "carrier_certs.h"

#ifdef CONFIG_LWM2M_CARRIER

#define ORANGE "\e[0;33m"
#define GREEN "\e[0;32m"
#define NORMAL "\e[0m"

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

int carrier_cert_provision(ca_cert_tags_t * const tags)
{
	int err;
	bool mismatch = 0;
	bool provisioned;

	if (tags == NULL) {
		printk(ORANGE "ERROR: " NORMAL "Invalid input argument!\n");
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

			printk("Certificate found, tag %d: %s\n" NORMAL, certs[i].tag,
				mismatch ? ORANGE "mismatch" : GREEN "match");
		} else {
			printk("Certificate tag %d " ORANGE "not found\n" NORMAL,
				  certs[i].tag);
		}

		if (mismatch || !provisioned) {
			/* overwrite the certificate */
			err = modem_key_mgmt_write(
				certs[i].tag, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
				certs[i].data, strlen(certs[i].data));
			if (err) {
				printk(ORANGE "ERROR: " NORMAL
				  "Unable to provision certificate, err: %d\n", err);
				goto cert_exit_empty;
			}
			printk("Certificate provisioned, tag %d\n", certs[i].tag);
		}
	}

	*tags = ca_tags;

	return 0;

cert_exit_empty:
	tags->count = 0;
	tags->tags = NULL;
	return -1;
}

#endif /* CONFIG_LWM2M_CARRIER */
