/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include <lwm2m_settings.h>
#include <lwm2m_carrier.h>
#include <zephyr/settings/settings.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(lwm2m_settings, CONFIG_LOG_DEFAULT_LEVEL);

#define LWM2M_SETTINGS_SUBTREE_NAME "carrier_init"

#define ENABLE_CUSTOM_CONFIG_SETTINGS_KEY		"enable_custom_config_settings"
#define CERTIFICATION_MODE_SETTINGS_KEY			"certification_mode"
#define DISABLE_BOOTSTRAP_FROM_SMARTCARD_SETTINGS_KEY	"disable_bootstrap_from_smartcard"

#define IS_BOOTSTRAP_SERVER_SETTINGS_KEY	"is_bootstrap_server"
#define SERVER_URI_SETTINGS_KEY			"server_uri"
#define SERVER_PSK_SETTINGS_KEY			"server_psk"
#define SERVER_LIFETIME_SETTINGS_KEY		"server_lifetime"
#define SESSION_IDLE_TIMEOUT_SETTINGS_KEY	"session_idle_timeout"

#define APN_SETTINGS_KEY		"apn"
#define MANUFACTURER_SETTINGS_KEY	"manufacturer"
#define MODEL_NUMBER_SETTINGS_KEY	"model_number"
#define DEVICE_TYPE_SETTINGS_KEY	"device_type"
#define HARDWARE_VERSION_SETTINGS_KEY	"hardware_version"
#define SOFTWARE_VERSION_SETTINGS_KEY	"software_version"
#define SERVICE_CODE_SETTINGS_KEY	"service_code"

static bool enable_custom_config;
static bool certification_mode;
static bool disable_bootstrap_from_smartcard;

static bool is_bootstrap_server;
static char server_uri[256];
static char server_psk[65];
static int32_t server_lifetime;
static int32_t session_idle_timeout;

static char apn[64];
static char manufacturer[33];
static char model_number[33];
static char device_type[33];
static char hardware_version[33];
static char software_version[33];
static char service_code[6];

static int settings_load_cb(const char *key, size_t len, settings_read_cb read_cb, void *cb_arg)
{
	ARG_UNUSED(len);
	ssize_t sz = 0;

	if (strcmp(key, ENABLE_CUSTOM_CONFIG_SETTINGS_KEY) == 0) {
		sz = read_cb(cb_arg, &enable_custom_config, sizeof(enable_custom_config));
	} else if (strcmp(key, CERTIFICATION_MODE_SETTINGS_KEY) == 0) {
		sz = read_cb(cb_arg, &certification_mode, sizeof(certification_mode));
	} else if (strcmp(key, DISABLE_BOOTSTRAP_FROM_SMARTCARD_SETTINGS_KEY) == 0) {
		sz = read_cb(cb_arg, &disable_bootstrap_from_smartcard,
		sizeof(disable_bootstrap_from_smartcard));
	} else if (strcmp(key, IS_BOOTSTRAP_SERVER_SETTINGS_KEY) == 0) {
		sz = read_cb(cb_arg, &is_bootstrap_server, sizeof(is_bootstrap_server));
	} else if (strcmp(key, SERVER_URI_SETTINGS_KEY) == 0) {
		sz = read_cb(cb_arg, server_uri, sizeof(server_uri));
	} else if (strcmp(key, SERVER_LIFETIME_SETTINGS_KEY) == 0) {
		sz = read_cb(cb_arg, &server_lifetime, sizeof(server_lifetime));
	} else if (strcmp(key, SESSION_IDLE_TIMEOUT_SETTINGS_KEY) == 0) {
		sz = read_cb(cb_arg, &session_idle_timeout, sizeof(session_idle_timeout));
	} else if (strcmp(key, SERVER_PSK_SETTINGS_KEY) == 0) {
		sz = read_cb(cb_arg, server_psk, sizeof(server_psk));
	} else if (strcmp(key, APN_SETTINGS_KEY) == 0) {
		sz = read_cb(cb_arg, apn, sizeof(apn));
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
	} else if (strcmp(key, SERVICE_CODE_SETTINGS_KEY) == 0) {
		sz = read_cb(cb_arg, service_code, sizeof(service_code));
	}

	if (sz < 0) {
		LOG_ERR("Read %s failed: %d", key, sz);
	}

	return 0;
}

static void settings_enable_custom_config(lwm2m_carrier_config_t *config)
{
	LOG_INF("Enable custom configuration");

	config->certification_mode = certification_mode;
	config->disable_bootstrap_from_smartcard = disable_bootstrap_from_smartcard;
	config->is_bootstrap_server = is_bootstrap_server;
	config->server_uri = server_uri;
	config->psk = server_psk;
	config->server_lifetime = server_lifetime;
	config->session_idle_timeout = session_idle_timeout;
	config->apn = apn;
	config->manufacturer = manufacturer;
	config->model_number = model_number;
	config->device_type = device_type;
	config->hardware_version = hardware_version;
	config->software_version = software_version;
	config->service_code = service_code;
}

int lwm2m_settings_init(lwm2m_carrier_config_t *config)
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

	if (enable_custom_config) {
		settings_enable_custom_config(config);
	}

	return 0;
}

bool lwm2m_settings_enable_custom_config_get(void)
{
	return enable_custom_config;
}

int lwm2m_settings_enable_custom_config_set(bool new_enable_custom_config)
{
	enable_custom_config = new_enable_custom_config;

	int err = settings_save_one(LWM2M_SETTINGS_SUBTREE_NAME "/"
					ENABLE_CUSTOM_CONFIG_SETTINGS_KEY, &enable_custom_config,
					sizeof(enable_custom_config));
	if (err) {
		LOG_ERR("Save " ENABLE_CUSTOM_CONFIG_SETTINGS_KEY " failed: %d", err);
	}

	return err;
}

bool lwm2m_settings_certification_mode_get(void)
{
	return certification_mode;
}

int lwm2m_settings_certification_mode_set(bool new_certification_mode)
{
	certification_mode = new_certification_mode;

	int err = settings_save_one(LWM2M_SETTINGS_SUBTREE_NAME "/"
					CERTIFICATION_MODE_SETTINGS_KEY, &certification_mode,
					sizeof(certification_mode));
	if (err) {
		LOG_ERR("Save " CERTIFICATION_MODE_SETTINGS_KEY " failed: %d", err);
	}

	return err;
}

bool lwm2m_settings_disable_bootstrap_from_smartcard_get(void)
{
	return disable_bootstrap_from_smartcard;
}

int lwm2m_settings_disable_bootstrap_from_smartcard_set(bool new_disable_bootstrap_from_smartcard)
{
	disable_bootstrap_from_smartcard = new_disable_bootstrap_from_smartcard;

	int err = settings_save_one(LWM2M_SETTINGS_SUBTREE_NAME "/"
					DISABLE_BOOTSTRAP_FROM_SMARTCARD_SETTINGS_KEY,
				    &disable_bootstrap_from_smartcard,
					sizeof(disable_bootstrap_from_smartcard));
	if (err) {
		LOG_ERR("Save " DISABLE_BOOTSTRAP_FROM_SMARTCARD_SETTINGS_KEY " failed: %d", err);
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
					IS_BOOTSTRAP_SERVER_SETTINGS_KEY, &is_bootstrap_server,
					sizeof(is_bootstrap_server));
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
	size_t server_uri_len = strlen(new_server_uri);

	if (server_uri_len > server_uri_max_len) {
		LOG_ERR("String is too long. Max size is %d", server_uri_max_len);
		return -ENOMEM;
	}

	memset(server_uri, 0, sizeof(server_uri));
	strncpy(server_uri, new_server_uri, sizeof(server_uri) - 1);

	int err = settings_save_one(LWM2M_SETTINGS_SUBTREE_NAME "/" SERVER_URI_SETTINGS_KEY,
				    server_uri, strlen(server_uri));
	if (err) {
		LOG_ERR("Save " SERVER_URI_SETTINGS_KEY " failed: %d", err);
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

int32_t lwm2m_settings_session_idle_timeout_get(void)
{
	return session_idle_timeout;
}

int lwm2m_settings_session_idle_timeout_set(const int32_t new_session_idle_timeout)
{
	session_idle_timeout = new_session_idle_timeout;

	int err = settings_save_one(LWM2M_SETTINGS_SUBTREE_NAME "/"
					SESSION_IDLE_TIMEOUT_SETTINGS_KEY, &session_idle_timeout,
					sizeof(session_idle_timeout));
	if (err) {
		LOG_ERR("Save " SESSION_IDLE_TIMEOUT_SETTINGS_KEY " failed: %d", err);
	}

	return err;
}

const char *lwm2m_settings_server_psk_get(void)
{
	return server_psk;
}

int lwm2m_settings_server_psk_set(const char *new_server_psk)
{
	size_t server_psk_max_len = sizeof(server_psk) - 1;
	size_t server_psk_len = strlen(new_server_psk);

	if (server_psk_len > server_psk_max_len) {
		LOG_ERR("String is too long. Max size is %d", server_psk_max_len);
		return -ENOMEM;
	}

	memset(server_psk, 0, sizeof(server_psk));
	strncpy(server_psk, new_server_psk, sizeof(server_psk) - 1);

	int err = settings_save_one(LWM2M_SETTINGS_SUBTREE_NAME "/" SERVER_PSK_SETTINGS_KEY,
				    server_psk, strlen(server_psk));
	if (err) {
		LOG_ERR("Save " SERVER_PSK_SETTINGS_KEY " failed: %d", err);
	}

	return err;
}


const char *lwm2m_settings_apn_get(void)
{
	return apn;
}

int lwm2m_settings_apn_set(const char *new_apn)
{
	size_t apn_max_len = sizeof(apn) - 1;
	size_t apn_len = strlen(new_apn);

	if (apn_len > apn_max_len) {
		LOG_ERR("String is too long. Max size is %d", apn_max_len);
		return -ENOMEM;
	}

	memset(apn, 0, sizeof(apn));
	strncpy(apn, new_apn, sizeof(apn) - 1);

	int err = settings_save_one(LWM2M_SETTINGS_SUBTREE_NAME "/" APN_SETTINGS_KEY,
				    apn, strlen(apn));
	if (err) {
		LOG_ERR("Save " APN_SETTINGS_KEY " failed: %d", err);
	}

	return err;
}

const char *lwm2m_settings_manufacturer_get(void)
{
	return manufacturer;
}

int lwm2m_settings_manufacturer_set(const char *new_manufacturer)
{
	size_t manufacturer_max_len = sizeof(manufacturer) - 1;
	size_t new_manufacturer_len = strlen(new_manufacturer);

	if (new_manufacturer_len > manufacturer_max_len) {
		LOG_ERR("String is too long. Max size is %d", manufacturer_max_len);
		return -ENOMEM;
	}

	memset(manufacturer, 0, sizeof(manufacturer));
	strncpy(manufacturer, new_manufacturer, sizeof(manufacturer) - 1);

	int err = settings_save_one(LWM2M_SETTINGS_SUBTREE_NAME "/" MANUFACTURER_SETTINGS_KEY,
				    manufacturer, strlen(manufacturer));
	if (err) {
		LOG_ERR("Save " MANUFACTURER_SETTINGS_KEY " failed: %d", err);
	}

	return err;
}

const char *lwm2m_settings_model_number_get(void)
{
	return model_number;
}

int lwm2m_settings_model_number_set(const char *new_model_number)
{
	size_t model_number_max_len = sizeof(model_number) - 1;
	size_t new_model_number_len = strlen(new_model_number);

	if (new_model_number_len > model_number_max_len) {
		LOG_ERR("String is too long. Max size is %d", model_number_max_len);
		return -ENOMEM;
	}

	memset(model_number, 0, sizeof(model_number));
	strncpy(model_number, new_model_number, sizeof(model_number) - 1);

	int err = settings_save_one(LWM2M_SETTINGS_SUBTREE_NAME "/" MODEL_NUMBER_SETTINGS_KEY,
				    model_number, strlen(model_number));
	if (err) {
		LOG_ERR("Save " MODEL_NUMBER_SETTINGS_KEY " failed: %d", err);
	}

	return err;
}

const char *lwm2m_settings_device_type_get(void)
{
	return device_type;
}

int lwm2m_settings_device_type_set(const char *new_device_type)
{
	size_t device_type_max_len = sizeof(device_type) - 1;
	size_t new_device_type_len = strlen(new_device_type);

	if (new_device_type_len > device_type_max_len) {
		LOG_ERR("String is too long. Max size is %d", device_type_max_len);
		return -ENOMEM;
	}

	memset(device_type, 0, sizeof(device_type));
	strncpy(device_type, new_device_type, sizeof(device_type) - 1);

	int err = settings_save_one(LWM2M_SETTINGS_SUBTREE_NAME "/" DEVICE_TYPE_SETTINGS_KEY,
				    device_type, strlen(device_type));
	if (err) {
		LOG_ERR("Save " DEVICE_TYPE_SETTINGS_KEY " failed: %d", err);
	}

	return err;
}

const char *lwm2m_settings_hardware_version_get(void)
{
	return hardware_version;
}

int lwm2m_settings_hardware_version_set(const char *new_hardware_version)
{
	size_t hardware_version_max_len = sizeof(hardware_version) - 1;
	size_t new_hardware_version_len = strlen(new_hardware_version);

	if (new_hardware_version_len > hardware_version_max_len) {
		LOG_ERR("String is too long. Max size is %d", hardware_version_max_len);
		return -ENOMEM;
	}

	memset(hardware_version, 0, sizeof(hardware_version));
	strncpy(hardware_version, new_hardware_version, sizeof(hardware_version) - 1);

	int err = settings_save_one(LWM2M_SETTINGS_SUBTREE_NAME "/" HARDWARE_VERSION_SETTINGS_KEY,
				    hardware_version, strlen(hardware_version));
	if (err) {
		LOG_ERR("Save " HARDWARE_VERSION_SETTINGS_KEY " failed: %d", err);
	}

	return err;
}

const char *lwm2m_settings_software_version_get(void)
{
	return software_version;
}

int lwm2m_settings_software_version_set(const char *new_software_version)
{
	size_t software_version_max_len = sizeof(software_version) - 1;
	size_t new_software_version_len = strlen(new_software_version);

	if (new_software_version_len > software_version_max_len) {
		LOG_ERR("String is too long. Max size is %d", software_version_max_len);
		return -ENOMEM;
	}

	memset(software_version, 0, sizeof(software_version));
	strncpy(software_version, new_software_version, sizeof(software_version) - 1);

	int err = settings_save_one(LWM2M_SETTINGS_SUBTREE_NAME "/" SOFTWARE_VERSION_SETTINGS_KEY,
				    software_version, strlen(software_version));
	if (err) {
		LOG_ERR("Save " SOFTWARE_VERSION_SETTINGS_KEY " failed: %d", err);
	}

	return err;
}

const char *lwm2m_settings_service_code_get(void)
{
	return service_code;
}

int lwm2m_settings_service_code_set(const char *new_service_code)
{
	size_t service_code_max_len = sizeof(service_code) - 1;
	size_t new_service_code_len = strlen(new_service_code);

	if (new_service_code_len > service_code_max_len) {
		LOG_ERR("String is too long. Max size is %d", service_code_max_len);
		return -ENOMEM;
	}

	memset(service_code, 0, sizeof(service_code));
	strncpy(service_code, new_service_code, sizeof(service_code) - 1);

	int err = settings_save_one(LWM2M_SETTINGS_SUBTREE_NAME "/" SERVICE_CODE_SETTINGS_KEY,
				    service_code, strlen(service_code));
	if (err) {
		LOG_ERR("Save " SERVICE_CODE_SETTINGS_KEY " failed: %d", err);
	}

	return err;
}

int lwm2m_carrier_custom_init(lwm2m_carrier_config_t *config)
{
	int	err = lwm2m_settings_init(config);

	return err;
}
