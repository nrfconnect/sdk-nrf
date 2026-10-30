/*
 * Copyright (c) 2023, Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/pm/device.h>
#include <zephyr/drivers/cellular.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/atomic.h>

#include <memfault/components.h>
#include <memfault/ports/zephyr/http.h>
#include <memfault/ports/zephyr/root_cert_storage.h>
#include <memfault_ncs.h>

#include <zephyr/dfu/mcuboot.h>

#include "ota_download.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

static const int memfault_sec_tags[] = {
    kMemfaultRootCert_DigicertRootG2,
    kMemfaultRootCert_AmazonRootCa1,
};

#define L4_EVENT_MASK \
	(NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED | NET_EVENT_DNS_SERVER_ADD)
#define L4_CONNECTED 1
#define L4_DNS_ADDED 2
#define APP_READY 4

#define IMEI_LEN 15

const struct device *modem = DEVICE_DT_GET(DT_ALIAS(modem));

static struct net_if *ppp_iface;
static K_EVENT_DEFINE(l4_event);

enum ota_operation {
	OTA_IDLE,
	OTA_DOWNLOAD,
	OTA_APPLY,
};

static atomic_t ota_operation;
static K_SEM_DEFINE(ota_request, 0, 1);

static void l4_event_handler(uint64_t event, struct net_if *iface, void *info, size_t info_length,
			     void *user_data)
{
	if (iface != ppp_iface) {
		return;
	}

	switch (event) {
	case NET_EVENT_L4_CONNECTED:
		k_event_post(&l4_event, L4_CONNECTED);
		break;
	case NET_EVENT_DNS_SERVER_ADD:
		k_event_post(&l4_event, L4_DNS_ADDED);
		break;
	case NET_EVENT_L4_DISCONNECTED:
		k_event_clear(&l4_event, L4_CONNECTED | L4_DNS_ADDED);
		break;
	default:
		break;
	}
}

NET_MGMT_REGISTER_EVENT_HANDLER(l4_events, L4_EVENT_MASK, l4_event_handler, NULL);

static void ota_download_run(void)
{
	char *url = NULL;
	int ret;

	LOG_INF("Checking for OTA update");
	ret = memfault_zephyr_port_get_download_url(&url);
	if (ret < 0) {
		LOG_ERR("OTA check failed (%d)", ret);
		return;
	}
	if (ret == 0) {
		LOG_INF("Up to date");
		return;
	}

	LOG_INF("Downloading OTA bundle");
	ret = ota_bundle_download(url, memfault_sec_tags, ARRAY_SIZE(memfault_sec_tags));
	memfault_zephyr_port_release_download_url(&url);
	if (ret < 0) {
		LOG_ERR("OTA download failed (%d)", ret);
	} else {
		LOG_INF("Bundle saved to %s", OTA_BUNDLE_PATH);
	}
}

static void ota_thread_fn(void *unused1, void *unused2, void *unused3)
{
	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);
	ARG_UNUSED(unused3);

	while (true) {
		k_sem_take(&ota_request, K_FOREVER);
		if (atomic_get(&ota_operation) == OTA_DOWNLOAD) {
			ota_download_run();
		} else {
			int ret = ota_bundle_apply();

			if (ret < 0) {
				LOG_ERR("OTA apply failed (%d)", ret);
			} else {
				LOG_INF("OTA application transfer complete");
			}
		}
		atomic_set(&ota_operation, OTA_IDLE);
	}
}

K_THREAD_DEFINE(ota_thread, 8192, ota_thread_fn, NULL, NULL, NULL,
		K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);

static int ota_submit(const struct shell *sh, enum ota_operation operation)
{
	if (k_event_test(&l4_event, APP_READY) == 0) {
		shell_error(sh, "Application is not ready");
		return -EAGAIN;
	}
	if (operation == OTA_DOWNLOAD && k_event_test(&l4_event, L4_CONNECTED) == 0) {
		shell_error(sh, "Network is not ready");
		return -EAGAIN;
	}
	if (!atomic_cas(&ota_operation, OTA_IDLE, operation)) {
		shell_error(sh, "An OTA operation is already running");
		return -EBUSY;
	}
	shell_print(sh, "OTA %s queued", operation == OTA_DOWNLOAD ? "download" : "apply");
	k_sem_give(&ota_request);
	return 0;
}

static int cmd_ota_download(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	return ota_submit(sh, OTA_DOWNLOAD);
}

static int cmd_ota_apply(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	return ota_submit(sh, OTA_APPLY);
}

SHELL_STATIC_SUBCMD_SET_CREATE(ota_commands,
	SHELL_CMD_ARG(download, NULL, "Download the available Memfault OTA bundle",
		      cmd_ota_download, 1, 0),
	SHELL_CMD_ARG(apply, NULL, "Verify and apply the saved OTA bundle", cmd_ota_apply, 1, 0),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(ota, &ota_commands, "OTA bundle operations", NULL);

static int memfault_device_id_init(void)
{
	char imei[IMEI_LEN + 1];
	int ret;

	ret = cellular_get_modem_info(modem, CELLULAR_MODEM_INFO_IMEI, imei, sizeof(imei));
	if (ret < 0) {
		LOG_ERR("Failed to get IMEI (%d)", ret);
		return ret;
	}

	LOG_INF("IMEI: %s", imei);

	return memfault_ncs_device_id_set(imei, strlen(imei));
}

int main(void)
{
	int ret;

	ppp_iface = net_if_get_first_by_type(&NET_L2_GET_NAME(PPP));

	LOG_INF("Powering on modem");
	pm_device_action_run(modem, PM_DEVICE_ACTION_RESUME);

	LOG_INF("Bring up network interface");
	ret = net_if_up(ppp_iface);
	if (ret < 0) {
		LOG_ERR("Failed to bring up network interface");
		return -1;
	}

	LOG_INF("Waiting for L4 connected");
	ret = k_event_wait(&l4_event, L4_CONNECTED, false, K_SECONDS(120));
	if (ret != L4_CONNECTED) {
		LOG_ERR("L4 was not connected in time");
		return -1;
	}

	ret = k_event_wait(&l4_event, L4_DNS_ADDED, false, K_SECONDS(10));
	if (ret != L4_DNS_ADDED) {
		LOG_ERR("DNS server was not added in time");
		return -1;
	}

	ret = memfault_device_id_init();
	if (ret < 0) {
		return -1;
	}

	ret = memfault_zephyr_port_install_root_certs();
	if (ret < 0) {
		LOG_ERR("Failed to install Memfault root certs (%d)", ret);
		return -1;
	}

	/* Capture a heartbeat now instead of waiting for the periodic timer. */
	memfault_metrics_heartbeat_debug_trigger();

	LOG_INF("Uploading data to Memfault");
	ret = memfault_zephyr_port_post_data();
	if (ret < 0) {
		LOG_ERR("Memfault upload failed (%d)", ret);
		return -1;
	}

	if (!boot_is_img_confirmed()) {
		LOG_INF("Confirming OTA update");
		ret = boot_write_img_confirmed();
		if (ret < 0) {
			LOG_ERR("Image confirmation failed (%d)", ret);
			return ret;
		}
	}

	k_event_post(&l4_event, APP_READY);
	return 0;
}
