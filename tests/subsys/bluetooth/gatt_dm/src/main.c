/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <ztest.h>
#include <kernel.h>
#include <stddef.h>
#include <misc/util.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt_dm.h>
#include "../mock/gatt_discover_mock.h"

/* Timeout for the discovery in ms */
#define SERVICE_DISCOVERY_TIMEOUT 2000

static char dummy_conn;
K_SEM_DEFINE(discovery_finished, 0, 1);


const struct bt_gatt_attr discover_sim[] = {
	/* HIDS */
	BT_GATT_DISCOVER_MOCK_SERV(1, BT_UUID_HIDS, 11),
	BT_GATT_DISCOVER_MOCK_CHRC(2, BT_UUID_HIDS_INFO, BT_GATT_CHRC_READ),
	BT_GATT_DISCOVER_MOCK_DESC(3, BT_UUID_HIDS_INFO),

	BT_GATT_DISCOVER_MOCK_CHRC(4, BT_UUID_HIDS_REPORT_MAP, BT_GATT_CHRC_READ),
	BT_GATT_DISCOVER_MOCK_DESC(5, BT_UUID_HIDS_REPORT_MAP),

	BT_GATT_DISCOVER_MOCK_CHRC(6, BT_UUID_HIDS_REPORT, BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY),
	BT_GATT_DISCOVER_MOCK_DESC(7, BT_UUID_HIDS_REPORT),
	BT_GATT_DISCOVER_MOCK_DESC(8, BT_UUID_GATT_CCC),
	BT_GATT_DISCOVER_MOCK_DESC(9, BT_UUID_HIDS_REPORT_REF),

	BT_GATT_DISCOVER_MOCK_CHRC(10, BT_UUID_HIDS_CTRL_POINT, BT_GATT_CHRC_WRITE_WITHOUT_RESP),
	BT_GATT_DISCOVER_MOCK_DESC(11, BT_UUID_HIDS_CTRL_POINT),

	/* DIS */
	BT_GATT_DISCOVER_MOCK_SERV(12, BT_UUID_DIS, 0xffff),
	BT_GATT_DISCOVER_MOCK_CHRC(13, BT_UUID_DIS_MODEL_NUMBER, BT_GATT_CHRC_READ),
	BT_GATT_DISCOVER_MOCK_DESC(14, BT_UUID_DIS_MODEL_NUMBER),

	BT_GATT_DISCOVER_MOCK_CHRC(15, BT_UUID_DIS_MANUFACTURER_NAME, BT_GATT_CHRC_READ),
	BT_GATT_DISCOVER_MOCK_DESC(16, BT_UUID_DIS_MANUFACTURER_NAME),
};


void test_cb_completed(struct bt_gatt_dm *dm, void *context)
{
	printk("%s\n", __func__);
	/* Saving discovery manager instance and giving the semaphore */
	*(struct bt_gatt_dm **)context = dm;
	k_sem_give(&discovery_finished);
}

void test_cb_service_not_found(struct bt_conn *conn, void *context)
{
	printk("%s\n", __func__);
	*(struct bt_gatt_dm **)context = NULL;
	k_sem_give(&discovery_finished);
}

void test_cb_error_found(struct bt_conn *conn, int err, void *context)
{
	printk("%s\n", __func__);
	zassert_unreachable("HIDS error found");
}


struct bt_gatt_dm_cb test_hids_cb = {
	.completed         = test_cb_completed,
	.service_not_found = test_cb_service_not_found,
	.error_found       = test_cb_error_found
};

void test_setup(void)
{
	k_sem_reset(&discovery_finished);
	bt_gatt_discover_mock_setup(discover_sim, ARRAY_SIZE(discover_sim));
}

struct bt_gatt_dm *run_dm(const struct bt_uuid *svc_uuid)
{
	struct bt_gatt_dm *dm;
	int err;

	err = bt_gatt_dm_start((struct bt_conn *)&dummy_conn,
				   svc_uuid,
				   &test_hids_cb,
				   &dm);
	zassert_false(err, "bt_gatt_dm_start finished with error: %d", err);

	err = k_sem_take(&discovery_finished, K_MSEC(SERVICE_DISCOVERY_TIMEOUT));
	zassert_equal(0, err, "It seems that no callback function was called: %d", err);

	return dm;
}

struct bt_gatt_dm *run_dm_next(struct bt_gatt_dm *dm)
{
	int err;
	struct bt_gatt_dm *dm_next;

	bt_gatt_dm_data_release(dm);
	bt_gatt_dm_continue(dm, &dm_next);

	err = k_sem_take(&discovery_finished, K_MSEC(SERVICE_DISCOVERY_TIMEOUT));
	zassert_equal(0, err, "It seems that no callback function was called: %d", err);

	return dm_next;
}

/* The service that is not present */
void test_gatt_none_serv(void)
{
	struct bt_gatt_dm *dm = run_dm(BT_UUID_BAS);

	zassert_is_null(dm, "Detected service that should be inviable");
}

/* This is just a simple test to check if another service than
 * first one can be accessed */
void test_gatt_DIS_simple_next_attr(void)
{
	const struct bt_gatt_attr *attr;
	struct bt_gatt_dm *dm = run_dm(BT_UUID_DIS);

	zassert_not_null(dm, "Device Manager pointer not set");

	zassert_equal(5,
		      bt_gatt_dm_attr_cnt(dm),
		      "Unexpected number of attributes detected: %d",
		      bt_gatt_dm_attr_cnt(dm));

	attr = NULL;
	for (int i = 13; i <= 16; ++i) {
		attr = bt_gatt_dm_attr_next(dm, attr);
		zassert_not_null(attr, "Attr handle: %d", i);
		zassert_equal(i, attr->handle, "Attr handle: %d", i);
	}
	attr = bt_gatt_dm_attr_next(dm, attr);
	zassert_is_null(attr, "Attr after 11 should be NULL");

	bt_gatt_dm_data_release(dm);
	zassert_equal(0, bt_gatt_dm_attr_cnt(dm), "Parameter count after clearing: %d", bt_gatt_dm_attr_cnt(dm));
}

void test_gatt_DIS_attr_by_handle(void)
{
	const struct bt_gatt_attr *attr;
	struct bt_gatt_dm *dm = run_dm(BT_UUID_DIS);

	zassert_not_null(dm, "Device Manager pointer not set");

	attr = bt_gatt_dm_attr_by_handle(dm, 11);
	zassert_is_null(attr, "Attr before 12 should be NULL");
	attr = bt_gatt_dm_attr_by_handle(dm, 17);
	zassert_is_null(attr, "Attr after 16 should be NULL");

	for (int i = 12; i < 16; ++i) {
		attr = bt_gatt_dm_attr_by_handle(dm, i);
		zassert_not_null(attr, "Attr handle: %d", i);
		zassert_equal(i, attr->handle, "Attr handle: %d", i);
	}
	bt_gatt_dm_data_release(dm);
	zassert_equal(0, bt_gatt_dm_attr_cnt(dm), "Parameter count after clearing: %d", bt_gatt_dm_attr_cnt(dm));
}

void test_gatt_HIDS_simple_next_attr(void)
{
	const struct bt_gatt_attr *attr;
	struct bt_gatt_dm *dm = run_dm(BT_UUID_HIDS);

	zassert_not_null(dm, "Device Manager pointer not set");

	attr = NULL;
	for (int i = 2; i <= 11; ++i) {
		attr = bt_gatt_dm_attr_next(dm, attr);
		zassert_not_null(attr, "Attr handle: %d", i);
		zassert_equal(i, attr->handle, "Attr handle: %d", i);
	}
	attr = bt_gatt_dm_attr_next(dm, attr);
	zassert_is_null(attr, "Attr after 11 should be NULL");

	bt_gatt_dm_data_release(dm);
	zassert_equal(0, bt_gatt_dm_attr_cnt(dm), "Parameter count after clearing: %d", bt_gatt_dm_attr_cnt(dm));
}

void test_gatt_HIDS_attr_by_handle(void)
{
	const struct bt_gatt_attr *attr;
	struct bt_gatt_dm *dm = run_dm(BT_UUID_HIDS);

	zassert_not_null(dm, "Device Manager pointer not set");

	attr = bt_gatt_dm_attr_by_handle(dm, 0);
	zassert_is_null(attr, "Attr before 1 should be NULL");
	attr = bt_gatt_dm_attr_by_handle(dm, 12);
	zassert_is_null(attr, "Attr after 11 should be NULL");

	for (int i = 1; i < 11; ++i) {
		attr = bt_gatt_dm_attr_by_handle(dm, i);
		zassert_not_null(attr, "Attr handle: %d", i);
		zassert_equal(i, attr->handle, "Attr handle: %d", i);
	}
	bt_gatt_dm_data_release(dm);
	zassert_equal(0, bt_gatt_dm_attr_cnt(dm), "Parameter count after clearing: %d", bt_gatt_dm_attr_cnt(dm));
}

void test_gatt_HIDS_next_chrc_access(void)
{
	struct bt_gatt_dm *dm;
	const struct bt_gatt_attr *attr_serv;
	const struct bt_gatt_attr *attr_chrc;
	const struct bt_gatt_attr *attr_desc;
	const struct bt_gatt_service_val *serv_val;
	const struct bt_gatt_chrc        *chrc_val;

	dm = run_dm(BT_UUID_HIDS);
	zassert_not_null(dm, "Device Manager pointer not set");

	/* Check service */
	attr_serv = bt_gatt_dm_service_get(dm);
	serv_val  = bt_gatt_dm_attr_service_val(attr_serv);
	zassert_true(!bt_uuid_cmp(BT_UUID_HIDS, serv_val->uuid), "Invalid service detected");
	zassert_equal(11,
		      bt_gatt_dm_attr_cnt(dm),
		      "Unexpected number of attributes detected: %d",
		      bt_gatt_dm_attr_cnt(dm));

	/* ------------------------------------------------------ */
	/* First characteristic: HIDS_INFO */
	attr_chrc = bt_gatt_dm_char_next(dm, NULL);
	zassert_not_null(attr_chrc, "Unexpected NULL instead of HIDS_INFO");
	zassert_equal(2, attr_chrc->handle, "Unexpected handle value for HIDS_INFO");
	chrc_val = bt_gatt_dm_attr_chrc_val(attr_chrc);
	zassert_not_null(chrc_val, "Unexpected NULL instead of HIDS_INFO value");
	zassert_true(!bt_uuid_cmp(BT_UUID_HIDS_INFO, chrc_val->uuid), "Unexpected UUID");
	zassert_equal(BT_GATT_CHRC_READ, chrc_val->properties, "Unexpected HIDS_INFO properties");
	/* Simple access to next descriptors */
	attr_desc = bt_gatt_dm_desc_next(dm, attr_chrc);
	zassert_not_null(attr_desc, "Unexpected NULL");
	zassert_equal(3, attr_desc->handle, "Unexpected handle");
	zassert_true(!bt_uuid_cmp(BT_UUID_HIDS_INFO, attr_desc->uuid), "Unexpected UUID");

	attr_desc = bt_gatt_dm_desc_next(dm, attr_desc);
	zassert_is_null(attr_desc, "Expecting NULL");

	/* ------------------------------------------------------ */
	/* Next characteristic: HIDS_REPORT_MAP */
	attr_chrc = bt_gatt_dm_char_next(dm, attr_chrc);
	zassert_not_null(attr_chrc, "Unexpected NULL instead of HIDS_REPORT_MAP");
	zassert_equal(4, attr_chrc->handle, "Unexpected handle value for HIDS_REPORT_MAP");
	chrc_val = bt_gatt_dm_attr_chrc_val(attr_chrc);
	zassert_not_null(chrc_val, "Unexpected NULL instead HIDS_REPORT_MAP value");
	zassert_true(!bt_uuid_cmp(BT_UUID_HIDS_REPORT_MAP, chrc_val->uuid), "Unexpected HIDS_REPORT_MAP UUID");
	zassert_equal(BT_GATT_CHRC_READ, chrc_val->properties, "Unexpected HIDS_REPORT_MAP properties");
	/* Simple access to next descriptors */
	attr_desc = bt_gatt_dm_desc_next(dm, attr_chrc);
	zassert_not_null(attr_desc, "Unexpected NULL");
	zassert_equal(5, attr_desc->handle, "Unexpected handle");
	attr_desc = bt_gatt_dm_desc_next(dm, attr_desc);
	zassert_is_null(attr_desc, "Expecting NULL");

	/* ------------------------------------------------------ */
	/* Next characteristic: BT_UUID_HIDS_REPORT */
	attr_chrc = bt_gatt_dm_char_next(dm, attr_chrc);
	zassert_not_null(attr_chrc, "Unexpected NULL instead of HIDS_REPORT");
	zassert_equal(6, attr_chrc->handle, "Unexpected handle value for HIDS_REPORT");
	chrc_val = bt_gatt_dm_attr_chrc_val(attr_chrc);
	zassert_not_null(chrc_val, "Unexpected NULL instead HIDS_REPORT value");
	zassert_true(!bt_uuid_cmp(BT_UUID_HIDS_REPORT, chrc_val->uuid), "Unexpected HIDS_REPORT UUID");
	zassert_equal(BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
		      chrc_val->properties,
		      "Unexpected HIDS_REPORT properties");
	/* Simple access to next descriptors */
	attr_desc = bt_gatt_dm_desc_next(dm, attr_chrc);
	zassert_not_null(attr_desc, "Unexpected NULL");
	zassert_equal(7, attr_desc->handle, "Unexpected handle");
	attr_desc = bt_gatt_dm_desc_next(dm, attr_desc);
	zassert_not_null(attr_desc, "Unexpected NULL");
	zassert_equal(8, attr_desc->handle, "Unexpected handle");
	attr_desc = bt_gatt_dm_desc_next(dm, attr_desc);
	zassert_not_null(attr_desc, "Unexpected NULL");
	zassert_equal(9, attr_desc->handle, "Unexpected handle");
	attr_desc = bt_gatt_dm_desc_next(dm, attr_desc);
	zassert_is_null(attr_desc, "Expecting NULL");

	/* ------------------------------------------------------ */
	/* Next characteristic: BT_UUID_HIDS_CTRL_POINT */
	attr_chrc = bt_gatt_dm_char_next(dm, attr_chrc);
	zassert_not_null(attr_chrc, "Unexpected NULL instead of HIDS_CTRL_POINT");
	attr_desc = bt_gatt_dm_desc_next(dm, attr_chrc);
	zassert_not_null(attr_desc, "Unexpected NULL");
	zassert_equal(11, attr_desc->handle, "Unexpected handle");
	attr_desc = bt_gatt_dm_desc_next(dm, attr_desc);
	zassert_is_null(attr_desc, "Expecting NULL");

	/* ------------------------------------------------------ */
	/* Next characteristic: none */
	attr_chrc = bt_gatt_dm_char_next(dm, attr_chrc);
	zassert_is_null(attr_chrc, "Unexpected characteristic detected");

	bt_gatt_dm_data_release(dm);
	zassert_equal(0, bt_gatt_dm_attr_cnt(dm), "Parameter count after clearing: %d", bt_gatt_dm_attr_cnt(dm));
}

void test_gatt_HIDS_chrc_by_uuid(void)
{
	struct bt_gatt_dm *dm;
	const struct bt_gatt_attr *attr_chrc;
	const struct bt_gatt_attr *attr_desc;

	dm = run_dm(BT_UUID_HIDS);
	zassert_not_null(dm, "Device Manager pointer not set");

	/* ------------------------------------------------------ */
	/* Searching for HIDS_REPORT by UUID */
	attr_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_HIDS_REPORT);
	zassert_not_null(attr_chrc, "Unexpected NULL");
	zassert_equal(6, attr_chrc->handle, "Unexpected handle: %d", attr_chrc->handle);
	/* Searching for CCC descriptor */
	attr_desc = bt_gatt_dm_desc_by_uuid(dm, attr_chrc, BT_UUID_GATT_CCC);
	zassert_not_null(attr_desc, "Unexpected NULL");
	zassert_equal(8, attr_desc->handle, "Unexpected handle: %d", attr_desc->handle);
	/* Searching for some descriptor that is not present */
	attr_desc = bt_gatt_dm_desc_by_uuid(dm, attr_chrc, BT_UUID_DIS_MANUFACTURER_NAME);
	zassert_is_null(attr_desc, "Expected NULL handle");
	/* Trying to search for a descriptor from next characteristic */
	attr_desc = bt_gatt_dm_desc_by_uuid(dm, attr_chrc, BT_UUID_HIDS_CTRL_POINT);
	zassert_is_null(attr_desc, "Expected NULL handle");
	/* Can we access for characteristic as a descriptor? */
	attr_desc = bt_gatt_dm_desc_by_uuid(dm, attr_chrc, BT_UUID_GATT_CHRC);
	zassert_is_null(attr_desc, "Expected NULL handle");

	/* ------------------------------------------------------ */
	/* Searching for characteristic that should not be found */
	attr_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_DIS_MODEL_NUMBER);
	zassert_is_null(attr_chrc, "Expected NULL");

	/* ------------------------------------------------------ */
	/* Clean up */
	bt_gatt_dm_data_release(dm);
	zassert_equal(0, bt_gatt_dm_attr_cnt(dm), "Parameter count after clearing: %d", bt_gatt_dm_attr_cnt(dm));
}

void test_gatt_generic_serv(void)
{
	struct bt_gatt_dm *dm;
	const struct bt_gatt_attr *attr_serv;
	const struct bt_gatt_service_val *serv_val;

	dm = run_dm(NULL);
	zassert_not_null(dm, "Device Manager pointer not set");
	attr_serv = bt_gatt_dm_service_get(dm);
	serv_val  = bt_gatt_dm_attr_service_val(attr_serv);
	zassert_true(!bt_uuid_cmp(BT_UUID_HIDS, serv_val->uuid), "Invalid service detected");
	zassert_equal(11,
		      bt_gatt_dm_attr_cnt(dm),
		      "Unexpected number of attributes detected: %d",
		      bt_gatt_dm_attr_cnt(dm));

	dm = run_dm_next(dm);
	zassert_not_null(dm, "Device Manager pointer not set");
	attr_serv = bt_gatt_dm_service_get(dm);
	serv_val  = bt_gatt_dm_attr_service_val(attr_serv);
	zassert_true(!bt_uuid_cmp(BT_UUID_DIS, serv_val->uuid), "Invalid service detected");
	zassert_equal(5,
		      bt_gatt_dm_attr_cnt(dm),
		      "Unexpected number of attributes detected: %d",
		      bt_gatt_dm_attr_cnt(dm));

	dm = run_dm_next(dm);
	zassert_is_null(dm, "Unexpected service detected");
	/* ------------------------------------------------------ */
	/* No cleanup here - cleanup is done in run_dm_next */
}

void test_main(void)
{
	ztest_test_suite(
		test_gatt,
		ztest_unit_test_setup_teardown(test_gatt_none_serv, test_setup, unit_test_noop),
		ztest_unit_test_setup_teardown(test_gatt_DIS_simple_next_attr, test_setup, unit_test_noop),
		ztest_unit_test_setup_teardown(test_gatt_DIS_attr_by_handle, test_setup, unit_test_noop),
		ztest_unit_test_setup_teardown(test_gatt_HIDS_simple_next_attr, test_setup, unit_test_noop),
		ztest_unit_test_setup_teardown(test_gatt_HIDS_attr_by_handle, test_setup, unit_test_noop),
		ztest_unit_test_setup_teardown(test_gatt_HIDS_next_chrc_access, test_setup, unit_test_noop),
		ztest_unit_test_setup_teardown(test_gatt_HIDS_chrc_by_uuid, test_setup, unit_test_noop),
		ztest_unit_test_setup_teardown(test_gatt_generic_serv, test_setup, unit_test_noop)
	);

	ztest_run_test_suite(test_gatt);
}
