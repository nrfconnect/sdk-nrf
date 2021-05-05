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

/* XT3412 command parameters */
#define AT_XT3412_SUB				"AT%%XT3412=1,%d,%d"
#define AT_XT3412_PARAMS_COUNT_MAX		4
#define AT_XT3412_TIME_INDEX			2
#define T3412_MAX				35712000000

/* NCELLMEAS notification parameters */
#define AT_NCELLMEAS_RESPONSE_PREFIX		"%NCELLMEAS"
#define AT_NCELLMEAS_START			"AT%NCELLMEAS"
#define AT_NCELLMEAS_STOP			"AT%NCELLMEASSTOP"
#define AT_NCELLMEAS_STATUS_INDEX		1
#define AT_NCELLMEAS_STATUS_VALUE_SUCCESS	0
#define AT_NCELLMEAS_STATUS_VALUE_FAIL		1
#define AT_NCELLMEAS_CELL_ID_INDEX		2
#define AT_NCELLMEAS_PLMN_INDEX			3
#define AT_NCELLMEAS_TAC_INDEX			4
#define AT_NCELLMEAS_TIMING_ADV_INDEX		5
#define AT_NCELLMEAS_EARFCN_INDEX		6
#define AT_NCELLMEAS_PHYS_CELL_ID_INDEX		7
#define AT_NCELLMEAS_RSRP_INDEX			8
#define AT_NCELLMEAS_RSRQ_INDEX			9
#define AT_NCELLMEAS_MEASUREMENT_TIME_INDEX	10
#define AT_NCELLMEAS_PRE_NCELLS_PARAMS_COUNT	11
/* The rest of the parameters are in repeating arrays per neighboring cell.
 * The indices below refer to their index within such a repeating array.
 */
#define AT_NCELLMEAS_N_EARFCN_INDEX		0
#define AT_NCELLMEAS_N_PHYS_CELL_ID_INDEX	1
#define AT_NCELLMEAS_N_RSRP_INDEX		2
#define AT_NCELLMEAS_N_RSRQ_INDEX		3
#define AT_NCELLMEAS_N_TIME_DIFF_INDEX		4
#define AT_NCELLMEAS_N_PARAMS_COUNT		5
#define AT_NCELLMEAS_N_MAX_ARRAY_SIZE		CONFIG_LTE_NEIGHBOR_CELLS_MAX

#define AT_NCELLMEAS_PARAMS_COUNT_MAX					\
	(AT_NCELLMEAS_PRE_NCELLS_PARAMS_COUNT +				\
	 AT_NCELLMEAS_N_PARAMS_COUNT * CONFIG_LTE_NEIGHBOR_CELLS_MAX)

/* XMODEMSLEEP command parameters. */
#define AT_XMODEMSLEEP_SUB			"AT%%XMODEMSLEEP=1,%d,%d"
#define AT_XMODEMSLEEP_PARAMS_COUNT_MAX		4
#define AT_XMODEMSLEEP_TYPE_INDEX		1
#define AT_XMODEMSLEEP_TIME_INDEX		2

/* CONEVAL command parameters */
#define AT_CONEVAL_READ				"AT%CONEVAL"
#define AT_CONEVAL_RESPONSE_PREFIX		"%CONEVAL"
#define AT_CONEVAL_PREFIX_INDEX			0
#define AT_CONEVAL_RESPONSE_MAX_LEN		110
#define AT_CONEVAL_PARAMS_MAX			19
#define AT_CONEVAL_RESULT_INDEX			1
#define AT_CONEVAL_RRC_STATE_INDEX		2
#define AT_CONEVAL_ENERGY_ESTIMATE_INDEX	3
#define AT_CONEVAL_RSRP_INDEX			4
#define AT_CONEVAL_RSRQ_INDEX			5
#define AT_CONEVAL_SNR_INDEX			6
#define AT_CONEVAL_CELL_ID_INDEX		7
#define AT_CONEVAL_PLMN_INDEX			8
#define AT_CONEVAL_PHYSICAL_CELL_ID_INDEX	9
#define AT_CONEVAL_EARFCN_INDEX			10
#define AT_CONEVAL_BAND_INDEX			11
#define AT_CONEVAL_TAU_TRIGGERED_INDEX		12
#define AT_CONEVAL_CE_LEVEL_INDEX		13
#define AT_CONEVAL_TX_POWER_INDEX		14
#define AT_CONEVAL_TX_REPETITIONS_INDEX		15
#define AT_CONEVAL_RX_REPETITIONS_INDEX		16
#define AT_CONEVAL_DL_PATHLOSS_INDEX		17

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

/* @brief Parses an XT3412 response and extracts the time until next TAU.
 *
 * @param at_response Pointer to buffer with AT response.
 * @param time Pointer to integer that the time until next TAU will be written to.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int parse_xt3412(const char *at_response, uint64_t *time);

/* @brief Get the number of neighboring cells reported in an NCELLMEAS response.
 *
 * @param at_response Pointer to buffer with AT response to parse.
 *
 * @return The number of neighbor cells found in the response.
 */
uint32_t neighborcell_count_get(const char *at_response);

/* @brief Parses an NCELLMEAS notification and stores neighboring cell
 *	  information in a struct.
 *
 * @param at_response Pointer to buffer with AT response.
 * @param ncell Pointer to ncell structure.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int parse_ncellmeas(const char *at_response, struct lte_lc_cells_info *cells);

/* @brief Parses an XMODEMSLEEP response and extracts the sleep type and time.
 *
 * @note If the time parameter -1 after this API call, time shall be considered infinite.
 *
 * @param at_response Pointer to buffer with AT response.
 * @param modem_sleep Pointer to a structure holding modem sleep data.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int parse_xmodemsleep(const char *at_response, struct lte_lc_modem_sleep *modem_sleep);

/* @brief Parses a CONEVAL response and populates a struct with parameters from the response.
 *
 * @param at_response Pointer to buffer with AT response.
 * @param params Pointer to a structure that will be populated with CONEVAL parameters.
 *
 * @return Zero on success, negative errno code if the API call fails, and a positive error
 *         code if the API call succeeds but connection evalution fails due to modem/network related
 *         reasons.
 *
 * @retval 0 Evaluation succeeded.
 * @retval 1 Evaluation failed, no cell available.
 * @retval 2 Evaluation failed, UICC not available.
 * @retval 3 Evaluation failed, only barred cells available.
 * @retval 4 Evaluation failed, radio busy (e.g GNSS activity)
 * @retval 5 Evaluation failed, aborted due to higher priority operation.
 * @retval 6 Evaluation failed, UE not registered to network.
 * @retval 7 Evaluation failed, Unspecified.
 */
int parse_coneval(const char *at_response, struct lte_lc_conn_eval_params *params);
