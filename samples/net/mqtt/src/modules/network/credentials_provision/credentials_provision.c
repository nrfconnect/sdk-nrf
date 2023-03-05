/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <modem/modem_key_mgmt.h>

/* Declare log module */
LOG_MODULE_DECLARE(transport, CONFIG_MQTT_SAMPLE_TRANSPORT_LOG_LEVEL);

static const unsigned char ca_certificate[] = {
#if __has_include(CONFIG_MQTT_SAMPLE_NETWORK_CERTIFICATE_FILE)
#include CONFIG_MQTT_SAMPLE_NETWORK_CERTIFICATE_FILE
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

int credentials_provision(void)
{
	int err = 0;
  LOG_DBG("Provision credentials");

	if (sizeof(ca_certificate) > 1) {
    LOG_DBG("Provision CA certificate '%s': %.64s", CONFIG_MQTT_SAMPLE_NETWORK_CERTIFICATE_FILE, ca_certificate);
		err = modem_key_mgmt_write(CONFIG_MQTT_HELPER_SEC_TAG,
					   MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
					   ca_certificate,
					   sizeof(ca_certificate) - 1);
		if (err) {
			return err;
		}
	}

	if (sizeof(device_certificate) > 1) {
    LOG_DBG("Provision device certificate");
		err = modem_key_mgmt_write(CONFIG_MQTT_HELPER_SEC_TAG,
					   MODEM_KEY_MGMT_CRED_TYPE_PUBLIC_CERT,
					   device_certificate,
					   sizeof(device_certificate) - 1);
		if (err) {
			return err;
		}
	}

	if (sizeof(private_key) > 1) {
    LOG_DBG("Provision private key");
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
    LOG_DBG("Provision secondary CA certificate");
		err = modem_key_mgmt_write(CONFIG_MQTT_HELPER_SECONDARY_SEC_TAG,
					   MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
					   ca_certificate_2,
					   sizeof(ca_certificate_2) - 1);
		if (err) {
			return err;
		}
	}

	if (sizeof(device_certificate_2) > 1) {
    LOG_DBG("Provision secondary device certificate");
		err = modem_key_mgmt_write(CONFIG_MQTT_HELPER_SECONDARY_SEC_TAG,
					   MODEM_KEY_MGMT_CRED_TYPE_PUBLIC_CERT,
					   device_certificate_2,
					   sizeof(device_certificate_2) - 1);
		if (err) {
			return err;
		}
	}

	if (sizeof(private_key_2) > 1) {
    LOG_DBG("Provision secondary private key");
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
