/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

#include <lwm2m_carrier.h>
#include <lwm2m_settings.h>
#include <shell/shell.h>

static int m_mem_free;

static int cmd_device_time_read(const struct shell *shell, size_t argc, char **argv)
{
	int32_t utc_time = 0;
	int utc_offset = 0;
	char *tz;

	if (argc != 1) {
		shell_print(shell, "%s does not accept arguments", argv[0]);
		return 0;
	}

	lwm2m_carrier_time_read(&utc_time, &utc_offset, (const char **)&tz);

	shell_print(shell, "UTC time is: %d", utc_time);
	shell_print(shell, "UTC offset is: %d", utc_offset);
	shell_print(shell, "Timezone is: %s", tz);

	return 0;
}

static int cmd_device_utc_time_read(const struct shell *shell, size_t argc, char **argv)
{
	int32_t utc_time = 0;

	if (argc != 1) {
		shell_print(shell, "%s does not accept arguments", argv[0]);
		return 0;
	}

	utc_time = lwm2m_carrier_utc_time_read();

	shell_print(shell, "UTC time is: %d", utc_time);
	return 0;
}

static int cmd_device_utc_offset_read(const struct shell *shell, size_t argc, char **argv)
{
	int utc_offset = 0;

	if (argc != 1) {
		shell_print(shell, "%s does not accept arguments", argv[0]);
		return 0;
	}

	utc_offset = lwm2m_carrier_utc_offset_read();

	shell_print(shell, "UTC offset is: %d", utc_offset);

	return 0;
}

static int cmd_device_timezone_read(const struct shell *shell, size_t argc, char **argv)
{
	char *tz;

	if (argc != 1) {
		shell_print(shell, "%s does not accept arguments", argv[0]);
		return 0;
	}

	tz = lwm2m_carrier_timezone_read();

	shell_print(shell, "Timezone is: %s", tz);

	return 0;
}

static int cmd_device_utc_time_write(const struct shell *shell, size_t argc, char **argv)
{
	int err = 0;
	int32_t time = 0;

	if (argc != 2) {
		shell_print(shell, "%s <time in seconds since epoc>", argv[0]);
		return 0;
	}
	time = atoi(argv[1]);

	err = lwm2m_carrier_utc_time_write(time);
	if (err != 0) {
		shell_print(shell, "Unable to set time to: %d", time);
	} else {
		shell_print(shell, "UTC time successfully set");
	}

	return 0;
}

static int cmd_device_utc_offset_write(const struct shell *shell, size_t argc, char **argv)
{
	int err = 0;
	int offset = 0;

	if (argc != 2) {
		shell_print(shell, "%s <offset in minutes>", argv[0]);
		return 0;
	}
	offset = atoi(argv[1]);

	err = lwm2m_carrier_utc_offset_write(offset);
	if (err != 0) {
		shell_print(shell, "Unable to set offset to: %d", offset);
	} else {
		shell_print(shell, "offset successfully set");
	}

	return 0;
}

static int cmd_device_timezone_write(const struct shell *shell, size_t argc, char **argv)
{
	int err = 0;

	if (argc != 2) {
		shell_print(shell, "%s <timezone>", argv[0]);
		return 0;
	}

	err = lwm2m_carrier_timezone_write(argv[1]);
	if (err != 0) {
		shell_print(shell, "Unable to set timezone to: %s", argv[1]);
	} else {
		shell_print(shell, "Timezone successfully set");
	}

	return 0;
}

static int cmd_device_power_sources_set(const struct shell *shell, size_t argc, char **argv)
{
	if (argc < 2) {
		shell_print(shell, " 0 = DC");
		shell_print(shell, " 1 = Internal battery");
		shell_print(shell, " 2 = External battery");
		shell_print(shell, " 4 = Ethernet");
		shell_print(shell, " 5 = USB");
		shell_print(shell, " 6 = AC");
		shell_print(shell, " 7 = Solar");
		return 0;
	}

	bool internal_battery = false;
	uint8_t power_source_count = argc - 1;
	uint8_t power_sources[power_source_count];

	for (int i = 0; i < power_source_count; i++) {
		power_sources[i] = (uint8_t)atoi(argv[i + 1]);
		if (power_sources[i] == LWM2M_CARRIER_POWER_SOURCE_INTERNAL_BATTERY) {
			internal_battery = true;
		}
	}

	switch (lwm2m_carrier_avail_power_sources_set(power_sources, power_source_count)) {
	case 0:
		shell_print(shell, "Available power sources set successfully");
		if (internal_battery) {
			lwm2m_carrier_battery_level_set(100);
			lwm2m_carrier_battery_status_set(LWM2M_CARRIER_BATTERY_STATUS_NORMAL);
			shell_print(shell, "Internal battery level set to 100 (normal)");
		}
		break;
	case -EINVAL:
		shell_print(shell, "Unsupported power source");
		break;
	case -E2BIG:
		shell_print(shell, "Unsupported number of power sources");
		break;
	case -ENOENT:
		shell_print(shell, "Object not initialized");
		break;
	default:
		shell_print(shell, "Error: %d", errno);
		break;
	};

	return 0;
}

static int cmd_device_voltage_set(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 3) {
		shell_print(shell,
			    "voltage_measurements <power source identifier> <voltage in mV>");
		return 0;
	}

	uint8_t power_source = (uint8_t)atoi(argv[1]);
	int32_t voltage = atoi(argv[2]);

	switch (lwm2m_carrier_power_source_voltage_set(power_source, voltage)) {
	case 0:
		shell_print(shell, "Voltage measurement updated successfully");
		break;
	case -ENODEV:
		shell_print(shell, "Power source not detected");
		break;
	case -EINVAL:
		shell_print(shell, "Unsupported power source type");
		break;
	default:
		shell_print(shell, "Error: %d", errno);
		break;
	};

	return 0;
}

static int cmd_device_current_set(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 3) {
		shell_print(shell,
			    "current_measurements <power source identifier> <current in mA>");
		return 0;
	}

	uint8_t power_source = (uint8_t)atoi(argv[1]);
	int32_t current = atoi(argv[2]);

	switch (lwm2m_carrier_power_source_current_set(power_source, current)) {
	case 0:
		shell_print(shell, "Current measurements updated successfully");
		break;
	case -ENODEV:
		shell_print(shell, "Power source not detected");
		break;
	case -EINVAL:
		shell_print(shell, "Unsupported power source type");
		break;
	default:
		shell_print(shell, "Error: %d", errno);
		break;
	};

	return 0;
}


static int cmd_device_battery_level_set(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_print(shell, "battery_level <battery level %%>");
		return 0;
	}

	switch (lwm2m_carrier_battery_level_set(atoi(argv[1]))) {
	case 0:
		shell_print(shell, "Battery level updated successfully");
		break;
	case -EINVAL:
		shell_print(shell, "Invalid value: %d", atoi(argv[1]));
		break;
	case -ENODEV:
		shell_print(shell, "No internal battery detected");
		break;
	default:
		shell_print(shell, "Error: %d", errno);
		break;
	};

	return 0;
}

static int cmd_device_battery_status_set(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_print(shell, " 0 = Normal");
		shell_print(shell, " 1 = Charging");
		shell_print(shell, " 2 = Charge complete");
		shell_print(shell, " 3 = Damaged");
		shell_print(shell, " 4 = Low battery");
		shell_print(shell, " 5 = Not installed");
		shell_print(shell, " 6 = Unknown");
		return 0;
	}

	int32_t status = (int32_t)atoi(argv[1]);

	switch (lwm2m_carrier_battery_status_set(status)) {
	case 0:
		shell_print(shell, "Battery status updated successfully");
		break;
	case -ENODEV:
		shell_print(shell, "No internal battery detected");
		break;
	case -EINVAL:
		shell_print(shell, "Unsupported battery status");
		break;
	default:
		shell_print(shell, "Error: %d", errno);
		break;
	};

	return 0;
}

static int cmd_device_error_code_add(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_print(shell, " 0 = No error");
		shell_print(shell, " 1 = Low charge");
		shell_print(shell, " 2 = External supply off");
		shell_print(shell, " 3 = GPS failure");
		shell_print(shell, " 4 = Low signal");
		shell_print(shell, " 5 = Out of memory");
		shell_print(shell, " 6 = SMS failure");
		shell_print(shell, " 7 = IP connectivity failure");
		shell_print(shell, " 8 = Peripheral malfunction");
		return 0;
	}

	switch (lwm2m_carrier_error_code_add((int32_t)atoi(argv[1]))) {
	case 0:
		shell_print(shell, "Error code added successfully");
		break;
	case -EINVAL:
		shell_print(shell, "Unsupported error code");
		break;
	case -ENOENT:
		shell_print(shell, "Object not initialized");
		break;
	default:
		shell_print(shell, "Error: %d", errno);
		break;
	}

	return 0;
}

static int cmd_device_error_code_remove(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_print(shell, " 0 = No error");
		shell_print(shell, " 1 = Low charge");
		shell_print(shell, " 2 = External supply off");
		shell_print(shell, " 3 = GPS failure");
		shell_print(shell, " 4 = Low signal");
		shell_print(shell, " 5 = Out of memory");
		shell_print(shell, " 6 = SMS failure");
		shell_print(shell, " 7 = IP connectivity failure");
		shell_print(shell, " 8 = Peripheral malfunction");
		return 0;
	}

	switch (lwm2m_carrier_error_code_remove((int32_t)atoi(argv[1]))) {
	case 0:
		shell_print(shell, "Error code removed successfully");
		break;
	case -ENOENT:
		shell_print(shell, "Error code not found");
		break;
	case -EINVAL:
		shell_print(shell, "Unsupported error code");
		break;
	default:
		shell_print(shell, "Error: %d", errno);
		break;
	}

	return 0;
}

static int cmd_device_memory_total_set(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_print(shell, "memory_total <total memory in kB>");
		return 0;
	}

	switch (lwm2m_carrier_memory_total_set(strtoul(argv[1], NULL, 10))) {
	case 0:
		shell_print(shell, "Total amount of storage space set successfully");
		break;
	case -EINVAL:
		shell_print(shell, "Reported value is negative or bigger than INT32_MAX");
		break;
	default:
		break;
	}

	return 0;
}

static int cmd_portfolio_read(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 3) {
		shell_print(shell, "%s <object instance> <resource instance>", argv[0]);
		return 0;
	}

	int ret;
	uint16_t instance_id;
	uint16_t identity_type;
	char buffer[200];
	uint16_t len = sizeof(buffer);

	instance_id = (uint16_t)atoi(argv[1]);
	identity_type = (uint16_t)atoi(argv[2]);

	ret = lwm2m_carrier_identity_read(instance_id, identity_type, buffer, &len);

	switch (ret) {
	case 0:
		shell_print(shell, "%s", buffer);
		break;
	case -ENOMEM:
		shell_print(shell, "Insufficient memory");
		break;
	case -ENOENT:
		shell_print(shell, "Object instance %d does not exist", instance_id);
		break;
	case -EINVAL:
		shell_print(shell, "Invalid Identity type %d", identity_type);
		break;
	default:
		shell_print(shell, "Unknown error %d", ret);
		break;
	}

	return 0;
}

static int cmd_portfolio_write(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 4) {
		shell_print(shell, "%s <object instance> <resource instance> <value>", argv[0]);
		return 0;
	}

	int err;
	uint16_t instance_id;
	uint16_t identity_type;
	const char *val;

	instance_id = (int)atoi(argv[1]);
	identity_type = (int)atoi(argv[2]);
	val = argv[3];

	err = lwm2m_carrier_identity_write(instance_id, identity_type, val);

	switch (err) {
	case 0:
		shell_print(shell, "Wrote /16/%d/0/%d", instance_id, identity_type);
		break;
	case -ENOENT:
		shell_print(shell, "Object instance %d does not exist", instance_id);
		break;
	case -ENOMEM:
		shell_print(shell, "Insufficient memory");
		break;
	case -EINVAL:
		shell_print(shell, "String is NULL or empty, or invalid Identity type %d",
			    identity_type);
		break;
	case -E2BIG:
		shell_print(shell, "String is too long");
		break;
	case -EPERM:
		shell_print(shell, "Cannot write to instance %d", instance_id);
		break;
	default:
		shell_print(shell, "Unknown error %d", err);
		break;
	}

	return 0;
}

static int cmd_portfolio_create(const struct shell *shell, size_t argc, char **argv)
{
	int err;
	uint16_t instance_id;

	if (argc != 2) {
		shell_print(shell, "%s <object instance>", argv[0]);
		return 0;
	}

	instance_id = (int)atoi(argv[1]);

	err = lwm2m_carrier_portfolio_instance_create(instance_id);

	switch (err) {
	case 0:
		shell_print(shell, "Wrote /16/%d", instance_id);
		break;
	case -ENOENT:
		shell_print(shell, "Portfolio object not initialized");
		break;
	case -ENOMEM:
		shell_print(shell, "No slots available or already created");
		break;
	case -EINVAL:
		shell_print(shell, "Instance %d already in use", instance_id);
		break;
	default:
		shell_print(shell, "Unknown error %d", err);
		break;
	}

	return 0;
}

static int cmd_location_position(const struct shell *shell, size_t argc, char **argv)
{
	double latitude, longitude;
	float altitude = 0, uncertainty = 0;

	switch (argc) {
	case 5:
		uncertainty = strtof(argv[4], NULL);
		/* Fall-through. */
	case 4:
		altitude = strtof(argv[3], NULL);
		/* Fall-through. */
	case 3:
		longitude = strtod(argv[2], NULL);
		latitude = strtod(argv[1], NULL);
		break;
	default:
		shell_print(shell, "%s <lat> <lon> [alt] [radius]", argv[0]);
		shell_print(shell, " arguments:");
		shell_print(shell, "   lat (deg)  - latitude, between -90.0 and 90.0");
		shell_print(shell, "   lon (deg)  - longitude, between -180.0 and 180.0");
		shell_print(shell, "   alt (m)    - altitude over sea level");
		shell_print(shell, "   radius (m) - uncertainty, non-negative");

		return 0;
	}

	uint32_t timestamp;
	int err;

	lwm2m_carrier_time_read(&timestamp, NULL, NULL);

	err = lwm2m_carrier_location_set(latitude, longitude, altitude, timestamp, uncertainty);
	switch (err) {
	case 0:
		shell_print(shell, "Location successfully set");
		break;
	case -ENOENT:
		shell_print(shell, "Location object not initialized");
		break;
	default:
		shell_print(shell, "Unknown error %d", err);
		break;
	}

	return 0;
}

static inline void arg_to_float(float *val, char *arg)
{
	if (strcmp("-", arg) != 0) {
		*val = strtof(arg, NULL);
	}
}

static int cmd_location_velocity(const struct shell *shell, size_t argc, char **argv)
{
	int err, heading;
	float speed_h = NAN, speed_v = NAN, uncertainty_h = NAN, uncertainty_v = NAN;

	switch (argc) {
	case 6:
		arg_to_float(&uncertainty_v, argv[5]);
		/* Fall-through. */
	case 5:
		arg_to_float(&uncertainty_h, argv[4]);
		/* Fall-through. */
	case 4:
		arg_to_float(&speed_v, argv[3]);
		/* Fall-through. */
	case 3:
		arg_to_float(&speed_h, argv[2]);
		heading = strtol(argv[1], NULL, 10);

		err = lwm2m_carrier_velocity_set(heading, speed_h, speed_v,
						 uncertainty_h, uncertainty_v);
		switch (err) {
		case 0:
			shell_print(shell, "Velocity successfully set");
			break;
		case -ENOENT:
			shell_print(shell, "Location object not initialized");
			break;
		default:
			shell_print(shell, "Unknown error %d", err);
			break;
	}

		return 0;
	default:
		break;
	}

	shell_print(shell, "%s <heading> <speed_h> [speed_v] [uncertainty_h] [uncertainty_v]",
		    argv[0]);
	shell_print(shell, " optional arguments can be set to \"-\" to be left unspecified");
	shell_print(shell, "   heading (deg)       - horizontal direction, integer 0-359");
	shell_print(shell, "   speed_h (m/s)       - horizontal speed, non-negative");
	shell_print(shell, "   speed_v (m/s)       - vertical speed, negative only if downward");
	shell_print(shell, "   uncertainty_h (m/s) - horizontal uncertainty speed, non-negative");
	shell_print(shell, "   uncertainty_v (m/s) - vertical uncertainty speed, non-negative");

	return 0;
}

int lwm2m_carrier_memory_free_read(void)
{
	return m_mem_free;
}

int memory_free_set(uint32_t memory_free)
{
	if (memory_free > INT32_MAX) {
		return -EINVAL;
	}
	m_mem_free = (int32_t)memory_free;

	return 0;
}

static int cmd_memory_free_read(const struct shell *shell, size_t argc, char **argv)
{
	int memory = lwm2m_carrier_memory_free_read();

	shell_print(shell, "Free memory: %d", memory);

	return 0;
}

static int cmd_device_memory_free_write(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_print(shell, "memory_free <available memory in kB>");
		return 0;
	}

	switch (memory_free_set(strtoul(argv[1], NULL, 10))) {
	case 0:
		shell_print(shell, "Estimated amount of storage space updated successfully");
		break;
	case -EINVAL:
		shell_print(shell, "Reported value is negative or bigger than INT32_MAX");
		break;
	default:
		break;
	}

	return 0;
}

static int cmd_app_data_send(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_print(shell, "%s <hex_string>", argv[0]);
		return 0;
	}

	/* Convert hex in place. */
	int err;
	size_t len = strlen(argv[1]);

	if (len & 1) {
		shell_print(shell, "Invalid hex string length");
		return 0;
	}

	err = lwm2m_carrier_app_data_send(argv[1], len);

	switch (err) {
	case 0:
		shell_print(shell, "%u bytes scheduled to send", len);
		break;
	case -ENOENT:
		shell_print(shell, "App Data Container not initialized");
		break;
	case -ENOMEM:
		shell_print(shell, "Not enough memory");
		break;
	default:
		shell_print(shell, "Unknown error: %d", err);
		break;
	}

	return 0;
}

static int cmd_enable_set(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_print(shell, "%s <0|1>", argv[0]);
		shell_print(shell, " 0 = disable");
		shell_print(shell, " 1 = enable");
		return 0;
	}

	int enable = atoi(argv[1]);

	if (enable != 0 && enable != 1) {
		shell_print(shell, "invalid value, must be 0 or 1");
		return 0;
	}

	lwm2m_settings_enable_custom_config_set(enable);

	shell_print(shell, "Set enable custom config: %s", enable ? "Yes" : "No");

	return 0;
}

static int cmd_certification_mode_set(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_print(shell, "%s <0|1>", argv[0]);
		shell_print(shell, " 0 = disable");
		shell_print(shell, " 1 = enable");
		return 0;
	}

	int certification_mode = atoi(argv[1]);

	if (certification_mode != 0 && certification_mode != 1) {
		shell_print(shell, "invalid value, must be 0 or 1");
		return 0;
	}

	lwm2m_settings_certification_mode_set(certification_mode);

	shell_print(shell, "Set certification mode: %s", certification_mode ? "Yes" : "No");

	return 0;
}

static int cmd_disable_bootstrap_from_smartcard_set(const struct shell *shell, size_t argc, char
	  **argv)
{
	if (argc != 2) {
		shell_print(shell, "%s <0|1>", argv[0]);
		shell_print(shell, " 0 = disable");
		shell_print(shell, " 1 = enable");
		return 0;
	}

	int disable_bootstrap_from_smartcard = atoi(argv[1]);

	if (disable_bootstrap_from_smartcard != 0 && disable_bootstrap_from_smartcard != 1) {
		shell_print(shell, "invalid value, must be 0 or 1");
		return 0;
	}

	lwm2m_settings_disable_bootstrap_from_smartcard_set(disable_bootstrap_from_smartcard);

	shell_print(shell, "Set disable bootstrap from smartcard: %s",
				disable_bootstrap_from_smartcard ? "Yes" : "No");

	return 0;
}

static int cmd_is_bootstrap_server_set(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_print(shell, "%s <0|1>", argv[0]);
		shell_print(shell, " 0 = disable");
		shell_print(shell, " 1 = enable");
		return 0;
	}

	int is_bootstrap_server = atoi(argv[1]);

	if (is_bootstrap_server != 0 && is_bootstrap_server != 1) {
		shell_print(shell, "invalid value, must be 0 or 1");
		return 0;
	}

	lwm2m_settings_is_bootstrap_server_set(is_bootstrap_server);

	shell_print(shell, "Set bootstrap server: %s", is_bootstrap_server ? "Yes" : "No");

	return 0;
}

static int cmd_server_uri_set(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_print(shell, "%s <URI>", argv[0]);
		return 0;
	}

	char *server_uri = argv[1];

	int err = lwm2m_settings_server_uri_set(server_uri);

	switch (err) {
	case 0:
		shell_print(shell, "Set server URI: %s", server_uri);
		break;
	case -ENOMEM:
		shell_print(shell, "Insufficient memory");
		break;
	default:
		shell_print(shell, "Unknown error %d", err);
		break;
	}

	return 0;
}


static int cmd_server_lifetime_set(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_print(shell, "%s <seconds>", argv[0]);
		return 0;
	}

	int32_t server_lifetime = atoi(argv[1]);

	if (server_lifetime < 0 || server_lifetime > 86400) {
		shell_print(shell, "invalid value, must be between 0 and 86400 (24 hours)");
		return 0;
	}

	lwm2m_settings_server_lifetime_set(server_lifetime);

	shell_print(shell, "Set server lifetime: %d seconds", server_lifetime);

	return 0;
}

static int cmd_session_idle_timeout_set(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_print(shell, "%s <seconds>", argv[0]);
		return 0;
	}

	int32_t session_idle_timeout = atoi(argv[1]);

	if (session_idle_timeout < 0 || session_idle_timeout > 86400) {
		shell_print(shell, "invalid value, must be between 0 and 86400 (24 hours)");
		return 0;
	}

	lwm2m_settings_session_idle_timeout_set(session_idle_timeout);

	shell_print(shell, "Set session idle timeout: %d seconds", session_idle_timeout);

	return 0;
}

static int cmd_server_psk_set(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_print(shell, "Pre-shared-key in hex format. Example: 3e48a2");
		return 0;
	}

	size_t string_len = strlen(argv[1]);

	if (((string_len % 2) != 0)) {
		shell_print(shell, "String is not valid hex format. Example: 3e48a2");
		return 0;
	}

	char *server_psk = argv[1];

	int err = lwm2m_settings_server_psk_set(server_psk);

	switch (err) {
	case 0:
		shell_print(shell, "Set server PSK: %s", server_psk);
		break;
	case -ENOMEM:
		shell_print(shell, "Insufficient memory");
		break;
	default:
		shell_print(shell, "Unknown error %d", err);
		break;
	}

	return 0;
}

static int cmd_apn_set(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_print(shell, "%s <APN>", argv[0]);
		return 0;
	}

	char *apn = argv[1];

	int err = lwm2m_settings_apn_set(apn);

	switch (err) {
	case 0:
		shell_print(shell, "Set APN: %s", apn);
		break;
	case -ENOMEM:
		shell_print(shell, "Insufficient memory");
		break;
	default:
		shell_print(shell, "Unknown error %d", err);
		break;
	}

	return 0;
}


static int cmd_manufacturer_set(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_print(shell, "%s <value>", argv[0]);
		return 0;
	}

	char *manufacturer = argv[1];

	int err = lwm2m_settings_manufacturer_set(manufacturer);

	switch (err) {
	case 0:
		shell_print(shell, "Set manufacturer: %s", manufacturer);
		break;
	case -ENOMEM:
		shell_print(shell, "Insufficient memory");
		break;
	default:
		shell_print(shell, "Unknown error %d", err);
		break;
	}

	return 0;
}

static int cmd_model_number_set(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_print(shell, "%s <value>", argv[0]);
		return 0;
	}

	char *model_number = argv[1];

	int err = lwm2m_settings_model_number_set(model_number);

	switch (err) {
	case 0:
		shell_print(shell, "Set model number: %s", model_number);
		break;
	case -ENOMEM:
		shell_print(shell, "Insufficient memory");
		break;
	default:
		shell_print(shell, "Unknown error %d", err);
		break;
	}


	return 0;
}

static int cmd_device_type_set(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_print(shell, "%s <value>", argv[0]);
		return 0;
	}

	char *device_type = argv[1];

	int err = lwm2m_settings_device_type_set(device_type);

	switch (err) {
	case 0:
		shell_print(shell, "Set device type: %s", device_type);
		break;
	case -ENOMEM:
		shell_print(shell, "Insufficient memory");
		break;
	default:
		shell_print(shell, "Unknown error %d", err);
		break;
	}

	return 0;
}

static int cmd_hardware_version_set(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_print(shell, "%s <value>", argv[0]);
		return 0;
	}

	char *hardware_version = argv[1];

	int err = lwm2m_settings_hardware_version_set(hardware_version);

	switch (err) {
	case 0:
		shell_print(shell, "Set hardware version: %s", hardware_version);
		break;
	case -ENOMEM:
		shell_print(shell, "Insufficient memory");
		break;
	default:
		shell_print(shell, "Unknown error %d", err);
		break;
	}

	return 0;
}

static int cmd_software_version_set(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_print(shell, "%s <value>", argv[0]);
		return 0;
	}

	char *software_version = argv[1];

	int err = lwm2m_settings_software_version_set(software_version);

	switch (err) {
	case 0:
		shell_print(shell, "Set software version: %s", software_version);
		break;
	case -ENOMEM:
		shell_print(shell, "Insufficient memory");
		break;
	default:
		shell_print(shell, "Unknown error %d", err);
		break;
	}

	return 0;
}

static int cmd_service_code_set(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_print(shell, "%s <value>", argv[0]);
		return 0;
	}

	char *service_code = argv[1];

	int err = lwm2m_settings_service_code_set(service_code);

	switch (err) {
	case 0:
		shell_print(shell, "Set service code: %s", service_code);
		break;
	case -ENOMEM:
		shell_print(shell, "Insufficient memory");
		break;
	default:
		shell_print(shell, "Unknown error %d", err);
		break;
	}

	return 0;
}

static int cmd_settings_print(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(shell, "Custom carrier settings");
	shell_print(shell, "  Enable custom settings             %s",
			lwm2m_settings_enable_custom_config_get() ? "Yes" : "No");
	shell_print(shell, "  Certification mode                 %s",
			lwm2m_settings_certification_mode_get() ? "Yes" : "No");
	shell_print(shell, "  Disable bootstrap from smartcard   %s",
			lwm2m_settings_disable_bootstrap_from_smartcard_get() ? "Yes" : "No");
	shell_print(shell, "  Bootstrap server                   %s",
			lwm2m_settings_is_bootstrap_server_get() ? "Yes" : "No");
	shell_print(shell, "  Server URI                         %s",
			lwm2m_settings_server_uri_get());
	shell_print(shell, "  Server PSK                         %s",
			lwm2m_settings_server_psk_get());
	shell_print(shell, "  Server lifetime                    %d",
			lwm2m_settings_server_lifetime_get());
	shell_print(shell, "  Session idle timeout               %d",
			lwm2m_settings_session_idle_timeout_get());
	shell_print(shell, "  APN                                %s", lwm2m_settings_apn_get());
	shell_print(shell, "  Manufacturer                       %s",
			lwm2m_settings_manufacturer_get());
	shell_print(shell, "  Model number                       %s",
			lwm2m_settings_model_number_get());
	shell_print(shell, "  Device type                        %s",
			lwm2m_settings_device_type_get());
	shell_print(shell, "  Hardware version                   %s",
			lwm2m_settings_hardware_version_get());
	shell_print(shell, "  Software version                   %s",
			lwm2m_settings_software_version_get());
	shell_print(shell, "  Service code                       %s",
			lwm2m_settings_service_code_get());

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_carrier_app_data,
		SHELL_CMD(send, NULL, "Send hex data using the app data container object",
			  cmd_app_data_send),
		SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_carrier_device,
		SHELL_CMD(battery_level, NULL, "Set battery level", cmd_device_battery_level_set),
		SHELL_CMD(battery_status, NULL, "Set battery status",
			  cmd_device_battery_status_set),
		SHELL_CMD(current, NULL, "Set current measurement on a power source",
			  cmd_device_current_set),
		SHELL_CMD(error_code_add, NULL, "Add individual error code",
			  cmd_device_error_code_add),
		SHELL_CMD(error_code_remove, NULL, "Remove individual error code",
			  cmd_device_error_code_remove),
		SHELL_CMD(memory_total, NULL, "Set total amount of storage space",
			  cmd_device_memory_total_set),
		SHELL_CMD(memory_free_read, NULL, "Read amount of free memory space",
			  cmd_memory_free_read),
		SHELL_CMD(memory_free_write, NULL, "Set total amount of free memory space",
			  cmd_device_memory_free_write),
		SHELL_CMD(power_sources, NULL, "Set available device power sources",
			  cmd_device_power_sources_set),
		SHELL_CMD(timezone_read, NULL, "Read timezone", cmd_device_timezone_read),
		SHELL_CMD(timezone_write, NULL, "Write timezone", cmd_device_timezone_write),
		SHELL_CMD(time_read, NULL, "Read all time values", cmd_device_time_read),
		SHELL_CMD(utc_offset_read, NULL, "Read UTC offset", cmd_device_utc_offset_read),
		SHELL_CMD(utc_offset_write, NULL, "Write UTC offset", cmd_device_utc_offset_write),
		SHELL_CMD(utc_time_read, NULL, "Read UTC time", cmd_device_utc_time_read),
		SHELL_CMD(utc_time_write, NULL, "Write UTC time", cmd_device_utc_time_write),
		SHELL_CMD(voltage, NULL, "Set voltage measurement on a power source",
			  cmd_device_voltage_set),
		SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_carrier_location,
		SHELL_CMD(position, NULL, "Set position information", cmd_location_position),
		SHELL_CMD(velocity, NULL, "Set velocity information", cmd_location_velocity),
		SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_carrier_portfolio,
		SHELL_CMD(create, NULL, "Create an instance of the Portfolio object",
			  cmd_portfolio_create),
		SHELL_CMD(read, NULL, "Read the Identity resource of a Portfolio object instance",
			  cmd_portfolio_read),
		SHELL_CMD(write, NULL, "Write into an instance of the Identity resource",
			  cmd_portfolio_write),
		SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_carrier_api,
		SHELL_CMD(app_data, &sub_carrier_app_data, "App data container operations",
			  NULL),
		SHELL_CMD(device, &sub_carrier_device, "Update or retrieve device information",
			  NULL),
		SHELL_CMD(location, &sub_carrier_location, "Location object operations",
			  NULL),
		SHELL_CMD(portfolio, &sub_carrier_portfolio, "Portfolio object operations",
			  NULL),
		SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_carrier_init,
		SHELL_CMD(apn, NULL, "APN", cmd_apn_set),
		SHELL_CMD(certification_mode, NULL, "Certification mode",
			  cmd_certification_mode_set),
		SHELL_CMD(device_type, NULL, "Device type", cmd_device_type_set),
		SHELL_CMD(disable_bootstrap_from_smartcard, NULL,
			  "Disable bootstrap from smartcard",
			  cmd_disable_bootstrap_from_smartcard_set),
		SHELL_CMD(enable, NULL, "Enable custom settings",
			  cmd_enable_set),
		SHELL_CMD(hardware_version, NULL, "Hardware version", cmd_hardware_version_set),
		SHELL_CMD(is_bootstrap_server, NULL, "Bootstrap server",
			  cmd_is_bootstrap_server_set),
		SHELL_CMD(manufacturer, NULL, "Manufacturer", cmd_manufacturer_set),
		SHELL_CMD(model_number, NULL, "Model number", cmd_model_number_set),
		SHELL_CMD(print, NULL, "Print custom settings", cmd_settings_print),
		SHELL_CMD(session_idle_timeout, NULL, "Session timeout time",
			  cmd_session_idle_timeout_set),
		SHELL_CMD(server_lifetime, NULL, "Server lifetime.", cmd_server_lifetime_set),
		SHELL_CMD(server_psk, NULL, "Server PSK", cmd_server_psk_set),
		SHELL_CMD(server_uri, NULL, "Server URI", cmd_server_uri_set),
		SHELL_CMD(service_code, NULL, "Service code", cmd_service_code_set),
		SHELL_CMD(software_version, NULL, "Software version", cmd_software_version_set),
		SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(carrier_api, &sub_carrier_api, "LwM2M carrier API", NULL);
SHELL_CMD_REGISTER(carrier_init, &sub_carrier_init, "LwM2M carrier custom settings", NULL);
