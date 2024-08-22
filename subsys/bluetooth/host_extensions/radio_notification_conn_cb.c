/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <bluetooth/radio_notification_cb.h>
#include <bluetooth/hci_vs_sdc.h>

#include <nrfx_egu.h>

#if defined(CONFIG_BT_RADIO_NOTIFICATION_CONN_CB_EGU_INST_EGU0)
#define EGU_INST_IDX 0
#elif defined(CONFIG_BT_RADIO_NOTIFICATION_CONN_CB_EGU_INST_EGU1)
#define EGU_INST_IDX 1
#elif defined(CONFIG_BT_RADIO_NOTIFICATION_CONN_CB_EGU_INST_EGU2)
#define EGU_INST_IDX 2
#elif defined(CONFIG_BT_RADIO_NOTIFICATION_CONN_CB_EGU_INST_EGU3)
#define EGU_INST_IDX 3
#elif defined(CONFIG_BT_RADIO_NOTIFICATION_CONN_CB_EGU_INST_EGU4)
#define EGU_INST_IDX 4
#elif defined(CONFIG_BT_RADIO_NOTIFICATION_CONN_CB_EGU_INST_EGU5)
#define EGU_INST_IDX 5
#elif defined(CONFIG_BT_RADIO_NOTIFICATION_CONN_CB_EGU_INST_EGU020)
#define EGU_INST_IDX 020
#else
#error "Unknown EGU instance"
#endif

static nrfx_egu_t egu_inst = NRFX_EGU_INSTANCE(EGU_INST_IDX);
static uint32_t configured_prepare_distance_us;
static const struct bt_radio_notification_conn_cb *registered_cb;

static struct bt_conn *conn_pointers[CONFIG_BT_MAX_CONN];
static struct k_work_delayable work_queue_items[CONFIG_BT_MAX_CONN];

static uint32_t conn_interval_us_get(const struct bt_conn *conn)
{
	struct bt_conn_info info;

	if (bt_conn_get_info(conn, &info)) {
		return 0;
	}

	return BT_CONN_INTERVAL_TO_US(info.le.interval);
}

static void work_handler(struct k_work *work)
{
	struct bt_conn_info info;
	uint8_t index = ARRAY_INDEX(work_queue_items, work);
	struct bt_conn *conn = conn_pointers[index];

	if (bt_conn_get_info(conn, &info)) {
		return;
	}

	if (info.state == BT_CONN_STATE_CONNECTED) {
		registered_cb->prepare(conn);

		uint32_t conn_interval_us = conn_interval_us_get(conn);

		/* Schedule the callback in case the connection event does not occur because
		 * of peripheral latency or scheduling conflicts.
		 */
		k_work_reschedule(&work_queue_items[index], K_USEC(conn_interval_us));
	}
}

static void egu_isr_handler(uint8_t event_idx, void *context)
{
	ARG_UNUSED(context);

	__ASSERT_NO_MSG(event_idx < ARRAY_SIZE(work_queue_items));

	struct bt_conn *conn = conn_pointers[event_idx];
	uint32_t conn_interval_us = conn_interval_us_get(conn);

	__ASSERT_NO_MSG(conn_interval_us > configured_prepare_distance_us);

	const uint32_t timer_distance_us =
		conn_interval_us - configured_prepare_distance_us;

	k_work_reschedule(&work_queue_items[event_idx], K_USEC(timer_distance_us));
}

static int setup_event_start_task(struct bt_conn *conn)
{
	int err;
	uint8_t conn_index;
	uint16_t conn_handle;

	err = bt_hci_get_conn_handle(conn, &conn_handle);
	if (err) {
		return err;
	}

	conn_index = bt_conn_index(conn);

	const sdc_hci_cmd_vs_set_event_start_task_t params = {
		.handle = conn_handle,
		.handle_type = SDC_HCI_VS_SET_EVENT_START_TASK_HANDLE_TYPE_CONN,
		.task_address = nrfx_egu_task_address_get(&egu_inst,
			nrf_egu_trigger_task_get(conn_index)),
	};

	return hci_vs_sdc_set_event_start_task(&params);
}

static void on_connected(struct bt_conn *conn, uint8_t conn_err)
{
	if (!registered_cb) {
		return;
	}

	if (conn_err) {
		return;
	}

	uint8_t conn_index = bt_conn_index(conn);

	conn_pointers[conn_index] = conn;

	setup_event_start_task(conn);
}

BT_CONN_CB_DEFINE(conn_params_updated_cb) = {
	.connected = on_connected,
};

int bt_radio_notification_conn_cb_register(const struct bt_radio_notification_conn_cb *cb,
					   uint32_t prepare_distance_us)
{
	if (registered_cb) {
		return -EALREADY;
	}

	for (size_t i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		nrfx_egu_int_enable(&egu_inst, nrf_egu_channel_int_get(i));
		k_work_init_delayable(&work_queue_items[i], work_handler);
	}

	registered_cb = cb;
	configured_prepare_distance_us = prepare_distance_us;

	return 0;
}

static int driver_init(const struct device *dev)
{
	nrfx_err_t err;

	err = nrfx_egu_init(&egu_inst,
			    CONFIG_BT_RADIO_NOTIFICATION_CONN_CB_EGU_TRIGGER_ISR_PRIO,
			    egu_isr_handler,
			    NULL);
	if (err != NRFX_SUCCESS) {
		return -EALREADY;
	}

	IRQ_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_EGU_INST_GET(EGU_INST_IDX)),
		    CONFIG_BT_RADIO_NOTIFICATION_CONN_CB_EGU_TRIGGER_ISR_PRIO,
		    nrfx_isr,
		    NRFX_EGU_INST_HANDLER_GET(EGU_INST_IDX),
		    0);
	irq_enable(NRFX_IRQ_NUMBER_GET(NRF_EGU_INST_GET(EGU_INST_IDX)));

	return 0;
}

DEVICE_DEFINE(conn_event_notification, "radion_notification_conn_cb",
	      driver_init, NULL,
	      NULL, NULL,
	      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
	      NULL);
