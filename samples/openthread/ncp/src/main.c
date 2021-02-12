/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(ncp_sample, CONFIG_OT_NCP_LOG_LEVEL);

#define WELCOME_TEXT \
	"\n\r"\
	"\n\r"\
	"=================================================\n\r"\
	"OpenThread NCP application is now running on NCS.\n\r"\
	"=================================================\n\r"


void main(void)
{
	LOG_INF(WELCOME_TEXT);
}
