/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <modem/modem_key_mgmt.h>
#include <modem/nrf_modem_lib.h>

static const unsigned char ca_certificate[] = {
#if __has_include("ca-cert.pem")
#include "ca-cert.pem"
#else
""
#endif
};

static const unsigned char device_certificate[] = {
#if __has_include("client-cert.pem")
#include "client-cert.pem"
#else
""
#endif
};

static const unsigned char private_key[] = {
#if __has_include("private-key.pem")
#include "private-key.pem"
#else
""
#endif
};

#if CONFIG_MQTT_HELPER_SECONDARY_SEC_TAG != -1

static const unsigned char ca_certificate_2[] = {
#if __has_include("ca-cert-2.pem")
#include "ca-cert-2.pem"
#else
""
#endif
};

static const unsigned char private_key_2[] = {
#if __has_include("private-key-2.pem")
#include "private-key-2.pem"
#else
""
#endif
};

static const unsigned char device_certificate_2[] = {
#if __has_include("client-cert-2.pem")
#include "client-cert-2.pem"
#else
""
#endif
};

#endif /* CONFIG_MQTT_HELPER_SECONDARY_SEC_TAG != -1 */

static int credentials_provision(void)
{
	int err = 0;

	if (sizeof(ca_certificate) > 1) {
		err = modem_key_mgmt_write(CONFIG_MQTT_HELPER_SEC_TAG,
					   MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
					   ca_certificate,
					   sizeof(ca_certificate) - 1);
		if (err) {
			return err;
		}
	}

	if (sizeof(device_certificate) > 1) {
		err = modem_key_mgmt_write(CONFIG_MQTT_HELPER_SEC_TAG,
					   MODEM_KEY_MGMT_CRED_TYPE_PUBLIC_CERT,
					   device_certificate,
					   sizeof(device_certificate) - 1);
		if (err) {
			return err;
		}
	}

	if (sizeof(private_key) > 1) {
		err = modem_key_mgmt_write(CONFIG_MQTT_HELPER_SEC_TAG,
					   MODEM_KEY_MGMT_CRED_TYPE_PRIVATE_CERT,
					   private_key,
					   sizeof(private_key) - 1);
		if (err) {
			return err;
		}
	}

	/* Secondary security tag entries. */

#if CONFIG_MQTT_HELPER_SECONDARY_SEC_TAG != -1

	if (sizeof(ca_certificate_2) > 1) {
		err = modem_key_mgmt_write(CONFIG_MQTT_HELPER_SECONDARY_SEC_TAG,
					   MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
					   ca_certificate_2,
					   sizeof(ca_certificate_2) - 1);
		if (err) {
			return err;
		}
	}

	if (sizeof(device_certificate_2) > 1) {
		err = modem_key_mgmt_write(CONFIG_MQTT_HELPER_SECONDARY_SEC_TAG,
					   MODEM_KEY_MGMT_CRED_TYPE_PUBLIC_CERT,
					   device_certificate_2,
					   sizeof(device_certificate_2) - 1);
		if (err) {
			return err;
		}
	}

	if (sizeof(private_key_2) > 1) {
		err = modem_key_mgmt_write(CONFIG_MQTT_HELPER_SECONDARY_SEC_TAG,
					   MODEM_KEY_MGMT_CRED_TYPE_PRIVATE_CERT,
					   private_key_2,
					   sizeof(private_key_2) - 1);
		if (err) {
			return err;
		}
	}

#endif /* CONFIG_MQTT_HELPER_SECONDARY_SEC_TAG != -1 */

	return err;
}

NRF_MODEM_LIB_ON_INIT(mqtt_sample_init_hook, on_modem_lib_init, NULL);

static void on_modem_lib_init(int ret, void *ctx)
{
	credentials_provision();
}
