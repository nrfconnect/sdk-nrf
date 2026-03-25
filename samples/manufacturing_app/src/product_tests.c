/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * Step 9: End product tests.
 *
 * This step is intentionally left as a stub. Manufacturers should replace or
 * extend step9_tests_run() with their own end-of-line validation logic.
 *
 */

#include "product_tests.h"
#include "mfg_log.h"

void step9_tests_run(void)
{
	MFG_LOG_STEP("End product tests");

	MFG_LOG_INF("This is a good place to add code for end product automatic tests.\n");
	MFG_LOG_INF("If the device shall be tested manually, add code waiting for an external event\n");
	MFG_LOG_INF("(message via serial port, key press or whatever is suitable for your device).\n");
}
