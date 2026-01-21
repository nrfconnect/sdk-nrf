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
#include <hw_id.h>

#ifdef CONFIG_NRF_MODEM_LIB
#include <modem/nrf_modem_lib.h>
#include <nrf_modem_at.h>
#endif

#if defined(CONFIG_MEMFAULT_NCS_DEVICE_ID_NET_MAC)
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi.h>
#endif /* defined(CONFIG_MEMFAULT_NCS_DEVICE_ID_NET_MAC) */

#include <memfault/core/build_info.h>
#include <memfault/core/compiler.h>
#include <memfault/core/platform/device_info.h>
#include <memfault/http/http_client.h>
#include <memfault/ports/zephyr/http.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(memfault_ncs, CONFIG_MEMFAULT_NCS_LOG_LEVEL);

#define IMEI_LEN 15

#if defined(CONFIG_SOC_SERIES_NRF91)
#define MEMFAULT_URL "https://goto.memfault.com/create-key/nrf91"
#else
#define MEMFAULT_URL "https://goto.memfault.com/create-key/nrf"
#endif

#if defined(CONFIG_BT_MDS) || defined(CONFIG_MEMFAULT_HTTP_ENABLE)
/* Project key check */
BUILD_ASSERT(sizeof(CONFIG_MEMFAULT_NCS_PROJECT_KEY) > 1,
	     "Memfault Project Key not configured. Please visit " MEMFAULT_URL " ");
#endif

extern void memfault_ncs_metrics_init(void);

/* Memfault HTTP client configuration
 *
 * This symbol has public scope- it's used by the Memfault SDK when executing
 * HTTP requests
 */
sMfltHttpClientConfig g_mflt_http_client_config = {
	.api_key = CONFIG_MEMFAULT_NCS_PROJECT_KEY,
};

#if defined(CONFIG_MEMFAULT_NCS_DEVICE_INFO_BUILTIN)
/* Firmware type check */
BUILD_ASSERT(sizeof(CONFIG_MEMFAULT_NCS_FW_TYPE) > 1, "Firmware type must be configured");

/* Firmware version checks */
#if defined(CONFIG_MEMFAULT_NCS_FW_VERSION_STATIC)
BUILD_ASSERT(sizeof(CONFIG_MEMFAULT_NCS_FW_VERSION_STATIC) > 1,
	     "Firmware version must be configured");
#endif

#if defined(CONFIG_MEMFAULT_NCS_DEVICE_ID_HW_ID)
static char device_serial[HW_ID_LEN + 1];
#elif defined(CONFIG_MEMFAULT_NCS_DEVICE_ID_NET_MAC) || defined(CONFIG_MEMFAULT_NCS_DEVICE_ID_IMEI)
static char device_serial[15 + 1]; /* IMEI is 15 characters, NET_MAC is 12 characters */
#elif defined(CONFIG_MEMFAULT_NCS_DEVICE_ID_STATIC)
BUILD_ASSERT(sizeof(CONFIG_MEMFAULT_NCS_DEVICE_ID) > 1, "The device ID must be configured");
static char device_serial[] = CONFIG_MEMFAULT_NCS_DEVICE_ID;
#elif defined(CONFIG_MEMFAULT_NCS_DEVICE_ID_RUNTIME)
static char device_serial[CONFIG_MEMFAULT_NCS_DEVICE_ID_MAX_LEN + 1];
#endif

/* Hardware version check */
BUILD_ASSERT(sizeof(CONFIG_MEMFAULT_NCS_HW_VERSION) > 1, "Hardware version must be configured");

#if defined(CONFIG_MEMFAULT_NCS_DEVICE_ID_HW_ID) && \
	defined(CONFIG_HW_ID_LIBRARY_SOURCE_BT_DEVICE_ADDRESS)
/* Forward declaration needed when fetching device info in this configuration */
static int device_info_init(void);
#endif

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

	/* When using hw_id and Bluetooth Address, the address is only available
	 * after bt_enable() or settings_load() has completed (bt_enable() can run
	 * deferred too!). It's possible the device serial has not been set: if this
	 * is running from non-ISR context, check and try to retrieve it again.
	 */
#if defined(CONFIG_MEMFAULT_NCS_DEVICE_ID_HW_ID) && \
	defined(CONFIG_HW_ID_LIBRARY_SOURCE_BT_DEVICE_ADDRESS)
	if (!k_is_in_isr()) {
		/* check if device_serial is "Unknown" */
		if (strcmp(device_serial, "Unknown") == 0) {
			(void)device_info_init();
		}
	}
#endif

	*info = (sMemfaultDeviceInfo){
		.device_serial = device_serial,
		.software_type = CONFIG_MEMFAULT_NCS_FW_TYPE,
		.software_version = fw_version,
		.hardware_version = CONFIG_MEMFAULT_NCS_HW_VERSION,
	};
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
#endif /* defined(CONFIG_MEMFAULT_NCS_DEVICE_INFO_BUILTIN) */

#if defined(CONFIG_MEMFAULT_NCS_DEVICE_ID_HW_ID)
static int device_info_init(void)
{
	int err;
	char hw_id_buf[HW_ID_LEN];

	err = hw_id_get(hw_id_buf, sizeof(hw_id_buf));
	if (err) {
		strncpy(device_serial, "Unknown", sizeof(device_serial) - 1);
#if defined(CONFIG_HW_ID_LIBRARY_SOURCE_BT_DEVICE_ADDRESS)
		err = 0; /* Not a critical error if BLE MAC is not ready yet */
#else
		LOG_ERR("Failed to get HW ID, error: %d", err);
#endif
	} else {
		strncpy(device_serial, hw_id_buf, sizeof(device_serial) - 1);
	}
	device_serial[sizeof(device_serial) - 1] = '\0';

	LOG_DBG("Device serial generated: %s", device_serial);

	return err;
}
#elif defined(CONFIG_MEMFAULT_NCS_DEVICE_ID_IMEI)
/* Note: IMEI and NET MAC implementations below duplicates functionality in the
 * hw_id library, but these are deprecated options and will be removed in the
 * future.
 */
static int device_info_init(void)
{
	char imei_buf[IMEI_LEN + 6 + 1]; /* Add 6 for \r\nOK\r\n and 1 for \0 */

	/* Retrieve device IMEI from modem. */
	int err = nrf_modem_at_cmd(imei_buf, ARRAY_SIZE(imei_buf), "AT+CGSN");

	if (err) {
		LOG_ERR("Failed to get IMEI, error: %d", err);
		return err;
	}

	/* Set null character at the end of the device IMEI. */
	imei_buf[IMEI_LEN] = 0;

	(void)strncpy(device_serial, imei_buf, sizeof(device_serial) - 1);
	device_serial[sizeof(device_serial) - 1] = '\0';

	return 0;
}
#elif defined(CONFIG_MEMFAULT_NCS_DEVICE_ID_NET_MAC)
static int device_info_init(void)
{
	struct net_if *iface = net_if_get_default();

	if (iface == NULL) {
		return -EIO;
	}

	struct net_linkaddr *linkaddr = net_if_get_link_addr(iface);

	if ((linkaddr == NULL) || (linkaddr->len != WIFI_MAC_ADDR_LEN)) {
		LOG_ERR("Invalid link address");
		return -EIO;
	}

	int ret = snprintk(device_serial, sizeof(device_serial), "%02X%02X%02X%02X%02X%02X",
			   linkaddr->addr[0], linkaddr->addr[1], linkaddr->addr[2],
			   linkaddr->addr[3], linkaddr->addr[4], linkaddr->addr[5]);

	if (ret < 0 || ret >= sizeof(device_serial)) {
		LOG_ERR("Failed to format device serial");
		return -ENOMEM;
	}

	return 0;
}
#endif /* defined(CONFIG_MEMFAULT_NCS_DEVICE_ID_HW_ID) */

static int init(void)
{
	int err = 0;

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

#if defined(CONFIG_MEMFAULT_NCS_DEVICE_ID_HW_ID) || defined(CONFIG_MEMFAULT_NCS_DEVICE_ID_IMEI) || \
	defined(CONFIG_MEMFAULT_NCS_DEVICE_ID_NET_MAC)
	err = device_info_init();
	if (err) {
		LOG_ERR("Device info initialization failed, error: %d", err);
	}
#endif

	if (IS_ENABLED(CONFIG_MEMFAULT_NCS_USE_DEFAULT_METRICS)) {
		memfault_ncs_metrics_init();
	}

	return err;
}

#if defined(CONFIG_NRF_MODEM_LIB)
NRF_MODEM_LIB_ON_INIT(memfault_ncs_init_hook, on_modem_lib_init, NULL);

static void on_modem_lib_init(int ret, void *ctx)
{
	if (ret != 0) {
		LOG_ERR("Modem library did not initialize: %d", ret);
		return;
	}

	init();
}

#else
SYS_INIT(init, APPLICATION, CONFIG_MEMFAULT_NCS_INIT_PRIORITY);
#endif
