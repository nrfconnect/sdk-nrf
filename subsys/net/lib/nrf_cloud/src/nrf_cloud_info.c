/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <net/nrf_cloud.h>
#if defined(CONFIG_MODEM_JWT)
#include <modem/modem_jwt.h>
#endif
#include "nrf_cloud_transport.h"
#include "nrf_cloud_client_id.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(nrf_cloud_info, CONFIG_NRF_CLOUD_LOG_LEVEL);

#if defined(CONFIG_NRF_MODEM_LIB)
#include <nrf_modem_at.h>
int nrf_cloud_get_imei(char *buf, size_t buf_sz)
{
	int err;
	char *eol;

	err = nrf_modem_at_cmd(buf, buf_sz, "AT+CGSN");
	if (err) {
		return err;
	}

	eol = strstr(buf, "\r\n");
	if (eol) {
		*eol = '\0';
	}
	return 0;
}

int nrf_cloud_get_uuid(char *buf, size_t buf_sz)
{
#if defined(CONFIG_MODEM_JWT)
	struct nrf_device_uuid dev_id;
	int err;

	if (buf == NULL) {
		return -EINVAL;
	}
	if (buf_sz <= NRF_DEVICE_UUID_STR_LEN) {
		return -ENOMEM;
	}

	err = modem_jwt_get_uuids(&dev_id, NULL);
	if (!err) {
		strncpy(buf, dev_id.str, buf_sz);
	}
	return err;
#else
	return -ENOTSUP;
#endif /* CONFIG_MODEM_JWT */
}

int nrf_cloud_get_modem_fw(char *buf, size_t buf_sz)
{
	int err;
	char *eol;

	err = nrf_modem_at_cmd(buf, buf_sz, "AT+CGMR");
	if (err) {
		return err;
	}

	eol = strstr(buf, "\r\n");
	if (eol) {
		*eol = '\0';
	}
	return 0;
}

#else
int nrf_cloud_get_imei(char *buf, size_t buf_sz)
{
	return -ENOTSUP;
}

int nrf_cloud_get_uuid(char *buf, size_t buf_sz)
{
	return -ENOTSUP;
}

int nrf_cloud_get_modem_fw(char *buf, size_t buf_sz)
{
	return -ENOTSUP;
}
#endif /* CONFIG_NRF_MODEM_LIB */

int nrf_cloud_print_details(void)
{
	int err;
	const char *device_id;

	err = nrf_cloud_client_id_ptr_get(&device_id);
	if (!err) {
		LOG_INF("Device ID: %s", device_id);
	} else {
		LOG_ERR("Error requesting the device ID: %d", err);
	}

#if defined(CONFIG_NRF_CLOUD_VERBOSE_DETAILS)
	const char *protocol = "Unknown";
	const char *host_name = "Unknown";
	char buf[100];

	err = nrf_cloud_get_imei(buf, sizeof(buf));
	if (!err) {
		LOG_INF("IMEI:      %s", buf);
	} else {
		LOG_ERR("Error requesting the IMEI: %d", err);
	}
	err = nrf_cloud_get_uuid(buf, sizeof(buf));
	if (!err) {
		LOG_INF("UUID:      %s", buf);
	} else {
		LOG_ERR("Error requesting the UUID: %d", err);
	}
	err = nrf_cloud_get_modem_fw(buf, sizeof(buf));
	if (!err) {
		LOG_INF("Modem FW:  %s", buf);
	} else {
		LOG_ERR("Error requesting the modem fw version: %d", err);
	}

#if defined(CONFIG_NRF_CLOUD_MQTT)
	protocol = "MQTT";
	host_name = CONFIG_NRF_CLOUD_HOST_NAME;
#elif defined(CONFIG_NRF_CLOUD_COAP)
	protocol = "CoAP";
	host_name = CONFIG_NRF_CLOUD_COAP_SERVER_HOSTNAME;
#elif defined(CONFIG_NRF_CLOUD_REST)
	protocol = "REST";
	host_name = CONFIG_NRF_CLOUD_REST_HOST_NAME;
#endif
	LOG_INF("Protocol:  %s", protocol);
	LOG_INF("Sec tag:   %d", nrf_cloud_sec_tag_get());
	LOG_INF("Host name: %s", host_name);

	/* Tenant will not be known unless device is connected to cloud with MQTT. */
#if defined(CONFIG_NRF_CLOUD_MQTT)
	char stage[NRF_CLOUD_STAGE_ID_MAX_LEN];
	char tenant[NRF_CLOUD_TENANT_ID_MAX_LEN];

	err = nct_stage_get(stage, sizeof(stage));
	if (!err) {
		LOG_INF("Stage:     %s", stage);
	} else {
		LOG_ERR("Error determining nRF Cloud stage: %d", err);
	}
	err = nct_tenant_id_get(tenant, sizeof(tenant));
	if (!err) {
		LOG_INF("Team ID:    %s", tenant);
	} else {
		LOG_ERR("Error determining team ID: %d", err);
	}
#endif
#endif /* CONFIG_NRF_CLOUD_VERBOSE_DETAILS */

	return err;
}
