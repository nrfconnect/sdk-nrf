/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>
#include <zephyr/fs/fs.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/mgmt/callbacks.h>
#include <zephyr/mgmt/mcumgr/grp/fs_mgmt/fs_mgmt_callbacks.h>
#include <zephyr/mgmt/mcumgr/transport/smp_bt.h>
#include <bluetooth/gatt_dm.h>
#include <bluetooth/scan.h>
#include <bluetooth/services/dfu_smp.h>
#include <dk_buttons_and_leds.h>

#include <dfu/dfu_target.h>
#include <dfu/dfu_target_smp.h>
#include <dfu/dfu_target_smp_bt.h>

#include <bootutil/image.h>

LOG_MODULE_REGISTER(central_smp_client, LOG_LEVEL_INF);

#define KEY_DFU_MASK    DK_BTN1_MSK
#define KEY_UPLOAD_MASK DK_BTN2_MSK

#define RUN_STATUS_LED              DK_LED1
#define RUN_LED_BLINK_INTERVAL_MS   500U
#define CON_STATUS_LED              DK_LED2

#define LFS_MOUNT_POINT   "/lfs1"
#define UPDATE_IMAGE_PATH LFS_MOUNT_POINT "/update.bin"

#define UPLOAD_ADV_TIMEOUT_10MS 6000U /* 60 seconds */

#define DFU_THREAD_STACK_SIZE 4096
#define DFU_THREAD_PRIO       K_PRIO_PREEMPT(5)
#define DFU_UPLOAD_BUF_SIZE   4096U

BUILD_ASSERT(DFU_UPLOAD_BUF_SIZE % CONFIG_MCUMGR_GRP_IMG_UPLOAD_DATA_ALIGNMENT_SIZE == 0,
	     "Upload buffer must be a multiple of CONFIG_MCUMGR_GRP_IMG_UPLOAD_DATA_ALIGNMENT_SIZE");

/* Connection with DFU target */
static struct bt_conn *central_conn;

static struct bt_dfu_smp dfu_smp;
static struct bt_le_ext_adv *ext_adv;
static atomic_t dfu_ready = ATOMIC_INIT(0);
static atomic_t dfu_active = ATOMIC_INIT(0);
static atomic_t image_ready = ATOMIC_INIT(0);

static void advertising_start(struct k_work *work);

static K_SEM_DEFINE(dfu_start_sem, 0, 1);
static K_WORK_DEFINE(advertise_work, advertising_start);

static void conn_terminate(struct bt_conn *conn)
{
	int err;

	err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	if (err && err != -ENOTCONN) {
		LOG_ERR("Failed to disconnect (err %d)", err);
	}
}

static int scan_start(void)
{
	int err;

	err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
	if (err) {
		LOG_ERR("Scanning failed to start (err %d)", err);
	}

	return err;
}

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, SMP_BT_SVC_UUID_VAL),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME,
		sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static void adv_sent(struct bt_le_ext_adv *adv,
		     struct bt_le_ext_adv_sent_info *info)
{
	ARG_UNUSED(adv);
	ARG_UNUSED(info);

	LOG_INF("Advertiser stopped. Press the advertising button to restart advertising.");
}

static const struct bt_le_ext_adv_cb ext_adv_cb = {
	.sent = adv_sent,
};

static void advertising_start(struct k_work *work)
{
	int err;
	struct bt_le_ext_adv_start_param start_param = {
		.timeout = UPLOAD_ADV_TIMEOUT_10MS,
	};

	/* A new image must not be uploaded while the current one is being read
	 * by the DFU thread.
	 */
	if (atomic_get(&dfu_active)) {
		LOG_WRN("Cannot upload a new image while the DFU is in progress");
		return;
	}

	/* Create the advertising set on first use, then reuse it for
	 * every subsequent call.
	 */
	if (ext_adv == NULL) {
		err = bt_le_ext_adv_create(BT_LE_ADV_CONN_FAST_1, &ext_adv_cb,
					   &ext_adv);
		if (err) {
			LOG_ERR("Failed to create advertising set (err %d)",
				err);
			return;
		}

		err = bt_le_ext_adv_set_data(ext_adv, ad, ARRAY_SIZE(ad),
					     sd, ARRAY_SIZE(sd));
		if (err) {
			LOG_ERR("Failed to set advertising data (err %d)", err);
			return;
		}
	}

	err = bt_le_ext_adv_start(ext_adv, &start_param);
	if (err) {
		LOG_ERR("Advertising failed to start (err %d)", err);
		return;
	}

	LOG_INF("Connectable advertising started. Connect to upload the "
		"FW image to %s", UPDATE_IMAGE_PATH);
}

static void scan_filter_match(struct bt_scan_device_info *device_info,
			      struct bt_scan_filter_match *filter_match,
			      bool connectable)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));

	LOG_INF("Filters matched. Address: %s connectable: %s",
		addr, connectable ? "yes" : "no");
}

static void scan_connecting_error(struct bt_scan_device_info *device_info)
{
	ARG_UNUSED(device_info);

	LOG_ERR("Connecting failed");
	(void)scan_start();
}

static void scan_connecting(struct bt_scan_device_info *device_info,
			    struct bt_conn *conn)
{
	ARG_UNUSED(device_info);

	central_conn = bt_conn_ref(conn);
}

BT_SCAN_CB_INIT(scan_cb, scan_filter_match, NULL,
		scan_connecting_error, scan_connecting);

static void discovery_completed_cb(struct bt_gatt_dm *dm,
				   void *context)
{
	int err;

	ARG_UNUSED(context);

	LOG_INF("The discovery procedure succeeded");

	bt_gatt_dm_data_print(dm);

	err = bt_dfu_smp_handles_assign(dm, &dfu_smp);
	if (err) {
		LOG_ERR("Could not init DFU SMP client object, error: %d", err);
		goto release;
	}

	err = dfu_target_smp_bt_transport_register(&dfu_smp);
	if (err) {
		LOG_ERR("Failed to register SMP BT transport (err %d)", err);
		goto release;
	}

	err = dfu_target_smp_client_init();
	if (err) {
		LOG_ERR("Failed to initialize SMP DFU client (err %d)", err);
		goto release;
	}

	atomic_set(&dfu_ready, 1);
	LOG_INF("SMP DFU client ready. Press the firmware update button to start the update.");

release:
	err = bt_gatt_dm_data_release(dm);
	if (err) {
		LOG_ERR("Could not release the discovery data, error code: %d",
			err);
	}
}

static void discovery_service_not_found_cb(struct bt_conn *conn,
					   void *context)
{
	ARG_UNUSED(context);

	LOG_ERR("The service could not be found during the discovery");

	conn_terminate(conn);
}

static void discovery_error_found_cb(struct bt_conn *conn,
				     int err,
				     void *context)
{
	ARG_UNUSED(context);

	LOG_ERR("The discovery procedure failed with %d", err);

	conn_terminate(conn);
}

static const struct bt_gatt_dm_cb discovery_cb = {
	.completed = discovery_completed_cb,
	.service_not_found = discovery_service_not_found_cb,
	.error_found = discovery_error_found_cb,
};

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	int err;
	char addr[BT_ADDR_LE_STR_LEN];
	struct bt_conn_info info;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	/* A peripheral role connection is used when an MCUmgr client connects to
	 * upload the firmware update image.
	 */
	err = bt_conn_get_info(conn, &info);
	if (err) {
		LOG_ERR("Failed to get connection info (err %d)", err);
	} else if (info.role == BT_CONN_ROLE_PERIPHERAL) {
		if (conn_err) {
			LOG_ERR("Uploader connection failed to %s, 0x%02x %s",
				addr, conn_err, bt_hci_err_to_str(conn_err));
			return;
		}

		LOG_INF("Uploader connected: %s", addr);
		dk_set_led_on(CON_STATUS_LED);

		return;
	}

	if (conn_err) {
		LOG_ERR("Failed to connect to %s, 0x%02x %s", addr, conn_err,
			bt_hci_err_to_str(conn_err));

		if (conn == central_conn) {
			bt_conn_unref(central_conn);
			central_conn = NULL;
			(void)scan_start();
		}

		return;
	}

	LOG_INF("Connected: %s", addr);

	dk_set_led_on(CON_STATUS_LED);

	if (conn == central_conn) {
		err = bt_gatt_dm_start(conn, BT_UUID_DFU_SMP_SERVICE,
				       &discovery_cb, NULL);
		if (err) {
			LOG_ERR("Could not start the discovery procedure "
				"(err %d)", err);
			conn_terminate(conn);
		}
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Disconnected: %s, reason 0x%02x %s", addr, reason,
		bt_hci_err_to_str(reason));

	dk_set_led_off(CON_STATUS_LED);

	if (conn != central_conn) {
		/* An uploader (peripheral role) connection was terminated. */
		return;
	}

	atomic_set(&dfu_ready, 0);

	bt_conn_unref(central_conn);
	central_conn = NULL;

	(void)scan_start();
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static int scan_init(void)
{
	int err;

	struct bt_scan_init_param init_param = {
		.connect_if_match = 1,
		.scan_param = NULL,
		.conn_param = BT_LE_CONN_PARAM_DEFAULT
	};

	bt_scan_init(&init_param);
	bt_scan_cb_register(&scan_cb);

	err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID,
				 BT_UUID_DFU_SMP_SERVICE);
	if (err) {
		LOG_ERR("Scanning filters cannot be set (err %d)", err);
		return err;
	}

	err = bt_scan_filter_enable(BT_SCAN_UUID_FILTER, false);
	if (err) {
		LOG_ERR("Filters cannot be turned on (err %d)", err);
		return err;
	}

	return 0;
}

static void dfu_smp_on_error(struct bt_dfu_smp *dfu_smp, int err)
{
	LOG_ERR("DFU SMP generic error: %d", err);
}

static const struct bt_dfu_smp_init_params init_params = {
	.error_cb = dfu_smp_on_error
};

static void dfu_target_evt_handler(enum dfu_target_evt_id evt_id)
{
	LOG_INF("DFU target event: %d", evt_id);
}

/* The mcumgr file management group gives a connected client access to every
 * file in the file system. This hook narrows the access down to the update
 * image. It also denies access while the image is being read by the DFU thread.
 */
static enum mgmt_cb_return fs_access_cb(uint32_t event,
					enum mgmt_cb_return prev_status,
					int32_t *rc, uint16_t *group,
					bool *abort_more, void *data,
					size_t data_size)
{
	const struct fs_mgmt_file_access *access = data;

	ARG_UNUSED(prev_status);
	ARG_UNUSED(group);
	ARG_UNUSED(abort_more);

	if (access == NULL || data_size != sizeof(*access)) {
		*rc = MGMT_ERR_EINVAL;
		return MGMT_CB_ERROR_RC;
	}

	if (strcmp(access->filename, UPDATE_IMAGE_PATH) != 0) {
		LOG_WRN("Denied access to %s: only %s is accessible",
			access->filename, UPDATE_IMAGE_PATH);
		*rc = MGMT_ERR_EACCESSDENIED;
		return MGMT_CB_ERROR_RC;
	}

	if (event == MGMT_EVT_OP_FS_MGMT_FILE_ACCESS_DONE) {
		if (access->access == FS_MGMT_FILE_ACCESS_WRITE) {
			atomic_set(&image_ready, 1);
			LOG_INF("Update image uploaded. Press the firmware "
				"update button to start the update.");
		}

		return MGMT_CB_OK;
	}

	if (atomic_get(&dfu_active)) {
		LOG_WRN("Denied access to %s while the DFU is in progress",
			UPDATE_IMAGE_PATH);
		*rc = MGMT_ERR_EACCESSDENIED;
		return MGMT_CB_ERROR_RC;
	}

	/* The image being overwritten is incomplete until the upload finishes. */
	if (access->access == FS_MGMT_FILE_ACCESS_WRITE) {
		atomic_set(&image_ready, 0);
	}

	return MGMT_CB_OK;
}

static struct mgmt_callback fs_access_callback = {
	.callback = fs_access_cb,
	.event_id = MGMT_EVT_OP_FS_MGMT_ALL,
};

/* An update image that was uploaded before a reset is complete and can be used
 * without uploading it again.
 */
static void image_ready_init(void)
{
	struct fs_dirent entry;

	if (fs_stat(UPDATE_IMAGE_PATH, &entry) != 0) {
		return;
	}

	LOG_INF("Found a stored update image at %s", UPDATE_IMAGE_PATH);
	atomic_set(&image_ready, 1);
}

/* Disconnect any peripheral connections to prevent file uploads to LittleFS during DFU. */
static void disconnect_uploader(struct bt_conn *conn, void *data)
{
	struct bt_conn_info info;

	ARG_UNUSED(data);

	if (bt_conn_get_info(conn, &info) != 0) {
		return;
	}

	if (info.role != BT_CONN_ROLE_PERIPHERAL ||
	    info.state != BT_CONN_STATE_CONNECTED) {
		return;
	}

	LOG_INF("Disconnecting peripheral connection to prevent file uploads during DFU");
	conn_terminate(conn);
}

/* Stop any active advertiser and terminate any active peripheral connections. */
static void prevent_file_uploads(void)
{
	int err;

	if (ext_adv != NULL) {
		err = bt_le_ext_adv_stop(ext_adv);
		if (err && err != -EALREADY) {
			LOG_ERR("Failed to stop advertising (err %d)", err);
		}
	}

	bt_conn_foreach(BT_CONN_TYPE_LE, disconnect_uploader, NULL);
}

/* Determine the size of the update image stored in the LittleFS partition
 * and check that it starts with a valid MCUboot image header.
 */
static int get_image_size(struct fs_file_t *file, size_t *size)
{
	int err;
	ssize_t rd;
	struct fs_dirent entry;
	struct image_header hdr;

	err = fs_stat(UPDATE_IMAGE_PATH, &entry);
	if (err) {
		LOG_ERR("Failed to stat %s (err %d)", UPDATE_IMAGE_PATH, err);
		return err;
	}

	if (entry.size < sizeof(hdr)) {
		LOG_ERR("Update image file too small (%zu bytes)",
			(size_t)entry.size);
		return -EINVAL;
	}

	rd = fs_read(file, &hdr, sizeof(hdr));
	if (rd < 0) {
		LOG_ERR("Failed to read image header (err %d)", (int)rd);
		return (int)rd;
	}

	if (!dfu_target_smp_identify(&hdr)) {
		LOG_ERR("No valid MCUboot image found in %s", UPDATE_IMAGE_PATH);
		return -EINVAL;
	}

	/* Reset the read offset */
	err = fs_seek(file, 0, FS_SEEK_SET);
	if (err) {
		LOG_ERR("Failed to rewind %s (err %d)", UPDATE_IMAGE_PATH, err);
		return err;
	}

	*size = (size_t)entry.size;

	return 0;
}

static int do_dfu(void)
{
	int err;
	struct fs_file_t file;
	/* Static to avoid placing the buffer on the thread stack. */
	static uint8_t dfu_read_buf[DFU_UPLOAD_BUF_SIZE];
	size_t image_size;
	size_t offset = 0;

	if (!atomic_get(&dfu_ready)) {
		LOG_ERR("The SMP DFU client is not ready");
		return -ENOTCONN;
	}

	prevent_file_uploads();

	fs_file_t_init(&file);

	err = fs_open(&file, UPDATE_IMAGE_PATH, FS_O_READ);
	if (err) {
		LOG_ERR("Failed to open %s (err %d)", UPDATE_IMAGE_PATH, err);
		return err;
	}

	err = get_image_size(&file, &image_size);
	if (err) {
		goto close;
	}

	LOG_INF("Update image size: %zu bytes", image_size);

	err = dfu_target_init(DFU_TARGET_IMAGE_TYPE_SMP, 0, image_size,
			      dfu_target_evt_handler);
	if (err) {
		LOG_ERR("dfu_target_init failed (err %d)", err);
		goto close;
	}

	while (offset < image_size) {
		size_t chunk = MIN((size_t)DFU_UPLOAD_BUF_SIZE, image_size - offset);
		ssize_t rd;

		if (!atomic_get(&dfu_ready)) {
			LOG_ERR("The DFU target disconnected at offset %zu",
				offset);
			err = -ECONNRESET;
			goto abort;
		}

		rd = fs_read(&file, dfu_read_buf, chunk);
		if (rd < 0) {
			LOG_ERR("File read failed at offset %zu (err %d)",
				offset, (int)rd);
			err = (int)rd;
			goto abort;
		}

		err = dfu_target_write(dfu_read_buf, chunk);
		if (err) {
			LOG_ERR("dfu_target_write failed at offset %zu (err %d)",
				offset, err);
			goto abort;
		}

		offset += chunk;
		LOG_INF("Uploaded %zu / %zu bytes", offset, image_size);
	}

	err = dfu_target_done(true);
	if (err) {
		LOG_ERR("dfu_target_done failed (err %d)", err);
		goto close;
	}

	/* The image is marked as a "test" image by the SMP backend, which means the
	 * target must confirm the image to make the update permanent. If it is not
	 * confirmed, the target's bootloader will revert to the previous FW image on
	 * the next reboot.
	 */
	err = dfu_target_schedule_update(0);
	if (err) {
		LOG_ERR("dfu_target_schedule_update failed (err %d)", err);
		goto close;
	}

	err = dfu_target_smp_reboot();
	if (err) {
		LOG_ERR("Failed to reboot the target (err %d)", err);
		goto close;
	}

	err = fs_close(&file);
	if (err) {
		LOG_WRN("Failed to close %s", UPDATE_IMAGE_PATH);
		return err;
	}

	return 0;

abort:
	(void)dfu_target_done(false);
close:
	if (fs_close(&file)) {
		LOG_WRN("Failed to close %s", UPDATE_IMAGE_PATH);
	}

	return err;
}

static void dfu_thread_fn(void *arg1, void *arg2, void *arg3)
{
	int err;

	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	while (true) {
		k_sem_take(&dfu_start_sem, K_FOREVER);
		LOG_INF("Starting DFU of the remote target over BLE SMP");

		err = do_dfu();
		if (err) {
			LOG_ERR("DFU failed (err %d)", err);
		} else {
			LOG_INF("DFU completed. The target will reboot to "
				"apply the update.");
		}

		atomic_set(&dfu_active, 0);
	}
}

K_THREAD_DEFINE(dfu_thread_id, DFU_THREAD_STACK_SIZE, dfu_thread_fn, NULL, NULL,
		NULL, DFU_THREAD_PRIO, 0, 0);

static void button_handler(uint32_t button_state, uint32_t has_changed)
{
	if ((has_changed & KEY_DFU_MASK) && (button_state & KEY_DFU_MASK)) {
		if (!atomic_get(&dfu_ready)) {
			LOG_WRN("SMP DFU client not ready: connect and complete "
				"discovery first");
		} else if (!atomic_get(&image_ready)) {
			LOG_WRN("No update image available: press the "
				"advertising button and upload %s",
				UPDATE_IMAGE_PATH);
		} else if (!atomic_cas(&dfu_active, 0, 1)) {
			LOG_WRN("A DFU is already in progress");
		} else {
			k_sem_give(&dfu_start_sem);
		}
	}

	if ((has_changed & KEY_UPLOAD_MASK) && (button_state & KEY_UPLOAD_MASK)) {
		k_work_submit(&advertise_work);
	}
}

int main(void)
{
	int err;
	int blink_status = 0;

	LOG_INF("Starting Bluetooth Central SMP Client sample");

	err = dk_leds_init();
	if (err) {
		LOG_ERR("Failed to initialize LEDs (err %d)", err);
		return err;
	}

	err = dk_buttons_init(button_handler);
	if (err) {
		LOG_ERR("Failed to initialize buttons (err %d)", err);
		return err;
	}

	err = bt_dfu_smp_init(&dfu_smp, &init_params);
	if (err) {
		LOG_ERR("Failed to initialize DFU SMP client (err %d)", err);
		return err;
	}

	mgmt_callback_register(&fs_access_callback);

	/* Check if a FW update image is available in LittleFS */
	image_ready_init();

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return err;
	}

	LOG_INF("Bluetooth initialized");

	err = scan_init();
	if (err) {
		LOG_ERR("Failed to initialize scanning (err %d)", err);
		return err;
	}

	err = scan_start();
	if (err) {
		return err;
	}

	LOG_INF("Scanning successfully started");

	while (true) {
		dk_set_led(RUN_STATUS_LED, (++blink_status) % 2);
		k_msleep(RUN_LED_BLINK_INTERVAL_MS);
	}
}
