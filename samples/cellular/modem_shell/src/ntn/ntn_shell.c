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

static int cmd_modem_location_enable(const struct shell *shell, size_t argc, char **argv)
{
	return ntn_modem_location_update_enable();
}

static int cmd_modem_location_disable(const struct shell *shell, size_t argc, char **argv)
{
	return ntn_modem_location_update_disable();
}

static int cmd_location_set(const struct shell *shell, size_t argc, char **argv)
{
	double latitude;
	double longitude;
	double altitude;
	uint32_t validity;
	char *endptr;
	int err;

	if (argc < 5) {
		mosh_error("set: missing arguments. Usage: ntn modem_location set "
			   "<latitude> <longitude> <altitude (m)> <validity (s)>");
		return -EINVAL;
	}

	latitude = strtod(argv[1], &endptr);
	if (*endptr != '\0') {
		mosh_error("set: invalid latitude value: %s", argv[1]);
		return -EINVAL;
	}

	longitude = strtod(argv[2], &endptr);
	if (*endptr != '\0') {
		mosh_error("set: invalid longitude value: %s", argv[2]);
		return -EINVAL;
	}

	altitude = strtod(argv[3], &endptr);
	if (*endptr != '\0') {
		mosh_error("set: invalid altitude value: %s", argv[3]);
		return -EINVAL;
	}

	err = 0;
	validity = shell_strtoul(argv[4], 10, &err);
	if (err != 0) {
		mosh_error("set: invalid validity value: %s", argv[4]);
		return -EINVAL;
	}

	err = ntn_modem_location_set(latitude, longitude, altitude, validity);
	if (err != 0) {
		mosh_error("set: failed to set location (err %d)", err);
		return -ENOEXEC;
	}

	mosh_print("Location set: lat=%.6f, lon=%.6f, alt=%.1f, validity=%u",
		   latitude, longitude, altitude, validity);

	return 0;
}

static int cmd_location_invalidate(const struct shell *shell, size_t argc, char **argv)
{
	int err;

	err = ntn_modem_location_invalidate();
	if (err != 0) {
		mosh_error("invalidate: failed to invalidate location (err %d)", err);
		return -ENOEXEC;
	}

	mosh_print("Location invalidated");

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_modem_location,
	SHELL_CMD_ARG(enable, NULL,
		      "Enable periodic modem location updates using external GNSS.",
		      cmd_modem_location_enable, 1, 0),
	SHELL_CMD_ARG(disable, NULL,
		      "Disable periodic modem location updates using external GNSS.",
		      cmd_modem_location_disable, 1, 0),
	SHELL_CMD_ARG(set, NULL,
		      "Set location <latitude> <longitude> <altitude (m)> <validity (s)>.",
		      cmd_location_set, 5, 0),
	SHELL_CMD_ARG(invalidate, NULL,
		      "Invalidate location.",
		      cmd_location_invalidate, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_ntn,
	SHELL_CMD(modem_location, &sub_modem_location,
		  "Modem location updates.", NULL),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(ntn, &sub_ntn, "Non-Terrestrial Network (NTN) commands.", NULL);
