/*
 * Copyright (c) 2025 PHY Wireless, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-PHYW
 */

#include <errno.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

#include <otdoa_al/otdoa_otdoa2al_api.h>
#include <otdoa_al/otdoa_al2otdoa_api.h>
#include "otdoa_http.h"
#include "otdoa_http_rest.h"
#include "otdoa_al_log.h"

LOG_MODULE_DECLARE(otdoa_al, LOG_LEVEL_DBG);


#define NONBLOCK_RETRY_MILLISECONDS 100
#define NONBLOCK_RETRY_LIMIT	    (13 * 10) /* 13 seconds */

extern ssize_t http_send(int socket, const void *buffer, size_t length, int flags);
extern ssize_t http_recv(int socket, void *buffer, size_t length, int flags);
extern int http_errno(void);
extern bool SetSocketBlocking(int fd, bool blocking);
extern void http_sleep(int msec);
extern int32_t http_uptime(void);

/* forward references: */
int otdoa_http_h1_process_config_data(tOTDOA_HTTP_MEMBERS *pG);

/* static function declarations for message handling */
static int handle_get_ubsa_message(tOTDOA_HTTP_MESSAGE *pMsg);
static int handle_get_config_message(tOTDOA_HTTP_MESSAGE *pMsg);
#ifdef CONFIG_OTDOA_ENABLE_RESULTS_UPLOAD
static int handle_upload_results_message(tOTDOA_HTTP_MESSAGE *pMsg);
#endif
static int handle_test_jwt_message(tOTDOA_HTTP_MESSAGE *pMsg);

/* static function declarations for UBSA download handling */
static int handle_ubsa_authentication(tOTDOA_HTTP_MEMBERS *p_http);
static int handle_ubsa_download_loop(tOTDOA_HTTP_MEMBERS *p_http);

/* static function declarations for HTTP receive handling */
static int receive_data_with_retry(tOTDOA_HTTP_MEMBERS *pG, size_t nAvail, int *iCount);

/* static function declarations for auth response processing */
static otdoa_api_error_codes_t handle_encryption_parameters(tOTDOA_HTTP_MEMBERS *pG,
	char *pValue, int iTokenCount);
static otdoa_api_error_codes_t handle_token_and_delay(tOTDOA_HTTP_MEMBERS *pG,
	char *pValue, int iTokenCount);

/* static function declarations for range response header processing */
static otdoa_api_error_codes_t validate_range_response_inputs(tOTDOA_HTTP_MEMBERS *pG,
	int *iHeaderLen, int *iTokenCount);
static otdoa_api_error_codes_t handle_http_response_code(tOTDOA_HTTP_MEMBERS *pG,
	char *pResponse, int iTokenCount, bool *bPartialContent);
static otdoa_api_error_codes_t handle_content_length_header(tOTDOA_HTTP_MEMBERS *pG,
	char *pLengthField);
static otdoa_api_error_codes_t handle_partial_content_response(tOTDOA_HTTP_MEMBERS *pG,
	int iTokenCount);
static otdoa_api_error_codes_t handle_non_partial_content_response(tOTDOA_HTTP_MEMBERS *pG,
	int iLastSegmentEnd);

#define HTTP_LINE_TERMINATOR "\r\n"

extern tOTDOA_HTTP_MEMBERS gHTTP;

/* enable encryption & compression by default*/
#ifdef OTDOA_ENABLE_BSA_CIPHER
static int encrypt_ubsa = 1;
#else
static int encrypt_ubsa;
#endif
static int compress_ubsa = 1;
static int nOverrideAuthResp; /* non-zero value overrides initial auth. response (for testing) */
static bool bSkipConfigDL;
static bool bDisableEncryption;

void http_set_ubsa_params_h1(int enc, int comp)
{
	encrypt_ubsa = enc;
	compress_ubsa = comp;
}

void otdoa_disable_tls(bool bDisableTLS)
{
	gHTTP.bDisableTLS = bDisableTLS;
}
bool otdoa_get_disable_tls(void)
{
	return gHTTP.bDisableTLS;
}

void otdoa_skip_config_dl(bool bSkipCfgDL)
{
	bSkipConfigDL = bSkipCfgDL;
}
bool otdoa_get_skip_config_dl(void)
{
	return bSkipConfigDL;
}

void otdoa_http_disable_encrypt(bool bDisableEncr)
{
	bDisableEncryption = bDisableEncr;
}
bool otdoa_http_get_encryption_disable(void)
{
	return bDisableEncryption;
}

void otdoa_http_override_auth_resp(int override)
{
	nOverrideAuthResp = override;
}
int otdoa_http_get_override_auth_resp(void)
{
	return nOverrideAuthResp;
}

/*
 * Returns an error if buffer is not allocated when we attempt
 * to free.  This allows unit tests to check for multiple frees
 * For application code, you should probably NOT check this return
 */
int otdoa_http_h1_free_cs_buffer(void)
{
	int iRC = (gHTTP.csBuffer ? 0 : -1);
#if !HTTP_USE_SCRATCHPAD
	free(gHTTP.csBuffer);
#endif
	gHTTP.csBuffer = NULL;
	return iRC;
}

int otdoa_http_h1_get_cs_buffer(void)
{
	if (gHTTP.csBuffer) {
		LOG_WRN("CS Buffer %p already initialized", (void *)(gHTTP.csBuffer));
		otdoa_http_h1_free_cs_buffer();
	}

	_Static_assert(HTTP_BUF_SIZE >= HTTPS_BUF_SIZE, "Invalid HTTP buffer size");
#if HTTP_USE_SCRATCHPAD
	_Static_assert(sizeof(tOTDOA_SCRATCHPAD) > HTTP_BUF_SIZE);
	gHTTP.csBuffer = OTDOA_getScratch();
#else
	gHTTP.csBuffer = calloc(1, HTTP_BUF_SIZE);
#endif
	if (gHTTP.csBuffer == NULL) {
		LOG_ERR("Failed to allocate CS buffer.");
		return -1;
	}
	return 0;
}

/**
 * Check for a pending stop request message
 * returns true if stop request is pending
 */
bool otdoa_http_check_pending_stop(void)
{
	return otdoa_message_check_pending_stop();
}

/**
 * Copy the ubsa parameters from the message to the global http structure
 *
 * @param pG pointer to gH1_HTTP
 * @param pM message to copy into gH1_HTTP
 */
void otdoa_http_h1_copy_ubsa_params(tOTDOA_HTTP_MEMBERS *pG, const tOTDOA_HTTP_MESSAGE *pM)
{
	pG->nRange = 0;
	pG->req.uEcgi = pM->http_get_ubsa.uEcgi;
	pG->req.uDlearfcn = pM->http_get_ubsa.uDlearfcn;
	pG->req.uRadius = pM->http_get_ubsa.uRadius;
	pG->req.uNumCells = pM->http_get_ubsa.uNumCells;
	pG->req.uMCC = pM->http_get_ubsa.u16MCC;
	pG->req.uMNC = pM->http_get_ubsa.u16MNC;
	pG->req.uPCI = pM->http_get_ubsa.u16PCI;
}

/**
 * Convert a string to a signed integer with comprehensive error checking
 *
 * @param[in] str String to convert
 * @param[out] result Pointer to store the converted integer
 * @return 0 on success, -1 on error
 */
static int parse_string_to_int_signed(const char *str, int *result)
{
	char *endptr;

	errno = 0;

	long temp = strtol(str, &endptr, 10);

	if (endptr == str || errno == ERANGE || temp < INT_MIN ||
	    temp > INT_MAX) {
		return -1;
	}

	*result = (int)temp;
	return 0;
}

/**
 * Convert a string to an unsigned integer with comprehensive error checking
 *
 * @param[in] str String to convert
 * @param[out] result Pointer to store the converted unsigned integer
 * @return 0 on success, -1 on error
 */
static int parse_string_to_int_unsigned(const char *str, unsigned int *result)
{
	char *endptr;

	errno = 0;

	unsigned long temp = strtoul(str, &endptr, 10);

	/* reject invalid formats */
	if (endptr == str || errno == ERANGE || temp > UINT_MAX) {
		return -1;
	}

	*result = (unsigned int)temp;
	return 0;
}

/**
 * Parse a hex-encoded key string into a byte array
 *
 * @param[in] keystr Key string to parse
 * @param[out] key Array to parse keystring into
 * @param[in] keylen Length of key array (destination array)
 * @return 0 on success, otherwise -1
 */
int parse_keystr(const char *keystr, uint8_t *key, size_t keylen)
{
	char tmp[3] = {0};

	/* clear output */
	memset(key, 0, keylen);

	/*
	 * check the length - must be even (2 chars per output byte)
	 * and max of 2*keylen chars long
	 */
	size_t len = strnlen(keystr, 2 * keylen + 1);

	const char *cur = keystr;

	for (size_t count = 0; count < len / 2; count++) {
		tmp[0] = *cur++;
		tmp[1] = *cur++;

		char *endptr;
		unsigned long value = strtoul(tmp, &endptr, 16);

		/* Check if conversion was successful */
		if (endptr != tmp + 2 || *endptr != '\0') {
			LOG_ERR("Invalid character in hex string: %s", tmp);
			return -1; /* Error: invalid hex character or incomplete conversion */
		}
		/* Check for invalid conversion due to negative input value */
		if (value > UINT8_MAX) {
			LOG_ERR("Invalid conversion in parse_keystr(): %s", tmp);
			return -1;
		}

		key[count] = (uint8_t)value;
	}
	return 0;
}
/**
 * Send a connect to the server
 *
 * @param pG Pointer to gH1_HTTP containing request data
 * @param host Hostname to connect to
 * @return 0 on success, otherwise error code
 */
int otdoa_http_h1_send_connect(tOTDOA_HTTP_MEMBERS *pG, const char *host)
{
	int iRC = http_connect(pG, host);

	if (iRC) {
		LOG_ERR("http_connect failed: %d", iRC);
		return OTDOA_EVENT_FAIL_NTWK_CONN;
	}

	return OTDOA_API_SUCCESS;
}

/**
 * Send a disconnect to the server
 *
 * @param pG Pointer to gH1_HTTP containing request data
 * @return 0 on success, otherwise error code
 */
int otdoa_http_h1_send_disconnect(tOTDOA_HTTP_MEMBERS *pG)
{
	if (http_disconnect(pG)) {
		return OTDOA_EVENT_FAIL_NTWK_CONN;
	}

	return OTDOA_API_SUCCESS;
}

/**
 * Format the HTTPS H1 authorization request. The
 * Authorization header is the initial header to request a token
 *
 * @param pG Pointer to gH1_HTTP containing request data
 * @return Number of bytes written to header
 */
int otdoa_http_h1_format_auth_request(tOTDOA_HTTP_MEMBERS *pG)
{
	int iRC = 0;

	char jwt_token[256];
	size_t jwt_len = 0;

	if (NULL == pG) {
		LOG_ERR("http_h1_format_request: NULL pG!");
		/* iRC is already 0 (error) */
		goto exit;
	}

	iRC = generate_jwt(jwt_token, sizeof(jwt_token), &jwt_len);
	if (iRC != 0) {
		LOG_ERR("http_h1_format_request: generate_jwt() error: %d", iRC);
		iRC = 0;
		goto exit;
	}
	jwt_token[jwt_len] = '\0'; /* ensure null-termination */

	if (NULL == pG->csBuffer) {
		LOG_ERR("http_h1_format_request: NULL pG->csBuffer!");
		/* iRC is already 0 (error) */
		goto exit;
	}

	unsigned int uBufferLen = pG->bDisableTLS ? HTTP_BUF_SIZE : HTTPS_BUF_SIZE;

	memset(pG->csBuffer, 0, uBufferLen);

	/* get the modem version */
	char modem_ver[30];

	iRC = otdoa_nordic_at_get_modem_version(modem_ver, sizeof(modem_ver));
	if (iRC) {
		LOG_ERR("http_h1_format_request: failed to get modem version: %d", iRC);
	}

	/* Build an Auth request */
	char *cursor = pG->csBuffer;
	size_t req_len = 0;

	/* start with the parameters we know we always have */
	iRC = snprintf(cursor, uBufferLen - req_len,
		"GET /v1/ubsa.php"
		"?ecgi=%" PRIu32
		"&encrypt=%u"
		"&radius=%d"
		"&mcc=%" PRIu16
		"&mnc=%" PRIu16
		"&otdoa_fwv=%s"
		"&mfwv=%s"
		"&num_cells=%d"
		"&compress_window=%d",
		pG->req.uEcgi,
		bDisableEncryption ? 0 : 1,
		pG->req.uRadius, pG->req.uMCC, pG->req.uMNC,
		otdoa_api_get_short_version(),
		modem_ver, pG->req.uNumCells,
		LOG2_COMPRESS_WINDOW);

	if (iRC < 0 || iRC > uBufferLen - req_len) {
		LOG_ERR("uBSA request overflowed HTTPS buffer");
		iRC = 0; /* 0 indicates error */
		goto exit;
	}

	cursor += iRC;
	req_len += iRC;

	/* append the DLEARFCN if we have it */
	if (pG->req.uDlearfcn != UNKNOWN_UBSA_DLEARFCN) {
		iRC = snprintf(cursor, uBufferLen - req_len,
			"&dlearfcn=%" PRIu32,
			pG->req.uDlearfcn);

		if (iRC < 0 || iRC > uBufferLen - req_len) {
			LOG_ERR("uBSA request overflowed HTTPS buffer");
			iRC = 0; /* 0 indicates error */
			goto exit;
		}

		cursor += iRC;
		req_len += iRC;
	}
	/* temporary hack until optional dlearfcn is supported */
	else {
		iRC = snprintf(cursor, uBufferLen - req_len,
			"&dlearfcn=%" PRIu32,
			DEFAULT_UBSA_DLEARFCN);

		if (iRC < 0 || iRC > uBufferLen - req_len) {
			LOG_ERR("uBSA request overflowed HTTPS buffer");
			iRC = 0; /* 0 indicates error */
			goto exit;
		}

		cursor += iRC;
		req_len += iRC;
	}

	/* append the PCI if we have it */
	if (pG->req.uPCI != UNKNOWN_UBSA_PCI) {
		iRC = snprintf(cursor, uBufferLen - req_len,
			"&pci=%" PRIu16,
			pG->req.uPCI);

		if (iRC < 0 || iRC > uBufferLen - req_len) {
			LOG_ERR("uBSA request overflowed HTTPS buffer");
			iRC = 0; /* 0 indicates error */
			goto exit;
		}

		cursor += iRC;
		req_len += iRC;
	}

	/* append the request body */
	iRC = snprintf(cursor, uBufferLen - req_len,
		   " HTTP/1.1\r\n"
		   "Host: %s:443\r\n"
		   "User-agent: https_client/2.2.3\r\n"
		   "Accept: */*\r\n"
		   "Connection: keep-alive\r\n"
		   "authorization: Bearer %s\r\n"
		   "\r\n",
		   otdoa_http_get_download_url(), jwt_token);

	/* check that we didn't overflow the buffer */
	if (iRC < 0 || iRC > uBufferLen - req_len) {
		LOG_ERR("uBSA request overflowed HTTPS buffer");
		iRC = 0; /* 0 indicates error */
		goto exit;
	}

exit:

	if (pG) {
		pG->nOff = 0;
		pG->szSend = pG->csBuffer;
	}

	return iRC;
}

/**
 * Format the HTTPS H1 range request. The Range header requests a range of
 * bytes and can be called multiple times during a uBSA transfer.
 *
 * @param pG Pointer to gH1_HTTP containing request data
 * @return Number of bytes written to header
 */
int otdoa_http_h1_format_range_request(tOTDOA_HTTP_MEMBERS *pG)
{
	int iRC = 0;
	int iChunkSize = 0;
	char jwt_token[256];

	if (NULL == pG) {
		LOG_ERR("http_h1_format_request: NULL pG!");
		/* iRC is already 0 (error) */
		goto exit;
	}

	if (NULL == pG->csBuffer) {
		LOG_ERR("http_h1_format_request: NULL pG->csBuffer!");
		/* iRC is already 0 (error) */
		goto exit;
	}
	unsigned int uBufferLen = pG->bDisableTLS ? HTTP_BUF_SIZE : HTTPS_BUF_SIZE;

	memset(pG->csBuffer, 0, uBufferLen);

	iChunkSize = pG->bDisableTLS ? (HTTP_RANGE_REQUEST_SIZE - 128)
					 : (HTTPS_RANGE_REQUEST_SIZE - 128);
	if ((pG->nRange + iChunkSize) >= pG->nRangeMax) {
		iChunkSize = pG->nRangeMax - (pG->nRange + 1);
	}

	size_t token_len = 0;

	iRC = generate_jwt(jwt_token, sizeof(jwt_token), &token_len);
	if (iRC != 0) {
		LOG_ERR("http_h1_format_request: generate_jwt() error: %d", iRC);
		iRC = 0;
		goto exit;
	}

	/* Build a range request header */
	iRC = snprintf(pG->csBuffer, uBufferLen,
			   "GET /v1/ubsa.php?token=%s HTTP/1.1\r\n"
			   "Host: %s\r\n"
			   "User-agent: https_client/2.2.3\r\n"
			   "Accept: */*\r\n"
			   "Range: bytes=%d-%d\r\n"
			   "authorization: Bearer %s\r\n"
			   "\r\n",
			   pG->ubsa_token, otdoa_http_get_download_url(), pG->nRange,
			   pG->nRange + iChunkSize, jwt_token);

	LOG_DBG("Range Request Header: %s\r\n", pG->csBuffer);

	/* check that we didn't overflow the buffer */
	if (iRC >= uBufferLen) {
		LOG_ERR("uBSA request overflowed HTTPS buffer");
		/* check that we didn't overflow the buffer */
		iRC = 0;
		goto exit;
	}

exit:

	if (pG) {
		pG->nOff = 0;
		pG->szSend = pG->csBuffer;
	}

	return iRC;
}

/**
 * HTTP FSM top-level message handling
 *
 * @param pMsg Pointer to message to handle
 * @return 0 on success, otherwise negative error code
 */
int otdoa_http_h1_handle_message(tOTDOA_HTTP_MESSAGE *pMsg)
{
	int rc = 0;

	if (!pMsg) {
		LOG_ERR("http_entry_point: pMsg null");
		rc = -EINVAL;
		goto exit;
	}

	switch (pMsg->header.u32MsgId) {
	case OTDOA_HTTP_MSG_GET_H1_UBSA:
		rc = handle_get_ubsa_message(pMsg);
		break;
	case OTDOA_HTTP_MSG_GET_H1_CONFIG_FILE:
		rc = handle_get_config_message(pMsg);
		break;
#ifdef CONFIG_OTDOA_ENABLE_RESULTS_UPLOAD
	case OTDOA_HTTP_MSG_UPLOAD_OTDOA_RESULTS:
		rc = handle_upload_results_message(pMsg);
		break;
#endif
	case OTDOA_HTTP_MSG_TEST_JWT:
		rc = handle_test_jwt_message(pMsg);
		break;
	default:
		LOG_WRN("unexpected HTTP message");
		rc = -EINVAL;
		break;
	}

exit:
	return rc;
}

/**
 * Handle OTDOA_HTTP_MSG_GET_H1_UBSA message
 */
static int handle_get_ubsa_message(tOTDOA_HTTP_MESSAGE *pMsg)
{
	int rc = 0;

	LOG_INF("HTTP H1 received OTDOA_HTTP_MSG_GET_H1_UBSA");
	if (bSkipConfigDL) {
		LOG_WRN("Skipping config DL.");
	} else if (gHTTP.nBSARequests++ % CFG_DL_INTERVAL == 0) {
		/* Always get config file first
		 * needs to have TLS enabled for the config endpoint.
		 */
		bool save_tls = gHTTP.bDisableTLS;

		gHTTP.bDisableTLS = false;
		rc = otdoa_http_h1_rebind(NULL);
		if (rc != 0) {
			LOG_WRN("Failed to bind socket (for cfg dl). rc = %d", rc);
			otdoa_http_invoke_callback_dl_compl(OTDOA_EVENT_FAIL_NO_CELL);
			return rc;
		}
		rc = otdoa_http_h1_handle_get_cfg(&gHTTP);
		gHTTP.bDisableTLS = save_tls;
		if (rc != 0) {
			LOG_WRN("Failed to get config file. API Status Result = %d", rc);
			otdoa_http_invoke_callback_dl_compl(rc);
			return rc;
		}
	} else {
		LOG_INF("Skipping config DL.");
	}

	if (pMsg->http_get_ubsa.bResetBlacklist) {
		otdoa_http_h1_blacklist_init(&gHTTP);
	}

	rc = otdoa_http_h1_download_ubsa(&gHTTP, pMsg);

	/* send result back to the OTDOA API */
	LOG_INF("API Status Result = %d", rc);
	otdoa_http_invoke_callback_dl_compl(rc);

	return rc;
}

/**
 * Handle OTDOA_HTTP_MSG_GET_H1_CONFIG_FILE message
 */
static int handle_get_config_message(tOTDOA_HTTP_MESSAGE *pMsg)
{
	int rc = 0;

	LOG_INF("HTTP_H1 received OTDOA_HTTP_MSG_GET_H1_CONFIG");
	/* force to use TLS */
	bool save_tls = gHTTP.bDisableTLS;

	gHTTP.bDisableTLS = false;
	rc = otdoa_http_h1_rebind(NULL);
	if (rc != 0) {
		LOG_WRN("Failed to bind socket (for cfg dl). rc = %d", rc);
		otdoa_http_invoke_callback_dl_compl(OTDOA_EVENT_FAIL_NO_CELL);
		return rc;
	}
	rc = otdoa_http_h1_handle_get_cfg(&gHTTP);
	gHTTP.bDisableTLS = save_tls;
	LOG_INF("Config DL Result = %d", rc);
	otdoa_http_h1_free_cs_buffer();
	/* NB: We don't send a result back to the OTDOA API in this case */

	return rc;
}

#ifdef CONFIG_OTDOA_ENABLE_RESULTS_UPLOAD
/**
 * Handle OTDOA_HTTP_MSG_UPLOAD_OTDOA_RESULTS message
 */
static int handle_upload_results_message(tOTDOA_HTTP_MESSAGE *pMsg)
{
	int rc = 0;

	LOG_INF("HTTP_H1 received OTDOA_HTTP_MSG_UPLOAD_OTDOA_RESULTS");
	rc = otdoa_http_h1_rebind(pMsg->http_upload_results.pURL);
	if (rc != 0) {
		otdoa_http_invoke_callback_ul_compl(OTDOA_EVENT_FAIL_NO_CELL);
		return rc;
	}

	rc = otdoa_http_h1_handle_otdoa_results(&gHTTP, pMsg);

	const char *prs_id = strstr(gHTTP.csBuffer, "id = ");

	if (prs_id) {
		prs_id += strlen("if = ");
		gHTTP.prsID = strtol(prs_id, NULL, 10);
	}
	free(pMsg->http_upload_results.pResults);
	/* NB:  This print is used to detect completion of position estimate for CI tests */
	LOG_INF("OTDOA position estimate %s", (rc == 0 ? "SUCCESS" : "FAILURE"));
	otdoa_http_invoke_callback_ul_compl(rc == 0 ? OTDOA_API_SUCCESS
							: OTDOA_EVENT_FAIL_NO_CELL);

	return rc;
}
#endif

/**
 * Handle OTDOA_HTTP_MSG_TEST_JWT message
 */
static int handle_test_jwt_message(tOTDOA_HTTP_MESSAGE *pMsg)
{
	int rc = 0;

	if (otdoa_http_h1_rebind(NULL)) {
		LOG_ERR("Failed to bind to server socket");
		return -1;
	}
	LOG_INF("Starting JWT test...");
	rc = otdoa_http_h1_test_jwt(&gHTTP);

	return rc;
}

/**
 * Handle UBSA authentication process
 */
static int handle_ubsa_authentication(tOTDOA_HTTP_MEMBERS *p_http)
{
	int rc = 0;

	if (!p_http->bSkipAuth) {
		/* Send initial authentication request */
		rc = otdoa_http_h1_send_get_ubsa_auth_req(p_http);
		if (rc != OTDOA_API_SUCCESS) {
			LOG_ERR("Failed to send auth request: %d\n", rc);
			return rc;
		}

		/* Receive and process auth response */
		rc = otdoa_http_h1_receive_header(p_http);
		if (rc <= 0) {
			LOG_ERR("Failed to receive auth response: %d\n", rc);
			rc = (rc == 0) ? OTDOA_EVENT_FAIL_NTWK_CONN : -rc;
			return rc;
		}

		/* possibly override the auth. response for testing */
		if (nOverrideAuthResp != 0) {
			LOG_WRN("Overriding auth response to %d", nOverrideAuthResp);
			rc = nOverrideAuthResp;
			return rc;
		}

		rc = otdoa_http_h1_process_auth_response(p_http);
		if (rc != 0) {
			LOG_ERR("Failed to process auth response: %d\n", rc);
			return rc;
		}
	}

	return rc;
}

/**
 * Handle UBSA download loop process
 */
static int handle_ubsa_download_loop(tOTDOA_HTTP_MEMBERS *p_http)
{
	int rc = 0;

	/* Download data in ranges */
	while (p_http->nRange < p_http->nRangeMax) {
		/* Send range request */
		rc = otdoa_http_h1_send_get_ubsa_range_req(p_http);
		if (rc != 0) {
			LOG_ERR("Failed to send range request: %d\n", rc);
			return rc;
		}

		/* Receive range response header */
		rc = otdoa_http_h1_receive_header(p_http);
		if (rc <= 0) {
			LOG_ERR("Failed to receive range response: %d\n", rc);
			rc = (rc == 0) ? OTDOA_EVENT_FAIL_NTWK_CONN : -rc;
			return rc;
		}

		/* Process range response header */
		rc = otdoa_http_h1_process_range_response_header(p_http);
		if (rc != OTDOA_API_SUCCESS && rc != OTDOA_EVENT_HTTP_PARTIAL_CONTENT) {
			LOG_ERR("Failed to process range response header: %d\n", rc);
			return rc;
		}

		/* Process range response content */
		rc = otdoa_http_h1_process_range_response_content(p_http);
		if (rc < 0) {
			LOG_ERR("Failed to process range response content: %d\n", rc);
			return rc;
		}
	}

	LOG_INF("UBSA download complete\n");
	return 0;
}

/**
 * Receive data with retry logic for non-blocking sockets
 */
static int receive_data_with_retry(tOTDOA_HTTP_MEMBERS *pG, size_t nAvail, int *iCount)
{
	int iBytesRead = 0;
	int iRC = 0;

	iBytesRead = http_recv(pG->fdSocket, &pG->csBuffer[pG->nOff], nAvail, 0);
	if (0 == iBytesRead) {
		LOG_INF("otdoa_http_h1_receive_header: server shut down.");
		iRC = 0;
	} else if (iBytesRead < 0) {
		int err_no = http_errno();

		if (err_no == EWOULDBLOCK) {
			if ((*iCount)++ > NONBLOCK_RETRY_LIMIT) {
				LOG_ERR("Retry limit reached in "
						  "otdoa_http_h1_receive_header()");
				iRC = -OTDOA_API_INTERNAL_ERROR;
			} else if (otdoa_http_check_pending_stop()) {
				/* NB negative return code */
				iRC = -OTDOA_EVENT_FAIL_CANCELLED;
				LOG_WRN("Handling stop request in HTTP receive");
			} else {
				LOG_DBG("Sleeping %d msec.",
						  NONBLOCK_RETRY_MILLISECONDS);
				http_sleep(NONBLOCK_RETRY_MILLISECONDS);
				iRC = -1; /* Indicate retry needed */
			}
		} else {
			LOG_ERR("recv failed. errno: %s, buffsize: %d", strerror(err_no),
					  nAvail);
			iRC = -OTDOA_API_INTERNAL_ERROR;
		}
	} else {
		LOG_DBG("receive_header: iBytesRead: %d\r", iBytesRead);
		iRC = iBytesRead; /* for positive return code */
		pG->nOff += iBytesRead;
	}

	return iRC;
}

/**
 * Handle encryption parameters (pubkey and IV) from auth response
 */
static otdoa_api_error_codes_t handle_encryption_parameters(tOTDOA_HTTP_MEMBERS *pG,
	char *pValue, int iTokenCount)
{
	otdoa_api_error_codes_t iRC = OTDOA_API_SUCCESS;
	size_t len = 0;

	/* pubkey, iv only present if encryption is enabled */
	if (!bDisableEncryption) {
		pValue = otdoa_http_h1_find_value(pG->csBuffer, "pubkey: ", iTokenCount);
		if (!pValue) {
			LOG_ERR("pubkey header field not found.");
			iRC = OTDOA_API_INTERNAL_ERROR;
			return iRC;
		}

		len = strlen(pValue);
		LOG_DBG("pubkey length: %zu", len);
		LOG_DBG("pubkey: %s", pValue);

		/* pubkey is in ASCII DER format.  So string is 2 bytes per byte of the
		 * binary DER format key
		 */
		if (len == 2 * PUBKEY_DER_LEN) {
			/* Parse the public key starting at fixed offset */
			/* times 2 since offset is binary, but pValue is ASCII */
			pValue += 2 * PUBKEY_DER_OFFSET;
			if (0 != parse_keystr(pValue, pG->pubkey, PUBKEY_LMAX)) {
				LOG_ERR("Failed to parse pubkey: %s", pValue);
				iRC = OTDOA_API_INTERNAL_ERROR;
				return iRC;
			}
			LOG_HEXDUMP_DBG(pG->pubkey, PUBKEY_LMAX, "pubkey: ");
		} else {
			LOG_ERR("Bad pubkey len: %zu", len);
			iRC = OTDOA_API_INTERNAL_ERROR;
			return iRC;
		}

		pValue = otdoa_http_h1_find_value(pG->csBuffer, "iv: ", iTokenCount);
		if (!pValue) {
			LOG_ERR("iv header field not found.");
			iRC = OTDOA_API_INTERNAL_ERROR;
			return iRC;
		}

		len = strlen(pValue);
		LOG_DBG("iv length: %zu", len);
		LOG_DBG("iv: %s", pValue);
		if (len == 2 * IV_LMAX) {
			if (0 != parse_keystr(pValue, pG->iv, IV_LMAX)) {
				LOG_ERR("Failed to convert IV: %s", pValue);
				iRC = OTDOA_API_INTERNAL_ERROR;
				return iRC;
			}
			LOG_INF("Converted IV: %s", pValue);
		} else {
			LOG_ERR("Bad IV length: %zu  IV: %s", len, pValue);
			iRC = OTDOA_API_INTERNAL_ERROR;
			return iRC;
		}

		/* Set key for use in decrypting uBSA */
		if (0 != otdoa_crypto_set_new_key(pG->pubkey, PUBKEY_LMAX)) {
			LOG_ERR("Failed to update encryption key");
			iRC = OTDOA_API_INTERNAL_ERROR;
			return iRC;
		}
	}

	return iRC;
}

/**
 * Handle ubsa-token and recommended-delay from auth response
 */
static otdoa_api_error_codes_t handle_token_and_delay(tOTDOA_HTTP_MEMBERS *pG,
	char *pValue, int iTokenCount)
{
	otdoa_api_error_codes_t iRC = OTDOA_API_SUCCESS;
	size_t len = 0;

	pValue = otdoa_http_h1_find_value(pG->csBuffer, "ubsa-token: ", iTokenCount);
	if (!pValue) {
		LOG_ERR("ubsa-token header field not found.");
		iRC = OTDOA_API_INTERNAL_ERROR;
		return iRC;
	}

	len = strlen(pValue);
	LOG_DBG("ubsa-token length: %zu", strlen(pValue));
	LOG_DBG("ubsa-token: %s", pValue);
	if (len < UBSA_TOKEN_LMAX) {
		strncpy(pG->ubsa_token, pValue, len);
		pG->ubsa_token[len] = '\0'; /* ensure null term */
	} else {
		LOG_ERR("ubsa-token too long");
		iRC = OTDOA_API_INTERNAL_ERROR;
		return iRC;
	}

	/* set recommended delay to zero - if it doesn't get set below zero is good */
	pG->req.uRecommendedDelay = 0;
	pValue = otdoa_http_h1_find_value(pG->csBuffer, "recommended-delay: ", iTokenCount);
	if (pValue) {
		if (parse_string_to_int_unsigned(pValue, &pG->req.uRecommendedDelay) != 0) {
			LOG_ERR("Invalid recommended-delay value: %s", pValue);
			pG->req.uRecommendedDelay = 0; /* Set to default on error */
		}
	} else {
		LOG_ERR("recommended-delay header field not found.");
	}
	LOG_INF("recommended-delay: %s (%i)", pValue, pG->req.uRecommendedDelay);

	return iRC;
}

/**
 * Validate inputs for range response header processing
 */
static otdoa_api_error_codes_t validate_range_response_inputs(tOTDOA_HTTP_MEMBERS *pG,
	int *iHeaderLen, int *iTokenCount)
{
	if (!pG) {
		LOG_ERR("pG null!");
		return OTDOA_API_INTERNAL_ERROR;
	}

	if (!pG->csBuffer) {
		LOG_ERR("pG->csBuffer null!");
		return OTDOA_API_INTERNAL_ERROR;
	}

	*iHeaderLen = otdoa_http_h1_get_header_len(pG->csBuffer);
	if (!*iHeaderLen) {
		LOG_ERR("http header delimiter not found.");
		return OTDOA_API_INTERNAL_ERROR;
	}

	pG->csBuffer[*iHeaderLen + 2] = 0;
	pG->pData = &(pG->csBuffer[*iHeaderLen + 4]);

	*iTokenCount = otdoa_http_h1_split_into_tokens(pG->csBuffer);
	LOG_DBG("Found %d tokens", *iTokenCount);

	return OTDOA_API_SUCCESS;
}

/**
 * Handle HTTP response code processing
 */
static otdoa_api_error_codes_t handle_http_response_code(tOTDOA_HTTP_MEMBERS *pG,
	char *pResponse, int iTokenCount, bool *bPartialContent)
{
	otdoa_api_error_codes_t iRC = OTDOA_API_INTERNAL_ERROR;
	int iHttpResponseCode = 0;

	if (!pResponse) {
		return iRC;
	}

	LOG_INF("response code: %s", pResponse);
	iHttpResponseCode = otdoa_http_h1_parse_response_code(pResponse);

	switch (iHttpResponseCode) {
	case 200:
		LOG_DBG("200:OK Success - no data follows");
		iRC = OTDOA_API_SUCCESS;
		break;
	case 202:
		LOG_DBG("202:Accepted - token exists, ubsa processing not done");
		pG->req.uRecommendedDelay = 0;
		char *pValue = otdoa_http_h1_find_value(pG->csBuffer,
								"recommended-delay: ", iTokenCount);

		if (pValue) {
			if (parse_string_to_int_unsigned(pValue, &pG->req.uRecommendedDelay) != 0) {
				LOG_ERR("Invalid recommended-delay : %s", pValue);
				pG->req.uRecommendedDelay = 0;
			}
		} else {
			LOG_ERR("recommended-delay header field not found.");
		}
		LOG_DBG("recommended-delay: %s (%i)", pValue, pG->req.uRecommendedDelay);
		iRC = OTDOA_EVENT_HTTP_NOT_READY;
		break;
	case 206:
		*bPartialContent = true;
		iRC = OTDOA_EVENT_HTTP_PARTIAL_CONTENT;
		LOG_DBG("206:Partial Content - chunck supplied, more data follows");
		break;
	case 400:
		LOG_DBG("400:Bad Request");
		iRC = OTDOA_EVENT_FAIL_HTTP_BAD_REQUEST;
		break;
	case 401:
		LOG_DBG("401:Unauthorized");
		iRC = OTDOA_EVENT_FAIL_HTTP_UNAUTHORIZED;
		break;
	case 429:
		LOG_DBG("429:Too Many Requests");
		iRC = OTDOA_EVENT_FAIL_HTTP_TOO_MANY_REQUESTS;
		break;
	case 500:
		LOG_DBG("500:Internal Server Error");
		iRC = OTDOA_EVENT_FAIL_HTTP_INTERNAL_SERVER_ERROR;
		break;
	default:
		iRC = OTDOA_API_INTERNAL_ERROR;
		LOG_ERR("Unexpected response code: %d", iHttpResponseCode);
		break;
	}

	return iRC;
}

/**
 * Handle Content-Length header processing
 */
static otdoa_api_error_codes_t handle_content_length_header(tOTDOA_HTTP_MEMBERS *pG,
	char *pLengthField)
{
	if (pLengthField) {
		if (parse_string_to_int_signed(pLengthField, &pG->nContentLength) != 0) {
			LOG_ERR("Invalid Content-Length value: %s", pLengthField);
			return OTDOA_API_INTERNAL_ERROR;
		}
		LOG_INF("Content-Length: %s", pLengthField);
	}
	return OTDOA_API_SUCCESS;
}

/**
 * Handle partial content (206) response processing
 */
static otdoa_api_error_codes_t handle_partial_content_response(tOTDOA_HTTP_MEMBERS *pG,
	int iTokenCount)
{
	char *pRangeField = otdoa_http_h1_find_value(pG->csBuffer,
							"Content-Range: bytes", iTokenCount);

	if (!pRangeField) {
		LOG_ERR("Content-Range header field not found.");
		return OTDOA_API_INTERNAL_ERROR;
	}

	LOG_DBG("Content-Range: %s", pRangeField);

	int parsedTokens = sscanf(pRangeField, "%d-%d/%d", &pG->nRange,
						&pG->nRangeSegmentEnd, &pG->nRangeMax);

	if (3 != parsedTokens) {
		LOG_ERR("found %d tokens in: %s", parsedTokens, pRangeField);
		return OTDOA_API_INTERNAL_ERROR;
	}

	/* Handle special case of only one chunk for download */
	if (pG->nRangeSegmentEnd > pG->nRangeMax - 1) {
		pG->nRangeSegmentEnd = pG->nRangeMax - 1;
	}
	if (pG->nContentLength >= pG->nRangeMax) {
		pG->nContentLength = pG->nRangeMax;
		pG->bDownloadComplete = true;
	}

	return OTDOA_API_SUCCESS;
}

/**
 * Handle non-partial content (200) response processing
 */
static otdoa_api_error_codes_t handle_non_partial_content_response(tOTDOA_HTTP_MEMBERS *pG,
	int iLastSegmentEnd)
{
	if (pG->nRange == 0) {
		/* Receiving just one chunk for the entire uBSA */
		pG->nRangeSegmentEnd = pG->nContentLength - 1;
		pG->nRangeMax = pG->nContentLength;
		pG->bDownloadComplete = true;
	} else {
		/* Not partial content--use the content-length provided in the header
		 * for this content length, but we still need the start since we're
		 * not provided with a content-range on last chunck (?). We can get
		 * the start of last chunk by saving rangeSegmentEnd for each and
		 * using on last chunk.
		 */
		if (iLastSegmentEnd) {
			pG->nRange = iLastSegmentEnd + 1;
		}
		pG->nRangeSegmentEnd += pG->nContentLength;
		LOG_DBG("nRange after final chunk: %d", pG->nRange);
		LOG_DBG("range end after final chunk: %d", pG->nRangeSegmentEnd);
		LOG_DBG("content length after final chunk: %d", pG->nContentLength);
		LOG_DBG("range max after final chunk: %d", pG->nRangeMax);
		pG->bDownloadComplete = true;
	}

	return OTDOA_API_SUCCESS;
}

/**
 * HTTP Config DL initialize
 */
void otdoa_http_h1_init_config_dl(void)
{

	gHTTP.nRangeMax = HTTPS_RANGE_MAX_DEFAULT;
	gHTTP.bDownloadComplete = false;
	gHTTP.nRange = 0;

	LOG_DBG("csBuffer: %p", (void *)(gHTTP.csBuffer));
}

/**
 * Download UBSA data using HTTP/1.1 range requests
 *
 * @param params The UBSA parameters
 * @return int 0 on success, negative error code on failure
 */
int otdoa_http_h1_download_ubsa_internal(tOTDOA_HTTP_MEMBERS *p_http, tOTDOA_HTTP_MESSAGE *params)
{
	int rc;

	if (!params) {
		LOG_ERR("download_ubsa: params null\n");
		return OTDOA_API_ERROR_PARAM;
	}
	if (!p_http) {
		LOG_ERR("download_ubsa: p_http null\n");
		return OTDOA_API_ERROR_PARAM;
	}

	/* Initialize HTTP context */
	gHTTP.bDownloadComplete = false;
	p_http->nRangeMax = HTTPS_RANGE_MAX_DEFAULT;

	if (0 != otdoa_http_h1_get_cs_buffer()) {
		LOG_ERR("Failed to allocate buffer\n");
		return OTDOA_API_INTERNAL_ERROR;
	}

	/* Copy UBSA parameters */
	otdoa_http_h1_copy_ubsa_params(p_http, params);

	/* Connect to server */
	rc = otdoa_http_h1_send_connect(p_http, otdoa_http_get_download_url());
	if (rc != OTDOA_API_SUCCESS) {
		LOG_ERR("Failed to connect to server: %d\n", rc);
		rc = OTDOA_EVENT_FAIL_NTWK_CONN;
		goto cleanup;
	}
	LOG_INF("Successfully connected to server\n");

	rc = handle_ubsa_authentication(p_http);
	if (rc != 0) {
		goto disconnect;
	}

	rc = handle_ubsa_download_loop(p_http);
	if (rc != 0) {
		goto disconnect;
	}

disconnect:
	otdoa_http_h1_send_disconnect(p_http);

cleanup:
	otdoa_http_h1_free_cs_buffer();

	/* never let this persist */
	p_http->bSkipAuth = false;

	/* cleanup on error */
	if (rc != 0) {
		otdoa_crypto_abort();
		otdoa_ubsa_proc_close_file();
		otdoa_ubsa_proc_remove_file(OTDOA_pxlGetBSAPath());
	}
	return rc;
}

/**
 * Download UBSA Data using HTTP/1.1 range requests
 * wrapper around otdoa_http_h1_download_ubsa_internal() to retry downloading
 * the UBSA if we have a recoverable failure
 *
 * @param params The UBSA parameters
 * @return int 0 on success, negative error code on failure
 */
int otdoa_http_h1_download_ubsa(tOTDOA_HTTP_MEMBERS *p_http, tOTDOA_HTTP_MESSAGE *params)
{
	if (!params) {
		LOG_ERR("download_ubsa: params null\n");
		return OTDOA_API_ERROR_PARAM;
	}
	if (!p_http) {
		LOG_ERR("download_ubsa: p_http null\n");
		return OTDOA_API_ERROR_PARAM;
	}

	/* check if we have a blacklisted ecgi */
	const int blacklist = otdoa_http_h1_blacklist_check(p_http, params->http_get_ubsa.uEcgi);

	(void)otdoa_http_h1_blacklist_tick(p_http);
	if (blacklist > 0) {
		LOG_WRN("download_ubsa: ecgi %u is blacklisted.  age = %d",
				  params->http_get_ubsa.uEcgi, blacklist);
		return OTDOA_EVENT_FAIL_BLACKLISTED;
	}
	if (blacklist < 0) {
		LOG_ERR("download_ubsa: failure checking ecgi blacklist: %d", blacklist);
	} else {
		LOG_DBG("download_ubsa: ecgi %u is not blacklisted.",
				  params->http_get_ubsa.uEcgi);
	}

	int rebind = otdoa_http_h1_rebind(NULL);

	if (rebind != 0) {
		LOG_WRN("Failed to bind socket (for uBSA dl). rc = %d", rebind);
		return OTDOA_EVENT_FAIL_NTWK_CONN;
	}

	do {
		const otdoa_api_error_codes_t rc =
			otdoa_http_h1_download_ubsa_internal(p_http, params);
		LOG_ERR("Auth resp = %d", rc);
		switch (rc) {
		case OTDOA_API_SUCCESS:
		case OTDOA_EVENT_HTTP_PARTIAL_CONTENT:
			/* download success, do nothing else */
			return rc;
		case OTDOA_EVENT_FAIL_HTTP_TOO_MANY_REQUESTS:
		case OTDOA_EVENT_FAIL_HTTP_CONFLICT:
			/* recoverable error, try again */
			continue;
		case OTDOA_EVENT_HTTP_NOT_READY:
			/* delay the recommended amount, then try again */
			otdoa_sleep_msec((int)p_http->req.uRecommendedDelay);
			p_http->bSkipAuth = true;
			continue;
		/*  hack: blacklist on 400 until phywi updates */
		case OTDOA_EVENT_FAIL_HTTP_BAD_REQUEST:
		case OTDOA_EVENT_FAIL_HTTP_UNPROCESSABLE_CONTENT:
			/* add the requested ecgi to the blacklist */
			LOG_ERR("download_ubsa: adding ecgi %u to blacklist\n",
					  params->http_get_ubsa.uEcgi);
			if (otdoa_http_h1_blacklist_add(p_http, params->http_get_ubsa.uEcgi) < 0) {
				LOG_ERR("download_ubsa: failed to add ecgi %u to blacklist",
						  params->http_get_ubsa.uEcgi);
			}
			return rc;
		default:
			/* pass the error code along */
			return rc;
		}
	} while (true);
}

/**
 * Format and send the HTTPS H1 auth request. This does a lot and I'd like
 * to separate some of this into more discrete functions
 *
 * @param pG Pointer to gH1_HTTP containing request data
 * @return 0 on success, otherwise error code
 */
int otdoa_http_h1_send_get_ubsa_auth_req(tOTDOA_HTTP_MEMBERS *pG)
{
	int iRC = OTDOA_API_SUCCESS;

	if (!pG) {
		LOG_ERR("otdoa_http_h1_send_get_ubsa_auth_req: pG is null!");
		iRC = OTDOA_API_ERROR_PARAM;
		goto exit;
	}

	if (!gHTTP.csBuffer) {
		LOG_ERR("otdoa_http_h1_send_get_ubsa_auth_req: failed to alloc send/receive "
				  "buffer");
		iRC = OTDOA_API_INTERNAL_ERROR;
		goto exit;
	}

	iRC = otdoa_http_h1_format_auth_request(pG);
	if (iRC <= 0) { /* greater than zero is number of bytes written to header */
		LOG_ERR("http_format_request failed: %d", iRC);
		iRC = OTDOA_API_INTERNAL_ERROR;
		goto exit;
	}

	LOG_DBG("ubsa_auth_req: %s", pG->csBuffer);
	iRC = http_send_request(pG, strlen(pG->szSend));

	if (iRC) {
		LOG_ERR("http_send_request failed: %d", iRC);
		return OTDOA_EVENT_FAIL_NTWK_CONN;
	}

exit:
	return iRC;
}

int otdoa_http_h1_send_get_ubsa_range_req(tOTDOA_HTTP_MEMBERS *pG)
{
	int iRC = 0;

	if (!pG) {
		LOG_ERR("otdoa_http_h1_send_get_ubsa_range_req: pG is null!");
		iRC = -EINVAL;
		goto exit;
	}

	if (!pG->csBuffer) {
		LOG_ERR("otdoa_http_h1_send_get_ubsa_range_req: failed to alloc send/receive "
				  "buffer");
		iRC = -ENOMEM;
		goto exit;
	}

	iRC = otdoa_http_h1_format_range_request(pG);
	if (iRC <= 0) { /* greater than zero is number of bytes written to header */
		LOG_ERR("otdoa_http_h1_send_get_ubsa_range_req: format range failed %d", iRC);
		goto exit;
	}

	if (pG->req.uRecommendedDelay > 0) {
		LOG_DBG("delaying %d", pG->req.uRecommendedDelay);
		http_sleep((int)pG->req.uRecommendedDelay);
	}

	/* Reset recommended delay--it will be set again if needed with a new 202 response. */
	pG->req.uRecommendedDelay = 0;

	LOG_DBG("ubsa_range_req: %s\r", pG->csBuffer);
	iRC = http_send_request(pG, strlen(pG->szSend));

	if (iRC) {
		LOG_DBG("otdoa_http_h1_format_range_request :http_send_request failed: %d",
				  iRC);
		goto exit;
	}

exit:

	return iRC;
}

/**
 * Split the buffer into null-terminated strings (tokens).  The buffer gets
 * modified--the 0x0d 0x0a line terminator is replaced by 0x00 0x0a for each
 * token found.
 *
 * @param pBuffer Pointer to buffer to search through.
 * @return Number of tokens found.
 */
int otdoa_http_h1_split_into_tokens(char *pBuffer)
{
	int iTokenCount = 0;
	char *pToken;
	char *pSave = NULL;

	if (!pBuffer) {
		LOG_ERR("otdoa_http_h1_split_into_tokens: pBuffer is null!");
		/* pToken already null */
		goto exit;
	}

	pToken = strtok_r(pBuffer, HTTP_LINE_TERMINATOR, &pSave);
	while (pToken) {
		iTokenCount++;
		pToken = strtok_r(NULL, HTTP_LINE_TERMINATOR, &pSave);
	}

exit:

	return iTokenCount;
}

char *otdoa_http_h1_find_value(char *pBuffer, const char *pKey, int iTokenMax)
{
	char *pToken = NULL;
	char *pValue = NULL;
	int iTokenCount = 0;

	if (!pBuffer) {
		LOG_ERR("otdoa_http_h1_find_value: pBuffer is null!");
		/* pToken already null */
		goto exit;
	}

	if (!pKey) {
		LOG_ERR("otdoa_http_h1_find_value: pKey is null!");
		/* pToken already null */
		goto exit;
	}

	pToken = pBuffer;

	LOG_DBG("searching %s for key: %s", pToken, pKey);

	pValue = strstr(pToken, pKey);
	while (iTokenCount < iTokenMax) {
		if (pValue) {
			/**
			 * found the key -- now need the value
			 * increment the pointer by length of the key
			 */
			pValue += strlen(pKey);
			break;
		}
		pToken += strlen(pToken) + 2; /* + 2  to get past 0x00 and 0x0a */
		pValue = strstr(pToken, pKey);
		iTokenCount++;
	}

exit:

	return pValue;
}

/**
 * Receive the HTTP Response Header -- it's possible more than the header gets read, but
 * at least the header will get read, even if this is a response with just a header and
 * no body.  If there is a body it will be indicated by the content-length field, which
 * gets parsed when at least the header had been received.
 *
 * Fill gH1_HTTP.csBuffer
 *
 * @param pG Pointer to gH1_HTTP containing response data
 * @return positive on success, 0 if server closed connection, else negatives of API error codes
 */
int otdoa_http_h1_receive_header(tOTDOA_HTTP_MEMBERS *pG)
{
	int iRC = 0;
	int iSB = 0;
	int iCount = 0;

	if (!pG) {
		LOG_ERR("pG null!");
		iRC = -OTDOA_API_ERROR_PARAM;
		goto exit;
	}

	if (!pG->csBuffer) {
		LOG_ERR("pG->csBuffer null!");
		iRC = -OTDOA_API_ERROR_PARAM;
		goto exit;
	}

	iSB = SetSocketBlocking(pG->fdSocket, false);
	if (!iSB) {
		LOG_ERR("otdoa_http_h1_receive_header: SetSocketBlocking(false) error: %d",
				  iSB);
	}
	pG->nOff = 0;

	unsigned int uBufferLen = pG->bDisableTLS ? HTTP_BUF_SIZE : HTTPS_BUF_SIZE;

	memset(pG->csBuffer, 0, uBufferLen);

	do {
		size_t nAvail = (uBufferLen - 1) - pG->nOff;

		if (nAvail < 2) {
			LOG_ERR("otdoa_http_h1_receive_header: recv() DEBUG nAvail=%u",
					  nAvail);
			iRC = -OTDOA_API_INTERNAL_ERROR;
			break;
		}
		iRC = receive_data_with_retry(pG, nAvail, &iCount);
		if (iRC == 0) {
			/* Server shut down */
			break;
		}
		if (iRC < 0) {
			if (iRC == -OTDOA_EVENT_FAIL_CANCELLED) {
				goto exit;
			} else if (iRC == -1) {
				/* Retry needed */
				continue;
			} else {
				/* Other error */
				goto exit;
			}
		}

		/* received data */
		pG->nHeaderLength = otdoa_http_h1_get_header_len(pG->csBuffer);

		/* if HeaderLength is not null, we got a header so we can get out now and
		 * finish getting content once we've parse the header
		 */
		if (pG->nHeaderLength) {
			break;
		}
	} while (pG->nOff <
		 (uBufferLen - 1)); /* this can become infinite loop -- need to account */
					/* for zero bytes received too many times*/

	if (pG->nOff >= (uBufferLen)) {
		LOG_ERR("download range %zu exceeds buffer size %d", pG->nOff,
				  uBufferLen - 1);
		iRC = -OTDOA_API_INTERNAL_ERROR;
	}

exit:

	return iRC;
}

/**
 * Get the length of the header (used for receiving body parts)
 *
 * @param pBuffer Pointer to buffer containing response
 * @return positive (length of header) on success, 0 otherwise
 */
int otdoa_http_h1_get_header_len(const char *pBuffer)
{
	int iHeaderLen = 0;
	char *pHeaderEnd = 0;

	if (NULL == pBuffer) {
		LOG_ERR("otdoa_http_h1_get_header_len: pBuffer NULL!\r");
		goto exit;
	}

	pHeaderEnd = strstr(pBuffer, "\r\n\r\n");
	if (NULL != pHeaderEnd) {
		iHeaderLen = pHeaderEnd - pBuffer;
	} else {
		/* double cr-lf not found; log an error */
		LOG_ERR("double cr-lf not found; invalid header");
	}

exit:
	return iHeaderLen;
}

/**
 * Receive the HTTP Response body if it hasn't already been received.
 *
 * Fill gH1_HTTP.csBuffer
 *
 * @param pG Pointer to gH1_HTTP containing response data
 * @param iContentLen length of body (content-length)
 * @return positive on success, 0 if server closed connection, else error code
 */
int otdoa_http_h1_receive_content(tOTDOA_HTTP_MEMBERS *pG, int iContentLen)
{
	int iBytesRead = 0;
	int iRC = 0;
	int nCount = 0;

	if (!pG) {
		LOG_ERR("pG null!");
		iRC = -EINVAL;
		goto exit;
	}

	if (!pG->csBuffer) {
		LOG_ERR("pG->csBuffer null!");
		iRC = -EINVAL;
		goto exit;
	}
	unsigned int uBufferLen = pG->bDisableTLS ? HTTP_BUF_SIZE : HTTPS_BUF_SIZE;

	do {
		size_t nAvail = (uBufferLen - 1) - pG->nOff;

		if (nAvail < 2) {
			LOG_ERR("otdoa_http_h1_receive_content: recv() DEBUG nAvail=%u",
					  nAvail);
			iRC = -ENOMEM;
			break;
		}
		iBytesRead = http_recv(pG->fdSocket, &pG->csBuffer[pG->nOff], nAvail, 0);
		if (0 == iBytesRead) {
			LOG_INF("otdoa_http_h1_receive_content: server shut down.");
			iRC = 0;
			break;
		} else if (iBytesRead < 0) {
			int err_no = http_errno();

			if (err_no == EWOULDBLOCK) {
				if (nCount++ > NONBLOCK_RETRY_LIMIT) {
					LOG_ERR("Retry limit reached in "
							  "otdoa_http_h1_receive_content()");
					iRC = -1;
					break;
				}
				if (otdoa_http_check_pending_stop()) {
					iRC = OTDOA_EVENT_FAIL_CANCELLED; /* NB negative return code
									   */
					LOG_WRN("Handling stop request in "
							  "otdoa_http_h1_receive_content()");
					goto exit;
				} else {
					http_sleep(NONBLOCK_RETRY_MILLISECONDS);
					continue;
				}
			}
			LOG_ERR("otdoa_http_h1_receive_content: EXIT recv failed: %s",
					  strerror(err_no));
			iRC = -1;
			break;
		}

		LOG_INF("receive_content: received %d bytes.", iBytesRead);
		iRC = iBytesRead; /* for positive return code */
		pG->nOff += iBytesRead;

		/* pG->nOff is being used as a length here */
	} while ((pG->nOff - pG->nHeaderLength) < iContentLen);

	if (pG->nOff >= (uBufferLen)) {
		LOG_ERR("download range %zu exceeds buffer size %d", pG->nOff,
				  uBufferLen - 1);
		iRC = -1;
	}

exit:

	return iRC;
}

/**
 * Parse the HTTP response code
 *
 * @param pG Pointer to gH1_HTTP containing response data
 * @return positive number (http return code int form) on success, else error code
 */
int otdoa_http_h1_parse_response_code(const char *pResponse)
{
	int iRC = 0;

	if (!pResponse) {
		LOG_ERR("pResponse null!");
		iRC = -EINVAL;
		goto exit;
	}

	if (strstr(pResponse, "200")) {
		iRC = 200;
	} else if (strstr(pResponse, "202")) {
		iRC = 202;
	} else if (strstr(pResponse, "206")) {
		iRC = 206;
	} else if (strstr(pResponse, "400")) {
		iRC = 400;
	} else if (strstr(pResponse, "401")) {
		iRC = 401;
	} else if (strstr(pResponse, "404")) {
		iRC = 404;
	} else if (strstr(pResponse, "409")) {
		iRC = 409;
	} else if (strstr(pResponse, "410")) {
		iRC = 410;
	} else if (strstr(pResponse, "422")) {
		iRC = 422;
	} else if (strstr(pResponse, "429")) {
		iRC = 429;
	} else if (strstr(pResponse, "500")) {
		iRC = 500;
	} else {
		LOG_ERR("response code not recognized");
		iRC = -EINVAL;
	}

exit:

	return iRC;
}

/**
 * Process the config response header
 *
 * @param pG Pointer to gH1_HTTP containing response header
 * @return type tOTDOA_HTTP_H1_RESPONSE_STATUS:
 *       HTTP_H1_OK;
 *       HTTP_H1_BAD_REQUEST;
 *       HTTP_H1_UNAUTHORIZED;
 *       HTTP_H1_TOO_MANY_REQUESTS;
 *       HTTP_H1_INTERNAL_SERVER_ERROR;
 *       HTTP_H1_ERROR;
 * @note these process_auth/config/range_response_header routines return enums that correspond
 * to the HTTP codes returned from the server.  This is because some action might be conditional
 * depending upon that HTTP code (retries, etc.).  So, that's why.
 */
otdoa_api_error_codes_t otdoa_http_h1_process_config_response_header(tOTDOA_HTTP_MEMBERS *pG)
{
	int iRC = OTDOA_API_SUCCESS;
	int iHeaderLen = 0;
	char *pResponse = 0;
	char *pLengthField = 0;
	int iTokenCount = 0;

	if (!pG) {
		LOG_ERR("pG null!");
		iRC = OTDOA_API_INTERNAL_ERROR;
		goto exit;
	}

	if (!pG->csBuffer) {
		LOG_ERR("pG->csBuffer null!");
		iRC = OTDOA_API_INTERNAL_ERROR;
		goto exit;
	}

	iHeaderLen = otdoa_http_h1_get_header_len(pG->csBuffer);
	if (!iHeaderLen) {
		LOG_ERR("http header delimiter not found.");
		iRC = OTDOA_API_INTERNAL_ERROR;
		goto exit;
	}

	pG->csBuffer[iHeaderLen + 2] = 0;
	pG->pData = &(pG->csBuffer[iHeaderLen + 4]);

	iTokenCount = otdoa_http_h1_split_into_tokens(pG->csBuffer);
	LOG_INF("Found %d tokens", iTokenCount);

	pResponse = otdoa_http_h1_find_value(pG->csBuffer, "HTTP/1.1 ", iTokenCount);
	if (pResponse) {
		int iHttpResponseCode = 0;

		LOG_INF("response code: %s", pResponse);
		iHttpResponseCode = otdoa_http_h1_parse_response_code(pResponse);
		switch (iHttpResponseCode) {
		case 200:
			LOG_DBG("200:OK Success - config data follows");
			iRC = OTDOA_API_SUCCESS;
			break;
		case 400:
			LOG_DBG("400:Bad Request");
			iRC = OTDOA_EVENT_FAIL_HTTP_BAD_REQUEST;
			break;
		case 401:
			LOG_DBG("401:Unauthorized");
			iRC = OTDOA_EVENT_FAIL_HTTP_UNAUTHORIZED;
			break;
		case 429:
			LOG_DBG("429:Too Many Requests");
			iRC = OTDOA_EVENT_FAIL_HTTP_TOO_MANY_REQUESTS;
			break;
		case 500:
			LOG_DBG("500:Internal Server Error");
			iRC = OTDOA_EVENT_FAIL_HTTP_INTERNAL_SERVER_ERROR;
			break;
		default:
			iRC = OTDOA_API_INTERNAL_ERROR;
			LOG_ERR("Unexpected response code: %d", iHttpResponseCode);
			break;
		}
	}

	pLengthField = otdoa_http_h1_find_value(pG->csBuffer, "Content-Length: ", iTokenCount);
	if (pLengthField) {
		if (parse_string_to_int_signed(pLengthField, &pG->nContentLength) != 0) {
			LOG_ERR("Invalid Content-Length value: %s", pLengthField);
			iRC = OTDOA_API_INTERNAL_ERROR;
			goto exit;
		}
		LOG_INF("Content-Length: %s", pLengthField);
	}
	/* Handle server error of not sending content length with good response */
	else if (OTDOA_API_SUCCESS == iRC) {
		LOG_ERR("Content-Length not found");
		pG->nContentLength = 0;
		iRC = OTDOA_EVENT_FAIL_BAD_CFG;
	} else {
		/* will return an error */
	}
exit:

	return iRC;
}

/**
 * Read the auth response and save the header values
 *
 * @param pG Pointer to gH1_HTTP containing response data
 * @return:  type otdoa_api_error_codes_t
 */
otdoa_api_error_codes_t otdoa_http_h1_process_auth_response(tOTDOA_HTTP_MEMBERS *pG)
{
	int iRC = 0;
	int iHeaderLen = 0;
	char *pValue = 0;
	int iTokenCount = 0;
	int iHttpResponseCode = 0;

	if (!pG) {
		LOG_ERR("pG null!");
		iRC = OTDOA_API_ERROR_PARAM;
		goto exit;
	}

	if (!pG->csBuffer) {
		LOG_ERR("pG->csBuffer null!");
		iRC = OTDOA_API_ERROR_PARAM;
		goto exit;
	}

	iHeaderLen = otdoa_http_h1_get_header_len(pG->csBuffer);
	if (!iHeaderLen) {
		LOG_ERR(
			"otdoa_http_h1_process_auth_response: http header delimiter not found.");
		iRC = OTDOA_API_INTERNAL_ERROR;
		goto exit;
	}

	pG->csBuffer[iHeaderLen + 2] = 0; /* I guess +2 to leave one "\r\n" at the end */
	iTokenCount = otdoa_http_h1_split_into_tokens(pG->csBuffer);
	LOG_DBG("Found %d tokens", iTokenCount);

	pValue = otdoa_http_h1_find_value(pG->csBuffer, "HTTP/1.1 ", iTokenCount);
	if (!pValue) {
		LOG_ERR("response code field not found.");
		iRC = OTDOA_API_INTERNAL_ERROR;
		goto exit;
	}

	LOG_INF("response code: %s", pValue);
	iHttpResponseCode = otdoa_http_h1_parse_response_code(pValue);

	switch (iHttpResponseCode) {
	case 200:
		LOG_DBG("200:OK Success - ubsa-token supplied");
		iRC = OTDOA_API_SUCCESS;
		break;
	case 400:
		LOG_DBG("400:Bad Request - Request parameters malformed / invalid (Includes "
				  "JWT format) - UE SHOULD NOT RETRY");
		iRC = OTDOA_EVENT_FAIL_HTTP_BAD_REQUEST;
		break;
	case 401:
		LOG_DBG("401:Unauthorized - unable to validate JWT - UE SHOULD NOT RETRY");
		iRC = OTDOA_EVENT_FAIL_HTTP_UNAUTHORIZED;
		break;
	case 404:
		LOG_DBG("404:Not Found - document at url not found - UE SHOULD NOT RETRY");
		iRC = OTDOA_EVENT_FAIL_HTTP_NOT_FOUND;
		break;
	case 409:
		LOG_DBG("409:Conflict - User already has pending uBSA - UE SHOULD RETRY");
		iRC = OTDOA_EVENT_FAIL_HTTP_CONFLICT;
		break;
	case 422:
		LOG_DBG("422:Unprocessable Content - uBSA generation not possible. - UE "
				  "SHOULD ????");
		iRC = OTDOA_EVENT_FAIL_HTTP_UNPROCESSABLE_CONTENT;
		break;
	case 429:
		LOG_DBG(
			"429:Too Many Requests (RFC 6585) - API limit reached - UE SHOULD RETRY");
		iRC = OTDOA_EVENT_FAIL_HTTP_TOO_MANY_REQUESTS;
		break;
	case 500:
		LOG_DBG("500:Server Error - Server error when generating uBSA - UE SHOULD "
				  "NOT RETRY");
		iRC = OTDOA_EVENT_FAIL_HTTP_INTERNAL_SERVER_ERROR;
		break;
	default:
		LOG_ERR("Unexpected HTTP status: %d", iHttpResponseCode);
		iRC = OTDOA_API_INTERNAL_ERROR;
		break;
	}

	if (OTDOA_API_SUCCESS == iRC) {
		/* Handle encryption parameters if encryption is enabled */
		iRC = handle_encryption_parameters(pG, pValue, iTokenCount);
		if (iRC != OTDOA_API_SUCCESS) {
			goto exit;
		}

		/* Handle ubsa-token and recommended-delay */
		iRC = handle_token_and_delay(pG, pValue, iTokenCount);
		if (iRC != OTDOA_API_SUCCESS) {
			goto exit;
		}
	} else {
		/* LOG the body if available */
		/* point to data following the '\r\n\r\n' indicating
		 * the end of the header (note null char insertion above)
		 */
		iHeaderLen += 4;
		pG->csBuffer[pG->nOff] = '\0'; /* ensure null termination */
		if (iHeaderLen < pG->nOff) {
			/* if we have more data, display body */
			LOG_ERR("Resp. Body: %s", pG->csBuffer + iHeaderLen);
		}
	}
exit:

	return iRC;
}

/**
 * Process the range response header
 *
 * @param pG Pointer to gH1_HTTP containing response data
 * @return type tOTDOA_HTTP_H1_RESPONSE_STATUS:
 *       HTTP_H1_OK;
 *       HTTP_H1_NOT_READY;
 *       HTTP_H1_PARTIAL_CONTENT;
 *       HTTP_H1_BAD_REQUEST;
 *       HTTP_H1_UNAUTHORIZED;
 *       HTTP_H1_GONE;
 *       HTTP_H1_TOO_MANY_REQUESTS;
 *       HTTP_H1_INTERNAL_SERVER_ERROR;
 *       HTTP_H1_ERROR;
 */
otdoa_api_error_codes_t otdoa_http_h1_process_range_response_header(tOTDOA_HTTP_MEMBERS *pG)
{
	otdoa_api_error_codes_t iRC = 0;
	int iHeaderLen = 0;
	char *pResponse = 0;
	char *pLengthField = 0;
	int iTokenCount = 0;
	bool bPartialContent = false;
	int iLastSegmentEnd = 0;

	/* Validate inputs and setup */
	iRC = validate_range_response_inputs(pG, &iHeaderLen, &iTokenCount);
	if (iRC != OTDOA_API_SUCCESS) {
		goto exit;
	}

	/* Process HTTP response code */
	pResponse = otdoa_http_h1_find_value(pG->csBuffer, "HTTP/1.1 ", iTokenCount);
	iRC = handle_http_response_code(pG, pResponse, iTokenCount, &bPartialContent);
	if (!pResponse || (iRC != OTDOA_EVENT_HTTP_PARTIAL_CONTENT && iRC != OTDOA_API_SUCCESS)) {
		LOG_WRN("Bad HTTP Response %d", iRC);
		goto exit;
	}

	/* Process Content-Length header */
	pLengthField = otdoa_http_h1_find_value(pG->csBuffer, "Content-Length: ", iTokenCount);
	iRC = handle_content_length_header(pG, pLengthField);
	if (iRC != OTDOA_API_SUCCESS) {
		goto exit;
	}

	/* Process content based on whether it's partial or not */
	iLastSegmentEnd = pG->nRangeSegmentEnd;
	if (bPartialContent) {
		iRC = handle_partial_content_response(pG, iTokenCount);
	} else {
		iRC = handle_non_partial_content_response(pG, iLastSegmentEnd);
	}
	if (iRC != OTDOA_API_SUCCESS) {
		goto exit;
	}

	LOG_INF("range start: %d range end: %d content length: %d range max: %d", pG->nRange,
			  pG->nRangeSegmentEnd, pG->nContentLength, pG->nRangeMax);

exit:
	return iRC;
}

/**
 * Process the config response content
 *
 * @param pG Pointer to gH1_HTTP containing response data
 * @return 0 or greater (number of bytes processed) on success, else
 * negative error code
 */
int otdoa_http_h1_process_config_response_content(tOTDOA_HTTP_MEMBERS *pG)
{
	int iRC = 0;
	size_t iRemainingData = 0;

	if (!pG) {
		LOG_ERR("pG null!");
		iRC = -EINVAL;
		goto exit;
	}

	if (!pG->csBuffer) {
		LOG_ERR("pG->csBuffer null!");
		iRC = -EINVAL;
		goto exit;
	}

	iRemainingData = pG->nContentLength - (pG->nOff - pG->nHeaderLength - 4);

	/* if there is more data call receive_content */
	if (iRemainingData > 0) {
		iRC = otdoa_http_h1_receive_content(pG, pG->nContentLength);
	}

	/* need to clear nOff in process data!! */
	if (iRC >= 0) {
		iRC = otdoa_http_h1_process_config_data(pG);
	}

exit:

	return iRC;
}

/**
 * Process the range response content
 *
 * @param pG Pointer to gH1_HTTP containing response data
 * @return 0 or greater (number of bytes processed) on success, else
 * negative error code
 */
int otdoa_http_h1_process_range_response_content(tOTDOA_HTTP_MEMBERS *pG)
{
	int iRC = 0;
	size_t iRemainingData = 0;

	if (!pG) {
		LOG_ERR("pG null!");
		iRC = -EINVAL;
		goto exit;
	}

	if (!pG->csBuffer) {
		LOG_ERR("pG->csBuffer null!");
		iRC = -EINVAL;
		goto exit;
	}

	iRemainingData = pG->nContentLength - (pG->nOff - pG->nHeaderLength - 4);

	/* if there is more data call receive_content */
	if (iRemainingData) {
		iRC = otdoa_http_h1_receive_content(pG, pG->nContentLength);
	}

	/* need to clear nOff in process data!! */
	if (iRC >= 0) {
		iRC = otdoa_http_h1_process_ubsa_data(pG);
	}

exit:

	return iRC;
}

int otdoa_http_h1_process_config_data(tOTDOA_HTTP_MEMBERS *pG)
{
	int iRC = 0;

	if (!pG) {
		LOG_ERR("pG null!");
		iRC = -EINVAL;
		goto exit;
	}

	if (!pG->csBuffer) {
		LOG_ERR("pG->csBuffer null!");
		iRC = -EINVAL;
		goto exit;
	}

#ifndef HOST
	const char *pPath = otdoa_api_cfg_get_file_path();

	iRC = http_write_to_file(pPath, pG->pData, pG->nContentLength);

	if (iRC > 0) {
		pG->nRange += iRC;
	} else {
		LOG_DBG("http_write_to_file returned %d", iRC);
		iRC = -1;
	}
#endif

exit:

	return iRC;
}

/**
 * Process ubsa data -- http_h1 entry to Ubsa_process_data
 *
 * @param pG Pointer to gH1_HTTP containing response data
 * @return 0 on success, else error code
 */
int otdoa_http_h1_process_ubsa_data(tOTDOA_HTTP_MEMBERS *pG)
{
	int iRC = 0;

	if (!pG) {
		LOG_ERR("pG null!");
		iRC = -EINVAL;
		goto exit;
	}

	if (!pG->csBuffer) {
		LOG_ERR("pG->csBuffer null!");
		iRC = -EINVAL;
		goto exit;
	}

	/* write pG->pData here; this is where http.c does "bytes = write..." and returns bytes
	 * which gets added to nRange, but only if write is successful.
	 */
	if (0 == pG->nRange) {

		/* enable encryption locally if it is disabled on server and we want to store it
		 * encrypted
		 */
		int enable_ue_encr = bDisableEncryption && encrypt_ubsa;

		/* Pass in the IV from server if data was encrypted by the server*/
		uint8_t *pu8_iv = (bDisableEncryption ? NULL : pG->iv);

		iRC = otdoa_ubsa_proc_start_h1(OTDOA_pxlGetBSAPath(), enable_ue_encr, compress_ubsa,
						   (1 << LOG2_COMPRESS_WINDOW), pu8_iv);
		if (0 != iRC) {
			LOG_ERR("otdoa_ubsa_proc_start() failed with %d", iRC);
			iRC = -1;
			goto exit;
		}
	}

	LOG_INF("Writing %d bytes to uBSA file.", pG->nContentLength);
	iRC = otdoa_ubsa_write_data(pG->pData, pG->nContentLength);
	if (iRC < 0) {
		LOG_ERR("Failed to write to uBSA: %d", iRC);
		/* NB: make sure we handle this correctly--what do we do? a number retries
		 * and then abort?  need tests for this
		 */
		goto exit;
	}

	pG->nRange += (pG->nRangeSegmentEnd - pG->nRange) + 1;

	LOG_INF("nRange: %d, nRangeSegmentEnd: %d", pG->nRange, pG->nRangeSegmentEnd);

	pG->nOff = 0;

	if (pG->bDownloadComplete) {
		LOG_INF("uBSA download complete.");
		otdoa_ubsa_finish_data();
	}

exit:

	return iRC;
}

/**
 * Rebind the addresses in gH1_HTTP (there are two now, one for
 * legacy and one for h1).  When the Rebind message comes in,
 * we need to set up both.
 *
 * @param url address to bind to
 * @return void
 */
int otdoa_http_h1_rebind(const char *url)
{

	http_unbind(&gHTTP);
	return http_bind(&gHTTP, url);
}

/**
 * Format the HTTP header for the next chunk of the CFG file
 *
 * @param pG Pointer to gHTTP containing request data
 * @return Number of bytes written to header
 */
int otdoa_http_h1_format_cfg_request(tOTDOA_HTTP_MEMBERS *pG)
{
	int nReturn = 0;
	char jwt_token[256];

	if (!pG) {
		LOG_ERR("pG is null!");
		nReturn = 0;
		goto exit;
	}

	if (!pG->csBuffer) {
		LOG_ERR("csBuffer is null!");
		nReturn = 0;
		goto exit;
	}

	nReturn = generate_jwt(jwt_token, sizeof(jwt_token), &pG->nOff);
	if (nReturn != 0) {
		LOG_ERR("http_h1_format_request: generate_jwt() error: %d", nReturn);
		nReturn = 0;
		goto exit;
	}

	unsigned int uBufferLen = pG->bDisableTLS ? HTTP_BUF_SIZE : HTTPS_BUF_SIZE;

	memset(pG->csBuffer, 0, uBufferLen);

	nReturn = snprintf(pG->csBuffer, uBufferLen,
			  "GET /v1/config"
			  "?ue_firmware_version=h1.001"
			  " HTTP/1.1\r\n"
			  "Host: api.qa.hellaphy.cloud\r\n"
			  "User-agent: https_client/2.2.3\r\n"
			  "Accept: */*\r\n"
			  "Connection: keep-alive\r\n"
			  "authorization: Bearer %s\r\n"
			  "\r\n",
			  jwt_token);

	pG->nOff = 0;
	pG->szSend = pG->csBuffer;

exit:

	return nReturn;
}

/**
 * Get one chunk of the config file
 *
 * @param pG Pointer to gHTTP container buffer to read into
 * @return 0 of success, else error code
 */
int otdoa_http_h1_send_get_cfg_req(tOTDOA_HTTP_MEMBERS *pG)
{
	int iRC = 0;

	if (!pG) {
		LOG_ERR("pG is NULL!");
		iRC = -1;
		goto exit;
	}

	if (NULL == pG->csBuffer) {
		LOG_ERR("csBuffer is NULL");
		iRC = -1;
		goto exit;
	}

	iRC = otdoa_http_h1_format_cfg_request(pG);
	if (iRC <= 0) { /* greater than 0 is number of bytes written to header */
		LOG_ERR("otdoa_http_h1_format_cfg_request: %d", iRC);
		goto exit;
	}

	LOG_DBG("get config request: %s", pG->csBuffer);

	iRC = http_send_request(pG, strlen(pG->szSend));
	if (iRC) {
		LOG_ERR("http_get_cfg_chunk: http_send_request failed: %d", iRC);
		goto exit;
	}

exit:

	return iRC;
}

int otdoa_http_h1_handle_get_cfg(tOTDOA_HTTP_MEMBERS *pG)
{
	int iRC = 0;
	int iBytes;

	if (!pG) {
		LOG_ERR("pG is NULL!");
		iRC = -1;
		return iRC;
	}

	if (0 != otdoa_http_h1_get_cs_buffer()) {
		LOG_ERR("Failed to allocate CS Buffer");
		iRC = -1;
		goto exit;
	}
	otdoa_http_h1_init_config_dl();
	pG->nRangeMax = HTTPS_RANGE_MAX_DEFAULT;

	iRC = otdoa_http_h1_send_connect(pG, NULL);
	if (iRC) {
		LOG_ERR("http_connect failed: %d", iRC);
		iRC = -1;
		goto exit;
	}

	/* send the get config request */
	iRC = otdoa_http_h1_send_get_cfg_req(pG);
	if (iRC) {
		LOG_ERR("http_get_cfg_chunk failed: %d", iRC);
		iRC = -1;
		goto exit;
	}

	/* receive the response header*/
	iRC = otdoa_http_h1_receive_header(pG);

	/* if success, we have a complete header and we can parse */
	if (iRC > 0) {
		iRC = otdoa_http_h1_process_config_response_header(pG);
		if (0 == iRC) {
			LOG_INF("received config response header");
		} else {
			LOG_ERR("error receiving config response header");
			goto exit;
		}
	} else if (iRC < 0) {
		LOG_ERR("receive error %d", iRC);
		goto exit;
	} else  { /* iRC == 0*/
		LOG_ERR("0 bytes!");
		iRC = OTDOA_API_INTERNAL_ERROR; /* usually zero bytes means other end shutdown */
		goto exit;
	}

	iBytes = otdoa_http_h1_process_config_response_content(pG);
	if (iBytes < 0) {
		iRC = OTDOA_API_INTERNAL_ERROR;
		goto exit;
	}

exit:
	otdoa_http_h1_send_disconnect(pG);
	LOG_INF("Received %d bytes. Config request complete with result %d", pG->nRange, iRC);
	otdoa_http_h1_free_cs_buffer();

	return iRC;
}

extern void log_response_string(const char *pszKey, const char *pszBuffer);

#ifdef CONFIG_OTDOA_ENABLE_RESULTS_UPLOAD
/*
 * Generate an HTTPS request from the OTDOA results
 */
int otdoa_http_h1_handle_otdoa_results(tOTDOA_HTTP_MEMBERS *pG, const tOTDOA_HTTP_MESSAGE *pM)
{
	/* pre-generate the sprintf format string for the header to get its length.
	 * %04d format specifier for content length is the same length as generated output,
	 * and zero-padding the content length is allowable within RFC2616
	 */
	static const char header_format[] =
		"POST /uploadResults.php HTTP/1.1\r\n"
		"Host: hellaphy.cloud\r\n"
		"user-agent: OTDOA/7.68.0\r\n"
		"accept: */*\r\n"
		"Connection: close\r\n"
		"content-length: %04d\r\n" /* zero pad to 4 characters for constant header length */
		"content-type: application/x-www-form-urlencoded\r\n"
		"\r\n";

	const size_t header_length = strlen(header_format);

	memset(pG->csBuffer, 0, HTTPS_BUF_SIZE);

#define APPEND_PARAM(formatter, value)                                                             \
	content_length +=                                                                          \
		snprintf(pG->csBuffer + header_length + content_length,                            \
			 HTTPS_BUF_SIZE - (header_length + content_length), formatter, value)

	const otdoa_api_results_t *results = pM->http_upload_results.pResults;
	int content_length = 0;

	APPEND_PARAM("imei=%s", otdoa_get_imei_string());
	APPEND_PARAM("&pass=%s", RESULTS_UPLOAD_PW);
	APPEND_PARAM("&version_id=%s", otdoa_api_get_version());
	APPEND_PARAM("&sc_ecgi=%" PRIu32, results->details.serving_cell_ecgi);
	APPEND_PARAM("&dlearfcn=%" PRIu32, results->details.dlearfcn);
	APPEND_PARAM("&sc_rssi=%" PRIi32, results->details.serving_rssi_dbm);
	APPEND_PARAM("&num_cells=%" PRIu32, results->details.num_measured_cells);
	APPEND_PARAM("&est_lat=%3.6f", results->latitude);
	APPEND_PARAM("&est_lon=%3.6f", results->longitude);
	APPEND_PARAM("&num_prs=%" PRIu32, results->details.session_length);
	APPEND_PARAM("&est_algo=%s", results->details.estimate_algorithm);
	APPEND_PARAM("&uptime=%" PRIi32, http_uptime());

	APPEND_PARAM("&est_acc=%5.0f", (double)(results->accuracy));

	if (pM->http_upload_results.p_notes) {
		APPEND_PARAM("&notes=%s", pM->http_upload_results.p_notes);
	}

	if (results->details.num_measured_cells > 0) {
		APPEND_PARAM("&ecgi_list=%" PRIu32, results->details.ecgi_list[0]);
		for (int i = 1; i < results->details.num_measured_cells; i++) {
			APPEND_PARAM(",%" PRIu32, results->details.ecgi_list[i]);
		}

		APPEND_PARAM("&detection_count=%" PRIu16, results->details.toa_detect_count[0]);
		for (int i = 1; i < results->details.num_measured_cells; i++) {
			APPEND_PARAM(",%" PRIu16, results->details.toa_detect_count[i]);
		}
	}
	/* ToDo: restore true lat/lon and notes fields */
	if (pM->http_upload_results.p_true_lat && pM->http_upload_results.p_true_lon) {
		APPEND_PARAM("&true_lat=%s", pM->http_upload_results.p_true_lat);
		APPEND_PARAM("&true_lon=%s", pM->http_upload_results.p_true_lon);
	}

#undef APPEND_PARAM

	/* sprintf really doesn't like printing without a null terminator, so fix the header here */
	char tmp = pG->csBuffer[header_length];		/* save char that will be overwritten */

	snprintf(pG->csBuffer, HTTPS_BUF_SIZE, header_format, content_length);
	pG->csBuffer[header_length] = tmp;	/* restore the char */

	int rc = 0;

	rc = otdoa_http_h1_send_connect(pG, pM->http_upload_results.pURL);
	if (rc) {
		LOG_ERR("otdoa_http_h1_handle_otdoa_results: failed to connect to server: %d",
				  rc);
		return rc;
	}

	pG->nOff = 0;
	pG->szSend = pG->csBuffer;

	rc = http_send_request(pG, strlen(pG->szSend));
	if (rc) {
		LOG_ERR("otdoa_http_h1_handle_otdoa_results: failed to send results upload "
				  "request: %d",
				  rc);
		goto cleanup;
	}

	rc = otdoa_http_h1_receive_header(pG);
	if (rc <= 0) {
		LOG_ERR("otdoa_http_h1_handle_otdoa_results: failed to receive results "
				  "upload header: %d",
				  rc);
		goto cleanup;
	} else {
		/* otdoa_http_h1_receive_header() returns number of bytes received
		 * (i.e. positive for success)
		 */
		rc = 0;
	}

	const char *data = strstr(pG->csBuffer, "\r\n\r\n");

	if (data) {
		log_response_string("[*]  id", gHTTP.csBuffer);
		log_response_string("[*]  upload_date", gHTTP.csBuffer);
		log_response_string("[*]  est_lat", gHTTP.csBuffer);
		log_response_string("[*]  est_lon", gHTTP.csBuffer);
		log_response_string("[*]  est_algo", gHTTP.csBuffer);
		log_response_string("[*]  est_acc", gHTTP.csBuffer);
		log_response_string("[*]  unique_cells", gHTTP.csBuffer);
	}

	if (strstr(pG->csBuffer, "Upload Successful") == NULL) {
		rc = -1; /* force failure */
		LOG_ERR("Upload Successful not found in server response");
	}

cleanup: {
	/* preserve rc */
	const int rv = otdoa_http_h1_send_disconnect(pG);

	if (rv) {
		LOG_ERR(
			"otdoa_http_h1_handle_otdoa_results: failed to disconnect from server: %d",
			rv);
	}
}

	return rc;
}
#endif

/**
 * Check if an ECGI is on the blacklist, and decrements the age of all entries
 * @param p_http pointer to http struct
 * @param ecgi cell to check for
 * @return negative value on error, otherwise 0 or positive remaining blacklist time
 */
int otdoa_http_h1_blacklist_check(tOTDOA_HTTP_MEMBERS *p_http, const unsigned int ecgi)
{
	if (!p_http) {
		return -1;
	}
	if (ecgi == 0) {
		/* 0 is a reserved value and shouldn't be requested anyway, erro r*/
		return -2;
	}

	for (int i = 0; i < BLACKLIST_SIZE; ++i) {
		/* if the ecgi is blocked, return the remaining time */
		if (p_http->blacklist[i].ecgi == ecgi) {
			return p_http->blacklist[i].age;
		}
	}

	/* ecgi was not blocked */
	return 0;
}

/**
 * Step the blacklist one step, clearing expired entries
 * @param p_http pointer to http struct
 * @return number of entries that were cleared
 */
int otdoa_http_h1_blacklist_tick(tOTDOA_HTTP_MEMBERS *p_http)
{
	int ctr = 0;

	for (int i = 0; i < BLACKLIST_SIZE; ++i) {
		/* decrement all valid entries, clear ones that have expired*/
		if (p_http->blacklist[i].ecgi != 0 && p_http->blacklist[i].age > 0) {
			p_http->blacklist[i].age--;
			if (p_http->blacklist[i].age == 0) {
				LOG_DBG("Clearing blacklist entry for ECGI %u",
						p_http->blacklist[i].ecgi);
				p_http->blacklist[i].ecgi = 0;
				ctr++;
			}
		}
	}
	return ctr;
}

/**
 * Add an ecgi to the blacklist
 * @param p_http pointer to http struct
 * @param ecgi cell to add to blacklist
 * @return negative value on error, otherwise zero
 */
int otdoa_http_h1_blacklist_add(tOTDOA_HTTP_MEMBERS *p_http, const unsigned int ecgi)
{
	if (!p_http) {
		return -1;
	}
	if (ecgi == 0) {
		/*  0 is a reserved value and shouldn't be requested anyway, error */
		return -2;
	}

	/* find the first unused blacklist slot, or the one with the next entry to expire */
	int oldest_index = 0;
	int oldest_age = BLACKLIST_TIMEOUT + 1;

	for (int i = 0; i < BLACKLIST_SIZE; ++i) {
		if (p_http->blacklist[i].age < oldest_age) {
			oldest_index = i;
			oldest_age = p_http->blacklist[i].age;
		}
		if (p_http->blacklist[i].ecgi == ecgi) {
			/* already have an entry? update it */
			oldest_index = i;
			break;
		}
	}

	p_http->blacklist[oldest_index].ecgi = ecgi;
	p_http->blacklist[oldest_index].age = BLACKLIST_TIMEOUT;

	return 0;
}

/**
 * Remove an entry from the blacklist
 * @param p_http pointer to http struct
 * @param ecgi cell to remove from list
 * @return negative value on error, otherwise positive age that was left in entry
 */
int otdoa_http_h1_blacklist_clear(tOTDOA_HTTP_MEMBERS *p_http, const unsigned int ecgi)
{
	if (!p_http) {
		return -1;
	}
	if (ecgi == 0) {
		/*  0 is a reserved value and shouldn't be requested anyway, error */
		return -2;
	}

	for (int i = 0; i < BLACKLIST_SIZE; ++i) {
		if (p_http->blacklist[i].ecgi == ecgi) {
			int prev_age = p_http->blacklist[i].age;

			p_http->blacklist[i].age = 0;
			p_http->blacklist[i].ecgi = 0;
			return prev_age;
		}
	}

	return -3; /* cell was not found */
}

int otdoa_http_h1_blacklist_init(tOTDOA_HTTP_MEMBERS *p_http)
{
	if (!p_http) {
		return -1;
	}
	memset(p_http->blacklist, 0, sizeof(p_http->blacklist));
	return 0;
}

int otdoa_http_h1_test_jwt(tOTDOA_HTTP_MEMBERS *pG)
{
	if (!pG) {
		LOG_ERR("no HTTP struct!");
		return -1;
	}

	if (otdoa_http_h1_rebind(NULL)) {
		LOG_ERR("Failed to rebind in test_jwt");
		return -1;
	}

	LOG_INF("Generating JWT token");

	char token[256];
	size_t token_length;
	int rc = generate_jwt(token, sizeof(token), &token_length);

	if (rc) {
		LOG_ERR("Failed to generate JWT token: %d", rc);
		return -1;
	}
	LOG_INF("Generated JWT token: %s", token);

	int request_size = snprintf(
		pG->csBuffer, HTTPS_BUF_SIZE,
		"GET "
		"/v1/ubsa.php?ecgi=20357892&encrypt=1&compress_window=%d&dlearfcn=5230&radius=100 "
		"HTTP/1.1\r\nHost: "
		"hellaphy.cloud:443\r\naccept: */*\r\nauthorization: Bearer %s\r\n\r\n",
		LOG2_COMPRESS_WINDOW, token);
	if (request_size >= HTTPS_BUF_SIZE) {
		LOG_ERR("Overflow when preparing HTTP request");
		return -2;
	}
	pG->nOff = 0;
	pG->szSend = pG->csBuffer;

	rc = otdoa_http_h1_send_connect(pG, NULL);
	if (rc) {
		LOG_ERR("Failed to connect to host: %d", rc);
		return -1;
	}

	rc = http_send_request(pG, request_size);
	if (rc) {
		LOG_ERR("Failed to send request: %d", rc);
		goto cleanup;
	}

	rc = otdoa_http_h1_receive_header(pG);
	if (rc <= 0) {
		LOG_ERR("Failed to receive response: %d", rc);
		goto cleanup;
	}

	rc = otdoa_http_h1_parse_response_code(pG->csBuffer);
	if (rc != 200) {
		LOG_ERR("Got unexpected response from server: %d", rc);
		rc = -1; /* force a failure */
	}

	LOG_INF("JWT test success!");

cleanup:
	if (otdoa_http_h1_send_disconnect(pG)) {
		LOG_ERR("Failed to disconnect from server.");
	}

	return rc;
}
