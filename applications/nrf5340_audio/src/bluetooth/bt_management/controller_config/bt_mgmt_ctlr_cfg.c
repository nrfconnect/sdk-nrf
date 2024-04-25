/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "bt_mgmt_ctlr_cfg_internal.h"

#include <zephyr/bluetooth/hci.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/task_wdt/task_wdt.h>
#include <zephyr/logging/log_ctrl.h>

#include "macros_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_mgmt_ctlr_cfg, CONFIG_BT_MGMT_CTLR_CFG_LOG_LEVEL);

#define COMPANY_ID_NORDIC 0x0059

#define WDT_TIMEOUT_MS	      3000
#define CTLR_POLL_INTERVAL_MS (WDT_TIMEOUT_MS - 1000)

static struct k_work work_ctlr_poll;

#define CTLR_POLL_WORK_STACK_SIZE 1024

K_THREAD_STACK_DEFINE(ctlr_poll_stack_area, CTLR_POLL_WORK_STACK_SIZE);

struct k_work_q ctrl_poll_work_q;

struct k_work_queue_config ctrl_poll_work_q_config = {
	.name = "ctlr_poll",
	.no_yield = false,
};

static void ctlr_poll_timer_handler(struct k_timer *timer_id);
static int wdt_ch_id;

K_TIMER_DEFINE(ctlr_poll_timer, ctlr_poll_timer_handler, NULL);

static void work_ctlr_poll_handler(struct k_work *work)
{
	int ret;
	uint16_t manufacturer = 0;

	ret = bt_mgmt_ctlr_cfg_manufacturer_get(false, &manufacturer);
	ERR_CHK_MSG(ret, "Failed to contact net core");

	ret = task_wdt_feed(wdt_ch_id);
	ERR_CHK_MSG(ret, "Failed to feed watchdog");
}

static void ctlr_poll_timer_handler(struct k_timer *timer_id)
{
	int ret;

	ret = k_work_submit_to_queue(&ctrl_poll_work_q, &work_ctlr_poll);
	if (ret < 0) {
		LOG_ERR("Work q submit failed: %d", ret);
	}
}

static void wdt_timeout_cb(int channel_id, void *user_data)
{
	ERR_CHK_MSG(-ETIMEDOUT, "No response from IPC or controller");
}

int bt_mgmt_ctlr_cfg_manufacturer_get(bool print_version, uint16_t *manufacturer)
{
	int ret;
	struct net_buf *rsp;

	ret = bt_hci_cmd_send_sync(BT_HCI_OP_READ_LOCAL_VERSION_INFO, NULL, &rsp);
	if (ret) {
		return ret;
	}

	struct bt_hci_rp_read_local_version_info *rp = (void *)rsp->data;

	if (print_version) {
		if (rp->manufacturer == COMPANY_ID_NORDIC) {
			/* NOTE: The string below is used by the Nordic CI system */
			LOG_INF("Controller: SoftDevice: Version %s (0x%02x), Revision %d",
				bt_hci_get_ver_str(rp->hci_version), rp->hci_version,
				rp->hci_revision);
		} else {
			LOG_ERR("Unsupported controller");
			return -EPERM;
		}
	}

	*manufacturer = sys_le16_to_cpu(rp->manufacturer);

	net_buf_unref(rsp);

	return 0;
}

int bt_mgmt_ctlr_cfg_init(bool watchdog_enable)
{
	int ret;
	uint16_t manufacturer = 0;

	ret = bt_mgmt_ctlr_cfg_manufacturer_get(true, &manufacturer);
	if (ret) {
		return ret;
	}

	if (watchdog_enable) {
		ret = task_wdt_init(NULL);
		if (ret != 0) {
			LOG_ERR("task wdt init failure: %d\n", ret);
			return ret;
		}

		wdt_ch_id = task_wdt_add(WDT_TIMEOUT_MS, wdt_timeout_cb, NULL);
		if (wdt_ch_id < 0) {
			return wdt_ch_id;
		}
		k_work_queue_init(&ctrl_poll_work_q);

		k_work_queue_start(&ctrl_poll_work_q, ctlr_poll_stack_area,
				   K_THREAD_STACK_SIZEOF(ctlr_poll_stack_area),
				   K_PRIO_PREEMPT(CONFIG_CTLR_POLL_WORK_Q_PRIO),
				   &ctrl_poll_work_q_config);

		k_work_init(&work_ctlr_poll, work_ctlr_poll_handler);
		k_timer_start(&ctlr_poll_timer, K_MSEC(CTLR_POLL_INTERVAL_MS),
			      K_MSEC(CTLR_POLL_INTERVAL_MS));
	}

	return 0;
}
