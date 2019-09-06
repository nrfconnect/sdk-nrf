/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "click_detector.h"

/* This configuration file is included only once from click_detector module
 * and holds information about click detector configuration.
 */

/* This structure enforces the header file is included only once in the build.
 * Violating this requirement triggers a multiple definition error at link time.
 */
const struct {} click_detector_def_include_once;

static const struct click_detector_config click_detector_config[] = {
	{
		.key_id = 0x0007,
		.consume_button_event = true,
	},
};
