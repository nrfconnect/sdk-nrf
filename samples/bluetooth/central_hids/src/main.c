/* main.c - Application main entry point */

/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>
#include <inttypes.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <bluetooth/gatt_dm.h>
#include <zephyr/sys/byteorder.h>
#include <bluetooth/scan.h>
#include <bluetooth/services/hogp.h>
#include <dk_buttons_and_leds.h>

#include <zephyr/settings/settings.h>

#if defined(CONFIG_SOC_SERIES_NRF54H) || defined(CONFIG_SOC_SERIES_NRF54L)
#define BUTTON_NUM_FOR_BOARD(button) (button)
#else
#define BUTTON_NUM_FOR_BOARD(button) ((button) + 1)
#endif

/**
 * Switch between boot protocol and report protocol mode.
 */
#define KEY_BOOTMODE_MASK DK_BTN2_MSK
/**
 * Switch CAPSLOCK state.
 *
 * @note
 * For simplicity of the code it works only in boot mode.
 */
#define KEY_CAPSLOCK_MASK DK_BTN1_MSK
/**
 * Switch CAPSLOCK state with response
 *
 * Write CAPSLOCK with response.
 * Just for testing purposes.
 * The result should be the same like usine @ref KEY_CAPSLOCK_MASK
 */
#define KEY_CAPSLOCK_RSP_MASK DK_BTN3_MSK

/* Key used to accept or reject passkey value */
#define KEY_PAIRING_ACCEPT DK_BTN1_MSK
#define KEY_PAIRING_REJECT DK_BTN2_MSK

/** Key used to enter/exit alternative button function mode (toggle on same DK button). */
#define KEY_ALTERNATIVE_FUNCTIONS_NUM  DK_BTN4
#define KEY_ALTERNATIVE_FUNCTIONS_MASK DK_BTN4_MSK

/* Key used to request next SCI mode */
#define KEY_ALTERNATIVE_1_SCI_MODE_NEXT_NUM DK_BTN2
#define KEY_ALTERNATIVE_1_SCI_MODE_NEXT_MASK DK_BTN2_MSK

enum button_functions_mode {
	BUTTON_FUNCTIONS_MODE_DEFAULT,
	BUTTON_FUNCTIONS_MODE_AUTHENTICATION,
	BUTTON_FUNCTIONS_MODE_ALTERNATIVE_1
};

#ifndef CONFIG_SAMPLE_BT_CENTRAL_HIDS_CONTINUOUS_REPORT_RX_STATS_RATE
/* Define used to satisfy the compiler when continuous report receiving is not enabled */
#define CONFIG_SAMPLE_BT_CENTRAL_HIDS_CONTINUOUS_REPORT_RX_STATS_RATE 1000
#endif

/**
 * Entrance into continuous receive mode: after every
 * CONT_REPORT_RX_ENTRANCE_SAMPLE_REPORTS notifications we measure how long that batch took;
 * if it is shorter than CONTINUOUS_REPORT_RECEIVING_ENTRANCE_THRESHOLD_MS,
 * we enable continuous mode.
 */
#define CONT_REPORT_RX_ENTRANCE_SAMPLE_REPORTS 10
#define CONTINUOUS_REPORT_RECEIVING_ENTRANCE_THRESHOLD_MS 1000

/**
 * Exit from continuous receive mode: if no report is received for
 * CONTINUOUS_REPORT_RECEIVING_EXIT_THRESHOLD_MS milliseconds,
 * continuous rx mode will be automatically disabled.
 */
#define CONTINUOUS_REPORT_RECEIVING_EXIT_THRESHOLD_MS 500

/**
 * Reports are considered to be within the expected window if the absolute difference between
 * the report interval and the connection interval is less than this value.
 * The sample tracks how many reports are received outside this expected time window,
 * both above and below the threshold.
 */
#define CONTINUOUS_REPORT_RECEIVING_WINDOW_DELTA_US 200

#define HID_SCI_MODE_CHANGE_TIMEOUT_MS 10000

#ifdef CONFIG_BT_HOGP_SCI

BUILD_ASSERT(CONFIG_CENTRAL_HIDS_SCI_SUBRATE_MAX * (1 + CONFIG_CENTRAL_HIDS_SCI_MAX_LATENCY) <=
		     500,
	     "Core connSubrate*(latency+1) must be <= 500");
BUILD_ASSERT(CONFIG_CENTRAL_HIDS_SCI_CONTINUATION_NUM < CONFIG_CENTRAL_HIDS_SCI_SUBRATE_MAX,
	     "continuation number must be less than subrate maximum");
BUILD_ASSERT(CONFIG_CENTRAL_HIDS_SCI_SUPERVISION_TIMEOUT_10MS * 10000ULL >
		     (uint64_t)(1 + CONFIG_CENTRAL_HIDS_SCI_MAX_LATENCY) *
			     CONFIG_CENTRAL_HIDS_SCI_SUBRATE_MAX *
			     CONFIG_CENTRAL_HIDS_SCI_INTERVAL_MAX_125US * 250,
	     "supervision timeout must exceed 2*(1+latency)*subrate*interval");
#endif

static struct bt_conn *default_conn;
static struct bt_hogp hogp;
static struct bt_conn *auth_conn;
static uint8_t capslock_state;
static enum button_functions_mode button_functions_mode = BUTTON_FUNCTIONS_MODE_DEFAULT;
static enum bt_hids_sci_mode_value sci_mode_requested = BT_HIDS_SCI_MODE_NONE;

static atomic_t cont_report_rx_on = ATOMIC_INIT(0);
static uint32_t cont_report_rx_previous_report_cycles;
static uint32_t cont_report_rx_interval_us;
static uint32_t cont_report_rx_max_interval_us;
static uint32_t cont_report_rx_min_interval_us = UINT32_MAX;
static uint32_t cont_report_rx_above_window_count;
static uint32_t cont_report_rx_below_window_count;

static uint32_t report_batch_start_cycles;
static uint32_t report_cnt;

static void hids_on_ready(struct k_work *work);
static K_WORK_DEFINE(hids_ready_work, hids_on_ready);

static void sci_mode_change_timeout_fn(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(sci_mode_change_timeout, sci_mode_change_timeout_fn);

static void cont_report_rx_exit_work_fn(struct k_work *work);
K_WORK_DELAYABLE_DEFINE(cont_report_rx_exit_work,
			cont_report_rx_exit_work_fn);

static void cont_report_rx_data_reset(uint32_t batch_start_cycles)
{
	cont_report_rx_max_interval_us = 0;
	cont_report_rx_min_interval_us = UINT32_MAX;
	cont_report_rx_above_window_count = 0;
	cont_report_rx_below_window_count = 0;

	cont_report_rx_previous_report_cycles = 0;
	report_batch_start_cycles = batch_start_cycles;
	report_cnt = 0;
}

static void cont_report_rx_stats_print(uint32_t batch_end_cycles)
{
	if (report_cnt == 0) {
		return;
	}

	uint32_t avg_interval_us;
	uint32_t total_time_us =
		k_cyc_to_us_floor32(batch_end_cycles - report_batch_start_cycles);

	printk("\nReceived %" PRIu32 " reports, in %" PRIu32 " us\n", report_cnt,
	       total_time_us);

	if (total_time_us > 0) {
		printk("HID Report rate: %" PRIu32 " reports/s\n",
			(uint32_t)(report_cnt * 1000000 / total_time_us));
	}

	avg_interval_us = total_time_us / report_cnt;
	printk("Average report interval: %" PRIu32 " us, max %" PRIu32 " us, min %" PRIu32
	       " us\n",
	       avg_interval_us,
	       cont_report_rx_max_interval_us,
	       cont_report_rx_min_interval_us);

	if (cont_report_rx_interval_us > 0) {
		printk("Report interval: %" PRIu32 " us\n", cont_report_rx_interval_us);
		printk("Intervals above %" PRIu32 " us: %" PRIu32 "\n",
		       cont_report_rx_interval_us
			       + CONTINUOUS_REPORT_RECEIVING_WINDOW_DELTA_US,
		       cont_report_rx_above_window_count);
		printk("Intervals below %" PRIu32 " us: %" PRIu32 "\n",
		       cont_report_rx_interval_us
			       - CONTINUOUS_REPORT_RECEIVING_WINDOW_DELTA_US,
		       cont_report_rx_below_window_count);
	}
}

static void cont_report_rx_exit_work_fn(struct k_work *work)
{
	ARG_UNUSED(work);

	/* Lock the scheduler to prevent race conditions leading to unexpected printed results
	 * and potential errors.
	 * printk uses deferred logging, so we will spent a relatively short time locked.
	 */
	k_sched_lock();
	cont_report_rx_stats_print(cont_report_rx_previous_report_cycles);

	atomic_set(&cont_report_rx_on, 0);
	printk("\nContinuous report receiving disabled due to prolonged inactivity\n");
	k_sched_unlock();
}

static void cont_report_rx_exit_reschedule(void)
{
	(void)k_work_reschedule(&cont_report_rx_exit_work,
			       K_MSEC(CONTINUOUS_REPORT_RECEIVING_EXIT_THRESHOLD_MS));
}

static void scan_filter_match(struct bt_scan_device_info *device_info,
			      struct bt_scan_filter_match *filter_match,
			      bool connectable)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (!filter_match->uuid.match ||
	    (filter_match->uuid.count != 1)) {

		printk("Invalid device connected\n");

		return;
	}

	const struct bt_uuid *uuid = filter_match->uuid.uuid[0];

	bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));

	printk("Filters matched on UUID 0x%04x.\nAddress: %s connectable: %s\n",
		BT_UUID_16(uuid)->val,
		addr, connectable ? "yes" : "no");
}

static void scan_connecting_error(struct bt_scan_device_info *device_info)
{
	printk("Connecting failed\n");
}

static void scan_connecting(struct bt_scan_device_info *device_info,
			    struct bt_conn *conn)
{
	default_conn = bt_conn_ref(conn);
}
/** .. include_startingpoint_scan_rst */
static void scan_filter_no_match(struct bt_scan_device_info *device_info,
				 bool connectable)
{
	int err;
	struct bt_conn *conn = NULL;
	char addr[BT_ADDR_LE_STR_LEN];

	if (device_info->recv_info->adv_type == BT_GAP_ADV_TYPE_ADV_DIRECT_IND) {
		bt_addr_le_to_str(device_info->recv_info->addr, addr,
				  sizeof(addr));
		printk("Direct advertising received from %s\n", addr);
		bt_scan_stop();

		err = bt_conn_le_create(device_info->recv_info->addr,
					BT_CONN_LE_CREATE_CONN,
					device_info->conn_param, &conn);

		if (!err) {
			default_conn = bt_conn_ref(conn);
			bt_conn_unref(conn);
		}
	}
}
/** .. include_endpoint_scan_rst */
BT_SCAN_CB_INIT(scan_cb, scan_filter_match, scan_filter_no_match,
		scan_connecting_error, scan_connecting);

static void discovery_completed_cb(struct bt_gatt_dm *dm,
				   void *context)
{
	int err;

	printk("The discovery procedure succeeded\n");

	bt_gatt_dm_data_print(dm);

	err = bt_hogp_handles_assign(dm, &hogp);
	if (err) {
		printk("Could not init HIDS client object, error: %d\n", err);
	}

	err = bt_gatt_dm_data_release(dm);
	if (err) {
		printk("Could not release the discovery data, error "
		       "code: %d\n", err);
	}
}

static void discovery_service_not_found_cb(struct bt_conn *conn,
					   void *context)
{
	printk("The service could not be found during the discovery\n");
}

static void discovery_error_found_cb(struct bt_conn *conn,
				     int err,
				     void *context)
{
	printk("The discovery procedure failed with %d\n", err);
}

static const struct bt_gatt_dm_cb discovery_cb = {
	.completed = discovery_completed_cb,
	.service_not_found = discovery_service_not_found_cb,
	.error_found = discovery_error_found_cb,
};

static void gatt_discover(struct bt_conn *conn)
{
	int err;

	if (conn != default_conn) {
		return;
	}

	err = bt_gatt_dm_start(conn, BT_UUID_HIDS, &discovery_cb, NULL);
	if (err) {
		printk("could not start the discovery procedure, error "
			"code: %d\n", err);
	}
}

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	int err;
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err) {
		printk("Failed to connect to %s, 0x%02x %s\n", addr, conn_err,
		       bt_hci_err_to_str(conn_err));
		if (conn == default_conn) {
			bt_conn_unref(default_conn);
			default_conn = NULL;

			/* This demo doesn't require active scan */
			err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
			if (err) {
				printk("Scanning failed to start (err %d)\n",
				       err);
			}
		}

		return;
	}

	printk("Connected: %s\n", addr);

	if (IS_ENABLED(CONFIG_SAMPLE_BT_CENTRAL_HIDS_CONTINUOUS_REPORT_RX)
	    && (conn == default_conn)) {
		struct bt_conn_info conn_info;

		err = bt_conn_get_info(conn, &conn_info);
		if (!err) {
			printk("Connection interval: %" PRIu32 " us\n", conn_info.le.interval_us);
			/** Currently connection interval is equal to expected report interval
			 *  This will however not be the case if subrating is enabled.
			 */
			cont_report_rx_interval_us = conn_info.le.interval_us;
			printk("Expected report interval: %" PRIu32 " us\n",
			       cont_report_rx_interval_us);
		} else {
			printk("Failed to get connection interval (err %d)\n", err);
			cont_report_rx_interval_us = 0;
		}
	}

	err = bt_conn_set_security(conn, BT_SECURITY_L2);
	if (err) {
		printk("Failed to set security: %d\n", err);

		gatt_discover(conn);
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];
	int err;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (auth_conn) {
		bt_conn_unref(auth_conn);
		auth_conn = NULL;
		button_functions_mode = BUTTON_FUNCTIONS_MODE_DEFAULT;
	}

	printk("Disconnected: %s, reason 0x%02x %s\n", addr, reason, bt_hci_err_to_str(reason));

	if (bt_hogp_assign_check(&hogp)) {
		printk("HIDS client active - releasing");
		bt_hogp_release(&hogp);
	}

	if (default_conn != conn) {
		return;
	}

	if (IS_ENABLED(CONFIG_SAMPLE_BT_CENTRAL_HIDS_CONTINUOUS_REPORT_RX)) {
		atomic_set(&cont_report_rx_on, 0);
		(void) k_work_cancel_delayable(&cont_report_rx_exit_work);
		cont_report_rx_data_reset(0);
	}

	bt_conn_unref(default_conn);
	default_conn = NULL;
	if (IS_ENABLED(CONFIG_BT_HOGP_SCI) && sci_mode_requested != BT_HIDS_SCI_MODE_NONE) {
		(void) k_work_cancel_delayable(&sci_mode_change_timeout);
		sci_mode_requested = BT_HIDS_SCI_MODE_NONE;
	}

	/* This demo doesn't require active scan */
	err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
	if (err) {
		printk("Scanning failed to start (err %d)\n", err);
	}
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
		printk("Security changed: %s level %u\n", addr, level);
	} else {
		printk("Security failed: %s level %u err %d %s\n", addr, level, err,
		       bt_security_err_to_str(err));
	}

	gatt_discover(conn);
}

#ifdef CONFIG_SAMPLE_BT_CENTRAL_HIDS_CONTINUOUS_REPORT_RX
static void le_param_updated(struct bt_conn *conn, uint16_t interval,
			      uint16_t latency, uint16_t timeout)
{
	ARG_UNUSED(latency);
	ARG_UNUSED(timeout);

	if (conn != default_conn) {
		return;
	}

	if (atomic_get(&cont_report_rx_on)) {
		cont_report_rx_stats_print(cont_report_rx_previous_report_cycles);
	}

	cont_report_rx_data_reset(cont_report_rx_previous_report_cycles);
	cont_report_rx_interval_us = BT_CONN_INTERVAL_TO_US(interval);
	printk("Connection interval updated to %" PRIu32 " us\n",
	       cont_report_rx_interval_us);
}

#if CONFIG_BT_HOGP_SCI
static uint32_t conn_rate_effective_report_interval_us(uint32_t interval_us, uint16_t subrate,
						       uint16_t continuation_number)
{
	__ASSERT(subrate > 0, "Subrate must be greater than 0");

	uint32_t events_per_period = continuation_number + 1U;

	if (events_per_period > subrate) {
		events_per_period = subrate;
	}

	return (interval_us * subrate) / events_per_period;
}

static void conn_rate_changed(struct bt_conn *conn, uint8_t status,
	const struct bt_conn_le_conn_rate_changed *params)
{
	ARG_UNUSED(status);

	if (conn != default_conn) {
		return;
	}

	if (atomic_get(&cont_report_rx_on)) {
		cont_report_rx_stats_print(cont_report_rx_previous_report_cycles);
	}
	cont_report_rx_data_reset(cont_report_rx_previous_report_cycles);
	printk("Connection rate changed: interval %u us, subrate factor %u, "
	       "continuation number %u\n",
	       params->interval_us, params->subrate_factor,
	       params->continuation_number);
	cont_report_rx_interval_us = conn_rate_effective_report_interval_us(params->interval_us,
		params->subrate_factor, params->continuation_number);
	printk("Expected report interval: %u us\n",
	       cont_report_rx_interval_us);
}
#endif

#endif

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected        = connected,
	.disconnected     = disconnected,
	.security_changed = security_changed,
#ifdef CONFIG_SAMPLE_BT_CENTRAL_HIDS_CONTINUOUS_REPORT_RX
	.le_param_updated = le_param_updated,
#ifdef CONFIG_BT_HOGP_SCI
	.conn_rate_changed = conn_rate_changed,
#endif
#endif
};

static void scan_init(void)
{
	int err;

	struct bt_scan_init_param scan_init = {
		.connect_if_match = 1,
		.scan_param = NULL,
		.conn_param = BT_LE_CONN_PARAM_DEFAULT
	};

	bt_scan_init(&scan_init);
	bt_scan_cb_register(&scan_cb);

	err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID, BT_UUID_HIDS);
	if (err) {
		printk("Scanning filters cannot be set (err %d)\n", err);

		return;
	}

	err = bt_scan_filter_enable(BT_SCAN_UUID_FILTER, false);
	if (err) {
		printk("Filters cannot be turned on (err %d)\n", err);
	}
}

static bool cont_report_rx_entrance_check(uint32_t now)
{
	static uint32_t rep_cnt;
	static uint32_t batch_start_cycles;

	if (rep_cnt == 0) {
		batch_start_cycles = now;
		rep_cnt++;
		return false;
	}

	rep_cnt++;
	if (rep_cnt >= CONT_REPORT_RX_ENTRANCE_SAMPLE_REPORTS) {
		/*
		 * Note: theoretically we could hit this condition if a notification is
		 * received right after the HW timer wrapped around, but this is extremely
		 * unlikely and the consequences are negligible. No extra check keeps the
		 * code simpler.
		 */
		rep_cnt = 0;
		if (k_cyc_to_ms_floor32(now - batch_start_cycles)
		    < CONTINUOUS_REPORT_RECEIVING_ENTRANCE_THRESHOLD_MS) {
			return true;
		}
	}

	return false;
}

static void cont_report_rx_work(uint32_t now, uint32_t last)
{
	if (report_cnt == 0) {
		/* First report in the batch - nothing to do */
		report_cnt++;
		return;
	}

	report_cnt++;
	uint32_t time_elapsed = k_cyc_to_us_floor32(now - last);

	if (time_elapsed > cont_report_rx_max_interval_us) {
		cont_report_rx_max_interval_us = time_elapsed;
	}
	if (time_elapsed < cont_report_rx_min_interval_us) {
		cont_report_rx_min_interval_us = time_elapsed;
	}
	if (cont_report_rx_interval_us > 0) {
		if (time_elapsed > cont_report_rx_interval_us
					+ CONTINUOUS_REPORT_RECEIVING_WINDOW_DELTA_US) {
			cont_report_rx_above_window_count++;
		}
		if (time_elapsed < cont_report_rx_interval_us
					   - CONTINUOUS_REPORT_RECEIVING_WINDOW_DELTA_US) {
			cont_report_rx_below_window_count++;
		}
	}

	if (report_cnt >=
		CONFIG_SAMPLE_BT_CENTRAL_HIDS_CONTINUOUS_REPORT_RX_STATS_RATE) {
		cont_report_rx_stats_print(now);
		cont_report_rx_data_reset(now);
	}

	cont_report_rx_exit_reschedule();
}

static void cont_report_rx_tasks(void)
{
	uint32_t now = k_cycle_get_32();
	uint32_t last = cont_report_rx_previous_report_cycles;

	cont_report_rx_previous_report_cycles = now;

	if (atomic_get(&cont_report_rx_on)) {
		cont_report_rx_work(now, last);
	} else {
		if (cont_report_rx_entrance_check(now)) {
			printk("Continuous report receiving enabled due to"
				" short time interval between notifications.\n"
				"Expected report interval: %" PRIu32 " us\n",
				cont_report_rx_interval_us);
			cont_report_rx_data_reset(now);
			atomic_set(&cont_report_rx_on, 1);
			cont_report_rx_exit_reschedule();
		}
	}
}

static uint8_t hogp_notify_cb(struct bt_hogp *hogp,
			     struct bt_hogp_rep_info *rep,
			     uint8_t err,
			     const uint8_t *data)
{
	if (!data) {
		return BT_GATT_ITER_STOP;
	}

	if (IS_ENABLED(CONFIG_SAMPLE_BT_CENTRAL_HIDS_CONTINUOUS_REPORT_RX)) {
		cont_report_rx_tasks();

		if (atomic_get(&cont_report_rx_on)) {
			return BT_GATT_ITER_CONTINUE;
		}
	}

	uint8_t i;
	uint8_t size = bt_hogp_rep_size(rep);

	printk("Notification, id: %u, size: %u, data:",
	       bt_hogp_rep_id(rep),
	       size);
	for (i = 0; i < size; ++i) {
		printk(" 0x%x", data[i]);
	}
	printk("\n");

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t hogp_boot_mouse_report(struct bt_hogp *hogp,
				     struct bt_hogp_rep_info *rep,
				     uint8_t err,
				     const uint8_t *data)
{
	if (!data) {
		return BT_GATT_ITER_STOP;
	}

	if (IS_ENABLED(CONFIG_SAMPLE_BT_CENTRAL_HIDS_CONTINUOUS_REPORT_RX)) {
		cont_report_rx_tasks();

		if (atomic_get(&cont_report_rx_on)) {
			return BT_GATT_ITER_CONTINUE;
		}
	}

	uint8_t size = bt_hogp_rep_size(rep);
	uint8_t i;

	printk("Notification, mouse boot, size: %u, data:", size);
	for (i = 0; i < size; ++i) {
		printk(" 0x%x", data[i]);
	}
	printk("\n");
	return BT_GATT_ITER_CONTINUE;
}

static uint8_t hogp_boot_kbd_report(struct bt_hogp *hogp,
				   struct bt_hogp_rep_info *rep,
				   uint8_t err,
				   const uint8_t *data)
{
	if (!data) {
		return BT_GATT_ITER_STOP;
	}

	if (IS_ENABLED(CONFIG_SAMPLE_BT_CENTRAL_HIDS_CONTINUOUS_REPORT_RX)) {
		cont_report_rx_tasks();

		if (atomic_get(&cont_report_rx_on)) {
			return BT_GATT_ITER_CONTINUE;
		}
	}

	uint8_t size = bt_hogp_rep_size(rep);
	uint8_t i;

	printk("Notification, keyboard boot, size: %u, data:", size);
	for (i = 0; i < size; ++i) {
		printk(" 0x%x", data[i]);
	}
	printk("\n");
	return BT_GATT_ITER_CONTINUE;
}

static void hogp_ready_cb(struct bt_hogp *hogp)
{
	k_work_submit(&hids_ready_work);
}

static const char *sci_mode_to_string(enum bt_hids_sci_mode_value mode)
{
	switch (mode) {
	case BT_HIDS_SCI_MODE_NONE:
		return "NONE";
	case BT_HIDS_SCI_MODE_DEFAULT:
		return "DEFAULT";
	case BT_HIDS_SCI_MODE_FAST:
		return "FAST";
	case BT_HIDS_SCI_MODE_LOW_POWER:
		return "LOW_POWER";
	case BT_HIDS_SCI_MODE_FULL_RANGE:
		return "FULL_RANGE";
	}

	return "UNKNOWN";
}

static void sci_mode_change_timeout_fn(struct k_work *work)
{
	printk("SCI mode change timeout occurred.\n");
	sci_mode_requested = BT_HIDS_SCI_MODE_NONE;
}

static void sci_mode_notify_cb(struct bt_conn *conn, const uint8_t mode)
{
	const char *mode_str = sci_mode_to_string(mode);

	printk("SCI mode changed notification received, new mode: %s\n", mode_str);

	if (mode == sci_mode_requested) {
		(void) k_work_cancel_delayable(&sci_mode_change_timeout);
		sci_mode_requested = BT_HIDS_SCI_MODE_NONE;
	} else {
		printk("The new SCI mode does not match the requested one\n");
	}
}

static void hids_on_ready(struct k_work *work)
{
	int err;
	struct bt_hogp_rep_info *rep = NULL;

	printk("HIDS is ready to work\n");

	while (NULL != (rep = bt_hogp_rep_next(&hogp, rep))) {
		if (bt_hogp_rep_type(rep) ==
		    BT_HIDS_REPORT_TYPE_INPUT) {
			printk("Subscribe to report id: %u\n",
			       bt_hogp_rep_id(rep));
			err = bt_hogp_rep_subscribe(&hogp, rep,
							   hogp_notify_cb);
			if (err) {
				printk("Subscribe error (%d)\n", err);
			}
		}
	}
	if (hogp.rep_boot.kbd_inp) {
		printk("Subscribe to boot keyboard report\n");
		err = bt_hogp_rep_subscribe(&hogp,
						   hogp.rep_boot.kbd_inp,
						   hogp_boot_kbd_report);
		if (err) {
			printk("Subscribe error (%d)\n", err);
		}
	}
	if (hogp.rep_boot.mouse_inp) {
		printk("Subscribe to boot mouse report\n");
		err = bt_hogp_rep_subscribe(&hogp,
						   hogp.rep_boot.mouse_inp,
						   hogp_boot_mouse_report);
		if (err) {
			printk("Subscribe error (%d)\n", err);
		}
	}

	if (IS_ENABLED(CONFIG_BT_HOGP_SCI)) {
		err = bt_hogp_sci_mode_subscribe(&hogp, sci_mode_notify_cb);
		if (err) {
			printk("SCI mode subscribe error (%d)\n", err);
		}
	}
}

static void hogp_prep_fail_cb(struct bt_hogp *hogp, int err)
{
	printk("ERROR: HIDS client preparation failed!\n");
}

static void hogp_pm_update_cb(struct bt_hogp *hogp)
{
	printk("Protocol mode updated: %s\n",
	      bt_hogp_pm_get(hogp) == BT_HIDS_PM_BOOT ?
	      "BOOT" : "REPORT");
}

/* HIDS client initialization parameters */
static const struct bt_hogp_init_params hogp_init_params = {
	.ready_cb      = hogp_ready_cb,
	.prep_error_cb = hogp_prep_fail_cb,
	.pm_update_cb  = hogp_pm_update_cb
};


static void button_bootmode(void)
{
	if (!bt_hogp_ready_check(&hogp)) {
		printk("HID device not ready\n");
		return;
	}
	int err;
	enum bt_hids_pm pm = bt_hogp_pm_get(&hogp);
	enum bt_hids_pm new_pm = ((pm == BT_HIDS_PM_BOOT) ? BT_HIDS_PM_REPORT : BT_HIDS_PM_BOOT);

	printk("Setting protocol mode: %s\n", (new_pm == BT_HIDS_PM_BOOT) ? "BOOT" : "REPORT");
	err = bt_hogp_pm_write(&hogp, new_pm);
	if (err) {
		printk("Cannot change protocol mode (err %d)\n", err);
	}
}

static void hidc_write_cb(struct bt_hogp *hidc,
			  struct bt_hogp_rep_info *rep,
			  uint8_t err)
{
	printk("Caps lock sent\n");
}

static void button_capslock(void)
{
	int err;
	uint8_t data;

	if (!bt_hogp_ready_check(&hogp)) {
		printk("HID device not ready\n");
		return;
	}
	if (!hogp.rep_boot.kbd_out) {
		printk("HID device does not have Keyboard OUT report\n");
		return;
	}
	if (bt_hogp_pm_get(&hogp) != BT_HIDS_PM_BOOT) {
		printk("This function works only in BOOT Report mode\n");
		return;
	}
	capslock_state = capslock_state ? 0 : 1;
	data = capslock_state ? 0x02 : 0;
	err = bt_hogp_rep_write_wo_rsp(&hogp, hogp.rep_boot.kbd_out,
				       &data, sizeof(data),
				       hidc_write_cb);

	if (err) {
		printk("Keyboard data write error (err: %d)\n", err);
		return;
	}
	printk("Caps lock send (val: 0x%x)\n", data);
}


static uint8_t capslock_read_cb(struct bt_hogp *hogp,
			     struct bt_hogp_rep_info *rep,
			     uint8_t err,
			     const uint8_t *data)
{
	if (err) {
		printk("Capslock read error (err: %u)\n", err);
		return BT_GATT_ITER_STOP;
	}
	if (!data) {
		printk("Capslock read - no data\n");
		return BT_GATT_ITER_STOP;
	}
	printk("Received data (size: %u, data[0]: 0x%x)\n",
	       bt_hogp_rep_size(rep), data[0]);

	return BT_GATT_ITER_STOP;
}


static void capslock_write_cb(struct bt_hogp *hogp,
			      struct bt_hogp_rep_info *rep,
			      uint8_t err)
{
	int ret;

	printk("Capslock write result: %u\n", err);

	ret = bt_hogp_rep_read(hogp, rep, capslock_read_cb);
	if (ret) {
		printk("Cannot read capslock value (err: %d)\n", ret);
	}
}


static void button_capslock_rsp(void)
{
	if (!bt_hogp_ready_check(&hogp)) {
		printk("HID device not ready\n");
		return;
	}
	if (!hogp.rep_boot.kbd_out) {
		printk("HID device does not have Keyboard OUT report\n");
		return;
	}
	int err;
	uint8_t data;

	capslock_state = capslock_state ? 0 : 1;
	data = capslock_state ? 0x02 : 0;
	err = bt_hogp_rep_write(&hogp, hogp.rep_boot.kbd_out, capslock_write_cb,
				&data, sizeof(data));
	if (err) {
		printk("Keyboard data write error (err: %d)\n", err);
		return;
	}
	printk("Caps lock send using write with response (val: 0x%x)\n", data);
}


static void num_comp_reply(bool accept)
{
	if (accept) {
		bt_conn_auth_passkey_confirm(auth_conn);
		printk("Numeric Match, conn %p\n", auth_conn);
	} else {
		bt_conn_auth_cancel(auth_conn);
		printk("Numeric Reject, conn %p\n", auth_conn);
	}

	bt_conn_unref(auth_conn);
	auth_conn = NULL;
	button_functions_mode = BUTTON_FUNCTIONS_MODE_DEFAULT;
}

#ifdef CONFIG_BT_HOGP_SCI
static int set_default_conn_rate(void)
{
	int err;
	uint16_t local_min_interval_us;
	uint16_t interval_min_125us = CONFIG_CENTRAL_HIDS_SCI_INTERVAL_MIN_125US;

	err = bt_conn_le_read_min_conn_interval(&local_min_interval_us);
	if (err) {
		printk("Failed to read min conn interval (err %d)\n", err);
	}

	if (!err && interval_min_125us < local_min_interval_us / 125U) {
		printk("Configured minimum connection interval (%u) is below controller "
		       "minimum %u; using %u\n",
		       interval_min_125us, local_min_interval_us / 125U,
		       local_min_interval_us / 125U);

		interval_min_125us = local_min_interval_us / 125U;
		if (interval_min_125us > CONFIG_CENTRAL_HIDS_SCI_INTERVAL_MAX_125US) {
			printk("ERROR: controller connection interval minimum is larger "
			       "than configured maximum (%u > %u)!\n",
			       interval_min_125us, CONFIG_CENTRAL_HIDS_SCI_INTERVAL_MAX_125US);
			return -EINVAL;
		}
	}

	const struct bt_conn_le_conn_rate_param params = {
		.interval_min_125us = interval_min_125us,
		.interval_max_125us = CONFIG_CENTRAL_HIDS_SCI_INTERVAL_MAX_125US,
		.subrate_min = CONFIG_CENTRAL_HIDS_SCI_SUBRATE_MIN,
		.subrate_max = CONFIG_CENTRAL_HIDS_SCI_SUBRATE_MAX,
		.max_latency = CONFIG_CENTRAL_HIDS_SCI_MAX_LATENCY,
		.continuation_number = CONFIG_CENTRAL_HIDS_SCI_CONTINUATION_NUM,
		.supervision_timeout_10ms = CONFIG_CENTRAL_HIDS_SCI_SUPERVISION_TIMEOUT_10MS,
		.min_ce_len_125us = BT_HCI_LE_SCI_CE_LEN_MIN_125US,
		.max_ce_len_125us = BT_HCI_LE_SCI_CE_LEN_MAX_125US,
	};

	return bt_conn_le_conn_rate_set_defaults(&params);
}
#endif

static void button_sci_mode_next(void)
{
	static uint8_t sci_mode_idx;
	static const uint8_t mode_val[] = {
		BT_HIDS_SCI_MODE_DEFAULT,
		BT_HIDS_SCI_MODE_FAST,
		BT_HIDS_SCI_MODE_LOW_POWER,
		BT_HIDS_SCI_MODE_FULL_RANGE,
	};
	int err = 0;

	if (!bt_hogp_ready_check(&hogp)) {
		printk("HID device not ready\n");
		return;
	}

	if (!bt_hogp_sci_supported(&hogp)) {
		printk("HID device does not support SCI\n");
		return;
	}

	if (sci_mode_requested != BT_HIDS_SCI_MODE_NONE) {
		printk("SCI mode request already in progress!\n");
		return;
	}

	switch (mode_val[sci_mode_idx]) {
	case BT_HIDS_SCI_MODE_DEFAULT:
		printk("Sending SCI DEFAULT mode request\n");
		break;
	case BT_HIDS_SCI_MODE_FAST:
		printk("Sending SCI FAST mode request\n");
		break;
	case BT_HIDS_SCI_MODE_LOW_POWER:
		if (!bt_hogp_sci_low_power_mode_supported(&hogp)) {
			printk("The connected Device doesn't support the SCI LOW POWER mode\n");
			err = -ENOTSUP;
			break;
		}
		printk("Sending SCI LOW POWER mode request\n");
		break;
	case BT_HIDS_SCI_MODE_FULL_RANGE:
		printk("Sending SCI FULL RANGE mode request\n");
		break;
	default:
		/* Should never get here */
		__ASSERT(0, "Unknown SCI mode");
		return;
	}

	if (!err) {
		err = bt_hogp_sci_mode_req(&hogp, mode_val[sci_mode_idx]);
	}

	if (!err) {
		printk("Sent %s SCI mode request to the device\n",
		       sci_mode_to_string(mode_val[sci_mode_idx]));
		sci_mode_requested = mode_val[sci_mode_idx];
		(void) k_work_schedule(&sci_mode_change_timeout,
				       K_MSEC(HID_SCI_MODE_CHANGE_TIMEOUT_MS));
	} else {
		printk("SCI mode request failed (err: %d)\n", err);
	}

	sci_mode_idx = (sci_mode_idx + 1) % ARRAY_SIZE(mode_val);
}

static void button_authentication_mode_handler(uint32_t button)
{
	__ASSERT(auth_conn, "No authentication connection");

	if (button & KEY_PAIRING_ACCEPT) {
		num_comp_reply(true);
	} else if (button & KEY_PAIRING_REJECT) {
		num_comp_reply(false);
	}
}

static void button_default_mode_handler(uint32_t button)
{
	if (button & KEY_BOOTMODE_MASK) {
		button_bootmode();
	}
	if (button & KEY_CAPSLOCK_MASK) {
		button_capslock();
	}
	if (button & KEY_CAPSLOCK_RSP_MASK) {
		button_capslock_rsp();
	}
	if (button & KEY_ALTERNATIVE_FUNCTIONS_MASK) {
		button_functions_mode = BUTTON_FUNCTIONS_MODE_ALTERNATIVE_1;
		printk("Alternative button functions activated.\n");

		if (IS_ENABLED(CONFIG_BT_HOGP_SCI)) {
			printk("Button %d: Next SCI mode\n",
			       BUTTON_NUM_FOR_BOARD(KEY_ALTERNATIVE_1_SCI_MODE_NEXT_NUM));
		}
		printk("Button %d: Exit alternative button functions\n",
		       BUTTON_NUM_FOR_BOARD(KEY_ALTERNATIVE_FUNCTIONS_NUM));
	}
}

static void button_alternative_1_mode_handler(uint32_t button)
{
	if (IS_ENABLED(CONFIG_BT_HOGP_SCI) &&
	    (button & KEY_ALTERNATIVE_1_SCI_MODE_NEXT_MASK)) {
		button_sci_mode_next();
	} else if (button & KEY_ALTERNATIVE_FUNCTIONS_MASK) {
		button_functions_mode = BUTTON_FUNCTIONS_MODE_DEFAULT;
		printk("Alternative button functions deactivated.\n");
	}
}

static void button_handler(uint32_t button_state, uint32_t has_changed)
{
	uint32_t button = button_state & has_changed;

	switch (button_functions_mode) {
	case BUTTON_FUNCTIONS_MODE_AUTHENTICATION:
		button_authentication_mode_handler(button);
		break;
	case BUTTON_FUNCTIONS_MODE_DEFAULT:
		button_default_mode_handler(button);
		break;
	case BUTTON_FUNCTIONS_MODE_ALTERNATIVE_1:
		button_alternative_1_mode_handler(button);
		break;
	default:
		printk("Unknown button functions mode: %u\n", (unsigned int) button_functions_mode);
		button_functions_mode = BUTTON_FUNCTIONS_MODE_DEFAULT;
		break;
	}
}


static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Passkey for %s: %06u\n", addr, passkey);
}


static void auth_passkey_confirm(struct bt_conn *conn, unsigned int passkey)
{
	char addr[BT_ADDR_LE_STR_LEN];

	auth_conn = bt_conn_ref(conn);
	button_functions_mode = BUTTON_FUNCTIONS_MODE_AUTHENTICATION;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Passkey for %s: %06u\n", addr, passkey);

	if (IS_ENABLED(CONFIG_SOC_SERIES_NRF54H) || IS_ENABLED(CONFIG_SOC_SERIES_NRF54L)) {
		printk("Press Button 0 to confirm, Button 1 to reject.\n");
	} else {
		printk("Press Button 1 to confirm, Button 2 to reject.\n");
	}
}


static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing cancelled: %s\n", addr);
}


static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing completed: %s, bonded: %d\n", addr, bonded);
}


static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing failed conn: %s, reason %d %s\n", addr, reason,
	       bt_security_err_to_str(reason));
}

static struct bt_conn_auth_cb conn_auth_callbacks = {
	.passkey_display = auth_passkey_display,
	.passkey_confirm = auth_passkey_confirm,
	.cancel = auth_cancel,
};

static struct bt_conn_auth_info_cb conn_auth_info_callbacks = {
	.pairing_complete = pairing_complete,
	.pairing_failed = pairing_failed
};


int main(void)
{
	int err;

	printk("Starting Bluetooth Central HIDS sample\n");

	bt_hogp_init(&hogp, &hogp_init_params);

	err = bt_conn_auth_cb_register(&conn_auth_callbacks);
	if (err) {
		printk("failed to register authorization callbacks.\n");
		return 0;
	}

	err = bt_conn_auth_info_cb_register(&conn_auth_info_callbacks);
	if (err) {
		printk("Failed to register authorization info callbacks.\n");
		return 0;
	}

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	printk("Bluetooth initialized\n");

#ifdef CONFIG_BT_HOGP_SCI
	err = set_default_conn_rate();
	if (err) {
		printk("Failed to set the default connection rate (err %d)\n", err);
		return 0;
	}
#endif

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	scan_init();

	err = dk_buttons_init(button_handler);
	if (err) {
		printk("Failed to initialize buttons (err %d)\n", err);
		return 0;
	}

	err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
	if (err) {
		printk("Scanning failed to start (err %d)\n", err);
		return 0;
	}

	printk("Scanning successfully started\n");
	return 0;
}
