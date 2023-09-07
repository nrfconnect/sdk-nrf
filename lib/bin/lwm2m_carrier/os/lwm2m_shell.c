/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <strings.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

#include <lwm2m_carrier.h>
#include <lwm2m_settings.h>
#include <zephyr/shell/shell.h>
#include <modem/modem_key_mgmt.h>
#include <nrf_modem_at.h>

#define RED	"\e[0;31m"
#define GREEN	"\e[0;32m"
#define NORMAL	"\e[0m"

static const char *pdn_type_str[4] = {
	"IPv4v6", "IPv4", "IPv6", "Non-IP"
};

static int m_mem_free;

static const struct {
	uint32_t bit_num;
	char *name;
} carriers_enabled_map[] = {
	{ LWM2M_CARRIER_GENERIC, "Generic" },
	{ LWM2M_CARRIER_VERIZON, "Verizon" },
	{ LWM2M_CARRIER_ATT, "AT&T" },
	{ LWM2M_CARRIER_LG_UPLUS, "LG U+" },
	{ LWM2M_CARRIER_T_MOBILE, "T-Mobile" },
	{ LWM2M_CARRIER_SOFTBANK, "SoftBank" }
};

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

	int err = lwm2m_carrier_avail_power_sources_set(power_sources, power_source_count);

	switch (err) {
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
		shell_print(shell, "Error: %d", err);
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
	int err = lwm2m_carrier_power_source_voltage_set(power_source, voltage);

	switch (err) {
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
		shell_print(shell, "Error: %d", err);
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
	int err = lwm2m_carrier_power_source_current_set(power_source, current);

	switch (err) {
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
		shell_print(shell, "Error: %d", err);
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

	int err = lwm2m_carrier_battery_level_set(atoi(argv[1]));

	switch (err) {
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
		shell_print(shell, "Error: %d", err);
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
	int err = lwm2m_carrier_battery_status_set(status);

	switch (err) {
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
		shell_print(shell, "Error: %d", err);
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

	int err = lwm2m_carrier_error_code_add((int32_t)atoi(argv[1]));

	switch (err) {
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
		shell_print(shell, "Error: %d", err);
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

	int err = lwm2m_carrier_error_code_remove((int32_t)atoi(argv[1]));

	switch (err) {
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
		shell_print(shell, "Error: %d", err);
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
		shell_print(shell, "No slots available");
		break;
	case -EBADR:
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

static int cmd_log_data_send(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_print(shell, "%s <data>", argv[0]);
		return 0;
	}

	int err;
	size_t len = strlen(argv[1]);

	err = lwm2m_carrier_log_data_set(argv[1], len);

	switch (err) {
	case 0:
		shell_print(shell, "Sent log data");
		break;
	case -ENOENT:
		shell_print(shell, "Event Log not initialized");
		break;
	case -EINVAL:
		shell_print(shell, "Invalid argument");
		break;
	case -ENOMEM:
		shell_print(shell, "Not enough memory");
		break;
	default:
		break;
	}

	return 0;
}

static int cmd_app_data_send(const struct shell *shell, size_t argc, char **argv)
{
	if (argc < 2 || argc > 4) {
		shell_print(shell, "%s <data> | [<instance_id> <resource_instance_id> [data]]",
			    argv[0]);
		shell_print(shell, " The data argument is required if using the App Data "
			    "Container.");
		shell_print(shell, " When using the Binary App Data Container it is optional,");
		shell_print(shell, "   not defining it clears the resource instance.");
		shell_print(shell, " The instance_id and resource_instance_id arguments are");
		shell_print(shell, "   only used when using the Binary App Data Container. ");
		return 0;
	}

	uint16_t path[4];
	uint8_t path_len;
	uint8_t *buffer = NULL;
	size_t buffer_len = 0;

	if (argc == 2) {
		/* Using the App Data Container object.*/
		path[0] = LWM2M_CARRIER_OBJECT_APP_DATA_CONTAINER;
		path[1] = 0;
		path[2] = 0;
		path_len = 3;
		buffer = argv[1];
		buffer_len = strlen(argv[1]);
	} else {
		/* Using the Binary App Data Container object. */
		path[0] = LWM2M_CARRIER_OBJECT_BINARY_APP_DATA_CONTAINER;
		path[1] = (uint16_t)atoi(argv[1]);
		path[2] = 0;
		path[3] = (uint16_t)atoi(argv[2]);
		path_len = 4;
		if (argc == 4) {
			buffer = argv[3];
			buffer_len = strlen(argv[3]);
		}
	}

	int err = lwm2m_carrier_app_data_send(path, path_len, buffer, buffer_len);

	switch (err) {
	case 0:
		shell_print(shell, "Wrote app data successfully");
		break;
	case -ENOENT:
		shell_print(shell, "Object not initialized or invalid instance");
		break;
	case -EINVAL:
		shell_print(shell, "Invalid resource instance");
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

static int cmd_request_link_up(const struct shell *shell, size_t argc, char **argv)
{
	int err;

	err = lwm2m_carrier_request(LWM2M_CARRIER_REQUEST_LINK_UP);

	switch (err) {
	case 0:
		shell_print(shell, "Requested link up successfully");
		break;
	default:
		shell_print(shell, "Unknown error: %d", err);
		break;
	}

	return 0;
}

static int cmd_request_link_down(const struct shell *shell, size_t argc, char **argv)
{
	int err;

	err = lwm2m_carrier_request(LWM2M_CARRIER_REQUEST_LINK_DOWN);

	switch (err) {
	case 0:
		shell_print(shell, "Requested link down successfully");
		break;
	default:
		shell_print(shell, "Unknown error: %d", err);
		break;
	}

	return 0;
}

static int cmd_request_reboot(const struct shell *shell, size_t argc, char **argv)
{
	int err;

	err = lwm2m_carrier_request(LWM2M_CARRIER_REQUEST_REBOOT);

	switch (err) {
	case 0:
		shell_print(shell, "Requested reboot successfully");
		break;
	default:
		shell_print(shell, "Unknown error: %d", err);
		break;
	}

	return 0;
}

static int cmd_enable_set(const struct shell *shell, size_t argc, char **argv)
{
	lwm2m_settings_enable_custom_config_set(true);

	shell_print(shell, "Enabled custom config");

	return 0;
}

static int cmd_disable_set(const struct shell *shell, size_t argc, char **argv)
{
	lwm2m_settings_enable_custom_config_set(false);

	shell_print(shell, "Disabled custom config");

	return 0;
}

static int string_to_bool(const char *str, bool *buffer)
{
	if (str[0] == 'y' || str[0] == 'Y' || str[0] == '1') {
		*buffer = true;
		return 0;
	}

	if (str[0] == 'n' || str[0] == 'N' ||  str[0] == '0') {
		*buffer = false;
		return 0;
	}

	return -EINVAL;
}

static char *carriers_enabled_str(void)
{
	static char oper_str[255] = { 0 };
	uint32_t carriers_enabled = lwm2m_settings_carriers_enabled_get();
	int offset = 0;

	if (carriers_enabled == UINT32_MAX) {
		return "All";
	} else if (carriers_enabled == 0) {
		return "None";
	}

	for (int i = 0; i < ARRAY_SIZE(carriers_enabled_map); i++) {
		if ((carriers_enabled & carriers_enabled_map[i].bit_num) &&
		    (offset < sizeof(oper_str))) {
			offset += snprintf(&oper_str[offset], sizeof(oper_str) - offset,
					   "%s%s (%u)", (offset == 0) ? "" : ", ",
					   carriers_enabled_map[i].name, i);
		}
	}

	return oper_str;
}

static int cmd_carriers_set(const struct shell *shell, size_t argc, char **argv)
{
	if (argc < 2) {
		shell_print(shell, "%s all", argv[0]);
		shell_print(shell, "  All carriers except AT&T");
		shell_print(shell, "%s <id1> <id2> ...", argv[0]);
		for (int i = 0; i < ARRAY_SIZE(carriers_enabled_map); i++) {
			shell_print(shell, "  %d = %s", i, carriers_enabled_map[i].name);
		}
		return 0;
	}

	uint32_t carriers_enabled = 0;

	if (strcasecmp(argv[1], "all") == 0) {
		carriers_enabled = UINT32_MAX;
	} else {
		for (int i = 1; i < argc; i++) {
			uint32_t oper_id = atoi(argv[i]);

			if (oper_id < ARRAY_SIZE(carriers_enabled_map)) {
				carriers_enabled |= carriers_enabled_map[oper_id].bit_num;
			} else {
				shell_error(shell, "Illegal operator: %u", oper_id);
			}
		}
	}

	lwm2m_settings_carriers_enabled_set(carriers_enabled);
	shell_print(shell, "Set carriers enabled: %s", carriers_enabled_str());

	return 0;
}

static int cmd_bootstrap_from_smartcard_set(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_print(shell, "%s <y|n>", argv[0]);
		return 0;
	}

	bool bootstrap_from_smartcard;
	int err = string_to_bool(argv[1], &bootstrap_from_smartcard);

	if (!err) {
		lwm2m_settings_bootstrap_from_smartcard_set(bootstrap_from_smartcard);
		shell_print(shell, "Set bootstrap from smartcard: %s",
		bootstrap_from_smartcard ? GREEN "Yes" NORMAL : RED "No" NORMAL);
	} else {
		shell_print(shell, "Invalid input: <y|n>");
	}

	return 0;
}

static int cmd_is_bootstrap_server_set(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_print(shell, "%s <y|n>", argv[0]);
		return 0;
	}

	bool is_bootstrap_server;
	int err = string_to_bool(argv[1], &is_bootstrap_server);

	if (!err) {
		lwm2m_settings_is_bootstrap_server_set(is_bootstrap_server);
		shell_print(shell, "Set is bootstrap server: %s",
		is_bootstrap_server ? GREEN "Yes" NORMAL : RED "No" NORMAL);
	} else {
		shell_print(shell, "Invalid input: <y|n>");
	}

	return 0;
}

static int cmd_server_binding_set(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_print(shell, "%s <binding>", argv[0]);
		shell_print(shell, "  U = UDP");
		shell_print(shell, "  N = Non-IP");
		return 0;
	}

	uint8_t server_binding = argv[1][0];

	if ((strlen(argv[1]) != 1) || (server_binding != 'U' && server_binding != 'N')) {
		shell_print(shell, "invalid value, must be 'U' or 'N'");
		return 0;
	}

	lwm2m_settings_server_binding_set(server_binding);

	shell_print(shell, "Set server binding: %c", server_binding);

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
		shell_print(shell, " -1 = disabled");
		shell_print(shell, "  0 = use default (60 seconds)");
		return 0;
	}

	int32_t session_idle_timeout = atoi(argv[1]);

	if (session_idle_timeout < -1 || session_idle_timeout > 86400) {
		shell_print(shell, "invalid value, must be -1 or between 0 and 86400 (24 hours)");
		return 0;
	}

	lwm2m_settings_session_idle_timeout_set(session_idle_timeout);

	shell_print(shell, "Set session idle timeout: %d seconds", session_idle_timeout);

	return 0;
}

static int cmd_coap_confirmable_interval(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_print(shell, "%s <seconds>", argv[0]);
		shell_print(shell, " -1 = always confirmable");
		shell_print(shell, "  0 = use default (24 hours)");
		return 0;
	}

	int32_t coap_con_interval = atoi(argv[1]);

	if (coap_con_interval < -1 || coap_con_interval > 86400) {
		shell_print(shell, "invalid value, must be -1 or between 0 and 86400 (24 hours)");
		return 0;
	}

	lwm2m_settings_coap_con_interval_set(coap_con_interval);

	shell_print(shell, "Set CoAP confirmable interval: %d seconds", coap_con_interval);

	return 0;
}

static int cmd_sec_tag_set(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_print(shell, "%s <security tag>", argv[0]);
		return 0;
	}

	uint32_t server_sec_tag = strtoul(argv[1], NULL, 10);
	bool provisioned;

	int err = modem_key_mgmt_exists(server_sec_tag, MODEM_KEY_MGMT_CRED_TYPE_PSK, &provisioned);

	if (err) {
		return -1;
	}

	lwm2m_settings_server_sec_tag_set(server_sec_tag);
	shell_print(shell, "Set security tag: %u", server_sec_tag);

	if (!provisioned && server_sec_tag != 0) {
		shell_print(shell, "Warning: a PSK does not exist in sec_tag %u.", server_sec_tag);
		shell_print(shell, "This can be written using AT%%CMNG=0,%u,3,\"PSK\"",
			    server_sec_tag);
	}

	return 0;
}

static int cmd_server_enable_set(const struct shell *shell, size_t argc, char **argv)
{
	lwm2m_settings_enable_custom_server_config_set(true);

	shell_print(shell, "Enabled custom server config");

	return 0;
}

static int cmd_server_disable_set(const struct shell *shell, size_t argc, char **argv)
{
	lwm2m_settings_enable_custom_server_config_set(false);

	shell_print(shell, "Disabled custom server config");

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

static int cmd_auto_startup_set(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_print(shell, "%s <y|n>", argv[0]);
		return 0;
	}

	bool auto_startup;
	int err = string_to_bool(argv[1], &auto_startup);

	if (!err) {
		lwm2m_settings_auto_startup_set(auto_startup);
		shell_print(shell, "Set auto startup: %s",
		auto_startup ? GREEN "Yes" NORMAL : RED "No" NORMAL);
	} else {
		shell_print(shell, "Invalid input: <y|n>");
	}

	return 0;
}

static int cmd_pdn_type_set(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_print(shell, "%s <pdn_type>", argv[0]);
		for (int i = 0; i < ARRAY_SIZE(pdn_type_str); i++) {
			shell_print(shell, "  %d = %s", i, pdn_type_str[i]);
		}
		return 0;
	}

	uint32_t pdn_type = atoi(argv[1]);

	if (pdn_type > LWM2M_CARRIER_PDN_TYPE_NONIP) {
		shell_print(shell, "invalid value, must be between 0 and %u",
			    LWM2M_CARRIER_PDN_TYPE_NONIP);
		return 0;
	}

	lwm2m_settings_pdn_type_set(pdn_type);

	shell_print(shell, "Set pdn_type: %s (%d)", pdn_type_str[pdn_type], pdn_type);

	return 0;
}

static int cmd_device_enable_set(const struct shell *shell, size_t argc, char **argv)
{
	lwm2m_settings_enable_custom_device_config_set(true);

	shell_print(shell, "Enabled custom device config");

	return 0;
}

static int cmd_device_disable_set(const struct shell *shell, size_t argc, char **argv)
{
	lwm2m_settings_enable_custom_device_config_set(false);

	shell_print(shell, "Disabled custom device config");

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

static int device_serial_no_type_get(int device_serial_no_type)
{
	switch (device_serial_no_type) {
	case 0:
		return LWM2M_CARRIER_LG_UPLUS_DEVICE_SERIAL_NO_IMEI;
	case 1:
		return LWM2M_CARRIER_LG_UPLUS_DEVICE_SERIAL_NO_2DID;
	default:
		return -EINVAL;
	}
}

static int cmd_device_serial_no_type_set(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_print(shell, " 0 = IMEI");
		shell_print(shell, " 1 = 2DID");
		return 0;
	}

	int32_t device_serial_no_type = device_serial_no_type_get(atoi(argv[1]));

	if (device_serial_no_type < 0) {
		shell_print(shell, "Invalid input");
		return 0;
	}

	lwm2m_settings_device_serial_no_type_set(device_serial_no_type);
	shell_print(shell, "LG U+ Device Serial Number type set successfully");

	return 0;
}

static int cmd_settings_print(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(shell, "Automatic startup                %s",
			lwm2m_settings_auto_startup_get() ?
			GREEN "Yes" NORMAL : RED "No" NORMAL);
	shell_print(shell, "");
	shell_print(shell, "Custom carrier settings          %s",
			lwm2m_settings_enable_custom_config_get() ?
			GREEN "Yes" NORMAL : RED "No" NORMAL);
	shell_print(shell, "  Carriers enabled               %s", carriers_enabled_str());
	shell_print(shell, "  Bootstrap from smartcard       %s",
			lwm2m_settings_bootstrap_from_smartcard_get() ? "Yes" : "No");
	shell_print(shell, "  Session idle timeout           %d",
			lwm2m_settings_session_idle_timeout_get());
	shell_print(shell, "  CoAP confirmable interval      %d",
			lwm2m_settings_coap_con_interval_get());
	shell_print(shell, "  APN                            %s", lwm2m_settings_apn_get());
	shell_print(shell, "  PDN type                       %s",
			pdn_type_str[lwm2m_settings_pdn_type_get()]);
	shell_print(shell, "  Service code                   %s",
			lwm2m_settings_service_code_get());
	shell_print(shell, "  Device Serial Number type      %d",
			lwm2m_settings_device_serial_no_type_get());
	shell_print(shell, "");
	shell_print(shell, "Custom carrier server settings   %s",
			lwm2m_settings_enable_custom_server_config_get() ?
			GREEN "Yes" NORMAL : RED "No" NORMAL);
	shell_print(shell, "  Is bootstrap server            %s  %s",
			lwm2m_settings_is_bootstrap_server_get() ? "Yes" : "No",
			strlen(lwm2m_settings_server_uri_get()) ?
			"" : "(Not used without server URI)");
	shell_print(shell, "  Server URI                     %s",
			lwm2m_settings_server_uri_get());
	shell_print(shell, "  PSK security tag               %u",
			lwm2m_settings_server_sec_tag_get());
	shell_print(shell, "  Server lifetime                %d  %s",
			lwm2m_settings_server_lifetime_get(),
			lwm2m_settings_is_bootstrap_server_get() ?
			"(Not used when bootstrap server)" : "");
	shell_print(shell, "  Server binding                 %c",
			lwm2m_settings_server_binding_get());
	shell_print(shell, "");
	shell_print(shell, "Custom carrier device settings   %s",
			lwm2m_settings_enable_custom_device_config_get() ?
			GREEN "Yes" NORMAL : RED "No" NORMAL);
	shell_print(shell, "  Manufacturer                   %s",
			lwm2m_settings_manufacturer_get());
	shell_print(shell, "  Model number                   %s",
			lwm2m_settings_model_number_get());
	shell_print(shell, "  Device type                    %s",
			lwm2m_settings_device_type_get());
	shell_print(shell, "  Hardware version               %s",
		lwm2m_settings_hardware_version_get());
	shell_print(shell, "  Software version               %s",
			lwm2m_settings_software_version_get());

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_carrier_event_log,
		SHELL_CMD(send, NULL, "Send log data using the event log object",
			  cmd_log_data_send),
		SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_carrier_app_data,
		SHELL_CMD(send, NULL, "Send data using an app data container object",
			  cmd_app_data_send),
		SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_carrier_api_device,
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

SHELL_STATIC_SUBCMD_SET_CREATE(sub_carrier_request,
		SHELL_CMD(link_down, NULL, "Link down", cmd_request_link_down),
		SHELL_CMD(link_up, NULL, "Link up", cmd_request_link_up),
		SHELL_CMD(reboot, NULL, "Reboot", cmd_request_reboot),
		SHELL_SUBCMD_SET_END /* Array terminated. */
);

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
		SHELL_CMD(device, &sub_carrier_api_device, "Update or retrieve device information",
			  NULL),
		SHELL_CMD(event_log, &sub_carrier_event_log, "Event log object operations", NULL),
		SHELL_CMD(location, &sub_carrier_location, "Location object operations",
			  NULL),
		SHELL_CMD(portfolio, &sub_carrier_portfolio, "Portfolio object operations",
			  NULL),
		SHELL_CMD(request, &sub_carrier_request, "Request an action", NULL),
		SHELL_SUBCMD_SET_END);


SHELL_STATIC_SUBCMD_SET_CREATE(sub_carrier_config_device,
		SHELL_CMD(device_type, NULL, "Device type", cmd_device_type_set),
		SHELL_CMD(disable, NULL, "Disable custom device settings",
			  cmd_device_disable_set),
		SHELL_CMD(enable, NULL, "Enable custom device settings", cmd_device_enable_set),
		SHELL_CMD(manufacturer, NULL, "Manufacturer", cmd_manufacturer_set),
		SHELL_CMD(model_number, NULL, "Model number", cmd_model_number_set),
		SHELL_CMD(hardware_version, NULL, "Hardware version", cmd_hardware_version_set),
		SHELL_CMD(software_version, NULL, "Software version", cmd_software_version_set),
		SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_carrier_config_server,
		SHELL_CMD(binding, NULL, "Server binding", cmd_server_binding_set),
		SHELL_CMD(disable, NULL, "Disable custom server settings", cmd_server_disable_set),
		SHELL_CMD(enable, NULL, "Enable custom server settings", cmd_server_enable_set),
		SHELL_CMD(is_bootstrap_server, NULL, "Is bootstrap server",
			  cmd_is_bootstrap_server_set),
		SHELL_CMD(lifetime, NULL, "Server lifetime", cmd_server_lifetime_set),
		SHELL_CMD(sec_tag, NULL, "Security tag", cmd_sec_tag_set),
		SHELL_CMD(uri, NULL, "Server URI", cmd_server_uri_set),
		SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_carrier_config,
		SHELL_CMD(apn, NULL, "APN", cmd_apn_set),
		SHELL_CMD(auto_startup, NULL, "Automatic startup", cmd_auto_startup_set),
		SHELL_CMD(bootstrap_from_smartcard, NULL, "Bootstrap from smartcard",
			  cmd_bootstrap_from_smartcard_set),
		SHELL_CMD(carriers, NULL, "Carriers enabled", cmd_carriers_set),
		SHELL_CMD(coap_confirmable_interval, NULL, "CoAP confirmable interval",
			  cmd_coap_confirmable_interval),
		SHELL_CMD(device, &sub_carrier_config_device, "Device configuration", NULL),
		SHELL_CMD(disable, NULL, "Disable custom settings", cmd_disable_set),
		SHELL_CMD(enable, NULL, "Enable custom settings", cmd_enable_set),
		SHELL_CMD(pdn_type, NULL, "PDN type", cmd_pdn_type_set),
		SHELL_CMD(print, NULL, "Print custom settings", cmd_settings_print),
		SHELL_CMD(server, &sub_carrier_config_server, "Server configuration", NULL),
		SHELL_CMD(service_code, NULL, "Service code", cmd_service_code_set),
		SHELL_CMD(device_serial_no_type, NULL, "Device Serial Number type",
			  cmd_device_serial_no_type_set),
		SHELL_CMD(session_idle_timeout, NULL, "Session timeout time",
			  cmd_session_idle_timeout_set),
		SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(carrier_api, &sub_carrier_api, "LwM2M carrier API", NULL);
SHELL_CMD_REGISTER(carrier_config, &sub_carrier_config, "LwM2M carrier configuration", NULL);
