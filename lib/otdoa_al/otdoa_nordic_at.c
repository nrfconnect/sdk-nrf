/*
 * Copyright (c) 2025 PHY Wireless, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-PHYW
 */

#include <stdint.h>
#include <stddef.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include "nrf_modem_at.h"

#include <otdoa_al/otdoa_api.h>
#include <otdoa_al/otdoa_nordic_at.h>
#include "otdoa_http.h"
#include "otdoa_al_log.h"

LOG_MODULE_DECLARE(otdoa_al, LOG_LEVEL_INF);

/*
 * Our own version of str_tok_r that stops on the first token found.
 * So it doesn't eat consecutive delimeters
 */
char *otdoa_nordic_at_strtok_r(char *s, char delim, char **save_ptr)
{
	char *end;

	if (s == NULL) {
		s = *save_ptr;
	}

	if (s == NULL) {
		return NULL;
	}

	if (*s == '\0') {
		*save_ptr = s;
		return NULL;
	}

	/* Scan leading delimiters.  */
	if (*s == delim) {
		/* will return empty token in this case */
		*s = '\0';
		end = s;
	} else {
		end = strchr(s, delim);
	}

	if (end == NULL) {
		*save_ptr = NULL;
		return s;
	}

	/* Terminate the token and make *SAVE_PTR point past it.  */
	*end = '\0';
	*save_ptr = end + 1;
	return s;
}

/* Registration values returned from the modem */

/* Not registered. UE is not currently searching for an operator */
#define REG_STATUS_NONE	      0
/* Registered, home network */
#define REG_STATUS_REGISTERED 1
/*  Not registered, but UE is currently trying to attach or searching an operator */
#define REG_STATUS_NOT_REG    2
/* Registration denied */
#define REG_STATUS_DENIED     3
/* Unknown (for example, out of E-UTRAN coverage) */
#define REG_STATUS_UNKNOWN    4
/* Registered, roaming */
#define REG_STATUS_ROAMING    5
/*  Not registered due to UICC failure */
#define REG_STATUS_UICC_FAIL  90

/* Maximum number of tokens we will parse in the AT%%XMONITOR response */
#define XMONITOR_RESP_MAX_TOKENS   16
#define XMONITOR_RESP_MIN_PLMN_LEN 5 /* three digits for MCC, two or three for MNC */
#define XMONITOR_UNKNOWN_ECGI     0xFFFFFFFF
#define XMONITOR_UNKNOWN_ACT      0xFFFFFFFF
#define XMONITOR_UNKNOWN_MCC      0xFFFF
#define XMONITOR_UNKNOWN_MNC      0xFFFF

/* Parse the response to AT%%XMONITOR and return ECGI & DLEARFCN */
int otdoa_nordic_at_parse_xmonitor_response(const char *const psz_resp, size_t u_resp_len,
					    otdoa_xmonitor_params_t *params)
{
	int i_ret = 0;
	int n_token = 0;

	/* these values are populated from the modem response */
	uint32_t u32_egci = XMONITOR_UNKNOWN_ECGI;
	uint32_t u32_dlearfcn = UNKNOWN_UBSA_DLEARFCN;
	uint32_t u32_reg_status = REG_STATUS_NONE;
	uint32_t u32_AcT = XMONITOR_UNKNOWN_ACT;
	uint16_t u16_mcc = XMONITOR_UNKNOWN_MCC;
	uint16_t u16_mnc = XMONITOR_UNKNOWN_MNC;
	uint16_t u16_pci = UNKNOWN_UBSA_PCI;

	if (!psz_resp) {
		LOG_ERR("otdoa_nordic_at_parse_xmonitor_response(): NULL pointer\n");
		return OTDOA_API_INTERNAL_ERROR;
	}
	if (*psz_resp == '\0') {
		LOG_ERR("otdoa_nordic_at_parse_xmonitor_response(): Empty string\n");
		return OTDOA_API_INTERNAL_ERROR;
	}

	/* Parse into tokens */
	char strCopy[128] = {0};

	strncpy(strCopy, psz_resp, sizeof(strCopy) - 1); /* NB: '-1" to ensure null term */
	char s = ',';
	char *pstrCopy = strCopy;
	char *token = strstr(pstrCopy, " "); /* Skip the AT string name */

	if (!token) {
		i_ret = OTDOA_EVENT_FAIL_BAD_MODEM_RESP;
		goto error_exit;
	}
	for (token = 1 + otdoa_nordic_at_strtok_r(token, s, &pstrCopy); token != NULL;
		token = otdoa_nordic_at_strtok_r(pstrCopy, s, &pstrCopy)) {

		switch (n_token) {
		case 0: /* Registration Status */
		{
			int i_scn_rv = sscanf(token, "%" SCNu32, &u32_reg_status);

			if (1 != i_scn_rv) {
				i_ret = OTDOA_EVENT_FAIL_BAD_MODEM_RESP;
			} else {
				/* allow both REGISTERED and ROAMINIG status */
				if (u32_reg_status != REG_STATUS_REGISTERED &&
				    u32_reg_status != REG_STATUS_ROAMING) {
					i_ret = OTDOA_EVENT_FAIL_NOT_REGISTERED;
				}
			}
			break;
		}
		case 3: /* PLMN - MCC/MNC */
		{
			if (strlen(token) < XMONITOR_RESP_MIN_PLMN_LEN) {
				i_ret = OTDOA_EVENT_FAIL_BAD_MODEM_RESP;
				break;
			}
			int i_scn_rv = sscanf(token, "\"%3" SCNu16, &u16_mcc);

			if (i_scn_rv != 1) {
				i_ret = OTDOA_EVENT_FAIL_BAD_MODEM_RESP;
				break;
			}
			i_scn_rv = sscanf(token + 4, "%" SCNu16,
					  &u16_mnc); /* add 4 for 3 digits of MCC plus leading " */
			if (i_scn_rv != 1) {
				i_ret = OTDOA_EVENT_FAIL_BAD_MODEM_RESP;
			}
			break;
		}
		case 5: /* AcT */
		{
			int i_scn_rv = sscanf(token, "%" SCNu32, &u32_AcT);

			if (1 != i_scn_rv) {
				i_ret = OTDOA_EVENT_FAIL_BAD_MODEM_RESP;
			} else {
				if (u32_AcT != 7) {
					i_ret = OTDOA_EVENT_FAIL_NOT_LTE_MODE;
				}
			}
			break;
		}
		case 7: /* ECGI */
		{
			/* NB: ECGI is in hex format, surrouded by quotes ("") */
			int i_scn_rv = sscanf(token, "\"%" SCNx32, &u32_egci);

			if (1 != i_scn_rv || u32_egci == 0) {
				i_ret = OTDOA_EVENT_FAIL_BAD_MODEM_RESP;
			}
			break;
		}
		case 8: /* PCI */
		{
			int i_scn_rv = sscanf(token, "%" SCNu16, &u16_pci);

			if (i_scn_rv != 1) {
				i_ret = OTDOA_EVENT_FAIL_NO_PCI;
				break;
			}
			break;
		}
		case 9: /* DLEARFCN */
		{
			int i_scn_rv = sscanf(token, "%" SCNu32 "", &u32_dlearfcn);

			if (1 != i_scn_rv || u32_dlearfcn == 0) {
				i_ret = OTDOA_EVENT_FAIL_NO_DLEARFCN;
			}
			break;
		}
		default:
			break;
		}
		if (i_ret) {
			/* Don't continue after an error */
			break;
		}
		if (++n_token > XMONITOR_RESP_MAX_TOKENS) {
			LOG_ERR("XMONITOR response: too many tokens");
			break;
		}
	}

error_exit:
	if (0 == i_ret) {
		/* if no errors in parsing */
		/* Check the parsed response values */
		if (u32_egci == 0 || u32_dlearfcn == 0) {
			i_ret = OTDOA_EVENT_FAIL_BAD_MODEM_RESP;
		} else {
			params->ecgi = u32_egci;
			params->dlearfcn = u32_dlearfcn;
			params->act = u32_AcT;
			params->mcc = u16_mcc;
			params->mnc = u16_mnc;
			params->pci = u16_pci;
			params->reg_status = u32_reg_status;
		}
	} else if (OTDOA_EVENT_FAIL_NO_DLEARFCN == i_ret && u32_egci != 0) {
		/* return the ECGI and default DLEARFCN, and error code indicating no DLEARFCN */
		params->ecgi = u32_egci;
		params->dlearfcn = UNKNOWN_UBSA_DLEARFCN;
		params->act = u32_AcT;
		params->mcc = u16_mcc;
		params->mnc = u16_mnc;
		params->pci = u16_pci;
		params->reg_status = u32_reg_status;
	} else if (OTDOA_EVENT_FAIL_NO_PCI == i_ret && u32_egci != 0) {
		/* return the ECGI and default DLEARFCN, and error code indicating no DLEARFCN */
		params->ecgi = u32_egci;
		params->dlearfcn = u32_dlearfcn;
		params->act = u32_AcT;
		params->mcc = u16_mcc;
		params->mnc = u16_mnc;
		params->pci = UNKNOWN_UBSA_PCI;
		params->reg_status = u32_reg_status;
	}

	if (i_ret != 0
		&& i_ret != OTDOA_EVENT_FAIL_NO_DLEARFCN && i_ret != OTDOA_EVENT_FAIL_NO_PCI) {
		if (psz_resp) {
			LOG_ERR("AT%%XMONITOR response %s", psz_resp);
		}
		LOG_ERR("Failed to parse AT%%XMONITOR response.  returning %d\n", i_ret);
	} else {
		LOG_DBG("otdoa_nordic_at_parse_xmonitor_response: ECGI=%" PRIu32
			      " (0x%08X), DLEARFCN=%" PRIu32 " returning %d\n",
			      u32_egci, (unsigned int)u32_egci, u32_dlearfcn, i_ret);
	}

	return i_ret;
}

/* Use AT%%XMONITOR command to get the current ECGI and DLEARFCN from the modem */
int otdoa_nordic_at_get_xmonitor(otdoa_xmonitor_params_t *params)
{
	int i_ret = 0;
	static char monitor_buf[256] = {0};

	memset(monitor_buf, 0, sizeof(monitor_buf));
	i_ret = nrf_modem_at_cmd(monitor_buf, sizeof(monitor_buf), "AT%%XMONITOR");
	if (i_ret) {
		LOG_ERR("otdoa_nordic_at_get_xmonitor: ERROR (%d) Failed to get "
			      "MODEM Status\n",
			      i_ret);
		i_ret = OTDOA_EVENT_FAIL_BAD_MODEM_RESP;
	} else {
		i_ret = otdoa_nordic_at_parse_xmonitor_response(monitor_buf,
						       strlen(monitor_buf), params);
	}
	return i_ret;
}

/*
 * The AT command query we will use.  Note that "=1" ensures we get the IMEI
 * value (see AT command reference)
 */
#define IMEI_QUERY	     "AT+CGSN=1"
#define IMEI_QUERY_LEN	     sizeof(IMEI_QUERY)
#define IMEI_RESP	     "+CGSN: \""
#define IMEI_RESP_LEN	     sizeof(IMEI_RESP)
#define NRF_IMEI_LEN	     15
#define CGSN_RESPONSE_LENGTH 35
static char otdoa_nordic_at_imei[NRF_IMEI_LEN + 1] = {0}; /* "+1" to ensure null term */

/**
 * Sends AT+CGSN=1
 * Resp:
 * +CGSN: "352656100367872"
 * OK
 *
 * Where the IMEI is a 15-digit string
 */
int otdoa_nordic_at_get_imei_from_modem(void)
{
	char imei_buf[CGSN_RESPONSE_LENGTH + 1];
	int err;

	memset(otdoa_nordic_at_imei, 0, sizeof(otdoa_nordic_at_imei));

	err = nrf_modem_at_cmd(imei_buf, sizeof(imei_buf), IMEI_QUERY);
	if (err) {
		LOG_ERR("Failed to get IMEI\n");
		return OTDOA_EVENT_FAIL_BAD_MODEM_RESP;
	}

	/* find '+CGSN: "' in response (including opening quote char) */
	imei_buf[CGSN_RESPONSE_LENGTH] = '\0'; /* ensure null termination */
	char *p_imei_token = strstr(imei_buf, IMEI_RESP);

	if (!p_imei_token ||
	    strnlen(p_imei_token, CGSN_RESPONSE_LENGTH) < (NRF_IMEI_LEN + IMEI_RESP_LEN)) {
		goto error_return;
	}

	/* find closing quote at end of IMEI string */
	p_imei_token += strlen(IMEI_RESP); /* point to start of IMEI string */
	char *p_imei_end = strchr(p_imei_token, '"');

	if (!p_imei_end) {
		goto error_return; /* no closing quote */
	}

	size_t len = p_imei_end - p_imei_token;

	if (len < NRF_IMEI_LEN) {
		goto error_return; /* IMEI too short */
	}
	if (len > NRF_IMEI_LEN) {
		len = NRF_IMEI_LEN;
	}
	strncpy((char *)otdoa_nordic_at_imei, p_imei_token, len);
	LOG_INF("Got IMEI %s\n", otdoa_nordic_at_imei);

	return 0;

error_return:
	LOG_ERR("otdoa_nordic_at_get_imei_from_modem() invalid response %s", imei_buf);
	return OTDOA_EVENT_FAIL_BAD_MODEM_RESP;
}

/* returns the current IMEI value */
const char *otdoa_get_imei_string(void)
{
	return otdoa_nordic_at_imei;
}

/* For unit tests: */
#ifdef HOST
void otdoa_set_imei_string(const char *const p_string)
{
	if (p_string) {
		strncpy(otdoa_nordic_at_imei, p_string, sizeof(otdoa_nordic_at_imei));
	} else {
		otdoa_nordic_at_imei[0] = '\0';
	}
}
#endif

#define MODEM_VER_QUERY	   "AT%%SHORTSWVER"
#define MODEM_VER_RESP_LEN 50
#define MODEM_VER_RESP	   "%SHORTSWVER: "
int otdoa_nordic_at_get_modem_version(char *psz_ver, unsigned int max_len)
{
	char resp[MODEM_VER_RESP_LEN] = {0};
	int err = nrf_modem_at_cmd(resp, sizeof(resp), MODEM_VER_QUERY);

	if (err) {
		LOG_ERR("Failed to get modem version %d\n", err);
		return OTDOA_EVENT_FAIL_BAD_MODEM_RESP;
	}

	resp[MODEM_VER_RESP_LEN - 1] = '\0'; /* ensure null termination */
	char *p_ver_token = strstr(resp, MODEM_VER_RESP);

	if (!p_ver_token) {
		LOG_ERR("Failed to parse modem version %s\n", resp);
		return OTDOA_EVENT_FAIL_BAD_MODEM_RESP;
	}
	/* Find the end and null-terminate */
	char *p_end = strchr(p_ver_token, '\r');

	if (p_end < resp + MODEM_VER_RESP_LEN) {
		*p_end = 0;
	}

	p_ver_token += strlen(MODEM_VER_RESP); /* point to start of version string */
	strncpy(psz_ver, p_ver_token, max_len);

	return OTDOA_API_SUCCESS;
}
