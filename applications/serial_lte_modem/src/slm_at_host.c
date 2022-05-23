/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <ctype.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/uart.h>
#include <hal/nrf_uarte.h>
#include <hal/nrf_gpio.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/sys/util.h>
#include <zephyr/pm/device.h>
#include <string.h>
#include <zephyr/init.h>
#include "slm_util.h"
#include "slm_at_host.h"
#include "slm_at_fota.h"
#if defined(CONFIG_SLM_NRF52_DFU_LEGACY)
#include "slip.h"
#endif

LOG_MODULE_REGISTER(slm_at_host, CONFIG_SLM_LOG_LEVEL);

#define OK_STR		"\r\nOK\r\n"
#define ERROR_STR	"\r\nERROR\r\n"
#define FATAL_STR	"FATAL ERROR\r\n"
#define SLM_SYNC_STR	"Ready\r\n"

/** The maximum allowed length of an AT command passed through the SLM.
 *  The space is allocated statically. */
#define AT_MAX_CMD_LEN          4096

#define UART_RX_BUF_NUM         2
#define UART_RX_LEN             256
#define UART_RX_TIMEOUT_US      2000
#define UART_ERROR_DELAY_MS     500
#define UART_RX_MARGIN_MS       10
#define UART_TX_DATA_SIZE	1024

#define HEXDUMP_DATAMODE_MAX    16

static enum slm_operation_modes {
	SLM_AT_COMMAND_MODE,  /* AT command host or bridge */
	SLM_DATA_MODE,        /* Raw data sending */
	SLM_DFU_MODE          /* nRF52 DFU controller */
} slm_operation_mode;

static const struct device *uart_dev;
static uint8_t at_buf[AT_MAX_CMD_LEN];
static uint16_t at_buf_len;
static bool at_buf_overflow;
static struct ring_buf data_rb;
static bool datamode_rx_disabled;
static slm_datamode_handler_t datamode_handler;
static struct k_work raw_send_work;
static struct k_work cmd_send_work;
static struct k_work datamode_quit_work;

RING_BUF_DECLARE(delayed_rb, UART_TX_DATA_SIZE);
static struct k_work delayed_send_work;

static uint8_t uart_rx_buf[UART_RX_BUF_NUM][UART_RX_LEN];
static uint8_t *next_buf;
static uint8_t *uart_tx_buf;
static bool uart_recovery_pending;
static struct k_work_delayable uart_recovery_work;

static K_SEM_DEFINE(tx_done, 0, 1);

/* global functions defined in different files */
int slm_at_parse(const char *at_cmd);
int slm_at_init(void);
void slm_at_uninit(void);
int slm_setting_uart_save(void);
int indicate_start(void);

/* global variable used across different files */
struct at_param_list at_param_list;           /* For AT parser */
char rsp_buf[SLM_AT_CMD_RESPONSE_MAX_LEN];    /* SLM URC and socket data */
uint16_t datamode_time_limit;                 /* Send trigger by time in data mode */

/* global variable defined in different files */
extern bool uart_configured;
extern struct uart_config slm_uart;

static int uart_send(const uint8_t *buffer, size_t len)
{
	int ret;
	enum pm_device_state state = PM_DEVICE_STATE_OFF;

	pm_device_state_get(uart_dev, &state);
	if (state != PM_DEVICE_STATE_ACTIVE) {
		(void)indicate_start();
		return -EAGAIN;
	}

	k_sem_take(&tx_done, K_FOREVER);

	uart_tx_buf = k_malloc(len);
	if (uart_tx_buf == NULL) {
		LOG_WRN("No ram buffer");
		k_sem_give(&tx_done);
		return -ENOMEM;
	}

	memcpy(uart_tx_buf, buffer, len);
	ret = uart_tx(uart_dev, uart_tx_buf, len, SYS_FOREVER_US);
	if (ret) {
		LOG_WRN("uart_tx failed: %d", ret);
		k_free(uart_tx_buf);
		k_sem_give(&tx_done);
	}

	return ret;
}

void rsp_send(const char *str, size_t len)
{
	if (len == 0 || slm_operation_mode == SLM_DFU_MODE) {
		return;
	}

	LOG_HEXDUMP_DBG(str, len, "TX");
	if (uart_send(str, len) < 0) {
		ring_buf_put(&delayed_rb, str, len);
	}
}

void data_send(const uint8_t *data, size_t len)
{
	if (slm_operation_mode == SLM_DFU_MODE) {
		return;
	}
	LOG_HEXDUMP_DBG(data, MIN(len, HEXDUMP_DATAMODE_MAX), "TX-DATA");
	if (uart_send(data, len) < 0) {
		ring_buf_put(&delayed_rb, data, len);
	}
}

static int uart_receive(void)
{
	int ret;

	ret = uart_rx_enable(uart_dev, uart_rx_buf[0], sizeof(uart_rx_buf[0]), UART_RX_TIMEOUT_US);
	if (ret && ret != -EBUSY) {
		LOG_ERR("UART RX failed: %d", ret);
		rsp_send(FATAL_STR, sizeof(FATAL_STR) - 1);
		return ret;
	}
	next_buf = uart_rx_buf[1];
	at_buf_overflow = false;
	at_buf_len = 0;

	return 0;
}

static void uart_recovery(struct k_work *work)
{
	ARG_UNUSED(work);

	(void)uart_receive();
	uart_recovery_pending = false;
	LOG_DBG("UART recovered");
}

int enter_datamode(slm_datamode_handler_t handler)
{
	if (handler == NULL || datamode_handler != NULL) {
		LOG_INF("Invalid, not enter datamode");
		return -EINVAL;
	}

	ring_buf_init(&data_rb, sizeof(at_buf), at_buf);
	datamode_handler = handler;
	slm_operation_mode = SLM_DATA_MODE;
	if (datamode_time_limit == 0) {
		if (slm_uart.baudrate > 0) {
			datamode_time_limit = UART_RX_LEN * (8 + 1 + 1) * 1000 / slm_uart.baudrate;
			datamode_time_limit += UART_RX_MARGIN_MS;
		} else {
			LOG_WRN("Baudrate not set");
			datamode_time_limit = 1000;
		}
	}
	LOG_INF("Enter datamode");

	return 0;
}

bool in_datamode(void)
{
	return (slm_operation_mode == SLM_DATA_MODE);
}

bool exit_datamode(int exit_mode)
{
	if (slm_operation_mode == SLM_DATA_MODE) {
		ring_buf_reset(&data_rb);
		/* reset UART to restore command mode */
		uart_rx_disable(uart_dev);
		k_sleep(K_MSEC(10));
		(void)uart_receive();

		if (exit_mode == DATAMODE_EXIT_OK) {
			strcpy(rsp_buf, OK_STR);
		} else if (exit_mode == DATAMODE_EXIT_ERROR) {
			strcpy(rsp_buf, ERROR_STR);
		} else {
			sprintf(rsp_buf, "\r\n#XDATAMODE: 0\r\n");
		}
		rsp_send(rsp_buf, strlen(rsp_buf));

		slm_operation_mode = SLM_AT_COMMAND_MODE;
		datamode_handler = NULL;
		LOG_INF("Exit datamode");
		return true;
	}

	return false;
}

int poweroff_uart(void)
{
	int err;

	uart_rx_disable(uart_dev);
	k_sleep(K_MSEC(100));
	err = pm_device_action_run(uart_dev, PM_DEVICE_ACTION_SUSPEND);
	if (err && err != -EALREADY) {
		LOG_ERR("Can't suspend uart: %d", err);
	}

	return err;
}

static void delayed_send(struct k_work *work)
{
	uint8_t *data = NULL;
	int size_send;

	/* NOTE ring_buf_get_claim() might not return full size */
	do {
		size_send = ring_buf_get_claim(&delayed_rb, &data, UART_TX_DATA_SIZE);
		if (data != NULL && size_send > 0) {
			(void)uart_send(data, size_send);
			(void)ring_buf_get_finish(&delayed_rb, size_send);
		} else {
			break;
		}
	} while (true);
}

int poweron_uart(void)
{
	int err;

	err = pm_device_action_run(uart_dev, PM_DEVICE_ACTION_RESUME);
	if (err && err != -EALREADY) {
		LOG_ERR("Can't resume uart: %d", err);
		return err;
	}

	k_sleep(K_MSEC(100));

	err = uart_receive();
	if (err) {
		return err;
	}

	k_sem_give(&tx_done);
	uart_send(SLM_SYNC_STR, sizeof(SLM_SYNC_STR)-1);
	k_work_submit(&delayed_send_work);

	return 0;
}

int slm_uart_configure(void)
{
	int err;

	LOG_DBG("Set uart baudrate to: %d, hw flow control %d", slm_uart.baudrate,
		slm_uart.flow_ctrl);

	err = uart_configure(uart_dev, &slm_uart);
	if (err != 0) {
		LOG_ERR("uart_configure: %d", err);
		return err;
	}
#if defined(CONFIG_SLM_UART_HWFC_RUNTIME)
/* Set HWFC dynamically */
	#if defined(CONFIG_SLM_CONNECT_UART_0)
		#define RTS_PIN DT_PROP(DT_NODELABEL(uart0), rts_pin)
		#define CTS_PIN DT_PROP(DT_NODELABEL(uart0), cts_pin)
		if (slm_uart.flow_ctrl == UART_CFG_FLOW_CTRL_RTS_CTS) {
			nrf_uarte_hwfc_pins_set(NRF_UARTE0,
						RTS_PIN,
						CTS_PIN);
		} else {
			nrf_gpio_pin_clear(RTS_PIN);
			nrf_gpio_cfg_output(RTS_PIN);
		}
	#endif
	#if defined(CONFIG_SLM_CONNECT_UART_2)
		#define RTS_PIN DT_PROP(DT_NODELABEL(uart2), rts_pin)
		#define CTS_PIN DT_PROP(DT_NODELABEL(uart2), cts_pin)
		if (slm_uart.flow_ctrl == UART_CFG_FLOW_CTRL_RTS_CTS) {
			nrf_uarte_hwfc_pins_set(NRF_UARTE2,
						RTS_PIN,
						CTS_PIN);
		} else {
			nrf_gpio_pin_clear(RTS_PIN);
			nrf_gpio_cfg_output(RTS_PIN);
		}
	#endif
#endif
	return err;
}

bool verify_datamode_control(uint16_t time_limit, uint16_t *min_time_limit)
{
	int min_time;

	if (slm_uart.baudrate == 0) {
		LOG_ERR("Baudrate not set");
		return false;
	}

	min_time = UART_RX_LEN * (8 + 1 + 1) * 1000 / slm_uart.baudrate;
	min_time += UART_RX_MARGIN_MS;

	if (time_limit > 0 && min_time > time_limit) {
		LOG_ERR("Invalid time_limit: %d, min: %d", time_limit, min_time);
		return false;
	}

	if (min_time_limit) {
		*min_time_limit = min_time;
	}

	return true;
}

AT_MONITOR(at_notify, ANY, notification_handler);

static void notification_handler(const char *response)
{
	if (slm_operation_mode == SLM_AT_COMMAND_MODE) {
		/* Forward the data over UART */
		rsp_send("\r\n", 2);
		rsp_send(response, strlen(response));
	}
}

#if defined(CONFIG_SLM_NRF52_DFU)
/* Overall, nRF52 DFU is supported or not */
#if defined(CONFIG_SLM_NRF52_DFU_LEGACY)
/* Functions for legacy Serial DFU Protocol, UART is in TX and RX */
#define DFU_RX_MAX_DELAY 1000 /* in milliseconds */
static K_SEM_DEFINE(dfu_response, 0, 1);

void enter_dfumode(void)
{
	if (slm_operation_mode != SLM_DFU_MODE) {
		slm_operation_mode = SLM_DFU_MODE;
		LOG_INF("Enter dfumode");
	}
}

void exit_dfumode(void)
{
	if (slm_operation_mode == SLM_DFU_MODE) {
		/* reset UART to restore command mode */
		uart_rx_disable(uart_dev);
		k_sleep(K_MSEC(10));
		slm_operation_mode = SLM_AT_COMMAND_MODE;
		(void) uart_receive();
		LOG_INF("Exit dfumode");
	}
}

int dfu_drv_tx(const uint8_t *data, uint16_t length)
{
	int ret = 0;

	if (length == 0 || slm_operation_mode != SLM_DFU_MODE) {
		return -EINVAL;
	}

	if (length > SLIP_SIZE_MAX) {
		ret = -ENOMEM;
	} else {
		uint32_t slip_pkt_len;

		encode_slip(rsp_buf, &slip_pkt_len, data, length);
		LOG_HEXDUMP_DBG(rsp_buf, slip_pkt_len, "DFU-TX");
		ret = uart_send(rsp_buf, slip_pkt_len);
	}

	return ret;
}

int dfu_drv_rx(uint8_t *data, uint32_t max_len, uint32_t *real_len)
{
	int ret;

	if (data == NULL) {
		return -EINVAL;
	}

	at_buf_len = 0;
	ret = k_sem_take(&dfu_response, K_MSEC(DFU_RX_MAX_DELAY));
	if (ret) {
		LOG_ERR("DFU response error: %d", ret);
		return -EAGAIN;
	}

	ret = decode_slip(data, real_len, at_buf, at_buf_len);
	if (ret) {
		return ret;
	}
	if (*real_len > max_len) {
		return -ENOMEM;
	}

	LOG_HEXDUMP_DBG(data, *real_len, "DFU-RX");

	return 0;
}

static int dfu_rx_handler(const uint8_t *data, int datalen)
{
	memcpy(at_buf + at_buf_len, data, datalen);
	at_buf_len += datalen;

	if (at_buf[at_buf_len - 1] == SLIP_END) {
		k_sem_give(&dfu_response);
	}

	return 0;
}
#else
/* Functions for MCUBOOT-based DFU protocol, UART is in TX only */
void enter_dfumode(void)
{
	if (slm_operation_mode != SLM_DFU_MODE) {
		slm_operation_mode = SLM_DFU_MODE;
		/* only UART Send */
		uart_rx_disable(uart_dev);
		k_sleep(K_MSEC(10));
		LOG_INF("Enter dfumode");
	}
}

void exit_dfumode(void)
{
	if (slm_operation_mode == SLM_DFU_MODE) {
		slm_operation_mode = SLM_AT_COMMAND_MODE;
		(void) uart_receive();
		LOG_INF("Exit dfumode");
	}
}

int dfu_drv_tx(const uint8_t *data, uint16_t length)
{
	if (length == 0 || slm_operation_mode != SLM_DFU_MODE) {
		return -EINVAL;
	}

	return uart_send(data, length);
}
#endif /* CONFIG_SLM_NRF52_DFU_LEGACY */
#endif /* CONFIG_SLM_NRF52_DFU */

static void raw_send(struct k_work *work)
{
	uint8_t *data = NULL;
	int size_send, size_sent;
	const char *quit_str = CONFIG_SLM_DATAMODE_TERMINATOR;
	int quit_str_len = strlen(quit_str);
	bool datamode_off_pending = false;

	ARG_UNUSED(work);

	/* NOTE ring_buf_get_claim() might not return full size */
	do {
		size_send = ring_buf_get_claim(&data_rb, &data, sizeof(at_buf));
		if (data != NULL && size_send > 0) {
			/* Exit datamode if apply */
			if (size_send >= quit_str_len) {
				char *quit = data + (size_send - quit_str_len);

				if (strncmp(quit, quit_str, quit_str_len) == 0) {
					memset(quit, 0x00, quit_str_len);
					size_send -= quit_str_len;
					datamode_off_pending = true;
				}
				if (size_send == 0) {
					k_work_submit(&datamode_quit_work);
					LOG_INF("datamode off pending");
					return;
				}
			}
			/* Raw data sending */
			int size_finish = datamode_off_pending ? quit_str_len : 0;

			LOG_INF("Raw send %d", size_send);
			LOG_HEXDUMP_DBG(data, MIN(size_send, HEXDUMP_DATAMODE_MAX), "RX-DATAMODE");
			if (datamode_handler && size_send > 0) {
				size_sent = datamode_handler(DATAMODE_SEND, data, size_send);
				if (size_sent > 0) {
					size_finish += size_sent;
				} else if (size_sent == 0) {
					size_finish += size_send;
				} else {
					LOG_WRN("Raw send failed, %d dropped", size_send);
					size_finish += size_send;
					datamode_off_pending = true;
				}
				(void)ring_buf_get_finish(&data_rb, size_finish);
			} else {
				LOG_WRN("no handler, %d dropped", size_send);
				(void)ring_buf_get_finish(&data_rb, size_send + size_finish);
			}

			if (datamode_off_pending) {
				k_work_submit(&datamode_quit_work);
				LOG_INF("datamode off pending");
				return;
			}
		} else {
			break;
		}
	} while (true);

	/* resume UART RX in case of stopped by buffer full */
	if (datamode_rx_disabled) {
		(void)uart_receive();
		datamode_rx_disabled = false;
	}
}

static void inactivity_timer_handler(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	LOG_INF("time limit reached");
	if (!ring_buf_is_empty(&data_rb)) {
		k_work_submit(&raw_send_work);
	} else {
		LOG_WRN("data buffer empty");
	}
}

K_TIMER_DEFINE(inactivity_timer, inactivity_timer_handler, NULL);

static void datamode_quit(struct k_work *work)
{
	ARG_UNUSED(work);

	/* quit datamode */
	if (datamode_handler) {
		(void)datamode_handler(DATAMODE_EXIT, NULL, 0);
	} else {
		LOG_WRN("missing datamode handler");
	}
	(void)exit_datamode(DATAMODE_EXIT_OK);
}

static int raw_rx_handler(const uint8_t *data, int datalen)
{
	int ret;

	k_timer_stop(&inactivity_timer);

	/* save data to buffer */
	ret = ring_buf_put(&data_rb, data, datalen);
	if (ret != datalen) {
		LOG_ERR("enqueue data error (%d, %d)", datalen, ret);
		uart_rx_disable(uart_dev);
		return -1;
	}
	ret = ring_buf_space_get(&data_rb);
	if (ret < UART_RX_LEN) {
		LOG_WRN("data buffer full (%d)", ret);
		uart_rx_disable(uart_dev);
		return -1;
	}

	/* start/restart inactivity timer */
	k_timer_start(&inactivity_timer, K_MSEC(datamode_time_limit), K_NO_WAIT);

	return 0;
}

/*
 * Check AT command grammar based on below.
 *  AT<NULL>
 *  AT<separator><body><NULL>
 *  AT<separator><body>=<NULL>
 *  AT<separator><body>?<NULL>
 *  AT<separator><body>=?<NULL>
 *  AT<separator><body>=<parameters><NULL>
 * In which
 * <separator>: +, %, #
 * <body>: alphanumeric char only, size > 0
 * <parameters>: arbitrary, size > 0
 */
static int cmd_grammar_check(const uint8_t *cmd, uint16_t length)
{
	const uint8_t *body;

	/* check AT (if not, no check) */
	if (length < 2 || toupper((int)cmd[0]) != 'A' || toupper((int)cmd[1]) != 'T') {
		return -EINVAL;
	}

	/* check AT<NULL> */
	cmd += 2;
	if (*cmd == '\0') {
		return 0;
	}

	/* check AT<separator> */
	if ((*cmd != '+') && (*cmd != '%') && (*cmd != '#')) {
		return -EINVAL;
	}

	/* check AT<separator><body> */
	cmd += 1;
	body = cmd;
	while (true) {
		/* check body is alphanumeric */
		if (!isalpha((int)*cmd) && !isdigit((int)*cmd)) {
			break;
		}
		cmd++;
	}

	/* check body size > 0 */
	if (cmd == body) {
		return -EINVAL;
	}

	/* check AT<separator><body><NULL> */
	if (*cmd == '\0') {
		return 0;
	}

	/* check AT<separator><body>= or check AT<separator><body>? */
	if (*cmd != '=' && *cmd != '?') {
		return -EINVAL;
	}

	/* check AT<separator><body>?<NULL> */
	if (*cmd == '?') {
		cmd += 1;
		if (*cmd == '\0') {
			return 0;
		} else {
			return -EINVAL;
		}
	}

	/* check AT<separator><body>=<NULL> */
	cmd += 1;
	if (*cmd == '\0') {
		return 0;
	}

	/* check AT<separator><body>=?<NULL> */
	if (*cmd == '?') {
		cmd += 1;
		if (*cmd == '\0') {
			return 0;
		} else {
			return -EINVAL;
		}
	}

	/* no need to check AT<separator><body>=<parameters><NULL> */
	return 0;
}

static void format_final_result(char *buf)
{
	static const char ok_str[] = "OK\r\n";
	static const char error_str[] = "ERROR\r\n";
	static const char cme_error_str[] = "+CME ERROR:";
	static const char cms_error_str[] = "+CMS ERROR:";
	static const char crlf_str[] = "\r\n";
	char *result = NULL, *temp;

	/* find the last occurrence of final result string */
	result = strstr(buf, ok_str);
	if (result) {
		temp = result;
		while (temp != NULL) {
			temp = strstr(result + strlen(ok_str), ok_str);
			if (temp) {
				result = temp;
			}
		}
		goto final_result;
	}
	result = strstr(buf, error_str);
	if (result) {
		temp = result;
		while (temp != NULL) {
			temp = strstr(result + strlen(error_str), error_str);
			if (temp) {
				result = temp;
			}
		}
		goto final_result;
	}
	result = strstr(buf, cme_error_str);
	if (result) {
		temp = result;
		while (temp != NULL) {
			temp = strstr(result + strlen(cme_error_str), cme_error_str);
			if (temp) {
				result = temp;
			}
		}
		goto final_result;
	}
	result = strstr(buf, cms_error_str);
	if (result) {
		temp = result;
		while (temp != NULL) {
			temp = strstr(result + strlen(cms_error_str), cms_error_str);
			if (temp) {
				result = temp;
			}
		}
	}
	if (result == NULL) {
		LOG_WRN("Final result not found");
		return;
	}

final_result:
	/* insert CRLF before final result if there is information response before it */
	if (result != buf) {
		memmove((void *)(result + strlen(crlf_str)), (void *)result, strlen(result) + 1);
		memcpy((void *)result, (void *)crlf_str, strlen(crlf_str));
	}
}

static void cmd_send(struct k_work *work)
{
	int err;

	ARG_UNUSED(work);

	if (at_buf_overflow) {
		rsp_send(ERROR_STR, sizeof(ERROR_STR) - 1);
		goto done;
	}

	LOG_HEXDUMP_DBG(at_buf, at_buf_len, "RX");

	if (cmd_grammar_check(at_buf, at_buf_len) != 0) {
		LOG_ERR("AT command invalid");
		rsp_send(ERROR_STR, sizeof(ERROR_STR) - 1);
		goto done;
	}

	err = slm_at_parse(at_buf);
	if (err == 0) {
		if (!in_datamode()) {
			rsp_send(OK_STR, sizeof(OK_STR) - 1);
		}
		goto done;
	} else if (err != -ENOENT) {
		rsp_send(ERROR_STR, sizeof(ERROR_STR) - 1);
		goto done;
	}

	/* Send to modem */
	err = nrf_modem_at_cmd(at_buf, sizeof(at_buf), "%s", at_buf);
	if (err < 0) {
		LOG_ERR("AT command failed: %d", err);
		rsp_send(ERROR_STR, sizeof(ERROR_STR) - 1);
		goto done;
	} else if (err > 0) {
		LOG_ERR("AT command error, type: %d", nrf_modem_at_err_type(err));
	}

	/** Format as TS 27.007 command V1 with verbose response format,
	 *  based on current return of API nrf_modem_at_cmd() and MFWv1.3.x
	 */
	if (strlen(at_buf) > 0) {
		rsp_send("\r\n", 2);		/* insert <CR><LF> before information response*/
		format_final_result(at_buf);	/* insert <CR><LF> before final result */
		rsp_send(at_buf, strlen(at_buf));
	}

done:
	(void)uart_receive();
}

static int cmd_rx_handler(uint8_t character)
{
	static bool inside_quotes;
	static size_t at_cmd_len;

	/* Handle control characters */
	switch (character) {
	case 0x08: /* Backspace. */
		/* Fall through. */
	case 0x7F: /* DEL character */
		if (at_cmd_len > 0) {
			at_cmd_len--;
		}
		return 0;
	}

	/* Handle termination characters, if outside quotes. */
	if (!inside_quotes) {
		switch (character) {
		case '\r':
#if defined(CONFIG_SLM_CR_TERMINATION)
			goto send;
#else
			break;
#endif
		case '\n':
#if defined(CONFIG_SLM_LF_TERMINATION)
			goto send;
#elif defined(CONFIG_SLM_CR_LF_TERMINATION)
			if (at_cmd_len > 0 && at_buf[at_cmd_len - 1] == '\r') {
				at_cmd_len--; /* trim the CR char */
				goto send;
			}
#endif
			break;
		}
	}

	/* Write character to AT buffer */
	at_buf[at_cmd_len] = character;
	at_cmd_len++;

	/* Detect AT command buffer overflow, leaving space for null */
	if (at_cmd_len > sizeof(at_buf) - 1) {
		LOG_ERR("Buffer overflow");
		at_cmd_len--;
		at_buf_overflow = true;
		goto send;
	}

	/* Handle special written character */
	if (character == '"') {
		inside_quotes = !inside_quotes;
	}

	return 0;

send:
	uart_rx_disable(uart_dev);

	at_buf[at_cmd_len] = '\0';
	at_buf_len = at_cmd_len;
	k_work_submit(&cmd_send_work);

	inside_quotes = false;
	at_cmd_len = 0;
	if (at_buf_overflow) {
		return -1;
	}
	return 0;
}

static void uart_callback(const struct device *dev, struct uart_event *evt, void *user_data)
{
	int err;
	static uint16_t pos;
	static bool enable_rx_retry;

	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);

	switch (evt->type) {
	case UART_TX_DONE:
		k_free(uart_tx_buf);
		k_sem_give(&tx_done);
		break;
	case UART_TX_ABORTED:
		k_free(uart_tx_buf);
		k_sem_give(&tx_done);
		LOG_INF("TX_ABORTED");
		break;
	case UART_RX_RDY:
		if (slm_operation_mode == SLM_AT_COMMAND_MODE) {
			for (int i = pos; i < (pos + evt->data.rx.len); i++) {
				err = cmd_rx_handler(evt->data.rx.buf[i]);
				if (err) {
					return;
				}
			}
		} else if (slm_operation_mode == SLM_DATA_MODE) {
			LOG_DBG("RX_RDY %d", evt->data.rx.len);
			err = raw_rx_handler(&(evt->data.rx.buf[pos]), evt->data.rx.len);
			if (err) {
				return;
			}
#if defined(CONFIG_SLM_NRF52_DFU_LEGACY)
		} else if (slm_operation_mode == SLM_DFU_MODE) {
			(void)dfu_rx_handler(&(evt->data.rx.buf[pos]), evt->data.rx.len);
#endif /* CONFIG_SLM_NRF52_DFU_LEGACY */
		} else {
			LOG_WRN("No handler");
		}
		pos += evt->data.rx.len;
		break;
	case UART_RX_BUF_REQUEST:
		pos = 0;
		err = uart_rx_buf_rsp(uart_dev, next_buf, sizeof(uart_rx_buf[0]));
		if (err) {
			LOG_WRN("UART RX buf rsp: %d", err);
		}
		break;
	case UART_RX_BUF_RELEASED:
		next_buf = evt->data.rx_buf.buf;
		break;
	case UART_RX_STOPPED:
		LOG_WRN("RX_STOPPED (%d)", evt->data.rx_stop.reason);
		/* Retry automatically in case of UART ERROR interrupt */
		if (evt->data.rx_stop.reason != 0) {
			enable_rx_retry = true;
		}
		break;
	case UART_RX_DISABLED:
		LOG_DBG("RX_DISABLED");
		if (slm_operation_mode == SLM_DATA_MODE) {
			datamode_rx_disabled = true;
			/* flush data in ring-buffer, if any */
			k_work_submit(&raw_send_work);
		}
		if (enable_rx_retry && !uart_recovery_pending) {
			k_work_schedule(&uart_recovery_work, K_MSEC(UART_ERROR_DELAY_MS));
			enable_rx_retry = false;
			uart_recovery_pending = true;
		}
		break;
	default:
		break;
	}
}

int slm_at_host_init(void)
{
	int err;
	uint32_t start_time;

	/* Initialize the UART module */
#if defined(CONFIG_SLM_CONNECT_UART_0)
	uart_dev = device_get_binding(DT_LABEL(DT_NODELABEL(uart0)));
#elif defined(CONFIG_SLM_CONNECT_UART_2)
	uart_dev = device_get_binding(DT_LABEL(DT_NODELABEL(uart2)));
#else
	LOG_ERR("Unsupported UART instance");
	return -EINVAL;
#endif
	if (uart_dev == NULL) {
		LOG_ERR("Cannot bind UART device\n");
		return -EINVAL;
	}
	/* Save UART configuration to setting page */
	if (!uart_configured) {
		uart_configured = true;
		err = uart_config_get(uart_dev, &slm_uart);
		if (err != 0) {
			LOG_ERR("uart_config_get: %d", err);
			return err;
		}
		err = slm_setting_uart_save();
		if (err != 0) {
			LOG_ERR("slm_setting_uart_save: %d", err);
			return err;
		}
	} else {
		/* else re-config UART based on setting page */
		LOG_DBG("UART baud: %d d/p/s-bits: %d/%d/%d HWFC: %d",
			slm_uart.baudrate, slm_uart.data_bits, slm_uart.parity,
			slm_uart.stop_bits, slm_uart.flow_ctrl);
		err = slm_uart_configure();
		if (err != 0) {
			LOG_ERR("Fail to set uart baudrate: %d", err);
			return err;
		}
	}
	/* Wait for the UART line to become valid */
	start_time = k_uptime_get_32();
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
	err = uart_receive();
	if (err) {
		return -EFAULT;
	}

	/* Initialize AT Parser */
	err = at_params_list_init(&at_param_list, CONFIG_SLM_AT_MAX_PARAM);
	if (err) {
		LOG_ERR("Failed to init AT Parser: %d", err);
		return err;
	}

	datamode_time_limit = 0;
	datamode_handler = NULL;

	err = slm_at_init();
	if (err) {
		return -EFAULT;
	}

	k_work_init(&raw_send_work, raw_send);
	k_work_init(&cmd_send_work, cmd_send);
	k_work_init(&datamode_quit_work, datamode_quit);
	k_work_init(&delayed_send_work, delayed_send);
	k_work_init_delayable(&uart_recovery_work, uart_recovery);
	k_sem_give(&tx_done);
	rsp_send(SLM_SYNC_STR, sizeof(SLM_SYNC_STR)-1);
	slm_fota_post_process();

	slm_operation_mode = SLM_AT_COMMAND_MODE;
	LOG_INF("at_host init done");
	return 0;
}

void slm_at_host_uninit(void)
{
	int err;

	if (slm_operation_mode == SLM_DATA_MODE) {
		k_timer_stop(&inactivity_timer);
	}
	datamode_handler = NULL;

	slm_at_uninit();

	/* Power off UART module */
	uart_rx_disable(uart_dev);
	k_sleep(K_MSEC(100));
	err = pm_device_action_run(uart_dev, PM_DEVICE_ACTION_SUSPEND);
	if (err) {
		LOG_WRN("Can't suspend uart: %d", err);
	}

	/* Un-initialize AT Parser */
	at_params_list_free(&at_param_list);

	LOG_DBG("at_host uninit done");
}
