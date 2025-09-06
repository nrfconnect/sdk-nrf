/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <string.h>
#include <zephyr/console/console.h>
#include <zephyr/sys/printk.h>
#include <zephyr/types.h>
#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>
#include <bluetooth/scan.h>
#include <bluetooth/hci_vs_sdc.h>

#ifdef CONFIG_SOC_COMPATIBLE_NRF54LX
#include <nrfx_gpiote.h>
#include <helpers/nrfx_gppi.h>
#include <nrfx_dppi.h>
#include <zephyr/drivers/gpio.h>
#endif

#include <hal/nrf_egu.h>

#define EGU_NODE DT_ALIAS(egu)
#define NRF_EGU ((NRF_EGU_Type *) DT_REG_ADDR(EGU_NODE))

#define NUM_TRIGGERS   (50)
#define INTERVAL_10MS  (0x8)
#define INTERVAL_100MS (0x50)

#define ADVERTISING_UUID128 BT_UUID_128_ENCODE(0x038a803f, 0xf6b3, 0x420b, 0xa95a, 0x10cc7b32b6db)

#ifdef CONFIG_SOC_COMPATIBLE_NRF54LX
#define EGU_TRIGGER_GPIO_PIN (43) /* PORT1 pin 11 */
#endif

static volatile uint8_t timestamp_log_index = NUM_TRIGGERS;
static uint32_t timestamp_log[NUM_TRIGGERS];
static struct bt_conn *active_conn;
static volatile bool connection_established;
static struct k_work work;
static void work_handler(struct k_work *w);

#ifdef CONFIG_SOC_COMPATIBLE_NRF54LX
const nrfx_gpiote_t gpiote = NRFX_GPIOTE_INSTANCE(20);
const nrfx_dppi_t dppi_radio_domain = NRFX_DPPI_INSTANCE(10);
const nrfx_dppi_t dppi_gpio_domain = NRFX_DPPI_INSTANCE(20);
static uint8_t gppi_chan_egu_event;
static uint8_t dppi_chan_egu_event;
static uint8_t dppi_chan_gpio_trigger;
#endif

static struct {
	bool setup;
	bool active;
	uint16_t conn_evt_counter_start;
	uint16_t period_in_events;
} m_trigger_params;

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, ADVERTISING_UUID128),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static int setup_qos_conn_event(void);

static void egu_handler(const void *context)
{
	nrf_egu_event_clear(NRF_EGU, NRF_EGU_EVENT_TRIGGERED0);

	if (m_trigger_params.active && timestamp_log_index < NUM_TRIGGERS) {
		/* GPIO toggling occurs every m_trigger_params.period_in_events.
		 * EGU ISR is entered every event.
		 * As a simple approach, non-relevant EGU ISRs are just ignored.
		 */
		k_work_submit(&work);
	}

#ifdef CONFIG_SOC_COMPATIBLE_NRF54LX
	/* Instead of disabling/enabling the PPI channel it's of course also possible to
	 * just toggle the GPIO here, since this handler executes while the radio is
	 * transceiving.
	 *
	 * In a single-link scenario it's possible to keep track of events by implementing
	 * a simple counter here as well
	 */
	__ASSERT_NO_MSG(nrfx_dppi_channel_disable(&dppi_gpio_domain, dppi_chan_gpio_trigger)
		== NRFX_SUCCESS);
#endif
}

static void work_handler(struct k_work *w)
{
	timestamp_log[timestamp_log_index] = k_ticks_to_us_near32((uint32_t)k_uptime_ticks());
	timestamp_log_index++;
}

static int setup_connection_event_trigger(struct bt_conn *conn,
	bool enable, uint16_t conn_evt_counter_start, uint16_t period_in_events)
{
	int err;
	sdc_hci_cmd_vs_set_event_start_task_t cmd_params;
	uint16_t conn_handle;

	err = bt_hci_get_conn_handle(conn, &conn_handle);
	if (err) {
		printk("Failed obtaining conn_handle (err %d)\n", err);
		return err;
	}

	cmd_params.handle_type = SDC_HCI_VS_SET_EVENT_START_TASK_HANDLE_TYPE_CONN;
	cmd_params.handle = conn_handle;

	/* Configure event task to trigger NRF_EGU_TASK_TRIGGER0.
	 * This will generate a software interrupt.
	 */
	if (enable) {
		cmd_params.task_address =
			nrf_egu_task_address_get(NRF_EGU, NRF_EGU_TASK_TRIGGER0);

		IRQ_CONNECT(DT_IRQN(EGU_NODE), 5, egu_handler, 0, 0);
		nrf_egu_int_enable(NRF_EGU, NRF_EGU_INT_TRIGGERED0);
		NVIC_EnableIRQ(DT_IRQN(EGU_NODE));
#ifdef CONFIG_SOC_COMPATIBLE_NRF54LX
		if (nrfx_gppi_channel_alloc(&gppi_chan_egu_event) !=
		    NRFX_SUCCESS) {
			printk("Failed allocating gppi_chan_egu_event\n");
			return -ENOMEM;
		}

		if (nrfx_dppi_channel_alloc(&dppi_radio_domain, &dppi_chan_egu_event) !=
		    NRFX_SUCCESS) {
			printk("Failed allocating dppi_chan_egu_event\n");
			return -ENOMEM;
		}

		if (nrfx_dppi_channel_alloc(&dppi_gpio_domain, &dppi_chan_gpio_trigger) !=
		    NRFX_SUCCESS) {
			printk("Failed allocating dppi_chan_gpio_trigger\n");
			return -ENOMEM;
		}

		if (nrfx_gppi_edge_connection_setup(gppi_chan_egu_event,
						    &dppi_radio_domain,
						    dppi_chan_egu_event,
						    &dppi_gpio_domain,
						    dppi_chan_gpio_trigger) != NRFX_SUCCESS) {
			printk("Failed edge setup chan ready\n");
			return -ENOMEM;
		}

		nrf_egu_publish_set(NRF_EGU, NRF_EGU_EVENT_TRIGGERED0, dppi_chan_egu_event);

		uint8_t egu_triggered_gpiote_channel;

		const nrfx_gpiote_output_config_t gpiote_output_cfg =
			NRFX_GPIOTE_DEFAULT_OUTPUT_CONFIG;

		if (nrfx_gpiote_channel_alloc(&gpiote, &egu_triggered_gpiote_channel) !=
		    NRFX_SUCCESS) {
			printk("Failed allocating GPIOTE chan\n");
			return -ENOMEM;
		}

		printk("Allocated GPIOTE channel %u\n", egu_triggered_gpiote_channel);

		const nrfx_gpiote_task_config_t gpiote_task_cfg = {
			.task_ch = egu_triggered_gpiote_channel,
			.polarity = NRF_GPIOTE_POLARITY_TOGGLE,
			.init_val = NRF_GPIOTE_INITIAL_VALUE_LOW,
		};

		if (nrfx_gpiote_output_configure(&gpiote,
						 EGU_TRIGGER_GPIO_PIN,
						 &gpiote_output_cfg,
						 &gpiote_task_cfg)
		    != NRFX_SUCCESS) {
			printk("Failed configuring GPIOTE chan\n");
			return -ENOMEM;
		}

		nrf_gpiote_subscribe_set(
			gpiote.p_reg,
			nrfx_gpiote_out_task_address_get(
				&gpiote, EGU_TRIGGER_GPIO_PIN),
			dppi_chan_gpio_trigger);

		if (nrfx_dppi_channel_enable(&dppi_radio_domain, dppi_chan_egu_event)
			!= NRFX_SUCCESS) {
			printk("Failed chan enable dppi_chan_egu_event\n");
			return -ENOMEM;
		}

		nrfx_gppi_channels_enable(NRFX_BIT(gppi_chan_egu_event));
		nrfx_gpiote_out_task_enable(&gpiote, EGU_TRIGGER_GPIO_PIN);
#endif
		setup_qos_conn_event();
	} else {
		cmd_params.task_address = 0;
		nrf_egu_int_disable(NRF_EGU, NRF_EGU_INT_TRIGGERED0);
		NVIC_DisableIRQ(DT_IRQN(EGU_NODE));
#ifdef CONFIG_SOC_COMPATIBLE_NRF54LX
		nrfx_gpiote_out_task_disable(&gpiote, EGU_TRIGGER_GPIO_PIN);
#endif
	}

	err = hci_vs_sdc_set_event_start_task(&cmd_params);
	if (err) {
		printk("Error for command hci_vs_sdc_set_event_start_task() (%d)\n",
		       err);
		return err;
	}

	printk("Successfully configured event trigger\n");

	m_trigger_params.conn_evt_counter_start = conn_evt_counter_start;
	m_trigger_params.period_in_events = period_in_events;
	m_trigger_params.setup = enable;

	return 0;
}

static bool on_vs_evt(struct net_buf_simple *buf)
{
	uint8_t code;
	sdc_hci_subevent_vs_qos_conn_event_report_t *evt;

	code = net_buf_simple_pull_u8(buf);
	if (code != SDC_HCI_SUBEVENT_VS_QOS_CONN_EVENT_REPORT) {
		return false;
	}

	evt = (void *)buf->data;

	if (timestamp_log_index < NUM_TRIGGERS && m_trigger_params.setup) {

		const uint16_t next_event_counter = evt->event_counter + 1;

		if ((int16_t)(next_event_counter - m_trigger_params.conn_evt_counter_start) >= 0) {
			m_trigger_params.active = true;
		}

#ifdef CONFIG_SOC_COMPATIBLE_NRF54LX
		const uint16_t events_since_start = next_event_counter -
			m_trigger_params.conn_evt_counter_start;

		if (m_trigger_params.active &&
			((events_since_start % m_trigger_params.period_in_events) == 0)) {
			if (nrfx_dppi_channel_enable(&dppi_gpio_domain, dppi_chan_gpio_trigger)
				!= NRFX_SUCCESS) {
				printk("Failed chan enable dppi_chan_gpio_trigger\n");
				return -ENOMEM;
			}
		}
#endif
	}

	return true;
}

static int setup_qos_conn_event(void)
{
	int err;
	sdc_hci_cmd_vs_qos_conn_event_report_enable_t cmd_enable;

	err = bt_hci_register_vnd_evt_cb(on_vs_evt);
	if (err) {
		printk("Failed registering vendor specific callback (err %d)\n",
		       err);
		return err;
	}

	cmd_enable.enable = true;

	err = hci_vs_sdc_qos_conn_event_report_enable(&cmd_enable);
	if (err) {
		printk("Could not enable QoS reports (err %d)\n", err);
		return err;
	}

	printk("Successfully enabled QoS reports\n");

	return 0;
}

static int change_connection_interval(struct bt_conn *conn, uint16_t new_interval)
{
	int err;
	struct bt_le_conn_param *params = BT_LE_CONN_PARAM(new_interval, new_interval, 0, 400);

	err = bt_conn_le_param_update(conn, params);

	if (err) {
		printk("Error when updating connection parameters (%d)\n", err);
	}

	return err;
}

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err) {
		printk("Failed to connect to %s, 0x%02x %s\n", addr, conn_err,
		       bt_hci_err_to_str(conn_err));

		if (conn == active_conn) {
			bt_conn_unref(active_conn);
			active_conn = NULL;
		}

		return;
	}

	printk("Connected: %s\n", addr);

	active_conn = bt_conn_ref(conn);
	connection_established = true;
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s, reason 0x%02x %s\n", addr, reason, bt_hci_err_to_str(reason));

	if (conn == active_conn) {
		bt_conn_unref(active_conn);
		active_conn = NULL;
	}
}

static void le_param_updated(struct bt_conn *conn, uint16_t interval, uint16_t latency,
			     uint16_t timeout)
{
	if (conn == active_conn) {
		printk("Connection parameters updated. New interval: %u ms\n",
					 interval * 1250 / 1000);
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.le_param_updated = le_param_updated,
};

static void adv_start(void)
{
	int err;

	err = bt_le_adv_start(
		BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONN,
			BT_GAP_ADV_FAST_INT_MIN_2,
			BT_GAP_ADV_FAST_INT_MAX_2,
			NULL),
		ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));

	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");
}

static void scan_filter_match(struct bt_scan_device_info *device_info,
			      struct bt_scan_filter_match *filter_match, bool connectable)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));

	printk("Filters matched. Address: %s connectable: %d\n", addr, connectable);
}

static void scan_connecting_error(struct bt_scan_device_info *device_info)
{
	printk("Connecting failed\n");
}

BT_SCAN_CB_INIT(scan_cb, scan_filter_match, NULL, scan_connecting_error, NULL);

static void scan_start(void)
{
	int err;

	struct bt_le_scan_param scan_param = {
		.type = BT_LE_SCAN_TYPE_PASSIVE,
		.options = BT_LE_SCAN_OPT_FILTER_DUPLICATE,
		.interval = 0x0010,
		.window = 0x0010,
	};

	struct bt_le_conn_param *conn_param =
		BT_LE_CONN_PARAM(INTERVAL_100MS, INTERVAL_100MS, 0, 400);

	struct bt_scan_init_param scan_init = {
		.connect_if_match = true,
		.scan_param = &scan_param,
		.conn_param = conn_param,
	};

	bt_scan_init(&scan_init);
	bt_scan_cb_register(&scan_cb);

	err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID,
				 BT_UUID_DECLARE_128(ADVERTISING_UUID128));

	if (err) {
		printk("Scanning filters cannot be set (err %d)\n", err);
		return;
	}

	err = bt_scan_filter_enable(BT_SCAN_UUID_FILTER, false);
	if (err) {
		printk("Filters cannot be turned on (err %d)\n", err);
	}

	err = bt_scan_start(BT_SCAN_TYPE_SCAN_PASSIVE);
	if (err) {
		printk("Starting scanning failed (err %d)\n", err);
		return;
	}

	printk("Scanning successfully started\n");
}

int main(void)
{
	int err;

	k_work_init(&work, work_handler);
	console_init();
	printk("Starting Event Trigger Sample.\n");

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	while (true) {
		printk("Choose device role - type c (central) or p (peripheral): ");

		char input_char = console_getchar();

		printk("\n");

		if (input_char == 'c') {
			printk("Central. Starting scanning\n");
			scan_start();
			break;
		} else if (input_char == 'p') {
			printk("Peripheral. Starting advertising\n");
			adv_start();
			break;
		}

		printk("Invalid role\n");
	}

	while (true) {
		if (connection_established) {
			printk("Connection established.\n");
			connection_established = false;
			break;
		}
	}

	for (;;) {
		printk("Press any key to switch to a 10ms connection interval and set up "
		       "event trigger:\n");

		(void)console_getchar();
		printk("\n");

		timestamp_log_index = 0;

		change_connection_interval(active_conn, INTERVAL_10MS);

		err = setup_connection_event_trigger(active_conn, true, 150, 2);

		if (err) {
			printk("Could not set up event trigger. (err %d)\n", err);
			return 0;
		}

		while (timestamp_log_index < NUM_TRIGGERS) {
			;
		}

		err = setup_connection_event_trigger(active_conn, false, 0, 0);

		if (err) {
			printk("Could not disable event trigger. (err %d)\n", err);
			return 0;
		}

		printk("Printing event trigger log.\n"
		       "+-------------+----------------+----------------------------------+\n"
		       "| Trigger no. | Timestamp (us) | Time since previous trigger (us) |\n"
		       "+-------------+----------------+----------------------------------+\n");

		for (uint16_t i = 1; i < NUM_TRIGGERS; i++) {
			printk("| %11d | %11u us | %29u us |\n", i, timestamp_log[i],
			       timestamp_log[i] - timestamp_log[i - 1]);
		}

		printk("+-------------+----------------+----------------------------------+\n");

		printk("\n");

		change_connection_interval(active_conn, INTERVAL_100MS);
	}
}
