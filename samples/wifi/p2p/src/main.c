/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Wi-Fi P2P sample - CLI (client) or GO (group owner) mode.
 */

#include <zephyr/sys/util.h>
#include "p2p.h"

int main(void)
{

	if (IS_ENABLED(CONFIG_SAMPLE_P2P_CLI_MODE)) {
		p2p_cli_run();
	}

	return 0;
}
