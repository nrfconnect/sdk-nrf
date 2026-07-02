/*
 * Copyright (c) 2026 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/reboot.h>
#include <ironside/se/api.h>
#include <ironside/se/boot_report.h>
#include <ironside/se/uicr_deploy.h>

LOG_MODULE_REGISTER(snapshot_suspend_to_ram, CONFIG_LOG_DEFAULT_LEVEL);

#define IRONSIDE_NV_COUNTER_ID IRONSIDE_SE_COUNTER_0
#define SNAPSHOT_MAX_CYCLES    6U
#define HEARTBEAT_PERIOD       K_SECONDS(3)
#define IDLE_PERIOD            K_MSEC(2000)

static const char *snapshot_status_str(uint8_t status)
{
	switch (status) {
	case IRONSIDE_SE_SNAPSHOT_STATUS_NONE:
		return "None";
	case IRONSIDE_SE_SNAPSHOT_STATUS_CAPTURE_SUCCESSFUL:
		return "Capture Successful";
	case IRONSIDE_SE_SNAPSHOT_STATUS_CAPTURE_FAILED:
		return "Capture Failed";
	case IRONSIDE_SE_SNAPSHOT_STATUS_RECOVERY:
		return "Recovery";
	case IRONSIDE_SE_SNAPSHOT_STATUS_CORRUPTION_DETECTED:
		return "Corruption Detected";
	default:
		return "Unknown";
	}
}

static void idle_forever(void)
{
	for (;;) {
		k_sleep(HEARTBEAT_PERIOD);
	}
}

/*
 * Low-power idle loop, equivalent to the multicore idle benchmark
 * (tests/benchmarks/multicore/idle). With CONFIG_PM enabled the system enters
 * suspend-to-RAM between wake-ups, so together with the empty idle image on the
 * radio core the whole SoC reaches low power.
 */
static void low_power_idle_loop(void)
{
	unsigned int cnt = 0;

	/*
	 * Offset the first sleep so the application and radio cores do not wake
	 * in perfect lock-step (matches the multicore idle benchmark).
	 */
	k_msleep(1000);

	LOG_INF("Entering low-power idle loop on %s", CONFIG_BOARD_TARGET);
	for (;;) {
		LOG_INF("Low-power idle iteration %u", cnt++);
		k_sleep(IDLE_PERIOD);
	}
}

int main(void)
{
	const struct ironside_se_boot_report *boot_report = IRONSIDE_SE_BOOT_REPORT;
	uint32_t nv_counter_at_boot;
	uint32_t nv_counter_current;
	uint8_t snapshot_status;
	int rc;

	if (boot_report == NULL || boot_report->magic != IRONSIDE_SE_BOOT_REPORT_MAGIC) {
		LOG_ERR("Invalid IronSide SE boot report");
		idle_forever();
	}

	rc = ironside_se_counter_get(IRONSIDE_NV_COUNTER_ID, &nv_counter_at_boot);
	if (rc != 0) {
		LOG_ERR("Failed to read NV counter: %d", rc);
		return rc;
	}

	nv_counter_current = nv_counter_at_boot + 1U;
	rc = ironside_se_counter_set(IRONSIDE_NV_COUNTER_ID, nv_counter_current);
	if (rc != 0) {
		LOG_ERR("Failed to increment NV counter (%u -> %u): %d", nv_counter_at_boot,
			nv_counter_current, rc);
		return rc;
	}
	LOG_INF("NV counter: %u -> %u", nv_counter_at_boot, nv_counter_current);

	if (nv_counter_at_boot == 0U) {
		int lock_rc = uicr_deploy_lock_contents();

		if (lock_rc == 0) {
			LOG_INF("UICR lock requested (takes effect after reset); rebooting");
			sys_reboot(SYS_REBOOT_COLD);
			CODE_UNREACHABLE;
		}

		/* -1: already locked (e.g. after prior boot programmed lock). */
		LOG_INF("UICR lock not applied (%d); continuing", lock_rc);
	}

	snapshot_status = boot_report->snapshot.status;
	if (snapshot_status <= IRONSIDE_SE_SNAPSHOT_STATUS_CORRUPTION_DETECTED) {
		LOG_INF("Snapshot status: %s", snapshot_status_str(snapshot_status));
	} else {
		LOG_WRN("Snapshot status: Unknown (0x%02x)", snapshot_status);
	}

	if (nv_counter_current >= SNAPSHOT_MAX_CYCLES) {
		LOG_INF("Reached cycle limit (%u), entering low-power idle loop",
			SNAPSHOT_MAX_CYCLES);
		low_power_idle_loop();
	}

	switch (snapshot_status) {
	case IRONSIDE_SE_SNAPSHOT_STATUS_NONE:
		LOG_INF("Requesting snapshot capture");
		rc = ironside_se_snapshot_capture(IRONSIDE_SE_SNAPSHOT_CAPTURE_NO_INCREMENT);
		LOG_ERR("snapshot_capture returned unexpectedly: %d", rc);
		return rc;

	case IRONSIDE_SE_SNAPSHOT_STATUS_CAPTURE_SUCCESSFUL:
		LOG_INF("Requesting snapshot recovery");
		rc = ironside_se_snapshot_recovery();
		LOG_ERR("snapshot_recovery returned unexpectedly: %d", rc);
		return rc;

	case IRONSIDE_SE_SNAPSHOT_STATUS_RECOVERY:
		LOG_INF("Snapshot recovery completed, rebooting");
		sys_reboot(SYS_REBOOT_COLD);
		break;

	case IRONSIDE_SE_SNAPSHOT_STATUS_CAPTURE_FAILED:
		LOG_ERR("Snapshot capture failed");
		idle_forever();
		break;

	case IRONSIDE_SE_SNAPSHOT_STATUS_CORRUPTION_DETECTED:
		LOG_ERR("Snapshot corruption detected");
		idle_forever();
		break;

	default:
		LOG_ERR("Unsupported snapshot status: 0x%02x", snapshot_status);
		idle_forever();
		break;
	}

	return 0;
}
