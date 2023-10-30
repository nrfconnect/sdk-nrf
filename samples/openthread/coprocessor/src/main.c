/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#if defined(CONFIG_RCP_SAMPLE_HCI)
#include "rcp_hci.h"
#endif

LOG_MODULE_REGISTER(coprocessor_sample, CONFIG_OT_COPROCESSOR_LOG_LEVEL);

#define WELCOME_TEXT                                                           \
	"\n\r"                                                                 \
	"\n\r"                                                                 \
	"=========================================================\n\r"        \
	"OpenThread Coprocessor application is now running on NCS.\n\r"        \
	"=========================================================\n\r"

int main(void)
{
	LOG_INF(WELCOME_TEXT);

#if defined(CONFIG_RCP_SAMPLE_HCI)
	run_hci();
#endif

	return 0;
}
