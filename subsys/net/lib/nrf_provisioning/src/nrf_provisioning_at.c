/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <errno.h>
#include <zephyr/logging/log.h>

#include <nrf_modem_at.h>

#include "nrf_provisioning_at.h"

#define AT_ATT_CMD "AT%ATTESTTOKEN"
#define GET_TIME_CMD "AT%%CCLK?"

LOG_MODULE_REGISTER(nrf_provisioning_at, CONFIG_NRF_PROVISIONING_LOG_LEVEL);

int nrf_provisioning_at_attest_token_get(void *const buff, size_t size)
{
	if (size < CONFIG_NRF_PROVISIONING_AT_ATTESTTOKEN_MAX_LEN) {
		return -ENOMEM;
	}

	int ret = nrf_modem_at_scanf(AT_ATT_CMD, "%%ATTESTTOKEN: \"%255[^\"]\"", buff);

	/* One match is expected */
	return ret == 1 ? 0 : ret;
}

int nrf_provisioning_at_time_get(void *const time_buff, size_t size)
{
	return nrf_modem_at_cmd(time_buff, size, "AT%%CCLK?");
}

bool nrf_provisioning_at_cmee_is_active(void)
{
	int err;
	int active;

	err = nrf_modem_at_scanf("AT+CMEE?", "+CMEE: %d", &active);
	if (err < 0) {
		LOG_WRN("Failed to retrieve CMEE status, err %d", err);
		return false;
	}

	return active ? true : false;
}

int nrf_provisioning_at_cmee_control(enum nrf_provisioning_at_cmee_state state)
{
	return nrf_modem_at_printf("AT+CMEE=%d", state);
}

bool nrf_provisioning_at_cmee_enable(void)
{
	bool ret;

	if (!nrf_provisioning_at_cmee_is_active()) {
		ret = false;
		nrf_provisioning_at_cmee_control(NRF_PROVISIONING_AT_CMEE_STATE_ENABLE);
	} else {
		ret = true;
	}

	return ret;
}

int nrf_provisioning_at_cmd(void *resp, size_t resp_sz, const char *cmd)
{
	return nrf_modem_at_cmd(resp, resp_sz, "%s", cmd);
}
