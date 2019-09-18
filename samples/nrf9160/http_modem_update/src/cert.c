/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <nrf_inbuilt_key.h>
#include <nrf_key_mgmt.h>

#define SECURITY_TAG 0x7A

/* Certificates can be concatenated */
static const char cert[] = {
	#include "../certs/DigiCert Baltimore CA-2 G2"
};

BUILD_ASSERT_MSG(sizeof(cert) <= KB(4),
		 "Concatenated certificate size may not exceed 4Kb");

int cert_provision(void)
{
	int err;
	u8_t dummy;
	bool exists = false;
	nrf_sec_tag_t sec_tag = SECURITY_TAG;

	err = nrf_inbuilt_key_exists(sec_tag, NRF_KEY_MGMT_CRED_TYPE_CA_CHAIN,
				     &exists, &dummy);
	if (err) {
		printk("Failed to check for certificates, err %d\n", errno);
		return err;
	}

	if (exists) {
		printk("Certificates already provisioned\n");
		return 0;
	}

	err = nrf_inbuilt_key_write(sec_tag,
				    NRF_KEY_MGMT_CRED_TYPE_CA_CHAIN,
				    (char *)cert, sizeof(cert) - 1);
	if (err) {
		printk("Could not provision cert, tag %d err: %d",
			sec_tag, errno);
		return err;
	}

	printk("Provisioned certificates, key 0x%x\n", sec_tag);

	return 0;
}

int cert_delete(void)
{
	int err;
	nrf_sec_tag_t sec_tag = SECURITY_TAG;

	err = nrf_inbuilt_key_delete(sec_tag, NRF_KEY_MGMT_CRED_TYPE_CA_CHAIN);
	if (err) {
		printk("Failed to delete credentials, tag 0x%x, err %d\n",
			sec_tag, errno);
	}

	printk("Certificates deleted\n");

	return 0;
}
