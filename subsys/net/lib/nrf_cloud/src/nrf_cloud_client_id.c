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
BUILD_ASSERT((HW_ID_LEN - 1) <= NRF_CLOUD_CLIENT_ID_MAX_LEN,
	"HW_ID_LEN must not exceed NRF_CLOUD_CLIENT_ID_MAX_LEN");
#endif

/* Semaphore for client ID buffer access */
static K_SEM_DEFINE(client_id_sem, 1, 1);
/* Null-terminated nRF Cloud device/client ID string*/
static char client_id_buf[NRF_CLOUD_CLIENT_ID_MAX_LEN + 1];
/* Client ID string length */
static size_t client_id_len;

static int configured_client_id_init(void)
{
	int err;
	int print_ret;
	const size_t buf_sz = sizeof(client_id_buf);

	(void)k_sem_take(&client_id_sem, K_FOREVER);

#if defined(CONFIG_NRF_CLOUD_CLIENT_ID_SRC_IMEI)
	char imei_buf[CGSN_RESPONSE_LENGTH];

	err = nrf_modem_at_cmd(imei_buf, sizeof(imei_buf), "AT+CGSN");
	if (err) {
		LOG_ERR("Failed to obtain IMEI, error: %d", err);
		goto cleanup;
	}

	imei_buf[NRF_IMEI_LEN] = 0;

	print_ret = snprintk(client_id_buf, buf_sz, "%s%.*s",
			     CONFIG_NRF_CLOUD_CLIENT_ID_PREFIX,
			     NRF_IMEI_LEN, imei_buf);

#elif defined(CONFIG_NRF_CLOUD_CLIENT_ID_SRC_INTERNAL_UUID)
	struct nrf_device_uuid dev_id;

	err = modem_jwt_get_uuids(&dev_id, NULL);
	if (err) {
		LOG_ERR("Failed to get device UUID: %d", err);
		goto cleanup;
	}

	print_ret = snprintk(client_id_buf, buf_sz, "%s", dev_id.str);

#elif defined(CONFIG_NRF_CLOUD_CLIENT_ID_SRC_HW_ID)
	char hw_id_buf[HW_ID_LEN];

	err = hw_id_get(hw_id_buf, ARRAY_SIZE(hw_id_buf));
	if (err) {
		LOG_ERR("Failed to obtain hardware ID, error: %d", err);
		goto cleanup;
	}
	print_ret = snprintk(client_id_buf, buf_sz, "%s", hw_id_buf);

#elif defined(CONFIG_NRF_CLOUD_CLIENT_ID_SRC_COMPILE_TIME)
	print_ret = snprintk(client_id_buf, buf_sz, "%s", CONFIG_NRF_CLOUD_CLIENT_ID);
#else
	if (IS_ENABLED(CONFIG_NRF_CLOUD_CLIENT_ID_SRC_RUNTIME)) {
		LOG_WRN("Configured for runtime client ID");
	} else {
		LOG_WRN("Unhandled client ID configuration");
	}

	err = -ENODEV;
	goto cleanup;
#endif

	if (print_ret <= 0) {
		err = -EIO;
		goto cleanup;
	} else if (print_ret >= buf_sz) {
		err = -EMSGSIZE;
		goto cleanup;
	}

	/* On success, set the ID length */
	client_id_len = print_ret;
	err = 0;

cleanup:
	k_sem_give(&client_id_sem);

	return err;
}

static int client_id_init(void)
{
	if (IS_ENABLED(CONFIG_NRF_CLOUD_CLIENT_ID_SRC_RUNTIME) && (!client_id_len)) {
		LOG_ERR("Runtime client ID has not been set");
		return -ENXIO;
	}

	if (!client_id_len) {
		int err = configured_client_id_init();

		if (err) {
			LOG_ERR("Failed to initialize client ID, error: %d", err);
			return -EIO;
		}
	}

	return 0;
}

int nrf_cloud_client_id_ptr_get(const char **client_id)
{
	if (!client_id) {
		return -EINVAL;
	}

	int err = client_id_init();

	if (!err) {
		*client_id = client_id_buf;
	}

	return err;
}

int nrf_cloud_client_id_get(char *id_buf, size_t id_buf_sz)
{
	if (!id_buf || (id_buf_sz == 0)) {
		return -EINVAL;
	}

	int err = client_id_init();

	if (!err) {
		(void)k_sem_take(&client_id_sem, K_FOREVER);

		const size_t reqd_sz = client_id_len + 1;

		if (id_buf_sz < reqd_sz) {
			LOG_ERR("Provided client ID buffer is too small, required size: %d ",
				reqd_sz);
			err = -EMSGSIZE;
		} else {
			memcpy(id_buf, client_id_buf, reqd_sz);
		}

		(void)k_sem_give(&client_id_sem);
	}

	return err;
}

int nrf_cloud_client_id_runtime_set(const char *const client_id)
{
	__ASSERT_NO_MSG(client_id != NULL);
	__ASSERT(IS_ENABLED(CONFIG_NRF_CLOUD_CLIENT_ID_SRC_RUNTIME),
		 "CONFIG_NRF_CLOUD_CLIENT_ID_SRC_RUNTIME is not enabled");

	size_t len = strlen(client_id);

	if (len > NRF_CLOUD_CLIENT_ID_MAX_LEN) {
		LOG_ERR("Client ID length %u exceeds max %u", len, NRF_CLOUD_CLIENT_ID_MAX_LEN);
		return -EINVAL;
	} else if (len == 0) {
		LOG_ERR("Invalid client ID; empty string");
		return -ENODATA;
	}

	(void)k_sem_take(&client_id_sem, K_FOREVER);

	client_id_len = len;
	memcpy(client_id_buf, client_id, client_id_len + 1);

	(void)k_sem_give(&client_id_sem);
	return 0;
}
