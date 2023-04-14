/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/init.h>
#include <modem/lte_lc_trace.h>
#include <memfault_ncs.h>

#ifdef CONFIG_NRF_MODEM_LIB
#include <modem/nrf_modem_lib.h>
#include <nrf_modem_at.h>
#endif

#include <memfault/core/build_info.h>
#include <memfault/core/compiler.h>
#include <memfault/core/platform/device_info.h>
#include <memfault/http/http_client.h>
#include <memfault/ports/zephyr/http.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(memfault_ncs, CONFIG_MEMFAULT_NCS_LOG_LEVEL);

#define IMEI_LEN 15

#if defined(CONFIG_SOC_NRF9160)
#define MEMFAULT_URL	"https://goto.memfault.com/create-key/nrf91"
#else
#define MEMFAULT_URL	"https://goto.memfault.com/create-key/nrf"
#endif

/* Project key check */
BUILD_ASSERT(sizeof(CONFIG_MEMFAULT_NCS_PROJECT_KEY) > 1,
	"Memfault Project Key not configured. Please visit " MEMFAULT_URL " ");

/* Firmware type check */
BUILD_ASSERT(sizeof(CONFIG_MEMFAULT_NCS_FW_TYPE) > 1, "Firmware type must be configured");

/* Firmware version checks */
#if defined(CONFIG_MEMFAULT_NCS_FW_VERSION_STATIC)
BUILD_ASSERT(sizeof(CONFIG_MEMFAULT_NCS_FW_VERSION_STATIC) > 1,
	     "Firmware version must be configured");
#endif

#if defined(CONFIG_MEMFAULT_NCS_DEVICE_ID_IMEI)
	static char device_serial[IMEI_LEN + 1];
#elif defined(CONFIG_MEMFAULT_NCS_DEVICE_ID_STATIC)
	BUILD_ASSERT(sizeof(CONFIG_MEMFAULT_NCS_DEVICE_ID) > 1,
		     "The device ID must be configured");
	static char device_serial[] = CONFIG_MEMFAULT_NCS_DEVICE_ID;
#elif defined(CONFIG_MEMFAULT_NCS_DEVICE_ID_RUNTIME)
	static char device_serial[CONFIG_MEMFAULT_NCS_DEVICE_ID_MAX_LEN + 1];
#endif

/* Hardware version check */
BUILD_ASSERT(sizeof(CONFIG_MEMFAULT_NCS_HW_VERSION) > 1, "Hardware version must be configured");

extern void memfault_ncs_metrcics_init(void);

sMfltHttpClientConfig g_mflt_http_client_config = {
	.api_key = CONFIG_MEMFAULT_NCS_PROJECT_KEY,
};

#if defined(CONFIG_MEMFAULT_DEVICE_INFO_BUILTIN)
void memfault_platform_get_device_info(sMemfaultDeviceInfo *info)
{
#if defined(CONFIG_MEMFAULT_NCS_FW_VERSION_AUTO)
	static bool is_init;

	static char fw_version[sizeof(CONFIG_MEMFAULT_NCS_FW_VERSION_PREFIX) + 8] =
	    CONFIG_MEMFAULT_NCS_FW_VERSION_PREFIX;

	if (!is_init) {
		const size_t version_len = strlen(fw_version);
		const size_t build_id_chars = 6 + 1 /* '\0' */;
		const size_t build_id_num_chars =
		    MIN(build_id_chars, sizeof(fw_version) - version_len - 1);

		memfault_build_id_get_string(&fw_version[version_len], build_id_num_chars);
		is_init = true;
	}
#else
	static const char *fw_version = CONFIG_MEMFAULT_NCS_FW_VERSION;
#endif /* defined(CONFIG_MEMFAULT_NCS_FW_VERSION_AUTO) */

	*info = (sMemfaultDeviceInfo) {
		.device_serial = device_serial,
		.software_type = CONFIG_MEMFAULT_NCS_FW_TYPE,
		.software_version = fw_version,
		.hardware_version = CONFIG_MEMFAULT_NCS_HW_VERSION,
	};
}
#endif /* defined(CONFIG_MEMFAULT_DEVICE_INFO_BUILTIN) */

#ifdef CONFIG_MEMFAULT_NCS_DEVICE_ID_IMEI
static int request_imei(const char *cmd, char *buf, size_t buf_len)
{
	int err = nrf_modem_at_cmd(buf, buf_len, cmd);

	if (err) {
		LOG_ERR("nrf_modem_at_cmd failed, error: %d", err);
		return -EFAULT;
	}

	return 0;
}

static int device_info_init(void)
{
	int err;
	char imei_buf[IMEI_LEN + 6 + 1]; /* Add 6 for \r\nOK\r\n and 1 for \0 */

	err = request_imei("AT+CGSN", imei_buf, sizeof(imei_buf));
	if (err) {
		strncat(device_serial, "Unknown",
			sizeof(device_serial) - strlen(device_serial) - 1);
		LOG_ERR("Failed to retrieve IMEI");
	} else {
		imei_buf[IMEI_LEN] = '\0';
		strncat(device_serial, imei_buf,
			sizeof(device_serial) - strlen(device_serial) - 1);
	}

	LOG_DBG("Device serial generated: %s", device_serial);

	return err;
}
#endif /* CONFIG_MEMFAULT_NCS_DEVICE_ID_IMEI */

static int init(const struct device *unused)
{
	int err = 0;

	ARG_UNUSED(unused);

	if (IS_ENABLED(CONFIG_MEMFAULT_NCS_PROVISION_CERTIFICATES)) {
		err = memfault_zephyr_port_install_root_certs();
		if (err) {
			LOG_ERR("Failed to provision certificates, error: %d", err);
			LOG_WRN("Certificates can not be provisioned while LTE is active");
			/* We don't consider this a critical failure, as the application
			 * can attempt to provision at a later stage.
			 */
		}
	}

#ifdef CONFIG_MEMFAULT_NCS_DEVICE_ID_IMEI
	err = device_info_init();
	if (err) {
		LOG_ERR("Device info initialization failed, error: %d", err);
	}
#endif /* CONFIG_MEMFAULT_NCS_DEVICE_ID_IMEI */

	if (IS_ENABLED(CONFIG_MEMFAULT_NCS_USE_DEFAULT_METRICS)) {
		memfault_ncs_metrcics_init();
	}

	return err;
}

int memfault_ncs_device_id_set(const char *device_id, size_t len)
{
	if (!IS_ENABLED(CONFIG_MEMFAULT_NCS_DEVICE_ID_RUNTIME)) {
		LOG_ERR("CONFIG_MEMFAULT_NCS_DEVICE_ID_RUNTIME is disabled");
		return -ENOTSUP;
	}

	if (device_id == NULL) {
		return -EINVAL;
	}

	if (len > (sizeof(device_serial) - 1)) {
		LOG_ERR("Device ID is longer than CONFIG_MEMFAULT_NCS_DEVICE_ID_MAX_LEN");
		LOG_WRN("The Memfault device ID will be truncated");
	}

	memcpy(device_serial, device_id, MIN(sizeof(device_serial) - 1, len));

	device_serial[MIN(sizeof(device_serial) - 1, len)] = '\0';

	return 0;
}

#if defined(CONFIG_NRF_MODEM_LIB)
NRF_MODEM_LIB_ON_INIT(memfault_ncs_init_hook, on_modem_lib_init, NULL);

static void on_modem_lib_init(int ret, void *ctx)
{
	if (ret != 0) {
		/* Return if modem initialization failed */
		return;
	}

	init(NULL);
}

#else
SYS_INIT(init, APPLICATION, CONFIG_MEMFAULT_NCS_INIT_PRIORITY);
#endif
