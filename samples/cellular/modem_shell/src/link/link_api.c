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
#include <zephyr/shell/shell.h>

#include <modem/modem_info.h>

#include <zephyr/posix/arpa/inet.h>
#include <zephyr/net/net_ip.h>

#include "utils/net_utils.h"
#include "mosh_print.h"

#include "link_shell.h"
#include "link_shell_print.h"
#include "link_shell_pdn.h"
#include "link_api.h"

#include <nrf_modem_at.h>
#include <net/nrf_cloud.h>
#include <modem/at_cmd_parser.h>
#include <modem/at_params.h>

extern char mosh_at_resp_buf[MOSH_AT_CMD_RESPONSE_MAX_LEN];
extern struct k_mutex mosh_at_resp_buf_mutex;

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

/* ****************************************************************************/

static void link_api_context_info_fill_activation_status(
	struct pdp_context_info_array *pdp_info)
{
	char buf[16] = { 0 };
	const char *p;
	/* Cannot use global mosh_at_resp_buf because this used from link_api_pdp_contexts_read()
	 * where global mosh_at_resp_buf is already used
	 */
	char at_response_str[256];
	int ret;
	int ctx_cnt = pdp_info->size;
	struct pdp_context_info *ctx_tbl = pdp_info->array;

	ret = nrf_modem_at_cmd(at_response_str, sizeof(at_response_str), "AT+CGACT?");
	if (ret) {
		mosh_error("Cannot get PDP contexts activation states, err: %d", ret);
		return;
	}

	/* For each contexts */
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

struct pdp_context_info *link_api_get_pdp_context_info_by_pdn_cid(int pdn_cid)
{
	int ret;
	struct pdp_context_info_array pdp_context_info_tbl;
	struct pdp_context_info *pdp_context_info = NULL;

	ret = link_api_pdp_contexts_read(&pdp_context_info_tbl);
	if (ret) {
		mosh_error("cannot read current connection info: %d", ret);
		goto exit;
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

exit:
	if (pdp_context_info_tbl.array != NULL) {
		free(pdp_context_info_tbl.array);
	}
	return pdp_context_info;
}

/* ****************************************************************************/

void link_api_coneval_read_for_shell(void)
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
		{ LTE_LC_CE_LEVEL_0,
		  "0: CE level 0, No repetitions or small nbr of repetitions" },
		{ LTE_LC_CE_LEVEL_1,
		  "1: CE level 1, Medium nbr of repetitions" },
		{ LTE_LC_CE_LEVEL_2,
		  "2: CE level 2, Large nbr of repetitions" },
		{ LTE_LC_CE_LEVEL_3,
		  "3: CE level 3, Very large nbr of repetitions" },
		{ LTE_LC_CE_LEVEL_UNKNOWN, "Unknown" },
		{ -1, NULL }
	};
	struct lte_lc_conn_eval_params params;
	char snum[128];
	int ret;

	ret = lte_lc_conn_eval_params_get(&params);

	if (ret > 0) {
		mosh_error(
			"Cannot evaluate connection parameters, result: %s, ret %d",
			((ret <= 7) ? coneval_result_strs[ret] : "unknown"),
			ret);
		return;
	} else if (ret < 0) {
		mosh_error("lte_lc_conn_eval_params_get() API failed %d", ret);
		return;
	}

	mosh_print("Evaluated connection parameters:");
	mosh_print("  result:          %s",
			((ret <= 8) ? coneval_result_strs[ret] : "unknown"));
	mosh_print("  rrc_state:       %s",
			link_shell_map_to_string(coneval_rrc_state_strs, params.rrc_state, snum));
	mosh_print("  energy estimate: %s",
			link_shell_map_to_string(coneval_energy_est_strs,
				params.energy_estimate, snum));
	mosh_print("  rsrp:            %d: %ddBm", params.rsrp,
			RSRP_IDX_TO_DBM(params.rsrp));
	mosh_print("  rsrq:            %d", params.rsrq);

	mosh_print("  snr:             %d: %ddB", params.snr,
			(params.snr - LINK_SNR_OFFSET_VALUE));
	mosh_print("  cell_id:         %d", params.cell_id);

	mosh_print("  mcc/mnc:         %d/%d", params.mcc, params.mnc);
	mosh_print("  phy_cell_id:     %d", params.phy_cid);
	mosh_print("  earfcn:          %d", params.earfcn);
	mosh_print("  band:            %d", params.band);
	mosh_print("  tau_triggered:   %s",
			link_shell_map_to_string(coneval_tau_strs, params.tau_trig, snum));
	mosh_print("  ce_level:        %s",
			link_shell_map_to_string(coneval_ce_level_strs, params.ce_level, snum));
	mosh_print("  tx_power:        %d", params.tx_power);
	mosh_print("  tx_repetitions:  %d", params.tx_rep);
	mosh_print("  rx_repetitions:  %d", params.rx_rep);
	mosh_print("  dl_pathloss:     %d", params.dl_pathloss);
}

/* ****************************************************************************/

/* For having a numeric constant in scanf string length */
#define L(x) STRINGIFY(x)

int link_api_xmonitor_read(struct lte_xmonitor_resp_t *resp)
{
	int ret = 0;
	int len;
	char tmp_cell_id_str[OP_CELL_ID_STR_MAX_LEN + 1] = { 0 };
	char tmp_tac_str[OP_TAC_STR_MAX_LEN + 1] = { 0 };

	memset(resp, 0, sizeof(struct lte_xmonitor_resp_t));

	/* All might not be included in a response if not camping on a cell,
	 * thus marking these specifically
	 */
	resp->rsrp = -1;
	resp->snr = -1;
	resp->pci = -1;
	resp->cell_id = -1;
	resp->tac = -1;

	ret = nrf_modem_at_scanf("AT%XMONITOR",
				 "%%XMONITOR: "
				 "%*u,"                                /* <reg_status>: ignored */
				 "%"L(OP_FULL_NAME_STR_MAX_LEN)"[^,]," /* <full_name> with quotes */
				 "%"L(OP_SHORT_NAME_STR_MAX_LEN)"[^,],"/* <short_name> with quotes*/
				 "%"L(OP_PLMN_STR_MAX_LEN)"[^,],"      /* <plmn> */
				 "%"L(OP_TAC_STR_MAX_LEN)"[^,],"       /* <tac> with quotes */
				 "%*d,"                                /* <AcT>: ignored */
				 "%u,"                                 /* <band> */
				 "%"L(OP_CELL_ID_STR_MAX_LEN)"[^,],"   /* <cell_id> with quotes */
				 "%u,"                                 /* <phys_cell_id> */
				 "%*u,"                                /* <EARFCN>: ignored */
				 "%d,"                                 /* <rsrp> */
				 "%d",                                 /* <snr> */
					resp->full_name_str,
					resp->short_name_str,
					resp->plmn_str,
					tmp_tac_str,
					&resp->band,
					tmp_cell_id_str,
					&resp->pci,
					&resp->rsrp,
					&resp->snr);
	if (ret < 0) {
		/* We don't want to print shell error from here, that should
		 * be done in a caller.
		 */
		return ret;
	}

	/* Strip out the quotes and convert to decimal */
	len = strlen(tmp_cell_id_str);
	if (len > 2 && len <= OP_CELL_ID_STR_MAX_LEN) {
		strncpy(resp->cell_id_str, tmp_cell_id_str + 1, len - 2);
		resp->cell_id = strtol(resp->cell_id_str, NULL, 16);
	}

	len = strlen(tmp_tac_str);
	if (len > 2 && len <= OP_TAC_STR_MAX_LEN) {
		strncpy(resp->tac_str, tmp_tac_str + 1, len - 2);
		resp->tac = strtol(resp->tac_str, NULL, 16);
	}

	return 0;
}

/* ****************************************************************************/

static void link_api_modem_operator_info_read_for_shell(void)
{
	struct lte_xmonitor_resp_t xmonitor_resp;
	int ret = link_api_xmonitor_read(&xmonitor_resp);

	if (ret) {
		mosh_error("link_api_xmonitor_read failed, result: ret %d", ret);
		return;
	}

	if (strlen(xmonitor_resp.full_name_str)) {
		mosh_print("Operator full name:   %s", xmonitor_resp.full_name_str);
	}

	if (strlen(xmonitor_resp.short_name_str)) {
		mosh_print("Operator short name:  %s", xmonitor_resp.short_name_str);
	}

	if (strlen(xmonitor_resp.plmn_str)) {
		mosh_print("Operator PLMN:        %s", xmonitor_resp.plmn_str);
	}

	if (xmonitor_resp.cell_id >= 0) {
		mosh_print("Current cell id:       %d (0x%s)", xmonitor_resp.cell_id,
			   xmonitor_resp.cell_id_str);
	}

	if (xmonitor_resp.pci >= 0) {
		mosh_print("Current phy cell id:   %d", xmonitor_resp.pci);
	}

	if (xmonitor_resp.band) {
		mosh_print("Current band:          %d", xmonitor_resp.band);
	}

	if (xmonitor_resp.tac >= 0) {
		mosh_print("Current TAC:           %d (0x%s)", xmonitor_resp.tac,
			   xmonitor_resp.tac_str);
	}

	if (xmonitor_resp.rsrp >= 0) {
		mosh_print(
			"Current rsrp:          %d: %ddBm",
			xmonitor_resp.rsrp,
			RSRP_IDX_TO_DBM(xmonitor_resp.rsrp));
	}

	if (xmonitor_resp.snr >= 0) {
		mosh_print(
			"Current snr:           %d: %ddB",
			xmonitor_resp.snr,
			(xmonitor_resp.snr - LINK_SNR_OFFSET_VALUE));
	}
}

/* ****************************************************************************/

static int link_api_pdp_context_dynamic_params_get(struct pdp_context_info *populated_info)
{
	int ret = 0;
	struct at_param_list param_list = { 0 };
	size_t param_str_len;

	/* Cannot use global mosh_at_resp_buf because this used from link_api_pdp_contexts_read()
	 * where global mosh_at_resp_buf is already used
	 */
	char cgcontrdp_at_rsp_buf[512];
	char *next_param_str;
	bool resp_continues = false;

	char *at_ptr;
	char *tmp_ptr;
	int lines = 0;
	int iterator = 0;
	char dns_addr_str[AT_CMD_PDP_CONTEXT_READ_IP_ADDR_STR_MAX_LEN];

	char at_cmd_pdp_context_read_info_cmd_str[15];

	int family;
	struct in_addr *addr;
	struct in6_addr *addr6;

	at_ptr = cgcontrdp_at_rsp_buf;
	tmp_ptr = cgcontrdp_at_rsp_buf;

	sprintf(at_cmd_pdp_context_read_info_cmd_str,
		AT_CMD_PDP_CONTEXT_READ_INFO, populated_info->cid);
	ret = nrf_modem_at_cmd(cgcontrdp_at_rsp_buf, sizeof(cgcontrdp_at_rsp_buf), "%s",
			       at_cmd_pdp_context_read_info_cmd_str);
	if (ret) {
		mosh_error(
			"nrf_modem_at_cmd returned err: %d for %s",
			ret,
			at_cmd_pdp_context_read_info_cmd_str);
		return ret;
	}

	/* Check how many rows of info do we have */
	while (strncmp(tmp_ptr, "OK", 2) &&
	       (tmp_ptr = strstr(tmp_ptr, AT_CMD_PDP_CONTEXT_READ_RSP_DELIM)) != NULL) {
		tmp_ptr += 2;
		lines++;
	}

	/* Parse the response */
	ret = at_params_list_init(&param_list, AT_CMD_PDP_CONTEXT_READ_INFO_PARAM_COUNT);
	if (ret) {
		mosh_error("Could not init AT params list, error: %d\n", ret);
		return ret;
	}

parse:
	resp_continues = false;
	ret = at_parser_max_params_from_str(at_ptr, &next_param_str, &param_list,
					    AT_CMD_PDP_CONTEXT_READ_INFO_PARAM_COUNT);
	if (ret == -EAGAIN) {
		resp_continues = true;
	} else if (ret == -E2BIG) {
		mosh_error("E2BIG, error: %d\n", ret);
	} else if (ret != 0) {
		mosh_error(
			"Could not parse AT response for %s, error: %d\n",
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
		mosh_error(
			"Could not parse dns str for cid %d, err: %d",
			populated_info->cid, ret);
		goto clean_exit;
	}
	dns_addr_str[param_str_len] = '\0';

	family = net_utils_sa_family_from_ip_string(dns_addr_str);

	if (family == AF_INET) {
		addr = &(populated_info->dns_addr4_primary);
		(void)inet_pton(AF_INET, dns_addr_str, addr);
	} else if (family == AF_INET6) {
		addr6 = &(populated_info->dns_addr6_primary);
		(void)inet_pton(AF_INET6, dns_addr_str, addr6);
	}

	/* Read secondary DNS address */
	param_str_len = sizeof(dns_addr_str);

	ret = at_params_string_get(
		&param_list,
		AT_CMD_PDP_CONTEXT_READ_INFO_DNS_ADDR_SECONDARY_INDEX,
		dns_addr_str, &param_str_len);
	if (ret) {
		mosh_error("Could not parse dns str, err: %d", ret);
		goto clean_exit;
	}
	dns_addr_str[param_str_len] = '\0';

	family = net_utils_sa_family_from_ip_string(dns_addr_str);

	if (family == AF_INET) {
		addr = &(populated_info->dns_addr4_secondary);
		(void)inet_pton(AF_INET, dns_addr_str, addr);
	} else if (family == AF_INET6) {
		addr6 = &(populated_info->dns_addr6_secondary);
		(void)inet_pton(AF_INET6, dns_addr_str, addr6);
	}

	/* Read link MTU if exists:
	 * AT command spec:
	 * Note: If the PDN connection has dual stack capabilities, at least one pair of
	 * lines with information is returned per <cid>: First one line with the IPv4
	 * parameters followed by one line with the IPv6 parameters.
	 */
	if (iterator == 1) {
		ret = at_params_int_get(&param_list, AT_CMD_PDP_CONTEXT_READ_INFO_MTU_INDEX,
					&(populated_info->ipv6_mtu));
		if (ret) {
			/* Don't care if it fails */
			ret = 0;
			populated_info->ipv6_mtu = 0;
		}
	} else {
		ret = at_params_int_get(&param_list, AT_CMD_PDP_CONTEXT_READ_INFO_MTU_INDEX,
					&(populated_info->ipv4_mtu));
		if (ret) {
			/* Don't care if it fails */
			ret = 0;
			populated_info->ipv4_mtu = 0;
		}
	}

	if (resp_continues) {
		at_ptr = next_param_str;
		iterator++;
		if (iterator < lines) {
			goto parse;
		}
	}

clean_exit:
	at_params_list_free(&param_list);

	return ret;
}

/* ****************************************************************************/

int link_api_pdp_contexts_read(struct pdp_context_info_array *pdp_info)
{
	int ret = 0;
	struct at_param_list param_list = { 0 };
	size_t param_str_len;
	char *next_param_str;
	bool resp_continues = false;

	char *at_ptr;
	char *tmp_ptr;
	int pdp_cnt = 0;
	int iterator = 0;
	struct pdp_context_info *populated_info;

	char ip_addr_str[AT_CMD_PDP_CONTEXT_READ_IP_ADDR_STR_MAX_LEN];
	char *ip_address1, *ip_address2;
	int family;
	struct in_addr *addr4;
	struct in6_addr *addr6;

	memset(pdp_info, 0, sizeof(struct pdp_context_info_array));

	k_mutex_lock(&mosh_at_resp_buf_mutex, K_FOREVER);
	at_ptr = mosh_at_resp_buf;
	tmp_ptr = mosh_at_resp_buf;

	ret = nrf_modem_at_cmd(mosh_at_resp_buf, sizeof(mosh_at_resp_buf),
			       AT_CMD_PDP_CONTEXTS_READ);
	if (ret) {
		mosh_error("nrf_modem_at_cmd returned err: %d", ret);
		goto clean_exit;
	}

	/* Check how many rows/context do we have */
	while (strncmp(tmp_ptr, "OK", 2) &&
	       (tmp_ptr = strstr(tmp_ptr, AT_CMD_PDP_CONTEXT_READ_RSP_DELIM)) != NULL) {
		tmp_ptr += 2;
		pdp_cnt++;
	}

	/* Allocate array of PDP info accordingly */
	pdp_info->array = calloc(pdp_cnt, sizeof(struct pdp_context_info));
	pdp_info->size = pdp_cnt;

	/* Parse the response */
	ret = at_params_list_init(&param_list, AT_CMD_PDP_CONTEXTS_READ_PARAM_COUNT);
	if (ret) {
		mosh_error("Could not init AT params list, error: %d\n", ret);
		goto clean_exit;
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
		mosh_error("Could not parse AT response, error: %d", ret);
		goto clean_exit;
	}

	ret = at_params_int_get(&param_list,
				AT_CMD_PDP_CONTEXTS_READ_CID_INDEX,
				&populated_info[iterator].cid);
	if (ret) {
		mosh_error("Could not parse CID, err: %d", ret);
		goto clean_exit;
	}
	ret = pdn_id_get(populated_info[iterator].cid);
	if (ret < 0) {
		mosh_error(
			"Could not get PDN for CID %d, err: %d\n",
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
		mosh_error("Could not parse pdp type, err: %d", ret);
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
		mosh_error("Could not parse apn str, err: %d", ret);
		goto clean_exit;
	}
	populated_info[iterator].apn_str[param_str_len] = '\0';

	param_str_len = sizeof(ip_addr_str);
	ret = at_params_string_get(
		&param_list, AT_CMD_PDP_CONTEXTS_READ_PDP_ADDR_INDEX,
		ip_addr_str, &param_str_len);
	if (ret) {
		mosh_error("Could not parse apn str, err: %d", ret);
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
		 * thus in following ipv4 branch should not be possible
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

	/* Get DNS addresses etc. for this IP context */
	if (populated_info[iterator].pdp_type != PDP_TYPE_NONIP) {
		(void)link_api_pdp_context_dynamic_params_get(&(populated_info[iterator]));
	}

	if (resp_continues) {
		at_ptr = next_param_str;
		iterator++;
		if (iterator < pdp_cnt) {
			goto parse;
		}
	}

	/* ...and finally, fill PDP context activation status for each */
	link_api_context_info_fill_activation_status(pdp_info);

clean_exit:
	at_params_list_free(&param_list);
	/* User need to free pdp_info->array also in case of error */

	k_mutex_unlock(&mosh_at_resp_buf_mutex);

	return ret;
}

/* *****************************************************************************/

void link_api_modem_info_get_for_shell(bool connected)
{
	struct pdp_context_info_array pdp_context_info_tbl;
	enum lte_lc_system_mode sys_mode_current;
	enum lte_lc_system_mode_preference sys_mode_preferred;
	enum lte_lc_lte_mode currently_active_mode;
	char info_str[MODEM_INFO_MAX_RESPONSE_SIZE + 1];
	char device_id[NRF_CLOUD_CLIENT_ID_MAX_LEN + 1];
	int ret;

	(void)link_shell_get_and_print_current_system_modes(
		&sys_mode_current, &sys_mode_preferred, &currently_active_mode);

	ret = modem_info_string_get(MODEM_INFO_FW_VERSION, info_str, sizeof(info_str));
	if (ret >= 0) {
		mosh_print("Modem FW version:      %s", info_str);
	} else {
		mosh_error("Unable to obtain modem FW version (%d)", ret);
	}

	/* Get the device id used with nRF Cloud */
#if defined(CONFIG_NRF_CLOUD_AGNSS) || defined(CONFIG_NRF_CLOUD_PGPS) || \
	defined(CONFIG_NRF_CLOUD_MQTT) || defined(CONFIG_NRF_CLOUD_REST)
	ret = nrf_cloud_client_id_get(device_id, sizeof(device_id));
#else
	ret = -ENOTSUP;
#endif
	mosh_print("Device ID:             %s", (ret) ? "Not known" : device_id);

	if (connected) {
		link_api_modem_operator_info_read_for_shell();

		ret = modem_info_string_get(MODEM_INFO_DATE_TIME, info_str, sizeof(info_str));
		if (ret >= 0) {
			mosh_print("Mobile network time and date: %s", info_str);
		}

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
					sprintf(tmp_str, "%d", info_tbl[i].pdn_id);
				}

				/* Parsed PDP context info */
				mosh_print("PDP context info %d:", (i + 1));
				mosh_print("  CID:                %d", info_tbl[i].cid);
				mosh_print("  PDN ID:             %s",
						(info_tbl[i].pdn_id_valid) ? tmp_str : "Not known");
				mosh_print("  PDP context active: %s",
						(info_tbl[i].ctx_active) ? "yes" : "no");
				mosh_print("  PDP type:           %s", info_tbl[i].pdp_type_str);
				mosh_print("  APN:                %s", info_tbl[i].apn_str);
				if (info_tbl[i].ipv4_mtu) {
					mosh_print("  IPv4 MTU:           %d",
						info_tbl[i].ipv4_mtu);
				}
				if (info_tbl[i].ipv6_mtu) {
					mosh_print("  IPv6 MTU:           %d",
						info_tbl[i].ipv6_mtu);
				}
				mosh_print("  IPv4 address:       %s", ipv4_addr);
				mosh_print("  IPv6 address:       %s", ipv6_addr);
				mosh_print("  IPv4 DNS address:   %s, %s",
						ipv4_dns_addr_primary, ipv4_dns_addr_secondary);
				mosh_print("  IPv6 DNS address:   %s, %s",
						ipv6_dns_addr_primary, ipv6_dns_addr_secondary);
			}
		} else {
			mosh_error("Unable to obtain pdp context info (%d)", ret);
		}
		if (pdp_context_info_tbl.array != NULL) {
			free(pdp_context_info_tbl.array);
		}
	}
}

int link_api_rai_status(bool *rai_status)
{
	int ret;
	int status;

	*rai_status = false;

	ret = nrf_modem_at_scanf("AT%RAI?", "%%RAI: %u", &status);
	if (ret < 0) {
		mosh_error("RAI AT command failed, error: %d", ret);
		return -EFAULT;
	} else if (ret < 1) {
		mosh_error("Could not parse RAI status, err %d", ret);
		return -EBADMSG;
	}
	*rai_status = (bool)status;
	return 0;
}
