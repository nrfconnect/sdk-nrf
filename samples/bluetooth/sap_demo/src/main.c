/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>

#include <bluetooth/services/sap.h>

#include "demo_credentials.h"

LOG_MODULE_REGISTER(sap_main, CONFIG_BT_SAP_LOG_LEVEL);

#if defined(CONFIG_SAMPLE_BT_SAP_ROLE_CENTRAL)
int sap_central_run(const struct bt_sap_policy *policy);
#endif

#if defined(CONFIG_SAMPLE_BT_SAP_ROLE_PERIPHERAL)
int sap_peripheral_run(const struct bt_sap_policy *policy);
#endif

static void set_name(enum sap_role role, const struct bt_sap_device_credential *credential)
{
	char name[24];
	int err;

	snprintk(name, sizeof(name), "SAP-%c-%u", (role == SAP_ROLE_CENTRAL) ? 'C' : 'P',
		 credential->cert.body.device_id);

	err = bt_set_name(name);
	if (err != 0) {
		LOG_WRN("Failed to set dynamic device name (%d)", err);
	}
}

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	struct bt_conn_info info;
	int err;

	err = bt_conn_get_info(conn, &info);
	if (err == 0) {
		LOG_INF("BLE pairing complete: bonded=%u level=%u key_size=%u", bonded,
			info.security.level, info.security.enc_key_size);
		return;
	}

	LOG_INF("BLE pairing complete: bonded=%u", bonded);
}

static void pairing_failed(struct bt_conn *conn, enum bt_security_err err)
{
	ARG_UNUSED(conn);

	LOG_ERR("BLE pairing failed: %d %s", err, bt_security_err_to_str(err));
}

static struct bt_conn_auth_info_cb auth_info_cb = {
	.pairing_complete = pairing_complete,
	.pairing_failed = pairing_failed,
};

int sap_demo_run(void)
{
	enum sap_role role = IS_ENABLED(CONFIG_SAMPLE_BT_SAP_ROLE_CENTRAL) ? SAP_ROLE_CENTRAL
									   : SAP_ROLE_PERIPHERAL;
	const struct bt_sap_device_credential *local_credential;
	struct bt_sap_policy policy;
	size_t ca_len;
	int err;

	local_credential = demo_credentials_select(role);
	memset(&policy, 0, sizeof(policy));
	policy.local_credential = local_credential;
	policy.ca_public_key = demo_credentials_ca_public_key(&ca_len);
	policy.ca_public_key_len = ca_len;
	policy.expected_group_id = CONFIG_SAMPLE_BT_SAP_EXPECTED_GROUP_ID;
	policy.allowed_central_id = CONFIG_SAMPLE_BT_SAP_ALLOWED_CENTRAL_ID;

	if (policy.local_credential == NULL || policy.ca_public_key == NULL ||
	    policy.ca_public_key_len != SAP_IDENTITY_PUBLIC_KEY_LEN) {
		LOG_ERR("Invalid SAP credential configuration");
		return -EINVAL;
	}

	err = bt_enable(NULL);
	if (err != 0) {
		LOG_ERR("Bluetooth init failed (%d)", err);
		return err;
	}

	err = bt_conn_auth_info_cb_register(&auth_info_cb);
	if (err != 0) {
		LOG_ERR("Failed to register pairing diagnostics (%d)", err);
		return err;
	}

	set_name(role, local_credential);

	LOG_INF("SAP sample starting as %s, local device id %u",
		(role == SAP_ROLE_CENTRAL) ? "central" : "peripheral",
		local_credential->cert.body.device_id);
	LOG_DBG("SAP policy: group=0x%02x allowed_central=%u", policy.expected_group_id,
		policy.allowed_central_id);

#if defined(CONFIG_SAMPLE_BT_SAP_ROLE_CENTRAL)
	return sap_central_run(&policy);
#else
	return sap_peripheral_run(&policy);
#endif
}

int main(void)
{
	return sap_demo_run();
}
