/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include <lwm2m_settings.h>
#include <lwm2m_carrier.h>
#include <zephyr/settings/settings.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lwm2m_settings, CONFIG_LOG_DEFAULT_LEVEL);

#define LWM2M_SETTINGS_SUBTREE_NAME			"carrier_config"

#define AUTO_STARTUP_SETTINGS_KEY			"auto_startup"

#define ENABLE_CUSTOM_CONFIG_SETTINGS_KEY		"enable_custom_config_settings"
#define CARRIERS_ENABLED_SETTINGS_KEY			"carriers_enabled"
#define SERVER_URI_SETTINGS_KEY				"server_uri"
#define IS_BOOTSTRAP_SERVER_SETTINGS_KEY		"is_bootstrap_server"
#define SERVER_SEC_TAG_SETTINGS_KEY			"server_sec_tag"
#define SERVER_LIFETIME_SETTINGS_KEY			"server_lifetime"
#define SERVER_BINDING_SETTINGS_KEY			"server_binding"
#define BOOTSTRAP_FROM_SMARTCARD_SETTINGS_KEY		"bootstrap_from_smartcard"
#define SESSION_IDLE_TIMEOUT_SETTINGS_KEY		"session_idle_timeout"
#define COAP_CON_INTERVAL_SETTINGS_KEY			"coap_con_interval"
#define APN_SETTINGS_KEY				"apn"
#define PDN_TYPE_SETTINGS_KEY				"pdn_type"
#define SERVICE_CODE_SETTINGS_KEY			"service_code"
#define DEVICE_SERIAL_NO_TYPE_SETTINGS_KEY              "device_serial_no_type"
#define FIRMWARE_DOWNLOAD_TIMEOUT_SETTINGS_KEY          "firmware_download_timeout"
#define QUEUE_MODE_KEY                                  "queue_mode"
#define AUTO_REGISTER_KEY                               "auto_register"

#define ENABLE_CUSTOM_DEVICE_SETTINGS_KEY		"enable_custom_device_settings"
#define MANUFACTURER_SETTINGS_KEY			"manufacturer"
#define MODEL_NUMBER_SETTINGS_KEY			"model_number"
#define DEVICE_TYPE_SETTINGS_KEY			"device_type"
#define HARDWARE_VERSION_SETTINGS_KEY			"hardware_version"
#define SOFTWARE_VERSION_SETTINGS_KEY			"software_version"

static bool auto_startup;
struct k_sem auto_startup_sem;

/* Default turn on certification mode when using custom configuration. */
static bool enable_custom_config = true;

static uint32_t carriers_enabled = 0
#ifdef CONFIG_LWM2M_CARRIER_GENERIC
		| LWM2M_CARRIER_GENERIC
#endif
#ifdef CONFIG_LWM2M_CARRIER_VERIZON
		| LWM2M_CARRIER_VERIZON
#endif
#ifdef CONFIG_LWM2M_CARRIER_LG_UPLUS
		| LWM2M_CARRIER_LG_UPLUS
#endif
#ifdef CONFIG_LWM2M_CARRIER_T_MOBILE
		| LWM2M_CARRIER_T_MOBILE
#endif
#ifdef CONFIG_LWM2M_CARRIER_SOFTBANK
		| LWM2M_CARRIER_SOFTBANK
#endif
#ifdef CONFIG_LWM2M_CARRIER_BELL_CA
		| LWM2M_CARRIER_BELL_CA
#endif
;

#ifdef CONFIG_LWM2M_CARRIER_AUTO_REGISTER
static bool auto_register = true;
#else
static bool auto_register;
#endif

#ifdef CONFIG_LWM2M_CARRIER_BOOTSTRAP_SMARTCARD
static bool bootstrap_from_smartcard = true;
#else
static bool bootstrap_from_smartcard;
#endif

static int32_t session_idle_timeout = CONFIG_LWM2M_CARRIER_SESSION_IDLE_TIMEOUT;
static int32_t coap_con_interval = CONFIG_LWM2M_CARRIER_COAP_CON_INTERVAL;
static char apn[64] = CONFIG_LWM2M_CARRIER_CUSTOM_APN;
static uint32_t pdn_type = CONFIG_LWM2M_CARRIER_PDN_TYPE;

#ifdef CONFIG_LWM2M_CARRIER_LG_UPLUS_SERVICE_CODE
static char service_code[6] = CONFIG_LWM2M_CARRIER_LG_UPLUS_SERVICE_CODE;
#else
static char service_code[6];
#endif

#ifdef LWM2M_CARRIER_LG_UPLUS_DEVICE_SERIAL_NO_2DID
static uint8_t device_serial_no_type = LWM2M_CARRIER_LG_UPLUS_DEVICE_SERIAL_NO_2DID;
#else
static uint8_t device_serial_no_type = LWM2M_CARRIER_LG_UPLUS_DEVICE_SERIAL_NO_IMEI;
#endif

static uint16_t firmware_download_timeout = CONFIG_LWM2M_CARRIER_FIRMWARE_DOWNLOAD_TIMEOUT;

#ifdef CONFIG_LWM2M_CARRIER_QUEUE_MODE
static bool queue_mode = true;
#else
static bool queue_mode;
#endif

#ifdef CONFIG_LWM2M_CARRIER_IS_BOOTSTRAP_SERVER
static bool is_bootstrap_server = true;
#else
static bool is_bootstrap_server;
#endif

static char server_uri[256] = CONFIG_LWM2M_CARRIER_CUSTOM_URI;
static uint32_t server_sec_tag = CONFIG_LWM2M_CARRIER_SERVER_SEC_TAG;
static int32_t server_lifetime = CONFIG_LWM2M_CARRIER_SERVER_LIFETIME;

static uint8_t server_binding = 0
#ifdef CONFIG_LWM2M_CARRIER_SERVER_BINDING_UDP
		| LWM2M_CARRIER_SERVER_BINDING_UDP
#endif
#ifdef CONFIG_LWM2M_CARRIER_SERVER_BINDING_NONIP
		| LWM2M_CARRIER_SERVER_BINDING_NONIP
#endif
;

static bool enable_custom_device_config;
static char manufacturer[33] = CONFIG_LWM2M_CARRIER_DEVICE_MANUFACTURER;
static char model_number[33] = CONFIG_LWM2M_CARRIER_DEVICE_MODEL_NUMBER;
static char device_type[33] = CONFIG_LWM2M_CARRIER_DEVICE_TYPE;
static char hardware_version[33] = CONFIG_LWM2M_CARRIER_DEVICE_HARDWARE_VERSION;
static char software_version[33] = CONFIG_LWM2M_CARRIER_DEVICE_SOFTWARE_VERSION;

static int settings_load_cb(const char *key, size_t len, settings_read_cb read_cb, void *cb_arg)
{
	ARG_UNUSED(len);
	ssize_t sz = 0;

	if (strcmp(key, AUTO_STARTUP_SETTINGS_KEY) == 0) {
		sz = read_cb(cb_arg, &auto_startup, sizeof(auto_startup));
	} else if (strcmp(key, ENABLE_CUSTOM_CONFIG_SETTINGS_KEY) == 0) {
		sz = read_cb(cb_arg, &enable_custom_config, sizeof(enable_custom_config));
	} else if (strcmp(key, CARRIERS_ENABLED_SETTINGS_KEY) == 0) {
		sz = read_cb(cb_arg, &carriers_enabled, sizeof(carriers_enabled));
	} else if (strcmp(key, BOOTSTRAP_FROM_SMARTCARD_SETTINGS_KEY) == 0) {
		sz = read_cb(cb_arg, &bootstrap_from_smartcard,
			     sizeof(bootstrap_from_smartcard));
	} else if (strcmp(key, SESSION_IDLE_TIMEOUT_SETTINGS_KEY) == 0) {
		sz = read_cb(cb_arg, &session_idle_timeout, sizeof(session_idle_timeout));
	} else if (strcmp(key, COAP_CON_INTERVAL_SETTINGS_KEY) == 0) {
		sz = read_cb(cb_arg, &coap_con_interval, sizeof(coap_con_interval));
	} else if (strcmp(key, APN_SETTINGS_KEY) == 0) {
		sz = read_cb(cb_arg, apn, sizeof(apn));
	} else if (strcmp(key, PDN_TYPE_SETTINGS_KEY) == 0) {
		sz = read_cb(cb_arg, &pdn_type, sizeof(pdn_type));
	} else if (strcmp(key, SERVICE_CODE_SETTINGS_KEY) == 0) {
		sz = read_cb(cb_arg, service_code, sizeof(service_code));
	} else if (strcmp(key, DEVICE_SERIAL_NO_TYPE_SETTINGS_KEY) == 0) {
		sz = read_cb(cb_arg, &device_serial_no_type, sizeof(device_serial_no_type));
	} else if (strcmp(key, FIRMWARE_DOWNLOAD_TIMEOUT_SETTINGS_KEY) == 0) {
		sz = read_cb(cb_arg, &firmware_download_timeout, sizeof(firmware_download_timeout));
	} else if (strcmp(key, QUEUE_MODE_KEY) == 0) {
		sz = read_cb(cb_arg, &queue_mode, sizeof(queue_mode));
	} else if (strcmp(key, AUTO_REGISTER_KEY) == 0) {
		sz = read_cb(cb_arg, &auto_register, sizeof(auto_register));
	} else if (strcmp(key, IS_BOOTSTRAP_SERVER_SETTINGS_KEY) == 0) {
		sz = read_cb(cb_arg, &is_bootstrap_server, sizeof(is_bootstrap_server));
	} else if (strcmp(key, SERVER_URI_SETTINGS_KEY) == 0) {
		sz = read_cb(cb_arg, server_uri, sizeof(server_uri));
	} else if (strcmp(key, SERVER_SEC_TAG_SETTINGS_KEY) == 0) {
		sz = read_cb(cb_arg, &server_sec_tag, sizeof(server_sec_tag));
	} else if (strcmp(key, SERVER_LIFETIME_SETTINGS_KEY) == 0) {
		sz = read_cb(cb_arg, &server_lifetime, sizeof(server_lifetime));
	} else if (strcmp(key, SERVER_BINDING_SETTINGS_KEY) == 0) {
		sz = read_cb(cb_arg, &server_binding, sizeof(server_binding));
		if (sz == 1) {
			/* Adapt setting from older version. */
			if (server_binding == 'U') {
				server_binding = LWM2M_CARRIER_SERVER_BINDING_UDP;
			} else if (server_binding == 'N') {
				server_binding = LWM2M_CARRIER_SERVER_BINDING_NONIP;
			}
		}
	} else if (strcmp(key, ENABLE_CUSTOM_DEVICE_SETTINGS_KEY) == 0) {
		sz = read_cb(cb_arg, &enable_custom_device_config,
			     sizeof(enable_custom_device_config));
	} else if (strcmp(key, MANUFACTURER_SETTINGS_KEY) == 0) {
		sz = read_cb(cb_arg, manufacturer, sizeof(manufacturer));
	} else if (strcmp(key, MODEL_NUMBER_SETTINGS_KEY) == 0) {
		sz = read_cb(cb_arg, model_number, sizeof(model_number));
	} else if (strcmp(key, DEVICE_TYPE_SETTINGS_KEY) == 0) {
		sz = read_cb(cb_arg, device_type, sizeof(device_type));
	} else if (strcmp(key, HARDWARE_VERSION_SETTINGS_KEY) == 0) {
		sz = read_cb(cb_arg, hardware_version, sizeof(hardware_version));
	} else if (strcmp(key, SOFTWARE_VERSION_SETTINGS_KEY) == 0) {
		sz = read_cb(cb_arg, software_version, sizeof(software_version));
	}

	if (sz < 0) {
		LOG_ERR("Read %s failed: %d", key, sz);
	}

	return 0;
}

static void settings_enable_custom_config(lwm2m_carrier_config_t *config)
{
	LOG_INF("Enable custom configuration");

	config->carriers_enabled = carriers_enabled;
	config->is_bootstrap_server = is_bootstrap_server;
	config->server_uri = server_uri;
	config->server_sec_tag = server_sec_tag;
	config->server_lifetime = server_lifetime;
	config->server_binding = server_binding;
	config->disable_auto_register = !auto_register;
	config->disable_bootstrap_from_smartcard = !bootstrap_from_smartcard;
	config->disable_queue_mode = !queue_mode;
	config->session_idle_timeout = session_idle_timeout;
	config->coap_con_interval = coap_con_interval;
	config->apn = apn;
	config->pdn_type = pdn_type;
	config->firmware_download_timeout = firmware_download_timeout;
	config->lg_uplus.service_code = service_code;
	config->lg_uplus.device_serial_no_type = device_serial_no_type;
}

static void settings_enable_custom_device_config(lwm2m_carrier_config_t *config)
{
	LOG_INF("Enable custom device configuration");

	config->manufacturer = manufacturer;
	config->model_number = model_number;
	config->device_type = device_type;
	config->hardware_version = hardware_version;
	config->software_version = software_version;
}

static void settings_enable(lwm2m_carrier_config_t *config)
{
	if (enable_custom_config) {
		settings_enable_custom_config(config);
	}

	if (enable_custom_device_config) {
		settings_enable_custom_device_config(config);
	}
}

static int settings_init(void)
{
	int err;
	static struct settings_handler sh = {
		.name = LWM2M_SETTINGS_SUBTREE_NAME,
		.h_set = settings_load_cb,
	};

	err = settings_subsys_init();
	if (err) {
		LOG_ERR("Init failed: %d", err);
		return err;
	}

	err = settings_register(&sh);
	if (err && err != -EEXIST) {
		LOG_ERR("Register failed: %d", err);
		return err;
	}

	err = settings_load_subtree(LWM2M_SETTINGS_SUBTREE_NAME);
	if (err) {
		LOG_ERR("Load subtree failed: %d", err);
		return err;
	}

	k_sem_init(&auto_startup_sem, 0, 1);

	return 0;
}

bool lwm2m_settings_auto_startup_get(void)
{
	return auto_startup;
}

int lwm2m_settings_auto_startup_set(bool new_auto_startup)
{
	auto_startup = new_auto_startup;

	int err = settings_save_one(LWM2M_SETTINGS_SUBTREE_NAME "/" AUTO_STARTUP_SETTINGS_KEY,
				    &auto_startup, sizeof(auto_startup));
	if (err) {
		LOG_ERR("Save " AUTO_STARTUP_SETTINGS_KEY " failed: %d", err);
	}

	if (auto_startup) {
		k_sem_give(&auto_startup_sem);
	}

	return err;
}

bool lwm2m_settings_enable_custom_config_get(void)
{
	return enable_custom_config;
}

int lwm2m_settings_enable_custom_config_set(bool new_enable_custom_config)
{
	enable_custom_config = new_enable_custom_config;

	int err = settings_save_one(LWM2M_SETTINGS_SUBTREE_NAME "/"
				    ENABLE_CUSTOM_CONFIG_SETTINGS_KEY,
				    &enable_custom_config, sizeof(enable_custom_config));
	if (err) {
		LOG_ERR("Save " ENABLE_CUSTOM_CONFIG_SETTINGS_KEY " failed: %d", err);
	}

	return err;
}

uint32_t lwm2m_settings_carriers_enabled_get(void)
{
	return carriers_enabled;
}

int lwm2m_settings_carriers_enabled_set(uint32_t new_carriers_enabled)
{
	carriers_enabled = new_carriers_enabled;

	int err = settings_save_one(LWM2M_SETTINGS_SUBTREE_NAME "/" CARRIERS_ENABLED_SETTINGS_KEY,
				    &carriers_enabled, sizeof(carriers_enabled));
	if (err) {
		LOG_ERR("Save " CARRIERS_ENABLED_SETTINGS_KEY " failed: %d", err);
	}

	return err;
}

bool lwm2m_settings_bootstrap_from_smartcard_get(void)
{
	return bootstrap_from_smartcard;
}

int lwm2m_settings_bootstrap_from_smartcard_set(bool new_bootstrap_from_smartcard)
{
	bootstrap_from_smartcard = new_bootstrap_from_smartcard;

	int err = settings_save_one(LWM2M_SETTINGS_SUBTREE_NAME "/"
				    BOOTSTRAP_FROM_SMARTCARD_SETTINGS_KEY,
				    &bootstrap_from_smartcard, sizeof(bootstrap_from_smartcard));
	if (err) {
		LOG_ERR("Save " BOOTSTRAP_FROM_SMARTCARD_SETTINGS_KEY " failed: %d", err);
	}

	return err;
}

bool lwm2m_settings_is_bootstrap_server_get(void)
{
	return is_bootstrap_server;
}

int lwm2m_settings_is_bootstrap_server_set(bool new_is_bootstrap_server)
{
	is_bootstrap_server = new_is_bootstrap_server;

	int err = settings_save_one(LWM2M_SETTINGS_SUBTREE_NAME "/"
				    IS_BOOTSTRAP_SERVER_SETTINGS_KEY,
				    &is_bootstrap_server, sizeof(is_bootstrap_server));
	if (err) {
		LOG_ERR("Save " IS_BOOTSTRAP_SERVER_SETTINGS_KEY " failed: %d", err);
	}

	return err;
}

const char *lwm2m_settings_server_uri_get(void)
{
	return server_uri;
}

int lwm2m_settings_server_uri_set(const char *new_server_uri)
{
	size_t server_uri_max_len = sizeof(server_uri) - 1;
	size_t new_server_uri_len = strlen(new_server_uri);
	int err;

	if (new_server_uri_len > server_uri_max_len) {
		LOG_ERR("String is too long. Max size is %d", server_uri_max_len);
		return -ENOMEM;
	}

	memset(server_uri, 0, sizeof(server_uri));
	strncpy(server_uri, new_server_uri, sizeof(server_uri) - 1);

	if (new_server_uri_len == 0) {
		err = settings_delete(LWM2M_SETTINGS_SUBTREE_NAME "/" SERVER_URI_SETTINGS_KEY);

		if (err) {
			LOG_ERR("Delete " SERVER_URI_SETTINGS_KEY " failed: %d", err);
		}
	} else {
		err = settings_save_one(LWM2M_SETTINGS_SUBTREE_NAME "/" SERVER_URI_SETTINGS_KEY,
					server_uri, strlen(server_uri));
		if (err) {
			LOG_ERR("Save " SERVER_URI_SETTINGS_KEY " failed: %d", err);
		}
	}

	return err;
}

uint32_t lwm2m_settings_server_sec_tag_get(void)
{
	return server_sec_tag;
}

int lwm2m_settings_server_sec_tag_set(const uint32_t new_server_sec_tag)
{
	server_sec_tag = new_server_sec_tag;

	int err = settings_save_one(LWM2M_SETTINGS_SUBTREE_NAME "/" SERVER_SEC_TAG_SETTINGS_KEY,
				    &server_sec_tag, sizeof(server_sec_tag));
	if (err) {
		LOG_ERR("Save " SERVER_SEC_TAG_SETTINGS_KEY " failed: %d", err);
	}

	return err;
}

int32_t lwm2m_settings_server_lifetime_get(void)
{
	return server_lifetime;
}

int lwm2m_settings_server_lifetime_set(const int32_t new_server_lifetime)
{
	server_lifetime = new_server_lifetime;

	int err = settings_save_one(LWM2M_SETTINGS_SUBTREE_NAME "/" SERVER_LIFETIME_SETTINGS_KEY,
				    &server_lifetime, sizeof(server_lifetime));
	if (err) {
		LOG_ERR("Save " SERVER_LIFETIME_SETTINGS_KEY " failed: %d", err);
	}

	return err;
}

uint8_t lwm2m_settings_server_binding_get(void)
{
	return server_binding;
}

int lwm2m_settings_server_binding_set(uint8_t new_server_binding)
{
	server_binding = new_server_binding;

	int err = settings_save_one(LWM2M_SETTINGS_SUBTREE_NAME "/" SERVER_BINDING_SETTINGS_KEY,
				    &server_binding, sizeof(server_binding));
	if (err) {
		LOG_ERR("Save " SERVER_BINDING_SETTINGS_KEY " failed: %d", err);
	}

	return err;
}

int32_t lwm2m_settings_session_idle_timeout_get(void)
{
	return session_idle_timeout;
}

int lwm2m_settings_session_idle_timeout_set(const int32_t new_session_idle_timeout)
{
	session_idle_timeout = new_session_idle_timeout;

	int err = settings_save_one(LWM2M_SETTINGS_SUBTREE_NAME "/"
				    SESSION_IDLE_TIMEOUT_SETTINGS_KEY,
				    &session_idle_timeout, sizeof(session_idle_timeout));
	if (err) {
		LOG_ERR("Save " SESSION_IDLE_TIMEOUT_SETTINGS_KEY " failed: %d", err);
	}

	return err;
}

int32_t lwm2m_settings_coap_con_interval_get(void)
{
	return coap_con_interval;
}

int lwm2m_settings_coap_con_interval_set(const int32_t new_coap_con_interval)
{
	coap_con_interval = new_coap_con_interval;

	int err = settings_save_one(LWM2M_SETTINGS_SUBTREE_NAME "/" COAP_CON_INTERVAL_SETTINGS_KEY,
				    &coap_con_interval, sizeof(coap_con_interval));
	if (err) {
		LOG_ERR("Save " COAP_CON_INTERVAL_SETTINGS_KEY " failed: %d", err);
	}

	return err;
}

const char *lwm2m_settings_apn_get(void)
{
	return apn;
}

int lwm2m_settings_apn_set(const char *new_apn)
{
	int err;
	size_t apn_max_len = sizeof(apn) - 1;
	size_t new_apn_len = strlen(new_apn);

	if (new_apn_len > apn_max_len) {
		LOG_ERR("String is too long. Max size is %d", apn_max_len);
		return -ENOMEM;
	}

	memset(apn, 0, sizeof(apn));
	strncpy(apn, new_apn, sizeof(apn) - 1);

	if (new_apn_len == 0) {
		err = settings_delete(LWM2M_SETTINGS_SUBTREE_NAME "/" APN_SETTINGS_KEY);

		if (err) {
			LOG_ERR("Delete " APN_SETTINGS_KEY " failed: %d", err);
		}
	} else {
		err = settings_save_one(LWM2M_SETTINGS_SUBTREE_NAME "/" APN_SETTINGS_KEY,
					apn, strlen(apn));
		if (err) {
			LOG_ERR("Save " APN_SETTINGS_KEY " failed: %d", err);
		}
	}

	return err;
}

int32_t lwm2m_settings_pdn_type_get(void)
{
	return pdn_type;
}

int lwm2m_settings_pdn_type_set(uint32_t new_pdn_type)
{
	pdn_type = new_pdn_type;

	int err = settings_save_one(LWM2M_SETTINGS_SUBTREE_NAME "/" PDN_TYPE_SETTINGS_KEY,
				    &pdn_type, sizeof(pdn_type));
	if (err) {
		LOG_ERR("Save " PDN_TYPE_SETTINGS_KEY " failed: %d", err);
	}

	return err;
}

bool lwm2m_settings_enable_custom_device_config_get(void)
{
	return enable_custom_device_config;
}

int lwm2m_settings_enable_custom_device_config_set(bool new_device_custom_server_config)
{
	enable_custom_device_config = new_device_custom_server_config;

	int err = settings_save_one(LWM2M_SETTINGS_SUBTREE_NAME "/"
				    ENABLE_CUSTOM_DEVICE_SETTINGS_KEY,
				    &enable_custom_device_config,
				    sizeof(enable_custom_device_config));
	if (err) {
		LOG_ERR("Save " ENABLE_CUSTOM_DEVICE_SETTINGS_KEY " failed: %d", err);
	}

	return err;
}

const char *lwm2m_settings_manufacturer_get(void)
{
	return manufacturer;
}

int lwm2m_settings_manufacturer_set(const char *new_manufacturer)
{
	int err;
	size_t manufacturer_max_len = sizeof(manufacturer) - 1;
	size_t new_manufacturer_len = strlen(new_manufacturer);

	if (new_manufacturer_len > manufacturer_max_len) {
		LOG_ERR("String is too long. Max size is %d", manufacturer_max_len);
		return -ENOMEM;
	}

	memset(manufacturer, 0, sizeof(manufacturer));
	strncpy(manufacturer, new_manufacturer, sizeof(manufacturer) - 1);

	if (new_manufacturer_len == 0) {
		err = settings_delete(LWM2M_SETTINGS_SUBTREE_NAME "/" MANUFACTURER_SETTINGS_KEY);

		if (err) {
			LOG_ERR("Delete " MANUFACTURER_SETTINGS_KEY " failed: %d", err);
		}
	} else {
		err = settings_save_one(LWM2M_SETTINGS_SUBTREE_NAME "/" MANUFACTURER_SETTINGS_KEY,
					manufacturer, strlen(manufacturer));
		if (err) {
			LOG_ERR("Save " MANUFACTURER_SETTINGS_KEY " failed: %d", err);
		}
	}

	return err;
}

const char *lwm2m_settings_model_number_get(void)
{
	return model_number;
}

int lwm2m_settings_model_number_set(const char *new_model_number)
{
	int err;
	size_t model_number_max_len = sizeof(model_number) - 1;
	size_t new_model_number_len = strlen(new_model_number);

	if (new_model_number_len > model_number_max_len) {
		LOG_ERR("String is too long. Max size is %d", model_number_max_len);
		return -ENOMEM;
	}

	memset(model_number, 0, sizeof(model_number));
	strncpy(model_number, new_model_number, sizeof(model_number) - 1);

	if (new_model_number_len == 0) {
		err = settings_delete(LWM2M_SETTINGS_SUBTREE_NAME "/" MODEL_NUMBER_SETTINGS_KEY);

		if (err) {
			LOG_ERR("Delete " MODEL_NUMBER_SETTINGS_KEY " failed: %d", err);
		}
	} else {
		err = settings_save_one(LWM2M_SETTINGS_SUBTREE_NAME "/" MODEL_NUMBER_SETTINGS_KEY,
					model_number, strlen(model_number));
		if (err) {
			LOG_ERR("Save " MODEL_NUMBER_SETTINGS_KEY " failed: %d", err);
		}
	}

	return err;
}

const char *lwm2m_settings_device_type_get(void)
{
	return device_type;
}

int lwm2m_settings_device_type_set(const char *new_device_type)
{
	int err;
	size_t device_type_max_len = sizeof(device_type) - 1;
	size_t new_device_type_len = strlen(new_device_type);

	if (new_device_type_len > device_type_max_len) {
		LOG_ERR("String is too long. Max size is %d", device_type_max_len);
		return -ENOMEM;
	}

	memset(device_type, 0, sizeof(device_type));
	strncpy(device_type, new_device_type, sizeof(device_type) - 1);

	if (new_device_type_len == 0) {
		err = settings_delete(LWM2M_SETTINGS_SUBTREE_NAME "/" DEVICE_TYPE_SETTINGS_KEY);

		if (err) {
			LOG_ERR("Delete " DEVICE_TYPE_SETTINGS_KEY " failed: %d", err);
		}
	} else {
		err = settings_save_one(LWM2M_SETTINGS_SUBTREE_NAME "/" DEVICE_TYPE_SETTINGS_KEY,
					device_type, strlen(device_type));
		if (err) {
			LOG_ERR("Save " DEVICE_TYPE_SETTINGS_KEY " failed: %d", err);
		}
	}

	return err;
}

const char *lwm2m_settings_hardware_version_get(void)
{
	return hardware_version;
}

int lwm2m_settings_hardware_version_set(const char *new_hardware_version)
{
	int err;
	size_t hardware_version_max_len = sizeof(hardware_version) - 1;
	size_t new_hardware_version_len = strlen(new_hardware_version);

	if (new_hardware_version_len > hardware_version_max_len) {
		LOG_ERR("String is too long. Max size is %d", hardware_version_max_len);
		return -ENOMEM;
	}

	memset(hardware_version, 0, sizeof(hardware_version));
	strncpy(hardware_version, new_hardware_version, sizeof(hardware_version) - 1);

	if (new_hardware_version_len == 0) {
		err =
		settings_delete(LWM2M_SETTINGS_SUBTREE_NAME "/" HARDWARE_VERSION_SETTINGS_KEY);

		if (err) {
			LOG_ERR("Delete " HARDWARE_VERSION_SETTINGS_KEY " failed: %d", err);
		}
	} else {
		err =
		settings_save_one(LWM2M_SETTINGS_SUBTREE_NAME "/" HARDWARE_VERSION_SETTINGS_KEY,
				  hardware_version, strlen(hardware_version));
		if (err) {
			LOG_ERR("Save " HARDWARE_VERSION_SETTINGS_KEY " failed: %d", err);
		}
	}

	return err;
}

const char *lwm2m_settings_software_version_get(void)
{
	return software_version;
}

int lwm2m_settings_software_version_set(const char *new_software_version)
{
	int err;
	size_t software_version_max_len = sizeof(software_version) - 1;
	size_t new_software_version_len = strlen(new_software_version);

	if (new_software_version_len > software_version_max_len) {
		LOG_ERR("String is too long. Max size is %d", software_version_max_len);
		return -ENOMEM;
	}

	memset(software_version, 0, sizeof(software_version));
	strncpy(software_version, new_software_version, sizeof(software_version) - 1);

	if (new_software_version_len == 0) {
		err =
		settings_delete(LWM2M_SETTINGS_SUBTREE_NAME "/" SOFTWARE_VERSION_SETTINGS_KEY);

		if (err) {
			LOG_ERR("Delete " SOFTWARE_VERSION_SETTINGS_KEY " failed: %d", err);
		}
	} else {
		err =
		settings_save_one(LWM2M_SETTINGS_SUBTREE_NAME "/" SOFTWARE_VERSION_SETTINGS_KEY,
				  software_version, strlen(software_version));
		if (err) {
			LOG_ERR("Save " SOFTWARE_VERSION_SETTINGS_KEY " failed: %d", err);
		}
	}

	return err;
}

const char *lwm2m_settings_service_code_get(void)
{
	return service_code;
}

int lwm2m_settings_service_code_set(const char *new_service_code)
{
	int err;
	size_t service_code_max_len = sizeof(service_code) - 1;
	size_t new_service_code_len = strlen(new_service_code);

	if (new_service_code_len > service_code_max_len) {
		LOG_ERR("String is too long. Max size is %d", service_code_max_len);
		return -ENOMEM;
	}

	memset(service_code, 0, sizeof(service_code));
	strncpy(service_code, new_service_code, sizeof(service_code) - 1);

	if (new_service_code_len == 0) {
		err = settings_delete(LWM2M_SETTINGS_SUBTREE_NAME "/" SERVICE_CODE_SETTINGS_KEY);

		if (err) {
			LOG_ERR("Delete " SERVICE_CODE_SETTINGS_KEY " failed: %d", err);
		}
	} else {
		err = settings_save_one(LWM2M_SETTINGS_SUBTREE_NAME "/" SERVICE_CODE_SETTINGS_KEY,
					service_code, strlen(service_code));
		if (err) {
			LOG_ERR("Save " SERVICE_CODE_SETTINGS_KEY " failed: %d", err);
		}
	}

	return err;
}

bool lwm2m_settings_device_serial_no_type_get(void)
{
	return device_serial_no_type;
}

int lwm2m_settings_device_serial_no_type_set(bool new_device_serial_no_type)
{
	device_serial_no_type = new_device_serial_no_type;

	int err = settings_save_one(LWM2M_SETTINGS_SUBTREE_NAME "/"
				    DEVICE_SERIAL_NO_TYPE_SETTINGS_KEY,
				    &device_serial_no_type, sizeof(device_serial_no_type));
	if (err) {
		LOG_ERR("Save " DEVICE_SERIAL_NO_TYPE_SETTINGS_KEY " failed: %d", err);
	}

	return err;
}

uint16_t lwm2m_settings_firmware_download_timeout_get(void)
{
	return firmware_download_timeout;
}

int lwm2m_settings_firmware_download_timeout_set(const uint16_t new_firmware_download_timeout)
{
	firmware_download_timeout = new_firmware_download_timeout;

	int err = settings_save_one(LWM2M_SETTINGS_SUBTREE_NAME "/"
				    FIRMWARE_DOWNLOAD_TIMEOUT_SETTINGS_KEY,
				    &firmware_download_timeout, sizeof(firmware_download_timeout));
	if (err) {
		LOG_ERR("Save " FIRMWARE_DOWNLOAD_TIMEOUT_SETTINGS_KEY " failed: %d", err);
	}

	return err;
}

bool lwm2m_settings_queue_mode_get(void)
{
	return queue_mode;
}

int lwm2m_settings_queue_mode_set(bool new_queue_mode)
{
	queue_mode = new_queue_mode;

	int err = settings_save_one(LWM2M_SETTINGS_SUBTREE_NAME "/" QUEUE_MODE_KEY,
				    &queue_mode, sizeof(queue_mode));
	if (err) {
		LOG_ERR("Save " QUEUE_MODE_KEY " failed: %d", err);
	}

	return err;
}

bool lwm2m_settings_auto_register_get(void)
{
	return auto_register;
}

int lwm2m_settings_auto_register_set(bool new_auto_register)
{
	auto_register = new_auto_register;

	int err = settings_save_one(LWM2M_SETTINGS_SUBTREE_NAME "/" AUTO_REGISTER_KEY,
				    &auto_register, sizeof(auto_register));
	if (err) {
		LOG_ERR("Save " AUTO_REGISTER_KEY " failed: %d", err);
	}

	return err;
}

int lwm2m_carrier_custom_init(lwm2m_carrier_config_t *config)
{
	int err = settings_init();

	if (!auto_startup) {
		LOG_WRN("Waiting for automatic startup to be enabled");
		k_sem_take(&auto_startup_sem, K_FOREVER);
	}

	settings_enable(config);

	return err;
}
