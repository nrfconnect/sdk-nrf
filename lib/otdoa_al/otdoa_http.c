/*
 * Copyright (c) 2025 PHY Wireless, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-PHYW
 */

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <otdoa_al/otdoa_fs.h>
#include <otdoa_al/otdoa_api.h>
#include <otdoa_al/otdoa_otdoa2al_api.h>
#include "otdoa_al_log.h"
#include "otdoa_http.h"
#include "otdoa_http_rest.h"

LOG_MODULE_DECLARE(otdoa_al, LOG_LEVEL_INF);

/* forward declarations */
tOTDOA_HTTP_MEMBERS gHTTP = {};

/* forward declarations */
#ifdef OTDOA_ENABLE_BSA_CIPHER
static int encrypt_ubsa = 1;
#else
static int encrypt_ubsa;
#endif
static int compress_ubsa = 1;

void http_set_ubsa_params(int enc, int comp)
{
	encrypt_ubsa = enc;
	compress_ubsa = comp;
}

/* functions inside nordic code */
extern ssize_t http_send(int socket, const void *buffer, size_t length, int flags);
extern ssize_t http_recv(int socket, void *buffer, size_t length, int flags);
extern int http_errno(void);
extern bool SetSocketBlocking(int fd, bool blocking);
extern void http_sleep(int msec);
extern int32_t http_uptime(void);

/**
 * Set default values in gHTTP
 *
 * @param gHTTP Pointer to tOTDOA_HTTP_MEMBERS struct to initialize
 */
void otdoa_http_defaults(tOTDOA_HTTP_MEMBERS *pG)
{
	pG->req.uEcgi = 10242563;
	pG->req.uDlearfcn = 5230;
	pG->req.uRadius = 3000;
	pG->req.uNumCells = 50;
	pG->prsID = 0;
}

/**
 * Send a specified number of bytes of the request to the server
 *
 * @param pG Pointer to gHTTP containing request to send
 * @param n Number of bytes to send
 * @return 0 on success, else -1
 */
int http_send_request(tOTDOA_HTTP_MEMBERS *pG, const size_t n)
{
	int bytes;

	do {
		bytes = http_send(pG->fdSocket, &pG->szSend[pG->nOff], n - pG->nOff, 0);
		if (bytes < 0) {
			LOG_ERR("http_send_request: send failed: %s", strerror(errno));
			return -1;
		}
		pG->nOff += bytes;
	} while (pG->nOff < strlen(pG->szSend));
	return 0;
}

/**
 * Open a file and write data to it
 *
 * @param path Path to write data to
 * @param data Data to write to file
 * @param len Length of data to write
 * @return Positive number of bytes written to file, otherwise negative error code
 */
int http_write_to_file(const char *path, void *data, size_t len)
{
	LOG_DBG("Attempting to write %zu bytes from %p to %s", len, data, path);
	int rc = 0;
	tOFS_FILE *fp = FOPEN(path, "w");

	if (!fp) {
		return -errno;
	}

	rc = FWRITE(data, len, 1, fp);
	if (rc != len) {
		rc = -1;
	}

	FCLOSE(fp);

	return rc;
}

/**
 * Log a response string
 *
 * Searches for a string in a response, and logs the
 * line containing that string
 */
#define OTDOA_MAX_LOG_LENGTH 96

void log_response_string(const char *pszKey, const char *pszBuffer)
{
	const char *pszStart = strstr(pszBuffer, pszKey);

	if (pszStart) {
		size_t len = strstr(pszStart, "\n") - pszStart;

		if (len >= OTDOA_MAX_LOG_LENGTH) {
			len = OTDOA_MAX_LOG_LENGTH - 1;
		}

		static char szLog[OTDOA_MAX_LOG_LENGTH];

		szLog[len] = '\0'; /* ensure null terimination */
		strncpy(szLog, pszStart, len);
		LOG_INF("    %s", szLog);
	} else {
		LOG_ERR("Key %s not found in response", pszKey);
	}
}

/**
 * HTTP FSM top-level message handling
 * Replaces http_entry_point from nordic/otdoa_thread_http.c
 *
 * @param pMsg Pointer to message to handle
 * @return 0 on success, otherwise negative error code
 */
int otdoa_http_handle_message(tOTDOA_HTTP_MESSAGE *pMsg)
{
	int rc = 0;

#if HTTP_USE_SCRATCHPAD
	gHTTP.csBuffer = OTDOA_getScratch();
#else
	gHTTP.csBuffer = calloc(1, HTTPS_BUF_SIZE);
#endif
	if (!gHTTP.csBuffer) {
		LOG_ERR("http_entry_point: failed to alloc send/receive buffer\n");
		rc = -ENOMEM;
		goto exit_error;
	}

	switch (pMsg->header.u32MsgId) {

	case OTDOA_HTTP_MSG_GET_H1_UBSA:
		LOG_INF("HTTP received OTDOA_HTTP_MSG_GET_H1_UBSA");
		otdoa_http_h1_handle_message(pMsg);
		break;

	case OTDOA_HTTP_MSG_GET_H1_CONFIG_FILE:

		LOG_INF("HTTP received OTDOA_HTTP_MSG_GET_H1_CONFIG_FILE");
		otdoa_http_h1_handle_message(pMsg);
		break;

	case OTDOA_HTTP_MSG_REBIND_SOCKET:

		LOG_INF("HTTP received HTTPS_COMMAND_REBIND_SOCKET");
		/* rebind the socket in case the configured upload URL has changed. has to be a
		 * message because config file parsing is slow and I can't just do this at the end
		 * of GET_CONFIG_FILE above
		 */
		otdoa_http_h1_rebind(NULL);
		break;

	case OTDOA_HTTP_MSG_UPLOAD_OTDOA_RESULTS:

		LOG_INF("HTTP received HTTPS_COMMAND_UPLOAD_RESULTS");
		otdoa_http_h1_handle_message(pMsg);
		break;

	case OTDOA_HTTP_MSG_TEST_JWT:

		LOG_INF("HTTP received HTTPS_COMMAND_TEST_JWT");
		otdoa_http_h1_handle_message(pMsg);
		break;

	default:
		LOG_WRN("unexpected HTTP message");
		break;
	}

#if !HTTP_USE_SCRATCHPAD
	free(gHTTP.csBuffer);
#endif
	gHTTP.csBuffer = NULL;

exit_error:

	/* Possible bug fix - ensure we unbind */
	http_unbind(&gHTTP);

	return rc;
}

int otdoa_http_send_message(tOTDOA_HTTP_MESSAGE *pMsg, uint32_t u32Len)
{
	return otdoa_queue_http_message(pMsg, u32Len);
}

int otdoa_http_send_results_upload(const char *pURL, otdoa_api_results_t *pResults,
				   const char *p_notes, const char *p_true_lat,
				   const char *p_true_lon)
{
	tOTDOA_MSG_HTTP_UPLOAD_RESULTS msg = { 0 };

	msg.u32MsgId = OTDOA_HTTP_MSG_UPLOAD_OTDOA_RESULTS;
	msg.u32MsgLen = sizeof(msg);
	strncpy(msg.pURL, pURL, sizeof(msg.pURL));
	msg.pResults = pResults;
	msg.p_notes = p_notes;
	msg.p_true_lat = p_true_lat;
	msg.p_true_lon = p_true_lon;
	return otdoa_http_send_message((tOTDOA_HTTP_MESSAGE *)&msg, msg.u32MsgLen);
}

int otdoa_http_send_test_jwt(void)
{
	tOTDOA_MSG_HTTP_TEST_JWT msg = { 0 };

	msg.u32MsgId = OTDOA_HTTP_MSG_TEST_JWT;
	msg.u32MsgLen = sizeof(msg);
	return otdoa_http_send_message((tOTDOA_HTTP_MESSAGE *)&msg, msg.u32MsgLen);
}

int otdoa_http_send_rebind_socket(void)
{
	tOTDOA_MSG_HTTP_REBIND msg = { 0 };

	msg.u32MsgId = OTDOA_HTTP_MSG_REBIND_SOCKET;
	msg.u32MsgLen = sizeof(msg);
	return otdoa_http_send_message((tOTDOA_HTTP_MESSAGE *)&msg, msg.u32MsgLen);
}

/**
 * Perform HTTP setup
 *
 * @return 0 on success, otherwise negative error code
 */
void otdoa_http_init(void)
{
	otdoa_http_defaults(&gHTTP);
	otdoa_http_h1_blacklist_init(&gHTTP);
}

static char szDownloadURL[51] = {0};
void otdoa_http_set_download_url(const char *const pszURL)
{
	if (pszURL == NULL) {
		memset(szDownloadURL, 0, sizeof(szDownloadURL));
	} else {
		strncpy(szDownloadURL, pszURL, sizeof(szDownloadURL) - 1);
	}
}

const char *otdoa_http_get_download_url(void)
{
	if (szDownloadURL[0] != '\0') {
		return szDownloadURL;
	} else {
		return BSA_DL_SERVER_URL;
	}
}
