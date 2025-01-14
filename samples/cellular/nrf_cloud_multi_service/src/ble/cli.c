/* Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#undef __XSI_VISIBLE
#define __XSI_VISIBLE 1
#undef _XOPEN_SOURCE
#define _XOPEN_SOURCE 1
#include <time.h>
#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/sys/printk.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>
#include <zephyr/shell/shell_history.h>
#include <zephyr/sys/reboot.h>
#include <version.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <fw_info.h>
#include <zephyr/posix/time.h>
#include <modem/modem_info.h>
#include <nrf_modem_at.h>
#include <modem/lte_lc.h>
#include <zephyr/net_buf.h>
#if defined(CONFIG_NRF_CLOUD_FOTA)
#include <net/fota_download.h>
#endif
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_vs.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sw_isr_table.h>
#include <modem/lte_lc.h>
#include "ncs_version.h"
#include "nrf_cloud_transport.h"
#include "ble.h"
#include "ble_codec.h"
#include "ble_conn_mgr.h"
#include "peripheral_dfu.h"
#include "gateway.h"
#include "flash_test.h"

LOG_MODULE_REGISTER(cli, CONFIG_NRFCLOUD_BLE_GATEWAY_LOG_LEVEL);

#define DEFAULT_PASSWORD CONFIG_SHELL_DEFAULT_PASSWORD

/* disable for now -- breaks BLE HCI after used */
#define PRINT_CTLR_INFO_ENABLED 0

/* set to 1 to enable showing cert list */
#define DISPLAY_CERT_LIST 0

#ifndef CONFIG_LOG_DOMAIN_ID
#define CONFIG_LOG_DOMAIN_ID 0
#endif

enum ble_cmd_type {
	BLE_CMD_ALL,
	BLE_CMD_MAC,
	BLE_CMD_NAME
};

struct modem_param_info modem_param;

/* was: CONFIG_AT_CMD_RESPONSE_MAX_LEN]; */
static char response_buf[2048];

static void set_at_prompt(const struct shell *shell, bool at_mode);
void peripheral_dfu_set_test_mode(bool test);

char *rem_eol(const char *s, char *d, int len)
{
	if (!s) {
		return "";
	}

	char *p = d;

	strncpy(d, s, len);

	while (*p) {
		if ((*p == '\r') || (*p == '\n')) {
			*p = '\0';
			break;
		}
		p++;
	}
	return d;
}

void cmd_testflash(const struct shell *shell)
{
	int ret;

	if (IS_ENABLED(CONFIG_FLASH_TEST)) {
		shell_print(shell, "Testing flash...");
		ret = flash_test();
	} else {
		shell_print(shell, "Test not enabled");
		ret = -ENOTSUP;
	}

	if (ret) {
		shell_error(shell, "Error testing flash: %d", ret);
	} else {
		shell_print(shell, "Test passed.");
	}
}

void print_fw_info(const struct shell *shell, bool verbose)
{
	extern struct fw_info m_firmware_info;
	struct fw_info *info = &m_firmware_info;

	if (info) {
		shell_print(shell, "Board name:         \t%s", CONFIG_BOARD);
		shell_print(shell, "App name:           \t%s", STRINGIFY(PROJECT_NAME));
		shell_print(shell, "App rev:            \t%s", CONFIG_APP_VERSION);
		shell_print(shell, "nRF Connect SDK rev:\t%s", STRINGIFY(NCS_BUILD_VERSION));
		shell_print(shell, "Zephyr rev:         \t%s", STRINGIFY(BUILD_VERSION));
		shell_print(shell, "Built date and time:\t%s %s", __DATE__, __TIME__);
		if (!verbose) {
			return;
		}
		if (!fw_info_check((uint32_t)info)) {
			shell_error(shell, "fw_info struct is invalid");
			return;
		}
		shell_print(shell, "Ver:                \t%u", info->version);
		shell_print(shell, "Size:               \t%u", info->size);
		shell_print(shell, "Start:              \t0x%08x", info->address);
		shell_print(shell, "Boot:               \t0x%08x", info->boot_address);
	}
}

#if defined(CONFIG_SYS_HEAP_RUNTIME_STATS)
extern struct sys_heap _system_heap;
#endif

int heap_shell(const struct shell *shell, size_t argc, char **argv)
{
#if defined(CONFIG_SYS_HEAP_RUNTIME_STATS)
	int err;
	struct sys_memory_stats kernel_stats;

	err = sys_heap_runtime_stats_get(&_system_heap, &kernel_stats);
	if (err) {
		shell_error(shell, "heap: failed to read kernel heap statistics, error: %d", err);
	} else {
		shell_print(shell, "kernel heap statistics:");
		shell_print(shell, "free:           %6d", kernel_stats.free_bytes);
		shell_print(shell, "allocated:      %6d", kernel_stats.allocated_bytes);
		shell_print(shell, "max. allocated: %6d\n", kernel_stats.max_allocated_bytes);
	}
#endif /* CONFIG_SYS_HEAP_RUNTIME_STATS */

/* Calculate the system heap maximum size. */
#define USED_RAM_END_ADDR POINTER_TO_UINT(&_end)
#define HEAP_BASE USED_RAM_END_ADDR
#define MAX_HEAP_SIZE (KB(CONFIG_SRAM_SIZE) - (HEAP_BASE - CONFIG_SRAM_BASE_ADDRESS))

	shell_print(shell, "system heap statistics:");
	shell_print(shell, "max. size:      %6ld", MAX_HEAP_SIZE);

	return 0;
}

void print_modem_info(const struct shell *shell, bool creds)
{
#ifdef CONFIG_MODEM_INFO
	modem_info_init();
	modem_info_params_init(&modem_param);

	int ret = modem_info_params_get(&modem_param);

	if (ret) {
		shell_error(shell, "Error getting modem info: %d", ret);
	} else {
		struct device_param *dev = &modem_param.device;
		struct network_param *net = &modem_param.network;
		struct sim_param *sim = &modem_param.sim;
		char buf[128];

		if (dev->modem_fw.type == MODEM_INFO_FW_VERSION) {
			shell_print(shell, "Modem fw: \t%s",
			  rem_eol(dev->modem_fw.value_string, buf, sizeof(buf)));
		} else {
			shell_print(shell, "Modem fw type: \t%u val %u",
				dev->modem_fw.type,
				dev->modem_fw.value);
		}
		if (dev->battery.type == MODEM_INFO_BATTERY) {
			shell_print(shell, "Battery: \t%u mV",
				    dev->battery.value);
		}
		if (dev->imei.type == MODEM_INFO_IMEI) {
			shell_print(shell, "IMEI: \t\t%s",
			      rem_eol(dev->imei.value_string, buf, sizeof(buf)));
		}
		shell_print(shell, "Board: \t\t%s",
			    rem_eol(dev->board, buf, sizeof(buf)));
		shell_print(shell, "App Name: \t%s",
			    rem_eol(dev->app_name, buf, sizeof(buf)));
		shell_print(shell, "App Ver: \t%s",
			    rem_eol(dev->app_version, buf, sizeof(buf)));

		if (sim->uicc.type == MODEM_INFO_UICC) {
			shell_print(shell, "UICC: \t\t%u",
				    sim->uicc.value);
		}
		if (sim->iccid.type == MODEM_INFO_ICCID) {
			shell_print(shell, "ICCID: \t\t%s",
				    rem_eol(sim->iccid.value_string, buf, sizeof(buf)));
		}
		if (sim->imsi.type == MODEM_INFO_IMSI) {
			shell_print(shell, "IMSI: \t\t%s",
				    rem_eol(sim->imsi.value_string, buf, sizeof(buf)));
		}

		if (net->current_band.type ==
		    MODEM_INFO_CUR_BAND) {
			shell_print(shell, "Cur band: \t%u",
				    net->current_band.value);
		}
		if (net->sup_band.type ==
		    MODEM_INFO_SUP_BAND) {
			shell_print(shell, "Sup band: \t%s",
				    rem_eol(net->sup_band.value_string, buf, sizeof(buf)));
		}
		if (net->area_code.type ==
		    MODEM_INFO_AREA_CODE) {
			shell_print(shell, "Area code: \t%s",
				 rem_eol(net->area_code.value_string, buf, sizeof(buf)));
		}
		if (net->current_operator.type ==
		    MODEM_INFO_OPERATOR) {
			shell_print(shell, "Operator: \t%s",
			     rem_eol(net->current_operator.value_string, buf, sizeof(buf)));
		}
		if (net->mcc.type ==
		    MODEM_INFO_MCC) {
			shell_print(shell, "MCC: \t\t%u",
				    net->mcc.value);
		}
		if (net->mnc.type ==
		    MODEM_INFO_MNC) {
			shell_print(shell, "MNC: \t\t%u",
				    net->mnc.value);
		}
		if (net->ip_address.type ==
		    MODEM_INFO_IP_ADDRESS) {
			shell_print(shell, "IP address: \t%s",
				   rem_eol(net->ip_address.value_string, buf, sizeof(buf)));
		}
		if (net->ue_mode.type ==
		    MODEM_INFO_UE_MODE) {
			shell_print(shell, "UE mode: \t%u",
				    net->ue_mode.value);
		}
		if (net->apn.type ==
		    MODEM_INFO_APN) {
			shell_print(shell, "Access point: \t%s",
				    rem_eol(net->apn.value_string, buf, sizeof(buf)));
		}
		if (net->lte_mode.value == 1) {
			shell_print(shell, "Mode: \t\tLTE-M");
		} else if (net->nbiot_mode.value == 1) {
			shell_print(shell, "Mode: \t\tNB-IoT");
		}
		shell_print(shell, "Cell ID: \t%ld",
			    (long)net->cellid_dec);

#if defined(CONFIG_MODEM_INFO_ADD_DATE_TIME) || defined(CONFIG_DATE_TIME_MODEM)
		char *str = net->date_time.value_string;
		struct timespec now;
		struct tm *tm;
		char *atime;

		shell_print(shell, "Net date/time: \t%s "
			"DST %d TZ %ld",
			str, _daylight, _timezone);
		clock_gettime(CLOCK_REALTIME, &now);
		tm = localtime(&now.tv_sec);
		if (tm == NULL) {
			shell_error(shell, "could not get local time");
			return;
		}
		atime = asctime(tm);
		if (atime == NULL) {
			shell_error(shell, "could not get asctime");
			return;
		}
		shell_print(shell, "Time now: \t%s", asctime(tm));
#else
		char time_str[30] = {0};

		if (get_time_str(time_str, sizeof(time_str))) {
			shell_print(shell, "Current UTC: \t%s", time_str);
		}
#endif
	}
#endif

	if (creds) {
		response_buf[0] = '\0';
		int err;

		err = nrf_modem_at_cmd(response_buf, sizeof(response_buf), "%s", "AT%CMNG=1");
		if (err < 0) {
			shell_error(shell, "ERROR: %d", err);
		} else if (err > 0) {
			shell_error(shell, "ERROR: 0x%X", nrf_modem_at_err(err));
		}

		shell_print(shell, "%s", response_buf);
	}
}

void print_connection_status(const struct shell *shell)
{
	int err;
	enum lte_lc_nw_reg_status status;
	enum lte_lc_system_mode smode;
	enum lte_lc_system_mode_preference smode_pref;
	enum lte_lc_func_mode fmode;

	err = lte_lc_nw_reg_status_get(&status);
	if (err) {
		shell_error(shell, "lte_lc_nw_reg_status_get: %d", err);
	} else {
		shell_print(shell, "LTE reg status: \t%d", (int)status);
	}
	err = lte_lc_system_mode_get(&smode, &smode_pref);
	if (err) {
		shell_error(shell, "lte_lc_system_mode_get: %d", err);
	} else {
		shell_print(shell, "LTE system mode: \t%d", (int)smode);
		shell_print(shell, "system mode pref: \t%d", (int)smode_pref);
	}
	err = lte_lc_func_mode_get(&fmode);
	if (err) {
		shell_error(shell, "lte_lc_func_mode_get: %d", err);
	} else {
		shell_print(shell, "LTE functional mode: \t%d", (int)fmode);
	}
}

static void print_cloud_info(const struct shell *shell)
{
	char stage[NRF_CLOUD_STAGE_ID_MAX_LEN] = "unknown";
	char tenant_id[NRF_CLOUD_TENANT_ID_MAX_LEN] = "unknown";
	static char id[NRF_CLOUD_CLIENT_ID_MAX_LEN] = "unknown";

	nct_stage_get(stage, sizeof(stage));
	nrf_cloud_tenant_id_get(tenant_id, sizeof(tenant_id));
	nrf_cloud_client_id_get(id, sizeof(id));
	print_connection_status(shell);

	shell_print(shell, "nrfcloud stage: \t%s", stage);
	shell_print(shell, "nrfcloud endpoint: \t%s", CONFIG_NRF_CLOUD_HOST_NAME);
	shell_print(shell, "nrfcloud tenant id: \t%s", tenant_id);
	shell_print(shell, "nrfcloud dev id: \t%s", id);
	shell_print(shell, "security sectag: \t%d", CONFIG_NRF_CLOUD_SEC_TAG);
}

static void print_ctlr_info(const struct shell *shell)
{
#if PRINT_CTLR_INFO_ENABLED
	struct net_buf *rsp;
	int ret;

	ret = bt_hci_cmd_send_sync(BT_HCI_OP_VS_READ_VERSION_INFO, NULL, &rsp);
	if (ret) {
		shell_error(shell, "Could not read version info: %d", ret);
	} else {
		struct bt_hci_rp_vs_read_version_info *info;

		info = (void *)rsp->data;
		shell_print(shell, "HW platform: \t0x%04x", info->hw_platform);
		shell_print(shell, "HW variant: \t0x%04x", info->hw_variant);
		shell_print(shell, "FW variant: \t0x%02x", info->fw_variant);
		shell_print(shell, "FW version: \t%u.%u",
			    info->fw_version,
			    sys_le16_to_cpu(info->fw_revision));
		shell_print(shell, "Build:    \t0x%08X", info->fw_build);
	}

	if (!IS_ENABLED(CONFIG_BT_LL_SOFTDEVICE)) {
		ret = bt_hci_cmd_send_sync(BT_HCI_OP_VS_READ_BUILD_INFO,
					   NULL, &rsp);
		if (ret) {
			LOG_ERR("Could not read build info: %d", ret);
		} else {
			struct bt_hci_rp_vs_read_build_info *build;

			build = (void *)rsp->data;
			shell_print(shell, "OS build: \t%s",
				    (const char *)build->info);
		}
	}

	if (IS_ENABLED(CONFIG_BT_LL_SOFTDEVICE)) {
		ret = bt_hci_cmd_send_sync(BT_HCI_OP_VS_READ_STATIC_ADDRS,
					   NULL, &rsp);
		if (ret || (rsp == NULL)) {
			LOG_ERR("Could not read static address: %d", ret);
		} else {
			struct bt_hci_rp_vs_read_static_addrs *addrs;
			uint8_t *a;

			addrs = (void *)rsp->data;
			a = &addrs->a[0].bdaddr.val[0];
			shell_print(shell, "Static addr: \t"
				    "%02X:%02X:%02X:%02X:%02X:%02X",
				    a[0], a[1], a[2], a[3], a[4], a[5]);
		}
	}
#else
	shell_print(shell, "unavailable");
#endif
}

static void print_scan_info(const struct shell *shell)
{
	unsigned int i;
	struct ble_scanned_dev *dev;

	shell_print(shell, "   MAC, type, RSSI, name");
	for (i = 0; i < MAX_SCAN_RESULTS; i++) {
		dev = get_scanned_device(i);
		if (dev == NULL) {
			shell_print(shell, "<end of list>");
			break;
		}
		shell_print(shell, "%u. %s, %d, %s",
			    i + 1, dev->ble_mac_addr_str, dev->rssi, dev->name);
	}
}

static void print_conn_info(const struct shell *shell, bool show_path,
			    bool notify)
{
	unsigned int i;
	unsigned int j;
	unsigned int k;
	int count = 0;
	struct ble_device_conn *dev;
	struct uuid_handle_pair *up;
	char uuid_str[BT_UUID_STR_LEN];
	char path[BT_MAX_PATH_LEN];
	char props[64];
	static const char * const types[] = {"svc", "chr", "---", "ccc"};
	static const char * const properties[] = {"brdcst", "read", "wrnorsp", "write",
						  "notif", "indi", "auth", "extprop"};

	if (!notify) {
		shell_print(shell, "   MAC, connected, discovered, shadow"
				   " updated, denylist status, ctrld by,"
				   " visible, num UUIDs");
	}
	for (i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		dev = get_connected_device(i);
		if (dev == NULL) {
			continue;
		}
		count++;
		shell_print(shell, "%u. %s, %s, %s, %s, %s, %s, %s, UUIDs:%u",
			    count,
			    dev->ble_mac_addr_str,
			    dev->connected ? "CONNNECTED" : "disconnected",
			    dev->discovered ? "DISCOVERED" :
			    (dev->discovering ? "DISCOVERING" : "not dscvred"),
			    dev->shadow_updated ? "SHADOW UPDATED" :
			    "shadow not set",
			    dev->added_to_allowlist ? "CONN ALLOWED" :
			    "conn denied",
			    ble_conn_mgr_enabled(dev->ble_mac_addr_str) ? "CLOUD" : "local",
			    !dev->hidden ? "VISIBLE" : "hidden",
			    (unsigned int)dev->num_pairs
			   );
		if (!notify) {
			shell_print(shell, "   is service, UUID, UUID type, "
					   "handle, type, path depth, "
					   "properties, sub index, sub "
					   "enabled, val_len");
		}
		for (j = 0; j < dev->num_pairs; j++) {
			up = dev->uuid_handle_pairs[j];
			if (up == NULL) {
				continue;
			}
			if (notify && !up->sub_enabled) {
				continue;
			}
			get_uuid_str(up, uuid_str, BT_UUID_STR_LEN);
			props[0] = '\0';
			for (k = 0; k < 8; k++) {
				if (up->properties & (1 << k)) {
					if (props[0]) {
						strncat(props, ", ", sizeof(props) - 1);
					}
					strncat(props, properties[k], sizeof(props) - 1);
				}
			}
			shell_print(shell,
				    "   %u, %s, %s, %u, %u, %s, %u, 0x%02X (%s), %u, %s, %u",
				    j + 1,
				    up->is_service ? "serv" : "char",
				    uuid_str,
				    up->uuid_type,
				    up->handle,
				    up->attr_type <= BT_ATTR_CCC ? types[up->attr_type] : "unk",
				    (unsigned int)up->path_depth,
				    (unsigned int)up->properties,
				    props,
				    (unsigned int)up->sub_index,
				    up->sub_enabled ? "NOTIFY ON" : "notify off",
				    up->value_len
			);
			if (up->value_len) {
				shell_hexdump(shell, up->value, up->value_len);
			}
			if (show_path) {
				ble_conn_mgr_generate_path(dev, up->handle, path,
						  up->attr_type == BT_ATTR_CCC);
				shell_print(shell, "       %u, %s",
					    up->handle, path);
			}
		}
	}
}

static void print_irq_info(const struct shell *shell)
{
	int i;
	extern char _vector_start[];
	void **vectors = (void *)_vector_start;

	shell_print(shell, "IRQn, entry, prio, en, pend, active, arg");
	for (i = -15; i < IRQ_TABLE_SIZE; i++) {
		void *entry;
		const void *arg;

		if (i < 0) {
			entry = vectors[i + 16];
			arg = 0;
		} else {
			entry = _sw_isr_table[i].isr;
			arg = _sw_isr_table[i].arg;
		}
		if (entry == z_irq_spurious) {
			continue;
		}
		shell_print(shell, "% 3d. %10p, %d, %d, %d, %d, %10p",
			    i, entry,
			    NVIC_GetPriority(i), NVIC_GetEnableIRQ(i),
			    NVIC_GetPendingIRQ(i), NVIC_GetActive(i),
			    arg);
	}
}

static int ble_conn_save(const struct shell *shell)
{
	int num;
	struct desired_conn *desired = get_desired_array(&num);

	return set_shadow_desired_conn(desired, num);
}

static int ble_conn_mac(const struct shell *shell, char *ble_mac_addr_str)
{
	int err;

	shell_print(shell, "  Adding connection to %s...", ble_mac_addr_str);
	err = ble_conn_mgr_add_conn(ble_mac_addr_str);
	if (!err) {
		shell_print(shell, "  Adding to desired list...");
		err = ble_conn_mgr_add_desired(ble_mac_addr_str, true);
	}
	if (err) {
		shell_error(shell, "  Failed: error %d", err);
	} else {
		shell_print(shell, "  Done.");
	}

	return err;
}

static int ble_conn_all(const struct shell *shell)
{
	unsigned int i;
	struct ble_scanned_dev *dev;
	int err = 0;
	int count = 0;

	for (i = 0; i < MAX_SCAN_RESULTS; i++) {
		dev = get_scanned_device(i);
		if (dev == NULL) {
			break;
		}
		err = ble_conn_mac(shell, dev->ble_mac_addr_str);
		if (!err) {
			count++;
		}
	}

	shell_print(shell, "Connected to %d devices", count);
	return err;
}

static int ble_conn_name(const struct shell *shell, char *name)
{
	unsigned int i;
	struct ble_scanned_dev *dev;

	for (i = 0; i < MAX_SCAN_RESULTS; i++) {
		dev = get_scanned_device(i);
		if (dev == NULL) {
			break;
		}
		if (strcmp(name, dev->name) == 0) {
			shell_print(shell, "Connecting to %s", name);
			return ble_conn_mac(shell, dev->ble_mac_addr_str);
		}
	}

	shell_error(shell, "BLE device %s not found", name);
	return -EINVAL;
}

static int ble_disconn_mac(const struct shell *shell, char *ble_mac_addr_str)
{
	int err;

	shell_print(shell, "  Disconnecting device...");
	if (!ble_conn_mgr_is_addr_connected(ble_mac_addr_str)) {
		err = ble_add_to_allowlist(ble_mac_addr_str, false);
		if (err) {
			shell_error(shell, "  Failed to remove from allowlist: %d", err);
		}
	}
	err = disconnect_device_by_addr(ble_mac_addr_str);
	if (err) {
		shell_error(shell, "  Error disconnecting device: %d", err);
	}
	shell_print(shell, "  Removing connection to %s...", ble_mac_addr_str);
	err = ble_conn_mgr_remove_conn(ble_mac_addr_str);
	if (!err) {
		shell_print(shell, "  Removing from desired list...");
		err = ble_conn_mgr_rem_desired(ble_mac_addr_str, true);
		if (err) {
			shell_error(shell, "  Failed to remove from desired list: %d", err);
		}
	}
	if (err) {
		shell_error(shell, "  Failed: error %d", err);
	} else {
		shell_print(shell, "  Done.");
	}

	return err;
}

static int ble_disconn_all(const struct shell *shell)
{
	unsigned int i;
	struct ble_device_conn *con;
	int err = 0;
	int count = 0;

	for (i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		con = get_connected_device(i);
		if (con == NULL) {
			continue;
		}
		err = ble_disconn_mac(shell, con->ble_mac_addr_str);
		if (!err) {
			count++;
		}
	}

	shell_print(shell, "Disconnected %d devices", count);
	return 0;
}

static int ble_disconn_name(const struct shell *shell, char *name)
{
	unsigned int i;
	struct ble_scanned_dev *dev;

	for (i = 0; i < MAX_SCAN_RESULTS; i++) {
		dev = get_scanned_device(i);
		if (dev == NULL) {
			break;
		}
		if (strcmp(name, dev->name) == 0) {
			shell_print(shell, "Disconnecting %s", name);
			return ble_disconn_mac(shell, dev->ble_mac_addr_str);
		}
	}

	shell_print(shell, "BLE device %s not found", name);
	return -EINVAL;
}

static enum ble_cmd_type get_cmd_type(char *arg)
{
	enum ble_cmd_type ret;

	if (strcmp(arg, "all") == 0) {
		ret = BLE_CMD_ALL;
	} else if (strchr(arg, ':')) {
		ret = BLE_CMD_MAC;
	} else {
		ret = BLE_CMD_NAME;
	}
	return ret;
}

/* This dynamic command helper is called repeatedly by the shell when
 * the user wants command completion which lists possible MAC addresses.
 * It calls this until it returns NULL.
 *
 * The first for loop returns all the devices we are already supposed to
 * connect to, because they're in the shadow's desiredConnections array.
 * The second nested for loop effectively appends to that list any scan
 * results which are not also devices we should connect with (so those
 * devices are not listed twice).
 *
 * This would happen if the user used the CLI to scan for advertising
 * devices with 'ble scan', then used 'ble conn' to connect to one
 * (which temporarily adds this new device to the desiredConnections
 * array), and then used a dynamic command completion which hits this
 * function.
 *
 * Often, the scan results are empty, and there is just a short list
 * of desired connections, so the second loop does nothing.
 */
static const char *get_mac_addr(size_t idx, bool all)
{
	unsigned int i;
	unsigned int j;
	struct ble_scanned_dev *dev;
	struct ble_device_conn *con;
	int count = 0;

	/* combine our list of desired connections and scan results */
	for (i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		con = get_connected_device(i);
		if (con == NULL) {
			continue;
		}
		if (count == idx) {
			return con->ble_mac_addr_str;
		}
		count++;
	}

	if (!all) {
		return NULL;
	}

	for (i = 0; i < MAX_SCAN_RESULTS; i++) {
		dev = get_scanned_device(i);
		if (dev == NULL) {
			break;   /* end of list of scanned devices */
		}
		for (j = 0; j < CONFIG_BT_MAX_CONN; j++) {
			con = get_connected_device(i);
			if (con == NULL) {
				continue;
			}
			if (strcmp(con->ble_mac_addr_str, dev->ble_mac_addr_str) == 0) {
				dev = NULL;
				break;
			}
		}
		if (dev == NULL) {
			continue; /* scan matches connected device, so skip */
		}
		if (count == idx) {
			return dev->ble_mac_addr_str;
		}
		count++;
	}

	return NULL;
}

static const char *get_device_name(size_t idx)
{
	unsigned int i;
	struct ble_scanned_dev *dev;
	int count = 0;

	for (i = 0; i < MAX_SCAN_RESULTS; i++) {
		dev = get_scanned_device(i);
		if (dev == NULL) {
			break;   /* end of list of scanned devices */
		}
		if (strlen(dev->name) == 0) {
			continue;
		}
		if (count == idx) {
			return dev->name;
		}
		count++;
	}

	return NULL;
}

static const char *get_cmd_param(size_t idx)
{
	/* all is a valid parameter */
	if (idx == 0) {
		return "all";
	}
	/* return any known device names */
	if (idx <= get_num_scan_names()) {
		return get_device_name(idx - 1);
	}
	/* then any MAC addresses */
	return get_mac_addr(idx - 1 - get_num_scan_names(), true);
}

uint32_t get_log_module_level(const struct shell *shell, const char *name)
{
	uint32_t modules_cnt = log_src_cnt_get(CONFIG_LOG_DOMAIN_ID);
	const char *tmp_name;
	uint32_t i;
	uint32_t level = LOG_LEVEL_NONE;

	/* if current log level of ble module >= INF, then no print needed */
	for (i = 0U; i < modules_cnt; i++) {
		tmp_name = log_source_name_get(CONFIG_LOG_DOMAIN_ID, i);
		if (tmp_name == NULL) {
			continue;
		}
		if (strcmp(tmp_name, name) == 0) {
			level = log_filter_get(shell->log_backend->backend,
					       CONFIG_LOG_DOMAIN_ID, i, true);
			break;
		}
	}
	return level;
}

/* COMMAND HANDLERS */

static int cmd_info_list(const struct shell *shell, size_t argc, char **argv)
{
	/* Get MAC from argv */
	shell_print(shell, "MAC selected: %s", argv[0]);

	size_t idx = 0;
	const char *ble_mac_addr_str;

	for (idx = 0;; idx++) {
		ble_mac_addr_str = get_mac_addr(idx, true);
		if (ble_mac_addr_str) {
			shell_print(shell, "%zd. %s", idx, ble_mac_addr_str);
		} else {
			shell_print(shell, "end of list");
			break;
		}
	}
	return 0;
}

#define DYNAMIC_ADDR_HELP " Valid BLE MAC address"

static void get_dynamic_addr(size_t idx, struct shell_static_entry *entry)
{
	entry->syntax = get_mac_addr(idx, true);

	if (entry->syntax == NULL) {
		return;
	}

	entry->handler = cmd_info_list;
	entry->subcmd = NULL;
	entry->help = DYNAMIC_ADDR_HELP;
	entry->args.mandatory = 1;
	entry->args.optional = 6;
}

SHELL_DYNAMIC_CMD_CREATE(dynamic_addr, get_dynamic_addr);

static int cmd_param_list(const struct shell *shell, size_t argc, char **argv)
{
	/* Get MAC from argv */
	shell_print(shell, "param selected: %s", argv[0]);

	size_t idx = 0;
	const char *param;

	shell_print(shell, "num scan names:   %d", get_num_scan_names());
	shell_print(shell, "num scan results: %d", get_num_scan_results());
	shell_print(shell, "num connected:    %d", get_num_connected());

	for (idx = 0;; idx++) {
		param = get_cmd_param(idx);
		if (param) {
			shell_print(shell, "%zd. %s", idx, param);
		} else {
			shell_print(shell, "end of list");
			break;
		}
	}
	return 0;
}

#define DYNAMIC_PARAM_HELP " all | name | MAC"

static void get_dynamic_param(size_t idx, struct shell_static_entry *entry)
{
	entry->syntax = get_cmd_param(idx);

	if (entry->syntax == NULL) {
		return;
	}

	entry->handler = cmd_param_list;
	entry->subcmd = NULL;
	entry->help = DYNAMIC_PARAM_HELP;
	entry->args.mandatory = 1;
	entry->args.optional = 6;
}

SHELL_DYNAMIC_CMD_CREATE(dynamic_param, get_dynamic_param);

static int cmd_info_gateway(const struct shell *shell, size_t argc, char **argv)
{
	bool detailed = false;

	if (argc > 1) {
		if (strcmp(argv[1], "verbose") == 0) {
			detailed = true;
		}
	}

	print_fw_info(shell, true);
	return 0;
}

static int cmd_info_modem(const struct shell *shell, size_t argc, char **argv)
{
	bool creds = false;

	if (argc > 1) {
		if (strcmp(argv[1], "verbose") == 0) {
			creds = true;
		}
	}
	print_modem_info(shell, creds);
	return 0;
}

static int cmd_info_cloud(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	print_cloud_info(shell);
	return 0;
}

static int cmd_info_ctlr(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	print_ctlr_info(shell);
	return 0;
}

static int cmd_info_scan(const struct shell *shell, size_t argc,
			      char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	print_scan_info(shell);
	return 0;
}

static int cmd_info_conn(const struct shell *shell, size_t argc,
				char **argv)
{
	bool path = false;
	bool notify = false;
	int i;

	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "path") == 0) {
			path = true;
		} else if (strcmp(argv[i], "notify") == 0) {
			notify = true;
		} else {
			shell_error(shell, "unknown option: %s", argv[i]);
			return 0;
		}
	}
	print_conn_info(shell, path, notify);
	return 0;
}

static int cmd_info_irq(const struct shell *shell, size_t argc,
			char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	print_irq_info(shell);
	return 0;
}

#if CONFIG_GATEWAY_BLE_FOTA
static int cmd_ble_test(const struct shell *shell, size_t argc, char **argv)
{
	static bool test_mode = true;

	shell_print(shell, "Setting BLE FOTA test mode to %u", test_mode);
	peripheral_dfu_set_test_mode(test_mode);
	test_mode = !test_mode;
	return 0;
}
#endif

static int cmd_ble_scan(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* if current log level of ble module < INF, then print needed */
	bool print_scan = (get_log_module_level(shell, "ble") < LOG_LEVEL_INF);

	scan_start(print_scan);
	return 0;
}

static int cmd_ble_save(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(shell, "save connection list to cloud...");
	return ble_conn_save(shell);
}

static int cmd_ble_conn(const struct shell *shell, size_t argc, char **argv)
{
	char *arg = argv[0];

	if (argc < 1) {
		return -EINVAL;
	}

	switch (get_cmd_type(arg)) {
	case BLE_CMD_ALL:
		shell_print(shell, "connecting all BLE devices...");
		return ble_conn_all(shell);
	case BLE_CMD_MAC:
		shell_print(shell, "connecting to MAC %s", arg);
		return ble_conn_mac(shell, arg);
	case BLE_CMD_NAME:
		shell_print(shell, "connecting to name %s", arg);
		return ble_conn_name(shell, arg);
	default:
		return -EINVAL;
	}
}

static void get_dynamic_ble_conn(size_t idx, struct shell_static_entry *entry)
{
	entry->syntax = get_cmd_param(idx);

	if (entry->syntax == NULL) {
		return;
	}

	entry->handler = cmd_ble_conn;
	entry->subcmd = NULL;
	entry->help = DYNAMIC_PARAM_HELP;
	entry->args.mandatory = 1;
	entry->args.optional = 0;
}

SHELL_DYNAMIC_CMD_CREATE(dynamic_ble_conn, get_dynamic_ble_conn);

static int cmd_ble_disc(const struct shell *shell, size_t argc, char **argv)
{
	char *arg = argv[0];

	if (argc < 1) {
		return -EINVAL;
	}

	switch (get_cmd_type(arg)) {
	case BLE_CMD_ALL:
		shell_print(shell, "disconnecting all BLE devices...");
		return ble_disconn_all(shell);
	case BLE_CMD_MAC:
		shell_print(shell, "disconnecting MAC %s", arg);
		return ble_disconn_mac(shell, arg);
	case BLE_CMD_NAME:
		shell_print(shell, "disconnecting name %s", arg);
		return ble_disconn_name(shell, arg);
	default:
		return -EINVAL;
	}
}

static void get_dynamic_ble_disc(size_t idx, struct shell_static_entry *entry)
{
	entry->syntax = get_cmd_param(idx);

	if (entry->syntax == NULL) {
		return;
	}

	entry->handler = cmd_ble_disc;
	entry->subcmd = NULL;
	entry->help = DYNAMIC_PARAM_HELP;
	entry->args.mandatory = 1;
	entry->args.optional = 0;
}

SHELL_DYNAMIC_CMD_CREATE(dynamic_ble_disc, get_dynamic_ble_disc);

static int cmd_ble_notif_en(const struct shell *shell, size_t argc, char **argv)
{
	char *arg = argv[0];

	if (argc < 1) {
		return -EINVAL;
	}

	switch (get_cmd_type(arg)) {
	case BLE_CMD_ALL:
		shell_print(shell, "enable notifications on all devices");
		struct ble_device_conn *dev;
		int i;
		int count = 0;

		for (i = 0; i < CONFIG_BT_MAX_CONN; i++) {
			dev = get_connected_device(i);
			if (dev == NULL) {
				continue;
			}
			shell_print(shell, "enabling on MAC %s...", dev->ble_mac_addr_str);
			ble_subscribe_all(dev->ble_mac_addr_str, BT_GATT_CCC_NOTIFY);
			count++;
		}
		if (!count) {
			shell_warn(shell, "Connect a device first.");
		}
		break;
	case BLE_CMD_MAC:
		if ((argc < 2) || (strcmp(argv[1], "all") == 0)) {
			shell_print(shell, "enable all notifications on MAC %s",
				    arg);
			ble_subscribe_all(arg, BT_GATT_CCC_NOTIFY);
			return 0;
		}
		uint16_t handle = atoi(argv[1]);

		shell_print(shell, "enable notification on MAC %s handle %u",
			    arg, handle);
		ble_subscribe_handle(arg, handle, BT_GATT_CCC_NOTIFY);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static void get_dynamic_ble_en(size_t idx, struct shell_static_entry *entry)
{
	entry->syntax = get_cmd_param(idx);

	if (entry->syntax == NULL) {
		return;
	}

	entry->handler = cmd_ble_notif_en;
	entry->subcmd = NULL;
	entry->help = DYNAMIC_PARAM_HELP;
	entry->args.mandatory = 1;
	entry->args.optional = 2;
}

SHELL_DYNAMIC_CMD_CREATE(dynamic_ble_en, get_dynamic_ble_en);

static int cmd_ble_notif_dis(const struct shell *shell, size_t argc, char **argv)
{
	char *arg = argv[0];

	if (argc < 1) {
		return -EINVAL;
	}

	switch (get_cmd_type(arg)) {
	case BLE_CMD_ALL:
		shell_print(shell, "disable notifications on all devices");
		struct ble_device_conn *dev;
		int i;

		for (i = 0; i < CONFIG_BT_MAX_CONN; i++) {
			dev = get_connected_device(i);
			if (dev == NULL) {
				continue;
			}
			shell_print(shell, "disabling on MAC %s...",
				    dev->ble_mac_addr_str);
			ble_subscribe_all(dev->ble_mac_addr_str, 0);
		}
		break;
	case BLE_CMD_MAC:
		if ((argc < 2) || (strcmp(argv[1], "all") == 0)) {
			shell_print(shell, "disable all notifications on MAC %s",
				    arg);
			ble_subscribe_all(arg, 0);
			return 0;
		}
		uint16_t handle = atoi(argv[1]);

		shell_print(shell, "disable notification on MAC %s handle %u",
			    arg, handle);
		ble_subscribe_handle(arg, handle, 0);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static void get_dynamic_ble_dis(size_t idx, struct shell_static_entry *entry)
{
	entry->syntax = get_cmd_param(idx);

	if (entry->syntax == NULL) {
		return;
	}

	entry->handler = cmd_ble_notif_dis;
	entry->subcmd = NULL;
	entry->help = DYNAMIC_PARAM_HELP;
	entry->args.mandatory = 1;
	entry->args.optional = 1;
}

SHELL_DYNAMIC_CMD_CREATE(dynamic_ble_dis, get_dynamic_ble_dis);

static void set_at_prompt(const struct shell *shell, bool at_mode)
{
	static bool normal_prompt = true;

	if (at_mode && normal_prompt) {
		normal_prompt = false;

		shell_print(shell, "Type 'exit' to exit AT mode");
		shell_prompt_change(shell, "");

		shell_use_colors_set(shell, false);

	} else if (!at_mode && !normal_prompt) {
		normal_prompt = true;

		shell_prompt_change(shell, CONFIG_SHELL_PROMPT_SECURE);

		/* hack: manually turn off select mode as is done in
		 * zephyr/subsys/shell/shell.c:alt_metakeys_handle()
		 * when it receives SHELL_VT100_ASCII_ALT_R -- there is
		 * no API for doing so
		 */
		shell_set_root_cmd(NULL);

		shell_use_colors_set(shell, true);
	}
}

static int app_cmd_at(const struct shell *shell, size_t argc, char **argv)
{
	if (argv[1] == NULL) {
		shell_help(shell);
		return -EINVAL;
	}
	if (strcmp(argv[1], "enable") == 0) {
		shell_set_root_cmd("at");
		set_at_prompt(shell, true);
		return 0;
	} else if (strcmp(argv[1], "exit") == 0) {
		set_at_prompt(shell, false);
		return 0;
	}
	int err;

	if (strcmp(argv[1], "AT+CFUN=4") == 0) {
		control_cloud_connection(false); /* disable reconnections */
	} else if (strcmp(argv[1], "AT+CFUN=1") == 0) {
		control_cloud_connection(true); /* enable reconnections */
	}

	char *c = argv[1];
	int i = 0;

	while ((c = strstr(c, "\\n")) != NULL) {
		c[0] = '\r';
		c[1] = '\n';
		i++;
	}

	err = nrf_modem_at_cmd(response_buf, sizeof(response_buf), "%s", argv[1]);
	if (err) {
		shell_error(shell, "ERROR");
		return -EINVAL;
	}

	shell_print(shell, "%s", response_buf);

	return 0;
}

static int app_exit(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	set_at_prompt(shell, false);
	return 0;
}

static int cmd_reboot(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	device_shutdown(true);
	return 0;
}

static int cmd_shutdown(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	device_shutdown(false);
	return 0;
}

/*
 * example:
 * fota firmware.nrfcloud.com 3c7003c6-45a0-4a74-9023-1e006ceeb835/APP*30f6ce17*1.4.0/app_update.bin
 */
static int cmd_fota(const struct shell *shell, size_t argc, char **argv)
{
#if defined(CONFIG_NRF_CLOUD_FOTA)
	if (argc < 3) {
		return -EINVAL;
	}
	char *host = argv[1];
	char *path = argv[2];
	int sec_tag = CONFIG_NRF_CLOUD_SEC_TAG;
	size_t frag = CONFIG_NRF_CLOUD_FOTA_DOWNLOAD_FRAGMENT_SIZE;

	if (argc > 3) {
		sec_tag = atoi(argv[3]);
	}
	if (argc > 4) {
		frag = atoll(argv[4]);
	}
	shell_print(shell, "starting FOTA download from host:%s, path:%s, "
			   "sec_tag:%d, frag_size:%zd",
			   host, path, sec_tag, frag);

	char *space = strstr(path, " ");

	if (space) {
		shell_print(shell, "mcuboot download detected");
	}
	int err = fota_download_start(host, path, sec_tag, 0, frag);

	if (err) {
		shell_error(shell, "Error %d starting download", err);
	}
	return 0;
#else
	shell_error(shell, "FOTA support not enabled in this build.");
	return 0;
#endif
}

/*
 * example:
 * ble fota C2:6B:AC:6D:05:A3 firmware.beta.nrfcloud.com ba1752ef-0d36-4fcf-8748-3cad9f8801b0/
 *      APP*f1078fc9*2.2.0/app_thingy_s132.bin 155272 1 2.2.0
 * ble fota C2:6B:AC:6D:05:A3 firmware.beta.nrfcloud.com ba1752ef-0d36-4fcf-8748-3cad9f8801b0/
 *      APP*56693cf8*2.2.0/app_thingy_s132dat.bin 135 1 2.2.0
 * ble fota C2:6B:AC:6D:05:A3 firmware.beta.nrfcloud.com ba1752ef-0d36-4fcf-8748-3cad9f8801b0/
 *      APP*f15b85a8*s132/sd_bl.bin 153344 0 s132
 * ble fota C2:6B:AC:6D:05:A3 firmware.beta.nrfcloud.com ba1752ef-0d36-4fcf-8748-3cad9f8801b0/
 *      APP*3fb54637*s132/sd_bl_dat.bin 139 1 s132
 */
#if CONFIG_GATEWAY_BLE_FOTA
static int cmd_ble_fota(const struct shell *shell, size_t argc, char **argv)
{
	if (argc < 3) {
		return -EINVAL;
	}
	char *addr = argv[0];
	char *host = argv[1];
	char *path = argv[2];
	int size = 0;
	bool init_packet = true;
	char *ver = "1";
	uint32_t crc = 0;
	int sec_tag = -1;
	char *apn = NULL;
	size_t frag = 0;

	if (argc > 3) {
		size = atoi(argv[3]);
	}
	if (argc > 4) {
		init_packet = atoi(argv[4]) != 0;
	}
	if (argc > 5) {
		ver = argv[5];
	}
	if (argc > 6) {
		crc = atoi(argv[6]);
	}
	if (argc > 7) {
		sec_tag = atoi(argv[7]);
	}
	if (argc > 8) {
		frag = atoll(argv[8]);
	}
	if (argc > 9) {
		apn = argv[9];
	}

	shell_print(shell, "starting BLE DFU to addr:%s, from host:%s, "
			   "path:%s, size:%d, final:%d, ver:%s, crc:%u, "
			   "sec_tag:%d, apn:%s, frag_size:%zd",
			   addr, host, path, size, init_packet, ver, crc,
			   sec_tag, apn ? apn : "<n/a>", frag);

	/* set parameters for BLE update */
	int err;

	err = peripheral_dfu_config(addr, size, ver, crc, init_packet, true);
	if (err) {
		peripheral_dfu_cleanup();
		shell_error(shell, "Error %d starting peripheral DFU", err);
	} else {
		err = peripheral_dfu_start(host, path, sec_tag, apn, frag);
		if (err) {
			shell_error(shell, "Error %d starting download", err);
		}
	}
	return 0;
}

static void get_dynamic_ble_fota(size_t idx, struct shell_static_entry *entry)
{
	/* build dynamic list of mac addresses of only devices we have
	 * connected with (not scan results too)
	 */
	entry->syntax = get_mac_addr(idx, false);

	if (entry->syntax == NULL) {
		return;
	}

	entry->handler = cmd_ble_fota;
	entry->subcmd = NULL;
	entry->help = DYNAMIC_PARAM_HELP;
	entry->args.mandatory = 5;
	entry->args.optional = 5;
}

SHELL_DYNAMIC_CMD_CREATE(dynamic_ble_fota, get_dynamic_ble_fota);
#endif


static int cmd_session(const struct shell *shell, size_t argc, char **argv)
{
	if (argc < 2) {
		shell_print(shell, "Persistent sessions = %d",
			    nct_get_session_state());
	} else {
		int flag = atoi(argv[1]);

		if ((nct_get_session_state() == 0) && (flag == 1)) {
			shell_warn(shell, "Setting persistent sessions true "
					  "when it is not, may result "
					  "in data loss; use at your own "
					  "risk.");
		}
		shell_print(shell, "Setting persistent sessions = %d", flag);
		nct_save_session_state(flag);
	}
	return 0;
}

int check_passwd(char *passwd)
{
	return strcmp(passwd, DEFAULT_PASSWORD);
}

int is_valid_passwd(char *passwd)
{
	if (strlen(passwd) < 8) {
		return -EINVAL;
	} else {
		return 0;
	}
}

int set_passwd(char *passwd)
{
	return -ENOTSUP;
}

static int cmd_login(const struct shell *shell, size_t argc, char **argv)
{
	static uint32_t attempts;

	if (argc < 2) {
		shell_print(shell, "Access requires: <password>");
		return -EINVAL;
	}

	if (check_passwd(argv[1]) == 0) {
		attempts = 0;
		shell_obscure_set(shell, false);
		z_shell_log_backend_enable(shell->log_backend, (void *)shell,
					   CONFIG_STARTING_LOG_LEVEL);
		z_shell_history_purge(shell->history);
		shell_set_root_cmd(NULL);
		shell_prompt_change(shell, CONFIG_SHELL_PROMPT_SECURE);

		shell_print(shell, "\nnRF Cloud Gateway\n");
		print_fw_info(shell, false);

		shell_print(shell, "\nHit tab for help.\n");
		return 0;
	}
	shell_error(shell, "Incorrect password!");
	attempts++;
	if (attempts > 3) {
		k_sleep(K_SECONDS(attempts));
	}
	return -EINVAL;
}

static int cmd_passwd(const struct shell *shell, size_t argc, char **argv)
{
	int err;

	err = check_passwd(argv[1]);
	if (err) {
		shell_error(shell, "Incorrect password!");
		return err;
	}

	err = is_valid_passwd(argv[2]);
	if (err) {
		shell_error(shell, "Invalid password.  Must be 8 characters or longer.");
		return err;
	}

	err = set_passwd(argv[2]);
	if (!err) {
		shell_print(shell, "Password changed.");
	} else {
		shell_error(shell, "Unable to store password.");
	}

	return err;
}

static int cmd_logout(const struct shell *shell, size_t argc, char **argv)
{
	shell_set_root_cmd("login");
	shell_obscure_set(shell, true);
	shell_prompt_change(shell, CONFIG_SHELL_PROMPT_UART);
	shell_print(shell, "\n");
	return 0;
}

void cli_init(void)
{
	const struct shell *shell = shell_backend_uart_get_ptr();

	if (shell) {
		/*
		 * Zephyr commit: SHA-1: 2b5723d4558619a9283683499c095e0b344d32d7
		 *                shell: add init backend configuration
		 * broke the config system regarding obscure, so for now,
		 * set it manually on
		 */
		shell_obscure_set(shell, IS_ENABLED(CONFIG_SHELL_START_OBSCURED));
	}

	if (IS_ENABLED(CONFIG_SHELL_START_OBSCURED)) {
		shell_set_root_cmd("login");
	} else {
		shell_prompt_change(shell, CONFIG_SHELL_PROMPT_SECURE);
	}

#if defined(CONFIG_STARTING_LOG_OVERRIDE)
	for (int i = 0; i < log_src_cnt_get(CONFIG_LOG_DOMAIN_ID); i++) {
		if (IS_ENABLED(CONFIG_NRF_CLOUD_GATEWAY_LOG_LEVEL_DBG)) {
			printk("%d. %s\n", i,
			       log_source_name_get(CONFIG_LOG_DOMAIN_ID, i));
		}
		log_filter_set(NULL, CONFIG_LOG_DOMAIN_ID, i,
			       CONFIG_STARTING_LOG_LEVEL);
	}
#endif
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_info,
	SHELL_CMD(cloud, NULL, "Cloud information.", cmd_info_cloud),
	SHELL_CMD(ctlr, NULL, "BLE controller information.", cmd_info_ctlr),
	SHELL_CMD(conn, NULL, "[path] [notify] Connected Bluetooth devices information.",
		  cmd_info_conn),
	SHELL_CMD(gateway, NULL, "<verbose> Gateway information.", cmd_info_gateway),
	SHELL_CMD(heap, NULL, "Print heap usage statistics.", heap_shell),
	SHELL_COND_CMD(CONFIG_GATEWAY_DBG_CMDS, irq, NULL, "Dump IRQ table.", cmd_info_irq),
	SHELL_COND_CMD(CONFIG_GATEWAY_DBG_CMDS, list, &dynamic_addr,
		       "List known BLE MAC addresses.", NULL),
	SHELL_CMD(modem, NULL, "<verbose> Modem information.", cmd_info_modem),
	SHELL_COND_CMD(CONFIG_GATEWAY_DBG_CMDS, param, &dynamic_param, "List parameters.", NULL),
	SHELL_CMD(scan, NULL, "Bluetooth scan results.", cmd_info_scan),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(info, &sub_info, "Informational commands", NULL);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_ble,
	SHELL_CMD(scan, NULL, "Scan for BLE devices.", cmd_ble_scan),
	SHELL_CMD(save, NULL, "Save desired connections to shadow.", cmd_ble_save),
	SHELL_CMD(conn, &dynamic_ble_conn, "<all | name | MAC> Connect to BLE device(s).", NULL),
	SHELL_CMD(disc, &dynamic_ble_disc, "<all | name | MAC> Disconnect BLE device(s).", NULL),
	SHELL_CMD(en, &dynamic_ble_en, "<all | MAC [all | handle]> Enable "
				       "notifications on BLE device(s).", NULL),
	SHELL_CMD(dis, &dynamic_ble_dis, "<all | MAC [all | handle]> Disable "
					 "notifications on BLE device(s).", NULL),
#if CONFIG_GATEWAY_BLE_FOTA
	SHELL_CMD(fota, &dynamic_ble_fota,
		 "<addr> <host> <path> <size> <final> [ver] [crc] [sec_tag] [frag_size] [apn] "
		 "BLE firmware over-the-air update.", NULL),
	SHELL_CMD(test, NULL, "Set BLE FOTA download test mode.", cmd_ble_test),
#endif
	SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_ARG_REGISTER(ble, &sub_ble, "Bluetooth commands", NULL, 0, 3);

SHELL_CMD_ARG_REGISTER(fota, NULL, "<host> <path> [sec_tag] [frag_size] [apn] "
				   "firmware over-the-air update.", cmd_fota, 2, 2);
SHELL_CMD_ARG_REGISTER(at, NULL, "<enable | AT<cmd> | exit> Execute an AT "
				 "command.  Use <at enable> first to remain "
				 "in AT command mode until 'exit'.",
		       app_cmd_at, 2, SHELL_OPT_ARG_RAW);
SHELL_CMD_ARG_REGISTER(session, NULL, "<0 | 1> Get or change persistent sessions flag.",
		       cmd_session, 0, 1);
SHELL_CMD_REGISTER(testflash, NULL, "Test the external flash.", cmd_testflash);
SHELL_CMD_REGISTER(reboot, NULL, "Reboot the gateway.", cmd_reboot);
SHELL_CMD_REGISTER(shutdown, NULL, "Shutdown the gateway.", cmd_shutdown);
SHELL_CMD_REGISTER(exit, NULL, "Exit 'select at' mode.", app_exit);
SHELL_CMD_ARG_REGISTER(login, NULL, "<password>", cmd_login, 2, 0);
SHELL_CMD_ARG_REGISTER(passwd, NULL, "<oldpw> <newpw>", cmd_passwd, 3, 0);
SHELL_CMD_REGISTER(logout, NULL, "Log out.", cmd_logout);
