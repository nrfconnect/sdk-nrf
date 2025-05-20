/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#if defined CONFIG_DESKTOP_DFU_BACKEND_MCUBOOT
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/mgmt/mcumgr/grp/img_mgmt/img_mgmt.h>
#elif defined CONFIG_DESKTOP_DFU_BACKEND_SUIT
#include <sdfw/sdfw_services/suit_service.h>
#include <dfu/suit_dfu.h>
#endif

#include <zephyr/mgmt/mcumgr/grp/os_mgmt/os_mgmt.h>
#include <zephyr/mgmt/mcumgr/mgmt/callbacks.h>
#include <zephyr/sys/math_extras.h>

#include <app_event_manager.h>
#include "dfu_lock.h"

#define MODULE dfu_mcumgr
#include <caf/events/module_state_event.h>
#include <caf/events/ble_smp_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_DFU_MCUMGR_LOG_LEVEL);

#define DFU_TIMEOUT	K_SECONDS(5)

static struct k_work_delayable dfu_timeout;

static atomic_t mcumgr_event_active = ATOMIC_INIT(false);

/* nRF Desktop MCUmgr DFU module cannot be enabled together with the CAF BLE SMP module. */
BUILD_ASSERT(!IS_ENABLED(CONFIG_CAF_BLE_SMP));

static void dfu_lock_owner_changed(const struct dfu_lock_owner *new_owner)
{
	LOG_DBG("MCUmgr progress reset due to the different DFU owner: %s", new_owner->name);

#ifdef CONFIG_DESKTOP_DFU_LOCK
	/* The function declaration is not included in MCUmgr's header file if the mutex locking
	 * of the image management state object is disabled.
	 */
#if defined CONFIG_DESKTOP_DFU_BACKEND_MCUBOOT
	img_mgmt_reset_upload();
#elif defined CONFIG_DESKTOP_DFU_BACKEND_SUIT
	int err = suit_dfu_cleanup();

	if (err) {
		module_set_state(MODULE_STATE_ERROR);
		LOG_ERR("Failed to cleanup SUIT DFU: %d", err);
	}
#endif
#endif /* CONFIG_DESKTOP_DFU_LOCK */
}

const static struct dfu_lock_owner mcumgr_owner = {
	.name = "MCUmgr",
	.owner_changed = dfu_lock_owner_changed,
};

static void dfu_timeout_handler(struct k_work *work)
{
	LOG_WRN("MCUmgr DFU timed out");

	if (IS_ENABLED(CONFIG_DESKTOP_DFU_LOCK)) {
		int err;

		err = dfu_lock_release(&mcumgr_owner);
		__ASSERT_NO_MSG(!err);
	}
}

static bool smp_cmd_is_suit_dfu_cmd(const struct mgmt_evt_op_cmd_arg *cmd,
				    const char **smp_cmd_name)
{
#if CONFIG_MGMT_SUITFU_GRP_SUIT
	if (cmd->group == CONFIG_MGMT_GROUP_ID_SUIT) {
		*smp_cmd_name = "SUIT Management";
		return true;
	}
#endif /* CONFIG_MGMT_SUITFU_GRP_SUIT */

	return false;
}

static bool smp_cmd_is_dfu_cmd(const struct mgmt_evt_op_cmd_arg *cmd)
{
	const char *smp_cmd_name = "Unknown";
	bool dfu_transfer_cmd = true;

	if (cmd->group == MGMT_GROUP_ID_IMAGE) {
		smp_cmd_name = "Image Management";
	} else if ((cmd->group == MGMT_GROUP_ID_OS) && (cmd->id == OS_MGMT_ID_RESET)) {
		smp_cmd_name = "OS Management Reset";
	} else {
		dfu_transfer_cmd = false;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_DFU_BACKEND_SUIT) && !dfu_transfer_cmd) {
		dfu_transfer_cmd = smp_cmd_is_suit_dfu_cmd(cmd, &smp_cmd_name);
	}

	if (dfu_transfer_cmd && smp_cmd_name) {
		LOG_DBG("MCUmgr %s event", smp_cmd_name);
	}

	return dfu_transfer_cmd;
}

static enum mgmt_cb_return smp_cmd_recv(uint32_t event, enum mgmt_cb_return prev_status,
					int32_t *rc, uint16_t *group, bool *abort_more,
					void *data, size_t data_size)
{
	const struct mgmt_evt_op_cmd_arg *cmd_recv;

	if (event != MGMT_EVT_OP_CMD_RECV) {
		LOG_ERR("Spurious event in recv cb: %" PRIu32, event);
		*rc = MGMT_ERR_EUNKNOWN;
		return MGMT_CB_ERROR_RC;
	}

	if (data_size != sizeof(*cmd_recv)) {
		LOG_ERR("Invalid data size in recv cb: %zu (expected: %zu)",
			data_size, sizeof(*cmd_recv));
		*rc = MGMT_ERR_EUNKNOWN;
		return MGMT_CB_ERROR_RC;
	}

	cmd_recv = data;

	LOG_DBG("MCUmgr SMP Command Recv Event: group_id=%d cmd_id=%d",
		cmd_recv->group, cmd_recv->id);

	/* Ignore commands not related to DFU over SMP. */
	if (!smp_cmd_is_dfu_cmd(cmd_recv)) {
		return MGMT_CB_OK;
	}

	k_work_reschedule(&dfu_timeout, DFU_TIMEOUT);
	if (IS_ENABLED(CONFIG_DESKTOP_DFU_LOCK) && dfu_lock_claim(&mcumgr_owner)) {
		(void)k_work_cancel_delayable(&dfu_timeout);
		*rc = MGMT_ERR_EACCESSDENIED;
		return MGMT_CB_ERROR_RC;
	}

	if (IS_ENABLED(CONFIG_MCUMGR_TRANSPORT_BT) &&
	    atomic_cas(&mcumgr_event_active, false, true)) {
		APP_EVENT_SUBMIT(new_ble_smp_transfer_event());
	}

	return MGMT_CB_OK;
}

static struct mgmt_callback cmd_recv_cb = {
	.callback = smp_cmd_recv,
	.event_id = MGMT_EVT_OP_CMD_RECV,
};

#if CONFIG_DESKTOP_DFU_BACKEND_MCUBOOT
static void dfu_backend_init(void)
{
	if (!IS_ENABLED(CONFIG_DESKTOP_DFU_MCUMGR_MCUBOOT_DIRECT_XIP)) {
		int err = boot_write_img_confirmed();

		if (err) {
			LOG_ERR("Cannot confirm a running image: %d", err);
		}
	}

	LOG_INF("MCUboot image version: %s", CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION);
}
#elif CONFIG_DESKTOP_DFU_BACKEND_SUIT
static const char *suit_release_type_str_get(suit_version_release_type_t type)
{
	switch (type) {
	case SUIT_VERSION_RELEASE_NORMAL:
		return NULL;
	case SUIT_VERSION_RELEASE_RC:
		return "rc";
	case SUIT_VERSION_RELEASE_BETA:
		return "beta";
	case SUIT_VERSION_RELEASE_ALPHA:
		return "alpha";
	default:
		__ASSERT(0, "Unknown release type");
		return NULL;
	}
};

static void dfu_backend_init(void)
{
	int err;
	bool is_semver_supported;
	unsigned int seq_num = 0;
	suit_ssf_manifest_class_info_t class_info;
	suit_semver_raw_t version_raw;
	suit_version_t version;

	err = suit_get_supported_manifest_info(SUIT_MANIFEST_APP_ROOT, &class_info);
	if (!err) {
		err = suit_get_installed_manifest_info(&(class_info.class_id),
				&seq_num, &version_raw, NULL, NULL, NULL);
	}
	if (!err) {
		/* Semantic versioning support has been added to the SDFW in the v0.6.2
		 * public release. Older SDFW versions return empty array in the version
		 * variable.
		 */
		is_semver_supported = (version_raw.len != 0);
		if (is_semver_supported) {
			err = suit_metadata_version_from_array(&version,
							       version_raw.raw,
							       version_raw.len);
		}
	}

	if (!err) {
		if (is_semver_supported) {
			const char *release_type;

			release_type = suit_release_type_str_get(version.type);
			if (release_type) {
				LOG_INF("SUIT manifest version: %d.%d.%d-%s%d",
					version.major, version.minor, version.patch,
					release_type, version.pre_release_number);
			} else {
				LOG_INF("SUIT manifest version: %d.%d.%d",
					version.major, version.minor, version.patch);
			}
		}

		LOG_INF("SUIT manifest sequence number: %d", seq_num);
	} else {
		LOG_ERR("SUIT manifest info retrieval failed (err: %d)", err);
	}
}
#else
#error "The DFU backend choice is not supported"
#endif /* CONFIG_DESKTOP_DFU_BACKEND_MCUBOOT */

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (IS_ENABLED(CONFIG_MCUMGR_TRANSPORT_BT) &&
	    is_ble_smp_transfer_event(aeh)) {
		bool res = atomic_cas(&mcumgr_event_active, true, false);

		__ASSERT_NO_MSG(res);
		ARG_UNUSED(res);

		return false;
	}

	if (is_module_state_event(aeh)) {
		const struct module_state_event *event =
			cast_module_state_event(aeh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			dfu_backend_init();

			k_work_init_delayable(&dfu_timeout, dfu_timeout_handler);

			mgmt_callback_register(&cmd_recv_cb);
		}
		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
#if CONFIG_MCUMGR_TRANSPORT_BT
APP_EVENT_SUBSCRIBE_FINAL(MODULE, ble_smp_transfer_event);
#endif
