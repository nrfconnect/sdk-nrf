/*
 * Copyright (c) 2023, Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/pm/device.h>
#include <zephyr/drivers/cellular.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/devicetree.h>
#include <zephyr/storage/flash_map.h>

#include <memfault/components.h>
#include <memfault/core/platform/device_info.h>
#include <memfault/ports/zephyr/http.h>
#include <memfault/ports/zephyr/root_cert_storage.h>
#include <memfault_ncs.h>

#include <zephyr/dfu/mcuboot.h>
#include <fw_info.h>

#include "ota_download.h"
#include "sm_xdfu.h"

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
static bool bluetooth_ready;

enum ota_operation {
	OTA_IDLE,
	OTA_DOWNLOAD,
	OTA_RESUME,
	OTA_STATUS,
	OTA_APPLY,
	OTA_VERSIONS,
};

static atomic_t ota_operation;
static K_SEM_DEFINE(ota_request, 0, 1);

static int cmd_modem_hard_reset(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "Hard resetting modem and cold rebooting host");
	modem_hard_reset();
	return 0;
}

SHELL_CMD_ARG_REGISTER(modem_hard_reset, NULL, "Hard reset modem and cold reboot host",
		       cmd_modem_hard_reset, 1, 0);

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

static void ota_resume_run(void)
{
	int ret = ota_bundle_resume(memfault_sec_tags, ARRAY_SIZE(memfault_sec_tags));

	if (ret < 0) {
		LOG_ERR("OTA resume failed (%d)", ret);
	} else {
		LOG_INF("Resumed OTA bundle saved to %s", OTA_BUNDLE_PATH);
	}
}

static const struct fw_info *firmware_info_at(uint32_t address)
{
	const struct fw_info *info = fw_info_find(address);

	if (info == NULL || info->valid != CONFIG_FW_INFO_VALID_VAL) {
		return NULL;
	}
	return info;
}

static void ota_versions_run(void)
{
	const struct fw_info *s0_info = firmware_info_at(DT_REG_ADDR(DT_NODELABEL(s0_partition)));
	const struct fw_info *s1_info = firmware_info_at(DT_REG_ADDR(DT_NODELABEL(s1_partition)));
	sMemfaultDeviceInfo device_info = {0};
	struct mcuboot_img_header app_secondary_header;
	struct mcuboot_img_header net_secondary_header;
	char mfw_version[128] = {0};
	char slm_version[64] = {0};
	bool was_up = net_if_is_up(ppp_iface);
	bool s0_valid = s0_info != NULL;
	bool s1_valid = s1_info != NULL;
	const char *active_slot = "unknown";
	int ret;
	struct net_buf *response = NULL;

	memfault_platform_get_device_info(&device_info);
	LOG_INF("nRF52 app version: %s",
		device_info.software_version != NULL ? device_info.software_version : "unavailable");

	if (s0_valid && s1_valid) {
		active_slot = s0_info->version >= s1_info->version ? "S0" : "S1";
	} else if (s0_valid) {
		active_slot = "S0";
	} else if (s1_valid) {
		active_slot = "S1";
	}
	if (s0_valid) {
		LOG_INF("nrf53_bl S0: version %u", s0_info->version);
	} else {
		LOG_INF("nrf53_bl S0: no valid firmware info");
	}
	if (s1_valid) {
		LOG_INF("nrf53_bl S1: version %u", s1_info->version);
	} else {
		LOG_INF("nrf53_bl S1: no valid firmware info");
	}
	LOG_INF("nrf53_bl: selected slot %s", active_slot);

	ret = boot_read_bank_header(PARTITION_ID(slot1_partition), &app_secondary_header,
				    sizeof(app_secondary_header));
	if (ret == 0 && app_secondary_header.mcuboot_version == 1) {
		const struct mcuboot_img_sem_ver *version = &app_secondary_header.h.v1.sem_ver;

		LOG_INF("nrf53_app secondary (slot1): MCUboot version %u.%u.%u+%u, image size %u",
			version->major, version->minor, version->revision, version->build_num,
			app_secondary_header.h.v1.image_size);
	} else if (ret == -EIO) {
		LOG_INF("nrf53_app secondary (slot1): no MCUboot header");
	} else {
		LOG_WRN("nrf53_app secondary (slot1): version unavailable (%d)",
			ret != 0 ? ret : -ENOTSUP);
	}

	ret = boot_read_bank_header(PARTITION_ID(slot3_partition), &net_secondary_header,
				    sizeof(net_secondary_header));
	if (ret == 0 && net_secondary_header.mcuboot_version == 1) {
		const struct mcuboot_img_sem_ver *version = &net_secondary_header.h.v1.sem_ver;

		LOG_INF("nrf53_net secondary (slot3): MCUboot version %u.%u.%u+%u, image size %u",
			version->major, version->minor, version->revision, version->build_num,
			net_secondary_header.h.v1.image_size);
	} else if (ret == -EIO) {
		LOG_INF("nrf53_net secondary (slot3): no MCUboot header");
	} else {
		LOG_WRN("nrf53_net secondary (slot3): version unavailable (%d)",
			ret != 0 ? ret : -ENOTSUP);
	}

	if (bluetooth_ready) {
		ret = bt_hci_cmd_send_sync(BT_HCI_OP_READ_LOCAL_VERSION_INFO, NULL, &response);
		if (ret == 0 && response != NULL &&
		    response->len >= sizeof(struct bt_hci_rp_read_local_version_info)) {
			const struct bt_hci_rp_read_local_version_info *version =
				(const struct bt_hci_rp_read_local_version_info *)response->data;

			if (version->status == 0) {
				LOG_INF("nrf53_net BLE controller: HCI %u rev 0x%04x, LMP %u, "
					"manufacturer 0x%04x, subversion 0x%04x",
					version->hci_version, sys_le16_to_cpu(version->hci_revision),
					version->lmp_version,
					sys_le16_to_cpu(version->manufacturer),
					sys_le16_to_cpu(version->lmp_subversion));
			} else {
				LOG_WRN("nrf53_net BLE controller version query status 0x%02x",
					version->status);
			}
		} else {
			LOG_WRN("nrf53_net BLE controller version unavailable (%d)", ret);
		}
		if (response != NULL) {
			net_buf_unref(response);
		}
	} else {
		LOG_WRN("nrf53_net BLE controller version unavailable (Bluetooth not initialized)");
	}

	ret = cellular_get_modem_info(modem, CELLULAR_MODEM_INFO_FW_VERSION,
				      mfw_version, sizeof(mfw_version) - 1);
	if (ret == 0) {
		mfw_version[sizeof(mfw_version) - 1] = '\0';
		LOG_INF("nrf91_mfw: %s", mfw_version);
	} else {
		LOG_WRN("nrf91_mfw: version unavailable (%d)", ret);
	}

	if (was_up) {
		ret = net_if_down(ppp_iface);
		if (ret < 0) {
			LOG_ERR("Cannot stop PPP to query SLM version: %d", ret);
			return;
		}
	}
	ret = pm_device_action_run(modem, PM_DEVICE_ACTION_SUSPEND);
	if (ret < 0 && ret != -EALREADY) {
		LOG_ERR("Cannot suspend modem to query SLM version: %d", ret);
		goto restore_interface;
	}
	ret = sm_xdfu_get_slm_version(modem, slm_version, sizeof(slm_version));
	if (ret == 0) {
		LOG_INF("nrf91_app (SLM): %s", slm_version);
	} else {
		LOG_WRN("nrf91_app (SLM): version unavailable (%d)", ret);
	}

	ret = pm_device_action_run(modem, PM_DEVICE_ACTION_RESUME);
	if (ret < 0) {
		LOG_ERR("Cannot resume modem after version query: %d", ret);
		return;
	}

restore_interface:
	if (was_up) {
		ret = net_if_up(ppp_iface);
		if (ret < 0) {
			LOG_ERR("Cannot restore PPP after version query: %d", ret);
		}
	}
}

static void ota_thread_fn(void *unused1, void *unused2, void *unused3)
{
	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);
	ARG_UNUSED(unused3);

	while (true) {
		k_sem_take(&ota_request, K_FOREVER);
		switch (atomic_get(&ota_operation)) {
		case OTA_DOWNLOAD:
			ota_download_run();
			break;
		case OTA_RESUME:
			ota_resume_run();
			break;
		case OTA_STATUS:
			ota_bundle_status();
			break;
		case OTA_APPLY: {
			int ret = ota_bundle_apply();

			if (ret < 0) {
				LOG_ERR("OTA apply failed (%d)", ret);
			} else {
				LOG_INF("OTA update transfer complete");
				ret = ota_bundle_delete();
				if (ret < 0) {
					LOG_WRN("Failed to delete OTA bundle (%d)", ret);
				}
			}
			modem_hard_reset();
			break;
		}
		case OTA_VERSIONS:
			ota_versions_run();
			break;
		default:
			LOG_ERR("Invalid OTA worker operation");
			break;
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
	if ((operation == OTA_DOWNLOAD || operation == OTA_RESUME) &&
	    k_event_test(&l4_event, L4_CONNECTED) == 0) {
		shell_error(sh, "Network is not ready");
		return -EAGAIN;
	}
	if (!atomic_cas(&ota_operation, OTA_IDLE, operation)) {
		shell_error(sh, "An OTA operation is already running");
		return -EBUSY;
	}
	const char *name = operation == OTA_DOWNLOAD ? "download" :
			   operation == OTA_RESUME ? "resume" :
			   operation == OTA_STATUS ? "status" :
			   operation == OTA_APPLY ? "apply" : "versions";
	shell_print(sh, "OTA %s queued", name);
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

static int cmd_ota_resume(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	return ota_submit(sh, OTA_RESUME);
}

static int cmd_ota_status(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	return ota_submit(sh, OTA_STATUS);
}

static int cmd_ota_versions(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	return ota_submit(sh, OTA_VERSIONS);
}

SHELL_STATIC_SUBCMD_SET_CREATE(ota_commands,
	SHELL_CMD_ARG(download, NULL, "Download the available Memfault OTA bundle",
		      cmd_ota_download, 1, 0),
	SHELL_CMD_ARG(resume, NULL, "Resume an interrupted OTA bundle download",
		      cmd_ota_resume, 1, 0),
	SHELL_CMD_ARG(status, NULL, "Show saved OTA bundle download state",
		      cmd_ota_status, 1, 0),
	SHELL_CMD_ARG(apply, NULL, "Verify and apply the saved OTA bundle", cmd_ota_apply, 1, 0),
	SHELL_CMD_ARG(versions, NULL, "Print versions of installed images", cmd_ota_versions, 1, 0),
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
		modem_hard_reset();
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

	ret = bt_enable(NULL);
	if (ret < 0) {
		LOG_ERR("Bluetooth initialization failed (%d)", ret);
	} else {
		bluetooth_ready = true;
		LOG_INF("Bluetooth host initialized over HCI IPC");
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
