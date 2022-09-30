/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <zboss_api.h>

#include <zephyr/kernel.h>
#include <zb_nrf_platform.h>


#define CONFIG_ZIGBEE_APP_CB_QUEUE_LENGTH 4


/**
 * Enumeration representing type of application callback to execute from ZBOSS
 * context.
 */
enum zb_callback_type_e {
	ZB_CALLBACK_TYPE_SINGLE_PARAM,
	ZB_CALLBACK_TYPE_TWO_PARAMS,
};

/**
 * Type definition of element of the application callback and alarm queue.
 */
struct zb_app_cb_s {
	enum zb_callback_type_e type;
	zb_callback_t func;
	zb_uint8_t param;
	zb_uint16_t user_param;
};


/**
 * Message queue, that is used to pass ZBOSS callbacks and alarms from
 * ISR and other threads to ZBOSS main loop context.
 */
K_MSGQ_DEFINE(zb_app_cb_msgq, sizeof(struct zb_app_cb_s),
	      CONFIG_ZIGBEE_APP_CB_QUEUE_LENGTH, 4);

/**
 * Work queue that will schedule processing of callbacks from the message queue.
 */
static struct k_work zb_app_cb_work;


static void zb_app_cb_process_schedule(struct k_work *item)
{
	struct zb_app_cb_s new_app_cb = {0};

	/**
	 * Process all requests.
	 */
	while (!k_msgq_peek(&zb_app_cb_msgq, &new_app_cb)) {
		switch (new_app_cb.type) {
		case ZB_CALLBACK_TYPE_SINGLE_PARAM:
			new_app_cb.func(new_app_cb.param);
			break;
		case ZB_CALLBACK_TYPE_TWO_PARAMS:
			((zb_callback2_t)new_app_cb.func)(
				new_app_cb.param,
				new_app_cb.user_param);
			break;
		default:
			zassert_unreachable(
				"Unimplemented Zigbee callback type");
			break;
		}

		/* Flush the element from the message queue. */
		k_msgq_get(&zb_app_cb_msgq, &new_app_cb, K_NO_WAIT);
	}
}


/**
 * Prototype defined in zb_nrf_platform.h
 */
zb_ret_t zigbee_schedule_callback(zb_callback_t func, zb_uint8_t param)
{
	struct zb_app_cb_s new_app_cb = {
		.type = ZB_CALLBACK_TYPE_SINGLE_PARAM,
		.func = func,
		.param = param,
	};

	if (k_msgq_put(&zb_app_cb_msgq, &new_app_cb, K_NO_WAIT)) {
		return RET_OVERFLOW;
	}

	k_work_submit(&zb_app_cb_work);

	return RET_OK;
}

/**
 * Prototype defined in zb_nrf_platform.h
 */
void zigbee_enable(void)
{
	/* Initialise work queue for processing app callbacks. */
	k_work_init(&zb_app_cb_work, zb_app_cb_process_schedule);
}
