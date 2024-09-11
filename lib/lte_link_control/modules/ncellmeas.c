/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <modem/at_monitor.h>
#include <modem/lte_lc.h>
#include <modem/lte_lc_trace.h>
#include <modem/at_parser.h>
#include <nrf_modem_at.h>

#include "common/event_handler_list.h"
#include "common/helpers.h"
#include "modules/ncellmeas.h"

LOG_MODULE_DECLARE(lte_lc, CONFIG_LTE_LINK_CONTROL_LOG_LEVEL);

/* NCELLMEAS notification parameters */
#define AT_NCELLMEAS_START		     "AT%%NCELLMEAS"
#define AT_NCELLMEAS_STOP		     "AT%%NCELLMEASSTOP"
#define AT_NCELLMEAS_STATUS_INDEX	     1
#define AT_NCELLMEAS_STATUS_VALUE_SUCCESS    0
#define AT_NCELLMEAS_STATUS_VALUE_FAIL	     1
#define AT_NCELLMEAS_STATUS_VALUE_INCOMPLETE 2
#define AT_NCELLMEAS_CELL_ID_INDEX	     2
#define AT_NCELLMEAS_PLMN_INDEX		     3
#define AT_NCELLMEAS_TAC_INDEX		     4
#define AT_NCELLMEAS_TIMING_ADV_INDEX	     5
#define AT_NCELLMEAS_EARFCN_INDEX	     6
#define AT_NCELLMEAS_PHYS_CELL_ID_INDEX	     7
#define AT_NCELLMEAS_RSRP_INDEX		     8
#define AT_NCELLMEAS_RSRQ_INDEX		     9
#define AT_NCELLMEAS_MEASUREMENT_TIME_INDEX  10
#define AT_NCELLMEAS_PRE_NCELLS_PARAMS_COUNT 11
/* The rest of the parameters are in repeating arrays per neighboring cell.
 * The indices below refer to their index within such a repeating array.
 */
#define AT_NCELLMEAS_N_EARFCN_INDEX	     0
#define AT_NCELLMEAS_N_PHYS_CELL_ID_INDEX    1
#define AT_NCELLMEAS_N_RSRP_INDEX	     2
#define AT_NCELLMEAS_N_RSRQ_INDEX	     3
#define AT_NCELLMEAS_N_TIME_DIFF_INDEX	     4
#define AT_NCELLMEAS_N_PARAMS_COUNT	     5
#define AT_NCELLMEAS_N_MAX_ARRAY_SIZE	     CONFIG_LTE_NEIGHBOR_CELLS_MAX

#define AT_NCELLMEAS_PARAMS_COUNT_MAX                                                              \
	(AT_NCELLMEAS_PRE_NCELLS_PARAMS_COUNT +                                                    \
	 AT_NCELLMEAS_N_PARAMS_COUNT * CONFIG_LTE_NEIGHBOR_CELLS_MAX)

#define AT_NCELLMEAS_GCI_CELL_PARAMS_COUNT 12

/* Requested NCELLMEAS params */
static struct lte_lc_ncellmeas_params ncellmeas_params;
/* Sempahore value 1 means ncellmeas is not ongoing, and 0 means it's ongoing. */
K_SEM_DEFINE(ncellmeas_idle_sem, 1, 1);

AT_MONITOR(ltelc_atmon_ncellmeas, "%NCELLMEAS", at_handler_ncellmeas);

/* Counts the frequency of a character in a null-terminated string. */
static uint32_t get_char_frequency(const char *str, char c)
{
	uint32_t count = 0;

	__ASSERT_NO_MSG(str != NULL);

	do {
		if (*str == c) {
			count++;
		}
	} while (*(str++) != '\0');

	return count;
}

static uint32_t neighborcell_count_get(const char *at_response)
{
	uint32_t comma_count, ncell_elements, ncell_count;

	__ASSERT_NO_MSG(at_response != NULL);

	comma_count = get_char_frequency(at_response, ',');
	if (comma_count < AT_NCELLMEAS_PRE_NCELLS_PARAMS_COUNT) {
		return 0;
	}

	/* Add one, as there's no comma after the last element. */
	ncell_elements = comma_count - (AT_NCELLMEAS_PRE_NCELLS_PARAMS_COUNT - 1) + 1;
	ncell_count = ncell_elements / AT_NCELLMEAS_N_PARAMS_COUNT;

	return ncell_count;
}

static int parse_ncellmeas_gci(struct lte_lc_ncellmeas_params *params, const char *at_response,
			       struct lte_lc_cells_info *cells)
{
	struct at_parser parser;
	struct lte_lc_ncell *ncells = NULL;
	int err, status, tmp_int, len;
	int16_t tmp_short;
	char tmp_str[7];
	bool incomplete = false;
	int curr_index;
	size_t i = 0, j = 0, k = 0;

	/* Count the actual number of parameters in the AT response before
	 * allocating heap for it. This may save quite a bit of heap as the
	 * worst case scenario is 96 elements.
	 * 3 is added to account for the parameters that do not have a trailing
	 * comma.
	 */
	size_t param_count = get_char_frequency(at_response, ',') + 3;

	__ASSERT_NO_MSG(at_response != NULL);
	__ASSERT_NO_MSG(params != NULL);
	__ASSERT_NO_MSG(cells != NULL);
	__ASSERT_NO_MSG(cells->gci_cells != NULL);

	/* Fill the defaults */
	cells->gci_cells_count = 0;
	cells->ncells_count = 0;
	cells->current_cell.id = LTE_LC_CELL_EUTRAN_ID_INVALID;

	for (i = 0; i < params->gci_count; i++) {
		cells->gci_cells[i].id = LTE_LC_CELL_EUTRAN_ID_INVALID;
		cells->gci_cells[i].timing_advance = LTE_LC_CELL_TIMING_ADVANCE_INVALID;
	}

	/*
	 * Response format for GCI search types:
	 * High level:
	 * status[,
	 *	GCI_cell_info1,neighbor_count1[,neighbor_cell1_1,neighbor_cell1_2...],
	 *	GCI_cell_info2,neighbor_count2[,neighbor_cell2_1,neighbor_cell2_2...]...]
	 *
	 * Detailed:
	 * %NCELLMEAS: status
	 * [,<cell_id>,<plmn>,<tac>,<ta>,<ta_meas_time>,<earfcn>,<phys_cell_id>,<rsrp>,<rsrq>,
	 *		<meas_time>,<serving>,<neighbor_count>
	 *	[,<n_earfcn1>,<n_phys_cell_id1>,<n_rsrp1>,<n_rsrq1>,<time_diff1>]
	 *	[,<n_earfcn2>,<n_phys_cell_id2>,<n_rsrp2>,<n_rsrq2>,<time_diff2>]...],
	 *  <cell_id>,<plmn>,<tac>,<ta>,<ta_meas_time>,<earfcn>,<phys_cell_id>,<rsrp>,<rsrq>,
	 *		<meas_time>,<serving>,<neighbor_count>
	 *	[,<n_earfcn1>,<n_phys_cell_id1>,<n_rsrp1>,<n_rsrq1>,<time_diff1>]
	 *	[,<n_earfcn2>,<n_phys_cell_id2>,<n_rsrp2>,<n_rsrq2>,<time_diff2>]...]...
	 */

	err = at_parser_init(&parser, at_response);
	__ASSERT_NO_MSG(err == 0);

	/* Status code */
	curr_index = AT_NCELLMEAS_STATUS_INDEX;
	err = at_parser_num_get(&parser, curr_index, &status);
	if (err) {
		LOG_DBG("Cannot parse NCELLMEAS status");
		goto clean_exit;
	}

	if (status == AT_NCELLMEAS_STATUS_VALUE_FAIL) {
		err = 1;
		LOG_WRN("NCELLMEAS failed");
		goto clean_exit;
	} else if (status == AT_NCELLMEAS_STATUS_VALUE_INCOMPLETE) {
		LOG_WRN("NCELLMEAS interrupted; results incomplete");
	}

	/* Go through the cells */
	for (i = 0; curr_index < (param_count - (AT_NCELLMEAS_GCI_CELL_PARAMS_COUNT + 1)) &&
		    i < params->gci_count;
	     i++) {
		struct lte_lc_cell parsed_cell;
		bool is_serving_cell;
		uint8_t parsed_ncells_count;

		/* <cell_id>  */
		curr_index++;
		err = string_param_to_int(&parser, curr_index, &tmp_int, 16);
		if (err) {
			LOG_ERR("Could not parse cell_id, index %d, i %d error: %d", curr_index, i,
				err);
			goto clean_exit;
		}

		if (tmp_int > LTE_LC_CELL_EUTRAN_ID_MAX) {
			LOG_WRN("cell_id = %d which is > LTE_LC_CELL_EUTRAN_ID_MAX; "
				"marking invalid",
				tmp_int);
			tmp_int = LTE_LC_CELL_EUTRAN_ID_INVALID;
		}
		parsed_cell.id = tmp_int;

		/* <plmn> */
		len = sizeof(tmp_str);

		curr_index++;
		err = at_parser_string_get(&parser, curr_index, tmp_str, &len);
		if (err) {
			LOG_ERR("Could not parse plmn, error: %d", err);
			goto clean_exit;
		}

		/* Read MNC and store as integer. The MNC starts as the fourth character
		 * in the string, following three characters long MCC.
		 */
		err = string_to_int(&tmp_str[3], 10, &parsed_cell.mnc);
		if (err) {
			LOG_ERR("string_to_int, error: %d", err);
			goto clean_exit;
		}

		/* Null-terminated MCC, read and store it. */
		tmp_str[3] = '\0';

		err = string_to_int(tmp_str, 10, &parsed_cell.mcc);
		if (err) {
			LOG_ERR("string_to_int, error: %d", err);
			goto clean_exit;
		}

		/* <tac> */
		curr_index++;
		err = string_param_to_int(&parser, curr_index, &tmp_int, 16);
		if (err) {
			LOG_ERR("Could not parse tracking_area_code in i %d, error: %d", i, err);
			goto clean_exit;
		}
		parsed_cell.tac = tmp_int;

		/* <ta> */
		curr_index++;
		err = at_parser_num_get(&parser, curr_index, &tmp_int);
		if (err) {
			LOG_ERR("Could not parse timing_advance, error: %d", err);
			goto clean_exit;
		}
		parsed_cell.timing_advance = tmp_int;

		/* <ta_meas_time> */
		curr_index++;
		err = at_parser_num_get(&parser, curr_index, &parsed_cell.timing_advance_meas_time);
		if (err) {
			LOG_ERR("Could not parse timing_advance_meas_time, error: %d", err);
			goto clean_exit;
		}

		/* <earfcn> */
		curr_index++;
		err = at_parser_num_get(&parser, curr_index, &parsed_cell.earfcn);
		if (err) {
			LOG_ERR("Could not parse earfcn, error: %d", err);
			goto clean_exit;
		}

		/* <phys_cell_id> */
		curr_index++;
		err = at_parser_num_get(&parser, curr_index, &parsed_cell.phys_cell_id);
		if (err) {
			LOG_ERR("Could not parse phys_cell_id, error: %d", err);
			goto clean_exit;
		}

		/* <rsrp> */
		curr_index++;
		err = at_parser_num_get(&parser, curr_index, &parsed_cell.rsrp);
		if (err) {
			LOG_ERR("Could not parse rsrp, error: %d", err);
			goto clean_exit;
		}

		/* <rsrq> */
		curr_index++;
		err = at_parser_num_get(&parser, curr_index, &parsed_cell.rsrq);
		if (err) {
			LOG_ERR("Could not parse rsrq, error: %d", err);
			goto clean_exit;
		}

		/* <meas_time> */
		curr_index++;
		err = at_parser_num_get(&parser, curr_index, &parsed_cell.measurement_time);
		if (err) {
			LOG_ERR("Could not parse meas_time, error: %d", err);
			goto clean_exit;
		}

		/* <serving> */
		curr_index++;
		err = at_parser_num_get(&parser, curr_index, &tmp_short);
		if (err) {
			LOG_ERR("Could not parse serving, error: %d", err);
			goto clean_exit;
		}
		is_serving_cell = tmp_short;

		/* <neighbor_count> */
		curr_index++;
		err = at_parser_num_get(&parser, curr_index, &tmp_short);
		if (err) {
			LOG_ERR("Could not parse neighbor_count, error: %d", err);
			goto clean_exit;
		}
		parsed_ncells_count = tmp_short;

		if (is_serving_cell) {
			int to_be_parsed_ncell_count = 0;

			/* This the current/serving cell.
			 * In practice the <neighbor_count> is always 0 for other than
			 * the serving cell, i.e. no neigbour cell list is available.
			 * Thus, handle neighbor cells only for the serving cell.
			 */
			cells->current_cell = parsed_cell;
			if (parsed_ncells_count != 0) {
				/* Allocate room for the parsed neighbor info. */
				if (parsed_ncells_count > CONFIG_LTE_NEIGHBOR_CELLS_MAX) {
					to_be_parsed_ncell_count = CONFIG_LTE_NEIGHBOR_CELLS_MAX;
					incomplete = true;
					LOG_WRN("Cutting response, because received neigbor cell"
						" count is bigger than configured max: %d",
						CONFIG_LTE_NEIGHBOR_CELLS_MAX);

				} else {
					to_be_parsed_ncell_count = parsed_ncells_count;
				}
				ncells = k_calloc(to_be_parsed_ncell_count,
						  sizeof(struct lte_lc_ncell));
				if (ncells == NULL) {
					LOG_WRN("Failed to allocate memory for the ncells"
						" (continue)");
					continue;
				}
				cells->neighbor_cells = ncells;
				cells->ncells_count = to_be_parsed_ncell_count;
			}

			/* Parse neighbors */
			for (j = 0; j < parsed_ncells_count; j++) {
				/* If maximum number of cells has been stored, skip the data for
				 * the remaining ncells to be able to continue from next GCI cell
				 */
				if (j >= to_be_parsed_ncell_count) {
					LOG_WRN("Ignoring ncell");
					curr_index += 5;
					continue;
				}
				/* <n_earfcn[j]> */
				curr_index++;
				err = at_parser_num_get(&parser, curr_index,
							&cells->neighbor_cells[j].earfcn);
				if (err) {
					LOG_ERR("Could not parse n_earfcn, error: %d", err);
					goto clean_exit;
				}

				/* <n_phys_cell_id[j]> */
				curr_index++;
				err = at_parser_num_get(&parser, curr_index,
							&cells->neighbor_cells[j].phys_cell_id);
				if (err) {
					LOG_ERR("Could not parse n_phys_cell_id, error: %d", err);
					goto clean_exit;
				}

				/* <n_rsrp[j]> */
				curr_index++;
				err = at_parser_num_get(&parser, curr_index, &tmp_int);
				if (err) {
					LOG_ERR("Could not parse n_rsrp, error: %d", err);
					goto clean_exit;
				}
				cells->neighbor_cells[j].rsrp = tmp_int;

				/* <n_rsrq[j]> */
				curr_index++;
				err = at_parser_num_get(&parser, curr_index, &tmp_int);
				if (err) {
					LOG_ERR("Could not parse n_rsrq, error: %d", err);
					goto clean_exit;
				}
				cells->neighbor_cells[j].rsrq = tmp_int;

				/* <time_diff[j]> */
				curr_index++;
				err = at_parser_num_get(&parser, curr_index,
							&cells->neighbor_cells[j].time_diff);
				if (err) {
					LOG_ERR("Could not parse time_diff, error: %d", err);
					goto clean_exit;
				}
			}
		} else {
			cells->gci_cells[k] = parsed_cell;
			cells->gci_cells_count++; /* Increase count for non-serving GCI cell */
			k++;
		}
	}

	if (incomplete) {
		err = -E2BIG;
		LOG_WRN("Buffer is too small; results incomplete: %d", err);
	}

clean_exit:
	return err;
}

static int parse_ncellmeas(const char *at_response, struct lte_lc_cells_info *cells)
{
	int err, status, tmp;
	struct at_parser parser;
	size_t count = 0;
	bool incomplete = false;

	__ASSERT_NO_MSG(at_response != NULL);
	__ASSERT_NO_MSG(cells != NULL);

	cells->ncells_count = 0;
	cells->current_cell.id = LTE_LC_CELL_EUTRAN_ID_INVALID;

	err = at_parser_init(&parser, at_response);
	__ASSERT_NO_MSG(err == 0);

	/* Status code */
	err = at_parser_num_get(&parser, AT_NCELLMEAS_STATUS_INDEX, &status);
	if (err) {
		goto clean_exit;
	}

	if (status == AT_NCELLMEAS_STATUS_VALUE_FAIL) {
		err = 1;
		LOG_WRN("NCELLMEAS failed");
		goto clean_exit;
	} else if (status == AT_NCELLMEAS_STATUS_VALUE_INCOMPLETE) {
		LOG_WRN("NCELLMEAS interrupted; results incomplete");
	}

	/* Current cell ID */
	err = string_param_to_int(&parser, AT_NCELLMEAS_CELL_ID_INDEX, &tmp, 16);
	if (err) {
		goto clean_exit;
	}

	if (tmp > LTE_LC_CELL_EUTRAN_ID_MAX) {
		tmp = LTE_LC_CELL_EUTRAN_ID_INVALID;
	}
	cells->current_cell.id = tmp;

	/* PLMN, that is, MCC and MNC */
	err = plmn_param_string_to_mcc_mnc(&parser, AT_NCELLMEAS_PLMN_INDEX,
					   &cells->current_cell.mcc, &cells->current_cell.mnc);
	if (err) {
		goto clean_exit;
	}

	/* Tracking area code */
	err = string_param_to_int(&parser, AT_NCELLMEAS_TAC_INDEX, &tmp, 16);
	if (err) {
		goto clean_exit;
	}

	cells->current_cell.tac = tmp;

	/* Timing advance */
	err = at_parser_num_get(&parser, AT_NCELLMEAS_TIMING_ADV_INDEX, &tmp);
	if (err) {
		goto clean_exit;
	}

	cells->current_cell.timing_advance = tmp;

	/* EARFCN */
	err = at_parser_num_get(&parser, AT_NCELLMEAS_EARFCN_INDEX, &cells->current_cell.earfcn);
	if (err) {
		goto clean_exit;
	}

	/* Physical cell ID */
	err = at_parser_num_get(&parser, AT_NCELLMEAS_PHYS_CELL_ID_INDEX,
				&cells->current_cell.phys_cell_id);
	if (err) {
		goto clean_exit;
	}

	/* RSRP */
	err = at_parser_num_get(&parser, AT_NCELLMEAS_RSRP_INDEX, &tmp);
	if (err) {
		goto clean_exit;
	}

	cells->current_cell.rsrp = tmp;

	/* RSRQ */
	err = at_parser_num_get(&parser, AT_NCELLMEAS_RSRQ_INDEX, &tmp);
	if (err) {
		goto clean_exit;
	}

	cells->current_cell.rsrq = tmp;

	/* Measurement time */
	err = at_parser_num_get(&parser, AT_NCELLMEAS_MEASUREMENT_TIME_INDEX,
				&cells->current_cell.measurement_time);
	if (err) {
		goto clean_exit;
	}

	/* Neighbor cell count */
	cells->ncells_count = neighborcell_count_get(at_response);

	/* Starting from modem firmware v1.3.1, timing advance measurement time
	 * information is added as the last parameter in the response.
	 */
	size_t ta_meas_time_index = AT_NCELLMEAS_PRE_NCELLS_PARAMS_COUNT +
				    cells->ncells_count * AT_NCELLMEAS_N_PARAMS_COUNT;

	err = at_parser_cmd_count_get(&parser, &count);
	if (err) {
		LOG_ERR("Could not get NCELLMEAS param count, "
			"potentially malformed notification, error: %d",
			err);
		goto clean_exit;
	}

	if (count > ta_meas_time_index) {
		err = at_parser_num_get(&parser, ta_meas_time_index,
					&cells->current_cell.timing_advance_meas_time);
		if (err) {
			goto clean_exit;
		}
	} else {
		cells->current_cell.timing_advance_meas_time = 0;
	}

	if (cells->ncells_count == 0) {
		goto clean_exit;
	}

	__ASSERT_NO_MSG(cells->neighbor_cells != NULL);

	if (cells->ncells_count > CONFIG_LTE_NEIGHBOR_CELLS_MAX) {
		cells->ncells_count = CONFIG_LTE_NEIGHBOR_CELLS_MAX;
		incomplete = true;
		LOG_WRN("Cutting response, because received neigbor cell"
			" count is bigger than configured max: %d",
			CONFIG_LTE_NEIGHBOR_CELLS_MAX);
	}

	/* Neighboring cells */
	for (size_t i = 0; i < cells->ncells_count; i++) {
		size_t start_idx =
			AT_NCELLMEAS_PRE_NCELLS_PARAMS_COUNT + i * AT_NCELLMEAS_N_PARAMS_COUNT;

		/* EARFCN */
		err = at_parser_num_get(&parser, start_idx + AT_NCELLMEAS_N_EARFCN_INDEX,
					&cells->neighbor_cells[i].earfcn);
		if (err) {
			goto clean_exit;
		}

		/* Physical cell ID */
		err = at_parser_num_get(&parser, start_idx + AT_NCELLMEAS_N_PHYS_CELL_ID_INDEX,
					&cells->neighbor_cells[i].phys_cell_id);
		if (err) {
			goto clean_exit;
		}

		/* RSRP */
		err = at_parser_num_get(&parser, start_idx + AT_NCELLMEAS_N_RSRP_INDEX, &tmp);
		if (err) {
			goto clean_exit;
		}

		cells->neighbor_cells[i].rsrp = tmp;

		/* RSRQ */
		err = at_parser_num_get(&parser, start_idx + AT_NCELLMEAS_N_RSRQ_INDEX, &tmp);
		if (err) {
			goto clean_exit;
		}

		cells->neighbor_cells[i].rsrq = tmp;

		/* Time difference */
		err = at_parser_num_get(&parser, start_idx + AT_NCELLMEAS_N_TIME_DIFF_INDEX,
					&cells->neighbor_cells[i].time_diff);
		if (err) {
			goto clean_exit;
		}
	}

	if (incomplete) {
		err = -E2BIG;
		LOG_WRN("Buffer is too small; results incomplete: %d", err);
	}

clean_exit:
	return err;
}

static void at_handler_ncellmeas_gci(const char *response)
{
	int err;
	struct lte_lc_evt evt = {0};
	const char *resp = response;
	struct lte_lc_cell *cells = NULL;

	__ASSERT_NO_MSG(response != NULL);
	__ASSERT_NO_MSG(ncellmeas_params.gci_count != 0);

	LOG_DBG("%%NCELLMEAS GCI notification parsing starts");

	cells = k_calloc(ncellmeas_params.gci_count, sizeof(struct lte_lc_cell));
	if (cells == NULL) {
		LOG_ERR("Failed to allocate memory for the GCI cells");
		return;
	}

	evt.cells_info.gci_cells = cells;
	err = parse_ncellmeas_gci(&ncellmeas_params, resp, &evt.cells_info);
	LOG_DBG("parse_ncellmeas_gci returned %d", err);
	switch (err) {
	case -E2BIG:
		LOG_WRN("Not all neighbor cells could be parsed. "
			"More cells than the configured max count of %d were found",
			CONFIG_LTE_NEIGHBOR_CELLS_MAX);
		/* Fall through */
	case 0: /* Fall through */
	case 1:
		LOG_DBG("Neighbor cell count: %d, GCI cells count: %d", evt.cells_info.ncells_count,
			evt.cells_info.gci_cells_count);
		evt.type = LTE_LC_EVT_NEIGHBOR_CELL_MEAS;
		event_handler_list_dispatch(&evt);
		break;
	default:
		LOG_ERR("Parsing of neighbor cells failed, err: %d", err);
		break;
	}

	k_free(cells);
	k_free(evt.cells_info.neighbor_cells);
}

static void at_handler_ncellmeas(const char *response)
{
	int err;
	struct lte_lc_evt evt = {0};

	__ASSERT_NO_MSG(response != NULL);

	if (event_handler_list_is_empty()) {
		/* No need to parse the response if there is no handler
		 * to receive the parsed data.
		 */
		goto exit;
	}

	if (ncellmeas_params.search_type > LTE_LC_NEIGHBOR_SEARCH_TYPE_EXTENDED_COMPLETE) {
		at_handler_ncellmeas_gci(response);
		goto exit;
	}

	int ncell_count = neighborcell_count_get(response);
	struct lte_lc_ncell *neighbor_cells = NULL;

	LOG_DBG("%%NCELLMEAS notification: neighbor cell count: %d", ncell_count);

	if (ncell_count != 0) {
		neighbor_cells = k_calloc(ncell_count, sizeof(struct lte_lc_ncell));
		if (neighbor_cells == NULL) {
			LOG_ERR("Failed to allocate memory for neighbor cells");
			goto exit;
		}
	}

	evt.cells_info.neighbor_cells = neighbor_cells;

	err = parse_ncellmeas(response, &evt.cells_info);

	switch (err) {
	case -E2BIG:
		LOG_WRN("Not all neighbor cells could be parsed");
		LOG_WRN("More cells than the configured max count of %d were found",
			CONFIG_LTE_NEIGHBOR_CELLS_MAX);
		/* Fall through */
	case 0: /* Fall through */
	case 1:
		evt.type = LTE_LC_EVT_NEIGHBOR_CELL_MEAS;
		event_handler_list_dispatch(&evt);
		break;
	default:
		LOG_ERR("Parsing of neighbor cells failed, err: %d", err);
		break;
	}

	if (neighbor_cells) {
		k_free(neighbor_cells);
	}
exit:
	k_sem_give(&ncellmeas_idle_sem);
}

int ncellmeas_start(struct lte_lc_ncellmeas_params *params)
{
	int err;
	/* lte_lc defaults for the used params */
	struct lte_lc_ncellmeas_params used_params = {
		.search_type = LTE_LC_NEIGHBOR_SEARCH_TYPE_DEFAULT,
		.gci_count = 0,
	};

	if (params == NULL) {
		LOG_DBG("Using default parameters");
	}
	LOG_DBG("Search type=%d, gci_count=%d",
		params != NULL ? params->search_type : used_params.search_type,
		params != NULL ? params->gci_count : used_params.gci_count);

	__ASSERT(!IN_RANGE((int)params, LTE_LC_NEIGHBOR_SEARCH_TYPE_DEFAULT,
			   LTE_LC_NEIGHBOR_SEARCH_TYPE_GCI_EXTENDED_COMPLETE),
		 "Invalid argument, API does not accept enum values directly anymore");

	if (params != NULL &&
	    (params->search_type == LTE_LC_NEIGHBOR_SEARCH_TYPE_GCI_DEFAULT ||
	     params->search_type == LTE_LC_NEIGHBOR_SEARCH_TYPE_GCI_EXTENDED_LIGHT ||
	     params->search_type == LTE_LC_NEIGHBOR_SEARCH_TYPE_GCI_EXTENDED_COMPLETE)) {
		if (params->gci_count < 2 || params->gci_count > 15) {
			LOG_ERR("Invalid GCI count, must be in range 2-15");
			return -EINVAL;
		}
	}

	if (k_sem_take(&ncellmeas_idle_sem, K_SECONDS(1)) != 0) {
		LOG_WRN("Neighbor cell measurement already in progress");
		return -EINPROGRESS;
	}

	if (params != NULL) {
		used_params = *params;
	}
	ncellmeas_params = used_params;

	/* Starting from modem firmware v1.3.1, there is an optional parameter to specify
	 * the type of search.
	 * If the type is LTE_LC_NEIGHBOR_SEARCH_TYPE_DEFAULT, we therefore use the AT
	 * command without parameters to avoid error messages for older firmware version.
	 * Starting from modem firmware v1.3.4, additional CGI search types and
	 * GCI count are supported.
	 */
	if (used_params.search_type == LTE_LC_NEIGHBOR_SEARCH_TYPE_EXTENDED_LIGHT) {
		err = nrf_modem_at_printf("AT%%NCELLMEAS=1");
	} else if (used_params.search_type == LTE_LC_NEIGHBOR_SEARCH_TYPE_EXTENDED_COMPLETE) {
		err = nrf_modem_at_printf("AT%%NCELLMEAS=2");
	} else if (used_params.search_type == LTE_LC_NEIGHBOR_SEARCH_TYPE_GCI_DEFAULT) {
		err = nrf_modem_at_printf("AT%%NCELLMEAS=3,%d", used_params.gci_count);
	} else if (used_params.search_type == LTE_LC_NEIGHBOR_SEARCH_TYPE_GCI_EXTENDED_LIGHT) {
		err = nrf_modem_at_printf("AT%%NCELLMEAS=4,%d", used_params.gci_count);
	} else if (used_params.search_type == LTE_LC_NEIGHBOR_SEARCH_TYPE_GCI_EXTENDED_COMPLETE) {
		err = nrf_modem_at_printf("AT%%NCELLMEAS=5,%d", used_params.gci_count);
	} else {
		/* Defaulting to use LTE_LC_NEIGHBOR_SEARCH_TYPE_DEFAULT */
		err = nrf_modem_at_printf("AT%%NCELLMEAS");
	}

	if (err) {
		LOG_ERR("Sending AT%%NCELLMEAS failed, error: %d", err);
		err = -EFAULT;
		k_sem_give(&ncellmeas_idle_sem);
	}

	return err;
}

int ncellmeas_cancel(void)
{
	LOG_DBG("Cancelling");

	int err = nrf_modem_at_printf(AT_NCELLMEAS_STOP);

	if (err) {
		err = -EFAULT;
	}

	k_sem_give(&ncellmeas_idle_sem);

	return err;
}
