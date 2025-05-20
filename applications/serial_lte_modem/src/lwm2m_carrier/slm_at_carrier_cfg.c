/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <stdio.h>
#include <string.h>
#include <zephyr/logging/log.h>
#include <lwm2m_carrier.h>
#include <lwm2m_settings.h>
#include "slm_at_host.h"
#include "slm_settings.h"
#include "slm_util.h"

LOG_MODULE_REGISTER(slm_carrier_cfg, CONFIG_SLM_LOG_LEVEL);

/* AT#XCARRIERCFG="apn"[,<apn>] */
SLM_AT_CMD_CUSTOM(xcarriercfg_apn, "AT#XCARRIERCFG=\"apn\"", do_cfg_apn);
static int do_cfg_apn(enum at_parser_cmd_type, struct at_parser *parser,
		      uint32_t param_count)
{
	if (param_count == 2) {
		rsp_send("\r\n#XCARRIERCFG: %s\r\n", lwm2m_settings_apn_get());
		return 0;
	} else if (param_count != 3) {
		return -EINVAL;
	}

	char apn[64];
	int ret, size = sizeof(apn);

	ret = util_string_get(parser, 2, apn, &size);
	if (ret) {
		return ret;
	}

	return lwm2m_settings_apn_set(apn);
}

/* AT#XCARRIERCFG="auto_startup"[,<0|1>] */
SLM_AT_CMD_CUSTOM(xcarriercfg_auto_startup, "AT#XCARRIERCFG=\"auto_startup\"", do_cfg_auto_startup);
static int do_cfg_auto_startup(enum at_parser_cmd_type, struct at_parser *parser,
			       uint32_t param_count)
{
#if !defined(CONFIG_SLM_CARRIER_AUTO_STARTUP)
	if (param_count == 2) {
		rsp_send("\r\n#XCARRIERCFG: %u\r\n", lwm2m_settings_auto_startup_get());
		return 0;
	} else if (param_count != 3) {
		return -EINVAL;
	}

	int ret;
	uint16_t auto_startup;

	ret = at_parser_num_get(parser, 2, &auto_startup);
	if (ret) {
		return ret;
	}

	if (auto_startup > 1) {
		LOG_ERR("AT#XCARRIERCFG=\"auto_startup\" failed: must be 0 or 1");
		return -EINVAL;
	}

	return lwm2m_settings_auto_startup_set(auto_startup);
#else
	LOG_ERR("AT#XCARRIERCFG=\"auto_startup\" not available when"
		" CONFIG_SLM_CARRIER_AUTO_STARTUP is enabled");

	return -1;
#endif
}

/* AT#XCARRIERCFG="auto_register"[,<0|1>] */
SLM_AT_CMD_CUSTOM(xcarriercfg_auto_register, "AT#XCARRIERCFG=\"auto_register\"",
		  do_cfg_auto_register);
static int do_cfg_auto_register(enum at_parser_cmd_type, struct at_parser *parser,
				uint32_t param_count)
{
	if (param_count == 2) {
		rsp_send("\r\n#XCARRIERCFG: %u\r\n", lwm2m_settings_auto_register_get());
		return 0;
	} else if (param_count != 3) {
		return -EINVAL;
	}

	int ret;
	uint16_t auto_register;

	ret = at_parser_num_get(parser, 2, &auto_register);
	if (ret) {
		return ret;
	}

	if (auto_register > 1) {
		LOG_ERR("AT#XCARRIERCFG=\"auto_register\" failed: must be 0 or 1");
		return -EINVAL;
	}

	return lwm2m_settings_auto_register_set(auto_register);
}

const static uint32_t carriers_enabled_map[] = {
	LWM2M_CARRIER_GENERIC,
	LWM2M_CARRIER_VERIZON,
	LWM2M_CARRIER_BELL_CA,
	LWM2M_CARRIER_LG_UPLUS,
	LWM2M_CARRIER_T_MOBILE,
	LWM2M_CARRIER_SOFTBANK
};

static const char *carriers_enabled_str(void)
{
	static char oper_str[17] = { 0 };
	uint32_t carriers_enabled = lwm2m_settings_carriers_enabled_get();
	int offset = 0;

	if (carriers_enabled == UINT32_MAX) {
		return "all";
	} else if (carriers_enabled == 0) {
		return "";
	}

	for (int i = 0; i < ARRAY_SIZE(carriers_enabled_map); i++) {
		if ((carriers_enabled & carriers_enabled_map[i]) && (offset < sizeof(oper_str))) {
			offset += snprintf(&oper_str[offset], sizeof(oper_str) - offset, "%s%u",
					   (offset == 0) ? "" : ", ", i);
		}
	}

	return oper_str;
}

/* AT#XCARRIERCFG="carriers"[,"all"|<carrier1>[<carrier2>[,...[,<carrier6>]]]] */
SLM_AT_CMD_CUSTOM(xcarriercfg_carriers, "AT#XCARRIERCFG=\"carriers\"", do_cfg_carriers);
static int do_cfg_carriers(enum at_parser_cmd_type, struct at_parser *parser,
			   uint32_t param_count)
{
	if (param_count == 2) {
		rsp_send("\r\n#XCARRIERCFG: %s\r\n", carriers_enabled_str());
		return 0;
	} else if (param_count - 2 > ARRAY_SIZE(carriers_enabled_map)) {
		LOG_ERR("AT#XCARRIERCFG=\"carriers\" failed: too many parameters");
		return -EINVAL;
	}

	uint32_t carriers_enabled = 0;
	int ret, size = sizeof("all");
	char size_buf[size];

	ret = util_string_get(parser, 2, size_buf, &size);
	if (!ret && slm_util_casecmp(size_buf, "all")) {
		carriers_enabled = UINT32_MAX;
	} else {
		for (int i = 2; i < param_count; i++) {
			uint16_t carrier;

			ret = at_parser_num_get(parser, i, &carrier);
			if (ret || (carrier >= ARRAY_SIZE(carriers_enabled_map))) {
				LOG_ERR("AT#XCARRIERCFG=\"carriers\" failed: illegal operator");
				return -EINVAL;
			}

			carriers_enabled |= carriers_enabled_map[carrier];
		}
	}

	return lwm2m_settings_carriers_enabled_set(carriers_enabled);
}

/* AT#XCARRIERCFG="coap_con_interval"[,<interval>] */
SLM_AT_CMD_CUSTOM(xcarriercfg_coap_con_interval, "AT#XCARRIERCFG=\"coap_con_interval\"",
	      do_cfg_coap_con_interval);
static int do_cfg_coap_con_interval(enum at_parser_cmd_type, struct at_parser *parser,
				    uint32_t param_count)
{
	if (param_count == 2) {
		rsp_send("\r\n#XCARRIERCFG: %d\r\n", lwm2m_settings_coap_con_interval_get());
		return 0;
	} else if (param_count != 3) {
		return -EINVAL;
	}

	int ret, coap_con_interval;

	ret = at_parser_num_get(parser, 2, &coap_con_interval);
	if (ret) {
		return ret;
	}

	if (coap_con_interval < -1 || coap_con_interval > 86400) {
		LOG_ERR("AT#XCARRIERCFG=\"coap_con_interval\" failed: value must be "
			"-1 or between 0 and 86400 (24 hours)");
		return -EINVAL;
	}

	return lwm2m_settings_coap_con_interval_set(coap_con_interval);
}

/* AT#XCARRIERCFG="download_timeout"[,<timeout>] */
SLM_AT_CMD_CUSTOM(xcarriercfg_download_timeout, "AT#XCARRIERCFG=\"download_timeout\"",
	      do_cfg_firmware_download_timeout);
static int do_cfg_firmware_download_timeout(enum at_parser_cmd_type,
					    struct at_parser *parser,
					    uint32_t param_count)
{
	if (param_count == 2) {
		rsp_send("\r\n#XCARRIERCFG: %hu\r\n",
			 lwm2m_settings_firmware_download_timeout_get());
		return 0;
	} else if (param_count != 3) {
		return -EINVAL;
	}

	int ret;
	uint16_t firmware_download_timeout;

	ret = at_parser_num_get(parser, 2, &firmware_download_timeout);
	if (ret) {
		return ret;
	}

	return lwm2m_settings_firmware_download_timeout_set(firmware_download_timeout);
}

/* AT#XCARRIERCFG="config_enable"[,<0|1>] */
SLM_AT_CMD_CUSTOM(xcarriercfg_config_enable, "AT#XCARRIERCFG=\"config_enable\"",
		  do_cfg_config_enable);
static int do_cfg_config_enable(enum at_parser_cmd_type, struct at_parser *parser,
				uint32_t param_count)
{
	if (param_count == 2) {
		rsp_send("\r\n#XCARRIERCFG: %u\r\n", lwm2m_settings_enable_custom_config_get());
		return 0;
	} else if (param_count != 3) {
		return -EINVAL;
	}

	int ret;
	uint16_t enabled;

	ret = at_parser_num_get(parser, 2, &enabled);
	if (ret) {
		return ret;
	}

	if (enabled > 1) {
		LOG_ERR("AT#XCARRIERCFG=\"enable\" failed: must be 0 or 1");
		return -EINVAL;
	}

	return lwm2m_settings_enable_custom_config_set(enabled);
}

/* AT#XCARRIERCFG="device_enable"[,<0|1>] */
SLM_AT_CMD_CUSTOM(xcarriercfg_device_enable, "AT#XCARRIERCFG=\"device_enable\"",
		  do_cfg_device_enable);
static int do_cfg_device_enable(enum at_parser_cmd_type, struct at_parser *parser,
				uint32_t param_count)
{
	if (param_count == 2) {
		rsp_send("\r\n#XCARRIERCFG: %u\r\n",
			 lwm2m_settings_enable_custom_device_config_get());
		return 0;
	} else if (param_count != 3) {
		return -EINVAL;
	}

	int ret;
	uint16_t enabled;

	ret = at_parser_num_get(parser, 2, &enabled);
	if (ret) {
		return ret;
	}

	if (enabled > 1) {
		LOG_ERR("AT#XCARRIERCFG=\"device_enable\" failed: must be 0 or 1");
		return -EINVAL;
	}

	return lwm2m_settings_enable_custom_device_config_set(enabled);
}

/* AT#XCARRIERCFG="device_type"[,<device_type>] */
SLM_AT_CMD_CUSTOM(xcarriercfg_device_type, "AT#XCARRIERCFG=\"device_type\"", do_cfg_device_type);
static int do_cfg_device_type(enum at_parser_cmd_type, struct at_parser *parser,
			      uint32_t param_count)
{
	if (param_count == 2) {
		rsp_send("\r\n#XCARRIERCFG: %s\r\n", lwm2m_settings_device_type_get());
		return 0;
	} else if (param_count != 3) {
		return -EINVAL;
	}

	int ret;
	char device_type[32];
	int size = sizeof(device_type);

	ret = util_string_get(parser, 2, device_type, &size);
	if (ret) {
		return ret;
	}

	return lwm2m_settings_device_type_set(device_type);
}

/* AT#XCARRIERCFG="hardware_version"[,<version>] */
SLM_AT_CMD_CUSTOM(xcarriercfg_hardware_version, "AT#XCARRIERCFG=\"hardware_version\"",
	      do_cfg_hardware_version);
static int do_cfg_hardware_version(enum at_parser_cmd_type, struct at_parser *parser,
				   uint32_t param_count)
{
	if (param_count == 2) {
		rsp_send("\r\n#XCARRIERCFG: %s\r\n", lwm2m_settings_hardware_version_get());
		return 0;
	} else if (param_count != 3) {
		return -EINVAL;
	}

	int ret;
	char hardware_version[32];
	int size = sizeof(hardware_version);

	ret = util_string_get(parser, 2, hardware_version, &size);
	if (ret) {
		return ret;
	}

	return lwm2m_settings_hardware_version_set(hardware_version);
}

/* AT#XCARRIERCFG="manufacturer"[,<manufacturer>] */
SLM_AT_CMD_CUSTOM(xcarriercfg_manufacturer, "AT#XCARRIERCFG=\"manufacturer\"", do_cfg_manufacturer);
static int do_cfg_manufacturer(enum at_parser_cmd_type, struct at_parser *parser,
			       uint32_t param_count)
{
	if (param_count == 2) {
		rsp_send("\r\n#XCARRIERCFG: %s\r\n", lwm2m_settings_manufacturer_get());
		return 0;
	} else if (param_count != 3) {
		return -EINVAL;
	}

	int ret;
	char manufacturer[32];
	int size = sizeof(manufacturer);

	ret = util_string_get(parser, 2, manufacturer, &size);
	if (ret) {
		return ret;
	}

	return lwm2m_settings_manufacturer_set(manufacturer);
}

/* AT#XCARRIERCFG="model_number"[,<model_number>] */
SLM_AT_CMD_CUSTOM(xcarriercfg_model_number, "AT#XCARRIERCFG=\"model_number\"", do_cfg_model_number);
static int do_cfg_model_number(enum at_parser_cmd_type, struct at_parser *parser,
			       uint32_t param_count)
{
	if (param_count == 2) {
		rsp_send("\r\n#XCARRIERCFG: %s\r\n", lwm2m_settings_model_number_get());
		return 0;
	} else if (param_count != 3) {
		return -EINVAL;
	}

	int ret;
	char model_number[32];
	int size = sizeof(model_number);

	ret = util_string_get(parser, 2, model_number, &size);
	if (ret) {
		return ret;
	}

	return lwm2m_settings_model_number_set(model_number);
}

/* AT#XCARRIERCFG="software_version"[,<version>] */
SLM_AT_CMD_CUSTOM(xcarriercfg_software_version, "AT#XCARRIERCFG=\"software_version\"",
	      do_cfg_software_version);
static int do_cfg_software_version(enum at_parser_cmd_type, struct at_parser *parser,
				   uint32_t param_count)
{
	if (param_count == 2) {
		rsp_send("\r\n#XCARRIERCFG: %s\r\n", lwm2m_settings_software_version_get());
		return 0;
	} else if (param_count != 3) {
		return -EINVAL;
	}

	int ret;
	char software_version[32];
	int size = sizeof(software_version);

	ret = util_string_get(parser, 2, software_version, &size);
	if (ret) {
		return ret;
	}

	return lwm2m_settings_software_version_set(software_version);
}

/* AT#XCARRIERCFG="session_idle_timeout"[,<timeout>] */
SLM_AT_CMD_CUSTOM(xcarriercfg_session_idle_timeout, "AT#XCARRIERCFG=\"session_idle_timeout\"",
	      do_cfg_session_idle_timeout);
static int do_cfg_session_idle_timeout(enum at_parser_cmd_type, struct at_parser *parser,
				       uint32_t param_count)
{
	if (param_count == 2) {
		rsp_send("\r\n#XCARRIERCFG: %d\r\n", lwm2m_settings_session_idle_timeout_get());
		return 0;
	} else if (param_count != 3) {
		return -EINVAL;
	}

	int ret, session_idle_timeout;

	ret = at_parser_num_get(parser, 2, &session_idle_timeout);
	if (ret) {
		return ret;
	}

	if (session_idle_timeout < -1 || session_idle_timeout > 86400) {
		LOG_ERR("AT#XCARRIERCFG=\"session_idle_timeout\" failed: value must be "
			"-1 or between 0 and 86400 (24 hours)");
		return -EINVAL;
	}

	return lwm2m_settings_session_idle_timeout_set(session_idle_timeout);
}

/* AT#XCARRIERCFG="device_serial_no_type"[,<0|1>] */
SLM_AT_CMD_CUSTOM(xcarriercfg_device_serial_no_type, "AT#XCARRIERCFG=\"device_serial_no_type\"",
	     do_cfg_device_serial_no_type);
static int do_cfg_device_serial_no_type(enum at_parser_cmd_type, struct at_parser *parser,
					uint32_t param_count)
{
	if (param_count == 2) {
		rsp_send("\r\n#XCARRIERCFG: %u\r\n", lwm2m_settings_device_serial_no_type_get());
		return 0;
	} else if (param_count != 3) {
		return -EINVAL;
	}

	int ret, serial_no_type;

	ret = at_parser_num_get(parser, 2, &serial_no_type);
	if (ret) {
		return ret;
	}

	switch (serial_no_type) {
	case 0:
		serial_no_type = LWM2M_CARRIER_LG_UPLUS_DEVICE_SERIAL_NO_IMEI;
		break;
	case 1:
		serial_no_type = LWM2M_CARRIER_LG_UPLUS_DEVICE_SERIAL_NO_2DID;
		break;
	default:
		LOG_ERR("AT#XCARRIERCFG=\"device_serial_no_type\" failed: value must be 0 or 1");
		return -EINVAL;
	}

	return lwm2m_settings_device_serial_no_type_set(serial_no_type);
}

/* AT#XCARRIERCFG="service_code"[,<service_code>] */
SLM_AT_CMD_CUSTOM(xcarriercfg_service_code, "AT#XCARRIERCFG=\"service_code\"", do_cfg_service_code);
static int do_cfg_service_code(enum at_parser_cmd_type, struct at_parser *parser,
			       uint32_t param_count)
{
	if (param_count == 2) {
		rsp_send("\r\n#XCARRIERCFG: %s\r\n", lwm2m_settings_service_code_get());
		return 0;
	} else if (param_count != 3) {
		return -EINVAL;
	}

	int ret;
	char service_code[5];
	int size = sizeof(service_code);

	ret = util_string_get(parser, 2, service_code, &size);
	if (ret) {
		return ret;
	}

	return lwm2m_settings_service_code_set(service_code);
}

/* AT#XCARRIERCFG="pdn_type"[,<pdn_type>] */
SLM_AT_CMD_CUSTOM(xcarriercfg_pdn_type, "AT#XCARRIERCFG=\"pdn_type\"", do_cfg_pdn_type);
static int do_cfg_pdn_type(enum at_parser_cmd_type, struct at_parser *parser,
			   uint32_t param_count)
{
	if (param_count == 2) {
		rsp_send("\r\n#XCARRIERCFG: %d\r\n", lwm2m_settings_pdn_type_get());
		return 0;
	} else if (param_count != 3) {
		return -EINVAL;
	}

	int ret;
	uint16_t pdn_type;

	ret = at_parser_num_get(parser, 2, &pdn_type);
	if (ret) {
		return ret;
	}

	switch (pdn_type) {
	case 0:
		pdn_type = LWM2M_CARRIER_PDN_TYPE_IPV4V6;
		break;
	case 1:
		pdn_type = LWM2M_CARRIER_PDN_TYPE_IPV4;
		break;
	case 2:
		pdn_type = LWM2M_CARRIER_PDN_TYPE_IPV6;
		break;
	case 3:
		pdn_type = LWM2M_CARRIER_PDN_TYPE_NONIP;
		break;
	default:
		LOG_ERR("AT#XCARRIERCFG=\"pdn_type\" failed: value must be between 0 and 3");
		return -EINVAL;
	}

	return lwm2m_settings_pdn_type_set(pdn_type);
}

/* AT#XCARRIERCFG="queue_mode"[,<0|1>] */
SLM_AT_CMD_CUSTOM(xcarriercfg_queue_mode, "AT#XCARRIERCFG=\"queue_mode\"", do_cfg_queue_mode);
static int do_cfg_queue_mode(enum at_parser_cmd_type, struct at_parser *parser,
			     uint32_t param_count)
{
	if (param_count == 2) {
		rsp_send("\r\n#XCARRIERCFG: %d\r\n", lwm2m_settings_queue_mode_get());
		return 0;
	} else if (param_count != 3) {
		return -EINVAL;
	}

	int ret;
	uint16_t queue_mode;

	ret = at_parser_num_get(parser, 2, &queue_mode);
	if (ret) {
		return ret;
	}

	if (queue_mode > 1) {
		LOG_ERR("AT#XCARRIERCFG=\"queue_mode\" failed: must be 0 or 1");
		return -EINVAL;
	}

	return lwm2m_settings_queue_mode_set(queue_mode);
}

/* AT#XCARRIERCFG="binding"[,<binding>] */
SLM_AT_CMD_CUSTOM(xcarriercfg_binding, "AT#XCARRIERCFG=\"binding\"", do_cfg_binding);
static int do_cfg_binding(enum at_parser_cmd_type, struct at_parser *parser,
			  uint32_t param_count)
{
	int ret;
	int size = sizeof("UN");
	char binding_string[size];
	uint8_t binding_mask = 0;

	if (param_count == 2) {
		binding_mask = lwm2m_settings_server_binding_get();
		memset(binding_string, 0, sizeof(binding_string));

		if (binding_mask & LWM2M_CARRIER_SERVER_BINDING_UDP) {
			strcat(binding_string, "U");
		}

		if (binding_mask & LWM2M_CARRIER_SERVER_BINDING_NONIP) {
			strcat(binding_string, "N");
		}

		rsp_send("\r\n#XCARRIERCFG: %s\r\n", binding_string);
		return 0;
	} else if (param_count != 3) {
		return -EINVAL;
	}

	ret = util_string_get(parser, 2, binding_string, &size);
	if (ret) {
		return ret;
	}

	for (int i = 0; i < strlen(binding_string); i++) {
		if (binding_string[i] == 'U') {
			binding_mask |= LWM2M_CARRIER_SERVER_BINDING_UDP;
		} else if (binding_string[i] == 'N') {
			binding_mask |= LWM2M_CARRIER_SERVER_BINDING_NONIP;
		} else {
			LOG_ERR("AT#XCARRIERCFG=\"binding\" failed: binding must be U or N");
			return -EINVAL;
		}
	}

	return lwm2m_settings_server_binding_set(binding_mask);
}

/* AT#XCARRIERCFG="is_bootstrap"[,<0|1>] */
SLM_AT_CMD_CUSTOM(xcarriercfg_is_bootstrap, "AT#XCARRIERCFG=\"is_bootstrap\"", do_cfg_is_bootstrap);
static int do_cfg_is_bootstrap(enum at_parser_cmd_type, struct at_parser *parser,
			       uint32_t param_count)
{
	if (param_count == 2) {
		rsp_send("\r\n#XCARRIERCFG: %d\r\n", lwm2m_settings_is_bootstrap_server_get());
		return 0;
	} else if (param_count != 3) {
		return -EINVAL;
	}

	int ret;
	uint16_t is_bootstrap;

	ret = at_parser_num_get(parser, 2, &is_bootstrap);
	if (ret) {
		return ret;
	}

	if (is_bootstrap > 1) {
		LOG_ERR("AT#XCARRIERCFG=\"is_bootstrap\" failed: must be 0 or 1");
		return -EINVAL;
	}

	return lwm2m_settings_is_bootstrap_server_set(is_bootstrap);
}

/* AT#XCARRIERCFG="lifetime"[,<lifetime>] */
SLM_AT_CMD_CUSTOM(xcarriercfg_lifetime, "AT#XCARRIERCFG=\"lifetime\"", do_cfg_lifetime);
static int do_cfg_lifetime(enum at_parser_cmd_type, struct at_parser *parser,
			   uint32_t param_count)
{
	if (param_count == 2) {
		rsp_send("\r\n#XCARRIERCFG: %d\r\n", lwm2m_settings_server_lifetime_get());
		return 0;
	} else if (param_count != 3) {
		return -EINVAL;
	}

	int ret, lifetime;

	ret = at_parser_num_get(parser, 2, &lifetime);
	if (ret) {
		return ret;
	}

	if (lifetime < 0 || lifetime > 86400) {
		LOG_ERR("AT#XCARRIERCFG=\"lifetime\" failed: value must be "
			"between 0 and 86400 (24 hours)");
		return -EINVAL;
	}

	return lwm2m_settings_server_lifetime_set(lifetime);
}

/* AT#XCARRIERCFG="sec_tag"[,<sec_tag>] */
SLM_AT_CMD_CUSTOM(xcarriercfg_sec_tag, "AT#XCARRIERCFG=\"sec_tag\"", do_cfg_sec_tag);
static int do_cfg_sec_tag(enum at_parser_cmd_type, struct at_parser *parser,
			  uint32_t param_count)
{
	if (param_count == 2) {
		rsp_send("\r\n#XCARRIERCFG: %u\r\n", lwm2m_settings_server_sec_tag_get());
		return 0;
	} else if (param_count != 3) {
		return -EINVAL;
	}

	int ret;
	uint32_t sec_tag;

	ret = at_parser_num_get(parser, 2, &sec_tag);
	if (ret) {
		return ret;
	}

	return lwm2m_settings_server_sec_tag_set(sec_tag);
}

/* AT#XCARRIERCFG="uri"[,<uri>] */
SLM_AT_CMD_CUSTOM(xcarriercfg_uri, "AT#XCARRIERCFG=\"uri\"", do_cfg_uri);
static int do_cfg_uri(enum at_parser_cmd_type, struct at_parser *parser, uint32_t param_count)
{
	if (param_count == 2) {
		rsp_send("\r\n#XCARRIERCFG: %s\r\n", lwm2m_settings_server_uri_get());
		return 0;
	} else if (param_count != 3) {
		return -EINVAL;
	}

	int ret;
	char server_uri[255];
	int size = sizeof(server_uri);

	ret = util_string_get(parser, 2, server_uri, &size);
	if (ret) {
		return ret;
	}

	return lwm2m_settings_server_uri_set(server_uri);
}

int slm_at_carrier_cfg_init(void)
{
	if (IS_ENABLED(CONFIG_SLM_CARRIER_AUTO_STARTUP)) {
		return lwm2m_settings_auto_startup_set(true);
	}

	return 0;
}
