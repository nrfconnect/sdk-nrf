/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <mpsl_fem_utils.h>

#if defined(CONFIG_MPSL_FEM_PIN_FORWARDER)

#include <stdbool.h>
#include <string.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/__assert.h>

void mpsl_fem_pin_extend_with_port(uint8_t *pin, const char *lbl)
{
	/* The pin numbering in the FEM configuration API follows the
	 * convention:
	 *   pin numbers 0..31 correspond to the gpio0 port
	 *   pin numbers 32..63 correspond to the gpio1 port
	 *
	 * Values obtained from devicetree are here adjusted to the ranges
	 * given above.
	 */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio0), okay)
	if (strcmp(lbl, DT_LABEL(DT_NODELABEL(gpio0))) == 0) {
		return;
	}
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio1), okay)
	if (strcmp(lbl, DT_LABEL(DT_NODELABEL(gpio1))) == 0) {
		*pin += 32;
		return;
	}
#endif
	(void)pin;

	__ASSERT(false, "Unknown GPIO port DT label");
}

#endif /* defined(CONFIG_MPSL_FEM_PIN_FORWARDER) */
