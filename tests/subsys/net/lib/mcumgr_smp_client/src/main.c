/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>
#include <mcumgr_smp_client.h>
#include <dfu/dfu_target_smp.h>
#include "fota_download_util.h"

static struct fota_download_evt fota_cb_event;
static fota_download_callback_t client_test_callback;
static bool scheduled_called;
static bool reboot_called;
static bool dfu_target_done_success;
static bool dfu_target_image_confirmed;
struct mcumgr_image_data image_list[2];

/* Stubbed Functions start */
int dfu_target_smp_recovery_mode_enable(dfu_target_reset_cb_t cb)
{
	return 0;
}

int dfu_target_smp_client_init(void)
{
	return 0;
}

bool nrfx_is_in_ram(char *const file)
{
	return true;
}

int dfu_target_init(int img_type, int img_num, size_t file_size, dfu_target_callback_t cb)
{
	return 0;
}

void fota_download_test_callback(const struct fota_download_evt *evt)
{
	fota_cb_event.id = evt->id;
	if (evt->id == FOTA_DOWNLOAD_EVT_ERROR) {
		fota_cb_event.cause = evt->cause;
	} else if (evt->id == FOTA_DOWNLOAD_EVT_PROGRESS) {
		fota_cb_event.progress = evt->progress;
	}

	printf("Fota CB %d\r\n", fota_cb_event.id);
}

int fota_download_start_with_image_type(const char *host, const char *file, int sec_tag,
					uint8_t pdn_id, size_t fragment_size,
					const enum dfu_target_image_type expected_type)
{

	return 0;
}

int fota_download_util_client_init(fota_download_callback_t client_callback, bool smp_image_type)
{
	client_test_callback = client_callback;
	return 0;
}

int fota_download_cancel(void)
{
	struct fota_download_evt fota_event;

	if (client_test_callback) {
		fota_event.id = FOTA_DOWNLOAD_EVT_CANCELLED;
		client_test_callback(&fota_event);
	}

	return 0;
}

int dfu_target_schedule_update(int img_num)
{
	scheduled_called = true;
	image_list[1].flags.pending = true;
	return 0;
}

int dfu_target_smp_reboot(void)
{
	reboot_called = true;
	return 0;
}

int dfu_target_smp_done(bool succesufully)
{
	dfu_target_done_success = succesufully;
	return 0;
}

int dfu_target_reset(void)
{
	return dfu_target_smp_done(false);
}

int dfu_target_smp_confirm_image(void)
{
	dfu_target_image_confirmed = true;
	image_list[0].flags.confirmed = true;
	return 0;
}

int dfu_target_reset_smp_test_cb(void)
{
	return 0;
}

int dfu_target_smp_image_list_get(struct mcumgr_image_state *res_buf)
{
	res_buf->status = MGMT_ERR_EOK;
	res_buf->image_list_length = 2;
	res_buf->image_list = image_list;
	return 0;
}

/* Stubbed Functions end */
void *mcumgr_smp_client_setup(void)
{
	int rc;
	/* Validate that Init is working */
	rc = mcumgr_smp_client_init(NULL);
	zassert_equal(rc, 0, "Init fail");
	return NULL;
}

ZTEST(mcumgr_smp_client_test, test_download)
{
	char test_url[] = "https://test_server.com/test.bin";
	struct fota_download_evt fota_event;
	int rc;

	rc = mcumgr_smp_client_download_start(test_url, 0, fota_download_test_callback);
	zassert_equal(rc, 0, "Download start fail");
	/* Test start a new which should fail */
	rc = mcumgr_smp_client_download_start(test_url, 0, fota_download_test_callback);
	zassert_equal(rc, -EBUSY, "Download start fail");
	/* Cancel started */
	rc = mcumgr_smp_client_download_cancel();
	zassert_equal(rc, 0, "Download cancel fail");
	zassert_equal(fota_cb_event.id, FOTA_DOWNLOAD_EVT_CANCELLED, "Cancel fail");

	/* Test Fota Callback handling */
	rc = mcumgr_smp_client_download_start(test_url, 0, fota_download_test_callback);
	zassert_equal(rc, 0, "Download start fail");

	/* Test Progress  */
	fota_event.id = FOTA_DOWNLOAD_EVT_PROGRESS;
	fota_event.progress = 10;
	client_test_callback(&fota_event);
	zassert_equal(fota_cb_event.id, FOTA_DOWNLOAD_EVT_PROGRESS, "Progress fail");
	zassert_equal(fota_cb_event.progress, 10, "No matching progress number");
	/* Test Fota cancel */
	rc = mcumgr_smp_client_download_cancel();
	zassert_equal(rc, 0, "Download cancel fail");

	/* Test Error handling */
	rc = mcumgr_smp_client_download_start(test_url, 0, fota_download_test_callback);
	zassert_equal(rc, 0, "Download start fail");
	fota_event.id = FOTA_DOWNLOAD_EVT_ERROR;
	fota_event.cause = FOTA_DOWNLOAD_ERROR_CAUSE_TYPE_MISMATCH;
	client_test_callback(&fota_event);
	zassert_equal(fota_cb_event.id, FOTA_DOWNLOAD_EVT_ERROR, "Progress fail");
	zassert_equal(fota_cb_event.cause, FOTA_DOWNLOAD_ERROR_CAUSE_TYPE_MISMATCH,
		      "Download start fail");

	/* Test finished download */
	rc = mcumgr_smp_client_download_start(test_url, 0, fota_download_test_callback);
	zassert_equal(rc, 0, "Download start fail");
	fota_event.id = FOTA_DOWNLOAD_EVT_FINISHED;
	client_test_callback(&fota_event);
	zassert_equal(fota_cb_event.id, FOTA_DOWNLOAD_EVT_FINISHED, "Finished fail");

	/* Test cancel download when it is already finished */
	rc = mcumgr_smp_client_download_cancel();
	zassert_equal(rc, -EACCES, "Not correct return value for cancel");
}

ZTEST(mcumgr_smp_client_test, test_schedule_image)
{
	int rc;
	struct fota_download_evt fota_event;
	char test_url[] = "https://test_server.com/test.bin";
	struct mcumgr_image_state res_buf;

	rc = mcumgr_smp_client_download_start(test_url, 0, fota_download_test_callback);
	zassert_equal(rc, 0, "Download start fail");
	fota_event.id = FOTA_DOWNLOAD_EVT_FINISHED;
	client_test_callback(&fota_event);
	zassert_equal(fota_cb_event.id, FOTA_DOWNLOAD_EVT_FINISHED, "Finished fail");

	scheduled_called = false;
	image_list[1].flags.pending = false;
	rc = mcumgr_smp_client_update();
	zassert_equal(rc, 0, "Image shedule fail");
	zassert_equal(scheduled_called, true, "schedule not called");
	rc = mcumgr_smp_client_read_list(&res_buf);
	zassert_equal(rc, 0, "Image read list fail");
	zassert_equal(res_buf.image_list[1].flags.pending, true, "Image read list fail");
}

ZTEST(mcumgr_smp_client_test, test_reset)
{
	int rc;

	reboot_called = false;
	rc = mcumgr_smp_client_reset();
	zassert_equal(rc, 0, "Reset fail");
	zassert_equal(reboot_called, true, "Reboot not called");
}

ZTEST(mcumgr_smp_client_test, test_erase_and_confirm)
{
	int rc;
	struct mcumgr_image_state res_buf;

	/* Test to erase by dfu_target_done(false) */
	dfu_target_done_success = true;
	rc = mcumgr_smp_client_erase();
	zassert_equal(rc, 0, "ERASE fail");
	zassert_equal(dfu_target_done_success, false, "DFU erase not done");

	/* Test confirmattion */
	dfu_target_image_confirmed = false;
	image_list[0].flags.confirmed = false;
	rc = mcumgr_smp_client_confirm_image();
	zassert_equal(rc, 0, "Confirm fail");
	zassert_equal(dfu_target_image_confirmed, true, "Image confirm not done");
	rc = mcumgr_smp_client_read_list(&res_buf);
	zassert_equal(rc, 0, "Image read list fail");
	zassert_equal(res_buf.image_list[0].flags.confirmed, true, "Image read list fail");
	/* Test earse outside by fota_download_util_image_reset() */
	dfu_target_done_success = true;
	rc = fota_download_util_image_reset(DFU_TARGET_IMAGE_TYPE_SMP);
	zassert_equal(rc, 0, "ERASE fail");
	zassert_equal(dfu_target_done_success, false, "DFU erase not done");
	/* Test Erase failure by recovery mode enabled */
	rc = mcumgr_smp_client_init(dfu_target_reset_smp_test_cb);
	zassert_equal(rc, 0, "Init fail");
	dfu_target_done_success = true;
	rc = mcumgr_smp_client_erase();
	zassert_equal(rc, -ENOTSUP, "ERASE fail");
	zassert_not_equal(dfu_target_done_success, false, "DFU erase done");
}

static void cleanup_test(void *p)
{
	/* init to idle state */
	mcumgr_smp_client_init(NULL);
}

ZTEST_SUITE(mcumgr_smp_client_test, NULL, mcumgr_smp_client_setup, NULL, cleanup_test, NULL);
