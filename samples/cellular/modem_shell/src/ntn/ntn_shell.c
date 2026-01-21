/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <zephyr/shell/shell.h>
#include <modem/ntn.h>

#include "mosh_print.h"
#include "ntn.h"

static int cmd_location_enable(const struct shell *shell, size_t argc, char **argv)
{
	uint32_t interval;
	int err = 0;

	interval = shell_strtoul(argv[1], 10, &err);
	if (err != 0) {
		mosh_error("enable: invalid interval value %s", argv[1]);
		return -EINVAL;
	}

	return ntn_location_update_enable(interval);
}

static int cmd_location_disable(const struct shell *shell, size_t argc, char **argv)
{
	return ntn_location_update_disable();
}

static int cmd_location_set(const struct shell *shell, size_t argc, char **argv)
{
	double latitude;
	double longitude;
	double altitude;
	uint32_t validity;
	char *endptr;
	int err;

	latitude = strtod(argv[1], &endptr);
	if (*endptr != '\0') {
		mosh_error("set: invalid latitude value %s", argv[1]);
		return -EINVAL;
	}

	longitude = strtod(argv[2], &endptr);
	if (*endptr != '\0') {
		mosh_error("set: invalid longitude value %s", argv[2]);
		return -EINVAL;
	}

	altitude = strtod(argv[3], &endptr);
	if (*endptr != '\0') {
		mosh_error("set: invalid altitude value %s", argv[3]);
		return -EINVAL;
	}

	err = 0;
	validity = shell_strtoul(argv[4], 10, &err);
	if (err != 0) {
		mosh_error("set: invalid validity value %s", argv[4]);
		return -EINVAL;
	}

	err = ntn_location_set(latitude, longitude, altitude, validity);
	if (err != 0) {
		mosh_error("set: failed to set location (err %d)", err);
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_location_invalidate(const struct shell *shell, size_t argc, char **argv)
{
	int err;

	err = ntn_location_invalidate();
	if (err != 0) {
		mosh_error("invalidate: failed to invalidate location (err %d)", err);
		return -ENOEXEC;
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_location,
	SHELL_CMD_ARG(enable, NULL,
		      "<interval (s)>\nEnable periodic modem location updates using external GNSS.",
		      cmd_location_enable, 2, 0),
	SHELL_CMD_ARG(disable, NULL,
		      "Disable periodic modem location updates using external GNSS.",
		      cmd_location_disable, 1, 0),
	SHELL_CMD_ARG(set, NULL,
		      "<latitude> <longitude> <altitude (m)> <validity (s)>\nSet location.",
		      cmd_location_set, 5, 0),
	SHELL_CMD_ARG(invalidate, NULL,
		      "Invalidate location.",
		      cmd_location_invalidate, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_ntn,
	SHELL_CMD(location, &sub_location,
		  "Modem location updates.", NULL),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(ntn, &sub_ntn, "Non-Terrestrial Network (NTN) commands.", NULL);
