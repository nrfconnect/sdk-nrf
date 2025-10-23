/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <assert.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/shell/shell.h>
#include <hal/nrf_gpio.h>
#include <modem/modem_slm.h>
#include <zephyr/shell/shell_uart.h>
#include <zephyr/shell/shell_rtt.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(mdm_slm, CONFIG_MODEM_SLM_LOG_LEVEL);

BUILD_ASSERT(CONFIG_MODEM_SLM_POWER_PIN >= 0, "Power pin not configured");

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

enum uart_recovery_state {
	RECOVERY_DISABLED,
	RECOVERY_IDLE,
	RECOVERY_ONGOING
};
static atomic_t recovery_state;
static K_SEM_DEFINE(at_rsp, 0, 1);

static slm_data_handler_t data_handler;
static enum at_cmd_state slm_at_state;

#if DT_HAS_CHOSEN(ncs_slm_gpio)
static const struct device *gpio_dev = DEVICE_DT_GET(DT_CHOSEN(ncs_slm_gpio));
#else
static const struct device *gpio_dev = DEVICE_DT_GET(DT_NODELABEL(gpio0));
#endif
static struct k_work_delayable gpio_power_pin_disable_work;
static slm_ind_handler_t ind_handler;
static slm_ind_handler_t ind_handler_backup;

#if defined(CONFIG_MODEM_SLM_SHELL)
static const struct shell *global_shell;
static const char at_usage_str[] = "Usage: slm <at_command>";
#endif

extern void slm_monitor_dispatch(const char *notif, size_t len);
extern char *strnstr(const char *haystack, const char *needle, size_t haystack_sz);

#if (CONFIG_MODEM_SLM_INDICATE_PIN >= 0)
static bool indicate_pin_enabled;

static void gpio_cb_func(const struct device *dev, struct gpio_callback *gpio_cb, uint32_t pins);
static struct gpio_callback gpio_cb;
#endif

static int indicate_pin_enable(void)
{
#if (CONFIG_MODEM_SLM_INDICATE_PIN >= 0)
	int err = 0;

	if (!indicate_pin_enabled) {
		err = gpio_pin_configure(gpio_dev, CONFIG_MODEM_SLM_INDICATE_PIN,
					 GPIO_INPUT | GPIO_PULL_UP | GPIO_ACTIVE_LOW);
		if (err) {
			LOG_ERR("GPIO config error: %d", err);
			return err;
		}

		gpio_init_callback(&gpio_cb, gpio_cb_func, BIT(CONFIG_MODEM_SLM_INDICATE_PIN));
		err = gpio_add_callback(gpio_dev, &gpio_cb);
		if (err) {
			LOG_WRN("GPIO add callback error: %d", err);
		}
		err = gpio_pin_interrupt_configure(gpio_dev, CONFIG_MODEM_SLM_INDICATE_PIN,
						   GPIO_INT_LEVEL_LOW);
		if (err) {
			LOG_WRN("GPIO interrupt configure error: %d", err);
		}
		indicate_pin_enabled = true;
		LOG_DBG("Indicate pin enabled");
	}
#endif
	return 0;
}

static void indicate_pin_disable(void)
{
#if (CONFIG_MODEM_SLM_INDICATE_PIN >= 0)
	if (indicate_pin_enabled) {
		gpio_remove_callback(gpio_dev, &gpio_cb);
		gpio_pin_interrupt_configure(gpio_dev, CONFIG_MODEM_SLM_INDICATE_PIN,
					     GPIO_INT_DISABLE);
		gpio_pin_configure(gpio_dev, CONFIG_MODEM_SLM_INDICATE_PIN, GPIO_DISCONNECTED);
		indicate_pin_enabled = false;
		LOG_DBG("Indicate pin disabled");
	}
#endif
}

static void gpio_power_pin_disable_work_fn(struct k_work *work)
{
	ARG_UNUSED(work);

	if (gpio_pin_set(gpio_dev, CONFIG_MODEM_SLM_POWER_PIN, 0) != 0) {
		LOG_WRN("GPIO set error");
	}
	/* When SLM is woken up, indicate pin must be enabled */
	(void)indicate_pin_enable();

	LOG_INF("Disable power pin");
}

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

	buf = buf_alloc();
	if (!buf) {
		LOG_DBG("Failed to allocate RX buffer");
		return -ENOMEM;
	}

	ret = uart_rx_enable(uart_dev, buf->buf, sizeof(buf->buf), UART_RX_TIMEOUT_US);
	if (ret) {
		LOG_WRN("uart_rx_enable failed: %d", ret);
		return ret;
	}

	return 0;
}

static int rx_disable(void)
{
	int err;

	/* Wait for possible rx_enable to complete. */
	if (atomic_set(&recovery_state, RECOVERY_DISABLED) == RECOVERY_ONGOING) {
		k_sleep(K_MSEC(10));
	}

	err = uart_rx_disable(uart_dev);
	if (err) {
		LOG_ERR("UART RX disable failed: %d", err);
		atomic_set(&recovery_state, RECOVERY_IDLE);
		return err;
	}

	return 0;
}

static void rx_recovery(void)
{
	int err;

	if (atomic_get(&recovery_state) != RECOVERY_ONGOING) {
		return;
	}

	err = rx_enable();
	if (err) {
		k_work_schedule(&rx_process_work, K_MSEC(UART_RX_MARGIN_MS));
		return;
	}

	atomic_cas(&recovery_state, RECOVERY_ONGOING, RECOVERY_IDLE);
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

	if (!IS_ENABLED(CONFIG_MODEM_SLM_SHELL) && data_handler != NULL) {
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

	while (sent < len) {
		ret = ring_buf_put(&tx_buf, data + sent, len - sent);
		if (ret) {
			sent += ret;
		} else {
			/* Buffer full, block and start TX. */
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

	if (flush && k_sem_take(&tx_done_sem, K_NO_WAIT) == 0) {
		err = tx_start();
		if (err) {
			LOG_ERR("tx_start failed: %d", err);
			k_sem_give(&tx_done_sem);
			return err;
		}
	}

	return 0;
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
		if (atomic_cas(&recovery_state, RECOVERY_IDLE, RECOVERY_ONGOING)) {
			k_work_submit((struct k_work *)&rx_process_work);
		}
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
	atomic_set(&recovery_state, RECOVERY_IDLE);
	err = rx_enable();

	/* Enable TX */
	k_sem_give(&tx_done_sem);

	return err;
}

#if (CONFIG_MODEM_SLM_INDICATE_PIN >= 0)
static struct gpio_callback gpio_cb;

static void gpio_cb_func(const struct device *dev, struct gpio_callback *gpio_cb, uint32_t pins)
{
	if ((BIT(CONFIG_MODEM_SLM_INDICATE_PIN) & pins) == 0) {
		return;
	}

	if (k_work_delayable_is_pending(&gpio_power_pin_disable_work)) {
		(void)k_work_cancel_delayable(&gpio_power_pin_disable_work);
		(void)gpio_pin_set(gpio_dev, CONFIG_MODEM_SLM_POWER_PIN, 0);
	} else {
		/* Disable indicate pin so that callbacks doesn't keep on coming. */
		indicate_pin_disable();
	}

	LOG_INF("Remote indication");
	if (ind_handler) {
		ind_handler();
	} else {
		LOG_WRN("Indicate PIN configured but slm_ind_handler_t not defined");
	}
}
#endif  /* CONFIG_MODEM_SLM_INDICATE_PIN */

static int gpio_init(void)
{
	int err;

	if (!device_is_ready(gpio_dev)) {
		LOG_ERR("GPIO controller not ready");
		return -ENODEV;
	}

	err = gpio_pin_configure(gpio_dev, CONFIG_MODEM_SLM_POWER_PIN,
				 GPIO_OUTPUT_INACTIVE | GPIO_ACTIVE_LOW);
	if (err) {
		LOG_ERR("GPIO config error: %d", err);
		return err;
	}

	err = indicate_pin_enable();

	return err;
}

int modem_slm_init(slm_data_handler_t handler)
{
	int err;

	if (handler != NULL && data_handler != NULL) {
		LOG_ERR("Already initialized");
		return -EFAULT;
	}

	data_handler = handler;
	ind_handler = NULL;
	ind_handler_backup = NULL;
	slm_at_state = AT_CMD_OK;

	err = gpio_init();
	if (err != 0) {
		LOG_WRN("Failed to init gpio");
		return err;
	}

	err = uart_init(uart_dev);
	if (err) {
		LOG_ERR("UART could not be initialized: %d", err);
		return -EFAULT;
	}

	k_work_init_delayable(&gpio_power_pin_disable_work, gpio_power_pin_disable_work_fn);
	k_work_init_delayable(&rx_process_work, rx_process);

	/* Initialize shell pointer so it's available for printing in callbacks */
#if defined(CONFIG_SHELL_BACKEND_SERIAL)
	global_shell = shell_backend_uart_get_ptr();
#elif defined(CONFIG_SHELL_BACKEND_RTT)
	global_shell = shell_backend_rtt_get_ptr();
#endif
	return 0;
}

int modem_slm_uninit(void)
{
	rx_disable();

	gpio_pin_configure(gpio_dev, CONFIG_MODEM_SLM_POWER_PIN, GPIO_DISCONNECTED);

	indicate_pin_disable();

	data_handler = NULL;
	ind_handler = NULL;
	ind_handler_backup = NULL;
	slm_at_state = AT_CMD_OK;

	return 0;
}

int modem_slm_register_ind(slm_ind_handler_t handler, bool wakeup)
{
#if (CONFIG_MODEM_SLM_INDICATE_PIN >= 0)
	if (ind_handler != NULL) {
		ind_handler_backup = ind_handler;
	}
	ind_handler = handler;

	if (wakeup) {
		/*
		 * Due to errata 4, Always configure PIN_CNF[n].INPUT before PIN_CNF[n].SENSE.
		 * At this moment indicate pin has already been configured as INPUT at init_gpio().
		 */
		nrf_gpio_cfg_sense_set(CONFIG_MODEM_SLM_INDICATE_PIN, NRF_GPIO_PIN_SENSE_LOW);
	}

	return 0;
#else
	return -EFAULT;
#endif
}

int modem_slm_power_pin_toggle(void)
{
	int err;

	if (k_work_delayable_is_pending(&gpio_power_pin_disable_work)) {
		return 0;
	}

	LOG_INF("Enable power pin");

	err = gpio_pin_set(gpio_dev, CONFIG_MODEM_SLM_POWER_PIN, 1);
	if (err) {
		LOG_ERR("GPIO set error: %d", err);
	} else {
		k_work_reschedule(
			&gpio_power_pin_disable_work,
			K_MSEC(CONFIG_MODEM_SLM_POWER_PIN_TIME));
	}

	return 0;
}

int modem_slm_send_cmd(const char *const command, uint32_t timeout)
{
	int ret;

	/* Enable indicate pin when command is sent. This should not be needed but is made
	 * currently to make sure it wouldn't be disabled forever.
	 */
	(void)indicate_pin_enable();

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

#if defined(CONFIG_MODEM_SLM_SHELL)

int modem_slm_shell(const struct shell *shell, size_t argc, char **argv)
{
	if (argc < 2) {
		shell_print(shell, "%s", at_usage_str);
		return 0;
	}

	return modem_slm_send_cmd(argv[1], 10);
}

int modem_slm_shell_slmsh_powerpin(const struct shell *shell, size_t argc, char **argv)
{
	int err;

	err = modem_slm_power_pin_toggle();
	if (err) {
		LOG_ERR("Failed to toggle power pin");
	}
	return 0;
}

int modem_slm_shell_slmsh_indicate_enable(const struct shell *shell, size_t argc, char **argv)
{
	LOG_INF("Enable indicate pin callback");
	(void)modem_slm_register_ind(ind_handler_backup, true);
	(void)indicate_pin_enable();
	return 0;
}

int modem_slm_shell_slmsh_indicate_disable(const struct shell *shell, size_t argc, char **argv)
{
	LOG_INF("Disable indicate pin callback");
	/* indicate_pin_disable() is not called so we get one indication where we just log
	 * a warning that indications are not coming and then disable the indication pin.
	 */
	return modem_slm_register_ind(NULL, true);
}

SHELL_CMD_REGISTER(slm, NULL, "Send AT commands to SLM device", modem_slm_shell);

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_indicate,
	SHELL_CMD(enable, NULL, "Enable/disable indicate pin callback",
		  modem_slm_shell_slmsh_indicate_enable),
	SHELL_CMD(disable, NULL, "Disable indicate pin callback",
		  modem_slm_shell_slmsh_indicate_disable),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_slmsh,
	SHELL_CMD(powerpin, NULL, "Toggle power pin configured with CONFIG_MODEM_SLM_POWER_PIN",
		  modem_slm_shell_slmsh_powerpin),
	SHELL_CMD(indicate, &sub_indicate, "Enable/disable indicate pin callback",
		  NULL),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(slmsh, &sub_slmsh, "Commands handled in SLM shell device", NULL);

#endif /* CONFIG_MODEM_SLM_SHELL */
