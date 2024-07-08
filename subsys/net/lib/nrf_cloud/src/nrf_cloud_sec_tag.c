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
