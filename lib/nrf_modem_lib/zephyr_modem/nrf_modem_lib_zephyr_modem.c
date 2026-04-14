/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file nrf_modem_lib_zephyr_modem.c
 *
 * Implements nrf_modem_lib + nrf_modem_at APIs on a non-nRF91 host (e.g.
 * nRF54L20) by managing a CMUX session directly to an nRF91 running Serial
 * Modem (ncs-serial-modem) over UART.
 *
 * No changes to the Zephyr or NRF driver trees are required.
 *
 * CMUX layout (after dial):
 *   DLCI 1  →  PPP data bearer  (modem_ppp)
 *   DLCI 2  →  AT command channel (raw pipe I/O + reader thread)
 *
 * Init sequence:
 *   1. Open UART backend pipe.
 *   2. Run init chat script (AT, AT+CFUN=4, ..., AT#XCMUX=1) on UART pipe.
 *   3. Transfer UART pipe to modem_cmux; connect CMUX.
 *   4. Init DLCI 1 (PPP) and DLCI 2 (AT channel after switch).
 *   5. Run dial chat script (AT#XPPP=1, AT#XCMUX=2) on DLCI 1
 *      (which is the default AT channel until AT#XCMUX=2 switches it).
 *      AT+CFUN=1 is intentionally NOT in the dial script: lte_lc must be
 *      able to set AT%XSYSTEMMODE while the modem is still in CFUN=4, and
 *      lte_lc itself drives the CFUN transition later.  The proxy
 *      intercepts that CFUN=1 in at_cmd_send() to also restart SLM PPP.
 *   6. Open DLCI 2 (now the AT channel), start reader thread.
 *   7. Attach modem_ppp to DLCI 1 (now the PPP channel).
 *   8. Call nrf_modem_lib_init() to fire ON_INIT callbacks.
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/modem/backend/uart.h>
#include <zephyr/modem/cmux.h>
#include <zephyr/modem/chat.h>
#include <zephyr/modem/pipe.h>
#include <zephyr/net/ppp.h>
#include <zephyr/modem/ppp.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdarg.h>
#include <ctype.h>

#include <nrf_modem_at.h>
#include <nrf_errno.h>
#include <modem/nrf_modem_lib.h>
#include <modem/at_monitor.h>

/* Register as "nrf_modem" so cfun_hooks.c LOG_MODULE_DECLARE(nrf_modem) resolves. */
LOG_MODULE_REGISTER(nrf_modem, LOG_LEVEL_INF);

/* UART device: parent of the DT alias "modem" node */
#define MODEM_UART_DEV  DEVICE_DT_GET(DT_BUS(DT_ALIAS(modem)))

/* Modem reset GPIO (optional) */
#define MODEM_NODE DT_ALIAS(modem)
#if DT_NODE_HAS_PROP(MODEM_NODE, mdm_reset_gpios)
static const struct gpio_dt_spec mdm_reset_spec =
	GPIO_DT_SPEC_GET(MODEM_NODE, mdm_reset_gpios);
#define HAS_MDM_RESET 1
#else
#define HAS_MDM_RESET 0
#endif

/* AT command buffer sizes */
#define AT_CMD_SIZE   256U
#define AT_BUF_SIZE   CONFIG_NRF_MODEM_LIB_ZEPHYR_MODEM_AT_BUF_SIZE

/* -------------------------------------------------------------------------
 * UART backend
 * ---------------------------------------------------------------------- */

static struct modem_backend_uart uart_backend;
static uint8_t uart_rx_buf[CONFIG_NRF_MODEM_LIB_ZEPHYR_MODEM_UART_BUF_SIZE];
static uint8_t uart_tx_buf[CONFIG_NRF_MODEM_LIB_ZEPHYR_MODEM_UART_BUF_SIZE];
static struct modem_pipe *uart_pipe;

/* -------------------------------------------------------------------------
 * CMUX
 * ---------------------------------------------------------------------- */

static struct modem_cmux cmux;
/* Work buffers sized for MTU + header + extra (see MODEM_CMUX_WORK_BUFFER_SIZE) */
static uint8_t cmux_rx_buf[512];
static uint8_t cmux_tx_buf[512];

/* DLCI 1 = PPP channel (after AT#XCMUX=2), DLCI 2 = AT channel */
static struct modem_cmux_dlci dlci1;
static struct modem_cmux_dlci dlci2;
static uint8_t dlci1_rx_buf[512];
static uint8_t dlci2_rx_buf[1024];
static struct modem_pipe *dlci1_pipe; /* PPP after dial */
static struct modem_pipe *dlci2_pipe; /* AT  after dial */

/* -------------------------------------------------------------------------
 * modem_chat (used only for init + dial scripts)
 * ---------------------------------------------------------------------- */

static struct modem_chat chat;
static uint8_t chat_rx_buf[512];
static uint8_t *chat_argv[32];

/* Chat match tables */
MODEM_CHAT_MATCHES_DEFINE(allow_match,
			  MODEM_CHAT_MATCH("OK", "", NULL),
			  MODEM_CHAT_MATCH("ERROR", "", NULL));

MODEM_CHAT_MATCH_DEFINE(ok_match, "OK", "", NULL);

MODEM_CHAT_MATCHES_DEFINE(abort_matches, MODEM_CHAT_MATCH("ERROR", "", NULL));

MODEM_CHAT_MATCHES_DEFINE(dial_abort_matches,
			  MODEM_CHAT_MATCH("ERROR", "", NULL),
			  MODEM_CHAT_MATCH("NO CARRIER", "", NULL));

/* Init script: runs on raw UART, starts CMUX */
MODEM_CHAT_SCRIPT_CMDS_DEFINE(init_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP_MULT("AT", allow_match),

			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CFUN=4", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CMEE=1", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG=1", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT#XCMUX=1", ok_match));

MODEM_CHAT_SCRIPT_DEFINE(init_script, init_script_cmds,
			 abort_matches, NULL, 10);

/* Dial script: runs on DLCI 1 (SLM default AT channel), switches to DLCI 2.
 *
 * Note: AT+CFUN=1 is intentionally omitted.  lte_lc needs to issue
 * AT%XSYSTEMMODE before the modem goes to CFUN=1 (the modem rejects mode
 * changes with +CME ERROR: 6 once active).  lte_lc itself drives CFUN=1
 * later, and at_cmd_send() pairs that CFUN=1 with AT#XPPP=1 to restart
 * the SLM PPP handler.
 */
MODEM_CHAT_SCRIPT_CMDS_DEFINE(dial_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT#XPPP=1", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT#XCMUX=2", ok_match));

MODEM_CHAT_SCRIPT_DEFINE(dial_script, dial_script_cmds,
			 dial_abort_matches, NULL, 30);

/* -------------------------------------------------------------------------
 * PPP
 * ---------------------------------------------------------------------- */

MODEM_PPP_DEFINE(ppp, NULL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, 1500, 64);

/* -------------------------------------------------------------------------
 * AT reader thread
 * ---------------------------------------------------------------------- */

static K_THREAD_STACK_DEFINE(at_rx_stack, CONFIG_NRF_MODEM_LIB_ZEPHYR_MODEM_STACK_SIZE);
static struct k_thread at_rx_thread;

static K_SEM_DEFINE(pipe_rx_sem, 0, K_SEM_MAX_LIMIT);

static void pipe_event_cb(struct modem_pipe *pipe,
			  enum modem_pipe_event event,
			  void *user_data)
{
	ARG_UNUSED(pipe);
	ARG_UNUSED(user_data);

	if (event == MODEM_PIPE_EVENT_RECEIVE_READY) {
		k_sem_give(&pipe_rx_sem);
	}
}

/* -------------------------------------------------------------------------
 * AT command engine state
 * ---------------------------------------------------------------------- */

static K_MUTEX_DEFINE(at_cmd_mutex);
static K_SEM_DEFINE(at_resp_sem, 0, 1);

static char  resp_buf[AT_BUF_SIZE]; /* accumulated response body */
static size_t resp_len;
static int   cmd_result;            /* 0=OK, >0=encoded error */
static bool  cmd_pending;
static char  current_cmd[AT_CMD_SIZE]; /* in-flight AT command (for URC disambiguation) */

static char  line_buf[AT_BUF_SIZE];
static size_t line_len;

/* -------------------------------------------------------------------------
 * Registered handlers + ready state
 * ---------------------------------------------------------------------- */

static bool initialized;
static nrf_modem_at_notif_handler_t notif_handler;
static nrf_modem_at_cfun_handler_t  cfun_handler;

/* -------------------------------------------------------------------------
 * AT response parsing helpers
 * ---------------------------------------------------------------------- */

static int encode_at_error(int type, int value)
{
	return (type << 16) | (value & 0xffff);
}

static bool is_terminal_ok(const char *line)
{
	return strcmp(line, "OK") == 0;
}

static bool is_terminal_error(const char *line)
{
	return strcmp(line, "ERROR") == 0;
}

static int parse_extended_error(const char *line)
{
	int err;

	if (sscanf(line, "+CME ERROR: %d", &err) == 1) {
		return encode_at_error(NRF_MODEM_AT_CME_ERROR, err);
	}
	if (sscanf(line, "+CMS ERROR: %d", &err) == 1) {
		return encode_at_error(NRF_MODEM_AT_CMS_ERROR, err);
	}
	return -1;
}

/* -------------------------------------------------------------------------
 * URC dispatch
 * ---------------------------------------------------------------------- */

static void dispatch_urc(const char *urc)
{
	int cfun_mode;

	LOG_DBG("URC: %s", urc);

	/* Intercept +CFUN so cfun_hooks and lte_lc see modem mode changes */
	if (cfun_handler != NULL && sscanf(urc, "+CFUN: %d", &cfun_mode) == 1) {
		cfun_handler(cfun_mode);
	}

	/*
	 * at_monitor registers itself here via nrf_modem_at_notif_handler_set,
	 * so at_monitor_dispatch() is called indirectly.
	 */
	if (notif_handler != NULL) {
		notif_handler(urc);
	}
}

/* -------------------------------------------------------------------------
 * PPP readiness flow control
 *
 * On a native nRF91 PDN activation (+CGEV: ME PDN ACT 0) means the modem's
 * IP stack is ready.  With CMUX/PPP the modem signals PDN activation before
 * PPP/IPCP has completed on the host, so the host has no IP yet.
 *
 * The proxy owns the PPP instance and can check its interface directly.
 * +CGEV PDN ACT URCs are held until IPCP has assigned an IPv4 address,
 * then released via a work item.  On reconnects where SLM consumes the
 * +CGEV on DLCI 1, IPCP completion itself proves PDN is active, so the
 * proxy forwards the URC on the host's behalf.
 * ---------------------------------------------------------------------- */

static char held_pdn_act_urc[64];
static bool ppp_ipv4_ready;

static bool ppp_has_ipv4(void)
{
	struct net_if *iface = modem_ppp_get_iface(&ppp);

	if (iface == NULL || iface->config.ip.ipv4 == NULL) {
		return false;
	}

	for (int i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
		struct net_if_addr *addr = &iface->config.ip.ipv4->unicast[i].ipv4;

		if (addr->is_used &&
		    !net_ipv4_is_addr_unspecified(&addr->address.in_addr)) {
			return true;
		}
	}

	return false;
}

static void ppp_ipv4_poll_work_handler(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(ppp_ipv4_poll_work, ppp_ipv4_poll_work_handler);

static void ppp_ipv4_poll_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	if (ppp_has_ipv4()) {
		ppp_ipv4_ready = true;

		if (held_pdn_act_urc[0] != '\0') {
			LOG_INF("PPP IPv4 ready, forwarding held +CGEV URC");
			dispatch_urc(held_pdn_act_urc);
			held_pdn_act_urc[0] = '\0';
		} else {
			LOG_INF("PPP IPv4 ready, forwarding +CGEV for default PDN");
			dispatch_urc("+CGEV: ME PDN ACT 0");
		}
		return;
	}

	k_work_reschedule(&ppp_ipv4_poll_work, K_MSEC(100));
}

static bool hold_pdn_act_if_ppp_not_ready(const char *line)
{
	if (strncmp(line, "+CGEV:", 6) != 0) {
		return false;
	}

	if (strstr(line, "ACT") == NULL || strstr(line, "DEACT") != NULL) {
		return false;
	}

	if (ppp_ipv4_ready) {
		return false;
	}

	LOG_INF("Holding +CGEV PDN ACT until PPP IPv4 ready");
	strncpy(held_pdn_act_urc, line, sizeof(held_pdn_act_urc) - 1);
	held_pdn_act_urc[sizeof(held_pdn_act_urc) - 1] = '\0';

	k_work_reschedule(&ppp_ipv4_poll_work, K_MSEC(100));
	return true;
}

/* -------------------------------------------------------------------------
 * Line processor (called for every \r\n-terminated line)
 * ---------------------------------------------------------------------- */

/* URCs that are never AT command responses and must always be dispatched.
 *
 * Lines that match one of these prefixes are routed to the URC handler
 * even while a command is pending, so they never end up appended to the
 * response buffer of an unrelated command.  Without this, callers like
 * modem_info that parse the raw response can fail with -ENOMSG when an
 * unrelated URC slips into e.g. an AT+CESQ response.
 *
 * The list intentionally only includes prefixes that are *always* sent
 * unsolicited (i.e. the corresponding AT command response uses a
 * different prefix or none at all, so there is no ambiguity with
 * cmd_pending).
 */
static bool is_unsolicited(const char *line)
{
	static const char * const prefixes[] = {
		/* 3GPP / nRF91 URCs */
		"+CGEV:",
		"+CNEC_ESM:",
		"+CEDRXP:",
		"%MDMEV:",
		"%XTIME:",
		"%NCELLMEAS:",
		/* SLM URCs */
		"#XPPP:",
	};

	for (size_t i = 0; i < ARRAY_SIZE(prefixes); i++) {
		if (strncmp(line, prefixes[i], strlen(prefixes[i])) == 0) {
			return true;
		}
	}

	return false;
}

/* Some URCs share their prefix with a regular AT command response (e.g.
 * +CEREG: is both the response to AT+CEREG? and an unsolicited
 * notification when registration changes).  Those must be dispatched as
 * URCs only when they don't belong to the in-flight command, otherwise
 * they would slip into the response buffer of an unrelated command and
 * break callers like modem_info that parse the raw response.
 *
 * For each ambiguous prefix we record the AT command stem that owns it
 * as a response.  If the in-flight command starts with that stem the
 * line is treated as a response (accumulated); otherwise it's a URC.
 */
static bool is_orphan_urc(const char *line)
{
	static const struct {
		const char *line_prefix;
		const char *cmd_stem;
	} ambiguous[] = {
		{ "+CEREG:",  "AT+CEREG"  },
		{ "+CSCON:",  "AT+CSCON"  },
		{ "+CGREG:",  "AT+CGREG"  },
	};

	for (size_t i = 0; i < ARRAY_SIZE(ambiguous); i++) {
		if (strncmp(line, ambiguous[i].line_prefix,
			    strlen(ambiguous[i].line_prefix)) != 0) {
			continue;
		}

		/* Line starts with this prefix: URC unless the in-flight
		 * command would generate this as its response.
		 */
		return strncasecmp(current_cmd, ambiguous[i].cmd_stem,
				   strlen(ambiguous[i].cmd_stem)) != 0;
	}

	return false;
}

static void process_line(const char *line)
{
	int ext_err;

	if (strlen(line) == 0) {
		return;
	}

	if (!cmd_pending) {
		if (!hold_pdn_act_if_ppp_not_ready(line)) {
			dispatch_urc(line);
		}
		return;
	}

	if (is_unsolicited(line) || is_orphan_urc(line)) {
		if (!hold_pdn_act_if_ppp_not_ready(line)) {
			dispatch_urc(line);
		}
		return;
	}

	if (is_terminal_ok(line)) {
		/* Append OK\r\n to match nrf_modem_at_cmd() output format.
		 * Callers like modem_info parse the raw buffer for "OK\r\n".
		 */
		if (resp_len + 4 < AT_BUF_SIZE) {
			memcpy(&resp_buf[resp_len], "OK\r\n", 4);
			resp_len += 4;
		}
		resp_buf[resp_len] = '\0';
		cmd_result  = 0;
		cmd_pending = false;
		k_sem_give(&at_resp_sem);
		return;
	}

	if (is_terminal_error(line)) {
		resp_buf[resp_len] = '\0';
		cmd_result  = encode_at_error(NRF_MODEM_AT_ERROR, 0);
		cmd_pending = false;
		k_sem_give(&at_resp_sem);
		return;
	}

	ext_err = parse_extended_error(line);
	if (ext_err >= 0) {
		resp_buf[resp_len] = '\0';
		cmd_result  = ext_err;
		cmd_pending = false;
		k_sem_give(&at_resp_sem);
		return;
	}

	/* Intermediate response line: accumulate into buffer */
	size_t llen = strlen(line);

	if (resp_len + llen + 2 < AT_BUF_SIZE) {
		memcpy(&resp_buf[resp_len], line, llen);
		resp_len += llen;
		resp_buf[resp_len++] = '\r';
		resp_buf[resp_len++] = '\n';
	} else {
		LOG_WRN("AT response buffer full, dropping: %s", line);
	}
}

/* -------------------------------------------------------------------------
 * AT reader thread
 * ---------------------------------------------------------------------- */

static void at_reader_fn(void *p1, void *p2, void *p3)
{
	static uint8_t rx[256];
	int received;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	LOG_INF("AT reader: running on DLCI 2");

	while (true) {
		k_sem_take(&pipe_rx_sem, K_FOREVER);

		do {
			received = modem_pipe_receive(dlci2_pipe, rx, sizeof(rx));
			if (received <= 0) {
				break;
			}

			for (int i = 0; i < received; i++) {
				char c = (char)rx[i];

				if (c == '\n') {
					if (line_len > 0 &&
					    line_buf[line_len - 1] == '\r') {
						line_len--;
					}
					line_buf[line_len] = '\0';
					process_line(line_buf);
					line_len = 0;
				} else if (line_len < sizeof(line_buf) - 1) {
					line_buf[line_len++] = c;
				}
			}
		} while (received > 0);
	}
}

/* -------------------------------------------------------------------------
 * Internal: synchronous AT command send/receive
 * ---------------------------------------------------------------------- */

static int pipe_transmit_all(struct modem_pipe *pipe, const uint8_t *buf, size_t len)
{
	size_t sent = 0;

	while (sent < len) {
		int ret = modem_pipe_transmit(pipe, buf + sent, len - sent);

		if (ret < 0) {
			return ret;
		}

		if (ret == 0) {
			k_sleep(K_MSEC(1));
			continue;
		}

		sent += ret;
	}

	return 0;
}

static int at_cmd_send(char *out_buf, size_t out_len, const char *cmd)
{
	int result;
	int err;
	size_t cmd_len;

	if (dlci2_pipe == NULL) {
		return -ENODEV;
	}

	k_mutex_lock(&at_cmd_mutex, K_FOREVER);
	LOG_INF("AT >> %s", cmd);

	resp_len    = 0;
	resp_buf[0] = '\0';
	cmd_result  = 0;
	k_sem_reset(&at_resp_sem);

	/* Intercept AT+CFUN= so we can fire the cfun_handler on success */
	int cfun_set = -1;

	if (strncasecmp(cmd, "AT+CFUN=", 8) == 0) {
		(void)sscanf(cmd + 8, "%d", &cfun_set);
	}

	if (cfun_set == 0 || cfun_set == 4) {
		ppp_ipv4_ready = false;
		held_pdn_act_urc[0] = '\0';
	}

	/* When transitioning to normal mode, tell SLM to (re-)start PPP
	 * management before the modem goes online.  Without this the SLM
	 * never restarts its PPP handler after a CFUN=4 cycle and the PDN
	 * activation event (+CGEV) is never generated.
	 */
	if (cfun_set == 1 || cfun_set == 21) {
		LOG_INF("AT >> AT#XPPP=1 (SLM PPP restart before CFUN=%d)",
			cfun_set);
		strncpy(current_cmd, "AT#XPPP=1", sizeof(current_cmd) - 1);
		current_cmd[sizeof(current_cmd) - 1] = '\0';
		cmd_pending = true;
		err = pipe_transmit_all(dlci2_pipe,
					(const uint8_t *)"AT#XPPP=1\r\n", 11);
		if (err >= 0) {
			err = k_sem_take(&at_resp_sem, K_MSEC(5000));
		}
		LOG_INF("AT << AT#XPPP=1 result=%d", cmd_result);
		cmd_pending = false;
		k_sem_reset(&at_resp_sem);
		resp_len = 0;
		resp_buf[0] = '\0';
		cmd_result = 0;
	}

	strncpy(current_cmd, cmd, sizeof(current_cmd) - 1);
	current_cmd[sizeof(current_cmd) - 1] = '\0';
	cmd_pending = true;

	cmd_len = strlen(cmd);
	err = pipe_transmit_all(dlci2_pipe, (const uint8_t *)cmd, cmd_len);
	if (err < 0) {
		cmd_pending = false;
		k_mutex_unlock(&at_cmd_mutex);
		return err;
	}

	err = pipe_transmit_all(dlci2_pipe, (const uint8_t *)"\r\n", 2);
	if (err < 0) {
		cmd_pending = false;
		k_mutex_unlock(&at_cmd_mutex);
		return err;
	}

	err = k_sem_take(&at_resp_sem,
			 K_MSEC(CONFIG_NRF_MODEM_LIB_ZEPHYR_MODEM_RESPONSE_TIMEOUT_MS));
	if (err != 0) {
		cmd_pending = false;
		LOG_ERR("AT command timed out: %s", cmd);
		k_mutex_unlock(&at_cmd_mutex);
		return -EAGAIN;
	}

	LOG_INF("AT << result=%d resp=\"%s\"", cmd_result, resp_buf);

	result = cmd_result;

	if (out_buf != NULL && out_len > 0) {
		strncpy(out_buf, resp_buf, out_len - 1);
		out_buf[out_len - 1] = '\0';
	}

	if (result == 0 && cfun_set >= 0 && cfun_handler != NULL) {
		cfun_handler(cfun_set);
	}

	k_mutex_unlock(&at_cmd_mutex);
	return result;
}

/* -------------------------------------------------------------------------
 * nrf_modem_at_* API
 * ---------------------------------------------------------------------- */

int nrf_modem_at_notif_handler_set(nrf_modem_at_notif_handler_t callback)
{
	notif_handler = callback;
	return 0;
}

void nrf_modem_at_cfun_handler_set(nrf_modem_at_cfun_handler_t handler)
{
	cfun_handler = handler;
}

int nrf_modem_at_printf(const char *fmt, ...)
{
	va_list args;
	char small[AT_CMD_SIZE];
	char *cmd = small;
	int ret;

	va_start(args, fmt);
	ret = vsnprintk(small, sizeof(small), fmt, args);
	va_end(args);

	if (ret >= (int)sizeof(small)) {
		size_t needed = ret + 1;

		cmd = k_malloc(needed);
		if (!cmd) {
			return -ENOMEM;
		}
		va_start(args, fmt);
		vsnprintk(cmd, needed, fmt, args);
		va_end(args);
	}

	ret = at_cmd_send(NULL, 0, cmd);

	if (cmd != small) {
		k_free(cmd);
	}

	return ret;
}

int nrf_modem_at_scanf(const char *cmd, const char *fmt, ...)
{
	char resp[AT_BUF_SIZE];
	va_list args;
	int matched;
	int err;

	err = at_cmd_send(resp, sizeof(resp), cmd);
	if (err != 0) {
		return (err < 0) ? err : -NRF_EBADMSG;
	}

	va_start(args, fmt);
	matched = vsscanf(resp, fmt, args);
	va_end(args);

	return (matched > 0) ? matched : -NRF_EBADMSG;
}

int nrf_modem_at_cmd(void *buf, size_t len, const char *fmt, ...)
{
	va_list args;
	char small[AT_CMD_SIZE];
	char *cmd = small;
	int ret;

	va_start(args, fmt);
	ret = vsnprintk(small, sizeof(small), fmt, args);
	va_end(args);

	if (ret >= (int)sizeof(small)) {
		size_t needed = ret + 1;

		cmd = k_malloc(needed);
		if (!cmd) {
			return -ENOMEM;
		}
		va_start(args, fmt);
		vsnprintk(cmd, needed, fmt, args);
		va_end(args);
	}

	ret = at_cmd_send((char *)buf, len, cmd);

	if (cmd != small) {
		k_free(cmd);
	}

	return ret;
}

int nrf_modem_at_cmd_async(nrf_modem_at_resp_handler_t callback,
			   const char *fmt, ...)
{
	va_list args;
	char small[AT_CMD_SIZE];
	char *cmd = small;
	char resp[AT_BUF_SIZE];
	int ret;

	if (callback == NULL) {
		return -EINVAL;
	}

	va_start(args, fmt);
	ret = vsnprintk(small, sizeof(small), fmt, args);
	va_end(args);

	if (ret >= (int)sizeof(small)) {
		size_t needed = ret + 1;

		cmd = k_malloc(needed);
		if (!cmd) {
			return -ENOMEM;
		}
		va_start(args, fmt);
		vsnprintk(cmd, needed, fmt, args);
		va_end(args);
	}

	ret = at_cmd_send(resp, sizeof(resp), cmd);
	callback(resp);

	if (cmd != small) {
		k_free(cmd);
	}

	return ret;
}

int nrf_modem_at_cmd_custom_set(struct nrf_modem_at_cmd_custom *custom_commands,
				size_t len)
{
	ARG_UNUSED(custom_commands);
	ARG_UNUSED(len);
	return 0;
}

int nrf_modem_at_sem_timeout_set(int timeout_ms)
{
	ARG_UNUSED(timeout_ms);
	return 0;
}

/* -------------------------------------------------------------------------
 * nrf_modem_lib_init / shutdown
 * ---------------------------------------------------------------------- */

int nrf_modem_lib_init(void)
{
	if (initialized) {
		LOG_INF("nrf_modem_lib_init: already initialized");
		return 0;
	}

	initialized = true;
	LOG_INF("nrf_modem_lib_init: firing ON_INIT callbacks");

	STRUCT_SECTION_FOREACH(nrf_modem_lib_init_cb, e) {
		e->callback(0, e->context);
	}

	return 0;
}

int nrf_modem_lib_shutdown(void)
{
	if (!initialized) {
		return 0;
	}

	STRUCT_SECTION_FOREACH(nrf_modem_lib_shutdown_cb, e) {
		e->callback(e->context);
	}

	initialized = false;
	return 0;
}

int nrf_modem_lib_bootloader_init(void)
{
	return -ENOSYS;
}

/* -------------------------------------------------------------------------
 * CMUX event callback
 * ---------------------------------------------------------------------- */

static void cmux_event_cb(struct modem_cmux *_cmux,
			  enum modem_cmux_event event,
			  void *user_data)
{
	if (event == MODEM_CMUX_EVENT_CONNECTED) {
		LOG_INF("CMUX connected");
	} else if (event == MODEM_CMUX_EVENT_DISCONNECTED) {
		LOG_WRN("CMUX disconnected");
	}
}

/* -------------------------------------------------------------------------
 * Init sequence
 * ---------------------------------------------------------------------- */

#if HAS_MDM_RESET
static int reset_modem(void)
{
	int err;

	if (!gpio_is_ready_dt(&mdm_reset_spec)) {
		LOG_WRN("Modem reset GPIO not ready");
		return -ENODEV;
	}

	LOG_INF("Resetting modem via GPIO...");

	/* Configure as output, asserted (active state) */
	err = gpio_pin_configure_dt(&mdm_reset_spec, GPIO_OUTPUT_ACTIVE);
	if (err) {
		LOG_ERR("Failed to configure modem reset GPIO: %d", err);
		return err;
	}

	/* Hold reset for 100ms */
	k_sleep(K_MSEC(100));

	/* Deassert reset (release) */
	gpio_pin_set_dt(&mdm_reset_spec, 0);

	/* Wait for modem to boot (nRF91 SLM boot time is ~1-2 seconds) */
	LOG_INF("Waiting for modem to boot...");
	k_sleep(K_MSEC(2500));

	return 0;
}
#endif /* HAS_MDM_RESET */

static int run_init_sequence(void)
{
	int err;

#if HAS_MDM_RESET
	(void)reset_modem();
#endif

	/* 1. Open UART backend pipe */
	const struct modem_backend_uart_config uart_cfg = {
		.uart = MODEM_UART_DEV,
		.receive_buf = uart_rx_buf,
		.receive_buf_size = sizeof(uart_rx_buf),
		.transmit_buf = uart_tx_buf,
		.transmit_buf_size = sizeof(uart_tx_buf),
	};

	uart_pipe = modem_backend_uart_init(&uart_backend, &uart_cfg);
	if (!uart_pipe) {
		LOG_ERR("Failed to init UART backend");
		return -ENODEV;
	}

	/* Open the UART pipe to start RX DMA (uart_rx_enable). Without this,
	 * modem_pipe_transmit works (TX can always fire) but incoming bytes are
	 * never received, so every AT script times out regardless of hardware. */
	err = modem_pipe_open(uart_pipe, K_SECONDS(5));
	if (err) {
		LOG_ERR("Failed to open UART pipe: %d", err);
		return err;
	}
	LOG_INF("UART pipe opened");

	/* 2. Inject a CMUX DISC frame on DLCI 0 before the AT init script.
	 *
	 * Problem: the nRF54L SYS_INIT is reset independently of the nRF9160.
	 * If a previous boot successfully ran AT#XCMUX=1, the nRF9160 SLM is now
	 * in CMUX framing mode.  Raw AT bytes are silently discarded as invalid
	 * CMUX frames — the init script then always times out.
	 *
	 * Fix: send a CMUX DISC (Disconnect) frame on DLCI 0.  This is a valid
	 * CMUX command telling the mux to close its control channel and return to
	 * normal AT mode.  If the SLM is NOT in CMUX mode it simply ignores the
	 * non-printable bytes (or returns ERRORs which the allow_match handles).
	 *
	 * Frame = 0xF9 0x03 0x53 0x01 0xFD 0xF9
	 *   SOF=0xF9, Addr=DLCI0 C/R=1 EA=1 (0x03),
	 *   Ctrl=DISC|PF=1 (0x43|0x10=0x53), Len=0 EA=1 (0x01),
	 *   FCS=0xFD (0xFF - crc8_rohc(0xFF,[0x03,0x53,0x01])), EOF=0xF9
	 */
	static const uint8_t cmux_disc_frame[] = { 0xF9, 0x03, 0x53, 0x01, 0xFD, 0xF9 };

	LOG_INF("Injecting CMUX DISC to exit any previous CMUX session");
	err = modem_pipe_transmit(uart_pipe, cmux_disc_frame, sizeof(cmux_disc_frame));
	if (err < 0) {
		LOG_WRN("CMUX DISC transmit failed: %d (continuing anyway)", err);
	}
	/* Give the SLM 200 ms to process the DISC and return to AT mode */
	k_sleep(K_MSEC(200));

	/* 3. Attach modem_chat to UART pipe, run init script */
	/* AT responses are CR-terminated; discard bare LF characters. */
	static uint8_t chat_delimiter[] = "\r";
	static uint8_t chat_filter[]    = "\n";

	const struct modem_chat_config chat_cfg = {
		.user_data = NULL,
		.receive_buf = chat_rx_buf,
		.receive_buf_size = sizeof(chat_rx_buf),
		.delimiter = chat_delimiter,
		.delimiter_size = sizeof(chat_delimiter) - 1,
		.filter = chat_filter,
		.filter_size = sizeof(chat_filter) - 1,
		.argv = chat_argv,
		.argv_size = ARRAY_SIZE(chat_argv),
		.unsol_matches = NULL,
		.unsol_matches_size = 0,
	};

	err = modem_chat_init(&chat, &chat_cfg);
	if (err) {
		LOG_ERR("modem_chat_init: %d", err);
		return err;
	}

	err = modem_chat_attach(&chat, uart_pipe);
	if (err) {
		LOG_ERR("modem_chat_attach (uart): %d", err);
		return err;
	}

	LOG_INF("Running init script (AT#XCMUX=1)...");
	err = modem_chat_run_script(&chat, &init_script);
	if (err) {
		LOG_ERR("Init script failed: %d", err);
		modem_chat_release(&chat);
		return err;
	}

	LOG_INF("Init script done, starting CMUX");

	/* 3. Release chat from UART pipe; hand UART pipe to CMUX */
	modem_chat_release(&chat);

	const struct modem_cmux_config cmux_cfg = {
		.callback = cmux_event_cb,
		.receive_buf = cmux_rx_buf,
		.receive_buf_size = sizeof(cmux_rx_buf),
		.transmit_buf = cmux_tx_buf,
		.transmit_buf_size = sizeof(cmux_tx_buf),
	};

	modem_cmux_init(&cmux, &cmux_cfg);

	/* 4. Create DLCI pipes (DLCI 1 = will be PPP, DLCI 2 = will be AT) */
	const struct modem_cmux_dlci_config dlci1_cfg = {
		.dlci_address = 1,
		.receive_buf = dlci1_rx_buf,
		.receive_buf_size = sizeof(dlci1_rx_buf),
	};
	const struct modem_cmux_dlci_config dlci2_cfg = {
		.dlci_address = 2,
		.receive_buf = dlci2_rx_buf,
		.receive_buf_size = sizeof(dlci2_rx_buf),
	};

	dlci1_pipe = modem_cmux_dlci_init(&cmux, &dlci1, &dlci1_cfg);
	dlci2_pipe = modem_cmux_dlci_init(&cmux, &dlci2, &dlci2_cfg);

	err = modem_cmux_attach(&cmux, uart_pipe);
	if (err) {
		LOG_ERR("modem_cmux_attach: %d", err);
		return err;
	}

	err = modem_cmux_connect(&cmux);
	if (err) {
		LOG_ERR("modem_cmux_connect: %d", err);
		return err;
	}

        /* 5. Open DLCI 1 (sends SABM, waits for UA from SLM) then attach
	 *    chat and run dial script.  Without opening the pipe first, DLCI 1
	 *    is in MODEM_CMUX_DLCI_STATE_CLOSED and modem_pipe_transmit returns
	 *    immediately without writing anything — the dial script times out
	 *    waiting for a response that never arrives. */
	err = modem_pipe_open(dlci1_pipe, K_SECONDS(5));
	if (err) {
		LOG_ERR("modem_pipe_open (dlci1): %d", err);
		return err;
	}
	LOG_INF("DLCI 1 (initial AT channel) open");

	err = modem_chat_attach(&chat, dlci1_pipe);
	if (err) {
		LOG_ERR("modem_chat_attach (dlci1): %d", err);
		return err;
	}

	LOG_INF("Running dial script (AT#XPPP=1, AT#XCMUX=2)...");
	err = modem_chat_run_script(&chat, &dial_script);
	if (err) {
		LOG_ERR("Dial script failed: %d", err);
		modem_chat_release(&chat);
		return err;
	}

	LOG_INF("Dial script done");

	/* Release chat — DLCI 1 is now closed, DLCI 2 is the AT channel */
	modem_chat_release(&chat);

	/* 6. Open DLCI 2 for raw AT I/O */
	modem_pipe_attach(dlci2_pipe, pipe_event_cb, NULL);

	err = modem_pipe_open(dlci2_pipe, K_SECONDS(5));
	if (err) {
		LOG_ERR("modem_pipe_open (dlci2): %d", err);
		return err;
	}

	LOG_INF("DLCI 2 (AT channel) open");

	/* 7. Start AT reader thread */
	k_thread_create(&at_rx_thread, at_rx_stack,
			K_THREAD_STACK_SIZEOF(at_rx_stack),
			at_reader_fn, NULL, NULL, NULL,
			K_LOWEST_APPLICATION_THREAD_PRIO, 0, K_NO_WAIT);
	k_thread_name_set(&at_rx_thread, "at_proxy_rx");

	/* 8. Attach PPP to DLCI 1 (now the PPP channel) */
	err = modem_ppp_attach(&ppp, dlci1_pipe);
	if (err) {
		LOG_ERR("modem_ppp_attach: %d", err);
		return err;
	}

	LOG_INF("PPP attached to DLCI 1");

	/* 9. Bring the PPP network interface up and assert carrier.
	 *
	 * modem_ppp_ppp_api_init sets NET_IF_NO_AUTO_START and NET_IF_CARRIER_OFF
	 * when the interface is first initialised (POST_KERNEL).  Without explicitly
	 * calling net_if_up() here, the NET_IF_UP flag is never set and
	 * net_recv_data() returns -ENETDOWN for every incoming LCP frame, so PPP
	 * negotiation never completes.
	 *
	 * The correct order is:
	 *   1. net_if_up   — sets NET_IF_UP, enables L2 (starts PPP LCP state machine)
	 *   2. net_if_carrier_on — signals physical link ready; PPP sends LCP Config-Req
	 */
	{
		struct net_if *ppp_iface = modem_ppp_get_iface(&ppp);

		/* PPP is point-to-point and has no real link-layer address, but
		 * two asserts require a valid-looking address:
		 *   - notify_iface_up() (net_if.c): len > 0
		 *   - ipv6cp_add_iid() (ipv6cp.c):  len >= 6
		 * Use a 6-byte zero address of type UNKNOWN; PPP never uses the
		 * link address for forwarding. */
		static uint8_t ppp_dummy_lladdr[6] = {0};

		net_if_set_link_addr(ppp_iface, ppp_dummy_lladdr,
				     sizeof(ppp_dummy_lladdr), NET_LINK_UNKNOWN);
		net_if_up(ppp_iface);
		net_if_carrier_on(ppp_iface);
		LOG_INF("PPP iface up + carrier on — LCP negotiation started");
	}

	return 0;
}

/* -------------------------------------------------------------------------
 * SYS_INIT
 * ---------------------------------------------------------------------- */

static int nrf91_slm_modem_init(void)
{
	int err;

	printk("do we get here?\n");

	LOG_INF("nRF91 SLM modem proxy: initializing (CMUX over UART)");

	err = run_init_sequence();
	if (err) {
		LOG_ERR("Init sequence failed: %d — modem not available", err);
		return 0;
	}

	LOG_INF("nRF91 SLM modem proxy: ready, firing ON_INIT callbacks");
	nrf_modem_lib_init();

	return 0;
}

SYS_INIT(nrf91_slm_modem_init,
	 APPLICATION,
	 CONFIG_NRF_MODEM_LIB_ZEPHYR_MODEM_INIT_PRIORITY);

/* -------------------------------------------------------------------------
 * nrf_socket stubs (lte_link_control + others reference these)
 * ---------------------------------------------------------------------- */

/* Delegate to Zephyr's net_addr_pton; remap return value to inet_pton(3) semantics. */
int nrf_inet_pton(int af, const char *restrict src, void *restrict dst)
{
	return (net_addr_pton((sa_family_t)af, src, dst) == 0) ? 1 : 0;
}

/* With PPP + Zephyr DNS resolver, DNS addresses come from IPCP negotiation.
 * The lte_link_control fallback DNS path calls this; safely ignore it. */
int nrf_setdnsaddr(int family, const void *in_addr, uint32_t in_size)
{
	ARG_UNUSED(family);
	ARG_UNUSED(in_addr);
	ARG_UNUSED(in_size);
	return 0;
}
