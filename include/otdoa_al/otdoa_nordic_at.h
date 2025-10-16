/*
 * Copyright (c) 2025 PHY Wireless, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-PHYW
 */

#ifndef INCLUDE_OTDOA_NORDIC_AT_H1_H_
#define INCLUDE_OTDOA_NORDIC_AT_H1_H_

#include <stdint.h>
#include <stddef.h>

/**
 * @file otdoa_nordic_at.h
 *
 * @defgroup otdoa_nordic_at_h1 OTDOA Nordic AT Command utilities
 * @{
 * @brief AT Command utilities for Nordic-Based "H1" product
 */

/**
 * @brief AT%XMONITOR parameters
 * This structure contains supported XMONITOR response fields
 */
typedef struct {
	/** Registration Status:
	 * 0: Not Registered
	 * 1: Registered, home network
	 * 2: Not registered, but searching
	 * 3: Registration denied
	 * 4: Unknown
	 * 5: Registered, roaming
	 * 90: Not registered due to UICC failure
	 */
	uint32_t reg_status;

	/** 4-byte E-UTRAN cell ID */
	uint32_t ecgi;

	/** EARFCN */
	uint32_t dlearfcn;

	/** Mobile Country Code */
	uint16_t mcc;

	/** Mobile Network Code */
	uint16_t mnc;

	/** Physical Cell ID */
	uint16_t pci;

	/** AcT:
	 * 7: E-UTRAN
	 * 9: E-UTRAN NB-S1
	 */
	uint16_t act;
} otdoa_xmonitor_params_t;

/** @brief Parse the response to AT%%XMONITOR and return ECGI & DLEARFCN
 * @param psz_resp      [in]   response string returned by the modem
 * @param u_resp_len    [in]   Length of the response string
 * @param params        [out]  Pointer to parameters struct to store response in
 * @return     0               Success
 *
 * @note response is documented in "nRF91 AT Commands Command Reference Guide" v2.0
 * it has the form:
 *             0 1                  2     3        4      5 6  7
 *  %XMONITOR: 1,"Verizon Wireless","VZW","311480","2F02",7,13,"00B7F901",
 *
 *             8   9    10 11 12 13          14         15
 *             282,5230,37,25,"","11100000","11100000","01011110"
 */
int otdoa_nordic_at_parse_xmonitor_response(const char *const psz_resp, size_t u_resp_len,
						otdoa_xmonitor_params_t *params);

/**
 * @brief Use AT%%XMONITOR command to get the current ECGI and DLEARFCN from the modem
 * @param params        [out]  parameters struct to store response values
 * @return  0 on success
 *          values from otdoa_api_error_codes_t (in otdoa_api.h) on any failure
 *
 */
int otdoa_nordic_at_get_xmonitor(otdoa_xmonitor_params_t *params);

/**
 * @brief Gets the IMEI from the modem
 * @return 0 on success
 *         OTDOA_EVENT_FAIL_BAD_MODEM_RESP for any failure
 */
int otdoa_nordic_at_get_imei_from_modem(void);

/**
 * @brief returns the current IMEI value
 * @return pointer to the IMEI value
 */
const char *otdoa_get_imei_string(void);

/**
 * @brief Our own version of str_tok_r that stops on the first token found.
 * So it doesn't eat consecutive delimeters
 */
char *otdoa_nordic_at_strtok_r(char *s, char delim, char **save_ptr);

/**
 * @brief Returns the version string from the modem firmware
 * @param psz_ver [out] output version string
 * @param max_len [in]  Maximum length of data to write
 * @return 0 on success
 */
int otdoa_nordic_at_get_modem_version(char *psz_ver, unsigned int max_len);

/** @} */

#endif /* INCLUDE_OTDOA_NORDIC_AT_H1_H_ */
