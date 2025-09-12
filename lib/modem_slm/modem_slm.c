/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <assert.h>
#include <stdlib.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gpio.h>
#include <hal/nrf_gpio.h>
#include <zephyr/shell/shell.h>
#include <modem/modem_slm.h>
#include <zephyr/shell/shell_uart.h>
#include <zephyr/shell/shell_rtt.h>
#include <zephyr/pm/device.h>
#include <zephyr/logging/log.h>


LOG_MODULE_REGISTER(mdm_slm, CONFIG_MODEM_SLM_LOG_LEVEL);

#define UART_RX_MARGIN_MS	10
#define UART_RX_TIMEOUT_US      2000
#define UART_ERROR_DELAY_MS     500
#define UART_TX_TIMEOUT_US      100000

/* SLM has formatted AT response based on TS 27.007 */
#define AT_CMD_OK_STR    "\r\nOK\r\n"
#define AT_CMD_ERROR_STR "\r\nERROR\r\n"
#define AT_CMD_CMS_STR   "\r\n+CMS ERROR:"
#define AT_CMD_CME_STR   "\r\n+CME ERROR:"

static const struct device *uart_dev = DEVICE_DT_GET(DT_CHOSEN(ncs_slm_uart));

static struct k_work_delayable rx_process_work;
struct rx_buf_t {
	atomic_t ref_counter;
	uint8_t buf[CONFIG_MODEM_SLM_UART_RX_BUF_SIZE];
};
BUILD_ASSERT(CONFIG_MODEM_SLM_AT_CMD_RESP_MAX_SIZE > CONFIG_MODEM_SLM_UART_RX_BUF_SIZE);

/* Slabs for RX buffer. */
#define UART_SLAB_BLOCK_SIZE sizeof(struct rx_buf_t)
#define UART_SLAB_BLOCK_COUNT CONFIG_SLM_UART_RX_BUF_COUNT
#define UART_SLAB_ALIGNMENT sizeof(uint32_t)
BUILD_ASSERT(UART_SLAB_BLOCK_SIZE % UART_SLAB_ALIGNMENT == 0);
K_MEM_SLAB_DEFINE_STATIC(rx_slab, UART_SLAB_BLOCK_SIZE, CONFIG_MODEM_SLM_UART_RX_BUF_COUNT,
			 UART_SLAB_ALIGNMENT);

/* Event queue for RX buffer usage. */
struct rx_event_t {
	uint8_t *buf;
	size_t len;
};
#define UART_RX_EVENT_COUNT CONFIG_MODEM_SLM_UART_RX_BUF_COUNT
K_MSGQ_DEFINE(rx_event_queue, sizeof(struct rx_event_t), UART_RX_EVENT_COUNT, 1);

/* Ring buffer for TX data. */
RING_BUF_DECLARE(tx_buf, CONFIG_MODEM_SLM_UART_TX_BUF_SIZE);
static K_SEM_DEFINE(tx_done_sem, 0, 1);
static K_SEM_DEFINE(uart_disabled_sem, 0, 1);

enum modem_slm_uart_state {
	MODEM_SLM_TX_ENABLED_BIT,
	MODEM_SLM_RX_ENABLED_BIT,
	MODEM_SLM_RX_RECOVERY_BIT,
	MODEM_SLM_RX_RECOVERY_DISABLED_BIT
};
static atomic_t uart_state;

static slm_ri_handler_t ri_handler;

static K_SEM_DEFINE(at_rsp, 0, 1);

static slm_data_handler_t data_handler;
static enum at_cmd_state slm_at_state;

#if defined(CONFIG_MODEM_SLM_SHELL)
static const struct shell *global_shell;
static const char at_usage_str[] = "Usage: slm <at_command>";
#endif

extern void slm_monitor_dispatch(const char *notif, size_t len);
extern char *strnstr(const char *haystack, const char *needle, size_t haystack_sz);
static int dtr_uart_enable(void);

struct dtr_config {

	/* DTR */
	const struct gpio_dt_spec dtr_gpio;

	/* RI */
	const struct gpio_dt_spec ri_gpio;

	/* Automatically activate DTR UART from RI and disable it after inactivity. */
	bool automatic;
	k_timeout_t inactivity;

	/* Current DTR state. */
	bool active;

	/* Work items for enabling and disabling DTR UART. */
	struct k_work dtr_uart_enable_work;
	struct k_work_delayable dtr_uart_disable_work;
};

static struct dtr_config dtr_config = {
	.dtr_gpio = GPIO_DT_SPEC_GET_OR(DT_NODELABEL(dte_dtr), dtr_gpios, {0}),
	.ri_gpio = GPIO_DT_SPEC_GET_OR(DT_NODELABEL(dte_dtr), ri_gpios, {0}),
};

static void gpio_cb_func(const struct device *dev, struct gpio_callback *gpio_cb, uint32_t pins);
static struct gpio_callback gpio_cb;

static inline struct rx_buf_t *block_start_get(uint8_t *buf)
{
	size_t block_num;

	/* Find the correct block. */
	block_num = (((size_t)buf - offsetof(struct rx_buf_t, buf) - (size_t)rx_slab.buffer) /
		     UART_SLAB_BLOCK_SIZE);

	return (struct rx_buf_t *) &rx_slab.buffer[block_num * UART_SLAB_BLOCK_SIZE];
}

static struct rx_buf_t *buf_alloc(void)
{
	struct rx_buf_t *buf;
	int err;

	err = k_mem_slab_alloc(&rx_slab, (void **) &buf, K_NO_WAIT);
	if (err) {
		return NULL;
	}

	atomic_set(&buf->ref_counter, 1);

	return buf;
}

static void buf_ref(void *buf)
{
	atomic_inc(&(block_start_get(buf)->ref_counter));
}

static void buf_unref(void *buf)
{
	struct rx_buf_t *uart_buf = block_start_get(buf);
	atomic_t ref_counter = atomic_dec(&uart_buf->ref_counter);

	/* atomic_dec returns the previous value of the atomic. */
	if (ref_counter == 1) {
		k_mem_slab_free(&rx_slab, (void *)uart_buf);
	}
}

static int rx_enable(void)
{
	struct rx_buf_t *buf;
	int ret;

	if (atomic_test_bit(&uart_state, MODEM_SLM_RX_ENABLED_BIT)) {
		return 0;
	}

	buf = buf_alloc();
	if (!buf) {
		LOG_DBG("Failed to allocate RX buffer");
		return -ENOMEM;
	}

	ret = uart_rx_enable(uart_dev, buf->buf, sizeof(buf->buf), UART_RX_TIMEOUT_US);
	if (ret) {
		LOG_WRN("uart_rx_enable failed: %d", ret);
		buf_unref(buf->buf);
		return ret;
	}

	atomic_set_bit(&uart_state, MODEM_SLM_RX_ENABLED_BIT);

	return 0;
}

static int rx_disable(void)
{
	int err;

	atomic_set_bit(&uart_state, MODEM_SLM_RX_RECOVERY_DISABLED_BIT);

	while (atomic_test_bit(&uart_state, MODEM_SLM_RX_RECOVERY_BIT)) {
		/* Wait until possible recovery is complete. */
		k_sleep(K_MSEC(10));
	}

	if (!atomic_test_bit(&uart_state, MODEM_SLM_RX_ENABLED_BIT)) {
		return 0;
	}

	k_sem_reset(&uart_disabled_sem);

	err = uart_rx_disable(uart_dev);
	if (err) {
		LOG_ERR("UART RX disable failed: %d", err);
		return err;
	}

	/* Wait until RX is actually disabled. */
	k_sem_take(&uart_disabled_sem, K_MSEC(100));
	atomic_clear_bit(&uart_state, MODEM_SLM_RX_ENABLED_BIT);

	return 0;
}

static void rx_recovery(void)
{
	int err;

	if (atomic_test_bit(&uart_state, MODEM_SLM_RX_RECOVERY_DISABLED_BIT)) {
		return;
	}

	atomic_set_bit(&uart_state, MODEM_SLM_RX_RECOVERY_BIT);

	err = rx_enable();
	if (err) {
		k_work_schedule(&rx_process_work, K_MSEC(UART_RX_MARGIN_MS));
	}

	atomic_clear_bit(&uart_state, MODEM_SLM_RX_RECOVERY_BIT);
}

/* Attempts to find AT responses in the UART buffer. */
static size_t parse_at_response(const char *data, size_t datalen)
{
	/* SLM AT responses are formatted based on TS 27.007. */
	static const char * const at_responses[] = {
		[AT_CMD_OK] = "\r\nOK\r\n",
		[AT_CMD_ERROR] = "\r\nERROR\r\n",
		[AT_CMD_ERROR_CMS] = "\r\n+CMS ERROR:",
		[AT_CMD_ERROR_CME] = "\r\n+CME ERROR:"
	};
	const char *match = NULL;

	/* Look for all the AT state responses. */
	for (size_t at_state = 0; at_state != ARRAY_SIZE(at_responses); ++at_state) {
		match = strnstr(data, at_responses[at_state], datalen);
		if (match) {
			if (at_state == AT_CMD_OK || at_state == AT_CMD_ERROR) {
				/* Found a match. */
				slm_at_state = at_state;
				return match - data + strlen(at_responses[at_state]);
			}
			/* Search for the "\r\n" at the end of the response. */
			match += strlen(at_responses[at_state]);
			match = strnstr(match, "\r\n", datalen - (match - data));
			if (match) {
				/* Found a match. */
				slm_at_state = at_state;
				return match - data + 2;
			}
		}
	}

	return 0;
}

static void response_handler(const uint8_t *data, const size_t len)
{
	static uint8_t at_cmd_resp[CONFIG_MODEM_SLM_AT_CMD_RESP_MAX_SIZE];
	static size_t resp_len;
	size_t copy_len;

	LOG_HEXDUMP_DBG(data, len, "RX");

	copy_len = MIN(len, sizeof(at_cmd_resp) - resp_len);
	memcpy(at_cmd_resp + resp_len, data, copy_len);
	resp_len += copy_len;

	if (slm_at_state == AT_CMD_PENDING) {
		size_t processed = parse_at_response(at_cmd_resp, resp_len);

		if (processed == 0 && resp_len == sizeof(at_cmd_resp)) {
			LOG_ERR("AT-response overflow. Increase "
				"CONFIG_MODEM_SLM_AT_CMD_RESP_MAX_SIZE");
			processed = resp_len;
		}
		if (processed > 0) {
#if defined(CONFIG_MODEM_SLM_SHELL)
			shell_print(global_shell, "%.*s", processed, (char *)at_cmd_resp);
#endif
			if (processed < resp_len) {
				/* In case there is data or unsolicited notification
				 * after the AT-command response, shift the remaining data
				 * to the beginning of the buffer.
				 */
				memmove(at_cmd_resp, at_cmd_resp + processed, resp_len - processed);
			}
			resp_len -= processed;

			/* Copy the possibly remaining data to the buffer. */
			if (copy_len < len) {
				assert((sizeof(at_cmd_resp) - resp_len) >= (len - copy_len));

				memcpy(at_cmd_resp + resp_len, data + copy_len, len - copy_len);
				resp_len += len - copy_len;
			}

			k_sem_give(&at_rsp);
		}
	}

	if (slm_at_state != AT_CMD_PENDING && resp_len > 0) {
		slm_monitor_dispatch((const char *)at_cmd_resp, resp_len);

#if defined(CONFIG_MODEM_SLM_SHELL)
		shell_print(global_shell, "%.*s", resp_len, (char *)at_cmd_resp);
#endif
		resp_len = 0;
	}

	if (data_handler) {
		data_handler(data, len);
	}
}

static void rx_process(struct k_work *work)
{
	struct rx_event_t rx_event;

	while (k_msgq_get(&rx_event_queue, &rx_event, K_NO_WAIT) == 0) {
		response_handler(rx_event.buf, rx_event.len);
		buf_unref(rx_event.buf);
	}

	rx_recovery();
}

static int tx_start(void)
{
	uint8_t *buf;
	size_t len;
	int err;

	if (!atomic_test_bit(&uart_state, MODEM_SLM_TX_ENABLED_BIT)) {
		return -EAGAIN;
	}

	len = ring_buf_get_claim(&tx_buf, &buf, ring_buf_capacity_get(&tx_buf));
	err = uart_tx(uart_dev, buf, len, UART_TX_TIMEOUT_US);
	if (err) {
		LOG_ERR("UART TX error: %d", err);
		ring_buf_get_finish(&tx_buf, 0);
		return err;
	}

	return 0;
}

/* Write the data to tx_buffer and trigger sending. */
static int tx_write(const uint8_t *data, size_t len, bool flush)
{
	size_t ret;
	size_t sent = 0;
	int err;

	LOG_HEXDUMP_DBG(data, len, "TX");

	if (dtr_config.automatic && !dtr_config.active) {
		err = dtr_uart_enable();
		if (err) {
			LOG_ERR("Failed to enable DTR (%d).", err);
		}
	}

	while (sent < len) {
		ret = ring_buf_put(&tx_buf, data + sent, len - sent);
		if (ret) {
			sent += ret;
		} else {
			/* Buffer full, block and start TX. */
			if (atomic_test_bit(&uart_state, MODEM_SLM_TX_ENABLED_BIT) == 0) {
				LOG_ERR("TX disabled, %zu bytes dropped", len - sent);
				return -EIO;
			}
			k_sem_take(&tx_done_sem, K_FOREVER);
			err = tx_start();
			if (err) {
				LOG_ERR("TX buf overflow, %d dropped. Unable to send: %d",
					len - sent,
					err);
				k_sem_give(&tx_done_sem);
				return err;
			}
		}
	}

	if (flush) {
		if (atomic_test_bit(&uart_state, MODEM_SLM_TX_ENABLED_BIT) == 0) {
			LOG_INF("TX disabled, data will be sent when enabled");
			return -EAGAIN;
		}
		if (k_sem_take(&tx_done_sem, K_NO_WAIT) == 0) {
			err = tx_start();
			if (err) {
				LOG_ERR("tx_start failed: %d", err);
				k_sem_give(&tx_done_sem);
				return err;
			}
		}
	}

	return 0;
}

static int tx_enable(void)
{
	if (!atomic_test_and_set_bit(&uart_state, MODEM_SLM_TX_ENABLED_BIT)) {
		k_sem_give(&tx_done_sem);
	}
	return 0;
}

static int tx_disable(k_timeout_t timeout)
{
	int err;

	if (!atomic_test_and_clear_bit(&uart_state, MODEM_SLM_TX_ENABLED_BIT)) {
		return 0;
	}

	if (k_sem_take(&tx_done_sem, timeout) == 0) {
		return 0;
	}

	err = uart_tx_abort(uart_dev);
	if (!err) {
		LOG_INF("TX aborted");
	} else if (err != -EFAULT) {
		LOG_ERR("uart_tx_abort failed (%d).", err);
		return err;
	}

	return 0;
}

static void reschedule_disable(void)
{
	if (dtr_config.active && dtr_config.automatic) {
		/* Restart the inactivity timer. */
		k_work_reschedule(&dtr_config.dtr_uart_disable_work, dtr_config.inactivity);
	} else {
		/* Stop the inactivity timer. */
		k_work_cancel_delayable(&dtr_config.dtr_uart_disable_work);
	}
}

static void uart_callback(const struct device*, struct uart_event *evt, void*)
{
	struct rx_buf_t *buf;
	struct rx_event_t rx_event;
	int err;

	switch (evt->type) {
	case UART_TX_DONE:
		err = ring_buf_get_finish(&tx_buf, evt->data.tx.len);
		if (err) {
			LOG_ERR("UART_TX_%s, ring_buf_get_finish: %d", "DONE", err);
		}
		if (ring_buf_is_empty(&tx_buf)) {
			k_sem_give(&tx_done_sem);
			break;
		}
		err = tx_start();
		if (err) {
			LOG_ERR("tx_start failed: %d", err);
			k_sem_give(&tx_done_sem);
		}
		reschedule_disable();
		break;
	case UART_TX_ABORTED:
		err = ring_buf_get_finish(&tx_buf, evt->data.tx.len);
		if (err) {
			LOG_ERR("UART_TX_%s, ring_buf_get_finish: %d", "ABORTED", err);
		}
		LOG_WRN("UART_TX_ABORTED, dropped: %d bytes", ring_buf_size_get(&tx_buf));
		ring_buf_reset(&tx_buf);
		k_sem_give(&tx_done_sem);
		break;
	case UART_RX_RDY:
		buf_ref(evt->data.rx.buf);
		rx_event.buf = &evt->data.rx.buf[evt->data.rx.offset];
		rx_event.len = evt->data.rx.len;
		err = k_msgq_put(&rx_event_queue, &rx_event, K_NO_WAIT);
		if (err) {
			LOG_ERR("RX_RDY failure: %d, dropped: %d", err, evt->data.rx.len);
			buf_unref(evt->data.rx.buf);
			break;
		}
		k_work_submit((struct k_work *)&rx_process_work);
		reschedule_disable();
		break;
	case UART_RX_BUF_REQUEST:
		buf = buf_alloc();
		if (!buf) {
			break;
		}
		err = uart_rx_buf_rsp(uart_dev, buf->buf, sizeof(buf->buf));
		if (err) {
			LOG_WRN("Disabling UART RX: %d", err);
			buf_unref(buf);
		}
		break;
	case UART_RX_BUF_RELEASED:
		if (evt->data.rx_buf.buf) {
			buf_unref(evt->data.rx_buf.buf);
		}
		break;
	case UART_RX_DISABLED:
		k_sem_give(&uart_disabled_sem);
		k_work_submit((struct k_work *)&rx_process_work);
		break;
	default:
		break;
	}
}

static int uart_init(const struct device *uart_dev)
{
	int err;
	struct uart_config uart_conf;

	if (!device_is_ready(uart_dev)) {
		LOG_ERR("UART device not ready");
		return -ENODEV;
	}

	err = uart_config_get(uart_dev, &uart_conf);
	if (err) {
		LOG_ERR("uart_config_get: %d", err);
		return err;
	}

	LOG_INF("UART baud: %d d/p/s-bits: %d/%d/%d HWFC: %d",
		uart_conf.baudrate, uart_conf.data_bits, uart_conf.parity,
		uart_conf.stop_bits, uart_conf.flow_ctrl);

	/* Wait for the UART line to become valid */
	uint32_t start_time = k_uptime_get_32();

	do {
		err = uart_err_check(uart_dev);
		if (err) {
			uint32_t now = k_uptime_get_32();

			if (now - start_time > UART_ERROR_DELAY_MS) {
				LOG_ERR("UART check failed: %d", err);
				return -EIO;
			}
			k_sleep(K_MSEC(10));
		}
	} while (err);

	/* Register async handling callback */
	err = uart_callback_set(uart_dev, uart_callback, NULL);
	if (err) {
		LOG_ERR("Cannot set callback: %d", err);
		return -EFAULT;
	}

	/* Enable RX */
	atomic_clear(&uart_state);
	err = rx_enable();

	/* Enable TX */
	tx_enable();

	return err;
}

static int uart_power_state_action(enum pm_device_action action)
{
	enum pm_device_state state = PM_DEVICE_STATE_OFF;
	int err = pm_device_state_get(uart_dev, &state);

	if (err) {
		LOG_ERR("Failed to get PM device state: %d", err);
		return err;
	}

	if (action != PM_DEVICE_ACTION_RESUME && action != PM_DEVICE_ACTION_SUSPEND) {
		return -EOPNOTSUPP;
	}

	if ((action == PM_DEVICE_ACTION_RESUME && state == PM_DEVICE_STATE_ACTIVE) ||
	    (action == PM_DEVICE_ACTION_SUSPEND && state == PM_DEVICE_STATE_SUSPENDED)) {
		return 0;
	}

	err = pm_device_action_run(uart_dev, action);
	if (err) {
		LOG_ERR("Action %d failed on UART device: %d", action, err);
		return err;
	}

	return 0;
}

static int dtr_pin_set(bool level)
{
	int err;

	if (gpio_is_ready_dt(&dtr_config.dtr_gpio)) {
		err = gpio_pin_set_dt(&dtr_config.dtr_gpio, level);
		if (err) {
			LOG_ERR("Failed to set DTR pin: %d", err);
			return -EFAULT;
		}
	} else {
		LOG_WRN("DTR pin not configured");
		return -EFAULT;
	}
	return 0;
}

static int dtr_uart_disable(void)
{
	int err;

	/* Wait until TX is done and disable TX. */
	err = tx_disable(K_NO_WAIT);
	if (err) {
		LOG_ERR("TX disable failed (%d).", err);
		return err;
	}

	/* Ask SLM to disable UART. */
	err = dtr_pin_set(0);
	if (err) {
		LOG_ERR("Failed to set DTR pin (%d).", err);
		return err;
	}

	/* Optional: Wait for possible SLM TX to complete. */
	/* k_sleep(K_MSEC(100)); */

	/* Disable RX. */
	err = rx_disable();
	if (err) {
		LOG_ERR("RX disable failed (%d).", err);
		return err;
	}

	/* Power off UART module */
	err = uart_power_state_action(PM_DEVICE_ACTION_SUSPEND);
	if (err) {
		LOG_ERR("Failed to suspend UART (%d).", err);
		return err;
	}

	dtr_config.active = false;

	LOG_DBG("DTR UART disabled");
	return 0;
}

static void dtr_uart_disable_work_fn(struct k_work *work)
{
	int err;

	ARG_UNUSED(work);

	err = dtr_uart_disable();
	if (err) {
		LOG_ERR("Failed to disable DTR UART (%d).", err);
	}
}

static int dtr_uart_enable(void)
{
	int err;

	/* Power on UART module */
	err = uart_power_state_action(PM_DEVICE_ACTION_RESUME);
	if (err) {
		LOG_ERR("Failed to resume UART (%d).", err);
		return err;
	}

	/* Enable RX. */
	atomic_clear_bit(&uart_state, MODEM_SLM_RX_RECOVERY_DISABLED_BIT);
	err = rx_enable();
	if (err) {
		LOG_ERR("Failed to enable RX (%d).", err);
		return err;
	}

	/* Ask SLM to enable UART. */
	err = dtr_pin_set(1);
	if (err) {
		LOG_ERR("Failed to set DTR pin (%d).", err);
		return err;
	}

	/* Optional: Wait for SLM to be ready */
	/* k_sleep(K_MSEC(100)); */

	/* Enable TX. */
	err = tx_enable();
	if (err) {
		LOG_ERR("Failed to enable TX (%d).", err);
		return err;
	}

	/* Start TX in case there is pending data. */
	err = tx_start();
	if (err) {
		LOG_ERR("Failed to start TX (%d).", err);
		return err;
	}

	dtr_config.active = true;

	reschedule_disable();

	LOG_DBG("DTR UART enabled");
	return 0;
}

static void dtr_uart_enable_work_fn(struct k_work *work)
{
	int err;

	ARG_UNUSED(work);

	err = dtr_uart_enable();
	if (err) {
		LOG_ERR("Failed to enable DTR UART (%d).", err);
	}
}

static void gpio_cb_func(const struct device *dev, struct gpio_callback *gpio_cb, uint32_t pins)
{
	if ((BIT(dtr_config.ri_gpio.pin) & pins) == 0) {
		return;
	}

	if (dtr_config.automatic && !dtr_config.active) {
		/* Wake up the application */
		k_work_submit(&dtr_config.dtr_uart_enable_work);
	}

	if (ri_handler) {
		ri_handler();
	} else {
		LOG_INF("Ring Indicate");
	}
}


static int gpio_init(void)
{
	int err;

	if (gpio_is_ready_dt(&dtr_config.dtr_gpio)) {
		err = gpio_pin_configure_dt(&dtr_config.dtr_gpio, GPIO_OUTPUT_ACTIVE);
		if (err) {
			LOG_ERR("Failed to configure DTR pin: %d", err);
			return -EFAULT;
		}
	} else {
		LOG_WRN("DTR GPIO is not ready");
	}

	if (gpio_is_ready_dt(&dtr_config.ri_gpio)) {
		gpio_flags_t flags;
		nrf_gpio_pin_sense_t sense;

		err = gpio_pin_configure_dt(&dtr_config.ri_gpio, GPIO_INPUT);
		if (err) {
			LOG_ERR("GPIO config error: %d", err);
			return err;
		}

		err = gpio_pin_get_config_dt(&dtr_config.ri_gpio, &flags);
		if (err) {
			LOG_ERR("Failed to get RI pin config: %d", err);
			return err;
		}

		if (flags & GPIO_PULL_DOWN) {
			LOG_DBG("Wakeup sense %s", "high");
			sense = NRF_GPIO_PIN_SENSE_HIGH;
		} else {
			LOG_DBG("Wakeup sense %s", "low");
			sense = NRF_GPIO_PIN_SENSE_LOW;
		}
		nrf_gpio_cfg_sense_set(dtr_config.ri_gpio.pin, sense);


		gpio_init_callback(&gpio_cb, gpio_cb_func, BIT(dtr_config.ri_gpio.pin));
		err = gpio_add_callback_dt(&dtr_config.ri_gpio, &gpio_cb);
		if (err) {
			LOG_WRN("GPIO add callback error: %d", err);
			return err;
		}
		err = gpio_pin_interrupt_configure_dt(&dtr_config.ri_gpio, GPIO_INT_EDGE_TO_ACTIVE);
		if (err) {
			LOG_WRN("GPIO interrupt configure error: %d", err);
			return err;
		}
	} else {
		LOG_WRN("RI GPIO is not ready");
	}

	return 0;
}


int modem_slm_init(slm_data_handler_t handler, bool automatic, k_timeout_t inactivity)
{
	int err;
	static bool initialized;

	if (initialized) {
		return -EALREADY;
	}
	initialized = true;

	data_handler = handler;
	ri_handler = NULL;
	slm_at_state = AT_CMD_OK;

	err = gpio_init();
	if (err) {
		LOG_ERR("GPIO init (%d)", err);
		return -EFAULT;
	}

	err = uart_init(uart_dev);
	if (err) {
		LOG_ERR("UART init (%d)", err);
		return -EFAULT;
	}

	k_work_init_delayable(&rx_process_work, rx_process);

	k_work_init(&dtr_config.dtr_uart_enable_work, dtr_uart_enable_work_fn);
	k_work_init_delayable(&dtr_config.dtr_uart_disable_work, dtr_uart_disable_work_fn);

	/* Initialize shell pointer so it's available for printing in callbacks */
#if defined(CONFIG_SHELL_BACKEND_SERIAL)
	global_shell = shell_backend_uart_get_ptr();
#elif defined(CONFIG_SHELL_BACKEND_RTT)
	global_shell = shell_backend_rtt_get_ptr();
#endif

	dtr_config.automatic = automatic;
	dtr_config.inactivity = inactivity;
	dtr_config.active = true;

	reschedule_disable();

	return 0;
}

int modem_slm_uninit(void)
{
	int err;

	err = dtr_uart_disable();
	if (err) {
		LOG_ERR("Failed to disable DTR UART (%d).", err);
	}

	err = gpio_pin_configure_dt(&dtr_config.dtr_gpio, GPIO_DISCONNECTED);
	if (err) {
		LOG_ERR("Failed to disconnect DTR pin: %d", err);
	}

	data_handler = NULL;
	ri_handler = NULL;
	slm_at_state = AT_CMD_OK;

	return 0;
}

int modem_slm_register_ri_handler(slm_ri_handler_t handler)
{
	if (!gpio_is_ready_dt(&dtr_config.ri_gpio)) {
		LOG_WRN("RI GPIO is not ready");
		return -EFAULT;
	}

	ri_handler = handler;

	return 0;
}

int modem_slm_send_cmd(const char *const command, uint32_t timeout)
{
	int ret;

	slm_at_state = AT_CMD_PENDING;
	ret = tx_write(command, strlen(command), false);
	if (ret < 0) {
		return ret;
	}
	/* send AT command terminator */
#if defined(CONFIG_MODEM_SLM_CR_LF_TERMINATION)
	ret = tx_write("\r\n", 2, true);
#elif defined(CONFIG_MODEM_SLM_CR_TERMINATION)
	ret = tx_write("\r", 1, true);
#elif defined(CONFIG_MODEM_SLM_LF_TERMINATION)
	ret = tx_write("\n", 1, true);
#endif
	if (ret < 0) {
		return ret;
	}
	if (timeout != 0) {
		ret = k_sem_take(&at_rsp, K_SECONDS(timeout));
	} else {
		ret = k_sem_take(&at_rsp, K_FOREVER);
	}
	if (ret) {
		LOG_ERR("timeout");
		return ret;
	}

	return slm_at_state;
}

int modem_slm_send_data(const uint8_t *const data, size_t datalen)
{
	return tx_write(data, datalen, true);
}

void modem_slm_configure_dtr_uart(bool automatic, k_timeout_t inactivity)
{
	dtr_config.automatic = automatic;
	dtr_config.inactivity = inactivity;

	if (dtr_config.automatic && !dtr_config.active && !ring_buf_is_empty(&tx_buf)) {
		/* If automatic DTR UART is enabled and there is data to send, enable DTR UART. */
		k_work_submit(&dtr_config.dtr_uart_enable_work);
	} else {
		reschedule_disable();
	}
}

void modem_slm_disable_dtr_uart(void)
{
	modem_slm_configure_dtr_uart(false, K_NO_WAIT);
	k_work_reschedule(&dtr_config.dtr_uart_disable_work, K_NO_WAIT);
}

void modem_slm_enable_dtr_uart(void)
{
	modem_slm_configure_dtr_uart(false, K_NO_WAIT);
	k_work_submit(&dtr_config.dtr_uart_enable_work);
}

#if defined(CONFIG_MODEM_SLM_SHELL)

int modem_slm_shell(const struct shell *shell, size_t argc, char **argv)
{
	if (argc < 2) {
		shell_print(shell, "%s", at_usage_str);
		return 0;
	}

	return modem_slm_send_cmd(argv[1], 10);
}

int modem_slm_shell_slmsh_dtr_uart_disable(const struct shell *shell, size_t argc, char **argv)
{
	shell_print(shell, "Disable DTR UART.");
	modem_slm_disable_dtr_uart();

	return 0;
}

int modem_slm_shell_slmsh_dtr_uart_enable(const struct shell *shell, size_t argc, char **argv)
{
	shell_print(shell, "Enable DTR UART.");
	modem_slm_enable_dtr_uart();

	return 0;
}

int modem_slm_shell_slmsh_dtr_uart_auto(const struct shell *shell, size_t argc, char **argv)
{
	uint32_t timeout = 0;

	if (argc >= 2) {
		timeout = strtoul(argv[1], NULL, 10);
	}
	if (timeout == 0) {
		shell_print(shell, "Usage: slmsh uart auto <timeout in ms>");
		return -EINVAL;
	}

	modem_slm_configure_dtr_uart(true, K_MSEC(timeout));

	shell_print(shell, "Automatic DTR UART. Inactivity timeout %u ms", timeout);

	return 0;
}

SHELL_CMD_REGISTER(slm, NULL, "Send AT commands to SLM device", modem_slm_shell);

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_uart,
	SHELL_CMD(enable, NULL, "Enable DTR UART. Disable automatic handling.",
		  modem_slm_shell_slmsh_dtr_uart_enable),
	SHELL_CMD(disable, NULL, "Disable DTR UART. Disable automatic handling.",
		  modem_slm_shell_slmsh_dtr_uart_disable),
	SHELL_CMD(auto, NULL,
		  "(Default) Automatically enable DTR UART from RI. Disable DTR UART after "
		  "inactivity period. Default is 100ms.",
		  modem_slm_shell_slmsh_dtr_uart_auto),
	SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_slmsh,
	SHELL_CMD(uart, &sub_uart, "Enable/Disable DTR UART.", NULL),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(slmsh, &sub_slmsh, "Commands handled in SLM shell device", NULL);

#endif /* CONFIG_MODEM_SLM_SHELL */
