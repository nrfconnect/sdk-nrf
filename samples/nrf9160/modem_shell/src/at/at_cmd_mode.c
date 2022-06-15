/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <stdlib.h>

#include <zephyr/shell/shell.h>
#include <modem/at_monitor.h>
#include <nrf_modem_at.h>

#include "mosh_defines.h"
#include "mosh_print.h"

#include "at_cmd.h"
#include "at_cmd_mode.h"

extern struct k_work_q mosh_common_work_q;

bool at_cmd_mode_dont_print;

static enum line_termination term_mode = CONFIG_MOSH_AT_CMD_MODE_TERMINATION;

#define AT_CMD_MODE_AT_CMD_MAX_LEN CONFIG_SHELL_CMD_BUFF_SIZE
#define AT_CMD_MODE_PIPELINED_AT_CMD_MAX_LEN 1023

static uint8_t at_buf[AT_CMD_MODE_AT_CMD_MAX_LEN + 1];
static uint8_t at_buf_pipelined[AT_CMD_MODE_PIPELINED_AT_CMD_MAX_LEN + 1];

AT_MONITOR(mosh_at_cmd_mode_handler, ANY, at_cmd_mode_event_handler, PAUSED);

struct at_cmd_mode_pipelining_info {
	uint8_t *pipelined_cmd_start; /* Pointers to at_buf with pipelining */
	int pipelined_cmd_len;
};

#define AT_MAX_CMD_PIPELINED 10
struct at_cmd_mode_work_data {
	struct k_work work;
	int pipe_cnt;
	struct at_cmd_mode_pipelining_info pipe_infos[AT_MAX_CMD_PIPELINED];
};
static struct at_cmd_mode_work_data at_cmd_mode_work;

static bool at_cmd_mode_active;
static bool inside_quotes;
static bool writing;
static size_t at_cmd_len;

K_MUTEX_DEFINE(at_buf_mutex);

static void at_cmd_mode_event_handler(const char *response)
{
	if (writing) {
		/* 1st we need to clear current writings */
		printk("\r");
		for (int i = 0; i < at_cmd_len; i++) {
			printk(" ");
		}

		/* Then print AT notification */
		printk("\r%s", response);

		/* And at last, print the command that user was writing back */
		k_mutex_lock(&at_buf_mutex, K_FOREVER);
		at_buf[at_cmd_len] = '\0';
		printk("%s", at_buf);
		k_mutex_unlock(&at_buf_mutex);
	} else {
		printk("%s", response);
	}
}

static void at_cmd_mode_send_at_cmd(const char *at_str, char *rsp_buf, int rsp_buf_len)
{
	int err;

	/* 1st: handle MoSh specific AT commands */
	err = at_cmd_mosh_execute(at_str);
	if (err == 0) {
		sprintf(rsp_buf, "OK\n");
		goto done;
	} else if (err != -ENOENT) {
		sprintf(rsp_buf, "ERROR\n");
		goto done;
	}

	/* 2nd: modem at commands */
	err = nrf_modem_at_cmd(rsp_buf, rsp_buf_len, "%s", at_str);
	if (err < 0) {
		sprintf(rsp_buf, "ERROR: AT command failed: %d\n", err);
	} /* else if (err > 0): print error from modem */
done:
	/* Response */
	printk("%s", rsp_buf);
}

static void at_cmd_mode_send_at_cmd_from_at_buf(void)
{
	at_cmd_mode_send_at_cmd(at_buf, at_buf, sizeof(at_buf));
}

static void at_cmd_mode_worker(struct k_work *item)
{
	int i;
	struct at_cmd_mode_work_data *data = CONTAINER_OF(item, struct at_cmd_mode_work_data, work);
	struct at_cmd_mode_pipelining_info *pipes = data->pipe_infos;

	if (data->pipe_cnt) {
		for (i = 0; i < AT_MAX_CMD_PIPELINED; i++) {
			if (pipes[i].pipelined_cmd_len) {
				strncpy(at_buf_pipelined, pipes[i].pipelined_cmd_start,
					pipes[i].pipelined_cmd_len);
				at_buf_pipelined[pipes[i].pipelined_cmd_len] = '\0';

				at_cmd_mode_send_at_cmd(at_buf_pipelined, at_buf_pipelined,
							sizeof(at_buf_pipelined));
				pipes[i].pipelined_cmd_len = 0;
			}
		}
	} else {
		at_cmd_mode_send_at_cmd_from_at_buf();
	}
	data->pipe_cnt = 0;
}

static void at_cmd_mode_cmd_rx_handler(uint8_t character)
{
	writing = true;

	/* Handle control characters */
	switch (character) {
	case 0x08: /* Backspace. */
		/* Fall through. */
	case 0x7F: /* DEL character */
		if (at_cmd_len > 0) {
			/* Reprint with DEL/Backspace  */
			k_mutex_lock(&at_buf_mutex, K_FOREVER);
			at_buf[at_cmd_len] = '\0';
			at_cmd_len--;
			at_buf[at_cmd_len] = ' ';
			printk("\r%s", at_buf);
			at_buf[at_cmd_len] = '\0';
			printk("\r%s", at_buf);
			k_mutex_unlock(&at_buf_mutex);
		}
		return;
	}

	/* Handle termination characters, if outside quotes. */
	if (!inside_quotes) {
		switch (character) {
		case '\0':
			if (term_mode == NULL_TERM) {
				goto send;
			}
			/* Ignored null and will terminate string early. */
			break;
		case '\r':
			if (term_mode == CR_TERM) {
				goto send;
			}
			break;
		case '\n':
			if (term_mode == LF_TERM) {
				goto send;
			}
			if (term_mode == CR_LF_TERM && at_cmd_len > 0 &&
			    at_buf[at_cmd_len - 1] == '\r') {
				goto send;
			}
			break;
		}
	}

	/* Detect AT command buffer overflow, leaving space for null */
	if (at_cmd_len + 1 > sizeof(at_buf) - 1) {
		printk("Buffer overflow, dropping '%c'\n", character);
		return;
	}

	/* Write character to AT buffer */
	k_mutex_lock(&at_buf_mutex, K_FOREVER);
	at_buf[at_cmd_len] = character;
	at_cmd_len++;
	k_mutex_unlock(&at_buf_mutex);

	/* Handle special written character */
	if (character == '"') {
		inside_quotes = !inside_quotes;
	}

	/* Echo */
	printk("%c", character);
	return;
send:

	k_mutex_lock(&at_buf_mutex, K_FOREVER);
	at_buf[at_cmd_len] = '\0'; /* Terminate the command string */
	inside_quotes = false;
	at_cmd_len = 0;
	k_mutex_unlock(&at_buf_mutex);

	/* Scan AT command buffer for possible pipelined at commands */
	at_cmd_mode_work.pipe_cnt = 0;

	if (strchr(at_buf, '|')) {
		struct at_cmd_mode_pipelining_info *pipes = at_cmd_mode_work.pipe_infos;
		char *next_char;
		char *tmp;
		int pipelined_at_cmd_len;

		next_char = at_buf;
		tmp = strtok(next_char, "|");

		while (tmp != NULL) {
			if (at_cmd_mode_work.pipe_cnt >= AT_MAX_CMD_PIPELINED) {
				printk("\nMax %d pipelined at commands supported, dropping: \"%s\"",
				       AT_MAX_CMD_PIPELINED, tmp);
			} else {
				pipelined_at_cmd_len = strlen(tmp);
				if (pipelined_at_cmd_len > AT_CMD_MODE_PIPELINED_AT_CMD_MAX_LEN) {
					printk("\nPipelined AT cmd max len is %d,"
					       " too long %d bytes AT cmd dropped",
					       AT_CMD_MODE_PIPELINED_AT_CMD_MAX_LEN,
					       pipelined_at_cmd_len);
				} else {
					pipes[at_cmd_mode_work.pipe_cnt].pipelined_cmd_start = tmp;
					pipes[at_cmd_mode_work.pipe_cnt].pipelined_cmd_len =
						pipelined_at_cmd_len;
					at_cmd_mode_work.pipe_cnt++;
				}
			}
			tmp = strtok(NULL, "|");
		}
	}

	/* Echo a line feed */
	printk("\n");

	writing = false;

	k_work_submit_to_queue(&mosh_common_work_q, &at_cmd_mode_work.work);
}

/* ctrl + x */
#define CHAR_CAN 0x18
/* ctrl + q */
#define CHAR_DC1 0x11

static void at_cmd_mode_bypass_cb(const struct shell *sh, uint8_t *recv, size_t len)
{
	static uint8_t tail;
	bool escape = false;

	/* Check if escape criteria is met. */
	if (tail == CHAR_CAN && recv[0] == CHAR_DC1) {
		escape = true;
	} else {
		for (int i = 0; i < (len - 1); i++) {
			if (recv[i] == CHAR_CAN && recv[i + 1] == CHAR_DC1) {
				escape = true;
				break;
			}
		}
	}

	if (escape) {
		/* Cannot use shell/mosh_print in bybass callback */
		printk("===========================================================\n");
		printk("MoSh AT command mode exited\n");

		shell_set_bypass(sh, NULL);
		at_cmd_mode_dont_print = false;
		at_monitor_pause(&mosh_at_cmd_mode_handler);
		at_cmd_mode_active = false;
		inside_quotes = false;
		tail = 0;
		at_cmd_len = 0;
		return;
	}

	/* Store last byte for escape sequence detection */
	tail = recv[len - 1];

	/* Handle byte by byte. */
	for (int i = 0; i < len; i++) {
		at_cmd_mode_cmd_rx_handler(recv[i]);
	}
}

void at_cmd_mode_line_termination_set(enum line_termination line_term)
{
	term_mode = line_term;
}

int at_cmd_mode_start(const struct shell *sh)
{
	if (at_cmd_mode_active) {
		printk("AT command mode already started\n");
		return -EBUSY;
	}

	k_work_init(&at_cmd_mode_work.work, at_cmd_mode_worker);
	at_cmd_mode_work.pipe_cnt = 0;

	printk("MoSh AT command mode started, press ctrl-x ctrl-q to escape\n");
	printk("MoSh specific AT commands:\n");
#if defined(CONFIG_MOSH_PING)
	printk("  ICMP Ping: ");
	printk("AT+NPING=<addr>[,<payload_length>,<timeout_msecs>,<count>"
		       "[,<interval_msecs>[,<cid>]]]\n");
#endif
	printk("Other custom functionalities:\n");
	printk("  AT command pipelining, for example:\n"
		       "    at+cgmr|at+cfun?|at+nping=\"example.com\"\n");
	printk("===========================================================\n");
	at_cmd_mode_dont_print = true;
	at_cmd_mode_active = true;

	at_monitor_resume(&mosh_at_cmd_mode_handler);
	shell_set_bypass(sh, at_cmd_mode_bypass_cb);

	return 0;
}
