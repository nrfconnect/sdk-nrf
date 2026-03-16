/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Wi-Fi P2P sample - CLI (client) or GO (group owner) mode.
 */

#include "p2p.h"

int main(void)
{

#ifdef CONFIG_P2P_SAMPLE_CLI_MODE
	p2p_cli_run();
#elif CONFIG_P2P_SAMPLE_GO_MODE
	p2p_go_run();
#endif

	return 0;
}
