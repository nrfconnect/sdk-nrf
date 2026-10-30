/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/kernel.h>

#include "bstests.h"
#include "babblekit/testcase.h"

#define CYCLES 200U

static void per_adv_restart(void)
{
	struct bt_le_adv_param conn_param =
		BT_LE_ADV_PARAM_INIT(BT_LE_ADV_OPT_EXT_ADV | BT_LE_ADV_OPT_CONN, 0x30, 0x30, NULL);
	struct bt_le_adv_param per_param =
		BT_LE_ADV_PARAM_INIT(BT_LE_ADV_OPT_EXT_ADV, 0x30, 0x30, NULL);
	struct bt_le_ext_adv *conn_adv;
	struct bt_le_ext_adv *per_adv;
	int err;

	err = bt_enable(NULL);
	TEST_ASSERT(!err, "bt_enable failed (%d)", err);

	conn_param.sid = 0U;
	per_param.sid = 1U;

	err = bt_le_ext_adv_create(&conn_param, NULL, &conn_adv);
	TEST_ASSERT(!err, "bt_le_ext_adv_create failed (%d)", err);

	err = bt_le_ext_adv_start(conn_adv, BT_LE_EXT_ADV_START_DEFAULT);
	TEST_ASSERT(!err, "bt_le_ext_adv_start failed (%d)", err);

	err = bt_le_ext_adv_create(&per_param, NULL, &per_adv);
	TEST_ASSERT(!err, "bt_le_ext_adv_create failed (%d)", err);

	err = bt_le_per_adv_set_param(per_adv,
				      BT_LE_PER_ADV_PARAM(0x78, 0xA0, BT_LE_PER_ADV_OPT_NONE));
	TEST_ASSERT(!err, "bt_le_per_adv_set_param failed (%d)", err);

	for (uint32_t i = 0U; i < CYCLES; i++) {
		k_sleep(K_USEC(1000U + (i * 997U) % 30000U));

		err = bt_le_ext_adv_start(per_adv, BT_LE_EXT_ADV_START_DEFAULT);
		TEST_ASSERT(!err, "bt_le_ext_adv_start failed (%d)", err);

		err = bt_le_per_adv_start(per_adv);
		TEST_ASSERT(!err, "bt_le_per_adv_start failed (%d)", err);

		k_sleep(K_MSEC(5));

		err = bt_le_per_adv_stop(per_adv);
		TEST_ASSERT(!err, "bt_le_per_adv_stop failed (%d)", err);

		err = bt_le_ext_adv_stop(per_adv);
		TEST_ASSERT(!err, "bt_le_ext_adv_stop failed (%d)", err);
	}

	TEST_PASS("%u periodic advertising restarts", CYCLES);
}

static const struct bst_test_instance test_to_add[] = {
	{
		.test_id = "per_adv_restart",
		.test_main_f = per_adv_restart,
	},
	BSTEST_END_MARKER,
};

static struct bst_test_list *install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_to_add);
}

bst_test_install_t test_installers[] = {install, NULL};

int main(void)
{
	bst_main();
	return 0;
}
