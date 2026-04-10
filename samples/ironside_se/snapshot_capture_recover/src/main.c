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

LOG_MODULE_REGISTER(snapshot_capture_recover, CONFIG_LOG_DEFAULT_LEVEL);

#define IRONSIDE_NV_COUNTER_ID IRONSIDE_SE_COUNTER_0
#define SNAPSHOT_MAX_CYCLES    6U
#define HEARTBEAT_PERIOD       K_SECONDS(3)
#define HEARTBEAT_ITERATIONS   100000
#define MRAM_BASE_ADDRESS      0x0E042000U
#define MRAM_REGION_SIZE       (256U * 1024U)
#define MRAM_WORD_COUNT	       (MRAM_REGION_SIZE / sizeof(uint32_t))
#define MRAM_CHECK_INTERVAL    5

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

static void mram_read_only_sweep_check(void)
{
	volatile uint32_t *const mram = (volatile uint32_t *)MRAM_BASE_ADDRESS;
	uint32_t checksum = 0U;
	uint32_t first_word;
	uint32_t last_word;

	LOG_INF("Starting read-only MRAM sweep over 0x%08x .. 0x%08x", MRAM_BASE_ADDRESS,
		MRAM_BASE_ADDRESS + MRAM_REGION_SIZE);

	first_word = mram[0];
	last_word = mram[MRAM_WORD_COUNT - 1U];
	for (size_t i = 0U; i < MRAM_WORD_COUNT; ++i) {
		checksum ^= mram[i];
	}

	/* Note that we don't request recovery based on a checksum
	 * mismatch here as ISE will request recovery automatically on ECC
	 * errors.
	 */

	LOG_INF("Finished read-only MRAM sweep: first=0x%08x last=0x%08x xor=0x%08x", first_word,
		last_word, checksum);
}

static void idle_forever(void)
{
	for (;;) {
		k_sleep(HEARTBEAT_PERIOD);
	}
}

static void heartbeat_with_mram_read_sweeps(void)
{
	for (int i = 0; i < HEARTBEAT_ITERATIONS; ++i) {
		LOG_INF("Heartbeat %d", i);
		if (((i + 1) % MRAM_CHECK_INTERVAL) == 0) {
			LOG_INF("Running read-only MRAM sweep check");
			mram_read_only_sweep_check();
		}
		k_sleep(HEARTBEAT_PERIOD);
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
		LOG_INF("Reached cycle limit (%u), stopping capture/recovery flow",
			SNAPSHOT_MAX_CYCLES);
		heartbeat_with_mram_read_sweeps();
		idle_forever();
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
