/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/mbox.h>
#include <zephyr/init.h>

#include <ironside/se/event_report.h>
#include <ironside/se/api.h>
#include <ironside/se/peripheral_interface.h>

#define WATCHED_EVENT_MASK (IRONSIDE_SE_EVENT_SPU_ALL_MASK | IRONSIDE_SE_EVENT_MPC_ALL_MASK | \
			    IRONSIDE_SE_EVENT_MRAMC_ECCERROR_ALL_MASK)

/* Bell 1: general (SPU/MPC/MRAMC) event report notifications. */
static const struct mbox_dt_spec event_bell = {
	.dev = DEVICE_DT_GET(DT_NODELABEL(cpuapp_bellboard)),
	.channel_id = IRONSIDE_SE_BELLBOARD_EVENT_RX_BELL_IDX,
};

/* Bell 2: CELL domain event report notifications. */
static const struct mbox_dt_spec cell_event_bell = {
	.dev = DEVICE_DT_GET(DT_NODELABEL(cpuapp_bellboard)),
	.channel_id = IRONSIDE_SE_BELLBOARD_CELL_EVENT_RX_BELL_IDX,
};

static const char *const spu_names[] = IRONSIDE_SE_EVENT_REPORT_SPU_NAME_ARRAY;
static const char *const mpc_names[] = IRONSIDE_SE_EVENT_REPORT_MPC_NAME_ARRAY;
static const char *const mramc_names[] = IRONSIDE_SE_EVENT_REPORT_MRAMC_NAME_ARRAY;

/**
 * Print and clear any pending events in the IronSide SE event report.
 *
 * Called directly from the bellboard ISR context. The events reported here can coincide with,
 * or be caused by, a fault on this core, so the report is printed immediately with printk()
 * instead of being deferred to a workqueue.
 */
static void ironside_se_event_report_print(void)
{
	struct ironside_se_event_report *report = IRONSIDE_SE_EVENT_REPORT;

	for (int i = 0; i < IRONSIDE_SE_EVENT_REPORT_SPU_NUM; i++) {
		if (ironside_se_spu_periphaccerr_event_check(report, i)) {
			NRF_SPU_PERIPHACCERR_Type periphaccerr;

			ironside_se_spu_periphaccerr_get(report, i, &periphaccerr);
			printk("IronSide SE event: SPU%s PERIPHACCERR address=0x%x info=0x%x\n",
			       spu_names[i], periphaccerr.ADDRESS, periphaccerr.INFO);
			ironside_se_spu_periphaccerr_event_clear(report, i);
		}
	}

	for (int i = 0; i < IRONSIDE_SE_EVENT_REPORT_MPC_NUM; i++) {
		if (ironside_se_mpc_memaccerr_event_check(report, i)) {
			NRF_MPC_MEMACCERR_Type memaccerr;

			ironside_se_mpc_memaccerr_get(report, i, &memaccerr);
			printk("IronSide SE event: MPC%s MEMACCERR address=0x%x info=0x%x\n",
			       mpc_names[i], memaccerr.ADDRESS, memaccerr.INFO);
			ironside_se_mpc_memaccerr_event_clear(report, i);
		}
	}

	for (int i = 0; i < IRONSIDE_SE_EVENT_REPORT_MRAMC_NUM; i++) {
		if (ironside_se_mramc_ecc_error_event_check(report, i)) {
			uint32_t erroraddr;

			ironside_se_mramc_ecc_erroraddr_get(report, i, &erroraddr);
			printk("IronSide SE event: MRAMC%s ECC ERROR address=0x%x\n",
			       mramc_names[i], erroraddr);
			ironside_se_mramc_ecc_error_event_clear(report, i);
		}
	}

	if (ironside_se_cell_reset_event_check(report)) {
		uint32_t resetreas;

		ironside_se_cell_resetreas_get(report, &resetreas);
		printk("IronSide SE event: CELL domain reset, resetreas=0x%x\n", resetreas);
		ironside_se_cell_reset_event_clear(report);
	}
}

/* Runs in ISR context (bellboard IRQ); called for both bell 1 and bell 2. */
static void event_bell_cb(const struct device *dev, uint32_t channel, void *user_data,
			  struct mbox_msg *data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(channel);
	ARG_UNUSED(user_data);
	ARG_UNUSED(data);

	ironside_se_event_report_print();
}

static int ironside_se_event_init(void)
{
	int err;

	err = mbox_register_callback_dt(&event_bell, event_bell_cb, NULL);
	if (err) {
		printk("Failed to register IronSide SE event bell callback, err %d\n", err);
		return err;
	}

	err = mbox_set_enabled_dt(&event_bell, true);
	if (err) {
		printk("Failed to enable IronSide SE event bell, err %d\n", err);
		return err;
	}

	err = mbox_register_callback_dt(&cell_event_bell, event_bell_cb, NULL);
	if (err) {
		printk("Failed to register IronSide SE CELL event bell callback, err %d\n", err);
		return err;
	}

	err = mbox_set_enabled_dt(&cell_event_bell, true);
	if (err) {
		printk("Failed to enable IronSide SE CELL event bell, err %d\n", err);
		return err;
	}

	/* Bell 2 (CELL domain reset) needs no explicit enabling; bell 1 events do. */
	err = ironside_se_events_enable(WATCHED_EVENT_MASK);
	if (err) {
		printk("Failed to enable IronSide SE events, err %d\n", err);
		return err;
	}

	return 0;
}

SYS_INIT(ironside_se_event_init, APPLICATION, 0);
