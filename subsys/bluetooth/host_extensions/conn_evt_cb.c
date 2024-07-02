/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <bluetooth/evt_cb.h>
#include <helpers/nrfx_gppi.h>
#include <bluetooth/hci_vs_sdc.h>

#include <nrfx_egu.h>

#if defined(CONFIG_BT_CONN_EVT_CB_EGU_INST_EGU0)
#define EGU_INST_IDX 0
#elif defined(CONFIG_BT_CONN_EVT_CB_EGU_INST_EGU1)
#define EGU_INST_IDX 1
#elif defined(CONFIG_BT_CONN_EVT_CB_EGU_INST_EGU2)
#define EGU_INST_IDX 2
#elif defined(CONFIG_BT_CONN_EVT_CB_EGU_INST_EGU3)
#define EGU_INST_IDX 3
#elif defined(CONFIG_BT_CONN_EVT_CB_EGU_INST_EGU4)
#define EGU_INST_IDX 4
#elif defined(CONFIG_BT_CONN_EVT_CB_EGU_INST_EGU5)
#define EGU_INST_IDX 5
#elif defined(CONFIG_BT_CONN_EVT_CB_EGU_INST_EGU020)
#define EGU_INST_IDX 020
#else
#error "Unknown EGU instance"
#endif

static nrfx_egu_t egu_inst = NRFX_EGU_INSTANCE(EGU_INST_IDX);

struct conn_evt_cb_cfg {
	struct bt_conn_evt_cb cb;
	struct bt_conn *conn;
	void *user_data;
	uint32_t prepare_time_us;

	uint8_t ppi_channel;
	uint16_t conn_interval_us;
};

static struct conn_evt_cb_cfg cfg[CONFIG_BT_MAX_CONN];
static struct k_work_delayable work_queue_items[CONFIG_BT_MAX_CONN];

static void work_handler(struct k_work *work)
{
	uint8_t index = ARRAY_INDEX(work_queue_items, work);

	cfg[index].cb.prepare(cfg[index].conn, cfg[index].user_data);
}

static void egu_event_handler(uint8_t event_idx, void *context)
{
	ARG_UNUSED(context);

	__ASSERT_NO_MSG(event_idx < ARRAY_SIZE(work_queue_items));
	__ASSERT_NO_MSG(cfg[event_idx].conn_interval_us > cfg[event_idx].prepare_time_us);

	const uint32_t timer_distance_us =
		cfg[event_idx].conn_interval_us - cfg[event_idx].prepare_time_us;

	k_work_schedule(&work_queue_items[event_idx], K_USEC(timer_distance_us));
}

static void on_conn_param_updated(struct bt_conn *conn,
				  uint16_t interval,
				  uint16_t latency,
				  uint16_t timeout)
{
	uint8_t index = bt_conn_index(conn);

	if (!cfg[index].cb.prepare) {
		return;
	}

	cfg[index].conn_interval_us = BT_CONN_INTERVAL_TO_US(interval);
}

static void on_disconnected(struct bt_conn *conn, uint8_t reason)
{
	uint8_t index = bt_conn_index(conn);

	if (!cfg[index].cb.prepare) {
		return;
	}

	cfg[index].cb.prepare = NULL;
	nrfx_egu_int_disable(&egu_inst, nrf_egu_channel_int_get(index));
	(void)k_work_cancel_delayable(&work_queue_items[index]);

	nrfx_err_t err = nrfx_gppi_channel_free(cfg[index].ppi_channel);

	__ASSERT_NO_MSG(err == NRFX_SUCCESS);
}

BT_CONN_CB_DEFINE(conn_params_updated_cb) = {
	.le_param_updated = on_conn_param_updated,
	.disconnected = on_disconnected,
};

static int setup_connection_event_trigger(struct conn_evt_cb_cfg *cfg, uint8_t index)
{
	int err;
	uint16_t conn_handle;

	err = bt_hci_get_conn_handle(cfg->conn, &conn_handle);
	if (err) {
		return err;
	}

	sdc_hci_cmd_vs_get_next_conn_event_counter_t get_next_conn_event_count_params = {
		.conn_handle = conn_handle,
	};
	sdc_hci_cmd_vs_get_next_conn_event_counter_return_t get_next_conn_event_count_ret_params;

	err = hci_vs_sdc_get_next_conn_event_counter(
		&get_next_conn_event_count_params, &get_next_conn_event_count_ret_params);
	if (err) {
		return err;
	}

	const sdc_hci_cmd_vs_set_conn_event_trigger_t params = {
		.conn_handle = conn_handle,
		.role = SDC_HCI_VS_CONN_EVENT_TRIGGER_ROLE_CONN,
		.ppi_ch_id = cfg->ppi_channel,
		.period_in_events = 1, /* Trigger every connection event. */
		.conn_evt_counter_start =
			get_next_conn_event_count_ret_params.next_conn_event_counter + 10,
		.task_endpoint = nrfx_egu_task_address_get(&egu_inst,
			nrf_egu_trigger_task_get(index)),
	};

	return hci_vs_sdc_set_conn_event_trigger(&params);
}

int bt_conn_evt_cb_register(struct bt_conn *conn,
			    struct bt_conn_evt_cb cb,
			    void *user_data,
			    uint32_t prepare_distance_us)
{
	int err;
	struct bt_conn_info info;
	uint8_t index = bt_conn_index(conn);

	if (cfg[index].cb.prepare) {
		return -EALREADY;
	}

	err = bt_conn_get_info(conn, &info);
	if (err) {
		return err;
	}

	if (nrfx_gppi_channel_alloc(&cfg[index].ppi_channel) != NRFX_SUCCESS) {
		return -ENOMEM;
	}

	cfg[index].cb = cb;
	cfg[index].conn_interval_us = BT_CONN_INTERVAL_TO_US(info.le.interval);
	cfg[index].conn = conn;
	cfg[index].prepare_time_us = prepare_distance_us;
	cfg[index].user_data = user_data;

	k_work_init_delayable(&work_queue_items[index], work_handler);
	nrfx_egu_int_enable(&egu_inst, nrf_egu_channel_int_get(index));

	setup_connection_event_trigger(&cfg[index], index);

	return 0;
}

static int driver_init(const struct device *dev)
{
	nrfx_err_t err;

	err = nrfx_egu_init(&egu_inst,
			    CONFIG_BT_CONN_EVT_CB_EGU_TRIGGER_ISR_PRIO,
			    egu_event_handler,
			    NULL);
	if (err != NRFX_SUCCESS) {
		return -EALREADY;
	}

	IRQ_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_EGU_INST_GET(EGU_INST_IDX)),
		    CONFIG_BT_CONN_EVT_CB_EGU_TRIGGER_ISR_PRIO,
		    nrfx_isr,
		    NRFX_EGU_INST_HANDLER_GET(EGU_INST_IDX),
		    0);
	irq_enable(NRFX_IRQ_NUMBER_GET(NRF_EGU_INST_GET(EGU_INST_IDX)));

	return 0;
}

DEVICE_DEFINE(conn_event_notification, "conn_evt_cb",
	      driver_init, NULL,
	      NULL, NULL,
	      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
	      NULL);
