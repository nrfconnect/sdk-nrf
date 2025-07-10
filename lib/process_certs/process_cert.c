/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "process_cert.h"
#include <ctrl_iface_zephyr.h>
#include <supp_main.h>

struct wifi_enterprise_creds_params params;

static const char ca_cert_test[] = {
       #include <wifi_enterprise_test_certs/ca.pem.inc>
       '\0'
};

static const char client_cert_test[] = {
       #include <wifi_enterprise_test_certs/client.pem.inc>
       '\0'
};

static const char client_key_test[] = {
       #include <wifi_enterprise_test_certs/client-key.pem.inc>
       '\0'
};

static const char ca_cert2_test[] = {
       #include <wifi_enterprise_test_certs/ca2.pem.inc>
       '\0'};

static const char client_cert2_test[] = {
       #include <wifi_enterprise_test_certs/client2.pem.inc>
       '\0'};

static const char client_key2_test[] = {
       #include <wifi_enterprise_test_certs/client-key2.pem.inc>
       '\0'};

static const char server_cert_test[] = {
       #include <wifi_enterprise_test_certs/server.pem.inc>
       '\0'
};

static const char server_key_test[] = {
       #include <wifi_enterprise_test_certs/server-key.pem.inc>
       '\0'
};

static void set_enterprise_creds_params(bool is_ap)
{
	memset(&params, 0, sizeof(struct wifi_enterprise_creds_params));
	params.ca_cert = (uint8_t *)ca_cert_test;
	params.ca_cert_len = ARRAY_SIZE(ca_cert_test);

	if (!is_ap) {
		params.client_cert = (uint8_t *)client_cert_test;
		params.client_cert_len = ARRAY_SIZE(client_cert_test);
		params.client_key = (uint8_t *)client_key_test;
		params.client_key_len = ARRAY_SIZE(client_key_test);
		params.ca_cert2 = (uint8_t *)ca_cert2_test;
		params.ca_cert2_len = ARRAY_SIZE(ca_cert2_test);
		params.client_cert2 = (uint8_t *)client_cert2_test;
		params.client_cert2_len = ARRAY_SIZE(client_cert2_test);
		params.client_key2 = (uint8_t *)client_key2_test;
		params.client_key2_len = ARRAY_SIZE(client_key2_test);

		return;
	}

	params.server_cert = (uint8_t *)server_cert_test;
	params.server_cert_len = ARRAY_SIZE(server_cert_test);
	params.server_key = (uint8_t *)server_key_test;
	params.server_key_len = ARRAY_SIZE(server_key_test);
}

static int wifi_set_enterprise_creds(void)
{
	struct net_if *iface = net_if_get_first_wifi();

	set_enterprise_creds_params(0);
	if (net_mgmt(NET_REQUEST_WIFI_ENTERPRISE_CREDS, iface, &params, sizeof(params))) {
		printf("Set enterprise credentials failed\n");
		return -1;
	}

	return 0;
}

int process_certificates(void)
{
	struct wpa_supplicant *wpa_s;

	wpa_s = zephyr_get_handle_by_ifname(CONFIG_WFA_QT_DEFAULT_INTERFACE);
	if (!wpa_s) {
		printf("Unable to find the interface: %s, quitting", CONFIG_WFA_QT_DEFAULT_INTERFACE);
		return -1;
	}

	wifi_set_enterprise_creds();

	if (config_process_blob(wpa_s->conf, "ca_cert",
					params.ca_cert,
					params.ca_cert_len)) {
		return -1;
	}

	if (config_process_blob(wpa_s->conf, "client_cert",
					params.client_cert,
					params.client_cert_len)) {
		return -1;
	}

	if (config_process_blob(wpa_s->conf, "private_key",
					params.client_key,
					params.client_key_len)) {
		return -1;
	}

	return 0;
}

int config_process_blob(struct wpa_config *config, char *name, uint8_t *data,
				uint32_t data_len)
{
	struct wpa_config_blob *blob;

	if (!data || !data_len) {
		return -1;
	}

	blob = os_zalloc(sizeof(*blob));
	if (blob == NULL) {
		return -1;
	}

	blob->data = os_zalloc(data_len);
	if (blob->data == NULL) {
		os_free(blob);
		return -1;
	}

	blob->name = os_strdup(name);

	if (blob->name == NULL) {
		wpa_config_free_blob(blob);
		return -1;
	}

	os_memcpy(blob->data, data, data_len);
	blob->len = data_len;

	wpa_config_set_blob(config, blob);

	return 0;
}
