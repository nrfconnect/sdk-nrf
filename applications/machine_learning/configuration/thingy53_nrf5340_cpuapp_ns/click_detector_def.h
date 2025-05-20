/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <caf/click_detector.h>

/* This configuration file is included only once from click_detector module
 * and holds information about click detector configuration.
 */

/* This structure enforces the header file is included only once in the build.
 * Violating this requirement triggers a multiple definition error at link time.
 */
const struct {} click_detector_def_include_once;

static const struct click_detector_config click_detector_config[] = {
	{
		.key_id = CONFIG_ML_APP_MODE_CONTROL_BUTTON_ID,
		.consume_button_event = true,
	},
};
