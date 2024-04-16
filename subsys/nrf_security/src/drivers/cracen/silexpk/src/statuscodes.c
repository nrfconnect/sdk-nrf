/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <silexpk/core.h>
#include <zephyr/sys/util.h>

static const char * const descriptions[] = {
	"OK",
	"Invalid parameter",
	"Unknown error",
	"Busy",
	"Non quadratic residue",
	"Composite value",
	"Not invertible",
	"Invalid signature",
	"Not implemented",
	"Point at infinity",
	"Out of range",
	"Invalid modulus (must be odd)",
	"Point not on curve",
	"Operand too large",
	"Platform error",
	"Expired",
	"Still in IK mode",
	"Invalid elliptic curve parameters",
	"IK not ready. Please initialize IK first",
	"Resources not available for a new operation. Retry later",
};

const char *sx_describe_statuscode(unsigned int code)
{
	if (code >= ARRAY_SIZE(descriptions)) {
		return "Invalid status code";
	}
	return descriptions[code];
}
