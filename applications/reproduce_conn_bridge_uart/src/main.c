#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main);

#define LOG_INTERVAL_MS 10

static const struct device *log_uart_dev = DEVICE_DT_GET(DT_ALIAS(log_uart));
static const struct device *extra_uart_dev = DEVICE_DT_GET(DT_ALIAS(extra_uart));

static K_SEM_DEFINE(log_uart_tx_sem, 0, 1);
static K_SEM_DEFINE(extra_uart_tx_sem, 0, 1);

void callback(const struct device *dev, struct uart_event *evt, void *user_data)
{
	ARG_UNUSED(user_data);

	if (evt->type == UART_TX_DONE || evt->type == UART_TX_ABORTED) {
		if (dev == log_uart_dev) {
			k_sem_give(&log_uart_tx_sem);
		} else if (dev == extra_uart_dev) {
			k_sem_give(&extra_uart_tx_sem);
		}
	}
}

int main(void)
{
	int err;

	LOG_INF("UART log test");

	BUILD_ASSERT(IS_ENABLED(CONFIG_UART_ASYNC_API));

	if (!device_is_ready(log_uart_dev)) {
		LOG_ERR("UART0 is not ready");
		return 1;
	}

	if (!device_is_ready(extra_uart_dev)) {
		LOG_ERR("UART1 is not ready");
		return 1;
	}

	err = uart_callback_set(log_uart_dev, callback, NULL);
	if (err) {
		LOG_ERR("uart_callback_set err, %d", err);
		return 1;
	}

	err = uart_callback_set(extra_uart_dev, callback, NULL);
	if (err) {
		LOG_ERR("uart_callback_set err, %d", err);
		return 1;
	}

	int app_counter = 0;
	int trace_counter = 0;

	k_sem_give(&log_uart_tx_sem);
	k_sem_give(&extra_uart_tx_sem);

	while (trace_counter < 500) {
		char app_log_msg[32];
		char trace_log_msg[32];

		int app_len = snprintf(app_log_msg, sizeof(app_log_msg), "app log %d\r\n",
			app_counter++);
		k_sem_take(&log_uart_tx_sem, K_FOREVER);
		uart_tx(log_uart_dev, app_log_msg, app_len, SYS_FOREVER_MS);

		int trace_len = snprintf(trace_log_msg, sizeof(trace_log_msg), "trace log %d\r\n",
			trace_counter++);
		k_sem_take(&extra_uart_tx_sem, K_FOREVER);
		uart_tx(extra_uart_dev, trace_log_msg, trace_len, SYS_FOREVER_MS);

		k_msleep(LOG_INTERVAL_MS);
	}

	sys_reboot(SYS_REBOOT_WARM);

	return 0;
}
