/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <net/nrf_cloud.h>
#if defined(CONFIG_NRF_CLOUD_CLIENT_ID_SRC_INTERNAL_UUID)
#include <modem/modem_jwt.h>
#endif
#if defined(CONFIG_NRF_CLOUD_CLIENT_ID_SRC_IMEI)
#include <nrf_modem_at.h>
#endif
#if defined(CONFIG_NRF_CLOUD_CLIENT_ID_SRC_HW_ID)
#include <hw_id.h>
#endif
#include <zephyr/kernel.h>
#include <stdio.h>
#include <zephyr/logging/log.h>
#include "nrf_cloud_client_id.h"
#include "nrf_cloud_transport.h"

LOG_MODULE_REGISTER(nrf_cloud_client_id, CONFIG_NRF_CLOUD_LOG_LEVEL);

#if defined(CONFIG_NRF_CLOUD_CLIENT_ID_SRC_COMPILE_TIME)
BUILD_ASSERT((sizeof(CONFIG_NRF_CLOUD_CLIENT_ID) - 1) <= NRF_CLOUD_CLIENT_ID_MAX_LEN,
	"CONFIG_NRF_CLOUD_CLIENT_ID must not exceed NRF_CLOUD_CLIENT_ID_MAX_LEN");
BUILD_ASSERT(sizeof(CONFIG_NRF_CLOUD_CLIENT_ID) > 1,
	"CONFIG_NRF_CLOUD_CLIENT_ID must not be empty");
#endif

#if defined(CONFIG_NRF_CLOUD_CLIENT_ID_SRC_IMEI)
#define NRF_IMEI_LEN 15
#define CGSN_RESPONSE_LENGTH (NRF_IMEI_LEN + 6 + 1) /* Add 6 for \r\nOK\r\n and 1 for \0 */
#define IMEI_CLIENT_ID_LEN (sizeof(CONFIG_NRF_CLOUD_CLIENT_ID_PREFIX) - 1 + NRF_IMEI_LEN)
BUILD_ASSERT(IMEI_CLIENT_ID_LEN <= NRF_CLOUD_CLIENT_ID_MAX_LEN,
	"NRF_CLOUD_CLIENT_ID_PREFIX plus IMEI must not exceed NRF_CLOUD_CLIENT_ID_MAX_LEN");
#endif

#if defined(CONFIG_NRF_CLOUD_CLIENT_ID_SRC_HW_ID)
BUILD_ASSERT((sizeof(HW_ID_LEN) - 1) <= NRF_CLOUD_CLIENT_ID_MAX_LEN,
	"HW_ID_LEN must not exceed NRF_CLOUD_CLIENT_ID_MAX_LEN");
#endif

int nrf_cloud_client_id_get(char *id_buf, size_t id_len)
{
	int ret;

#if defined(CONFIG_NRF_CLOUD_MQTT)
	/* For MQTT, the client ID is allocated and generated when nrf_cloud_init is called,
	 * so just get a copy.
	 */
	ret = nct_client_id_get(id_buf, id_len);
#else
	ret = -ENODEV;
#endif

#if defined(CONFIG_NRF_CLOUD_REST) || defined(CONFIG_NRF_CLOUD_COAP)
	if (ret != 0) {
		/* For REST and CoAP, the client ID is generated on demand. */
		ret = nrf_cloud_configured_client_id_get(id_buf, id_len);
	}
#endif

	return ret;
}

size_t nrf_cloud_configured_client_id_length_get(void)
{
#if defined(CONFIG_NRF_CLOUD_CLIENT_ID_SRC_IMEI)
	return IMEI_CLIENT_ID_LEN;
#elif defined(CONFIG_NRF_CLOUD_CLIENT_ID_SRC_INTERNAL_UUID)
	return NRF_DEVICE_UUID_STR_LEN;
#elif defined(CONFIG_NRF_CLOUD_CLIENT_ID_SRC_COMPILE_TIME)
	return (sizeof(CONFIG_NRF_CLOUD_CLIENT_ID) - 1);
#elif defined(CONFIG_NRF_CLOUD_CLIENT_ID_SRC_HW_ID)
	return HW_ID_LEN - 1;
#else
	return 0;
#endif
}

int nrf_cloud_configured_client_id_get(char * const buf, const size_t buf_sz)
{
	if (!buf || !buf_sz) {
		return -EINVAL;
	}

	int err;
	int print_ret;

#if defined(CONFIG_NRF_CLOUD_CLIENT_ID_SRC_IMEI)
	char imei_buf[CGSN_RESPONSE_LENGTH];

	err = nrf_modem_at_cmd(imei_buf, sizeof(imei_buf), "AT+CGSN");
	if (err) {
		LOG_ERR("Failed to obtain IMEI, error: %d", err);
		return err;
	}

	imei_buf[NRF_IMEI_LEN] = 0;

	print_ret = snprintk(buf, buf_sz, "%s%.*s",
			     CONFIG_NRF_CLOUD_CLIENT_ID_PREFIX,
			     NRF_IMEI_LEN, imei_buf);

#elif defined(CONFIG_NRF_CLOUD_CLIENT_ID_SRC_INTERNAL_UUID)
	struct nrf_device_uuid dev_id;

	err = modem_jwt_get_uuids(&dev_id, NULL);
	if (err) {
		LOG_ERR("Failed to get device UUID: %d", err);
		return err;
	}

	print_ret = snprintk(buf, buf_sz, "%s", dev_id.str);

#elif defined(CONFIG_NRF_CLOUD_CLIENT_ID_SRC_HW_ID)
	char hw_id_buf[HW_ID_LEN];

	err = hw_id_get(hw_id_buf, ARRAY_SIZE(hw_id_buf));
	if (err) {
		LOG_ERR("Failed to obtain hardware ID, error: %d", err);
		return err;
	}
	print_ret = snprintk(buf, buf_sz, "%s", hw_id_buf);

#elif defined(CONFIG_NRF_CLOUD_CLIENT_ID_SRC_COMPILE_TIME)
	ARG_UNUSED(err);
	print_ret = snprintk(buf, buf_sz, "%s", CONFIG_NRF_CLOUD_CLIENT_ID);
#else
	ARG_UNUSED(err);
	if (IS_ENABLED(CONFIG_NRF_CLOUD_CLIENT_ID_SRC_RUNTIME)) {
		LOG_WRN("Configured for runtime client ID");
	} else {
		LOG_WRN("Unhandled client ID configuration");
	}
	return -ENODEV;
#endif

	if (print_ret <= 0) {
		return -EIO;
	} else if (print_ret >= buf_sz) {
		return -EMSGSIZE;
	}

	return 0;
}
