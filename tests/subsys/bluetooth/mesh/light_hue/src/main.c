/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <zephyr/ztest.h>
#include <zephyr/bluetooth/mesh.h>
#include <bluetooth/mesh/models.h>
#include <bluetooth/mesh/light_hue_srv.h>
#include "model_utils.h"

#define HUE_TO_LVL(_hue) ((_hue) - 32768)

static uint16_t light_hue_val;

static void hue_set(struct bt_mesh_light_hue_srv *srv,
			  struct bt_mesh_msg_ctx *ctx,
			  const struct bt_mesh_light_hue *set,
			  struct bt_mesh_light_hue_status *rsp)
{
	ztest_check_expected_value(set->lvl);

	if (set->transition) {
		ztest_check_expected_value(set->transition->time);
		ztest_check_expected_value(set->direction);
	}

	if (!set->transition) {
		rsp->current = set->lvl;
		rsp->remaining_time = 0;
	} else {
		rsp->current = light_hue_val;
		rsp->target = set->lvl;
		rsp->remaining_time = set->transition->time;
	}
}

static void hue_move_set(struct bt_mesh_light_hue_srv *srv,
			       struct bt_mesh_msg_ctx *ctx,
			       const struct bt_mesh_light_hue_move *move,
			       struct bt_mesh_light_hue_status *rsp)
{
	ztest_check_expected_value(move->delta);

	if (move->transition) {
		ztest_check_expected_value(move->transition->time);
	}

	if (!move->transition) {
		rsp->current = light_hue_val;
		rsp->remaining_time = 0;
	} else {
		rsp->current = light_hue_val;
		rsp->target = light_hue_val + move->delta;
		rsp->remaining_time = move->transition->time;
	}
}

static void hue_get(struct bt_mesh_light_hue_srv *srv,
			  struct bt_mesh_msg_ctx *ctx,
			  struct bt_mesh_light_hue_status *rsp)
{
	rsp->current = light_hue_val;
	rsp->target = light_hue_val;
	rsp->remaining_time = 0;
}

static void hue_default_update(struct bt_mesh_light_hue_srv *srv,
				     struct bt_mesh_msg_ctx *ctx,
				     uint16_t old_default,
				     uint16_t new_default)
{
	zassert_true(false);
}

static void hue_range_update(
		struct bt_mesh_light_hue_srv *srv,
		struct bt_mesh_msg_ctx *ctx,
		const struct bt_mesh_light_hsl_range *old_range,
		const struct bt_mesh_light_hsl_range *new_range)
{
	zassert_true(false);
}

static struct bt_mesh_light_hue_srv_handlers light_hue_srv_handlers = {
	.set = hue_set,
	.move_set = hue_move_set,
	.get = hue_get,
	.default_update = hue_default_update,
	.range_update = hue_range_update,
};

static struct bt_mesh_light_hue_srv light_hue_srv =
	BT_MESH_LIGHT_HUE_SRV_INIT(&light_hue_srv_handlers);

static const struct bt_mesh_model mock_light_hue_srv_model = {
	.rt = &(struct bt_mesh_model_rt_ctx){.user_data = &light_hue_srv, .elem_idx = 1}};

/** Mocks ******************************************/

static int hue_status_pub(struct net_buf_simple *buf)
{
	uint16_t current, target;
	int32_t remaining_time;

	current = net_buf_simple_pull_le16(buf);
	ztest_check_expected_value(current);

	if (buf->len > 0) {
		target = net_buf_simple_pull_le16(buf);
		remaining_time = model_transition_decode(net_buf_simple_pull_u8(buf));

		ztest_check_expected_value(target);
		ztest_check_expected_value(remaining_time);

		zassert_equal(buf->len, 0);
	}

	return 0;
}

int bt_mesh_msg_send(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
	       struct net_buf_simple *buf)
{
	uint16_t opcode;

	ztest_check_expected_value(model);
	ztest_check_expected_value(ctx);
	ztest_check_expected_value(buf->len);

	/* Only 2 octets opcodes are supported by this test. */
	zassert_equal(buf->data[0] >> 6, 2);

	opcode = net_buf_simple_pull_be16(buf);
	zassert_equal(BT_MESH_LIGHT_HUE_OP_STATUS, opcode);
	hue_status_pub(buf);

	return 0;
}

void bt_mesh_model_msg_init(struct net_buf_simple *msg, uint32_t opcode)
{
	zassert_equal(BT_MESH_MODEL_OP_LEN(opcode), 2);

	net_buf_simple_init(msg, 0);
	net_buf_simple_add_be16(msg, opcode);
}

int bt_mesh_model_extend(const struct bt_mesh_model *mod,
			 const struct bt_mesh_model *base_mod)
{
	return 0;
}

int bt_mesh_lvl_srv_pub(struct bt_mesh_lvl_srv *srv,
			struct bt_mesh_msg_ctx *ctx,
			const struct bt_mesh_lvl_status *status)
{
	return 0;
}

/** End Mocks **************************************/

static int hue_set_unack_send(struct bt_mesh_light_hue *set)
{
	static uint8_t tid;
	struct bt_mesh_msg_ctx ctx = {
		.addr = 1,
	};

	NET_BUF_SIMPLE_DEFINE(buf, 5);
	net_buf_simple_init(&buf, 0);

	net_buf_simple_add_le16(&buf, set->lvl);
	net_buf_simple_add_u8(&buf, tid++);

	if (set->transition) {
		model_transition_buf_add(&buf, set->transition);
	}

	return _bt_mesh_light_hue_srv_op[2].func(&mock_light_hue_srv_model, &ctx, &buf);
}

static int lvl_set_unack_send(struct bt_mesh_lvl_set *set)
{
	struct bt_mesh_msg_ctx ctx = {
		.addr = 1,
	};
	struct bt_mesh_lvl_status rsp_stub;

	light_hue_srv.lvl.handlers->set(&light_hue_srv.lvl, &ctx, set, &rsp_stub);

	return 0;
}

static int lvl_delta_set_unack_send(struct bt_mesh_lvl_delta_set *set)
{
	struct bt_mesh_msg_ctx ctx = {
		.addr = 1,
	};
	struct bt_mesh_lvl_status rsp_stub;

	light_hue_srv.lvl.handlers->delta_set(&light_hue_srv.lvl, &ctx, set, &rsp_stub);

	return 0;
}

static int lvl_move_set_unack_send(struct bt_mesh_lvl_move_set *set)
{
	struct bt_mesh_msg_ctx ctx = {
		.addr = 1,
	};
	struct bt_mesh_lvl_status rsp_stub;

	light_hue_srv.lvl.handlers->move_set(&light_hue_srv.lvl, &ctx, set, &rsp_stub);

	return 0;
}

static void expect_hue_status_pub(struct bt_mesh_light_hue_status *status)
{
	ztest_expect_value(bt_mesh_msg_send, model, &mock_light_hue_srv_model);
	ztest_expect_value(bt_mesh_msg_send, ctx, NULL);
	ztest_expect_value(bt_mesh_msg_send, buf->len, status->remaining_time ? 7 : 4);

	ztest_expect_value(hue_status_pub, current, status->current);

	if (status->remaining_time != 0) {
		ztest_expect_value(hue_status_pub, target, status->target);
		ztest_expect_value(hue_status_pub, remaining_time, status->remaining_time);
	}
}

static void expect_hue_set_cb(struct bt_mesh_light_hue *set)
{
	ztest_expect_value(hue_set, set->lvl, set->lvl);

	if (set->transition) {
		ztest_expect_value(hue_set, set->transition->time, set->transition->time);
		ztest_expect_value(hue_set, set->direction, set->direction);
	}
}

static void expect_hue_move_set_cb(struct bt_mesh_light_hue_move *move)
{
	ztest_expect_value(hue_move_set, move->delta, move->delta);

	if (move->transition) {
		ztest_expect_value(hue_move_set, move->transition->time, move->transition->time);
	}
}

static void setup(void *f)
{
	zassert_not_null(_bt_mesh_light_hue_srv_cb.init, "Init cb is null");
	zassert_ok(_bt_mesh_light_hue_srv_cb.init(&mock_light_hue_srv_model), "Init failed");
}

static void teardown(void *f)
{
	zassert_not_null(_bt_mesh_light_hue_srv_cb.reset, "Reset cb is null");
	_bt_mesh_light_hue_srv_cb.reset(&mock_light_hue_srv_model);
}

/* Test Hue value clamping without transition. */
ZTEST(light_hue_srv_test, test_hue_set)
{
	struct {
		uint16_t min;
		uint16_t max;
		uint16_t current_hue;
		uint16_t target_hue;
		uint16_t expected_hue;
	} test_vector[] = {
		/* Range Min, Range Max, Current, Target, Expected */
		{  0,         0xFFFF,    0,       0,      0       },
		{  0,         0xFFFF,    0,       0xFFFF, 0xFFFF  },
		/* [100, 0xFF00] */
		{  100,       0xFF00,    0xFFFF,  99,     100     },
		{  100,       0xFF00,    100,     0xFF01, 0xFF00  },
		{  100,       0xFF00,    0xFF00,  101,    101     },
		{  100,       0xFF00,    101,     0xFEFF, 0xFEFF  },
		/* [0 - 100, 0xFF00 - 0xFFFF] */
		{  0xFF00,    100,       0xFEFF,  101,    100     },
		{  0xFF00,    100,       100,     0xFEFF, 0xFF00  },
		{  0xFF00,    100,       0xFF00,  99,     99      },
		{  0xFF00,    100,       99,      0xFF01, 0xFF01  },
	};

	for (size_t i = 0; i < ARRAY_SIZE(test_vector); i++) {
		struct bt_mesh_light_hue_status status = {};

		printk("Checking test_vector[%d]\n", i);

		light_hue_srv.range.min = test_vector[i].min;
		light_hue_srv.range.max = test_vector[i].max;
		light_hue_val = test_vector[i].current_hue;

		status.current = test_vector[i].expected_hue;
		expect_hue_status_pub(&status);

		struct bt_mesh_light_hue hue_set = {
			.lvl = test_vector[i].target_hue,
		};
		struct bt_mesh_light_hue exp_hue_set = {
			.lvl = test_vector[i].expected_hue,
		};

		expect_hue_set_cb(&exp_hue_set);
		zassert_ok(hue_set_unack_send(&hue_set));
	}
}

/* Test Hue value clamping with transition. */
ZTEST(light_hue_srv_test, test_hue_set_transition)
{
	struct {
		uint16_t min;
		uint16_t max;
		uint16_t current_hue;
		uint16_t target_hue;
		uint16_t expected_hue;
		int direction;
	} test_vector[] = {
		/* Range Min, Range Max, Current, Target, Expected, Diretion */
		{  0,         0xFFFF,    0,       0x7FFF, 0x7FFF,    1  },
		{  0,         0xFFFF,    0xFFFF,  0x8000, 0x8000,   -1  },
		/* Direction points to the shortest path which causes the value to wrap around. */
		{  0,         0xFFFF,    0xFFFF,  0,      0,         1  },
		{  0,         0xFFFF,    0,       0xFFFF, 0xFFFF,   -1  },
		/* [100, 0xFF00] */
		{  100,       0xFF00,    0xFFFF,  99,     100,      -1  },
		{  100,       0xFF00,    100,     0xFF01, 0xFF00,    1  },
		{  100,       0xFF00,    0xFF00,  101,    101,      -1  },
		{  100,       0xFF00,    101,     0xFEFF, 0xFEFF,    1  },
		/* [0 - 100, 0xFF00 - 0xFFFF] */
		{  0xFF00,    100,       0xFEFF,  101,    100,       1  },
		{  0xFF00,    100,       100,     0xFEFF, 0xFF00,   -1  },
		{  0xFF00,    100,       0xFF00,  99,     99,        1  },
		{  0xFF00,    100,       99,      0xFF01, 0xFF01,   -1  },
	};

	for (size_t i = 0; i < ARRAY_SIZE(test_vector); i++) {
		struct bt_mesh_light_hue_status status = {
			.current = test_vector[i].current_hue,
			.target = test_vector[i].expected_hue,
			.remaining_time = 100, /* Same as Transition Time set below. */
		};

		printk("Checking test_vector[%d]\n", i);

		light_hue_srv.range.min = test_vector[i].min;
		light_hue_srv.range.max = test_vector[i].max;
		light_hue_val = test_vector[i].current_hue;

		expect_hue_status_pub(&status);

		/* Use arbitrary Transition Time value to see different current and target values
		 * in the status message.
		 */
		struct bt_mesh_model_transition transition = {
			.time = 100,
		};
		struct bt_mesh_light_hue hue_set = {
			.lvl = test_vector[i].target_hue,
			.transition = &transition,
		};
		struct bt_mesh_light_hue exp_hue_set = {
			.lvl = test_vector[i].expected_hue,
			.transition = &transition,
			.direction = test_vector[i].direction,
		};

		expect_hue_set_cb(&exp_hue_set);
		zassert_ok(hue_set_unack_send(&hue_set));
	}
}

/* Test Hue value clamping and binding with Generic Level state when sending Generic Level Set
 * message.
 */
ZTEST(light_hue_srv_test, test_lvl_set)
{
	struct {
		uint16_t min;
		uint16_t max;
		uint16_t current_hue;
		int16_t target_lvl;
		uint16_t expected_hue;
		int direction;
	} test_vector[] = {
		/* See binding between Hue state and Generic Level state in MshMDLd1.1 spec,
		 * section 6.1.4.1.1:
		 *
		 *     Generic Level state
		 * -0x8000      0      0x7FFF
		 *    |---------|--------|
		 *    0      0x8000    0xFFFF
		 *     Light Hue state
		 */
		/* Range Min, Range Max, Current, Level,    Expected, Diretion */
		{  0,         0xFFFF,    0,        0,       0x8000,   -1  },
		{  0,         0xFFFF,    0,       -1,       0x7FFF,    1  },
		{  0,         0xFFFF,    0,        0x7FFF,  0xFFFF,   -1  },
		{  0,         0xFFFF,    0xFF00,  -0x7FFF,  1,         1  },
		{  0,         0xFFFF,    0x64,     0x7FFF,  0xFFFF,   -1  },
		{  0,         0xFFFF,    0xFF00,  -0x7FFF,  1,         1  },
		/* [100, 0xFF00] */
		{  100,       0xFF00,    0x7F00,  -0x7F9B,  101,      -1  },
		{  100,       0xFF00,    0x7F00,   0x7EFF,  0xFEFF,    1  },
		{  100,       0xFF00,    0x7F00,  -0x7F9D,  100,      -1  },
		{  100,       0xFF00,    0x7F00,   0x7F01,  0xFF00,    1  },
		/* [0 - 100, 0xFF00 - 0xFFFF] */
		{  0xFF00,    0x100,     0,        0x7FFF,  0xFFFF,   -1   },
		{  0xFF00,    0x100,     0,       -0x7FFF,  1,         1   },
		{  0xFF00,    0x100,     0,        0x7EFF,  0xFF00,   -1   },
		{  0xFF00,    0x100,     0,       -0x7EFF,  0x100,     1   },
	};

	for (size_t i = 0; i < ARRAY_SIZE(test_vector); i++) {
		struct bt_mesh_light_hue_status status = {
			.current = test_vector[i].current_hue,
			.target = test_vector[i].expected_hue,
			.remaining_time = 100, /* Same as Transition Time set below. */
		};

		printk("Checking test_vector[%d]\n", i);

		light_hue_srv.range.min = test_vector[i].min;
		light_hue_srv.range.max = test_vector[i].max;
		light_hue_val = test_vector[i].current_hue;

		expect_hue_status_pub(&status);

		/* Use arbitrary Transition Time value to see different current and target values
		 * in the status message.
		 */
		struct bt_mesh_model_transition transition = {
			.time = 100,
		};
		struct bt_mesh_lvl_set lvl_set = {
			.lvl = test_vector[i].target_lvl,
			.transition = &transition,
			.new_transaction = true,
		};
		struct bt_mesh_light_hue exp_hue_set = {
			.lvl = test_vector[i].expected_hue,
			.transition = &transition,
			.direction = test_vector[i].direction,
		};

		expect_hue_set_cb(&exp_hue_set);
		zassert_ok(lvl_set_unack_send(&lvl_set));
	}
}

/* Test Hue value clamping and binding with Generic Level state when sending Generic Delta Set
 * message.
 */
ZTEST(light_hue_srv_test, test_lvl_delta_set)
{
	struct {
		uint16_t min;
		uint16_t max;
		uint16_t current_hue;
		int32_t lvl_delta;
		uint16_t expected_hue;
		int direction;
	} test_vector[] = {
		/* See binding between Hue state and Generic Level state in MshMDLd1.1 spec,
		 * section 6.1.4.1.1:
		 *
		 *     Generic Level state
		 * -0x8000      0      0x7FFF
		 *    |---------|--------|
		 *    0      0x8000    0xFFFF
		 *     Light Hue state
		 */
		/* Range Min, Range Max, Current, Delta,    Expected, Diretion */
		{  0,         0xFFFF,    0,        0x7FFF,  0x7FFF,    1  },
		{  0,         0xFFFF,    0xFFFF,  -0x7FFF,  0x8000,   -1  },
		{  0,         0xFFFF,    0x8001,   0x7FFF,  0,         1  },
		{  0,         0xFFFF,    0x8000,   0x7FFF,  0xFFFF,    1  },
		{  0,         0xFFFF,    0xFF00,   0x101,   1,         1  },
		{  0,         0xFFFF,    0x100,   -0x101,   0xFFFF,   -1  },
		{  0,         0xFFFF,    0,       -0x8000,  0x8000,   -1  },
		{  0,         0xFFFF,    0xFFFF,   0x7FFF,  0x7FFE,    1  },
		/* [100, 0xFF00] */
		{  0x100,     0xFF00,    0x7F00,  -0x7F01,  0x100,    -1  },
		{  0x100,     0xFF00,    0x8000,   0x7F01,  0xFF00,    1  },
		{  0x100,     0xFF00,    0x100,   -0x8000,  0x100,     1  },
		{  0x100,     0xFF00,    0xFF00,   0x7FFF,  0xFF00,    1  },
		/* [0 - 100, 0xFF00 - 0xFFFF] */
		{  0xFF00,    0x100,     0,        0x101,   0x100,     1   },
		{  0xFF00,    0x100,     0,       -0x101,   0xFF00,   -1   },
	};

	for (size_t i = 0; i < ARRAY_SIZE(test_vector); i++) {
		struct bt_mesh_light_hue_status status = {
			.current = test_vector[i].current_hue,
			.target = test_vector[i].expected_hue,
			.remaining_time = 100, /* Same as Transition Time set below. */
		};

		printk("Checking test_vector[%d]\n", i);

		light_hue_srv.range.min = test_vector[i].min;
		light_hue_srv.range.max = test_vector[i].max;
		light_hue_val = test_vector[i].current_hue;

		expect_hue_status_pub(&status);

		/* Use arbitrary Transition Time value to see different current and target values
		 * in the status message.
		 */
		struct bt_mesh_model_transition transition = {
			.time = 100,
		};
		struct bt_mesh_lvl_delta_set lvl_delta_set = {
			.delta = test_vector[i].lvl_delta,
			.transition = &transition,
			.new_transaction = true,
		};
		struct bt_mesh_light_hue exp_hue_set = {
			.lvl = test_vector[i].expected_hue,
			.transition = &transition,
			.direction = test_vector[i].direction,
		};

		expect_hue_set_cb(&exp_hue_set);
		zassert_ok(lvl_delta_set_unack_send(&lvl_delta_set));
	}
}

/* Test Hue value clamping and binding with Generic Level state when sending Generic Move Set
 * message when Hue value range is limited.
 */
ZTEST(light_hue_srv_test, test_lvl_move_set_finite)
{
	struct {
		uint16_t min;
		uint16_t max;
		uint16_t current_hue;
		int32_t lvl_move;
		uint16_t expected_hue;
		int direction;
		int32_t remaining_time;
	} test_vector[] = {
		/* See binding between Hue state and Generic Level state in MshMDLd1.1 spec,
		 * section 6.1.4.1.1:
		 *
		 *     Generic Level state
		 * -0x8000      0      0x7FFF
		 *    |---------|--------|
		 *    0      0x8000    0xFFFF
		 *     Light Hue state
		 */
		/* Range Min, Range Max, Current, Move,     Expected, Diretion, Rem Time */
		/* [100, 0xFF00] */
		{  0x100,     0xFF00,    0x7F00,   0x100,   0xFF00,    1,       128 * 100    },
		{  0x100,     0xFF00,    0x7F00,  -0x100,   0x100,    -1,       126 * 100    },
		/* [0 - 100, 0xFF00 - 0xFFFF] */
		{  0xFF00,    0x100,     0,        0x1,     0x100,     1,       0x100 * 100  },
		{  0xFF00,    0x100,     0,       -0x1,     0xFF00,   -1,       0x100 * 100  },
	};

	for (size_t i = 0; i < ARRAY_SIZE(test_vector); i++) {
		struct bt_mesh_light_hue_status status = {
			.current = test_vector[i].current_hue,
			.target = test_vector[i].expected_hue,
			.remaining_time = model_transition_decode(
					model_transition_encode(test_vector[i].remaining_time)),
		};

		printk("Checking test_vector[%d]\n", i);

		light_hue_srv.range.min = test_vector[i].min;
		light_hue_srv.range.max = test_vector[i].max;
		light_hue_val = test_vector[i].current_hue;

		expect_hue_status_pub(&status);

		/* Use arbitrary Transition Time value to see different current and target values
		 * in the status message.
		 */
		struct bt_mesh_model_transition transition = {
			.time = 100,
		};
		struct bt_mesh_lvl_move_set lvl_move_set = {
			.delta = test_vector[i].lvl_move,
			.transition = &transition,
			.new_transaction = true,
		};
		struct bt_mesh_model_transition exp_transition = {
			.time = test_vector[i].remaining_time,
		};
		struct bt_mesh_light_hue exp_hue_set = {
			.lvl = test_vector[i].expected_hue,
			.transition = &exp_transition,
			.direction = test_vector[i].direction,
		};

		expect_hue_set_cb(&exp_hue_set);
		zassert_ok(lvl_move_set_unack_send(&lvl_move_set));
	}
}

/* Test Hue value clamping and binding with Generic Level state when sending Generic Move Set
 * message when Hue value range is not limited.
 */
ZTEST(light_hue_srv_test, test_lvl_move_set_infinite)
{
	struct {
		uint16_t current_hue;
		int32_t lvl_move;
		uint16_t expected_hue;
	} test_vector[] = {
		/* See binding between Hue state and Generic Level state in MshMDLd1.1 spec,
		 * section 6.1.4.1.1:
		 *
		 *     Generic Level state
		 * -0x8000      0      0x7FFF
		 *    |---------|--------|
		 *    0      0x8000    0xFFFF
		 *     Light Hue state
		 */
		/* Current, Move,    Expected */
		{  0x7F00,   0x100,  0x8000   },
		{  0x7F00,  -0x100,  0x7E00   },
	};

	light_hue_srv.range.min = 0;
	light_hue_srv.range.max = 0xFFFF;

	for (size_t i = 0; i < ARRAY_SIZE(test_vector); i++) {
		struct bt_mesh_light_hue_status status = {
			.current = test_vector[i].current_hue,
			.target = test_vector[i].expected_hue,
			.remaining_time = 100, /* Same as Transition Time set below. */
		};

		printk("Checking test_vector[%d]\n", i);

		light_hue_val = test_vector[i].current_hue;

		expect_hue_status_pub(&status);

		/* Use arbitrary Transition Time value to see different current and target values
		 * in the status message.
		 */
		struct bt_mesh_model_transition transition = {
			.time = 100,
		};
		struct bt_mesh_lvl_move_set lvl_move_set = {
			.delta = test_vector[i].lvl_move,
			.transition = &transition,
			.new_transaction = true,
		};
		struct bt_mesh_light_hue_move exp_hue_move = {
			.delta = test_vector[i].lvl_move,
			.transition = &transition,
		};

		expect_hue_move_set_cb(&exp_hue_move);
		zassert_ok(lvl_move_set_unack_send(&lvl_move_set));
	}
}

ZTEST_SUITE(light_hue_srv_test, NULL, NULL, setup, teardown, NULL);
