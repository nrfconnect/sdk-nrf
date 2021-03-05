/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <zephyr/types.h>
#include <string.h>
#include <stdio.h>
#include <modem/lte_lc.h>
#include <modem/at_cmd.h>
#include <modem/at_cmd_parser.h>
#include <modem/at_params.h>
#include <logging/log.h>

#define LC_MAX_READ_LENGTH			128

#define AT_CMD_SIZE(x)				(sizeof(x) - 1)
#define AT_RESPONSE_PREFIX_INDEX		0
#define AT_CFUN_READ				"AT+CFUN?"
#define AT_CFUN_RESPONSE_PREFIX			"+CFUN"
#define AT_CFUN_MODE_INDEX			1
#define AT_CFUN_PARAMS_COUNT			2
#define AT_CFUN_RESPONSE_MAX_LEN		20
#define AT_CEREG_5				"AT+CEREG=5"
#define AT_CEREG_READ				"AT+CEREG?"
#define AT_CEREG_RESPONSE_PREFIX		"+CEREG"
#define AT_CEREG_PARAMS_COUNT_MAX		11
#define AT_CEREG_REG_STATUS_INDEX		1
#define AT_CEREG_READ_REG_STATUS_INDEX		2
#define AT_CEREG_TAC_INDEX			2
#define AT_CEREG_READ_TAC_INDEX			3
#define AT_CEREG_CELL_ID_INDEX			3
#define AT_CEREG_READ_CELL_ID_INDEX		4
#define AT_CEREG_ACT_INDEX			4
#define AT_CEREG_READ_ACT_INDEX			5
#define AT_CEREG_ACTIVE_TIME_INDEX		7
#define AT_CEREG_READ_ACTIVE_TIME_INDEX		8
#define AT_CEREG_TAU_INDEX			8
#define AT_CEREG_READ_TAU_INDEX			9
#define AT_CEREG_RESPONSE_MAX_LEN		80
#define AT_XSYSTEMMODE_READ			"AT%XSYSTEMMODE?"
#define AT_XSYSTEMMODE_RESPONSE_PREFIX		"%XSYSTEMMODE"
#define AT_XSYSTEMMODE_PROTO			"AT%%XSYSTEMMODE=%d,%d,%d,%d"

/* The indices are for the set command. Add 1 for the read command indices. */
#define AT_XSYSTEMMODE_READ_LTEM_INDEX		1
#define AT_XSYSTEMMODE_READ_NBIOT_INDEX		2
#define AT_XSYSTEMMODE_READ_GPS_INDEX		3
#define AT_XSYSTEMMODE_READ_PREFERENCE_INDEX	4
#define AT_XSYSTEMMODE_PARAMS_COUNT		5
#define AT_XSYSTEMMODE_RESPONSE_MAX_LEN		30

/* CEDRXS command parameters */
#define AT_CEDRXS_MODE_INDEX
#define AT_CEDRXS_ACTT_WB			4
#define AT_CEDRXS_ACTT_NB			5

/* CEDRXP notification parameters */
#define AT_CEDRXP_PARAMS_COUNT_MAX		6
#define AT_CEDRXP_ACTT_INDEX			1
#define AT_CEDRXP_REQ_EDRX_INDEX		2
#define AT_CEDRXP_NW_EDRX_INDEX			3
#define AT_CEDRXP_NW_PTW_INDEX			4

/* CSCON command parameters */
#define AT_CSCON_RESPONSE_PREFIX		"+CSCON"
#define AT_CSCON_PARAMS_COUNT_MAX		4
#define AT_CSCON_RRC_MODE_INDEX			1
#define AT_CSCON_READ_RRC_MODE_INDEX		2

/* @brief Helper function to check if a response is what was expected.
 *
 * @param response Pointer to response prefix
 * @param response_len Length of the response to be checked
 * @param check The buffer with "truth" to verify the response against,
 *		for example "+CEREG"
 *
 * @return True if the provided buffer and check are equal, false otherwise.
 */
bool response_is_valid(const char *response, size_t response_len,
		       const char *check);

/* @brief Parses an AT command response, and returns the current RRC mode.
 *
 * @param at_response Pointer to buffer with AT response.
 * @param mode Pointer to where the RRC mode is stored.
 * @param mode_index Parameter index for mode.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int parse_rrc_mode(const char *at_response,
		   enum lte_lc_rrc_mode *mode,
		   size_t mode_index);

/* @brief Parses an AT command response and returns the current eDRX configuration.
 *
 * @note It's assumed that the network only reports valid eDRX values when
 *	 in each mode (LTE-M and NB1). There's no sanity-check of these values.
 *
 * @param at_response Pointer to buffer with AT response.
 * @param cfg Pointer to where the eDRX configuration is stored.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int parse_edrx(const char *at_response, struct lte_lc_edrx_cfg *cfg);

/* @brief Parses an AT+CEREG response parameter list and extracts the PSM
 *	  configuration.
 *
 * @param at_params Pointer to AT parameter list.
 * @param is_notif Indicates if the AT list is for a notification.
 * @param psm_cfg Pointer to where the PSM configuration is stored.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int parse_psm(struct at_param_list *at_params,
		  bool is_notif,
		  struct lte_lc_psm_cfg *psm_cfg);

/* @brief Parses an CEREG response and returns network registration status,
 *	  cell information, LTE mode and pSM configuration.
 *
 * @param at_response Pointer to buffer with AT response.
 * @param is_notif The buffer in at_response is a notification.
 * @param reg_status Pointer to where the registration status is stored.
 *		     Can be NULL.
 * @param cell Pointer to cell information struct. Can be NULL.
 * @param lte_mode Pointer to LTE mode struct. Can be NULL.
 * @param psm_cfg Pointer to PSM configuration struct. Can be NULL.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int parse_cereg(const char *at_response,
		bool is_notif,
		enum lte_lc_nw_reg_status *reg_status,
		struct lte_lc_cell *cell,
		enum lte_lc_lte_mode *lte_mode,
		struct lte_lc_psm_cfg *psm_cfg);
