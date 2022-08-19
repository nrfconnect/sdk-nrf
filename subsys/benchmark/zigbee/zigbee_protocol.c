/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "protocol_api.h"
#include <zboss_api.h>
#include <zigbee/zigbee_error_handler.h>

bool protocol_is_error(uint32_t error_code)
{
	return (zb_ret_t)error_code != RET_OK;
}

void protocol_init(void)
{
}

void protocol_process(void)
{
}

void protocol_sleep(void)
{
}

void protocol_soc_evt_handler(uint32_t soc_evt)
{
}
