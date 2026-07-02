/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

/*
 * Idle image for the radio core. It mostly sleeps so the core stays idle,
 * which lets the whole SoC reach low power while the application core is idle.
 *
 * A dedicated image is used instead of the standard empty net-core image
 * (SB_CONFIG_NETCORE_EMPTY, whose main() returns and never runs again) so the
 * core wakes periodically. The periodic wake-up acts as a liveness signal,
 * showing the radio core is still alive rather than hung.
 */
int main(void)
{
	while (1) {
		k_sleep(K_SECONDS(2));
	}

	return 0;
}
