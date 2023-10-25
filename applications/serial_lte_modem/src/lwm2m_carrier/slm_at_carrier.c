/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <lwm2m_carrier.h>
#include <zephyr/logging/log.h>
#include <modem/nrf_modem_lib.h>
#include "slm_at_host.h"
#include "slm_settings.h"
#include "slm_util.h"
#include "slm_at_carrier.h"

LOG_MODULE_REGISTER(slm_carrier, CONFIG_SLM_LOG_LEVEL);

/**@brief LwM2M Carrier operations. */
enum slm_carrier_operation {
	/* Carrier AppData Operation */
	CARRIER_OP_APPDATA_SEND,
	/* Carrier Device Operation */
	CARRIER_OP_DEVICE_BATTERY_LEVEL,
	CARRIER_OP_DEVICE_BATTERY_STATUS,
	CARRIER_OP_DEVICE_CURRENT,
	CARRIER_OP_DEVICE_ERROR,
	CARRIER_OP_DEVICE_MEM_TOTAL,
	CARRIER_OP_DEVICE_MEM_FREE,
	CARRIER_OP_DEVICE_POWER_SOURCES,
	CARRIER_OP_DEVICE_TIMEZONE,
	CARRIER_OP_DEVICE_TIME,
	CARRIER_OP_DEVICE_UTC_OFFSET,
	CARRIER_OP_DEVICE_UTC_TIME,
	CARRIER_OP_DEVICE_VOLTAGE,
	/* Carrier Event Log Operation */
	CARRIER_OP_EVENT_LOG_LOG_DATA,
	/* Carrier Location Operation */
	CARRIER_OP_LOCATION_POSITION,
	CARRIER_OP_LOCATION_VELOCITY,
	/* Carrier Portfolio Operation */
	CARRIER_OP_PORTFOLIO,
	/* Carrier Request Operation */
	CARRIER_OP_REQUEST_REBOOT,
	CARRIER_OP_REQUEST_LINK_DOWN,
	CARRIER_OP_REQUEST_LINK_UP,
	/* Count */
	CARRIER_OP_MAX
};

struct carrier_op_list {
	uint8_t op_code;
	char *op_str;
	int (*handler)(void);
};

/* Static variable to report the memory free resource. */
static int m_mem_free;

/** forward declaration of cmd handlers **/
static int do_carrier_appdata_send(void);
static int do_carrier_device_battery_level(void);
static int do_carrier_device_battery_status(void);
static int do_carrier_device_current(void);
static int do_carrier_device_error(void);
static int do_carrier_device_mem_total(void);
static int do_carrier_device_mem_free(void);
static int do_carrier_device_power_sources(void);
static int do_carrier_device_timezone(void);
static int do_carrier_device_time(void);
static int do_carrier_device_utc_offset(void);
static int do_carrier_device_utc_time(void);
static int do_carrier_device_voltage(void);
static int do_carrier_event_log_log_data(void);
static int do_carrier_location_position(void);
static int do_carrier_location_velocity(void);
static int do_carrier_portfolio(void);
static int do_carrier_request_reboot(void);
static int do_carrier_request_link_down(void);
static int do_carrier_request_link_up(void);

/**@brief SLM AT Command list type. */
static struct carrier_op_list op_list[CARRIER_OP_MAX] = {
	{CARRIER_OP_APPDATA_SEND, "app_data", do_carrier_appdata_send},
	{CARRIER_OP_DEVICE_BATTERY_LEVEL, "battery_level", do_carrier_device_battery_level},
	{CARRIER_OP_DEVICE_BATTERY_STATUS, "battery_status", do_carrier_device_battery_status},
	{CARRIER_OP_DEVICE_CURRENT, "current", do_carrier_device_current},
	{CARRIER_OP_DEVICE_ERROR, "error", do_carrier_device_error},
	{CARRIER_OP_DEVICE_MEM_TOTAL, "memory_total", do_carrier_device_mem_total},
	{CARRIER_OP_DEVICE_MEM_FREE, "memory_free", do_carrier_device_mem_free},
	{CARRIER_OP_DEVICE_POWER_SOURCES, "power_sources", do_carrier_device_power_sources},
	{CARRIER_OP_DEVICE_TIMEZONE, "timezone", do_carrier_device_timezone},
	{CARRIER_OP_DEVICE_TIME, "time", do_carrier_device_time},
	{CARRIER_OP_DEVICE_UTC_OFFSET, "utc_offset", do_carrier_device_utc_offset},
	{CARRIER_OP_DEVICE_UTC_TIME, "utc_time", do_carrier_device_utc_time},
	{CARRIER_OP_DEVICE_VOLTAGE, "voltage", do_carrier_device_voltage},
	{CARRIER_OP_EVENT_LOG_LOG_DATA, "log_data", do_carrier_event_log_log_data},
	{CARRIER_OP_LOCATION_POSITION, "position", do_carrier_location_position},
	{CARRIER_OP_LOCATION_VELOCITY, "velocity", do_carrier_location_velocity},
	{CARRIER_OP_PORTFOLIO, "portfolio", do_carrier_portfolio},
	{CARRIER_OP_REQUEST_REBOOT, "reboot", do_carrier_request_reboot},
	{CARRIER_OP_REQUEST_LINK_DOWN, "link_down", do_carrier_request_link_down},
	{CARRIER_OP_REQUEST_LINK_UP, "link_up", do_carrier_request_link_up},
};

#define SLM_CARRIER_OP_STR_MAX (sizeof("battery_status") + 1)

struct k_work_delayable reconnect_work;

/* Global functions defined in different files. */
int lte_auto_connect(void);

static void print_err(const lwm2m_carrier_event_t *evt)
{
	const lwm2m_carrier_event_error_t *err = evt->data.error;

	static const char * const strerr[] = {
		[LWM2M_CARRIER_ERROR_NO_ERROR] =
			"No error",
		[LWM2M_CARRIER_ERROR_BOOTSTRAP] =
			"Bootstrap error",
		[LWM2M_CARRIER_ERROR_LTE_LINK_UP_FAIL] =
			"Failed to connect to the LTE network",
		[LWM2M_CARRIER_ERROR_LTE_LINK_DOWN_FAIL] =
			"Failed to disconnect from the LTE network",
		[LWM2M_CARRIER_ERROR_FOTA_FAIL] =
			"Modem firmware update failed",
		[LWM2M_CARRIER_ERROR_CONFIGURATION] =
			"Illegal object configuration detected",
		[LWM2M_CARRIER_ERROR_INIT] =
			"Initialization failure",
		[LWM2M_CARRIER_ERROR_RUN] =
			"Configuration failure",
	};

	__ASSERT(PART_OF_ARRAY(strerr[err->type]),
		 "Unhandled liblwm2m_carrier error");

	LOG_ERR("LWM2M_CARRIER_EVENT_ERROR: %s, reason %d", strerr[err->type], err->value);
}

static void print_deferred(const lwm2m_carrier_event_t *evt)
{
	const lwm2m_carrier_event_deferred_t *def = evt->data.deferred;

	static const char * const strdef[] = {
		[LWM2M_CARRIER_DEFERRED_NO_REASON] =
			"No reason given",
		[LWM2M_CARRIER_DEFERRED_PDN_ACTIVATE] =
			"Failed to activate PDN",
		[LWM2M_CARRIER_DEFERRED_BOOTSTRAP_NO_ROUTE] =
			"No route to bootstrap server",
		[LWM2M_CARRIER_DEFERRED_BOOTSTRAP_CONNECT] =
			"Failed to connect to bootstrap server",
		[LWM2M_CARRIER_DEFERRED_BOOTSTRAP_SEQUENCE] =
			"Bootstrap sequence not completed",
		[LWM2M_CARRIER_DEFERRED_SERVER_NO_ROUTE] =
			"No route to server",
		[LWM2M_CARRIER_DEFERRED_SERVER_CONNECT] =
			"Failed to connect to server",
		[LWM2M_CARRIER_DEFERRED_SERVER_REGISTRATION] =
			"Server registration sequence not completed",
		[LWM2M_CARRIER_DEFERRED_SERVICE_UNAVAILABLE] =
			"Server in maintenance mode",
		[LWM2M_CARRIER_DEFERRED_SIM_MSISDN] =
			"Waiting for SIM MSISDN",
	};

	__ASSERT(PART_OF_ARRAY(strdef[def->reason]),
		 "Unhandled deferred reason");

	LOG_INF("LWM2M_CARRIER_EVENT_DEFERRED: reason %s, timeout %d seconds",
		strdef[def->reason], def->timeout);
}

static void on_event_app_data(const lwm2m_carrier_event_t *event)
{
	lwm2m_carrier_event_app_data_t *app_data = event->data.app_data;

	rsp_send("\r\n#XCARRIEREVT: %d,%d\r\n", event->type, app_data->buffer_len);

	memcpy(slm_data_buf, app_data->buffer, app_data->buffer_len);

	data_send(slm_data_buf, app_data->buffer_len);
}

static void reconnect_wk(struct k_work *work)
{
	(void)lte_auto_connect();
}

int lwm2m_carrier_event_handler(const lwm2m_carrier_event_t *event)
{
	int err = 0;
	static bool fota_started;

	switch (event->type) {
	case LWM2M_CARRIER_EVENT_LTE_LINK_UP:
		LOG_DBG("LWM2M_CARRIER_EVENT_LTE_LINK_UP");
		if (fota_started) {
			fota_started = false;
			k_work_reschedule(&reconnect_work, K_MSEC(100));
		} else {
			/* AT+CFUN=1 to be issued. */
			err = -1;
		}
		break;
	case LWM2M_CARRIER_EVENT_LTE_LINK_DOWN:
		LOG_DBG("LWM2M_CARRIER_EVENT_LTE_LINK_DOWN");
		/* AT+CFUN=4 to be issued. */
		err = -1;
		break;
	case LWM2M_CARRIER_EVENT_LTE_POWER_OFF:
		LOG_DBG("LWM2M_CARRIER_EVENT_LTE_POWER_OFF");
		/* TODO: defer setting the modem to minimum funtional mode. */
		err = nrf_modem_at_printf("AT+CFUN=0");
		break;
	case LWM2M_CARRIER_EVENT_BOOTSTRAPPED:
		LOG_DBG("LWM2M_CARRIER_EVENT_BOOTSTRAPPED");
		break;
	case LWM2M_CARRIER_EVENT_REGISTERED:
		LOG_DBG("LWM2M_CARRIER_EVENT_REGISTERED");
		break;
	case LWM2M_CARRIER_EVENT_DEFERRED:
		print_deferred(event);
		break;
	case LWM2M_CARRIER_EVENT_FOTA_START:
		LOG_DBG("LWM2M_CARRIER_EVENT_FOTA_START");
		fota_started = true;
		break;
	case LWM2M_CARRIER_EVENT_FOTA_SUCCESS:
		LOG_DBG("LWM2M_CARRIER_EVENT_FOTA_SUCCESS");
		break;
	case LWM2M_CARRIER_EVENT_REBOOT:
		LOG_DBG("LWM2M_CARRIER_EVENT_REBOOT");
		/* Return -1 to defer the reboot until the application decides to do so. */
		err = -1;
		break;
	case LWM2M_CARRIER_EVENT_APP_DATA:
		LOG_DBG("LWM2M_CARRIER_EVENT_APP_DATA");
		on_event_app_data(event);
		return 0;
	case LWM2M_CARRIER_EVENT_MODEM_INIT:
		LOG_DBG("LWM2M_CARRIER_EVENT_MODEM_INIT");
		nrf_modem_lib_init();
		break;
	case LWM2M_CARRIER_EVENT_MODEM_SHUTDOWN:
		LOG_DBG("LWM2M_CARRIER_EVENT_MODEM_SHUTDOWN");
		nrf_modem_lib_shutdown();
		break;
	case LWM2M_CARRIER_EVENT_ERROR:
		print_err(event);
		break;
	}

	rsp_send("\r\n#XCARRIEREVT: %d,%d\r\n", event->type, err);

	return err;
}

/* Carrier App Data Send data mode handler */
static int carrier_datamode_callback(uint8_t op, const uint8_t *data, int len, uint8_t flags)
{
	int ret = 0;

	if (op == DATAMODE_SEND) {
		if ((flags & SLM_DATAMODE_FLAGS_MORE_DATA) != 0) {
			LOG_ERR("Datamode buffer overflow");
			(void)exit_datamode_handler(-EOVERFLOW);
			return -EOVERFLOW;
		}
		uint16_t path[3] = { LWM2M_CARRIER_OBJECT_APP_DATA_CONTAINER, 0, 0 };
		uint8_t path_len = 3;

		ret = lwm2m_carrier_app_data_send(path, path_len, data, len);
		LOG_INF("datamode send: %d", ret);
		if (ret < 0) {
			(void)exit_datamode_handler(ret);
		}
	} else if (op == DATAMODE_EXIT) {
		LOG_DBG("datamode exit");
	}

	return ret;
}

/* AT#XCARRIER="app_data"[,<data>][,<instance_id>,<resource_instance_id>] */
static int do_carrier_appdata_send(void)
{
	int ret = 0;

	uint32_t param_count = at_params_valid_count_get(&slm_at_param_list);

	if (param_count == 2) {
		/* enter data mode */
		ret = enter_datamode(carrier_datamode_callback);
		if (ret) {
			return ret;
		}
	} else if (param_count == 3) {
		char data[CONFIG_SLM_CARRIER_APP_DATA_BUFFER_LEN] = {0};
		int size = CONFIG_SLM_CARRIER_APP_DATA_BUFFER_LEN;

		uint16_t path[3] = { LWM2M_CARRIER_OBJECT_APP_DATA_CONTAINER, 0, 0 };
		uint8_t path_len = 3;

		ret = util_string_get(&slm_at_param_list, 2, data, &size);
		if (ret) {
			return ret;
		}

		ret = lwm2m_carrier_app_data_send(path, path_len, data, size);
	} else if (param_count == 4 || param_count == 5) {
		uint8_t *data = NULL;
		char buffer[CONFIG_SLM_CARRIER_APP_DATA_BUFFER_LEN] = {0};
		int size = 0;

		uint16_t inst_id;
		uint16_t res_inst_id;

		ret = at_params_unsigned_short_get(&slm_at_param_list, param_count - 2, &inst_id);
		if (ret) {
			return ret;
		}

		ret = at_params_unsigned_short_get(&slm_at_param_list, param_count - 1,
						   &res_inst_id);
		if (ret) {
			return ret;
		}

		uint16_t path[4] = { LWM2M_CARRIER_OBJECT_BINARY_APP_DATA_CONTAINER, inst_id, 0,
				     res_inst_id };
		uint8_t path_len = 4;

		if (param_count == 5) {
			size = CONFIG_SLM_CARRIER_APP_DATA_BUFFER_LEN;

			ret = util_string_get(&slm_at_param_list, 2, buffer, &size);
			if (ret) {
				return ret;
			}

			data = buffer;
		}

		ret = lwm2m_carrier_app_data_send(path, path_len, data, size);
	}

	return ret;
}

/* AT#XCARRIER="battery_level",<battery_level> */
static int do_carrier_device_battery_level(void)
{
	int ret;
	uint16_t battery_level;

	ret = at_params_unsigned_short_get(&slm_at_param_list, 2, &battery_level);
	if (ret) {
		return ret;
	}

	return lwm2m_carrier_battery_level_set(battery_level);
}

/* AT#XCARRIER="battery_status",<battery_status> */
static int do_carrier_device_battery_status(void)
{
	int ret, battery_status;

	ret = at_params_int_get(&slm_at_param_list, 2, &battery_status);
	if (ret) {
		return ret;
	}

	return lwm2m_carrier_battery_status_set(battery_status);
}

/* AT#XCARRIER="current",<power_source>,<current> */
static int do_carrier_device_current(void)
{
	int ret, current;
	uint16_t power_source;

	ret = at_params_unsigned_short_get(&slm_at_param_list, 2, &power_source);
	if (ret) {
		return ret;
	}

	ret = at_params_int_get(&slm_at_param_list, 3, &current);
	if (ret) {
		return ret;
	}

	return lwm2m_carrier_power_source_current_set((uint8_t)power_source, current);
}

/* AT#XCARRIER="error","add|remove",<error> */
static int do_carrier_device_error(void)
{
	int ret;
	int32_t error_code;
	char operation[7];
	int size = sizeof(operation);

	ret = util_string_get(&slm_at_param_list, 2, operation, &size);
	if (ret) {
		return ret;
	}

	ret = at_params_int_get(&slm_at_param_list, 3, &error_code);
	if (ret) {
		return ret;
	}

	if (slm_util_cmd_casecmp(operation, "ADD")) {
		return lwm2m_carrier_error_code_add(error_code);
	} else if (slm_util_cmd_casecmp(operation, "REMOVE")) {
		return lwm2m_carrier_error_code_remove(error_code);
	}

	LOG_DBG("AT#XCARRIER=\"error\" failed: invalid operation");

	return -EINVAL;
}

int memory_free_set(uint32_t memory_free)
{
	if (memory_free > INT32_MAX) {
		return -EINVAL;
	}

	m_mem_free = (int32_t)memory_free;

	return 0;
}

int lwm2m_carrier_memory_free_read(void)
{
	return m_mem_free;
}

/* AT#XCARRIER="memory_free","read|write"[,<memory>] */
static int do_carrier_device_mem_free(void)
{
	int ret, memory;
	char operation[6];
	int size = sizeof(operation);

	ret = util_string_get(&slm_at_param_list, 2, operation, &size);
	if (ret) {
		return ret;
	}

	if (slm_util_cmd_casecmp(operation, "read")) {
		memory = lwm2m_carrier_memory_free_read();

		rsp_send("\r\n#XCARRIER: %d\r\n", memory);
	} else if (slm_util_cmd_casecmp(operation, "write")) {
		ret = at_params_int_get(&slm_at_param_list, 3, &memory);
		if (ret) {
			return ret;
		}

		return memory_free_set(memory);
	}

	return 0;
}

/* AT#XCARRIER="memory_total",<memory> */
static int do_carrier_device_mem_total(void)
{
	int ret, memory_total;

	ret = at_params_int_get(&slm_at_param_list, 2, &memory_total);
	if (ret) {
		return ret;
	}

	return lwm2m_carrier_memory_total_set(memory_total);
}

/* AT#XCARRIER="power_sources"[,<source1>[<source2>[,...[,<source7>]]]] */
static int do_carrier_device_power_sources(void)
{
	int ret;
	uint8_t sources[7], count;
	uint16_t source;

	count = at_params_valid_count_get(&slm_at_param_list);
	if (count - 2 > sizeof(sources)) {
		LOG_DBG("AT#XCARRIER=\"power_sources\" failed: too many parameters");
		return -EINVAL;
	}

	for (int i = 2; i < count; i++) {
		ret = at_params_unsigned_short_get(&slm_at_param_list, i, &source);
		if (ret) {
			return ret;
		}

		sources[i - 2] = (uint8_t)source;
	}

	return lwm2m_carrier_avail_power_sources_set(sources, count - 2);
}

/* AT#XCARRIER="timezone","read|write"[,<timezone>] */
static int do_carrier_device_timezone(void)
{
	int ret;
	char operation[6];
	size_t size = sizeof(operation);

	ret = util_string_get(&slm_at_param_list, 2, operation, &size);
	if (ret) {
		return ret;
	}

	if (slm_util_cmd_casecmp(operation, "READ")) {
		const char *timezone;

		timezone = lwm2m_carrier_timezone_read();

		if (timezone) {
			rsp_send("\r\n#XCARRIER: %s\r\n", timezone);
		} else {
			rsp_send("\r\n#XCARRIER: \r\n");
		}

		return 0;
	} else if (slm_util_cmd_casecmp(operation, "WRITE")) {
		char timezone[64];

		size = sizeof(timezone);

		ret = util_string_get(&slm_at_param_list, 3, timezone, &size);
		if (ret) {
			return ret;
		}

		return lwm2m_carrier_timezone_write(timezone);
	}

	LOG_DBG("AT#XCARRIER=\"timezone\" failed: invalid operation");

	return -EINVAL;
}

#define TIME_STR_SIZE sizeof("1970-01-01T00:00:00Z")

static void print_utc_time(char *output, int32_t timestamp)
{
	time_t time = (time_t)timestamp;
	struct tm *tm = gmtime(&time);

	strftime(output, TIME_STR_SIZE, "%FT%TZ", tm);
}

/* AT#XCARRIER="time" */
static int do_carrier_device_time(void)
{
	int utc_time, utc_offset;
	const char *timezone = NULL;
	char time_str[TIME_STR_SIZE];

	lwm2m_carrier_time_read(&utc_time, &utc_offset, &timezone);

	print_utc_time(time_str, utc_time);

	if (timezone) {
		rsp_send("\r\n#XCARRIER: UTC_TIME: %s, UTC_OFFSET: %d, TIMEZONE: %s\r\n",
			 time_str, utc_offset, timezone);
	} else {
		rsp_send("\r\n#XCARRIER: UTC_TIME: %s, UTC_OFFSET: %d, TIMEZONE:\r\n",
			 time_str, utc_offset);
	}

	return 0;
}

/* AT#XCARRIER="utc_offset","read|write"[,<utc_offset>] */
static int do_carrier_device_utc_offset(void)
{
	int ret, utc_offset;
	char operation[6];
	size_t size = sizeof(operation);

	ret = util_string_get(&slm_at_param_list, 2, operation, &size);
	if (ret) {
		return ret;
	}

	if (slm_util_cmd_casecmp(operation, "READ")) {
		utc_offset = lwm2m_carrier_utc_offset_read();

		rsp_send("\r\n#XCARRIER: %d\r\n", utc_offset);

		return 0;
	} else if (slm_util_cmd_casecmp(operation, "WRITE")) {
		ret = at_params_int_get(&slm_at_param_list, 3, &utc_offset);
		if (ret) {
			return ret;
		}

		return lwm2m_carrier_utc_offset_write(utc_offset);
	}

	LOG_DBG("AT#XCARRIER=\"utc_offset\" failed: invalid operation");

	return -EINVAL;
}

/* AT#XCARRIER="utc_time","read|write"[,<utc_time>] */
static int do_carrier_device_utc_time(void)
{
	int ret, utc_time;
	char operation[6];
	size_t size = sizeof(operation);
	char time_str[TIME_STR_SIZE];

	ret = util_string_get(&slm_at_param_list, 2, operation, &size);
	if (ret) {
		return ret;
	}

	if (slm_util_cmd_casecmp(operation, "READ")) {
		utc_time = lwm2m_carrier_utc_time_read();
		print_utc_time(time_str, utc_time);

		rsp_send("\r\n#XCARRIER: %s\r\n", time_str);

		return 0;
	} else if (slm_util_cmd_casecmp(operation, "WRITE")) {
		ret = at_params_int_get(&slm_at_param_list, 3, &utc_time);
		if (ret) {
			return ret;
		}

		return lwm2m_carrier_utc_time_write(utc_time);
	}

	LOG_DBG("AT#XCARRIER=\"utc_time\" failed: invalid operation");

	return -EINVAL;
}

/* AT#XCARRIER="voltage",<power_source>,<voltage> */
static int do_carrier_device_voltage(void)
{
	int ret, voltage;
	uint16_t power_source;

	ret = at_params_unsigned_short_get(&slm_at_param_list, 2, &power_source);
	if (ret) {
		return ret;
	}

	ret = at_params_int_get(&slm_at_param_list, 3, &voltage);
	if (ret) {
		return ret;
	}

	return lwm2m_carrier_power_source_voltage_set((uint8_t)power_source, voltage);
}

/* AT#XCARRIER="log_data",<data> */
static int do_carrier_event_log_log_data(void)
{
	char data[CONFIG_SLM_CARRIER_APP_DATA_BUFFER_LEN] = {0};
	int size = CONFIG_SLM_CARRIER_APP_DATA_BUFFER_LEN;

	int ret = util_string_get(&slm_at_param_list, 2, data, &size);

	if (ret) {
		return ret;
	}

	return lwm2m_carrier_log_data_set(data, size);
}

/* AT#XCARRIER="position",<latitude>,<longitude>,<altitude>,<timestamp>,<uncertainty> */
static int do_carrier_location_position(void)
{
	int ret, param_count;
	double latitude, longitude;
	float altitude, uncertainty;
	uint32_t timestamp;

	param_count = at_params_valid_count_get(&slm_at_param_list);
	if (param_count != 7) {
		LOG_DBG("AT#XCARRIER=\"position\" failed: invalid number of arguments");
		return -EINVAL;
	}

	ret = util_string_to_double_get(&slm_at_param_list, 2, &latitude);
	if (ret) {
		return ret;
	}

	ret = util_string_to_double_get(&slm_at_param_list, 3, &longitude);
	if (ret) {
		return ret;
	}

	ret = util_string_to_float_get(&slm_at_param_list, 4, &altitude);
	if (ret) {
		return ret;
	}

	ret = at_params_unsigned_int_get(&slm_at_param_list, 5, &timestamp);
	if (ret) {
		return ret;
	}

	ret = util_string_to_float_get(&slm_at_param_list, 6, &uncertainty);
	if (ret) {
		return ret;
	}

	return lwm2m_carrier_location_set(latitude, longitude, altitude, timestamp, uncertainty);
}

/* AT#XCARRIER="velocity",<heading>,<speed_h>,<speed_v>,<uncertainty_h>,<uncertainty_v> */
static int do_carrier_location_velocity(void)
{
	int ret, param_count, heading;
	float speed_h, speed_v, uncertainty_h, uncertainty_v;

	param_count = at_params_valid_count_get(&slm_at_param_list);
	if (param_count != 7) {
		LOG_DBG("AT#XCARRIER=\"velocity\" failed: invalid number of arguments");
		return -EINVAL;
	}

	ret = at_params_int_get(&slm_at_param_list, 2, &heading);
	if (ret) {
		return ret;
	}

	ret = util_string_to_float_get(&slm_at_param_list, 3, &speed_h);
	if (ret) {
		return ret;
	}

	ret = util_string_to_float_get(&slm_at_param_list, 4, &speed_v);
	if (ret) {
		return ret;
	}

	ret = util_string_to_float_get(&slm_at_param_list, 5, &uncertainty_h);
	if (ret) {
		return ret;
	}

	ret = util_string_to_float_get(&slm_at_param_list, 6, &uncertainty_v);
	if (ret) {
		return ret;
	}

	return lwm2m_carrier_velocity_set(heading, speed_h, speed_v, uncertainty_h, uncertainty_v);
}

/* AT#XCARRIER="portfolio","create|read|write",<instance_id>[,<identity_type>[,<identity>]] */
static int do_carrier_portfolio(void)
{
	int ret, param_count;
	uint16_t instance_id, identity_type;
	char operation[7], buffer[64];
	size_t size = sizeof(operation);
	uint16_t buf_len = sizeof(buffer);

	ret = util_string_get(&slm_at_param_list, 2, operation, &size);
	if (ret) {
		return ret;
	}

	ret = at_params_unsigned_short_get(&slm_at_param_list, 3, &instance_id);
	if (ret) {
		return ret;
	}

	param_count = at_params_valid_count_get(&slm_at_param_list);
	if (param_count > 4) {
		ret = at_params_unsigned_short_get(&slm_at_param_list, 4, &identity_type);
		if (ret) {
			return ret;
		}
	}

	if (slm_util_cmd_casecmp(operation, "READ") && (param_count > 4)) {
		ret = lwm2m_carrier_identity_read(instance_id, identity_type, buffer, &buf_len);
		if (ret) {
			return ret;
		}

		rsp_send("\r\n#XCARRIER: %s\r\n", buffer);

		return 0;
	} else if (slm_util_cmd_casecmp(operation, "WRITE") && (param_count > 4)) {
		size = sizeof(buffer);

		ret = util_string_get(&slm_at_param_list, 5, buffer, &size);
		if (ret) {
			return ret;
		}

		return lwm2m_carrier_identity_write(instance_id, identity_type, buffer);
	} else if (slm_util_cmd_casecmp(operation, "CREATE")) {
		return lwm2m_carrier_portfolio_instance_create(instance_id);
	}

	LOG_DBG("AT#XCARRIER=\"portfolio\" failed: invalid operation");

	return -EINVAL;
}

/* AT#XCARRIER="reboot" */
static int do_carrier_request_reboot(void)
{
	return lwm2m_carrier_request(LWM2M_CARRIER_REQUEST_REBOOT);
}

/* AT#XCARRIER="link_down" */
static int do_carrier_request_link_down(void)
{
	return lwm2m_carrier_request(LWM2M_CARRIER_REQUEST_LINK_DOWN);
}

/* AT#XCARRIER="link_up" */
static int do_carrier_request_link_up(void)
{
	return lwm2m_carrier_request(LWM2M_CARRIER_REQUEST_LINK_UP);
}

/**@brief API to handle Carrier AT command
 */
int handle_at_carrier(enum at_cmd_type cmd_type)
{
	int ret;
	char op_str[SLM_CARRIER_OP_STR_MAX];
	int size = sizeof(op_str);

	if (cmd_type != AT_CMD_TYPE_SET_COMMAND) {
		return -EINVAL;
	}
	ret = util_string_get(&slm_at_param_list, 1, op_str, &size);
	if (ret) {
		return ret;
	}
	ret = -EINVAL;
	for (int i = 0; i < CARRIER_OP_MAX; i++) {
		if (slm_util_casecmp(op_str, op_list[i].op_str)) {
			ret = op_list[i].handler();
			break;
		}
	}

	return ret;
}

int slm_at_carrier_init(void)
{
	k_work_init_delayable(&reconnect_work, reconnect_wk);

	return 0;
}

int slm_at_carrier_uninit(void)
{
	return 0;
}
