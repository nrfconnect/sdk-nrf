/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/ztest.h>
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/smp/smp_client.h>
#include <zephyr/mgmt/mcumgr/grp/img_mgmt/img_mgmt_client.h>
#include <zephyr/mgmt/mcumgr/grp/os_mgmt/os_mgmt.h>
#include <zephyr/mgmt/mcumgr/grp/os_mgmt/os_mgmt_client.h>
#include <dfu/dfu_target.h>
#include <dfu/dfu_target_smp.h>
#include <zephyr/devicetree.h>
#include "smp_stub.h"
#include "os_gr_stub.h"
#include "img_gr_stub.h"

static uint8_t image_hash[32];
static uint8_t image_dummy[1024];

static int reset_cb_response = -1;

int dfu_target_reset_call(void)
{
	return reset_cb_response;
}

ZTEST(dfu_target_smp, test_reboot)
{
	int rc;

	rc = dfu_target_smp_reboot();

	/* Test timeout */
	rc = dfu_target_smp_reboot();
	zassert_equal(MGMT_ERR_ETIMEOUT, rc, "Expected to receive %d response %d",
		      MGMT_ERR_ETIMEOUT, rc);
	/* Testing reset successfully handling */
	os_reset_response();
	rc = dfu_target_smp_reboot();
	zassert_equal(MGMT_ERR_EOK, rc, "Expected to receive %d response %d", MGMT_ERR_EOK, rc);
}

ZTEST(dfu_target_smp, test_reset)
{
	int rc;
	size_t offset;

	rc = dfu_target_smp_init(TEST_IMAGE_SIZE, TEST_IMAGE_NUM, NULL);
	zassert_equal(rc, MGMT_ERR_EOK, "DFU target Init fail");

	/* Allocate response buf */
	img_upload_response(1024, MGMT_ERR_EOK);
	rc = dfu_target_smp_write(image_dummy, 1024);
	zassert_equal(MGMT_ERR_EOK, rc, "Expected to receive %d response %d", MGMT_ERR_EOK, rc);
	rc = dfu_target_smp_offset_get(&offset);
	zassert_equal(MGMT_ERR_EOK, rc, "Expected to receive %d response %d", MGMT_ERR_EOK, rc);
	zassert_equal(1024, offset, "Expected to receive offset %d response %d", 1024, offset);
	img_erase_response(MGMT_ERR_EOK);
	rc = dfu_target_smp_reset();
	zassert_equal(MGMT_ERR_EOK, rc, "Expected to receive %d response %d", MGMT_ERR_EOK, rc);
	rc = dfu_target_smp_offset_get(&offset);
	zassert_equal(MGMT_ERR_EOK, rc, "Expected to receive %d response %d", MGMT_ERR_EOK, rc);
	zassert_equal(0, offset, "Expected to receive offset %d response %d", 0, offset);
}

ZTEST(dfu_target_smp, test_image_schedule)
{
	int rc;
	struct mcumgr_image_state res_buf;

	/* Force list size to 1 */
	img_read_response(1);
	rc = dfu_target_smp_image_list_get(&res_buf);
	zassert_equal(MGMT_ERR_EOK, rc, "Expected to receive %d response %d", MGMT_ERR_EOK, rc);

	rc = dfu_target_smp_schedule_update(1);
	zassert_equal(rc, -ENOENT, "DFU target Init fail: %d response %d", -ENOENT, rc);

	img_read_response(2);
	rc = dfu_target_smp_image_list_get(&res_buf);
	zassert_equal(MGMT_ERR_EOK, rc, "Expected to receive %d response %d", MGMT_ERR_EOK, rc);

	smp_stub_set_rx_data_verify(img_state_write_verify);
	rc = dfu_target_smp_schedule_update(1);
	zassert_equal(rc, MGMT_ERR_EOK, "DFU target Init fail: %d response %d", MGMT_ERR_EOK, rc);
	img_read_response(2);
	rc = dfu_target_smp_image_list_get(&res_buf);
	zassert_equal(MGMT_ERR_EOK, rc, "Expected to receive %d response %d", MGMT_ERR_EOK, rc);
	zassert_equal(2, res_buf.image_list_length, "Expected to receive %d response %d", 2,
		      res_buf.image_list_length);
	/* Test Pending status */
	zassert_equal(true, res_buf.image_list[1].flags.pending,
		      "Expected to receive %d response %d", true,
		      res_buf.image_list[1].flags.pending);
}

ZTEST(dfu_target_smp, test_image_confirm)
{
	int rc;
	struct mcumgr_image_state res_buf;

	img_confirmation_bit_clear();
	img_read_response(2);
	rc = dfu_target_smp_image_list_get(&res_buf);
	zassert_equal(MGMT_ERR_EOK, rc, "Expected to receive %d response %d", MGMT_ERR_EOK, rc);
	zassert_equal(2, res_buf.image_list_length, "Expected to receive %d response %d", 2,
		      res_buf.image_list_length);
	zassert_equal(false, res_buf.image_list[0].flags.confirmed,
		      "Expected to receive %d response %d", false,
		      res_buf.image_list[0].flags.confirmed);

	smp_stub_set_rx_data_verify(img_state_write_verify);
	rc = dfu_target_smp_confirm_image();
	zassert_equal(MGMT_ERR_EOK, rc, "Expected to receive %d response %d", MGMT_ERR_EOK, rc);

	img_read_response(2);
	rc = dfu_target_smp_image_list_get(&res_buf);
	zassert_equal(2, res_buf.image_list_length, "Expected to receive %d response %d", 2,
		      res_buf.image_list_length);
	zassert_equal(true, res_buf.image_list[0].flags.confirmed,
		      "Expected to receive %d response %d", true,
		      res_buf.image_list[0].flags.confirmed);
}

ZTEST(dfu_target_smp, test_image_upload)
{
	int rc;
	size_t offset;

	rc = dfu_target_smp_init(TEST_IMAGE_SIZE, -1, NULL);
	zassert_equal(rc, -ENOENT, "Expected to receive %d response %d", -ENOENT, rc);

	rc = dfu_target_smp_init(TEST_IMAGE_SIZE, TEST_IMAGE_NUM, NULL);
	zassert_equal(rc, MGMT_ERR_EOK, "DFU target Init fail");

	/* Start upload  and test Timeout */
	rc = dfu_target_smp_write(image_dummy, 1024);
	zassert_equal(MGMT_ERR_ETIMEOUT, rc, "Expected to receive %d response %d",
		      MGMT_ERR_ETIMEOUT, rc);

	rc = dfu_target_smp_init(TEST_IMAGE_SIZE, TEST_IMAGE_NUM, NULL);
	zassert_equal(rc, MGMT_ERR_EOK, "DFU target Init fail");
	smp_client_send_status_stub(MGMT_ERR_EOK);

	/* Allocate response buf */
	img_upload_response(0, MGMT_ERR_EINVAL);
	rc = dfu_target_smp_write(image_dummy, 1024);
	zassert_equal(MGMT_ERR_EINVAL, rc, "Expected to receive %d response %d", MGMT_ERR_EINVAL,
		      rc);
	dfu_target_smp_done(false);

	rc = dfu_target_smp_init(TEST_IMAGE_SIZE, TEST_IMAGE_NUM, NULL);
	zassert_equal(rc, MGMT_ERR_EOK, "DFU target Init fail");

	img_upload_response(1024, MGMT_ERR_EOK);
	smp_stub_set_rx_data_verify(img_upload_init_verify);
	img_upload_stub_init();

	rc = dfu_target_smp_write(image_dummy, 1024);
	zassert_equal(MGMT_ERR_EOK, rc, "Expected to receive %d response %d", MGMT_ERR_EOK, rc);
	rc = dfu_target_smp_offset_get(&offset);
	zassert_equal(MGMT_ERR_EOK, rc, "Expected to receive %d response %d", MGMT_ERR_EOK, rc);
	zassert_equal(1024, offset, "Expected to receive offset %d response %d", 1024, offset);

	/* Send last frame from image */
	rc = dfu_target_smp_write(image_dummy, 1024);
	zassert_equal(MGMT_ERR_EOK, rc, "Expected to receive %d response %d", MGMT_ERR_EOK, rc);
	rc = dfu_target_smp_offset_get(&offset);
	zassert_equal(TEST_IMAGE_SIZE, offset, "Expected to receive offset %d response %d",
		      TEST_IMAGE_SIZE, offset);

	smp_stub_set_rx_data_verify(NULL);
	img_read_response(2);
	rc = dfu_target_smp_done(true);
	zassert_equal(rc, MGMT_ERR_EOK, "DFU target Init fail");
}

ZTEST(dfu_target_smp, test_list_read)
{
	int ret;
	struct mcumgr_image_state res_buf;

	/* Test Timeout*/
	smp_client_send_status_stub(MGMT_ERR_EOK);
	ret = dfu_target_smp_image_list_get(&res_buf);
	zassert_equal(MGMT_ERR_ETIMEOUT, ret, "Expected to receive %d response %d",
		      MGMT_ERR_ETIMEOUT, ret);

	/* Testing read successfully 1 image info and print that */
	img_read_response(1);
	ret = dfu_target_smp_image_list_get(&res_buf);
	zassert_equal(MGMT_ERR_EOK, ret, "Expected to receive %d response %d", MGMT_ERR_EOK, ret);
	zassert_equal(1, res_buf.image_list_length, "Expected to receive %d response %d", 1,
		      res_buf.image_list_length);
	img_read_response(2);
	ret = dfu_target_smp_image_list_get(&res_buf);
	zassert_equal(MGMT_ERR_EOK, ret, "Expected to receive %d response %d", MGMT_ERR_EOK, ret);
	zassert_equal(2, res_buf.image_list_length, "Expected to receive %d response %d", 2,
		      res_buf.image_list_length);

	/* Test Failing Recovery mode*/
	dfu_target_smp_recovery_mode_enable(dfu_target_reset_call);
	ret = dfu_target_smp_image_list_get(&res_buf);
	zassert_equal(-2, ret, "Expected to receive %d response %d", -2, ret);

	reset_cb_response = 0;
	img_read_response(2);
	ret = dfu_target_smp_image_list_get(&res_buf);
	zassert_equal(MGMT_ERR_EOK, ret, "Expected to receive %d response %d", MGMT_ERR_EOK, ret);
	zassert_equal(2, res_buf.image_list_length, "Expected to receive %d response %d", 2,
		      res_buf.image_list_length);
}

void *dfu_target_smp_setup(void)
{
	int ret;

	ret = dfu_target_smp_client_init();
	zassert_equal(ret, MGMT_ERR_EINVAL, "Init fail");
	stub_smp_client_transport_register();

	ret = dfu_target_smp_client_init();
	zassert_equal(ret, MGMT_ERR_EOK, "Init fail");

	img_gr_stub_data_init(image_hash);
	smp_stub_set_rx_data_verify(NULL);
	smp_client_send_status_stub(MGMT_ERR_EOK);
	printf("DFU target SMP init OK and stubbed\r\n");
	return NULL;
}

static void cleanup_test(void *p)
{
	smp_client_response_buf_clean();
	smp_stub_set_rx_data_verify(NULL);
	smp_client_send_status_stub(MGMT_ERR_EOK);
	dfu_target_smp_recovery_mode_enable(NULL);
	img_gr_stub_data_init(image_hash);
}

ZTEST_SUITE(dfu_target_smp, NULL, dfu_target_smp_setup, NULL, cleanup_test, NULL);
