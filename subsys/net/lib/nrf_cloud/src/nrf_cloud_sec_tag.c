/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <net/nrf_cloud.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(nrf_cloud_sec_tag, CONFIG_NRF_CLOUD_LOG_LEVEL);

static sec_tag_t nrf_cloud_sec_tag =
#if defined(CONFIG_NRF_CLOUD_COAP)
	CONFIG_NRF_CLOUD_COAP_SEC_TAG;
#else
	CONFIG_NRF_CLOUD_SEC_TAG;
#endif

void nrf_cloud_sec_tag_set(const sec_tag_t sec_tag)
{
	nrf_cloud_sec_tag = sec_tag;
	LOG_DBG("Sec tag updated: %d", nrf_cloud_sec_tag);
}

sec_tag_t nrf_cloud_sec_tag_get(void)
{
	return nrf_cloud_sec_tag;
}

#if defined(CONFIG_NRF_CLOUD_COAP)
static sec_tag_t coap_jwt_sec_tag = CONFIG_NRF_CLOUD_COAP_JWT_SEC_TAG;

void nrf_cloud_sec_tag_coap_jwt_set(const sec_tag_t sec_tag)
{
	coap_jwt_sec_tag = sec_tag;
	LOG_DBG("CoAP JWT sec tag updated: %d", coap_jwt_sec_tag);
}

sec_tag_t nrf_cloud_sec_tag_coap_jwt_get(void)
{
	return coap_jwt_sec_tag;
}
#endif /* CONFIG_NRF_CLOUD_COAP */
