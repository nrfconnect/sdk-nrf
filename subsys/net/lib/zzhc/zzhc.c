/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(zzhc, CONFIG_ZZHC_LOG_LEVEL);

#include <zephyr.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <net/socket.h>
#include "zzhc_internal.h"

#define REGVER            2         /** Self-registration protocol version */
#define MANUFACTURER_CODE "NOD"     /** Manufactuerer code */
#define SERVER_PORT       9999      /** Server's port number */
#define RX_BUF_LEN        128       /** Length of Rx Buffer */
#define JSON_MAX_LEN      320       /** Length of json */
#define IMEI_LEN          15        /** Length of IMEI */
#define RETRY_TIMEOUT     3600      /** 1 hour, 3600s */
#define RETRY_CNT_MAX     3         /** Max. value of retry counter */
#define WHITELISTED_OP_ID 6         /** Whitelisted Operator ID */

#define THREAD_PRIORITY   K_PRIO_PREEMPT(CONFIG_ZZHC_THREAD_PRIO)
#define STACK_SIZE        CONFIG_ZZHC_STACK_SIZE

/**@brief Host name of self-registration server. */
static char const hostname[] = "zzhc.vnet.cn";

/**@brief +CEREG event prefix. */
static char const status_cereg[7] = "+CEREG:";

/**@brief %XSIM event prefix. */
static char const status_xsim[6] = "%XSIM:";

/**@brief Lookup table for chip revision. */
static char const chip_rev[4] = "XA01";

/**@brief Function to handle AT-command notifications. */
static void at_notif_handler(void *context, char *response)
{
	int rc;
	struct zzhc *ctx = (struct zzhc *)context;

	if (ctx == NULL) {
		return;
	}

	/* Check +CEREG events */
	if (memcmp(response, status_cereg, sizeof(status_cereg)) == 0) {
		rc = zzhc_get_at_param_short(ctx, response, 0);
		if (rc < 0) {
			return;
		}

		LOG_DBG("+CEREG: %d", rc);
		if (rc == NW_REGISTERED_HOME || rc == NW_REGISTERED_ROAM) {
			LOG_DBG("Registered to operator network");
			ctx->registered = true;
			zzhc_sem_post(ctx->sem);
		} else {
			LOG_DBG("De-registered from operator network");
			ctx->registered = false;
		}
		return;
	}

	/* Check %XSIM events */
	if (memcmp(response, status_xsim, sizeof(status_xsim)) == 0) {
		rc = zzhc_get_at_param_short(ctx, response, 0);
		if (rc < 0) {
			return;
		}

		LOG_DBG("%%XSIM: %d", rc);
		if (rc == 0) {
			return;
		}

		if (ctx->state == STATE_RETRY_TIMEOUT) {
			zzhc_wakeup(ctx->thread);
		}
	}
}

/**@brief Search and replace. */
static void search_char_and_replace(char *data, char tok, char substitute)
{
	char *cptr = data;

	while (cptr != NULL) {
		cptr = strchr(cptr, tok);
		if (cptr != NULL) {
			*cptr++ = substitute;
		}
	}
}

/**@brief Build encrypted json file. */
static int build_json(struct zzhc *ctx, char *buff, int len)
{
	char l_buff[JSON_MAX_LEN];
	char duple[96];
	char *cptr;
	int i = 0, rc;

	l_buff[0] = 0;

	/* Self-registration protocol version. */
	sprintf(duple, "{\"REGVER\":\"%d\",", REGVER);
	strcat(l_buff, duple);

	/* IMEI */
	if (zzhc_at_cmd_xfer("AT+CGSN=1", ctx->at_resp, AT_RESPONSE_LEN) != 0) {
		return -EIO;
	}
	cptr = strchr(ctx->at_resp, '"');
	if (cptr == NULL) {
		return -EILSEQ;
	}
	cptr++;
	cptr[IMEI_LEN] = 0;

	sprintf(duple, "\"MEID\":\"%s\",", cptr);
	strcat(l_buff, duple);

	/* MODEL */
	if (zzhc_at_cmd_xfer("AT+CGMM", ctx->at_resp, AT_RESPONSE_LEN) != 0) {
		return -EIO;
	}
	search_char_and_replace(ctx->at_resp, '-', '_');
	cptr = strstr(ctx->at_resp, "\r\n");
	if (cptr != NULL) {
		*cptr = 0;
	}

	rc = zzhc_read_chip_rev(&i);
	if (rc != 0) {
		return rc;
	}

	i = MIN(i, sizeof(chip_rev) - 1);
	LOG_DBG("i = %d", i);

	sprintf(duple, "\"MODEL\":\"%s-%s_B%cA\",", MANUFACTURER_CODE,
		ctx->at_resp, chip_rev[i]);
	strcat(l_buff, duple);

	/* SWVER */
	if (zzhc_at_cmd_xfer("AT%SHORTSWVER", ctx->at_resp,
		AT_RESPONSE_LEN) != 0) {
		return -EIO;
	}
	cptr = strstr(ctx->at_resp, "\r\n");
	if (cptr != NULL) {
		*cptr = 0;
	}
	i = strlen("%SHORTSWVER: ");
	sprintf(duple, "\"SWVER\":\"%s\",", &ctx->at_resp[i]);
	strcat(l_buff, duple);

	/* SIM1ICCID */
	sprintf(duple, "\"SIM1ICCID\":\"%s\",", ctx->ueiccid);
	strcat(l_buff, duple);

	/* SIM1LTEIMSI */
	if (zzhc_at_cmd_xfer("AT+CIMI", ctx->at_resp, AT_RESPONSE_LEN) != 0) {
		return -EIO;
	}
	cptr = strstr(ctx->at_resp, "\r\n");
	if (cptr != NULL) {
		*cptr = 0;
	}
	sprintf(duple, "\"SIM1LTEIMSI\":\"%s\"}", ctx->at_resp);
	strcat(l_buff, duple);

	return zzhc_base64(buff, len, l_buff, strlen(l_buff));
}

/**@brief Compile HTTP request packet. */
static int build_http_req(struct zzhc *ctx)
{
	char buff[64];
	char json_buff[JSON_MAX_LEN];
	int rc;

	strcat(ctx->http_pkt, "POST / HTTP/1.1\r\n");

	sprintf(buff, "Host: %s:%d\r\n", hostname, SERVER_PORT);
	strcat(ctx->http_pkt, buff);

	strcat(ctx->http_pkt, "User-Agent: zzhc/1.0.0\r\n");
	strcat(ctx->http_pkt, "Accept: */*\r\n");
	strcat(ctx->http_pkt, "x-forwarded-for: 117.61.5.70\r\n");
	strcat(ctx->http_pkt, "Content-Type: application/encrypted-json\r\n");
	strcat(ctx->http_pkt, "Connection: close\r\n");
	strcat(ctx->http_pkt, "Content-Length: ");

	rc = build_json(ctx, json_buff, JSON_MAX_LEN);
	if (rc <= 0) {
		return rc;
	}

	sprintf(buff, "%d\r\n\r\n", rc);
	strcat(ctx->http_pkt, buff);
	strcat(ctx->http_pkt, json_buff);

	ctx->http_pkt_len = strlen(ctx->http_pkt);
	return ctx->http_pkt_len;
}

/**@brief Resolve hostname and TCP connect. */
static int resolve_and_connect(struct zzhc *ctx)
{
	int rc;
	struct addrinfo *addr;
	struct addrinfo *info;

	if (ctx == NULL) {
		return -EINVAL;
	}

	/* Lookup host */
	struct addrinfo hints = {
		.ai_family = AF_INET,
		.ai_socktype = SOCK_STREAM,
		.ai_protocol = IPPROTO_TCP,
	};

	rc = getaddrinfo(hostname, NULL, &hints, &info);
	if (rc != 0) {
		LOG_DBG("Can't resolve hostname %s.", log_strdup(hostname));
		return -EIO;
	}

	LOG_DBG("Found host: %s.", hostname);

	ctx->sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ctx->sock_fd < 0) {
		LOG_DBG("Can't create socket, errno=%d.", errno);
		rc = -EIO;
		goto cleanup;
	}

	/* Not connected */
	rc = -1;

	/* Try to connect */
	for (addr = info; addr != NULL; addr = addr->ai_next) {
		struct sockaddr * const sa = addr->ai_addr;

		((struct sockaddr_in *)sa)->sin_port = htons(SERVER_PORT);
		rc = connect(ctx->sock_fd, sa, addr->ai_addrlen);
		if (rc == 0) {
			LOG_DBG("Connected to: %s.", hostname);
			break;
		}
	}

cleanup:
	freeaddrinfo(info);

	if (rc != 0) {
		LOG_DBG("Unable to connect.");
		close(ctx->sock_fd);
		ctx->sock_fd = -1;
	}

	return rc;
}

/**@brief TCP disconnect. */
static int disconnect(struct zzhc *ctx)
{
	int rc;

	if (ctx == NULL || ctx->sock_fd < 0) {
		return -EINVAL;
	}

	rc = close(ctx->sock_fd);
	if (rc) {
		LOG_DBG("Can't close socket, errno=%d", errno);
		return -errno;
	}

	LOG_DBG("Disconnected from: %s.", hostname);
	ctx->sock_fd = -1;
	return 0;
}

/**@brief Send HTTP POST request. */
static int post_request_send(struct zzhc *ctx)
{
	int sent;
	int len = ctx->http_pkt_len;
	size_t off = 0;

	LOG_DBG("Sending HTTP request (len=%d)...", ctx->http_pkt_len);

	while (len) {
		sent = send(ctx->sock_fd, ctx->http_pkt + off, len, 0);
		if (sent <= 0) {
			return -EIO;
		}

		off += sent;
		len -= sent;
	}

	memset(ctx->http_pkt, 0, ctx->http_pkt_len);
	ctx->http_pkt_len = 0;
	return 0;
}

/**@brief Check "\r\n\r\n", returns the offset of the last "\n". */
static int check_delimiter(struct zzhc *ctx, char *data, int len)
{
	for (int i = 0; i < len; i++) {
		ctx->delimiter = (ctx->delimiter << 8) | data[i];
		if (ctx->delimiter == 0x0D0A0D0A) {
			return i;
		}
	}
	return -1;
}

/**@brief Set delimiter buffer. */
static void set_delimiter_buff(struct zzhc *ctx, char *data, int len)
{
	int i;

	ctx->delimiter = 0;
	for (i = 0; i < len; i++) {
		ctx->delimiter = (ctx->delimiter << 8) | data[i];
	}
}

/**@brief Receive HTTP response. */
static int receive_response(struct zzhc *ctx)
{
	int i;
	char rx_buff[RX_BUF_LEN + 1];

	int len = recv(ctx->sock_fd, rx_buff, RX_BUF_LEN, 0);

	LOG_DBG("len=%d", len);
	if (len <= 0) {
		return len;
	}

	if (!ctx->header_recvd) {
		/* Search for the end of header */
		i = check_delimiter(ctx, rx_buff, len);
		if (i++ >= 0) {
			LOG_DBG("Found the end of header.");
			ctx->header_recvd = true;
			ctx->rx_offset = len - i;
			memcpy(ctx->http_pkt, rx_buff + i, len - i);
			ctx->http_pkt_len += len - i;
			LOG_DBG("Copied %d bytes.", len - i);
			return len;
		}

		i = MAX(len - 3, 0);
		set_delimiter_buff(ctx, &rx_buff[i], len - i);
	} else {
		i = MIN(HTTP_PKT_LEN - ctx->rx_offset, len);
		memcpy(ctx->http_pkt + ctx->rx_offset, rx_buff, i);
		ctx->http_pkt_len += i;
		LOG_DBG("Copied %d bytes.", i);
		ctx->rx_offset += i;
	}
	return len;
}

/**@brief Check simcard to see if PLMN is whitelisted.
 *
 * @return 0 if whitelisted, non-zero else.
 */
static int check_simcard_whitelist(struct zzhc *ctx)
{
	int rc;

	/* Read operator ID. For China Telecom, it's 6. */
	rc = zzhc_at_cmd_xfer("AT%XOPERID", ctx->at_resp, AT_RESPONSE_LEN);
	if (rc != 0) {
		return -EIO;
	}

	rc = zzhc_get_at_param_short(ctx, ctx->at_resp, 0);
	if (rc < 0) {
		return rc;
	}

	if (rc != WHITELISTED_OP_ID) {
		LOG_DBG("Simcard not allowed.");
		return -ENOENT;
	}

	return 0;
}

/**@brief Check ICCID
 *
 * @return 0 if identical, non-zero else.
 */
static int check_iccid(struct zzhc *ctx)
{
	int rc, i;
	char *cptr;
	char c;

	/* SIM1ICCID */
	if (zzhc_at_cmd_xfer("AT+CRSM=176,12258,0,0,0", ctx->at_resp,
		AT_RESPONSE_LEN) != 0) {
		return -EIO;
	}

	cptr = strchr(ctx->at_resp, '"');
	if (cptr == NULL) {
		return -EILSEQ;
	}
	cptr++;

	/* Swap bytes, including the extra check-digit */
	for (i = 0; i <= ICCID_LEN; i += 2) {
		c = cptr[i];
		cptr[i] = cptr[i + 1];
		cptr[i + 1] = c;
	}
	cptr[ICCID_LEN] = 0;

	rc = memcmp(ctx->ueiccid, cptr, ICCID_LEN);
	if (rc == 0) {
		LOG_DBG("Identical ICCID.");
	} else {
		memcpy(ctx->ueiccid, cptr, ICCID_LEN);
	}

	return rc;
}

/**@brief Finite state machine */
static int fsm(struct zzhc *ctx)
{
	int rc;

	switch (ctx->state) {
	case STATE_INIT:
		LOG_DBG("STATE_INIT");

		zzhc_sem_init(ctx->sem, 0, 0);
		zzhc_register_handler(ctx, at_notif_handler);
		ctx->state = STATE_WAIT_FOR_REG;
		break;

	case STATE_WAIT_FOR_REG:
		LOG_DBG("STATE_WAIT_FOR_REG");

		if (zzhc_at_cmd_xfer("AT+CEREG?", ctx->at_resp,
			AT_RESPONSE_LEN) != 0) {
			return -EIO;
		}

		rc = zzhc_get_at_param_short(ctx, ctx->at_resp, 1);
		if (rc == NW_REGISTERED_HOME || rc == NW_REGISTERED_ROAM) {
			ctx->registered = true;
		} else {
			zzhc_sem_wait(ctx->sem);
		}

		if (check_iccid(ctx) == 0 ||
			check_simcard_whitelist(ctx) != 0) {
			ctx->state = STATE_END;
		} else {
			ctx->state = STATE_START;
		}
		break;

	case STATE_START:
		LOG_DBG("STATE_START");

		/* Compile HTTP request. */
		rc = build_http_req(ctx);
		if (rc <= 0) {
			ctx->state = STATE_RETRY_TIMEOUT;
			break;
		}

		/* Send. */
		if (resolve_and_connect(ctx) != 0) {
			ctx->state = STATE_RETRY_TIMEOUT;
			break;
		}

		if (post_request_send(ctx) != 0) {
			ctx->state = STATE_RETRY_TIMEOUT;
			break;
		}

		ctx->rx_offset = 0;
		ctx->delimiter = 0;
		ctx->header_recvd = false;
		ctx->state = STATE_HTTP_REQ_SENT;
		break;

	case STATE_HTTP_REQ_SENT:
		LOG_DBG("STATE_HTTP_REQ_SENT");

		rc = receive_response(ctx);
		if (rc < 0) {
			LOG_DBG("Error reading from socket, errno=%d", errno);
			ctx->state = STATE_RETRY_TIMEOUT;
			break;
		}

		if (rc == 0) {
			LOG_DBG("Peer closed connection!");
			ctx->state = STATE_HTTP_RESP_RECVD;
			break;
		}
		break;

	case STATE_HTTP_RESP_RECVD:
		LOG_DBG("STATE_HTTP_RESP_RECVD");

		/* Parse HTTP response. */
		if (zzhc_check_http_payload(ctx)) {
			/* Try to write ICCID to local storage. */
			rc = zzhc_save_iccid(ctx->ueiccid, ICCID_LEN);
			if (rc <= 0) {
				LOG_DBG("zzhc_save_iccid() = %d", rc);
			}

			ctx->state = STATE_END;
		} else {
			ctx->state = STATE_RETRY_TIMEOUT;
		}
		break;

	case STATE_RETRY_TIMEOUT:
		LOG_DBG("STATE_RETRY_TIMEOUT");

		/* Cleanup */
		disconnect(ctx);
		ctx->http_pkt_len = 0;
		memset(ctx->http_pkt, 0, HTTP_PKT_LEN);

		if (ctx->retry_cnt >= RETRY_CNT_MAX) {
			ctx->state = STATE_END;
			break;
		}

		zzhc_sleep(RETRY_TIMEOUT);
		ctx->retry_cnt++;
		ctx->state = (ctx->registered)
			? STATE_START : STATE_WAIT_FOR_REG;
		break;

	case STATE_END:
		LOG_DBG("STATE_END");

		/* Clean-up */
		disconnect(ctx);
		zzhc_deregister_handler(ctx, at_notif_handler);
		return -ECANCELED;

	default:
		LOG_DBG("Unknown state=%d", (int)ctx->state);
		return -ECANCELED;
	}
	return 0;
}

/**@brief Function for initializing self-registration daemon. */
static void zzhc_init(void *arg1, void *arg2, void *arg3)
{
	int rc;
	struct zzhc *ctx;

	/* Unconditionally subscribe +CEREG events */
	if (zzhc_at_cmd_xfer("AT+CEREG=5", NULL, 0) != 0) {
		LOG_DBG("Can't subscribe to +CEREG events.");
		return;
	}

	/* Unconditionally subscribe %XSIM events */
	if (zzhc_at_cmd_xfer("AT%XSIM=1", NULL, 0) != 0) {
		LOG_DBG("Can't subscribe to %%XSIM events.");
		return;
	}

	/* Allocate resource. */
	ctx = (struct zzhc *)zzhc_malloc(sizeof(struct zzhc));
	if (ctx == NULL) {
		LOG_DBG("Can't allocate memory for context.");
		return;
	}

	/* Initial values */
	memset(ctx, 0, sizeof(struct zzhc));
	ctx->state      = STATE_INIT;
	ctx->registered = false;
	ctx->sim_ready  = false;
	ctx->sock_fd    = -1;
	ctx->retry_cnt  = 0;
	ctx->thread     = zzhc_get_current_thread();

	/* Initialize external modules */
	rc = zzhc_ext_init(ctx);
	if (rc != 0) {
		goto zzhc_init_cleanup;
	}

	/* Try to read ICCID from local storage. */
	rc = zzhc_load_iccid(ctx->ueiccid, ICCID_LEN);
	if (rc < 0) {
		LOG_DBG("zzhc_load_iccid() = %d", rc);
	}

	while (true) {
		if (fsm(ctx) == -ECANCELED) {
			break;
		}
	}

zzhc_init_cleanup:
	zzhc_ext_uninit(ctx);
	zzhc_free(ctx);
	LOG_DBG("Exit.");
}

K_THREAD_DEFINE(zzhc_thread, STACK_SIZE,
		zzhc_init, NULL, NULL, NULL,
		THREAD_PRIORITY, 0, K_NO_WAIT);
