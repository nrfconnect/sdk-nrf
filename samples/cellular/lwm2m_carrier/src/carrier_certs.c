/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <string.h>
#include <zephyr/toolchain.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>
#include <modem/modem_key_mgmt.h>
#include "carrier_certs.h"

#ifdef CONFIG_LWM2M_CARRIER

#define ORANGE "\e[0;33m"
#define GREEN "\e[0;32m"
#define NORMAL "\e[0m"

static const char ca411[] = {
	/* VzW and Motive, AT&T */
	#include "../certs/DigiCertGlobalRootG2.pem"
};
static const char ca412[] = {
	/* VzW and Motive, AT&T */
	#include "../certs/DigiCertGlobalRootCA.pem"
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

int carrier_cert_provision(void)
{
	int err;
	bool mismatch = 0;
	bool provisioned;

	for (int i = 0; i < ARRAY_SIZE(certs); i++) {
		err = modem_key_mgmt_exists(
			certs[i].tag, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN, &provisioned);

		if (err) {
			return -1;
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
				return -1;
			}
			printk("Certificate provisioned, tag %d\n", certs[i].tag);
		}
	}
	return 0;
}

int carrier_psk_provision(void)
{
	if (CONFIG_LWM2M_CARRIER_SERVER_SEC_TAG) {
		char *p_psk = CONFIG_CARRIER_APP_PSK;

		if (strlen(p_psk) <= 0) {
			return 0;
		}

		uint8_t psk_len = strlen(p_psk);
		int err = 0;

		if (psk_len > 0) {
			err = modem_key_mgmt_write(CONFIG_LWM2M_CARRIER_SERVER_SEC_TAG,
			MODEM_KEY_MGMT_CRED_TYPE_PSK, p_psk, psk_len);
			printk("PSK provisioned, tag %d\n", CONFIG_LWM2M_CARRIER_SERVER_SEC_TAG);
		}
		if (err) {
			printk("Unable to provision PSK, tag %d\n",
				CONFIG_LWM2M_CARRIER_SERVER_SEC_TAG);
			return -1;
		}
	}

	return 0;
}

#endif /* CONFIG_LWM2M_CARRIER */
