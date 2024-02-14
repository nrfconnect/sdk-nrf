/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(CONFIG_POSIX_API)
#include <zephyr/posix/arpa/inet.h>
#else
#include <zephyr/net/socket.h>
#endif

#include "lte_params.h"
#include "utils.h"

/* Minimum XMONITOR MCC/MNC column buffer length with double quotes */
#define MIN_XMONITOR_MCC_MNC_LEN 7
/* Minimum XMONITOR TAC column buffer length with double quotes */
#define MIN_XMONITOR_TAC_LEN 6
/* Minimum XMONITOR CID column buffer length with double quotes */
#define MIN_XMONITOR_CID_LEN 10

#define TAC_STR_LEN 4
#define CID_STR_LEN 8

/**
 * @brief Parse and fill XMONITOR MCC to client context lte_params
 *
 * @param[in,out] lte       LTE params to fill
 * @param[in]     buf       Input buffer. Assumes form "24412" or "244321",
 *                          where first three digits after quotes define MCC
 *                          and the following two or three define MNC.
 * @param[in]     buf_len   Input buffer length. Error, if length < 7.
 *
 * @return 0 when MCC filled successfully, -1 otherwise
 */
static int parse_lte_mcc(struct lte_params *lte,
			 const char *buf,
			 int buf_len)
{
	if (buf_len < MIN_XMONITOR_MCC_MNC_LEN) {
		memset(lte->mcc, 0, 3);
		return -1;
	}

	uint8_t c;

	if (char2hex(buf[1], &c) != 0) {
		memset(lte->mcc, 0, 3);
		return -1;
	}
	lte->mcc[0] = c;

	if (char2hex(buf[2], &c) != 0) {
		memset(lte->mcc, 0, 3);
		return -1;
	}
	lte->mcc[1] = c;

	if (char2hex(buf[3], &c) != 0) {
		memset(lte->mcc, 0, 3);
		return -1;
	}
	lte->mcc[2] = c;

	return 0;
}

/**
 * @brief Parse and fill XMONITOR MNC to client context lte_params
 *
 * @param[in,out] lte       LTE params to fill
 * @param[in]     buf       Input buffer. Assumes form "24412" or "244321",
 *                          where first three digits after quotes define MCC
 *                          and the following two or three define MNC.
 * @param[in]     buf_len   Input buffer length. Error, if length < 7.
 *
 * @return 0 when MNC filled successfully, -1 otherwise
 */
static int parse_lte_mnc(struct lte_params *lte,
			 const char *buf,
			 int buf_len)
{
	if (buf_len < MIN_XMONITOR_MCC_MNC_LEN) {
		memset(lte->mnc, 0, 3);
		return -1;
	}

	uint8_t c;

	if (char2hex(buf[4], &c) != 0) {
		memset(lte->mnc, 0, 3);
		return -1;
	}
	lte->mnc[0] = c;

	if (char2hex(buf[5], &c) != 0) {
		memset(lte->mnc, 0, 3);
		return -1;
	}
	lte->mnc[1] = c;

	/* mnc is two or three digits long */
	if (buf_len == MIN_XMONITOR_MCC_MNC_LEN) {
		lte->mnc_length = 2;
	} else {
		lte->mnc_length = 3;
		if (char2hex(buf[6], &c) != 0) {
			memset(lte->mnc, 0, 3);
			return -1;
		}
		lte->mnc[2] = c;
	}

	return 0;
}

/**
 * @brief Parse and fill XMONITOR TAC to client context lte_params
 *
 * @param[in,out] lte       LTE params to fill
 * @param[in]     buf       Input buffer. Assumes form "00B7",
 *                          a 2-byte TAC in hexadecimal format with quotes.
 * @param[in]     buf_len   Input buffer length. Error, if length < 6.
 *
 * @return 0 when TAC filled successfully, -1 otherwise
 */
static int parse_lte_tac(struct lte_params *lte,
			 const char *buf,
			 int buf_len)
{
	if (buf_len < MIN_XMONITOR_TAC_LEN) {
		memset(lte->tac, 0, 2);
		return -1;
	}

	if (sizeof(lte->tac) != hexstr2hex(&buf[1],
					   TAC_STR_LEN,
					   lte->tac,
					   sizeof(lte->tac))) {
		memset(lte->tac, 0, 2);
		return -1;
	}

	return 0;
}

/**
 * @brief Parse and fill XMONITOR CID to client context lte_params
 *
 * @param[in,out] lte       LTE params to fill
 * @param[in]     buf       Input buffer. Assumes form "00011B07",
 *                          a 4-byte E-UTRAN cell ID in hexadecimal
 *                          format with quotes.
 * @param[in]     buf_len   Input buffer length. Error, if length < 10.
 *
 * @return 0 when CID filled successfully, -1 otherwise
 */
static int parse_lte_cid(struct lte_params *lte,
			 const char *buf,
			 int buf_len)
{
	if (buf_len < MIN_XMONITOR_CID_LEN) {
		memset(lte->cid, 0, 4);
		return -1;
	}

	if (sizeof(lte->cid) != hexstr2hex(&buf[1],
					   CID_STR_LEN,
					   lte->cid,
					   sizeof(lte->cid))) {
		memset(lte->cid, 0, 4);
		return -1;
	}

	return 0;
}

/**
 * @brief Parse and fill XMONITOR physical CID to client context lte_params
 *
 * @param[in,out] lte pointer to client context to fill
 * @param[in]     buf pointer to input buffer. Assumes integer without quotes.
 * @param[in]     buf_len input buffer length. Error, if length < 1.
 *
 * @return 0 when physical CID filled successfully, -1 otherwise
 */
static int parse_lte_phys_cid(struct lte_params *lte,
			      const char *buf,
			      int buf_len)
{
	if (buf_len < 1) {
		lte->phys_cid = 0;
		return -1;
	}

	lte->phys_cid = (uint16_t)strtol(&buf[0], NULL, 0);

	return 0;
}

void lte_params_init(struct lte_params *params)
{
	memset(params, 0, sizeof(struct lte_params));

	/* status is always set to true as a workaround for a bug in the SUPL client library.
	 * When LTE registration information is not available, all parameters are zeroes.
	 */
	params->status = true;
}

int handler_at_xmonitor(struct supl_session_ctx *session_ctx,
			int col_number,
			const char *buf,
			int buf_len)
{
	switch (col_number) {
	case 3:
		if (parse_lte_mcc(&session_ctx->lte_params,
				  buf,
				  buf_len) != 0) {
			return -1;
		}
		if (parse_lte_mnc(&session_ctx->lte_params,
				  buf,
				  buf_len) != 0) {
			return -1;
		}
		break;
	case 4:
		return parse_lte_tac(&session_ctx->lte_params,
				     buf,
				     buf_len);
	case 7:
		return parse_lte_cid(&session_ctx->lte_params,
				     buf,
				     buf_len);
	case 8:
		return parse_lte_phys_cid(&session_ctx->lte_params,
					  buf,
					  buf_len);
	default:
		break;
	}

	return 0;
}

void device_id_init(struct supl_session_ctx *session_ctx)
{
	session_ctx->device_id_choice = LIBSUPL_ID_CHOICE_IPV4;
	memset(&session_ctx->device_id, 0, sizeof(session_ctx->device_id));
}

/**
 * @brief Parse and fill device ID to the session context
 *
 * @param[in,out] lte pointer to session context to fill
 * @param[in]     buf pointer to input buffer. Assumes a quoted string.
 * @param[in]     buf_len input buffer length.
 *
 * @return 0 when device ID was filled successfully, -1 otherwise
 */
static int parse_ip_address(struct supl_session_ctx *session_ctx,
			    const char *buf,
			    int buf_len)
{
	char addr_buf[INET6_ADDRSTRLEN] = { 0 };

	/* Start from index 1 to skip first quote and copy the first IP address */
	for (int i = 1; i < buf_len && i <= INET6_ADDRSTRLEN; i++) {
		if (buf[i] == '\"' || buf[i] == ' ') {
			/* End of the quoted string or a space between IP addresses */
			break;
		}
		addr_buf[i - 1] = buf[i];
	}

	if (inet_pton(AF_INET, addr_buf, session_ctx->device_id)) {
		session_ctx->device_id_choice = LIBSUPL_ID_CHOICE_IPV4;
	} else if (inet_pton(AF_INET6, addr_buf, session_ctx->device_id)) {
		session_ctx->device_id_choice = LIBSUPL_ID_CHOICE_IPV6;
	} else {
		return -1;
	}

	return 0;
}

int handler_at_cgdcont(struct supl_session_ctx *session_ctx,
		       int col_number,
		       const char *buf,
		       int buf_len)
{
	switch (col_number) {
	case 3:
		return parse_ip_address(session_ctx,
					buf,
					buf_len);
	default:
		break;
	}

	return 0;
}
