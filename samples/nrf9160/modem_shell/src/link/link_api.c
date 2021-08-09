/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <assert.h>

#include <sys/types.h>
#include <shell/shell.h>

#include <modem/modem_info.h>

#include <posix/arpa/inet.h>
#include <net/net_ip.h>

#include "utils/net_utils.h"

#include "link_shell.h"
#include "link_shell_print.h"
#include "link_shell_pdn.h"
#include "link_api.h"

#if defined(CONFIG_AT_CMD)

#include <modem/at_cmd.h>
#include <modem/at_cmd_parser.h>
#include <modem/at_params.h>

#define AT_CMD_BUFFER_LEN (CONFIG_AT_CMD_RESPONSE_MAX_LEN + 1)
#define AT_CMD_PDP_CONTEXTS_READ "AT+CGDCONT?"
#define AT_CMD_PDP_CONTEXTS_READ_PARAM_COUNT 12
#define AT_CMD_PDP_CONTEXTS_READ_CID_INDEX 1
#define AT_CMD_PDP_CONTEXTS_READ_PDP_TYPE_INDEX 2
#define AT_CMD_PDP_CONTEXTS_READ_APN_INDEX 3
#define AT_CMD_PDP_CONTEXTS_READ_PDP_ADDR_INDEX 4

#define AT_CMD_PDP_CONTEXT_READ_INFO \
	"AT+CGCONTRDP=%d" /* Use sprintf to add CID into command */
#define AT_CMD_PDP_CONTEXT_READ_INFO_PARAM_COUNT 20
#define AT_CMD_PDP_CONTEXT_READ_INFO_CID_INDEX 1
#define AT_CMD_PDP_CONTEXT_READ_INFO_DNS_ADDR_PRIMARY_INDEX 6
#define AT_CMD_PDP_CONTEXT_READ_INFO_DNS_ADDR_SECONDARY_INDEX 7
#define AT_CMD_PDP_CONTEXT_READ_INFO_MTU_INDEX 12

#define AT_CMD_PDP_CONTEXT_READ_RSP_DELIM "\r\n"

extern const struct shell *shell_global;

#define AT_CMD_PDP_CONTEXTS_ACT_READ "AT+CGACT?"

static void link_api_context_info_fill_activation_status(
	struct pdp_context_info_array *pdp_info)
{
	char buf[16] = { 0 };
	const char *p;
	char at_response_str[256];
	int ret;
	int ctx_cnt = pdp_info->size;
	struct pdp_context_info *ctx_tbl = pdp_info->array;

	ret = at_cmd_write(AT_CMD_PDP_CONTEXTS_ACT_READ, at_response_str,
			   sizeof(at_response_str), NULL);
	if (ret) {
		shell_error(
			shell_global,
			"Cannot get PDP contexts activation states, err: %d",
			ret);
		return;
	}

	/* For each contexts: */
	for (int i = 0; i < ctx_cnt; i++) {
		/* Search for a string +CGACT: <cid>,<state> */
		snprintf(buf, sizeof(buf), "+CGACT: %d,1", ctx_tbl[i].cid);
		p = strstr(at_response_str, buf);
		if (p) {
			ctx_tbl[i].ctx_active = true;
		}
	}
}

/* ****************************************************************************/

static int link_api_pdn_id_get(uint8_t cid)
{
	int ret;
	char *p;
	char resp[32];
	static char at_buf[32];

	errno = 0;

	ret = snprintf(at_buf, sizeof(at_buf), "AT%%XGETPDNID=%u", cid);
	if (ret < 0 || ret >= sizeof(at_buf)) {
		return -ENOBUFS;
	}

	ret = at_cmd_write(at_buf, resp, sizeof(resp), NULL);
	if (ret) {
		return ret;
	}

	p = strchr(resp, ':');
	if (!p) {
		return -EBADMSG;
	}
	ret = strtoul(p + 1, NULL, 10);
	if (errno) {
		ret = -errno;
	}

	return ret;
}

/* ****************************************************************************/

struct pdp_context_info *link_api_get_pdp_context_info_by_pdn_cid(int pdn_cid)
{
	int ret;
	struct pdp_context_info_array pdp_context_info_tbl;
	struct pdp_context_info *pdp_context_info = NULL;

	ret = link_api_pdp_contexts_read(&pdp_context_info_tbl);
	if (ret) {
		shell_error(shell_global, "cannot read current connection info: %d", ret);
		return NULL;
	}

	/* Find PDP context info for the requested CID */
	for (int i = 0; i < pdp_context_info_tbl.size; i++) {
		if (pdp_context_info_tbl.array[i].cid == pdn_cid) {
			pdp_context_info = calloc(1, sizeof(struct pdp_context_info));
			memcpy(pdp_context_info,
			       &(pdp_context_info_tbl.array[i]),
			       sizeof(struct pdp_context_info));
			break;
		}
	}

	if (pdp_context_info_tbl.array != NULL) {
		free(pdp_context_info_tbl.array);
	}
	return pdp_context_info;
}

int link_api_pdp_context_dynamic_params_get(struct pdp_context_info *populated_info)
{
	int ret = 0;
	struct at_param_list param_list = { 0 };
	size_t param_str_len;
	char *next_param_str;
	bool resp_continues = false;

	char at_response_str[MOSH_AT_CMD_RESPONSE_MAX_LEN + 1];
	char *at_ptr = at_response_str;
	char *tmp_ptr = at_response_str;
	int lines = 0;
	int iterator = 0;
	char dns_addr_str[AT_CMD_PDP_CONTEXT_READ_IP_ADDR_STR_MAX_LEN];

	char at_cmd_pdp_context_read_info_cmd_str[15];

	int family;
	struct in_addr *addr;
	struct in6_addr *addr6;

	sprintf(at_cmd_pdp_context_read_info_cmd_str,
		AT_CMD_PDP_CONTEXT_READ_INFO, populated_info->cid);
	ret = at_cmd_write(at_cmd_pdp_context_read_info_cmd_str,
			   at_response_str, sizeof(at_response_str), NULL);
	if (ret) {
		shell_error(
			shell_global,
			"at_cmd_write returned err: %d for %s",
			ret,
			at_cmd_pdp_context_read_info_cmd_str);
		return ret;
	}

	/* Check how many rows of info do we have: */
	while ((tmp_ptr = strstr(tmp_ptr, AT_CMD_PDP_CONTEXT_READ_RSP_DELIM)) != NULL) {
		++tmp_ptr;
		++lines;
	}

	/* Parse the response */
	ret = at_params_list_init(&param_list, AT_CMD_PDP_CONTEXT_READ_INFO_PARAM_COUNT);
	if (ret) {
		shell_error(
			shell_global,
			"Could not init AT params list, error: %d\n",
			ret);
		return ret;
	}

parse:
	resp_continues = false;
	ret = at_parser_max_params_from_str(at_ptr, &next_param_str, &param_list, 13);
	if (ret == -EAGAIN) {
		resp_continues = true;
	} else if (ret == -E2BIG) {
		shell_error(shell_global, "E2BIG, error: %d\n", ret);
	} else if (ret != 0) {
		shell_error(
			shell_global, "Could not parse AT response for %s, error: %d\n",
			at_cmd_pdp_context_read_info_cmd_str,
			ret);
		goto clean_exit;
	}

	/* Read primary DNS address */
	param_str_len = sizeof(dns_addr_str);
	ret = at_params_string_get(
		&param_list,
		AT_CMD_PDP_CONTEXT_READ_INFO_DNS_ADDR_PRIMARY_INDEX,
		dns_addr_str, &param_str_len);
	if (ret) {
		shell_error(
			shell_global, "Could not parse dns str for cid %d, err: %d",
			populated_info->cid, ret);
		goto clean_exit;
	}
	dns_addr_str[param_str_len] = '\0';

	if (dns_addr_str != NULL) {
		family = net_utils_sa_family_from_ip_string(dns_addr_str);

		if (family == AF_INET) {
			addr = &(populated_info->dns_addr4_primary);
			(void)inet_pton(AF_INET, dns_addr_str, addr);
		} else if (family == AF_INET6) {
			addr6 = &(populated_info->dns_addr6_primary);
			(void)inet_pton(AF_INET6, dns_addr_str, addr6);
		}
	}

	/* Read secondary DNS address */
	param_str_len = sizeof(dns_addr_str);

	ret = at_params_string_get(
		&param_list,
		AT_CMD_PDP_CONTEXT_READ_INFO_DNS_ADDR_SECONDARY_INDEX,
		dns_addr_str, &param_str_len);
	if (ret) {
		shell_error(shell_global, "Could not parse dns str, err: %d", ret);
		goto clean_exit;
	}
	dns_addr_str[param_str_len] = '\0';

	if (dns_addr_str != NULL) {
		family = net_utils_sa_family_from_ip_string(dns_addr_str);

		if (family == AF_INET) {
			addr = &(populated_info->dns_addr4_secondary);
			(void)inet_pton(AF_INET, dns_addr_str, addr);
		} else if (family == AF_INET6) {
			addr6 = &(populated_info->dns_addr6_secondary);
			(void)inet_pton(AF_INET6, dns_addr_str, addr6);
		}
	}

	/* Read link MTU if exists: */
	ret = at_params_int_get(&param_list,
				AT_CMD_PDP_CONTEXT_READ_INFO_MTU_INDEX,
				&(populated_info->mtu));
	if (ret) {
		/* Don't care if it fails: */
		ret = 0;
		populated_info->mtu = 0;
	}

	if (resp_continues) {
		at_ptr = next_param_str;
		iterator++;
		assert(iterator < lines);
		goto parse;
	}

clean_exit:
	at_params_list_free(&param_list);

	return ret;
}

/* ****************************************************************************/

/** SNR offset value that is used when mapping to dBs  */
#define link_API_SNR_OFFSET_VALUE 25

void link_api_coneval_read_for_shell(const struct shell *shell)
{
	static const char * const coneval_result_strs[] = {
		"0: Connection evaluation successful",
		"1: Evaluation failed, no cell available",
		"2: Evaluation failed, UICC not available",
		"3: Evaluation failed, only barred cells available",
		"4: Evaluation failed, busy (e.g. GNSS activity)",
		"5: Evaluation failed, aborted because of higher priority operation",
		"6: Evaluation failed, not registered",
		"7: Evaluation failed, unspecified"
	};
	struct mapping_tbl_item const coneval_rrc_state_strs[] = {
		{ LTE_LC_RRC_MODE_IDLE,
		  "0: RRC connection in idle state during measurements" },
		{ LTE_LC_RRC_MODE_CONNECTED,
		  "1: RRC connection in connected state during measurements" },
		{ -1, NULL }
	};

	struct mapping_tbl_item const coneval_energy_est_strs[] = {
		{ LTE_LC_ENERGY_CONSUMPTION_EXCESSIVE,
		  "5: Energy estimate: -2, excessive energy consumption estimated" },
		{ LTE_LC_ENERGY_CONSUMPTION_INCREASED,
		  "6: Energy estimate: -1, slightly increased" },
		{ LTE_LC_ENERGY_CONSUMPTION_NORMAL,
		  "7: Energy estimate: 0, normal" },
		{ LTE_LC_ENERGY_CONSUMPTION_REDUCED,
		  "8: Energy estimate: +1, slightly reduced" },
		{ LTE_LC_ENERGY_CONSUMPTION_EFFICIENT,
		  "9: Energy estimate: +2, energy efficient transmission estimated" },
		{ -1, NULL }
	};

	struct mapping_tbl_item const coneval_tau_strs[] = {
		{ LTE_LC_CELL_IN_TAI_LIST,
		  "0: Evaluated cell is part of TAI list" },
		{ LTE_LC_CELL_NOT_IN_TAI_LIST,
		  "1: Evaluated cell is NOT part of TAI list, TAU will be triggered" },
		{ LTE_LC_CELL_UNKNOWN, "Unknown" },
		{ -1, NULL }
	};

	struct mapping_tbl_item const coneval_ce_level_strs[] = {
		{ LTE_LC_CE_LEVEL_0_NO_REPETITION,
		  "0: CE level 0, No repetitions or small nbr of repetitions" },
		{ LTE_LC_CE_LEVEL_1_LOW_REPETITION,
		  "1: CE level 1, Medium nbr of repetitions" },
		{ LTE_LC_CE_LEVEL_2_MEDIUM_REPETITION,
		  "2: CE level 2, Large nbr of repetitions" },
		{ 3, "3: CE level 3, Very large nbr of repetitions" },
		{ LTE_LC_CE_LEVEL_UNKNOWN, "Unknown" },
		{ -1, NULL }
	};
	struct lte_lc_conn_eval_params params;
	char snum[128];
	int ret;

	ret = lte_lc_conn_eval_params_get(&params);

	if (ret > 0) {
		shell_error(
			shell,
			"Cannot evaluate connection parameters, result: %s, ret %d",
			((ret <= 7) ? coneval_result_strs[ret] : "unknown"),
			ret);
		return;
	} else if (ret < 0) {
		shell_error(shell,
			    "lte_lc_conn_eval_params_get() API failed %d", ret);
		return;
	}

	shell_print(shell, "Evaluated connection parameters:");
	shell_print(shell, "  result:          %s",
		    ((ret <= 8) ? coneval_result_strs[ret] : "unknown"));
	shell_print(shell, "  rrc_state:       %s",
		    link_shell_map_to_string(coneval_rrc_state_strs,
					      params.rrc_state, snum));
	shell_print(shell, "  energy estimate: %s",
		    link_shell_map_to_string(coneval_energy_est_strs,
					      params.energy_estimate, snum));
	shell_print(shell, "  rsrp:            %d: %ddBm", params.rsrp,
		    (params.rsrp - MODEM_INFO_RSRP_OFFSET_VAL));
	shell_print(shell, "  rsrq:            %d", params.rsrq);

	shell_print(shell, "  snr:             %d: %ddB", params.snr,
		    (params.snr - link_API_SNR_OFFSET_VALUE));
	shell_print(shell, "  cell_id:         %d", params.cell_id);

	shell_print(shell, "  mcc/mnc:         %d/%d", params.mcc, params.mnc);
	shell_print(shell, "  phy_cell_id:     %d", params.phy_cid);
	shell_print(shell, "  earfcn:          %d", params.earfcn);
	shell_print(shell, "  band:            %d", params.band);
	shell_print(shell, "  tau_triggered:   %s",
		    link_shell_map_to_string(coneval_tau_strs, params.tau_trig,
					      snum));
	shell_print(shell, "  ce_level:        %s",
		    link_shell_map_to_string(coneval_ce_level_strs,
					      params.ce_level, snum));
	shell_print(shell, "  tx_power:        %d", params.tx_power);
	shell_print(shell, "  tx_repetitions:  %d", params.tx_rep);
	shell_print(shell, "  rx_repetitions:  %d", params.rx_rep);
	shell_print(shell, "  dl_pathloss:     %d", params.dl_pathloss);
}

/* ****************************************************************************/
#define XMONITOR_RESP_REG_STATUS_VALID 1
#define XMONITOR_RESP_FULL_NAME_VALID 2
#define XMONITOR_RESP_SHORT_NAME_VALID 4
#define XMONITOR_RESP_PLMN_VALID 8
#define XMONITOR_RESP_BAND_VALID 16
#define XMONITOR_RESP_CELL_ID_VALID 32
#define XMONITOR_RESP_RSRP_VALID 64
#define XMONITOR_RESP_SNR_VALID 128

#define OPERATOR_FULL_NAME_STR_MAX_LEN 124
#define OPERATOR_SHORT_NAME_STR_MAX_LEN 64
#define OPERATOR_CELL_ID_STR_MAX_LEN 32
#define OPERATOR_PLMN_STR_MAX_LEN 32

/* Note: not all stored / parsed from xmonitor response */
struct lte_xmonitor_resp_t {
	uint8_t reg_status;
	uint8_t band;
	int16_t rsrp;
	char validity_bits;
	int8_t snr;
	char full_name_str[OPERATOR_FULL_NAME_STR_MAX_LEN + 1];
	char short_name_str[OPERATOR_SHORT_NAME_STR_MAX_LEN + 1];
	char plmn_str[OPERATOR_PLMN_STR_MAX_LEN + 1];
	char cell_id_str[OPERATOR_CELL_ID_STR_MAX_LEN + 1];
};

#define AT_CMD_XMONITOR "AT%XMONITOR"
#define AT_CMD_XMONITOR_RESP_PARAM_COUNT 17

#define AT_CMD_XMONITOR_RESP_REG_STATUS_INDEX 1
#define AT_CMD_XMONITOR_RESP_FULL_NAME_INDEX 2
#define AT_CMD_XMONITOR_RESP_SHORT_NAME_INDEX 3
#define AT_CMD_XMONITOR_RESP_PLMN_INDEX 4
#define AT_CMD_XMONITOR_RESP_BAND_INDEX 7
#define AT_CMD_XMONITOR_RESP_CELL_ID_INDEX 8
#define AT_CMD_XMONITOR_RESP_RSRP_INDEX 11
#define AT_CMD_XMONITOR_RESP_SNR_INDEX 12

#define AT_CMD_X_MONITOR_MAX_HANDLED_INDEX AT_CMD_XMONITOR_RESP_SNR_INDEX
#define AT_CMD_XMONITOR_RESP_MAX_STR_LEN OPERATOR_FULL_NAME_STR_MAX_LEN

static int link_api_xmonitor_read(struct lte_xmonitor_resp_t *resp)
{
	int ret = 0;
	int i;
	int value;
	struct at_param_list param_list = { 0 };
	char at_response_str[512] = { 0 };
	char str_buf[OPERATOR_FULL_NAME_STR_MAX_LEN + 1];
	int len = sizeof(str_buf);

	memset(resp, 0, sizeof(struct lte_xmonitor_resp_t));

	ret = at_cmd_write(AT_CMD_XMONITOR, at_response_str,
			   sizeof(at_response_str), NULL);
	if (ret) {
		shell_error(
			shell_global, "at_cmd_write for \"%s\" returned err: %d",
			AT_CMD_XMONITOR, ret);
		return ret;
	}

	ret = at_params_list_init(&param_list,
				  AT_CMD_XMONITOR_RESP_PARAM_COUNT);
	if (ret) {
		shell_error(
			shell_global, "Could not init AT params list for \"%s\", error: %d",
			AT_CMD_XMONITOR, ret);
		return ret;
	}

	ret = at_parser_params_from_str(at_response_str, NULL, &param_list);
	if (ret) {
		shell_error(
			shell_global, "Could not parse %s response, error: %d",
			AT_CMD_XMONITOR, ret);
		goto clean_exit;
	}

	for (i = 1; i <= AT_CMD_X_MONITOR_MAX_HANDLED_INDEX; i++) {
		if (i == AT_CMD_XMONITOR_RESP_FULL_NAME_INDEX ||
		    i == AT_CMD_XMONITOR_RESP_SHORT_NAME_INDEX ||
		    i == AT_CMD_XMONITOR_RESP_PLMN_INDEX ||
		    i == AT_CMD_XMONITOR_RESP_CELL_ID_INDEX) {

			len = sizeof(str_buf);
			ret = at_params_string_get(&param_list, i, str_buf, &len);
			if (ret) {
				/* Invalid AT string resp parameter at given index */
				continue;
			}
			assert(len <= AT_CMD_XMONITOR_RESP_MAX_STR_LEN);
			str_buf[len] = '\0';

			if (i == AT_CMD_XMONITOR_RESP_CELL_ID_INDEX) {
				strcpy(resp->cell_id_str, str_buf);
				if (strlen(resp->cell_id_str)) {
					resp->validity_bits |= XMONITOR_RESP_CELL_ID_VALID;
				}
			} else if (i == AT_CMD_XMONITOR_RESP_PLMN_INDEX) {
				strcpy(resp->plmn_str, str_buf);
				if (strlen(resp->plmn_str)) {
					resp->validity_bits |= XMONITOR_RESP_PLMN_VALID;
				}
			} else if (i == AT_CMD_XMONITOR_RESP_FULL_NAME_INDEX) {
				strcpy(resp->full_name_str, str_buf);
				if (strlen(resp->full_name_str)) {
					resp->validity_bits |= XMONITOR_RESP_FULL_NAME_VALID;
				}
			} else {
				assert(i == AT_CMD_XMONITOR_RESP_SHORT_NAME_INDEX);
				strcpy(resp->short_name_str, str_buf);
				if (strlen(resp->short_name_str)) {
					resp->validity_bits |= XMONITOR_RESP_SHORT_NAME_VALID;
				}
			}
		} else if (i == AT_CMD_XMONITOR_RESP_REG_STATUS_INDEX ||
			   i == AT_CMD_XMONITOR_RESP_BAND_INDEX ||
			   i == AT_CMD_XMONITOR_RESP_RSRP_INDEX ||
			   i == AT_CMD_XMONITOR_RESP_SNR_INDEX) {
			ret = at_params_int_get(&param_list, i, &value);
			if (ret) {
				/* Invalid AT int resp parameter at given index */
				continue;
			}

			switch (i) {
			case AT_CMD_XMONITOR_RESP_REG_STATUS_INDEX:
				resp->reg_status = value;
				resp->validity_bits |= XMONITOR_RESP_REG_STATUS_VALID;
				break;
			case AT_CMD_XMONITOR_RESP_BAND_INDEX:
				resp->band = value;
				resp->validity_bits |= XMONITOR_RESP_BAND_VALID;
				break;
			case AT_CMD_XMONITOR_RESP_RSRP_INDEX:
				resp->rsrp = value;
				resp->validity_bits |= XMONITOR_RESP_RSRP_VALID;
				break;
			case AT_CMD_XMONITOR_RESP_SNR_INDEX:
				resp->snr = value;
				resp->validity_bits |= XMONITOR_RESP_SNR_VALID;
				break;
			}
		}
	}

clean_exit:
	at_params_list_free(&param_list);
	return 0;
}

static void link_api_modem_operator_info_read_for_shell(const struct shell *shell)
{
	struct lte_xmonitor_resp_t xmonitor_resp;
	int ret = link_api_xmonitor_read(&xmonitor_resp);
	int cell_id;

	if (ret) {
		shell_error(shell, "Operation failed, result: ret %d", ret);
		return;
	}

	if (xmonitor_resp.validity_bits & XMONITOR_RESP_FULL_NAME_VALID) {
		shell_print(shell, "Operator full name:   \"%s\"",
			    xmonitor_resp.full_name_str);
	}

	if (xmonitor_resp.validity_bits & XMONITOR_RESP_SHORT_NAME_VALID) {
		shell_print(shell, "Operator short name:  \"%s\"",
			    xmonitor_resp.short_name_str);
	}

	if (xmonitor_resp.validity_bits & XMONITOR_RESP_PLMN_VALID) {
		shell_print(shell, "Operator PLMN:        \"%s\"",
			    xmonitor_resp.plmn_str);
	}

	if (xmonitor_resp.validity_bits & XMONITOR_RESP_CELL_ID_VALID) {
		cell_id = strtol(xmonitor_resp.cell_id_str, NULL, 16);
		shell_print(shell, "Current cell id:       %d (0x%s)", cell_id,
			    xmonitor_resp.cell_id_str);
	}

	if (xmonitor_resp.validity_bits & XMONITOR_RESP_BAND_VALID) {
		shell_print(shell, "Current band:          %d",
			    xmonitor_resp.band);
	}

	if (xmonitor_resp.validity_bits & XMONITOR_RESP_RSRP_VALID) {
		shell_print(shell, "Current rsrp:          %d: %ddBm",
			    xmonitor_resp.rsrp,
			    (xmonitor_resp.rsrp - MODEM_INFO_RSRP_OFFSET_VAL));
	}

	if (xmonitor_resp.validity_bits & XMONITOR_RESP_SNR_VALID) {
		shell_print(shell, "Current snr:           %d: %ddB",
			    xmonitor_resp.snr,
			    (xmonitor_resp.snr - link_API_SNR_OFFSET_VALUE));
	}
}

/* ****************************************************************************/

int link_api_pdp_contexts_read(struct pdp_context_info_array *pdp_info)
{
	int ret = 0;
	struct at_param_list param_list = { 0 };
	size_t param_str_len;
	char *next_param_str;
	bool resp_continues = false;

	char at_response_str[MOSH_AT_CMD_RESPONSE_MAX_LEN + 1];
	char *at_ptr = at_response_str;
	char *tmp_ptr = at_response_str;
	int pdp_cnt = 0;
	int iterator = 0;
	struct pdp_context_info *populated_info;

	char ip_addr_str[AT_CMD_PDP_CONTEXT_READ_IP_ADDR_STR_MAX_LEN];
	char *ip_address1, *ip_address2;
	int family;
	struct in_addr *addr4;
	struct in6_addr *addr6;

	memset(pdp_info, 0, sizeof(struct pdp_context_info_array));

	ret = at_cmd_write(AT_CMD_PDP_CONTEXTS_READ, at_response_str,
			   sizeof(at_response_str), NULL);
	if (ret) {
		shell_error(shell_global, "at_cmd_write returned err: %d", ret);
		return ret;
	}

	/* Check how many rows/context do we have: */
	while ((tmp_ptr = strstr(tmp_ptr, AT_CMD_PDP_CONTEXT_READ_RSP_DELIM)) != NULL) {
		++tmp_ptr;
		++pdp_cnt;
	}

	/* Allocate array of PDP info accordingly: */
	pdp_info->array = calloc(pdp_cnt, sizeof(struct pdp_context_info));
	pdp_info->size = pdp_cnt;

	/* Parse the response */
	ret = at_params_list_init(&param_list, AT_CMD_PDP_CONTEXTS_READ_PARAM_COUNT);
	if (ret) {
		shell_error(shell_global, "Could not init AT params list, error: %d\n", ret);
		return ret;
	}
	populated_info = pdp_info->array;

parse:
	resp_continues = false;
	ret = at_parser_max_params_from_str(
		at_ptr, &next_param_str, &param_list,
		AT_CMD_PDP_CONTEXTS_READ_PARAM_COUNT);
	if (ret == -EAGAIN) {
		resp_continues = true;
	} else if (ret != 0 && ret != -EAGAIN) {
		shell_error(shell_global, "Could not parse AT response, error: %d", ret);
		goto clean_exit;
	}

	ret = at_params_int_get(&param_list,
				AT_CMD_PDP_CONTEXTS_READ_CID_INDEX,
				&populated_info[iterator].cid);
	if (ret) {
		shell_error(shell_global, "Could not parse CID, err: %d", ret);
		goto clean_exit;
	}
	ret = link_api_pdn_id_get(populated_info[iterator].cid);
	if (ret < 0) {
		shell_error(
			shell_global, "Could not get PDN for CID %d, err: %d\n",
			populated_info[iterator].cid, ret);
	} else {
		populated_info[iterator].pdn_id_valid = true;
		populated_info[iterator].pdn_id = ret;
	}

	param_str_len = sizeof(populated_info[iterator].pdp_type_str);
	ret = at_params_string_get(
		&param_list, AT_CMD_PDP_CONTEXTS_READ_PDP_TYPE_INDEX,
		populated_info[iterator].pdp_type_str, &param_str_len);
	if (ret) {
		shell_error(shell_global, "Could not parse pdp type, err: %d", ret);
		goto clean_exit;
	}
	populated_info[iterator].pdp_type_str[param_str_len] = '\0';

	populated_info[iterator].pdp_type = PDP_TYPE_UNKNOWN;
	if (strcmp(populated_info[iterator].pdp_type_str, "IPV4V6") == 0) {
		populated_info[iterator].pdp_type = PDP_TYPE_IP4V6;
	} else if (strcmp(populated_info[iterator].pdp_type_str, "IPV6") == 0) {
		populated_info[iterator].pdp_type = PDP_TYPE_IPV6;
	} else if (strcmp(populated_info[iterator].pdp_type_str, "IP") == 0) {
		populated_info[iterator].pdp_type = PDP_TYPE_IPV4;
	} else if (strcmp(populated_info[iterator].pdp_type_str, "Non-IP") == 0) {
		populated_info[iterator].pdp_type = PDP_TYPE_NONIP;
	}

	param_str_len = sizeof(populated_info[iterator].apn_str);
	ret = at_params_string_get(
		&param_list,
		AT_CMD_PDP_CONTEXTS_READ_APN_INDEX,
		populated_info[iterator].apn_str,
		&param_str_len);
	if (ret) {
		shell_error(shell_global, "Could not parse apn str, err: %d", ret);
		goto clean_exit;
	}
	populated_info[iterator].apn_str[param_str_len] = '\0';

	param_str_len = sizeof(ip_addr_str);
	ret = at_params_string_get(
		&param_list, AT_CMD_PDP_CONTEXTS_READ_PDP_ADDR_INDEX,
		ip_addr_str, &param_str_len);
	if (ret) {
		shell_error(shell_global, "Could not parse apn str, err: %d", ret);
		goto clean_exit;
	}
	ip_addr_str[param_str_len] = '\0';

	/* Parse IP addresses from space delimited string. */

	/* Get 1st 2 IP addresses from a CGDCONT string.
	 * Notice that ip_addr_str is slightly modified by strtok()
	 */
	ip_address1 = strtok(ip_addr_str, " ");
	ip_address2 = strtok(NULL, " ");

	if (ip_address1 != NULL) {
		family = net_utils_sa_family_from_ip_string(ip_address1);
		if (family == AF_INET) {
			struct in_addr *addr4 = &populated_info[iterator].ip_addr4;
			(void)inet_pton(AF_INET, ip_address1, addr4);
		} else if (family == AF_INET6) {
			struct in6_addr *addr6 = &populated_info[iterator].ip_addr6;

			(void)inet_pton(AF_INET6, ip_address1, addr6);
		}
	}
	if (ip_address2 != NULL) {
		/* Note: If we are here, PDP_addr_2 should be IPv6,
		 * thus in following ipv4 branch should not be possible:
		 */
		family = net_utils_sa_family_from_ip_string(ip_address2);
		if (family == AF_INET) {
			addr4 = &populated_info[iterator].ip_addr4;
			(void)inet_pton(AF_INET, ip_address2, addr4);
		} else if (family == AF_INET6) {
			addr6 = &populated_info[iterator].ip_addr6;
			(void)inet_pton(AF_INET6, ip_address2, addr6);
		}
	}

	/* Get DNS addresses etc.  for this IP context: */
	if (populated_info[iterator].pdp_type != PDP_TYPE_NONIP) {
		(void)link_api_pdp_context_dynamic_params_get(&(populated_info[iterator]));
	}

	if (resp_continues) {
		at_ptr = next_param_str;
		iterator++;
		assert(iterator < pdp_cnt);
		goto parse;
	}

	/* ...and finally, fill PDP context activation status for each: */
	link_api_context_info_fill_activation_status(pdp_info);

clean_exit:
	at_params_list_free(&param_list);
	/* user need do free pdp_info->array also in case of error */

	return ret;
}
#endif /* CONFIG_AT_CMD */

/* *****************************************************************************/

void link_api_modem_info_get_for_shell(const struct shell *shell, bool connected)
{
	struct pdp_context_info_array pdp_context_info_tbl;
	enum lte_lc_system_mode sys_mode_current;
	enum lte_lc_system_mode_preference sys_mode_preferred;
	enum lte_lc_lte_mode currently_active_mode;
	char info_str[MODEM_INFO_MAX_RESPONSE_SIZE + 1];
	int ret;

	(void)link_shell_get_and_print_current_system_modes(
		shell, &sys_mode_current, &sys_mode_preferred,
		&currently_active_mode);

	ret = modem_info_string_get(MODEM_INFO_FW_VERSION, info_str, sizeof(info_str));
	if (ret >= 0) {
		shell_print(shell, "Modem FW version:      %s", info_str);
	} else {
		shell_error(shell, "Unable to obtain modem FW version (%d)", ret);
	}

	if (connected) {
		link_api_modem_operator_info_read_for_shell(shell);

		ret = modem_info_string_get(MODEM_INFO_DATE_TIME, info_str, sizeof(info_str));
		if (ret >= 0) {
			shell_print(shell, "Mobile network time and date: %s", info_str);
		}

#if defined(CONFIG_AT_CMD)
		ret = link_api_pdp_contexts_read(&pdp_context_info_tbl);
		if (ret >= 0) {
			char ipv4_addr[NET_IPV4_ADDR_LEN];
			char ipv6_addr[NET_IPV6_ADDR_LEN];
			char ipv4_dns_addr_primary[NET_IPV4_ADDR_LEN];
			char ipv4_dns_addr_secondary[NET_IPV4_ADDR_LEN];
			char ipv6_dns_addr_primary[NET_IPV6_ADDR_LEN];
			char ipv6_dns_addr_secondary[NET_IPV6_ADDR_LEN];
			char tmp_str[12];

			int i = 0;
			struct pdp_context_info *info_tbl =
				pdp_context_info_tbl.array;

			for (i = 0; i < pdp_context_info_tbl.size; i++) {
				inet_ntop(AF_INET, &(info_tbl[i].ip_addr4),
					  ipv4_addr, sizeof(ipv4_addr));
				inet_ntop(AF_INET6, &(info_tbl[i].ip_addr6),
					  ipv6_addr, sizeof(ipv6_addr));

				inet_ntop(AF_INET,
					  &(info_tbl[i].dns_addr4_primary),
					  ipv4_dns_addr_primary,
					  sizeof(ipv4_dns_addr_primary));
				inet_ntop(AF_INET,
					  &(info_tbl[i].dns_addr4_secondary),
					  ipv4_dns_addr_secondary,
					  sizeof(ipv4_dns_addr_secondary));

				inet_ntop(AF_INET6,
					  &(info_tbl[i].dns_addr6_primary),
					  ipv6_dns_addr_primary,
					  sizeof(ipv6_dns_addr_primary));
				inet_ntop(AF_INET6,
					  &(info_tbl[i].dns_addr6_secondary),
					  ipv6_dns_addr_secondary,
					  sizeof(ipv6_dns_addr_secondary));

				if (info_tbl[i].pdn_id_valid) {
					sprintf(tmp_str, "%d",
						info_tbl[i].pdn_id);
				}

				/* Parsed PDP context info: */
				shell_print(shell,
					    "PDP context info %d:\n"
					    "  CID:                    %d\n"
					    "  PDN ID:                 %s\n"
					    "  PDP context active:     %s\n"
					    "  PDP type:               %s\n"
					    "  APN:                    %s\n"
					    "  IPv4 MTU:               %d\n"
					    "  IPv4 address:           %s\n"
					    "  IPv6 address:           %s\n"
					    "  IPv4 DNS address:       %s, %s\n"
					    "  IPv6 DNS address:       %s, %s",
					    (i + 1), info_tbl[i].cid,
					    (info_tbl[i].pdn_id_valid) ?
					    tmp_str :
					    "Not known",
					    (info_tbl[i].ctx_active) ? "yes" :
					    "no",
					    info_tbl[i].pdp_type_str,
					    info_tbl[i].apn_str,
					    info_tbl[i].mtu, ipv4_addr,
					    ipv6_addr, ipv4_dns_addr_primary,
					    ipv4_dns_addr_secondary,
					    ipv6_dns_addr_primary,
					    ipv6_dns_addr_secondary);
			}
		} else {
			shell_error(shell, "Unable to obtain pdp context info (%d)", ret);
		}
		if (pdp_context_info_tbl.array != NULL) {
			free(pdp_context_info_tbl.array);
		}
#endif /* CONFIG_AT_CMD */
	}
}

#define AT_CMD_RAI "AT%RAI?"
#define AT_CMD_RAI_RESP_PARAM_COUNT 2
#define AT_CMD_RAI_RESP_ENABLE_INDEX 1

int link_api_rai_status(bool *rai_status)
{
	enum at_cmd_state state = AT_CMD_ERROR;
	struct at_param_list param_list = { 0 };
	char at_response_str[20];
	int err;
	int status;

	if (rai_status == NULL) {
		return -EINVAL;
	}
	*rai_status = false;

	err = at_cmd_write(AT_CMD_RAI, at_response_str, sizeof(at_response_str), &state);
	if (state != AT_CMD_OK) {
		shell_error(shell_global, "Error state=%d, error=%d", state, err);
		return -EFAULT;
	}

	err = at_params_list_init(&param_list, AT_CMD_RAI_RESP_PARAM_COUNT);
	if (err) {
		shell_error(
			shell_global,
			"Could not init AT params list for \"%s\", error: %d",
			AT_CMD_RAI, err);
		return err;
	}

	err = at_parser_params_from_str(at_response_str, NULL, &param_list);
	if (err) {
		shell_error(
			shell_global,
			"Could not parse %s response, error: %d",
			AT_CMD_RAI, err);
		goto clean_exit;
	}

	err = at_params_int_get(&param_list, AT_CMD_RAI_RESP_ENABLE_INDEX, &status);
	if (err) {
		/* Invalid AT int resp parameter at given index */
		shell_error(
			shell_global,
			"Could not find int parameter index=%d from response=%s, error: %d",
			AT_CMD_RAI_RESP_ENABLE_INDEX, at_response_str, err);
		goto clean_exit;
	}

	*rai_status = (bool)status;

clean_exit:
	at_params_list_free(&param_list);
	return err;
}
