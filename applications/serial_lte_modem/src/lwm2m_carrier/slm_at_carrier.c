/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <stdint.h>
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

/* Static variable to report the memory free resource. */
static int m_mem_free;

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
			"Firmware update failed",
		[LWM2M_CARRIER_ERROR_CONFIGURATION] =
			"Illegal object configuration detected",
		[LWM2M_CARRIER_ERROR_INIT] =
			"Initialization failure",
		[LWM2M_CARRIER_ERROR_RUN] =
			"Configuration failure",
		[LWM2M_CARRIER_ERROR_CONNECT] =
			"Connection failure",
	};

	__ASSERT(PART_OF_ARRAY(strerr[err->type]),
		 "Unhandled liblwm2m_carrier error");

	LOG_ERR("LWM2M_CARRIER_EVENT_ERROR: %s, reason %d", strerr[err->type], err->value);

	rsp_send("\r\n#XCARRIEREVT: %u,%u,%d\r\n",
		 LWM2M_CARRIER_EVENT_ERROR, err->type, err->value);
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

	rsp_send("\r\n#XCARRIEREVT: %u,%u,%d\r\n",
		 LWM2M_CARRIER_EVENT_DEFERRED, def->reason, def->timeout);
}

static void on_event_app_data(const lwm2m_carrier_event_t *event)
{
	int ret;
	lwm2m_carrier_event_app_data_t *app_data = event->data.app_data;

	if (app_data->path_len > ARRAY_SIZE(app_data->path)) {
		LOG_ERR("on_event_app_data: invalid path length");
		return;
	}

	/* Longest possible URI path. */
	char uri_path[sizeof(STRINGIFY(65535)) * ARRAY_SIZE(app_data->path) + 1];
	uint32_t off = 0;

	for (int i = 0; i < app_data->path_len; i++) {
		off += snprintf(&uri_path[off], sizeof(uri_path) - off, "/%hu", app_data->path[i]);
		if (off >= sizeof(uri_path)) {
			LOG_ERR("on_event_app_data: insufficient memory");
			return;
		}
	}

	if (app_data->type == LWM2M_CARRIER_APP_DATA_EVENT_DATA_WRITE) {
		/* In theory, slm_data_buf should be twice as large as
		 * SLM_CARRIER_APP_DATA_BUFFER_LEN to account for the hex string.
		 * However, slm_data_buf is not expected to receive more than 512 bytes in downlink.
		 */
		ret = slm_util_htoa(app_data->buffer, app_data->buffer_len,
				    slm_data_buf, sizeof(slm_data_buf));
		if (ret < 0) {
			LOG_ERR("Failed to encode hex array to hex string: %d", ret);
			return;
		}

		rsp_send("\r\n#XCARRIEREVT: %u,%hhu,\"%s\",%d\r\n\"", event->type, app_data->type,
			 uri_path, ret);
		data_send(slm_data_buf, ret);
		rsp_send("\"");
	} else {
		rsp_send("\r\n#XCARRIEREVT: %u,%hhu,\"%s\"\r\n", event->type, app_data->type,
			 uri_path);
	}
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
		err = slm_util_at_printf("AT+CFUN=0");
		break;
	case LWM2M_CARRIER_EVENT_BOOTSTRAPPED:
		LOG_DBG("LWM2M_CARRIER_EVENT_BOOTSTRAPPED");
		break;
	case LWM2M_CARRIER_EVENT_REGISTERED:
		LOG_DBG("LWM2M_CARRIER_EVENT_REGISTERED");
		break;
	case LWM2M_CARRIER_EVENT_DEREGISTERED:
		LOG_DBG("LWM2M_CARRIER_EVENT_DEREGISTERED");
		break;
	case LWM2M_CARRIER_EVENT_DEFERRED:
		print_deferred(event);
		return 0;
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
	case LWM2M_CARRIER_EVENT_MODEM_DOMAIN:
		LOG_DBG("LWM2M_CARRIER_EVENT_MODEM_DOMAIN");
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
	case LWM2M_CARRIER_EVENT_ERROR_CODE_RESET:
		LOG_DBG("LWM2M_CARRIER_EVENT_ERROR_CODE_RESET");
		break;
	case LWM2M_CARRIER_EVENT_ERROR:
		print_err(event);
		return 0;
	}

	rsp_send("\r\n#XCARRIEREVT: %d,%d\r\n", event->type, err);

	return err;
}

/* Carrier App Data Set data mode handler */
static int carrier_datamode_callback(uint8_t op, const uint8_t *data, int len, uint8_t flags)
{
	int ret = 0;

	if (op == DATAMODE_SEND) {
		if ((flags & SLM_DATAMODE_FLAGS_MORE_DATA) != 0) {
			LOG_ERR("Datamode buffer overflow");
			exit_datamode_handler(-EOVERFLOW);
			return -EOVERFLOW;
		}

		size_t size = CONFIG_SLM_CARRIER_APP_DATA_BUFFER_LEN / 2;

		ret = slm_util_atoh(data, len, slm_data_buf, size);
		if (ret < 0) {
			LOG_ERR("Failed to decode hex string to hex array");
			return ret;
		}

		uint16_t path[3] = { LWM2M_CARRIER_OBJECT_APP_DATA_CONTAINER, 0, 0 };
		uint8_t path_len = 3;

		ret = lwm2m_carrier_app_data_set(path, path_len, slm_data_buf, ret);
		LOG_INF("datamode send: %d", ret);
		if (ret < 0) {
			exit_datamode_handler(ret);
		}
	} else if (op == DATAMODE_EXIT) {
		LOG_DBG("datamode exit");
	}

	return ret;
}

/* AT#XCARRIER="app_data_create",<obj_inst_id>,<res_inst_id> */
SLM_AT_CMD_CUSTOM(xcarrier_app_data_create, "AT#XCARRIER=\"app_data_create\"",
		  do_carrier_appdata_create);
static int do_carrier_appdata_create(enum at_cmd_type, const struct at_param_list *param_list,
				     uint32_t param_count)
{
	int ret;
	uint16_t inst_id, res_inst_id;

	if (param_count != 4) {
		return -EINVAL;
	}

	ret = at_params_unsigned_short_get(param_list, param_count - 2, &inst_id);
	if (ret) {
		return ret;
	}

	ret = at_params_unsigned_short_get(param_list, param_count - 1, &res_inst_id);
	if (ret) {
		return ret;
	}

	uint16_t path[4] = { LWM2M_CARRIER_OBJECT_BINARY_APP_DATA_CONTAINER, inst_id, 0,
			     res_inst_id };
	uint8_t path_len = 4;

	ret = lwm2m_carrier_app_data_set(path, path_len, "", 0);

	return ret;
}

/* AT#XCARRIER="app_data_set"[,<data>][,<obj_inst_id>,<res_inst_id>] */
SLM_AT_CMD_CUSTOM(xcarrier_app_data_set, "AT#XCARRIER=\"app_data_set\"", do_carrier_appdata_set);
static int do_carrier_appdata_set(enum at_cmd_type, const struct at_param_list *param_list,
				  uint32_t param_count)
{
	if (param_count == 2) {
		/* enter data mode */
		return enter_datamode(carrier_datamode_callback);
	} else if ((param_count < 2) || (param_count > 5)) {
		return -EINVAL;
	}

	int ret = 0;

	char data_ascii[CONFIG_SLM_CARRIER_APP_DATA_BUFFER_LEN] = {0};
	size_t data_ascii_len = CONFIG_SLM_CARRIER_APP_DATA_BUFFER_LEN;

	char data_hex[CONFIG_SLM_CARRIER_APP_DATA_BUFFER_LEN / 2];
	size_t data_hex_len = CONFIG_SLM_CARRIER_APP_DATA_BUFFER_LEN / 2;

	if (param_count == 3) {
		uint16_t path[3] = { LWM2M_CARRIER_OBJECT_APP_DATA_CONTAINER, 0, 0 };
		uint8_t path_len = 3;

		ret = util_string_get(param_list, 2, data_ascii, &data_ascii_len);
		if (ret) {
			return ret;
		}

		ret = slm_util_atoh(data_ascii, data_ascii_len, data_hex, data_hex_len);
		if (ret < 0) {
			LOG_ERR("Failed to decode hex string to hex array");
			return ret;
		}

		ret = lwm2m_carrier_app_data_set(path, path_len, data_hex, ret);
	} else if (param_count == 4 || param_count == 5) {
		uint8_t *data = NULL;
		int size = 0;

		uint16_t inst_id;
		uint16_t res_inst_id;

		ret = at_params_unsigned_short_get(param_list, param_count - 2, &inst_id);
		if (ret) {
			return ret;
		}

		ret = at_params_unsigned_short_get(param_list, param_count - 1,
						   &res_inst_id);
		if (ret) {
			return ret;
		}

		uint16_t path[4] = { LWM2M_CARRIER_OBJECT_BINARY_APP_DATA_CONTAINER, inst_id, 0,
				     res_inst_id };
		uint8_t path_len = 4;

		if (param_count == 5) {
			ret = util_string_get(param_list, 2, data_ascii, &data_ascii_len);
			if (ret) {
				return ret;
			}

			ret = slm_util_atoh(data_ascii, data_ascii_len, data_hex, data_hex_len);
			if (ret < 0) {
				LOG_ERR("Failed to decode hex string to hex array");
				return ret;
			}

			data = data_hex;
			size = ret;
		}

		ret = lwm2m_carrier_app_data_set(path, path_len, data, size);
	}

	return ret;
}

/* AT#XCARRIER="battery_level",<battery_level> */
SLM_AT_CMD_CUSTOM(xcarrier_battery_level, "AT#XCARRIER=\"battery_level\"",
	      do_carrier_device_battery_level);
static int do_carrier_device_battery_level(enum at_cmd_type, const struct at_param_list *param_list,
					   uint32_t)
{
	int ret;
	uint16_t battery_level;

	ret = at_params_unsigned_short_get(param_list, 2, &battery_level);
	if (ret) {
		return ret;
	}

	return lwm2m_carrier_battery_level_set(battery_level);
}

/* AT#XCARRIER="battery_status",<battery_status> */
SLM_AT_CMD_CUSTOM(xcarrier_battery_status, "AT#XCARRIER=\"battery_status\"",
	      do_carrier_device_battery_status);
static int do_carrier_device_battery_status(enum at_cmd_type,
					    const struct at_param_list *param_list, uint32_t)
{
	int ret, battery_status;

	ret = at_params_int_get(param_list, 2, &battery_status);
	if (ret) {
		return ret;
	}

	return lwm2m_carrier_battery_status_set(battery_status);
}

/* AT#XCARRIER="current",<power_source>,<current> */
SLM_AT_CMD_CUSTOM(xcarrier_current, "AT#XCARRIER=\"current\"", do_carrier_device_current);
static int do_carrier_device_current(enum at_cmd_type, const struct at_param_list *param_list,
				     uint32_t)
{
	int ret, current;
	uint16_t power_source;

	ret = at_params_unsigned_short_get(param_list, 2, &power_source);
	if (ret) {
		return ret;
	}

	ret = at_params_int_get(param_list, 3, &current);
	if (ret) {
		return ret;
	}

	return lwm2m_carrier_power_source_current_set((uint8_t)power_source, current);
}

/* AT#XCARRIER="error","add|remove",<error> */
SLM_AT_CMD_CUSTOM(xcarrier_error, "AT#XCARRIER=\"error\"", do_carrier_device_error);
static int do_carrier_device_error(enum at_cmd_type, const struct at_param_list *param_list,
				   uint32_t)
{
	int ret;
	int32_t error_code;
	char operation[7];
	int size = sizeof(operation);

	ret = util_string_get(param_list, 2, operation, &size);
	if (ret) {
		return ret;
	}

	ret = at_params_int_get(param_list, 3, &error_code);
	if (ret) {
		return ret;
	}

	if (slm_util_casecmp(operation, "ADD")) {
		return lwm2m_carrier_error_code_add(error_code);
	} else if (slm_util_casecmp(operation, "REMOVE")) {
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
SLM_AT_CMD_CUSTOM(xcarrier_memory_free, "AT#XCARRIER=\"memory_free\"", do_carrier_device_mem_free);
static int do_carrier_device_mem_free(enum at_cmd_type, const struct at_param_list *param_list,
				      uint32_t)
{
	int ret, memory;
	char operation[6];
	int size = sizeof(operation);

	ret = util_string_get(param_list, 2, operation, &size);
	if (ret) {
		return ret;
	}

	if (slm_util_casecmp(operation, "read")) {
		memory = lwm2m_carrier_memory_free_read();

		rsp_send("\r\n#XCARRIER: %d\r\n", memory);
	} else if (slm_util_casecmp(operation, "write")) {
		ret = at_params_int_get(param_list, 3, &memory);
		if (ret) {
			return ret;
		}

		return memory_free_set(memory);
	}

	return 0;
}

/* AT#XCARRIER="memory_total",<memory> */
SLM_AT_CMD_CUSTOM(xcarrier_memory_total, "AT#XCARRIER=\"memory_total\"",
	      do_carrier_device_mem_total);
static int do_carrier_device_mem_total(enum at_cmd_type, const struct at_param_list *param_list,
				       uint32_t)
{
	int ret, memory_total;

	ret = at_params_int_get(param_list, 2, &memory_total);
	if (ret) {
		return ret;
	}

	return lwm2m_carrier_memory_total_set(memory_total);
}

/* AT#XCARRIER="power_sources"[,<source1>[<source2>[,...[,<source7>]]]] */
SLM_AT_CMD_CUSTOM(xcarrier_power_sources, "AT#XCARRIER=\"power_sources\"",
	      do_carrier_device_power_sources);
static int do_carrier_device_power_sources(enum at_cmd_type, const struct at_param_list *param_list,
					   uint32_t param_count)
{
	int ret;
	uint8_t sources[7];
	uint16_t source;

	if (param_count - 2 > sizeof(sources)) {
		LOG_DBG("AT#XCARRIER=\"power_sources\" failed: too many parameters");
		return -EINVAL;
	}

	for (int i = 2; i < param_count; i++) {
		ret = at_params_unsigned_short_get(param_list, i, &source);
		if (ret) {
			return ret;
		}

		sources[i - 2] = (uint8_t)source;
	}

	return lwm2m_carrier_avail_power_sources_set(sources, param_count - 2);
}

/* AT#XCARRIER="timezone",<timezone> */
SLM_AT_CMD_CUSTOM(xcarrier_timezone, "AT#XCARRIER=\"timezone\"", do_carrier_device_timezone);
static int do_carrier_device_timezone(enum at_cmd_type, const struct at_param_list *param_list,
				      uint32_t)
{
	int ret;
	char operation[6];
	size_t size = sizeof(operation);

	ret = util_string_get(param_list, 2, operation, &size);
	if (ret) {
		return ret;
	}

	if (slm_util_casecmp(operation, "READ")) {
		const char *timezone;

		timezone = lwm2m_carrier_timezone_read();

		if (timezone) {
			rsp_send("\r\n#XCARRIER: %s\r\n", timezone);
		} else {
			rsp_send("\r\n#XCARRIER: \r\n");
		}

		return 0;
	} else if (slm_util_casecmp(operation, "WRITE")) {
		char timezone[64];

		size = sizeof(timezone);

		ret = util_string_get(param_list, 3, timezone, &size);
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
SLM_AT_CMD_CUSTOM(xcarrier_time, "AT#XCARRIER=\"time\"", do_carrier_device_time);
static int do_carrier_device_time(enum at_cmd_type, const struct at_param_list *, uint32_t)
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
SLM_AT_CMD_CUSTOM(xcarrier_utc_offset, "AT#XCARRIER=\"utc_offset\"",
	      do_carrier_device_utc_offset);
static int do_carrier_device_utc_offset(enum at_cmd_type, const struct at_param_list *param_list,
					uint32_t)
{
	int ret, utc_offset;
	char operation[6];
	size_t size = sizeof(operation);

	ret = util_string_get(param_list, 2, operation, &size);
	if (ret) {
		return ret;
	}

	if (slm_util_casecmp(operation, "READ")) {
		utc_offset = lwm2m_carrier_utc_offset_read();

		rsp_send("\r\n#XCARRIER: %d\r\n", utc_offset);

		return 0;
	} else if (slm_util_casecmp(operation, "WRITE")) {
		ret = at_params_int_get(param_list, 3, &utc_offset);
		if (ret) {
			return ret;
		}

		return lwm2m_carrier_utc_offset_write(utc_offset);
	}

	LOG_DBG("AT#XCARRIER=\"utc_offset\" failed: invalid operation");

	return -EINVAL;
}

/* AT#XCARRIER="utc_time","read|write"[,<utc_time>] */
SLM_AT_CMD_CUSTOM(xcarrier_utc_time, "AT#XCARRIER=\"utc_time\"", do_carrier_device_utc_time);
static int do_carrier_device_utc_time(enum at_cmd_type, const struct at_param_list *param_list,
				      uint32_t)
{
	int ret, utc_time;
	char operation[6];
	size_t size = sizeof(operation);
	char time_str[TIME_STR_SIZE];

	ret = util_string_get(param_list, 2, operation, &size);
	if (ret) {
		return ret;
	}

	if (slm_util_casecmp(operation, "READ")) {
		utc_time = lwm2m_carrier_utc_time_read();
		print_utc_time(time_str, utc_time);

		rsp_send("\r\n#XCARRIER: %s\r\n", time_str);

		return 0;
	} else if (slm_util_casecmp(operation, "WRITE")) {
		ret = at_params_int_get(param_list, 3, &utc_time);
		if (ret) {
			return ret;
		}

		return lwm2m_carrier_utc_time_write(utc_time);
	}

	LOG_DBG("AT#XCARRIER=\"utc_time\" failed: invalid operation");

	return -EINVAL;
}

/* AT#XCARRIER="voltage",<power_source>,<voltage> */
SLM_AT_CMD_CUSTOM(xcarrier_voltage, "AT#XCARRIER=\"voltage\"", do_carrier_device_voltage);
static int do_carrier_device_voltage(enum at_cmd_type, const struct at_param_list *param_list,
				     uint32_t)
{
	int ret, voltage;
	uint16_t power_source;

	ret = at_params_unsigned_short_get(param_list, 2, &power_source);
	if (ret) {
		return ret;
	}

	ret = at_params_int_get(param_list, 3, &voltage);
	if (ret) {
		return ret;
	}

	return lwm2m_carrier_power_source_voltage_set((uint8_t)power_source, voltage);
}

/* AT#XCARRIER="log_data",<data> */
SLM_AT_CMD_CUSTOM(xcarrier_log_data, "AT#XCARRIER=\"log_data\"",
	      do_carrier_event_log_log_data);
static int do_carrier_event_log_log_data(enum at_cmd_type, const struct at_param_list *param_list,
					 uint32_t)
{
	char data_ascii[CONFIG_SLM_CARRIER_APP_DATA_BUFFER_LEN] = {0};
	size_t data_ascii_len = CONFIG_SLM_CARRIER_APP_DATA_BUFFER_LEN;

	char data_hex[CONFIG_SLM_CARRIER_APP_DATA_BUFFER_LEN / 2];
	size_t data_hex_len = CONFIG_SLM_CARRIER_APP_DATA_BUFFER_LEN / 2;

	int ret = util_string_get(param_list, 2, data_ascii, &data_ascii_len);

	if (ret) {
		return ret;
	}

	ret = slm_util_atoh(data_ascii, data_ascii_len, data_hex, data_hex_len);
	if (ret < 0) {
		LOG_ERR("Failed to decode hex string to hex array");
		return ret;
	}

	return lwm2m_carrier_log_data_set(data_hex, ret);
}

/* AT#XCARRIER="position",<latitude>,<longitude>,<altitude>,<timestamp>,<uncertainty> */
SLM_AT_CMD_CUSTOM(xcarrier_position, "AT#XCARRIER=\"position\"",
	      do_carrier_location_position);
static int do_carrier_location_position(enum at_cmd_type, const struct at_param_list *param_list,
					uint32_t param_count)
{
	int ret;
	double latitude, longitude;
	float altitude, uncertainty;
	uint32_t timestamp;

	if (param_count != 7) {
		LOG_DBG("AT#XCARRIER=\"position\" failed: invalid number of arguments");
		return -EINVAL;
	}

	ret = util_string_to_double_get(param_list, 2, &latitude);
	if (ret) {
		return ret;
	}

	ret = util_string_to_double_get(param_list, 3, &longitude);
	if (ret) {
		return ret;
	}

	ret = util_string_to_float_get(param_list, 4, &altitude);
	if (ret) {
		return ret;
	}

	ret = at_params_unsigned_int_get(param_list, 5, &timestamp);
	if (ret) {
		return ret;
	}

	ret = util_string_to_float_get(param_list, 6, &uncertainty);
	if (ret) {
		return ret;
	}

	return lwm2m_carrier_location_set(latitude, longitude, altitude, timestamp, uncertainty);
}

/* AT#XCARRIER="velocity",<heading>,<speed_h>,<speed_v>,<uncertainty_h>,<uncertainty_v> */
SLM_AT_CMD_CUSTOM(xcarrier_velocity, "AT#XCARRIER=\"velocity\"",
	      do_carrier_location_velocity);
static int do_carrier_location_velocity(enum at_cmd_type, const struct at_param_list *param_list,
					uint32_t param_count)
{
	int ret, heading;
	float speed_h, speed_v, uncertainty_h, uncertainty_v;

	if (param_count != 7) {
		LOG_DBG("AT#XCARRIER=\"velocity\" failed: invalid number of arguments");
		return -EINVAL;
	}

	ret = at_params_int_get(param_list, 2, &heading);
	if (ret) {
		return ret;
	}

	ret = util_string_to_float_get(param_list, 3, &speed_h);
	if (ret) {
		return ret;
	}

	ret = util_string_to_float_get(param_list, 4, &speed_v);
	if (ret) {
		return ret;
	}

	ret = util_string_to_float_get(param_list, 5, &uncertainty_h);
	if (ret) {
		return ret;
	}

	ret = util_string_to_float_get(param_list, 6, &uncertainty_v);
	if (ret) {
		return ret;
	}

	return lwm2m_carrier_velocity_set(heading, speed_h, speed_v, uncertainty_h, uncertainty_v);
}

/* AT#XCARRIER="portfolio","create|read|write",<obj_inst_id>[,<identity_type>[,<identity>]] */
SLM_AT_CMD_CUSTOM(xcarrier_portfolio, "AT#XCARRIER=\"portfolio\"", do_carrier_portfolio);
static int do_carrier_portfolio(enum at_cmd_type, const struct at_param_list *param_list,
				uint32_t param_count)
{
	int ret;
	uint16_t instance_id, identity_type;
	char operation[7], buffer[64];
	size_t size = sizeof(operation);
	uint16_t buf_len = sizeof(buffer);

	ret = util_string_get(param_list, 2, operation, &size);
	if (ret) {
		return ret;
	}

	ret = at_params_unsigned_short_get(param_list, 3, &instance_id);
	if (ret) {
		return ret;
	}

	if (param_count > 4) {
		ret = at_params_unsigned_short_get(param_list, 4, &identity_type);
		if (ret) {
			return ret;
		}
	}

	if (slm_util_casecmp(operation, "READ") && (param_count > 4)) {
		ret = lwm2m_carrier_identity_read(instance_id, identity_type, buffer, &buf_len);
		if (ret) {
			return ret;
		}

		rsp_send("\r\n#XCARRIER: %s\r\n", buffer);

		return 0;
	} else if (slm_util_casecmp(operation, "WRITE") && (param_count > 4)) {
		size = sizeof(buffer);

		ret = util_string_get(param_list, 5, buffer, &size);
		if (ret) {
			return ret;
		}

		return lwm2m_carrier_identity_write(instance_id, identity_type, buffer);
	} else if (slm_util_casecmp(operation, "CREATE")) {
		return lwm2m_carrier_portfolio_instance_create(instance_id);
	}

	LOG_DBG("AT#XCARRIER=\"portfolio\" failed: invalid operation");

	return -EINVAL;
}

/* AT#XCARRIER="reboot" */
SLM_AT_CMD_CUSTOM(xcarrier_reboot, "AT#XCARRIER=\"reboot\"", do_carrier_request_reboot);
static int do_carrier_request_reboot(enum at_cmd_type, const struct at_param_list *, uint32_t)
{
	return lwm2m_carrier_request(LWM2M_CARRIER_REQUEST_REBOOT);
}

/* AT#XCARRIER="regup" */
SLM_AT_CMD_CUSTOM(xcarrier_regup, "AT#XCARRIER=\"regup\"", do_carrier_request_regup);
static int do_carrier_request_regup(enum at_cmd_type, const struct at_param_list *, uint32_t)
{
	return lwm2m_carrier_request(LWM2M_CARRIER_REQUEST_REGISTER);
}

/* AT#XCARRIER="dereg" */
SLM_AT_CMD_CUSTOM(xcarrier_dereg, "AT#XCARRIER=\"dereg\"", do_carrier_request_dereg);
static int do_carrier_request_dereg(enum at_cmd_type, const struct at_param_list *, uint32_t)
{
	return lwm2m_carrier_request(LWM2M_CARRIER_REQUEST_DEREGISTER);
}

/* AT#XCARRIER="link_down" */
SLM_AT_CMD_CUSTOM(xcarrier_link_down, "AT#XCARRIER=\"link_down\"",
	      do_carrier_request_link_down);
static int do_carrier_request_link_down(enum at_cmd_type, const struct at_param_list *, uint32_t)
{
	return lwm2m_carrier_request(LWM2M_CARRIER_REQUEST_LINK_DOWN);
}

/* AT#XCARRIER="link_up" */
SLM_AT_CMD_CUSTOM(xcarrier_link_up, "AT#XCARRIER=\"link_up\"", do_carrier_request_link_up);
static int do_carrier_request_link_up(enum at_cmd_type, const struct at_param_list *, uint32_t)
{
	return lwm2m_carrier_request(LWM2M_CARRIER_REQUEST_LINK_UP);
}

/* AT#XCARRIER="send",<obj_id>,<obj_inst_id>,<res_id>[,<res_inst_id>] */
SLM_AT_CMD_CUSTOM(xcarrier_send, "AT#XCARRIER=\"send\"", do_carrier_send);
static int do_carrier_send(enum at_cmd_type, const struct at_param_list *param_list,
			   uint32_t param_count)
{
	int ret = 0;

	if (param_count != 5 && param_count != 6) {
		LOG_DBG("AT#XCARRIER=\"send\" failed: invalid number of arguments");
		return -EINVAL;
	}

	uint16_t path[4];
	uint8_t path_len = 0;

	for (int i = 2; i < param_count; i++) {
		ret = at_params_unsigned_short_get(param_list, i, &path[i - 2]);
		if (ret) {
			return ret;
		}

		++path_len;
	}

	return lwm2m_carrier_data_send(path, path_len);
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
