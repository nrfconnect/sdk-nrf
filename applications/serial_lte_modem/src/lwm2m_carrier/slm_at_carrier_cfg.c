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

/**@brief LwM2M Carrier Configuration operations. */
enum slm_carrier_cfg_operation {
	/* Enable Automatic Startup */
	CARRIER_CFG_OP_AUTO_STARTUP,
	/* General Custom Configuration */
	CARRIER_CFG_OP_CONFIG_ENABLE,
	CARRIER_CFG_OP_CONFIG_APN,
	CARRIER_CFG_OP_CONFIG_BOOTSTRAP_FROM_SMARTCARD,
	CARRIER_CFG_OP_CONFIG_CARRIERS,
	CARRIER_CFG_OP_CONFIG_COAP_CON_INTERVAL,
	CARRIER_CFG_OP_CONFIG_LGU_DEVICE_SERIAL_NO_TYPE,
	CARRIER_CFG_OP_CONFIG_LGU_SERVICE_CODE,
	CARRIER_CFG_OP_CONFIG_PDN_TYPE,
	CARRIER_CFG_OP_CONFIG_SESSION_IDLE_TIMEOUT,
	/* Device Custom Configuration */
	CARRIER_CFG_OP_DEVICE_ENABLE,
	CARRIER_CFG_OP_DEVICE_DEVICE_TYPE,
	CARRIER_CFG_OP_DEVICE_HW_VERSION,
	CARRIER_CFG_OP_DEVICE_MANUFACTURER,
	CARRIER_CFG_OP_DEVICE_MODEL_NUMBER,
	CARRIER_CFG_OP_DEVICE_SW_VERSION,
	/* Server Custom Configuration */
	CARRIER_CFG_OP_SERVER_ENABLE,
	CARRIER_CFG_OP_SERVER_BINDING,
	CARRIER_CFG_OP_SERVER_IS_BOOTSTRAP,
	CARRIER_CFG_OP_SERVER_LIFETIME,
	CARRIER_CFG_OP_SERVER_SEC_TAG,
	CARRIER_CFG_OP_SERVER_URI,
	/* Count */
	CARRIER_CFG_OP_MAX
};

/** forward declaration of configuration cmd handlers **/
static int do_cfg_apn(void);
static int do_cfg_auto_startup(void);
static int do_cfg_bootstrap_from_smartcard(void);
static int do_cfg_carriers(void);
static int do_cfg_coap_con_interval(void);
static int do_cfg_config_enable(void);
static int do_cfg_device_enable(void);
static int do_cfg_device_type(void);
static int do_cfg_hardware_version(void);
static int do_cfg_manufacturer(void);
static int do_cfg_model_number(void);
static int do_cfg_session_idle_timeout(void);
static int do_cfg_software_version(void);
static int do_cfg_device_serial_no_type(void);
static int do_cfg_service_code(void);
static int do_cfg_pdn_type(void);
static int do_cfg_binding(void);
static int do_cfg_server_enable(void);
static int do_cfg_is_bootstrap(void);
static int do_cfg_lifetime(void);
static int do_cfg_sec_tag(void);
static int do_cfg_uri(void);

struct carrier_cfg_op_list {
	uint8_t op_code;
	char *op_str;
	int (*handler)(void);
};

/**@brief SLM Carrier Configuration AT Command list type. */
static struct carrier_cfg_op_list cfg_op_list[CARRIER_CFG_OP_MAX] = {
	{CARRIER_CFG_OP_AUTO_STARTUP, "auto_startup", do_cfg_auto_startup},
	{CARRIER_CFG_OP_CONFIG_APN, "apn", do_cfg_apn},
	{CARRIER_CFG_OP_CONFIG_BOOTSTRAP_FROM_SMARTCARD, "bootstrap_smartcard",
	 do_cfg_bootstrap_from_smartcard},
	{CARRIER_CFG_OP_CONFIG_CARRIERS, "carriers", do_cfg_carriers},
	{CARRIER_CFG_OP_CONFIG_COAP_CON_INTERVAL, "coap_con_interval", do_cfg_coap_con_interval},
	{CARRIER_CFG_OP_CONFIG_ENABLE, "config_enable", do_cfg_config_enable},
	{CARRIER_CFG_OP_CONFIG_LGU_DEVICE_SERIAL_NO_TYPE, "device_serial_no_type",
	 do_cfg_device_serial_no_type},
	{CARRIER_CFG_OP_CONFIG_LGU_SERVICE_CODE, "service_code", do_cfg_service_code},
	{CARRIER_CFG_OP_CONFIG_PDN_TYPE, "pdn_type", do_cfg_pdn_type},
	{CARRIER_CFG_OP_CONFIG_SESSION_IDLE_TIMEOUT, "session_idle_timeout",
	 do_cfg_session_idle_timeout},
	{CARRIER_CFG_OP_DEVICE_ENABLE, "device_enable", do_cfg_device_enable},
	{CARRIER_CFG_OP_DEVICE_DEVICE_TYPE, "device_type", do_cfg_device_type},
	{CARRIER_CFG_OP_DEVICE_HW_VERSION, "hardware_version", do_cfg_hardware_version},
	{CARRIER_CFG_OP_DEVICE_MANUFACTURER, "manufacturer", do_cfg_manufacturer},
	{CARRIER_CFG_OP_DEVICE_MODEL_NUMBER, "model_number", do_cfg_model_number},
	{CARRIER_CFG_OP_DEVICE_SW_VERSION, "software_version", do_cfg_software_version},
	{CARRIER_CFG_OP_SERVER_BINDING, "binding", do_cfg_binding},
	{CARRIER_CFG_OP_SERVER_ENABLE, "server_enable", do_cfg_server_enable},
	{CARRIER_CFG_OP_SERVER_IS_BOOTSTRAP, "is_bootstrap", do_cfg_is_bootstrap},
	{CARRIER_CFG_OP_SERVER_LIFETIME, "lifetime", do_cfg_lifetime},
	{CARRIER_CFG_OP_SERVER_SEC_TAG, "sec_tag", do_cfg_sec_tag},
	{CARRIER_CFG_OP_SERVER_URI, "uri", do_cfg_uri},
};

#define SLM_CARRIER_CFG_OP_STR_MAX (sizeof("device_serial_no_type") + 1)

/**
 * @brief API to handle AT#XCARRIERCFG command.
 */
int handle_at_carrier_cfg(enum at_cmd_type cmd_type)
{
	int ret;
	char op_str[SLM_CARRIER_CFG_OP_STR_MAX];
	int size = sizeof(op_str);

	if (cmd_type != AT_CMD_TYPE_SET_COMMAND) {
		return -EINVAL;
	}

	ret = util_string_get(&slm_at_param_list, 1, op_str, &size);
	if (ret) {
		return ret;
	}

	ret = -EINVAL;
	for (int i = 0; i < CARRIER_CFG_OP_MAX; i++) {
		if (slm_util_casecmp(op_str, cfg_op_list[i].op_str)) {
			ret = cfg_op_list[i].handler();
			break;
		}
	}

	return ret;
}

/* AT#XCARRIERCFG="apn"[,<apn>] */
static int do_cfg_apn(void)
{
	uint32_t count;

	count = at_params_valid_count_get(&slm_at_param_list);
	if (count == 2) {
		rsp_send("\r\n#XCARRIERCFG: %s\r\n", lwm2m_settings_apn_get());
		return 0;
	} else if (count != 3) {
		return -EINVAL;
	}

	char apn[64];
	int ret, size = sizeof(apn);

	ret = util_string_get(&slm_at_param_list, 2, apn, &size);
	if (ret) {
		return ret;
	}

	return lwm2m_settings_apn_set(apn);
}

/* AT#XCARRIERCFG="auto_startup"[,<0|1>] */
static int do_cfg_auto_startup(void)
{
	uint32_t count;

	count = at_params_valid_count_get(&slm_at_param_list);
	if (count == 2) {
		rsp_send("\r\n#XCARRIERCFG: %u\r\n", lwm2m_settings_auto_startup_get());
		return 0;
	} else if (count != 3) {
		return -EINVAL;
	}

	int ret;
	uint16_t auto_startup;

	ret = at_params_unsigned_short_get(&slm_at_param_list, 2, &auto_startup);
	if (ret) {
		return ret;
	}

	if (auto_startup > 1) {
		LOG_ERR("AT#XCARRIERCFG=\"auto_startup\" failed: must be 0 or 1");
		return -EINVAL;
	}

	return lwm2m_settings_auto_startup_set(auto_startup);
}

/* AT#XCARRIERCFG="bootstrap_smartcard"[,<0|1>] */
static int do_cfg_bootstrap_from_smartcard(void)
{
	uint32_t count;

	count = at_params_valid_count_get(&slm_at_param_list);
	if (count == 2) {
		rsp_send("\r\n#XCARRIERCFG: %u\r\n", lwm2m_settings_bootstrap_from_smartcard_get());
		return 0;
	} else if (count != 3) {
		return -EINVAL;
	}

	int ret;
	uint16_t enabled;

	ret = at_params_unsigned_short_get(&slm_at_param_list, 2, &enabled);
	if (ret) {
		return ret;
	}

	if (enabled > 1) {
		LOG_ERR("AT#XCARRIERCFG=\"bootstrap_smartcard\" failed: must be 0 or 1");
		return -EINVAL;
	}

	return lwm2m_settings_bootstrap_from_smartcard_set(enabled);
}

const static uint32_t carriers_enabled_map[] = {
	LWM2M_CARRIER_GENERIC,
	LWM2M_CARRIER_VERIZON,
	LWM2M_CARRIER_ATT,
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
static int do_cfg_carriers(void)
{
	uint32_t count;

	count = at_params_valid_count_get(&slm_at_param_list);
	if (count == 2) {
		rsp_send("\r\n#XCARRIERCFG: %s\r\n", carriers_enabled_str());
		return 0;
	} else if (count - 2 > ARRAY_SIZE(carriers_enabled_map)) {
		LOG_ERR("AT#XCARRIERCFG=\"carriers\" failed: too many parameters");
		return -EINVAL;
	}

	uint32_t carriers_enabled = 0;
	int ret, size = sizeof("all");
	char buf[size];

	ret = util_string_get(&slm_at_param_list, 2, buf, &size);
	if (!ret && slm_util_casecmp(buf, "all")) {
		carriers_enabled = UINT32_MAX;
	} else {
		for (int i = 2; i < count; i++) {
			uint16_t carrier;

			ret = at_params_unsigned_short_get(&slm_at_param_list, i, &carrier);
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
static int do_cfg_coap_con_interval(void)
{
	uint32_t count;

	count = at_params_valid_count_get(&slm_at_param_list);
	if (count == 2) {
		rsp_send("\r\n#XCARRIERCFG: %d\r\n", lwm2m_settings_coap_con_interval_get());
		return 0;
	} else if (count != 3) {
		return -EINVAL;
	}

	int ret, coap_con_interval;

	ret = at_params_int_get(&slm_at_param_list, 2, &coap_con_interval);
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

/* AT#XCARRIERCFG="config_enable"[,<0|1>] */
static int do_cfg_config_enable(void)
{
	uint32_t count;

	count = at_params_valid_count_get(&slm_at_param_list);
	if (count == 2) {
		rsp_send("\r\n#XCARRIERCFG: %u\r\n", lwm2m_settings_enable_custom_config_get());
		return 0;
	} else if (count != 3) {
		return -EINVAL;
	}

	int ret;
	uint16_t enabled;

	ret = at_params_unsigned_short_get(&slm_at_param_list, 2, &enabled);
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
static int do_cfg_device_enable(void)
{
	uint32_t count;

	count = at_params_valid_count_get(&slm_at_param_list);
	if (count == 2) {
		rsp_send("\r\n#XCARRIERCFG: %u\r\n",
			 lwm2m_settings_enable_custom_device_config_get());
		return 0;
	} else if (count != 3) {
		return -EINVAL;
	}

	int ret;
	uint16_t enabled;

	ret = at_params_unsigned_short_get(&slm_at_param_list, 2, &enabled);
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
static int do_cfg_device_type(void)
{
	uint32_t count;

	count = at_params_valid_count_get(&slm_at_param_list);
	if (count == 2) {
		rsp_send("\r\n#XCARRIERCFG: %s\r\n", lwm2m_settings_device_type_get());
		return 0;
	} else if (count != 3) {
		return -EINVAL;
	}

	int ret;
	char device_type[32];
	int size = sizeof(device_type);

	ret = util_string_get(&slm_at_param_list, 2, device_type, &size);
	if (ret) {
		return ret;
	}

	return lwm2m_settings_device_type_set(device_type);
}

/* AT#XCARRIERCFG="hardware_version"[,<version>] */
static int do_cfg_hardware_version(void)
{
	uint32_t count;

	count = at_params_valid_count_get(&slm_at_param_list);
	if (count == 2) {
		rsp_send("\r\n#XCARRIERCFG: %s\r\n", lwm2m_settings_hardware_version_get());
		return 0;
	} else if (count != 3) {
		return -EINVAL;
	}

	int ret;
	char hardware_version[32];
	int size = sizeof(hardware_version);

	ret = util_string_get(&slm_at_param_list, 2, hardware_version, &size);
	if (ret) {
		return ret;
	}

	return lwm2m_settings_hardware_version_set(hardware_version);
}

/* AT#XCARRIERCFG="manufacturer"[,<manufacturer>] */
static int do_cfg_manufacturer(void)
{
	uint32_t count;

	count = at_params_valid_count_get(&slm_at_param_list);
	if (count == 2) {
		rsp_send("\r\n#XCARRIERCFG: %s\r\n", lwm2m_settings_manufacturer_get());
		return 0;
	} else if (count != 3) {
		return -EINVAL;
	}

	int ret;
	char manufacturer[32];
	int size = sizeof(manufacturer);

	ret = util_string_get(&slm_at_param_list, 2, manufacturer, &size);
	if (ret) {
		return ret;
	}

	return lwm2m_settings_manufacturer_set(manufacturer);
}

/* AT#XCARRIERCFG="model_number"[,<model_number>] */
static int do_cfg_model_number(void)
{
	uint32_t count;

	count = at_params_valid_count_get(&slm_at_param_list);
	if (count == 2) {
		rsp_send("\r\n#XCARRIERCFG: %s\r\n", lwm2m_settings_model_number_get());
		return 0;
	} else if (count != 3) {
		return -EINVAL;
	}

	int ret;
	char model_number[32];
	int size = sizeof(model_number);

	ret = util_string_get(&slm_at_param_list, 2, model_number, &size);
	if (ret) {
		return ret;
	}

	return lwm2m_settings_model_number_set(model_number);
}

/* AT#XCARRIERCFG="software_version"[,<version>] */
static int do_cfg_software_version(void)
{
	uint32_t count;

	count = at_params_valid_count_get(&slm_at_param_list);
	if (count == 2) {
		rsp_send("\r\n#XCARRIERCFG: %s\r\n", lwm2m_settings_software_version_get());
		return 0;
	} else if (count != 3) {
		return -EINVAL;
	}

	int ret;
	char software_version[32];
	int size = sizeof(software_version);

	ret = util_string_get(&slm_at_param_list, 2, software_version, &size);
	if (ret) {
		return ret;
	}

	return lwm2m_settings_software_version_set(software_version);
}

/* AT#XCARRIERCFG="session_idle_timeout"[,<timeout>] */
static int do_cfg_session_idle_timeout(void)
{
	uint32_t count;

	count = at_params_valid_count_get(&slm_at_param_list);
	if (count == 2) {
		rsp_send("\r\n#XCARRIERCFG: %d\r\n", lwm2m_settings_session_idle_timeout_get());
		return 0;
	} else if (count != 3) {
		return -EINVAL;
	}

	int ret, session_idle_timeout;

	ret = at_params_int_get(&slm_at_param_list, 2, &session_idle_timeout);
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
static int do_cfg_device_serial_no_type(void)
{
	uint32_t count;

	count = at_params_valid_count_get(&slm_at_param_list);
	if (count == 2) {
		rsp_send("\r\n#XCARRIERCFG: %u\r\n", lwm2m_settings_device_serial_no_type_get());
		return 0;
	} else if (count != 3) {
		return -EINVAL;
	}

	int ret, serial_no_type;

	ret = at_params_int_get(&slm_at_param_list, 2, &serial_no_type);
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
static int do_cfg_service_code(void)
{
	uint32_t count;

	count = at_params_valid_count_get(&slm_at_param_list);
	if (count == 2) {
		rsp_send("\r\n#XCARRIERCFG: %s\r\n", lwm2m_settings_service_code_get());
		return 0;
	} else if (count != 3) {
		return -EINVAL;
	}

	int ret;
	char service_code[5];
	int size = sizeof(service_code);

	ret = util_string_get(&slm_at_param_list, 2, service_code, &size);
	if (ret) {
		return ret;
	}

	return lwm2m_settings_service_code_set(service_code);
}

/* AT#XCARRIERCFG="pdn_type"[,<pdn_type>] */
static int do_cfg_pdn_type(void)
{
	uint32_t count;

	count = at_params_valid_count_get(&slm_at_param_list);
	if (count == 2) {
		rsp_send("\r\n#XCARRIERCFG: %d\r\n", lwm2m_settings_pdn_type_get());
		return 0;
	} else if (count != 3) {
		return -EINVAL;
	}

	int ret;
	uint16_t pdn_type;

	ret = at_params_unsigned_short_get(&slm_at_param_list, 2, &pdn_type);
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

/* AT#XCARRIERCFG="binding"[,<binding>] */
static int do_cfg_binding(void)
{
	uint32_t count;

	count = at_params_valid_count_get(&slm_at_param_list);
	if (count == 2) {
		rsp_send("\r\n#XCARRIERCFG: %c\r\n", lwm2m_settings_server_binding_get());
		return 0;
	} else if (count != 3) {
		return -EINVAL;
	}

	int ret;
	int size = sizeof("U");
	char binding[size];

	ret = util_string_get(&slm_at_param_list, 2, binding, &size);
	if (ret) {
		return ret;
	}

	if ((binding[0] != 'U') && (binding[0] != 'N')) {
		LOG_ERR("AT#XCARRIERCFG=\"binding\" failed: binding must be U or N");
		return -EINVAL;
	}

	return lwm2m_settings_server_binding_set(binding[0]);
}

/* AT#XCARRIERCFG="server_enable"[,<0|1>] */
static int do_cfg_server_enable(void)
{
	uint32_t count;

	count = at_params_valid_count_get(&slm_at_param_list);
	if (count == 2) {
		rsp_send("\r\n#XCARRIERCFG: %u\r\n",
			 lwm2m_settings_enable_custom_server_config_get());
		return 0;
	} else if (count != 3) {
		return -EINVAL;
	}

	int ret;
	uint16_t enabled;

	ret = at_params_unsigned_short_get(&slm_at_param_list, 2, &enabled);
	if (ret) {
		return ret;
	}

	if (enabled > 1) {
		LOG_ERR("AT#XCARRIERCFG=\"server_enable\" failed: must be 0 or 1");
		return -EINVAL;
	}

	return lwm2m_settings_enable_custom_server_config_set(enabled);
}

/* AT#XCARRIERCFG="is_bootstrap"[,<0|1>] */
static int do_cfg_is_bootstrap(void)
{
	uint32_t count;

	count = at_params_valid_count_get(&slm_at_param_list);
	if (count == 2) {
		rsp_send("\r\n#XCARRIERCFG: %d\r\n", lwm2m_settings_is_bootstrap_server_get());
		return 0;
	} else if (count != 3) {
		return -EINVAL;
	}

	int ret;
	uint16_t is_bootstrap;

	ret = at_params_unsigned_short_get(&slm_at_param_list, 2, &is_bootstrap);
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
static int do_cfg_lifetime(void)
{
	uint32_t count;

	count = at_params_valid_count_get(&slm_at_param_list);
	if (count == 2) {
		rsp_send("\r\n#XCARRIERCFG: %d\r\n", lwm2m_settings_server_lifetime_get());
		return 0;
	} else if (count != 3) {
		return -EINVAL;
	}

	int ret, lifetime;

	ret = at_params_int_get(&slm_at_param_list, 2, &lifetime);
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
static int do_cfg_sec_tag(void)
{
	uint32_t count;

	count = at_params_valid_count_get(&slm_at_param_list);
	if (count == 2) {
		rsp_send("\r\n#XCARRIERCFG: %u\r\n", lwm2m_settings_server_sec_tag_get());
		return 0;
	} else if (count != 3) {
		return -EINVAL;
	}

	int ret;
	uint32_t sec_tag;

	ret = at_params_unsigned_int_get(&slm_at_param_list, 2, &sec_tag);
	if (ret) {
		return ret;
	}

	return lwm2m_settings_server_sec_tag_set(sec_tag);
}

/* AT#XCARRIERCFG="uri"[,<uri>] */
static int do_cfg_uri(void)
{
	uint32_t count;

	count = at_params_valid_count_get(&slm_at_param_list);
	if (count == 2) {
		rsp_send("\r\n#XCARRIERCFG: %s\r\n", lwm2m_settings_server_uri_get());
		return 0;
	} else if (count != 3) {
		return -EINVAL;
	}

	int ret;
	char server_uri[255];
	int size = sizeof(server_uri);

	ret = util_string_get(&slm_at_param_list, 2, server_uri, &size);
	if (ret) {
		return ret;
	}

	return lwm2m_settings_server_uri_set(server_uri);
}
