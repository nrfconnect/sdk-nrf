/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <unity.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <modem/lte_lc.h>
#include <nrf_errno.h>
#include <mock_nrf_modem_at.h>

#include "cmock_nrf_modem_at.h"
#include "cmock_nrf_modem.h"
#include "cmock_nrf_socket.h"

#define TEST_EVENT_MAX_COUNT 20
#define IGNORE NULL

static const char cgmr_resp_9160[] = "mfw_nrf9160_1.3.7\r\nOK\r\n";
static const char cgmr_resp_91x1[] = "mfw_nrf91x1_2.0.3\r\nOK\r\n";
static const char cgmr_resp_9151_ntn[] = "mfw_nrf9151-ntn_1.0.0\r\nOK\r\n";
static const char cgmr_resp_unknown[] = "foobar\r\nOK\r\n";

static struct lte_lc_evt test_event_data[TEST_EVENT_MAX_COUNT];
static struct lte_lc_ncell test_neighbor_cells[CONFIG_LTE_NEIGHBOR_CELLS_MAX];
static struct lte_lc_cell test_gci_cells[CONFIG_LTE_NEIGHBOR_CELLS_MAX];
static uint8_t lte_lc_callback_count_occurred;
static uint8_t lte_lc_callback_count_expected;
static char at_notif[2048];

K_SEM_DEFINE(event_handler_called_sem, 0, TEST_EVENT_MAX_COUNT);

/* at_monitor_dispatch() is implemented in at_monitor library and
 * we'll call it directly to fake received AT commands/notifications
 */
extern void at_monitor_dispatch(const char *at_notif);

/* lte_lc_edrx_on_modem_cfun() is implemented in lte_lc library and
 * we'll call it directly to fake nrf_modem_lib call to this function
 */
extern void lte_lc_edrx_on_modem_cfun(int mode, void *ctx);

static void lte_lc_connect_inprogress_work_fn(struct k_work *work)
{
	int ret;

	ret = lte_lc_connect();
	TEST_ASSERT_EQUAL(-EINPROGRESS, ret);
}

static K_WORK_DELAYABLE_DEFINE(lte_lc_connect_inprogress_work, lte_lc_connect_inprogress_work_fn);

static void at_notif_dispatch_work_fn(struct k_work *work)
{
	at_monitor_dispatch(at_notif);
}

static K_WORK_DELAYABLE_DEFINE(at_notif_dispatch_work, at_notif_dispatch_work_fn);

static int lte_lc_event_handler_custom_count;

static void lte_lc_event_handler_custom(const struct lte_lc_evt *const evt)
{
	TEST_ASSERT_EQUAL(test_event_data[lte_lc_event_handler_custom_count].type, evt->type);

	lte_lc_event_handler_custom_count++;
}

static void lte_lc_event_handler(const struct lte_lc_evt *const evt)
{
	uint8_t index = lte_lc_callback_count_occurred;

	TEST_ASSERT_MESSAGE(lte_lc_callback_count_occurred < lte_lc_callback_count_expected,
			    "LTE LC event callback called more times than expected");

	TEST_ASSERT_EQUAL(test_event_data[index].type, evt->type);

	switch (evt->type) {
	case LTE_LC_EVT_NW_REG_STATUS:
		TEST_ASSERT_EQUAL(test_event_data[index].nw_reg_status, evt->nw_reg_status);
		break;

	case LTE_LC_EVT_PSM_UPDATE:
		TEST_ASSERT_EQUAL(test_event_data[index].psm_cfg.tau, evt->psm_cfg.tau);
		TEST_ASSERT_EQUAL(
			test_event_data[index].psm_cfg.active_time, evt->psm_cfg.active_time);
		break;

	case LTE_LC_EVT_EDRX_UPDATE:
		TEST_ASSERT_EQUAL(test_event_data[index].edrx_cfg.mode, evt->edrx_cfg.mode);
		TEST_ASSERT_EQUAL_FLOAT(test_event_data[index].edrx_cfg.edrx, evt->edrx_cfg.edrx);
		TEST_ASSERT_EQUAL_FLOAT(test_event_data[index].edrx_cfg.ptw, evt->edrx_cfg.ptw);
		break;

	case LTE_LC_EVT_RRC_UPDATE:
		TEST_ASSERT_EQUAL(test_event_data[index].rrc_mode, evt->rrc_mode);
		break;

	case LTE_LC_EVT_CELL_UPDATE:
		TEST_ASSERT_EQUAL(test_event_data[index].cell.mcc, evt->cell.mcc);
		TEST_ASSERT_EQUAL(test_event_data[index].cell.mnc, evt->cell.mnc);
		TEST_ASSERT_EQUAL(test_event_data[index].cell.id, evt->cell.id);
		TEST_ASSERT_EQUAL(test_event_data[index].cell.tac, evt->cell.tac);
		TEST_ASSERT_EQUAL(test_event_data[index].cell.earfcn, evt->cell.earfcn);
		TEST_ASSERT_EQUAL(
			test_event_data[index].cell.timing_advance, evt->cell.timing_advance);
		TEST_ASSERT_EQUAL(
			test_event_data[index].cell.timing_advance_meas_time,
			evt->cell.timing_advance_meas_time);
		TEST_ASSERT_EQUAL(
			test_event_data[index].cell.measurement_time, evt->cell.measurement_time);
		TEST_ASSERT_EQUAL(test_event_data[index].cell.phys_cell_id, evt->cell.phys_cell_id);
		TEST_ASSERT_EQUAL(test_event_data[index].cell.rsrp, evt->cell.rsrp);
		TEST_ASSERT_EQUAL(test_event_data[index].cell.rsrq, evt->cell.rsrq);
		break;

	case LTE_LC_EVT_LTE_MODE_UPDATE:
		TEST_ASSERT_EQUAL(test_event_data[index].lte_mode, evt->lte_mode);
		break;

	case LTE_LC_EVT_TAU_PRE_WARNING:
		TEST_ASSERT_EQUAL(test_event_data[index].time, evt->time);
		break;

	case LTE_LC_EVT_NEIGHBOR_CELL_MEAS:
		/* Current cell */
		TEST_ASSERT_EQUAL(
			test_event_data[index].cells_info.current_cell.mcc,
			evt->cells_info.current_cell.mcc);
		TEST_ASSERT_EQUAL(
			test_event_data[index].cells_info.current_cell.mnc,
			evt->cells_info.current_cell.mnc);
		TEST_ASSERT_EQUAL(
			test_event_data[index].cells_info.current_cell.id,
			evt->cells_info.current_cell.id);
		TEST_ASSERT_EQUAL(
			test_event_data[index].cells_info.current_cell.tac,
			evt->cells_info.current_cell.tac);
		TEST_ASSERT_EQUAL(
			test_event_data[index].cells_info.current_cell.earfcn,
			evt->cells_info.current_cell.earfcn);
		TEST_ASSERT_EQUAL(
			test_event_data[index].cells_info.current_cell.timing_advance,
			evt->cells_info.current_cell.timing_advance);
		TEST_ASSERT_EQUAL(
			test_event_data[index].cells_info.current_cell.timing_advance_meas_time,
			evt->cells_info.current_cell.timing_advance_meas_time);
		TEST_ASSERT_EQUAL(
			test_event_data[index].cells_info.current_cell.measurement_time,
			evt->cells_info.current_cell.measurement_time);
		TEST_ASSERT_EQUAL(
			test_event_data[index].cells_info.current_cell.phys_cell_id,
			evt->cells_info.current_cell.phys_cell_id);
		TEST_ASSERT_EQUAL(
			test_event_data[index].cells_info.current_cell.rsrp,
			evt->cells_info.current_cell.rsrp);
		TEST_ASSERT_EQUAL(
			test_event_data[index].cells_info.current_cell.rsrq,
			evt->cells_info.current_cell.rsrq);

		/* Neighbor cells */
		TEST_ASSERT_EQUAL(test_event_data[index].cells_info.ncells_count,
			evt->cells_info.ncells_count);
		for (int i = 0; i < evt->cells_info.ncells_count; i++) {
			TEST_ASSERT_EQUAL(
				test_neighbor_cells[i].earfcn,
				evt->cells_info.neighbor_cells[i].earfcn);
			TEST_ASSERT_EQUAL(
				test_neighbor_cells[i].time_diff,
				evt->cells_info.neighbor_cells[i].time_diff);
			TEST_ASSERT_EQUAL(
				test_neighbor_cells[i].phys_cell_id,
				evt->cells_info.neighbor_cells[i].phys_cell_id);
			TEST_ASSERT_EQUAL(
				test_neighbor_cells[i].rsrp,
				evt->cells_info.neighbor_cells[i].rsrp);
			TEST_ASSERT_EQUAL(
				test_neighbor_cells[i].rsrq,
				evt->cells_info.neighbor_cells[i].rsrq);
		}

		/* Surrounding cells found by the GCI search types */
		TEST_ASSERT_EQUAL(test_event_data[index].cells_info.gci_cells_count,
			evt->cells_info.gci_cells_count);
		for (int i = 0; i < evt->cells_info.gci_cells_count; i++) {
			TEST_ASSERT_EQUAL(test_gci_cells[i].mcc, evt->cells_info.gci_cells[i].mcc);
			TEST_ASSERT_EQUAL(test_gci_cells[i].mnc, evt->cells_info.gci_cells[i].mnc);
			TEST_ASSERT_EQUAL(test_gci_cells[i].id, evt->cells_info.gci_cells[i].id);
			TEST_ASSERT_EQUAL(test_gci_cells[i].tac, evt->cells_info.gci_cells[i].tac);
			TEST_ASSERT_EQUAL(
				test_gci_cells[i].earfcn, evt->cells_info.gci_cells[i].earfcn);
			TEST_ASSERT_EQUAL(
				test_gci_cells[i].timing_advance,
				evt->cells_info.gci_cells[i].timing_advance);
			TEST_ASSERT_EQUAL(
				test_gci_cells[i].timing_advance_meas_time,
				evt->cells_info.gci_cells[i].timing_advance_meas_time);
			TEST_ASSERT_EQUAL(
				test_gci_cells[i].measurement_time,
				evt->cells_info.gci_cells[i].measurement_time);
			TEST_ASSERT_EQUAL(
				test_gci_cells[i].phys_cell_id,
				evt->cells_info.gci_cells[i].phys_cell_id);
			TEST_ASSERT_EQUAL(
				test_gci_cells[i].rsrp, evt->cells_info.gci_cells[i].rsrp);
			TEST_ASSERT_EQUAL(
				test_gci_cells[i].rsrq, evt->cells_info.gci_cells[i].rsrq);
		}
		break;

	case LTE_LC_EVT_MODEM_SLEEP_EXIT_PRE_WARNING:
	case LTE_LC_EVT_MODEM_SLEEP_EXIT:
	case LTE_LC_EVT_MODEM_SLEEP_ENTER:
		TEST_ASSERT_EQUAL(test_event_data[index].modem_sleep.type, evt->modem_sleep.type);
		TEST_ASSERT_EQUAL(test_event_data[index].modem_sleep.time, evt->modem_sleep.time);
		break;

	case LTE_LC_EVT_MODEM_EVENT:
		TEST_ASSERT_EQUAL(test_event_data[index].modem_evt.type, evt->modem_evt.type);
		switch (evt->modem_evt.type) {
		case LTE_LC_MODEM_EVT_CE_LEVEL:
			TEST_ASSERT_EQUAL(
				test_event_data[index].modem_evt.ce_level,
				evt->modem_evt.ce_level);
			break;
		case LTE_LC_MODEM_EVT_INVALID_BAND_CONF:
			TEST_ASSERT_EQUAL(
				test_event_data[index].modem_evt.invalid_band_conf.status_ltem,
				evt->modem_evt.invalid_band_conf.status_ltem);
			TEST_ASSERT_EQUAL(
				test_event_data[index].modem_evt.invalid_band_conf.status_nbiot,
				evt->modem_evt.invalid_band_conf.status_nbiot);
			TEST_ASSERT_EQUAL(
				test_event_data[index].modem_evt.invalid_band_conf.status_ntn_nbiot,
				evt->modem_evt.invalid_band_conf.status_ntn_nbiot);
			break;
		case LTE_LC_MODEM_EVT_DETECTED_COUNTRY:
			TEST_ASSERT_EQUAL(
				test_event_data[index].modem_evt.detected_country,
				evt->modem_evt.detected_country);
			break;
		default:
			/* No payload for this event type */
			break;
		}
		break;

	case LTE_LC_EVT_RAI_UPDATE:
		TEST_ASSERT_EQUAL(test_event_data[index].rai_cfg.cell_id, evt->rai_cfg.cell_id);
		TEST_ASSERT_EQUAL(test_event_data[index].rai_cfg.mcc, evt->rai_cfg.mcc);
		TEST_ASSERT_EQUAL(test_event_data[index].rai_cfg.mnc, evt->rai_cfg.mnc);
		TEST_ASSERT_EQUAL(test_event_data[index].rai_cfg.as_rai, evt->rai_cfg.as_rai);
		TEST_ASSERT_EQUAL(test_event_data[index].rai_cfg.cp_rai, evt->rai_cfg.cp_rai);
		break;

	case LTE_LC_EVT_ENV_EVAL_RESULT:
		TEST_ASSERT_EQUAL(
			test_event_data[index].env_eval_result.status,
			evt->env_eval_result.status);
		TEST_ASSERT_EQUAL(
			test_event_data[index].env_eval_result.result_count,
			evt->env_eval_result.result_count);
		for (int i = 0; i < evt->env_eval_result.result_count; i++) {
			TEST_ASSERT_EQUAL(
				test_event_data[index].env_eval_result.results[i].rrc_state,
				evt->env_eval_result.results[i].rrc_state);
			TEST_ASSERT_EQUAL(
				test_event_data[index].env_eval_result.results[i].energy_estimate,
				evt->env_eval_result.results[i].energy_estimate);
			TEST_ASSERT_EQUAL(
				test_event_data[index].env_eval_result.results[i].tau_trig,
				evt->env_eval_result.results[i].tau_trig);
			TEST_ASSERT_EQUAL(
				test_event_data[index].env_eval_result.results[i].ce_level,
				evt->env_eval_result.results[i].ce_level);
			TEST_ASSERT_EQUAL(
				test_event_data[index].env_eval_result.results[i].earfcn,
				evt->env_eval_result.results[i].earfcn);
			TEST_ASSERT_EQUAL(
				test_event_data[index].env_eval_result.results[i].dl_pathloss,
				evt->env_eval_result.results[i].dl_pathloss);
			TEST_ASSERT_EQUAL(
				test_event_data[index].env_eval_result.results[i].rsrp,
				evt->env_eval_result.results[i].rsrp);
			TEST_ASSERT_EQUAL(
				test_event_data[index].env_eval_result.results[i].rsrq,
				evt->env_eval_result.results[i].rsrq);
			TEST_ASSERT_EQUAL(
				test_event_data[index].env_eval_result.results[i].tx_rep,
				evt->env_eval_result.results[i].tx_rep);
			TEST_ASSERT_EQUAL(
				test_event_data[index].env_eval_result.results[i].rx_rep,
				evt->env_eval_result.results[i].rx_rep);
			TEST_ASSERT_EQUAL(
				test_event_data[index].env_eval_result.results[i].phy_cid,
				evt->env_eval_result.results[i].phy_cid);
			TEST_ASSERT_EQUAL(
				test_event_data[index].env_eval_result.results[i].band,
				evt->env_eval_result.results[i].band);
			TEST_ASSERT_EQUAL(
				test_event_data[index].env_eval_result.results[i].snr,
				evt->env_eval_result.results[i].snr);
			TEST_ASSERT_EQUAL(
				test_event_data[index].env_eval_result.results[i].tx_power,
				evt->env_eval_result.results[i].tx_power);
			TEST_ASSERT_EQUAL(
				test_event_data[index].env_eval_result.results[i].mcc,
				evt->env_eval_result.results[i].mcc);
			TEST_ASSERT_EQUAL(
				test_event_data[index].env_eval_result.results[i].mnc,
				evt->env_eval_result.results[i].mnc);
			TEST_ASSERT_EQUAL(
				test_event_data[index].env_eval_result.results[i].cell_id,
				evt->env_eval_result.results[i].cell_id);
		}
		break;

	default:
		TEST_FAIL_MESSAGE("Unhandled test event");
		break;
	}

	lte_lc_callback_count_occurred++;

	k_sem_give(&event_handler_called_sem);
}

static void helper_lte_lc_data_clear(void)
{
	memset(&test_event_data, 0, sizeof(test_event_data));
	memset(&test_neighbor_cells, 0, sizeof(test_neighbor_cells));
	memset(&test_gci_cells, 0, sizeof(test_gci_cells));
}

void setUp(void)
{
	mock_nrf_modem_at_Init();

	helper_lte_lc_data_clear();
	lte_lc_register_handler(lte_lc_event_handler);

	lte_lc_callback_count_occurred = 0;
	lte_lc_callback_count_expected = 0;
	k_sem_reset(&event_handler_called_sem);
}

void tearDown(void)
{
	int ret;

	TEST_ASSERT_EQUAL(lte_lc_callback_count_expected, lte_lc_callback_count_occurred);

	ret = lte_lc_deregister_handler(lte_lc_event_handler);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	mock_nrf_modem_at_Verify();
}

/* This is needed because AT Monitor library is initialized in SYS_INIT. */
static int sys_init_helper(void)
{
	__cmock_nrf_modem_at_notif_handler_set_ExpectAnyArgsAndReturn(0);

	return 0;
}

extern void on_modem_init(int err, void *ctx);
extern void lte_lc_on_modem_cfun(int mode, void *ctx);

void enable_notifications(void)
{
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CEREG=5", EXIT_SUCCESS);
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CSCON=1", EXIT_SUCCESS);
	if (IS_ENABLED(CONFIG_LTE_LC_MODEM_SLEEP_NOTIFICATIONS)) {
		__mock_nrf_modem_at_printf_ExpectAndReturn("AT%XMODEMSLEEP=1,5000,1200000",
							   EXIT_SUCCESS);
	}
	if (IS_ENABLED(CONFIG_LTE_LC_TAU_PRE_WARNING_NOTIFICATIONS)) {
		__mock_nrf_modem_at_printf_ExpectAndReturn("AT%XT3412=1,5000,1200000",
							   EXIT_SUCCESS);
	}
}

/* Helper function to initialize LTE LC with the given firmware version. */
void lte_lc_on_modem_init_with_firmware(const char *cgmr_resp, uint16_t cgmr_resp_len)
{
	/* Read modem firmware type. */
	__cmock_nrf_modem_at_cmd_ExpectAndReturn(NULL, 0, "AT+CGMR", 0);
	__cmock_nrf_modem_at_cmd_IgnoreArg_buf();
	__cmock_nrf_modem_at_cmd_IgnoreArg_len();
	__cmock_nrf_modem_at_cmd_ReturnArrayThruPtr_buf((char *)cgmr_resp, cgmr_resp_len);
	/* lte_lc_system_mode_set() */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%XSYSTEMMODE=1,1,1,3", EXIT_SUCCESS);
	/* lte_lc_psm_req(false) */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CPSMS=", EXIT_SUCCESS);
	/* lte_lc_proprietary_psm_req() */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%FEACONF=0,0,0", EXIT_SUCCESS);
	/* CONFIG_LTE_PLMN_SELECTION_OPTIMIZATION=y */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%FEACONF=0,3,1", EXIT_SUCCESS);
	/* lte_lc_edrx_req(false) */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CEDRXS=3", EXIT_SUCCESS);
	/* CONFIG_LTE_RAI_REQ=n */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%RAI=0", EXIT_SUCCESS);
	/* CONFIG_LTE_LC_DNS_FALLBACK_MODULE */
	__cmock_nrf_inet_pton_ExpectAndReturn(NRF_AF_INET, IGNORE, IGNORE, 0 /* error */);
	__cmock_nrf_inet_pton_IgnoreArg_src();
	__cmock_nrf_inet_pton_IgnoreArg_dst();
	/* Must try IPv6 if IPv4 fails */
	__cmock_nrf_inet_pton_ExpectAndReturn(NRF_AF_INET6, IGNORE, IGNORE, 1 /* success */);
	__cmock_nrf_inet_pton_IgnoreArg_src();
	__cmock_nrf_inet_pton_IgnoreArg_dst();
	__cmock_nrf_setdnsaddr_ExpectAndReturn(NRF_AF_INET6, IGNORE, sizeof(struct nrf_in6_addr),
					       EXIT_SUCCESS);
	__cmock_nrf_setdnsaddr_IgnoreArg_in_addr();

	on_modem_init(0, NULL);
}

void test_lte_lc_on_modem_init_success(void)
{
	lte_lc_on_modem_init_with_firmware(cgmr_resp_unknown, sizeof(cgmr_resp_unknown));
}

void test_lte_lc_on_modem_init_error_fail(void)
{
	on_modem_init(-NRF_EFAULT, NULL);
}

void test_lte_lc_on_modem_init_all_fails(void)
{
	/* Read modem firmware type. */
	__cmock_nrf_modem_at_cmd_ExpectAndReturn(NULL, 0, "AT+CGMR", 0);
	__cmock_nrf_modem_at_cmd_IgnoreArg_buf();
	__cmock_nrf_modem_at_cmd_IgnoreArg_len();
	__cmock_nrf_modem_at_cmd_ReturnArrayThruPtr_buf(
		(char *)cgmr_resp_unknown,
		sizeof(cgmr_resp_unknown));
	/* lte_lc_system_mode_set() */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%XSYSTEMMODE=1,1,1,3", -NRF_EFAULT);
	/* lte_lc_psm_req(false) */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CPSMS=", -NRF_EFAULT);
	/* lte_lc_proprietary_psm_req() */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%FEACONF=0,0,0", -NRF_EFAULT);
	/* CONFIG_LTE_PLMN_SELECTION_OPTIMIZATION=y */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%FEACONF=0,3,1", -NRF_EFAULT);
	/* lte_lc_edrx_req(false) */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CEDRXS=3", -NRF_EFAULT);
	/* CONFIG_LTE_RAI_REQ=n */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%RAI=0", -NRF_EFAULT);
	/* CONFIG_LTE_LC_DNS_FALLBACK_MODULE */
	__cmock_nrf_inet_pton_ExpectAndReturn(NRF_AF_INET, IGNORE, IGNORE, 0 /* error */);
	__cmock_nrf_inet_pton_IgnoreArg_src();
	__cmock_nrf_inet_pton_IgnoreArg_dst();
	__cmock_nrf_inet_pton_ExpectAndReturn(NRF_AF_INET6, IGNORE, IGNORE, 0 /* error */);
	__cmock_nrf_inet_pton_IgnoreArg_src();
	__cmock_nrf_inet_pton_IgnoreArg_dst();

	on_modem_init(0, NULL);
}

void test_lte_lc_on_modem_init_dns_fail(void)
{
	/* Read modem firmware type. */
	__cmock_nrf_modem_at_cmd_ExpectAndReturn(NULL, 0, "AT+CGMR", 0);
	__cmock_nrf_modem_at_cmd_IgnoreArg_buf();
	__cmock_nrf_modem_at_cmd_IgnoreArg_len();
	__cmock_nrf_modem_at_cmd_ReturnArrayThruPtr_buf(
		(char *)cgmr_resp_unknown,
		sizeof(cgmr_resp_unknown));
	/* lte_lc_system_mode_set() */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%XSYSTEMMODE=1,1,1,3", EXIT_SUCCESS);
	/* lte_lc_psm_req(false) */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CPSMS=", EXIT_SUCCESS);
	/* lte_lc_proprietary_psm_req() */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%FEACONF=0,0,0", EXIT_SUCCESS);
	/* CONFIG_LTE_PLMN_SELECTION_OPTIMIZATION=y */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%FEACONF=0,3,1", EXIT_SUCCESS);
	/* lte_lc_edrx_req(false) */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CEDRXS=3", EXIT_SUCCESS);
	/* CONFIG_LTE_RAI_REQ=n */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%RAI=0", EXIT_SUCCESS);
	/* CONFIG_LTE_LC_DNS_FALLBACK_MODULE */
	__cmock_nrf_inet_pton_ExpectAndReturn(NRF_AF_INET, IGNORE, IGNORE, 1 /* success */);
	__cmock_nrf_inet_pton_IgnoreArg_src();
	__cmock_nrf_inet_pton_IgnoreArg_dst();
	/* Setting IPv4 DNS address fails */
	__cmock_nrf_setdnsaddr_ExpectAndReturn(NRF_AF_INET, IGNORE, sizeof(struct nrf_in_addr),
					       -1);
	__cmock_nrf_setdnsaddr_IgnoreArg_in_addr();

	on_modem_init(0, NULL);

	/* Read modem firmware type. */
	__cmock_nrf_modem_at_cmd_ExpectAndReturn(NULL, 0, "AT+CGMR", 0);
	__cmock_nrf_modem_at_cmd_IgnoreArg_buf();
	__cmock_nrf_modem_at_cmd_IgnoreArg_len();
	__cmock_nrf_modem_at_cmd_ReturnArrayThruPtr_buf(
		(char *)cgmr_resp_unknown,
		sizeof(cgmr_resp_unknown));
	/* lte_lc_system_mode_set() */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%XSYSTEMMODE=1,1,1,3", EXIT_SUCCESS);
	/* lte_lc_psm_req(false) */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CPSMS=", EXIT_SUCCESS);
	/* lte_lc_proprietary_psm_req() */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%FEACONF=0,0,0", EXIT_SUCCESS);
	/* CONFIG_LTE_PLMN_SELECTION_OPTIMIZATION=y */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%FEACONF=0,3,1", EXIT_SUCCESS);
	/* lte_lc_edrx_req(false) */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CEDRXS=3", EXIT_SUCCESS);
	/* CONFIG_LTE_RAI_REQ=n */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%RAI=0", EXIT_SUCCESS);
	/* CONFIG_LTE_LC_DNS_FALLBACK_MODULE */
	__cmock_nrf_inet_pton_ExpectAndReturn(NRF_AF_INET, IGNORE, IGNORE, 0 /* error */);
	__cmock_nrf_inet_pton_IgnoreArg_src();
	__cmock_nrf_inet_pton_IgnoreArg_dst();
	__cmock_nrf_inet_pton_ExpectAndReturn(NRF_AF_INET6, IGNORE, IGNORE, 1 /* success */);
	__cmock_nrf_inet_pton_IgnoreArg_src();
	__cmock_nrf_inet_pton_IgnoreArg_dst();
	/* Setting IPv6 DNS address fails */
	__cmock_nrf_setdnsaddr_ExpectAndReturn(NRF_AF_INET6, IGNORE, sizeof(struct nrf_in6_addr),
					       -1);
	__cmock_nrf_setdnsaddr_IgnoreArg_in_addr();

	on_modem_init(0, NULL);
}

void test_lte_lc_register_handler_null(void)
{
	lte_lc_register_handler(NULL);
}

void test_lte_lc_deregister_handler_null(void)
{
	int ret;

	ret = lte_lc_deregister_handler(NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

void test_lte_lc_register_handler_twice(void)
{
	int ret;

	lte_lc_register_handler((void *) 1);
	lte_lc_register_handler((void *) 1);
	ret = lte_lc_deregister_handler((void *) 1);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_deregister_handler_unknown(void)
{
	int ret;

	ret = lte_lc_deregister_handler(lte_lc_event_handler_custom);
	TEST_ASSERT_EQUAL(0, ret);
}

void test_lte_lc_connect_success(void)
{
	int ret;

	/* AT commands triggered by lte_lc_connect() */
	__mock_nrf_modem_at_scanf_ExpectAndReturn("AT+CEREG?", "+CEREG: %*u,%hu,%*[^,],\"%x\",", 1);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(LTE_LC_NW_REG_NOT_REGISTERED);

	__mock_nrf_modem_at_scanf_ExpectAndReturn("AT+CFUN?", "+CFUN: %hu", 1);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(LTE_LC_FUNC_MODE_OFFLINE);

	enable_notifications();
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CFUN=1", 0);

	/* AT commands triggered by lte_lc_offline() */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CFUN=4", 0);

	lte_lc_callback_count_expected = 8;

	/* LTE-M registration */
	test_event_data[0].type = LTE_LC_EVT_NW_REG_STATUS;
	test_event_data[0].nw_reg_status = LTE_LC_NW_REG_REGISTERED_HOME;

	test_event_data[1].type = LTE_LC_EVT_CELL_UPDATE;
	test_event_data[1].cell.id = 0x12beef;
	test_event_data[1].cell.tac = 0x2f;

	test_event_data[2].type = LTE_LC_EVT_LTE_MODE_UPDATE;
	test_event_data[2].lte_mode = LTE_LC_LTE_MODE_LTEM;

	test_event_data[3].type = LTE_LC_EVT_PSM_UPDATE;
	test_event_data[3].psm_cfg.tau = 11400;
	test_event_data[3].psm_cfg.active_time = 10;

	/* De-registration */
	test_event_data[4].type = LTE_LC_EVT_NW_REG_STATUS;
	test_event_data[4].nw_reg_status = LTE_LC_NW_REG_NOT_REGISTERED;

	test_event_data[5].type = LTE_LC_EVT_CELL_UPDATE;
	test_event_data[5].cell.id = LTE_LC_CELL_EUTRAN_ID_INVALID;
	test_event_data[5].cell.tac = LTE_LC_CELL_TAC_INVALID;

	test_event_data[6].type = LTE_LC_EVT_LTE_MODE_UPDATE;
	test_event_data[6].lte_mode = LTE_LC_LTE_MODE_NONE;

	test_event_data[7].type = LTE_LC_EVT_PSM_UPDATE;
	test_event_data[7].psm_cfg.tau = -1;
	test_event_data[7].psm_cfg.active_time = -1;

	/* Schedule another lte_lc_connect which will cause -EINPOGRESS */
	k_work_schedule(&lte_lc_connect_inprogress_work, K_MSEC(1));

	/* Schedule +CEREG notification to be dispatched while lte_lc_connect() blocks */
	strcpy(at_notif, "+CEREG: 1,\"002F\",\"0012BEEF\",7,,,\"00000101\",\"00010011\"\r\n");
	k_work_schedule(&at_notif_dispatch_work, K_MSEC(2));

	ret = lte_lc_connect();
	TEST_ASSERT_EQUAL(0, ret);

	ret = lte_lc_offline();
	TEST_ASSERT_EQUAL(0, ret);

	/* Send +CEREG notification to trigger de-registration events */
	strcpy(at_notif, "+CEREG: 0\r\n");
	at_monitor_dispatch(at_notif);
}

void test_lte_lc_connect_already_registered_success(void)
{
	int ret;

	__mock_nrf_modem_at_scanf_ExpectAndReturn("AT+CEREG?", "+CEREG: %*u,%hu,%*[^,],\"%x\",", 2);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(LTE_LC_NW_REG_REGISTERED_HOME);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(0x12345678);

	ret = lte_lc_connect();
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	__mock_nrf_modem_at_scanf_ExpectAndReturn("AT+CEREG?", "+CEREG: %*u,%hu,%*[^,],\"%x\",", 2);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(LTE_LC_NW_REG_REGISTERED_ROAMING);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(0x87654321);

	ret = lte_lc_connect_async(NULL);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_connect_timeout_fail(void)
{
	int ret;

	/* AT commands triggered by lte_lc_connect() */
	__mock_nrf_modem_at_scanf_ExpectAndReturn("AT+CEREG?", "+CEREG: %*u,%hu,%*[^,],\"%x\",", 1);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(LTE_LC_NW_REG_NOT_REGISTERED);

	__mock_nrf_modem_at_scanf_ExpectAndReturn("AT+CFUN?", "+CFUN: %hu", 1);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(LTE_LC_FUNC_MODE_POWER_OFF);

	enable_notifications();
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CFUN=1", 0);

	/* Modem switched back to original functional mode after the timeout */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CFUN=0", 0);

	lte_lc_callback_count_expected = 1;

	test_event_data[0].type = LTE_LC_EVT_NW_REG_STATUS;
	test_event_data[0].nw_reg_status = LTE_LC_NW_REG_SEARCHING;

	/* Schedule +CEREG notification to be dispatched while lte_lc_connect() blocks */
	strcpy(at_notif, "+CEREG: 2\r\n");
	k_work_schedule(&at_notif_dispatch_work, K_MSEC(1));

	ret = lte_lc_connect();
	TEST_ASSERT_EQUAL(-ETIMEDOUT, ret);
}

void test_lte_lc_connect_cereg_query_fail(void)
{
	int ret;

	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT+CEREG?", "+CEREG: %*u,%hu,%*[^,],\"%x\",", -NRF_ENOMEM);

	ret = lte_lc_connect();
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_connect_cfun_query_fail(void)
{
	int ret;

	__mock_nrf_modem_at_scanf_ExpectAndReturn("AT+CEREG?", "+CEREG: %*u,%hu,%*[^,],\"%x\",", 1);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(LTE_LC_NW_REG_NOT_REGISTERED);

	__mock_nrf_modem_at_scanf_ExpectAndReturn("AT+CFUN?", "+CFUN: %hu", -NRF_ENOMEM);

	ret = lte_lc_connect();
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_connect_cfun_normal_set_fail(void)
{
	int ret;

	__mock_nrf_modem_at_scanf_ExpectAndReturn("AT+CEREG?", "+CEREG: %*u,%hu,%*[^,],\"%x\",", 1);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(LTE_LC_NW_REG_NOT_REGISTERED);

	__mock_nrf_modem_at_scanf_ExpectAndReturn("AT+CFUN?", "+CFUN: %hu", 1);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(LTE_LC_FUNC_MODE_POWER_OFF);

	enable_notifications();
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CFUN=1", -NRF_ENOMEM);

	ret = lte_lc_connect();
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_connect_async_success(void)
{
	int ret;

	/* AT commands triggered by lte_lc_connect() */
	__mock_nrf_modem_at_scanf_ExpectAndReturn("AT+CEREG?", "+CEREG: %*u,%hu,%*[^,],\"%x\",", 2);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(LTE_LC_NW_REG_REGISTERED_HOME);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(LTE_LC_CELL_EUTRAN_ID_INVALID);

	__mock_nrf_modem_at_scanf_ExpectAndReturn("AT+CFUN?", "+CFUN: %hu", 1);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(LTE_LC_FUNC_MODE_OFFLINE);

	enable_notifications();
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CFUN=1", 0);

	/* AT commands triggered by lte_lc_offline() */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CFUN=4", 0);

	lte_lc_callback_count_expected = 6;

	/* LTE-M registration. Status is unknown because cell ID is invalid. */
	test_event_data[0].type = LTE_LC_EVT_NW_REG_STATUS;
	test_event_data[0].nw_reg_status = LTE_LC_NW_REG_UNKNOWN;

	test_event_data[1].type = LTE_LC_EVT_CELL_UPDATE;
	test_event_data[1].cell.id = LTE_LC_CELL_EUTRAN_ID_INVALID;
	test_event_data[1].cell.tac = 0x2f;

	test_event_data[2].type = LTE_LC_EVT_LTE_MODE_UPDATE;
	test_event_data[2].lte_mode = LTE_LC_LTE_MODE_LTEM;

	/* De-registration */
	test_event_data[3].type = LTE_LC_EVT_NW_REG_STATUS;
	test_event_data[3].nw_reg_status = LTE_LC_NW_REG_NOT_REGISTERED;

	test_event_data[4].type = LTE_LC_EVT_CELL_UPDATE;
	test_event_data[4].cell.id = LTE_LC_CELL_EUTRAN_ID_INVALID;
	test_event_data[4].cell.tac = LTE_LC_CELL_TAC_INVALID;

	test_event_data[5].type = LTE_LC_EVT_LTE_MODE_UPDATE;
	test_event_data[5].lte_mode = LTE_LC_LTE_MODE_NONE;

	ret = lte_lc_connect_async(lte_lc_event_handler_custom);
	TEST_ASSERT_EQUAL(0, ret);
	k_sleep(K_MSEC(1));

	strcpy(at_notif, "+CEREG: 1,\"002F\",\"FFFFFFFF\",7,,,\"00000101\",\"00010011\"\r\n");
	at_monitor_dispatch(at_notif);
	k_sleep(K_MSEC(1));

	ret = lte_lc_offline();
	TEST_ASSERT_EQUAL(0, ret);
	k_sleep(K_MSEC(1));

	/* Send +CEREG notification to trigger de-registration events */
	strcpy(at_notif, "+CEREG: 0\r\n");
	at_monitor_dispatch(at_notif);
	k_sleep(K_MSEC(1));

	TEST_ASSERT_EQUAL(lte_lc_callback_count_expected, lte_lc_event_handler_custom_count);
	ret = lte_lc_deregister_handler(lte_lc_event_handler_custom);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_connect_async_null_fail(void)
{
	int ret;

	/* Deregister handler set in setUp() so that we have no handlers */
	ret = lte_lc_deregister_handler(lte_lc_event_handler);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	ret = lte_lc_connect_async(NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	/* Register handler so that tearDown() doesn't cause unnecessary warning log */
	lte_lc_register_handler(lte_lc_event_handler);
}

void test_lte_lc_normal_success(void)
{
	int ret;

	enable_notifications();
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CFUN=1", EXIT_SUCCESS);
	ret = lte_lc_normal();
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_normal_cereg_fail(void)
{
	int ret;

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CEREG=5", -NRF_ENOMEM);
	ret = lte_lc_normal();
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_normal_cscon_fail(void)
{
	int ret;

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CEREG=5", EXIT_SUCCESS);
	/* enable_notifications */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CSCON=1", -NRF_ENOMEM);

	ret = lte_lc_normal();
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_normal_cfun_fail(void)
{
	int ret;

	enable_notifications();
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CFUN=1", -NRF_ENOMEM);
	ret = lte_lc_normal();
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_offline_success(void)
{
	int ret;

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CFUN=4", EXIT_SUCCESS);
	ret = lte_lc_offline();
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_offline_fail(void)
{
	int ret;

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CFUN=4", -NRF_ENOMEM);
	ret = lte_lc_offline();
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_power_off_success(void)
{
	int ret;

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CFUN=0", EXIT_SUCCESS);
	ret = lte_lc_power_off();
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_power_off_fail(void)
{
	int ret;

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CFUN=0", -NRF_ENOMEM);
	ret = lte_lc_power_off();
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_rx_only(void)
{
	int ret;

	enable_notifications();
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CFUN=2", 0);

	enable_notifications();
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CFUN=1", 0);

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CFUN=4", 0);

	lte_lc_callback_count_expected = 9;

	/* Events caused by PLMN search in RX only mode, registration status is not sent
	 * because it has not changed.
	 */

	test_event_data[0].type = LTE_LC_EVT_CELL_UPDATE;
	test_event_data[0].cell.id = 0x001b8b1e;
	test_event_data[0].cell.tac = 0x0138;

	test_event_data[1].type = LTE_LC_EVT_LTE_MODE_UPDATE;
	test_event_data[1].lte_mode = LTE_LC_LTE_MODE_LTEM;

	test_event_data[2].type = LTE_LC_EVT_MODEM_EVENT;
	test_event_data[2].modem_evt.type = LTE_LC_MODEM_EVT_SEARCH_DONE;

	/* Events caused by normal mode */

	test_event_data[3].type = LTE_LC_EVT_NW_REG_STATUS;
	test_event_data[3].nw_reg_status = LTE_LC_NW_REG_REGISTERED_HOME;

	test_event_data[4].type = LTE_LC_EVT_PSM_UPDATE;
	test_event_data[4].psm_cfg.tau = 11400;
	test_event_data[4].psm_cfg.active_time = -1;

	/* Events caused by offline mode */

	test_event_data[5].type = LTE_LC_EVT_NW_REG_STATUS;
	test_event_data[5].nw_reg_status = LTE_LC_NW_REG_NOT_REGISTERED;

	test_event_data[6].type = LTE_LC_EVT_CELL_UPDATE;
	test_event_data[6].cell.id = LTE_LC_CELL_EUTRAN_ID_INVALID;
	test_event_data[6].cell.tac = LTE_LC_CELL_TAC_INVALID;

	test_event_data[7].type = LTE_LC_EVT_LTE_MODE_UPDATE;
	test_event_data[7].lte_mode = LTE_LC_LTE_MODE_NONE;

	test_event_data[8].type = LTE_LC_EVT_PSM_UPDATE;
	test_event_data[8].psm_cfg.tau = -1;
	test_event_data[8].psm_cfg.active_time = -1;

	/* RX only mode with PLMN search */

	ret = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_RX_ONLY);
	TEST_ASSERT_EQUAL(0, ret);

	strcpy(at_notif, "%MDMEV: SEARCH STATUS 2\r\n");
	at_monitor_dispatch(at_notif);

	strcpy(at_notif, "+CEREG: 0,\"0138\",\"001B8B1E\",7\r\n");
	at_monitor_dispatch(at_notif);

	/* Sleep until delayed LTE_LC_MODEM_EVT_SEARCH_DONE is dispatched */
	k_sleep(K_MSEC(150));

	/* Normal mode */

	ret = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_NORMAL);
	TEST_ASSERT_EQUAL(0, ret);

	strcpy(at_notif, "+CEREG: 1,\"0138\",\"001B8B1E\",7,,,\"11100000\",\"00010011\"\r\n");
	at_monitor_dispatch(at_notif);

	/* Offline mode*/

	ret = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_OFFLINE);
	TEST_ASSERT_EQUAL(0, ret);

	strcpy(at_notif, "+CEREG: 0\r\n");
	at_monitor_dispatch(at_notif);

	k_sleep(K_MSEC(1));
}

void test_lte_lc_psm_param_set_good_weather(void)
{
	int ret;

	ret = lte_lc_psm_param_set("00000111", "01001001");
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_psm_param_set_both_null(void)
{
	int ret;

	ret = lte_lc_psm_param_set(NULL, NULL);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_psm_param_set_rptau_too_short(void)
{
	int ret;

	ret = lte_lc_psm_param_set("", NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

void test_lte_lc_psm_param_set_rat_too_short(void)
{
	int ret;

	ret = lte_lc_psm_param_set(NULL, "");
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

void test_lte_lc_psm_req_enable_both_default(void)
{
	int ret;

	ret = lte_lc_psm_param_set(NULL, NULL);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CPSMS=1", EXIT_SUCCESS);
	ret = lte_lc_psm_req(true);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_psm_req_enable_rptau_default(void)
{
	int ret;

	ret = lte_lc_psm_param_set(NULL, "01001001");
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CPSMS=1,,,,\"01001001\"", EXIT_SUCCESS);
	ret = lte_lc_psm_req(true);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_psm_req_enable_rat_default(void)
{
	int ret;

	ret = lte_lc_psm_param_set("00000111", NULL);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CPSMS=1,,,\"00000111\"", EXIT_SUCCESS);
	ret = lte_lc_psm_req(true);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_psm_req_enable_both_set(void)
{
	int ret;

	ret = lte_lc_psm_param_set("00000111", "01001001");
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	__mock_nrf_modem_at_printf_ExpectAndReturn(
		"AT+CPSMS=1,,,\"00000111\",\"01001001\"", EXIT_SUCCESS);
	ret = lte_lc_psm_req(true);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_psm_req_enable_fail(void)
{
	int ret;

	ret = lte_lc_psm_param_set(NULL, NULL);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CPSMS=1", NRF_MODEM_AT_ERROR);
	ret = lte_lc_psm_req(true);
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_psm_req_disable_success(void)
{
	int ret;

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CPSMS=", EXIT_SUCCESS);
	ret = lte_lc_psm_req(false);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_psm_param_set_seconds_enable_both_set_1(void)
{
	int ret;

	ret = lte_lc_psm_param_set_seconds(60, 600);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	__mock_nrf_modem_at_printf_ExpectAndReturn(
		"AT+CPSMS=1,,,\"01111110\",\"00101010\"", EXIT_SUCCESS);
	ret = lte_lc_psm_req(true);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_psm_param_set_seconds_enable_both_set_2(void)
{
	int ret;

	ret = lte_lc_psm_param_set_seconds(123456, 89);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	__mock_nrf_modem_at_printf_ExpectAndReturn(
		"AT+CPSMS=1,,,\"01000100\",\"00100010\"", EXIT_SUCCESS);
	ret = lte_lc_psm_req(true);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_psm_param_set_seconds_enable_both_zero(void)
{
	int ret;

	ret = lte_lc_psm_param_set_seconds(0, 0);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	__mock_nrf_modem_at_printf_ExpectAndReturn(
		"AT+CPSMS=1,,,\"00000000\",\"00000000\"", EXIT_SUCCESS);
	ret = lte_lc_psm_req(true);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_psm_param_set_seconds_enable_both_default(void)
{
	int ret;

	ret = lte_lc_psm_param_set_seconds(-1, -1);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CPSMS=1", EXIT_SUCCESS);
	ret = lte_lc_psm_req(true);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_psm_param_set_seconds_enable_both_1_rounding_to_2(void)
{
	int ret;

	ret = lte_lc_psm_param_set_seconds(1, 1);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	__mock_nrf_modem_at_printf_ExpectAndReturn(
		"AT+CPSMS=1,,,\"01100001\",\"00000001\"", EXIT_SUCCESS);
	ret = lte_lc_psm_req(true);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_psm_param_set_seconds_enable_both_max_values(void)
{
	int ret;

	ret = lte_lc_psm_param_set_seconds(35712000, 11160);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	__mock_nrf_modem_at_printf_ExpectAndReturn(
		"AT+CPSMS=1,,,\"11011111\",\"01011111\"", EXIT_SUCCESS);
	ret = lte_lc_psm_req(true);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_psm_param_set_seconds_tau_too_big(void)
{
	int ret;

	ret = lte_lc_psm_param_set_seconds(35712001, 61);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CPSMS=1", EXIT_SUCCESS);
	ret = lte_lc_psm_req(true);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_psm_param_set_seconds_active_time_too_big(void)
{
	int ret;

	ret = lte_lc_psm_param_set_seconds(61, 11161);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CPSMS=1", EXIT_SUCCESS);
	ret = lte_lc_psm_req(true);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_psm_get_null_fail(void)
{
	int ret;
	int tau;
	int active_time;

	ret = lte_lc_psm_get(NULL, &active_time);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	ret = lte_lc_psm_get(&tau, NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

void test_lte_lc_psm_get_badmsg_no_reg_status_number_fail(void)
{
	int ret;
	int tau;
	int active_time;

	/* <reg_status> as string instead of number to make getting it fail */
	static const char xmonitor_resp[] =
		"%XMONITOR: invalid";
	__cmock_nrf_modem_at_cmd_ExpectAndReturn(NULL, 0, "AT%%XMONITOR", 0);
	__cmock_nrf_modem_at_cmd_IgnoreArg_buf();
	__cmock_nrf_modem_at_cmd_IgnoreArg_len();
	__cmock_nrf_modem_at_cmd_ReturnArrayThruPtr_buf(
		(char *)xmonitor_resp, sizeof(xmonitor_resp));

	ret = lte_lc_psm_get(&tau, &active_time);
	TEST_ASSERT_EQUAL(-EBADMSG, ret);
}

void test_lte_lc_psm_get_badmsg_no_active_time_fail(void)
{
	int ret;
	int tau;
	int active_time;

	static const char xmonitor_resp[] =
		"%XMONITOR: 1,\"Operator\",\"OP\",\"20065\",\"002F\",7,20,\"0012BEEF\","
		"334,6200,66,44,\"\"";
	__cmock_nrf_modem_at_cmd_ExpectAndReturn(NULL, 0, "AT%%XMONITOR", 0);
	__cmock_nrf_modem_at_cmd_IgnoreArg_buf();
	__cmock_nrf_modem_at_cmd_IgnoreArg_len();
	__cmock_nrf_modem_at_cmd_ReturnArrayThruPtr_buf(
		(char *)xmonitor_resp, sizeof(xmonitor_resp));

	ret = lte_lc_psm_get(&tau, &active_time);
	TEST_ASSERT_EQUAL(-EBADMSG, ret);
}

void test_lte_lc_psm_get_badmsg_no_tau_ext_fail(void)
{
	int ret;
	int tau;
	int active_time;

	static const char xmonitor_resp[] =
		"%XMONITOR: 1,\"Operator\",\"OP\",\"20065\",\"002F\",7,20,\"0012BEEF\","
		"334,6200,66,44,\"\",\"00000101\"";
	__cmock_nrf_modem_at_cmd_ExpectAndReturn(NULL, 0, "AT%%XMONITOR", 0);
	__cmock_nrf_modem_at_cmd_IgnoreArg_buf();
	__cmock_nrf_modem_at_cmd_IgnoreArg_len();
	__cmock_nrf_modem_at_cmd_ReturnArrayThruPtr_buf(
		(char *)xmonitor_resp, sizeof(xmonitor_resp));

	ret = lte_lc_psm_get(&tau, &active_time);
	TEST_ASSERT_EQUAL(-EBADMSG, ret);
}

void test_lte_lc_psm_get_badmsg_no_legacy_tau_fail(void)
{
	int ret;
	int tau;
	int active_time;

	static const char xmonitor_resp[] =
		"%XMONITOR: 1,\"Operator\",\"OP\",\"20065\",\"002F\",7,20,\"0012BEEF\","
		"334,6200,66,44,\"\",\"00000101\",\"00010011\"";
	__cmock_nrf_modem_at_cmd_ExpectAndReturn(NULL, 0, "AT%%XMONITOR", 0);
	__cmock_nrf_modem_at_cmd_IgnoreArg_buf();
	__cmock_nrf_modem_at_cmd_IgnoreArg_len();
	__cmock_nrf_modem_at_cmd_ReturnArrayThruPtr_buf(
		(char *)xmonitor_resp, sizeof(xmonitor_resp));

	ret = lte_lc_psm_get(&tau, &active_time);
	TEST_ASSERT_EQUAL(-EBADMSG, ret);
}

void test_lte_lc_psm_get_badmsg_legacy_tau_len_not_8_fail(void)
{
	int ret;
	int tau;
	int active_time;

	static const char xmonitor_resp[] =
		"%XMONITOR: 1,\"Operator\",\"OP\",\"20065\",\"002F\",7,20,\"0012BEEF\","
		"334,6200,66,44,\"\",\"00000101\",\"00010011\",\"001001\"";
	__cmock_nrf_modem_at_cmd_ExpectAndReturn(NULL, 0, "AT%%XMONITOR", 0);
	__cmock_nrf_modem_at_cmd_IgnoreArg_buf();
	__cmock_nrf_modem_at_cmd_IgnoreArg_len();
	__cmock_nrf_modem_at_cmd_ReturnArrayThruPtr_buf(
		(char *)xmonitor_resp, sizeof(xmonitor_resp));

	ret = lte_lc_psm_get(&tau, &active_time);
	TEST_ASSERT_EQUAL(-EBADMSG, ret);
}

void test_lte_lc_psm_get_badmsg_tau_ext_len_not_8_fail(void)
{
	int ret;
	int tau;
	int active_time;

	static const char xmonitor_resp[] =
		"%XMONITOR: 1,\"Operator\",\"OP\",\"20065\",\"002F\",7,20,\"0012BEEF\","
		"334,6200,66,44,\"\",\"00000101\",\"001001\",\"00100101\"";
	__cmock_nrf_modem_at_cmd_ExpectAndReturn(NULL, 0, "AT%%XMONITOR", 0);
	__cmock_nrf_modem_at_cmd_IgnoreArg_buf();
	__cmock_nrf_modem_at_cmd_IgnoreArg_len();
	__cmock_nrf_modem_at_cmd_ReturnArrayThruPtr_buf(
		(char *)xmonitor_resp, sizeof(xmonitor_resp));

	ret = lte_lc_psm_get(&tau, &active_time);
	TEST_ASSERT_EQUAL(-EBADMSG, ret);
}

void test_lte_lc_psm_get_badmsg_invalid_reg_status_fail(void)
{
	int ret;
	int tau;
	int active_time;

	/* <reg_status> is unknown to make sure we don't parse the rest of the response.
	 * the rest of the response is intentionally valid so that we would see a successful
	 * return value if unknown status would be accepted.
	 */
	static const char xmonitor_resp[] =
		"%XMONITOR: 4,\"Operator\",\"OP\",\"20065\",\"002F\",7,20,\"0012BEEF\","
		"334,6200,66,44,\"\",\"00000101\",\"00010011\",\"00100101\"";
	__cmock_nrf_modem_at_cmd_ExpectAndReturn(NULL, 0, "AT%%XMONITOR", 0);
	__cmock_nrf_modem_at_cmd_IgnoreArg_buf();
	__cmock_nrf_modem_at_cmd_IgnoreArg_len();
	__cmock_nrf_modem_at_cmd_ReturnArrayThruPtr_buf(
		(char *)xmonitor_resp, sizeof(xmonitor_resp));

	ret = lte_lc_psm_get(&tau, &active_time);
	TEST_ASSERT_EQUAL(-EBADMSG, ret);
}

void test_lte_lc_proprietary_psm_req_enable_success(void)
{
	int ret;

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%FEACONF=0,0,1", EXIT_SUCCESS);
	ret = lte_lc_proprietary_psm_req(true);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_proprietary_psm_req_disable_success(void)
{
	int ret;

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%FEACONF=0,0,0", EXIT_SUCCESS);
	ret = lte_lc_proprietary_psm_req(false);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_proprietary_psm_req_enable_fail(void)
{
	int ret;

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%FEACONF=0,0,1", -NRF_ENOMEM);
	ret = lte_lc_proprietary_psm_req(true);
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_edrx_param_set_wrong_mode(void)
{
	int ret;

	ret = lte_lc_edrx_param_set(LTE_LC_LTE_MODE_NONE, NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

void test_lte_lc_edrx_param_set_wrong_edrx(void)
{
	int ret;

	ret = lte_lc_edrx_param_set(LTE_LC_LTE_MODE_LTEM, "");
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

void test_lte_lc_edrx_param_set_empty_edrx(void)
{
	int ret;

	ret = lte_lc_edrx_param_set(LTE_LC_LTE_MODE_LTEM, NULL);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_edrx_param_set_valid_edrx(void)
{
	int ret;

	ret = lte_lc_edrx_param_set(LTE_LC_LTE_MODE_LTEM, "0000");
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_ptw_wrong_mode_fail(void)
{
	int ret;

	ret = lte_lc_ptw_set(LTE_LC_LTE_MODE_NONE, NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

void test_lte_lc_ptw_wrong_ptw_fail(void)
{
	int ret;

	ret = lte_lc_ptw_set(LTE_LC_LTE_MODE_LTEM, "");
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

void test_lte_lc_ptw_empty_ptw(void)
{
	int ret;

	ret = lte_lc_ptw_set(LTE_LC_LTE_MODE_LTEM, NULL);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_ptw_valid_ptw(void)
{
	int ret;

	ret = lte_lc_ptw_set(LTE_LC_LTE_MODE_LTEM, "0000");
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_edrx_req_disable_fail(void)
{
	int ret;

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CEDRXS=3", -NRF_ENOMEM);
	ret = lte_lc_edrx_req(false);
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_edrx_req_disable_success(void)
{
	int ret;

	lte_lc_callback_count_expected = 1;

	test_event_data[0].type = LTE_LC_EVT_EDRX_UPDATE;
	test_event_data[0].edrx_cfg.mode = LTE_LC_LTE_MODE_NONE;

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CEDRXS=3", EXIT_SUCCESS);
	ret = lte_lc_edrx_req(false);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	strcpy(at_notif, "+CEDRXP: 0\r\n");
	at_monitor_dispatch(at_notif);
	k_sleep(K_MSEC(1));

	/* eDRX is disabled, so no AT command expected for it on +CFUN=0. */
	/* CONFIG_LTE_RAI_REQ=n */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%RAI=0", EXIT_SUCCESS);
	/* CONFIG_LTE_LC_DNS_FALLBACK_MODULE */
	__cmock_nrf_inet_pton_ExpectAndReturn(NRF_AF_INET, IGNORE, IGNORE, 1 /* success */);
	__cmock_nrf_inet_pton_IgnoreArg_src();
	__cmock_nrf_inet_pton_IgnoreArg_dst();
	__cmock_nrf_setdnsaddr_ExpectAndReturn(NRF_AF_INET, IGNORE, sizeof(struct nrf_in_addr),
					       EXIT_SUCCESS);
	__cmock_nrf_setdnsaddr_IgnoreArg_in_addr();

	lte_lc_on_modem_cfun(LTE_LC_FUNC_MODE_POWER_OFF, NULL);
	k_sleep(K_MSEC(1));
}

void test_lte_lc_edrx_req_enable_cedrxs1_fail(void)
{
	int ret;

	/* Executed with unknown firmware type. eDRX for NTN NB-IoT is configured, because
	 * firmware type can not be determined.
	 */

	 ret = lte_lc_edrx_param_set(LTE_LC_LTE_MODE_NTN_NBIOT, "0001");
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CEDRXS=2,6,\"0001\"", -NRF_ENOMEM);
	ret = lte_lc_edrx_req(true);
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_edrx_req_enable_cedrxs2_fail(void)
{
	int ret;

	/* Executed with nrf9160 firmware, NTN NB-IoT not supported. */

	lte_lc_on_modem_init_with_firmware(cgmr_resp_9160, sizeof(cgmr_resp_9160));

	ret = lte_lc_edrx_param_set(LTE_LC_LTE_MODE_LTEM, "0101");
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	ret = lte_lc_edrx_param_set(LTE_LC_LTE_MODE_NBIOT, "0011");
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CEDRXS=2,4,\"0101\"", EXIT_SUCCESS);
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CEDRXS=2,5,\"0011\"", -NRF_ENOMEM);
	ret = lte_lc_edrx_req(true);
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_edrx_req_enable_success(void)
{
	int ret;

	/* Executed with nrf91x1 firmware, NTN NB-IoT not supported. */

	lte_lc_on_modem_init_with_firmware(cgmr_resp_91x1, sizeof(cgmr_resp_91x1));

	lte_lc_callback_count_expected = 3;

	test_event_data[0].type = LTE_LC_EVT_EDRX_UPDATE;
	test_event_data[0].edrx_cfg.mode = LTE_LC_LTE_MODE_LTEM;
	test_event_data[0].edrx_cfg.edrx = 20.48;
	test_event_data[0].edrx_cfg.ptw = 5.12;

	test_event_data[1].type = LTE_LC_EVT_EDRX_UPDATE;
	test_event_data[1].edrx_cfg.mode = LTE_LC_LTE_MODE_LTEM;
	test_event_data[1].edrx_cfg.edrx = 20.48;
	test_event_data[1].edrx_cfg.ptw = 19.2;

	test_event_data[2].type = LTE_LC_EVT_EDRX_UPDATE;
	test_event_data[2].edrx_cfg.mode = LTE_LC_LTE_MODE_LTEM;
	test_event_data[2].edrx_cfg.edrx = 20.48;
	test_event_data[2].edrx_cfg.ptw = 5.12;

	ret = lte_lc_edrx_param_set(LTE_LC_LTE_MODE_LTEM, "0010");
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	ret = lte_lc_edrx_param_set(LTE_LC_LTE_MODE_NBIOT, "0100");
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	ret = lte_lc_edrx_param_set(LTE_LC_LTE_MODE_NTN_NBIOT, "0100");
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	ret = lte_lc_ptw_set(LTE_LC_LTE_MODE_LTEM, "0011");
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	ret = lte_lc_ptw_set(LTE_LC_LTE_MODE_NBIOT, NULL);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	/* Not setting PTW for NB-IoT to test that requested PTW is not 4 characters long */
	ret = lte_lc_ptw_set(LTE_LC_LTE_MODE_NTN_NBIOT, NULL);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	/* Not setting PTW for NTN NB-IoT to test that requested PTW is not 4 characters long */

	strcpy(at_notif, "+CEDRXP: 4,\"0010\",\"0010\",\"0011\"\r\n");
	at_monitor_dispatch(at_notif);
	k_sleep(K_MSEC(1));

	/* AT+CEDRXS=2,4,"0010" is not sent by lte_lc because eDRX value for LTE-M is the same */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CEDRXS=2,5,\"0100\"", EXIT_SUCCESS);
	ret = lte_lc_edrx_req(true);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	/* CEDRXP indicates a different PTW than requested so lte_lc requests it and
	 * we get another notification
	 */
	strcpy(at_notif, "+CEDRXP: 4,\"0010\",\"0010\",\"1110\"\r\n");
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%XPTW=4,\"0011\"", EXIT_SUCCESS);
	at_monitor_dispatch(at_notif);
	k_sleep(K_MSEC(1));

	strcpy(at_notif, "+CEDRXP: 4,\"0010\",\"0010\",\"0011\"\r\n");
	at_monitor_dispatch(at_notif);
	k_sleep(K_MSEC(1));
}

void test_lte_lc_edrx_req_enable_success_ntn(void)
{
	int ret;

	/* Executed with nrf9151-ntn firmware, NTN NB-IoT supported. */

	lte_lc_on_modem_init_with_firmware(cgmr_resp_9151_ntn, sizeof(cgmr_resp_9151_ntn));

	lte_lc_callback_count_expected = 3;

	test_event_data[0].type = LTE_LC_EVT_EDRX_UPDATE;
	test_event_data[0].edrx_cfg.mode = LTE_LC_LTE_MODE_LTEM;
	test_event_data[0].edrx_cfg.edrx = 20.48;
	test_event_data[0].edrx_cfg.ptw = 5.12;

	test_event_data[1].type = LTE_LC_EVT_EDRX_UPDATE;
	test_event_data[1].edrx_cfg.mode = LTE_LC_LTE_MODE_LTEM;
	test_event_data[1].edrx_cfg.edrx = 20.48;
	test_event_data[1].edrx_cfg.ptw = 19.2;

	test_event_data[2].type = LTE_LC_EVT_EDRX_UPDATE;
	test_event_data[2].edrx_cfg.mode = LTE_LC_LTE_MODE_LTEM;
	test_event_data[2].edrx_cfg.edrx = 20.48;
	test_event_data[2].edrx_cfg.ptw = 5.12;

	ret = lte_lc_edrx_param_set(LTE_LC_LTE_MODE_LTEM, "0010");
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	ret = lte_lc_edrx_param_set(LTE_LC_LTE_MODE_NBIOT, "0100");
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	ret = lte_lc_edrx_param_set(LTE_LC_LTE_MODE_NTN_NBIOT, "0100");
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	ret = lte_lc_ptw_set(LTE_LC_LTE_MODE_LTEM, "0011");
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	ret = lte_lc_ptw_set(LTE_LC_LTE_MODE_NBIOT, NULL);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	/* Not setting PTW for NB-IoT to test that requested PTW is not 4 characters long */
	ret = lte_lc_ptw_set(LTE_LC_LTE_MODE_NTN_NBIOT, NULL);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	/* Not setting PTW for NTN NB-IoT to test that requested PTW is not 4 characters long */

	strcpy(at_notif, "+CEDRXP: 4,\"0010\",\"0010\",\"0011\"\r\n");
	at_monitor_dispatch(at_notif);
	k_sleep(K_MSEC(1));

	/* AT+CEDRXS=2,4,"0010" is not sent by lte_lc because eDRX value for LTE-M is the same */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CEDRXS=2,6,\"0100\"", EXIT_SUCCESS);
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CEDRXS=2,5,\"0100\"", EXIT_SUCCESS);
	ret = lte_lc_edrx_req(true);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	/* CEDRXP indicates a different PTW than requested so lte_lc requests it and
	 * we get another notification
	 */
	strcpy(at_notif, "+CEDRXP: 4,\"0010\",\"0010\",\"1110\"\r\n");
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%XPTW=4,\"0011\"", EXIT_SUCCESS);
	at_monitor_dispatch(at_notif);
	k_sleep(K_MSEC(1));

	strcpy(at_notif, "+CEDRXP: 4,\"0010\",\"0010\",\"0011\"\r\n");
	at_monitor_dispatch(at_notif);
	k_sleep(K_MSEC(1));
}

void test_lte_lc_edrx_req_enable_no_edrx_value_set_success(void)
{
	int ret;

	/* Executed with nrf9151-ntn firmware type, NTN NB-IoT supported. */

	ret = lte_lc_edrx_param_set(LTE_LC_LTE_MODE_LTEM, NULL);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	ret = lte_lc_edrx_param_set(LTE_LC_LTE_MODE_NBIOT, NULL);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	ret = lte_lc_edrx_param_set(LTE_LC_LTE_MODE_NTN_NBIOT, NULL);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CEDRXS=2,6", EXIT_SUCCESS);
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CEDRXS=2,4", EXIT_SUCCESS);
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CEDRXS=2,5", EXIT_SUCCESS);
	ret = lte_lc_edrx_req(true);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_cedrxp(void)
{
	int ret;

	/* Executed with nrf9151-ntn firmware type, NTN NB-IoT supported. */

	strcpy(at_notif, "+CEDRXP: 5,\"1000\",\"0010\",\"1110\"\r\n");

	lte_lc_callback_count_expected = 1;

	test_event_data[0].type = LTE_LC_EVT_EDRX_UPDATE;
	test_event_data[0].edrx_cfg.mode = LTE_LC_LTE_MODE_NBIOT;
	test_event_data[0].edrx_cfg.edrx = 20.48;
	test_event_data[0].edrx_cfg.ptw = 38.4;

	ret = lte_lc_edrx_param_set(LTE_LC_LTE_MODE_LTEM, "0010");
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	ret = lte_lc_edrx_param_set(LTE_LC_LTE_MODE_NBIOT, "0100");
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	ret = lte_lc_edrx_param_set(LTE_LC_LTE_MODE_NTN_NBIOT, "0100");
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	ret = lte_lc_ptw_set(LTE_LC_LTE_MODE_LTEM, "1110");
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	ret = lte_lc_ptw_set(LTE_LC_LTE_MODE_NBIOT, "0001");
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	ret = lte_lc_ptw_set(LTE_LC_LTE_MODE_NTN_NBIOT, "0001");
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	/* Test AT%XPTW failure which is not really tested except in the code coverage data */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%XPTW=6,\"0001\"", -NRF_ENOMEM);
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%XPTW=4,\"1110\"", EXIT_SUCCESS);
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%XPTW=5,\"0001\"", EXIT_SUCCESS);
	at_monitor_dispatch(at_notif);
	k_sleep(K_MSEC(1));
}

void test_lte_lc_edrx_on_modem_cfun(void)
{
	/* Executed with nrf9151-ntn firmware type, NTN NB-IoT supported. */

	/* Power off callback from nrf_modem_lib causes AT commands */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CEDRXS=2,6,\"0100\"", EXIT_SUCCESS);
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CEDRXS=2,4,\"0010\"", EXIT_SUCCESS);
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CEDRXS=2,5,\"0100\"", EXIT_SUCCESS);
	/* CONFIG_LTE_RAI_REQ=n */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%RAI=0", EXIT_SUCCESS);
	/* CONFIG_LTE_LC_DNS_FALLBACK_MODULE */
	__cmock_nrf_inet_pton_ExpectAndReturn(NRF_AF_INET, IGNORE, IGNORE, 1 /* success */);
	__cmock_nrf_inet_pton_IgnoreArg_src();
	__cmock_nrf_inet_pton_IgnoreArg_dst();
	__cmock_nrf_setdnsaddr_ExpectAndReturn(NRF_AF_INET, IGNORE, sizeof(struct nrf_in_addr),
					       EXIT_SUCCESS);
	__cmock_nrf_setdnsaddr_IgnoreArg_in_addr();

	lte_lc_on_modem_cfun(LTE_LC_FUNC_MODE_POWER_OFF, NULL);
	k_sleep(K_MSEC(1));

	/* Other modes than offline doesn't cause actions */
	lte_lc_on_modem_cfun(LTE_LC_FUNC_MODE_NORMAL, NULL);
	k_sleep(K_MSEC(1));
}

void test_lte_lc_cedrxp_no_params_fail(void)
{
	strcpy(at_notif, "+CEDRXP:\r\n");
	at_monitor_dispatch(at_notif);
}

void test_lte_lc_cedrxp_invalid_mode_fail(void)
{
	strcpy(at_notif, "+CEDRXP: 1,\"1000\",\"0101\",\"1011\"\r\n");
	at_monitor_dispatch(at_notif);
}

void test_lte_lc_edrx_get_disabled_success(void)
{
	int ret;
	struct lte_lc_edrx_cfg edrx_cfg;
	static const char cedrxrdp_resp[] = "+CEDRXRDP: 0";

	__cmock_nrf_modem_at_cmd_ExpectAndReturn(NULL, 0, "AT+CEDRXRDP", 0);
	__cmock_nrf_modem_at_cmd_IgnoreArg_buf();
	__cmock_nrf_modem_at_cmd_IgnoreArg_len();
	__cmock_nrf_modem_at_cmd_ReturnArrayThruPtr_buf(
		(char *)cedrxrdp_resp, sizeof(cedrxrdp_resp));

	ret = lte_lc_edrx_get(&edrx_cfg);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	TEST_ASSERT_EQUAL(LTE_LC_LTE_MODE_NONE, edrx_cfg.mode);
}

void test_lte_lc_edrx_get_ltem1_success(void)
{
	int ret;
	struct lte_lc_edrx_cfg edrx_cfg;
	static const char cedrxrdp_resp[] = "+CEDRXRDP: 4,\"1000\",\"0101\",\"1011\"";

	__cmock_nrf_modem_at_cmd_ExpectAndReturn(NULL, 0, "AT+CEDRXRDP", 0);
	__cmock_nrf_modem_at_cmd_IgnoreArg_buf();
	__cmock_nrf_modem_at_cmd_IgnoreArg_len();
	__cmock_nrf_modem_at_cmd_ReturnArrayThruPtr_buf(
		(char *)cedrxrdp_resp, sizeof(cedrxrdp_resp));

	ret = lte_lc_edrx_get(&edrx_cfg);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	TEST_ASSERT_EQUAL(LTE_LC_LTE_MODE_LTEM, edrx_cfg.mode);
	TEST_ASSERT_EQUAL_FLOAT(81.92, edrx_cfg.edrx);
	TEST_ASSERT_EQUAL_FLOAT(15.36, edrx_cfg.ptw);
}

void test_lte_lc_edrx_get_ltem2_success(void)
{
	int ret;
	struct lte_lc_edrx_cfg edrx_cfg;
	static const char cedrxrdp_resp[] = "+CEDRXRDP: 4,\"1000\",\"0000\",\"1110\"";

	__cmock_nrf_modem_at_cmd_ExpectAndReturn(NULL, 0, "AT+CEDRXRDP", 0);
	__cmock_nrf_modem_at_cmd_IgnoreArg_buf();
	__cmock_nrf_modem_at_cmd_IgnoreArg_len();
	__cmock_nrf_modem_at_cmd_ReturnArrayThruPtr_buf(
		(char *)cedrxrdp_resp, sizeof(cedrxrdp_resp));

	ret = lte_lc_edrx_get(&edrx_cfg);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	TEST_ASSERT_EQUAL(LTE_LC_LTE_MODE_LTEM, edrx_cfg.mode);
	TEST_ASSERT_EQUAL_FLOAT(5.12, edrx_cfg.edrx);
	TEST_ASSERT_EQUAL_FLOAT(19.2, edrx_cfg.ptw);
}

void test_lte_lc_edrx_get_nbiot1_success(void)
{
	int ret;
	struct lte_lc_edrx_cfg edrx_cfg;
	static const char cedrxrdp_resp[] = "+CEDRXRDP: 5,\"1000\",\"1101\",\"0111\"";

	__cmock_nrf_modem_at_cmd_ExpectAndReturn(NULL, 0, "AT+CEDRXRDP", 0);
	__cmock_nrf_modem_at_cmd_IgnoreArg_buf();
	__cmock_nrf_modem_at_cmd_IgnoreArg_len();
	__cmock_nrf_modem_at_cmd_ReturnArrayThruPtr_buf(
		(char *)cedrxrdp_resp, sizeof(cedrxrdp_resp));

	ret = lte_lc_edrx_get(&edrx_cfg);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	TEST_ASSERT_EQUAL(LTE_LC_LTE_MODE_NBIOT, edrx_cfg.mode);
	TEST_ASSERT_EQUAL_FLOAT(2621.44, edrx_cfg.edrx);
	TEST_ASSERT_EQUAL_FLOAT(20.48, edrx_cfg.ptw);
}

void test_lte_lc_edrx_get_nbiot2_success(void)
{
	int ret;
	struct lte_lc_edrx_cfg edrx_cfg;
	static const char cedrxrdp_resp[] = "+CEDRXRDP: 5,\"1000\",\"1101\",\"0101\"";

	__cmock_nrf_modem_at_cmd_ExpectAndReturn(NULL, 0, "AT+CEDRXRDP", 0);
	__cmock_nrf_modem_at_cmd_IgnoreArg_buf();
	__cmock_nrf_modem_at_cmd_IgnoreArg_len();
	__cmock_nrf_modem_at_cmd_ReturnArrayThruPtr_buf(
		(char *)cedrxrdp_resp, sizeof(cedrxrdp_resp));

	ret = lte_lc_edrx_get(&edrx_cfg);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	TEST_ASSERT_EQUAL(LTE_LC_LTE_MODE_NBIOT, edrx_cfg.mode);
	TEST_ASSERT_EQUAL_FLOAT(2621.44, edrx_cfg.edrx);
	TEST_ASSERT_EQUAL_FLOAT(15.36, edrx_cfg.ptw);
}

void test_lte_lc_edrx_get_invalid_mode_fail(void)
{
	int ret;
	struct lte_lc_edrx_cfg edrx_cfg;
	static const char cedrxrdp_resp[] = "+CEDRXRDP: 1,\"1000\",\"0101\",\"1011\"";

	__cmock_nrf_modem_at_cmd_ExpectAndReturn(NULL, 0, "AT+CEDRXRDP", 0);
	__cmock_nrf_modem_at_cmd_IgnoreArg_buf();
	__cmock_nrf_modem_at_cmd_IgnoreArg_len();
	__cmock_nrf_modem_at_cmd_ReturnArrayThruPtr_buf(
		(char *)cedrxrdp_resp, sizeof(cedrxrdp_resp));

	ret = lte_lc_edrx_get(&edrx_cfg);
	TEST_ASSERT_EQUAL(-EBADMSG, ret);
}

void test_lte_lc_edrx_get_invalid_nw_edrx_value_fail(void)
{
	int ret;
	struct lte_lc_edrx_cfg edrx_cfg;
	static const char cedrxrdp_resp[] = "+CEDRXRDP: 4,\"1000\",0101,\"1011\"";

	__cmock_nrf_modem_at_cmd_ExpectAndReturn(NULL, 0, "AT+CEDRXRDP", 0);
	__cmock_nrf_modem_at_cmd_IgnoreArg_buf();
	__cmock_nrf_modem_at_cmd_IgnoreArg_len();
	__cmock_nrf_modem_at_cmd_ReturnArrayThruPtr_buf(
		(char *)cedrxrdp_resp, sizeof(cedrxrdp_resp));

	ret = lte_lc_edrx_get(&edrx_cfg);
	TEST_ASSERT_EQUAL(-EBADMSG, ret);
}

void test_lte_lc_edrx_get_invalid_nw_edrx_value_empty(void)
{
	int ret;
	struct lte_lc_edrx_cfg edrx_cfg;
	static const char cedrxrdp_resp[] = "+CEDRXRDP: 4,\"1000\",\"\",\"1011\"";

	__cmock_nrf_modem_at_cmd_ExpectAndReturn(NULL, 0, "AT+CEDRXRDP", 0);
	__cmock_nrf_modem_at_cmd_IgnoreArg_buf();
	__cmock_nrf_modem_at_cmd_IgnoreArg_len();
	__cmock_nrf_modem_at_cmd_ReturnArrayThruPtr_buf(
		(char *)cedrxrdp_resp, sizeof(cedrxrdp_resp));

	ret = lte_lc_edrx_get(&edrx_cfg);
	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(LTE_LC_LTE_MODE_NONE, edrx_cfg.mode);
	TEST_ASSERT_EQUAL_FLOAT(0, edrx_cfg.edrx);
	TEST_ASSERT_EQUAL_FLOAT(0, edrx_cfg.ptw);
}

void test_lte_lc_edrx_get_missing_nw_edrx_value(void)
{
	int ret;
	struct lte_lc_edrx_cfg edrx_cfg;
	static const char cedrxrdp_resp[] = "+CEDRXRDP: 4,\"1000\"";

	__cmock_nrf_modem_at_cmd_ExpectAndReturn(NULL, 0, "AT+CEDRXRDP", 0);
	__cmock_nrf_modem_at_cmd_IgnoreArg_buf();
	__cmock_nrf_modem_at_cmd_IgnoreArg_len();
	__cmock_nrf_modem_at_cmd_ReturnArrayThruPtr_buf(
		(char *)cedrxrdp_resp, sizeof(cedrxrdp_resp));

	ret = lte_lc_edrx_get(&edrx_cfg);
	TEST_ASSERT_EQUAL(-EBADMSG, ret);
}

void test_lte_lc_edrx_get_missing_ptw_value_fail(void)
{
	int ret;
	struct lte_lc_edrx_cfg edrx_cfg;
	static const char cedrxrdp_resp[] = "+CEDRXRDP: 4,\"1000\",\"0101\"";

	__cmock_nrf_modem_at_cmd_ExpectAndReturn(NULL, 0, "AT+CEDRXRDP", 0);
	__cmock_nrf_modem_at_cmd_IgnoreArg_buf();
	__cmock_nrf_modem_at_cmd_IgnoreArg_len();
	__cmock_nrf_modem_at_cmd_ReturnArrayThruPtr_buf(
		(char *)cedrxrdp_resp, sizeof(cedrxrdp_resp));

	ret = lte_lc_edrx_get(&edrx_cfg);
	TEST_ASSERT_EQUAL(-EBADMSG, ret);
}

void test_lte_lc_edrx_get_null_fail(void)
{
	int ret;

	ret = lte_lc_edrx_get(NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

void test_lte_lc_edrx_get_at_cmd_fail(void)
{
	int ret;
	struct lte_lc_edrx_cfg edrx_cfg;

	__cmock_nrf_modem_at_cmd_ExpectAndReturn(NULL, 0, "AT+CEDRXRDP", 65535);
	__cmock_nrf_modem_at_cmd_IgnoreArg_buf();
	__cmock_nrf_modem_at_cmd_IgnoreArg_len();

	ret = lte_lc_edrx_get(&edrx_cfg);
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_nw_reg_status_get_null_fail(void)
{
	int ret;

	ret = lte_lc_nw_reg_status_get(NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

void test_lte_lc_nw_reg_status_get_invalid_cell_id_fail(void)
{
	int ret;
	enum lte_lc_nw_reg_status status;

	__mock_nrf_modem_at_scanf_ExpectAndReturn("AT+CEREG?", "+CEREG: %*u,%hu,%*[^,],\"%x\",", 2);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(1);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(LTE_LC_CELL_EUTRAN_ID_INVALID);

	ret = lte_lc_nw_reg_status_get(&status);
	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(LTE_LC_NW_REG_UNKNOWN, status);
}

void test_lte_lc_func_mode_get_null(void)
{
	int ret;

	ret = lte_lc_func_mode_get(NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

void test_lte_lc_func_mode_get_at_fail(void)
{
	int ret;
	enum lte_lc_func_mode mode;

	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT+CFUN?", "+CFUN: %hu", -NRF_ENOMEM);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(1);

	ret = lte_lc_func_mode_get(&mode);
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_func_mode_get_success(void)
{
	int ret;
	enum lte_lc_func_mode mode;

	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT+CFUN?", "+CFUN: %hu", 1);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(1);

	ret = lte_lc_func_mode_get(&mode);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	TEST_ASSERT_EQUAL(1, mode);
}

static int system_modes[7][4] = {
	{LTE_LC_SYSTEM_MODE_LTEM,           1, 0, 0},
	{LTE_LC_SYSTEM_MODE_NBIOT,          0, 1, 0},
	{LTE_LC_SYSTEM_MODE_GPS,            0, 0, 1},
	{LTE_LC_SYSTEM_MODE_LTEM_GPS,       1, 0, 1},
	{LTE_LC_SYSTEM_MODE_NBIOT_GPS,      0, 1, 1},
	{LTE_LC_SYSTEM_MODE_LTEM_NBIOT,     1, 1, 0},
	{LTE_LC_SYSTEM_MODE_LTEM_NBIOT_GPS, 1, 1, 1},
};

void test_lte_lc_system_mode_set_all_modes(void)
{
	int ret;
	char at_cmd[32];

	for (size_t i = 0; i < ARRAY_SIZE(system_modes); ++i) {
		for (int preference = 0; preference < 5; ++preference) {
			sprintf(at_cmd, "AT%%XSYSTEMMODE=%d,%d,%d,%d",
				system_modes[i][1],
				system_modes[i][2],
				system_modes[i][3],
				preference);
			__mock_nrf_modem_at_printf_ExpectAndReturn(at_cmd, EXIT_SUCCESS);
			ret = lte_lc_system_mode_set(system_modes[i][0], preference);
			TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
		}
	}
}

void test_lte_lc_system_mode_set_invalid_mode_fail(void)
{
	int ret;

	ret = lte_lc_system_mode_set(0xFF, LTE_LC_SYSTEM_MODE_PREFER_AUTO);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

void test_lte_lc_system_mode_set_invalid_pref_fail(void)
{
	int ret;

	ret = lte_lc_system_mode_set(LTE_LC_SYSTEM_MODE_LTEM_NBIOT, 0xFF);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

void test_lte_lc_system_mode_set_at_printf_fail(void)
{
	int ret;

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%XSYSTEMMODE=0,0,1,0", -NRF_ENOMEM);

	ret = lte_lc_system_mode_set(LTE_LC_SYSTEM_MODE_GPS, LTE_LC_SYSTEM_MODE_PREFER_AUTO);
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_system_mode_get_all_modes(void)
{
	int ret;
	enum lte_lc_system_mode mode;
	enum lte_lc_system_mode_preference preference;

	for (size_t i = 0; i < ARRAY_SIZE(system_modes); ++i) {
		__mock_nrf_modem_at_scanf_ExpectAndReturn(
			"AT%XSYSTEMMODE?", "%%XSYSTEMMODE: %d,%d,%d,%d,%d", 4);
		__mock_nrf_modem_at_scanf_ReturnVarg_int(system_modes[i][1]); /* ltem_mode */
		__mock_nrf_modem_at_scanf_ReturnVarg_int(system_modes[i][2]); /* nbiot_mode */
		__mock_nrf_modem_at_scanf_ReturnVarg_int(system_modes[i][3]); /* gps_mode */
		__mock_nrf_modem_at_scanf_ReturnVarg_int(0); /* mode_preference */

		ret = lte_lc_system_mode_get(&mode, &preference);
		TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
		TEST_ASSERT_EQUAL(system_modes[i][0], mode);
		TEST_ASSERT_EQUAL(LTE_LC_SYSTEM_MODE_PREFER_AUTO, preference);
	}
}

void test_lte_lc_system_mode_get_all_preferences(void)
{
	int ret;
	enum lte_lc_system_mode mode;
	enum lte_lc_system_mode_preference preference;

	for (int i = 0; i < 5; ++i) {
		__mock_nrf_modem_at_scanf_ExpectAndReturn(
			"AT%XSYSTEMMODE?", "%%XSYSTEMMODE: %d,%d,%d,%d,%d", 4);
		__mock_nrf_modem_at_scanf_ReturnVarg_int(1); /* ltem_mode */
		__mock_nrf_modem_at_scanf_ReturnVarg_int(1); /* nbiot_mode */
		__mock_nrf_modem_at_scanf_ReturnVarg_int(0); /* gps_mode */
		__mock_nrf_modem_at_scanf_ReturnVarg_int(i); /* mode_preference */

		ret = lte_lc_system_mode_get(&mode, &preference);
		TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
		TEST_ASSERT_EQUAL(LTE_LC_SYSTEM_MODE_LTEM_NBIOT, mode);
		TEST_ASSERT_EQUAL(i, preference);
	}
}

void test_lte_lc_system_mode_get_no_pref(void)
{
	int ret;
	enum lte_lc_system_mode mode;

	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT%XSYSTEMMODE?", "%%XSYSTEMMODE: %d,%d,%d,%d,%d", 4);
	__mock_nrf_modem_at_scanf_ReturnVarg_int(1); /* ltem_mode */
	__mock_nrf_modem_at_scanf_ReturnVarg_int(0); /* nbiot_mode */
	__mock_nrf_modem_at_scanf_ReturnVarg_int(1); /* gps_mode */
	__mock_nrf_modem_at_scanf_ReturnVarg_int(0); /* mode_preference */

	ret = lte_lc_system_mode_get(&mode, NULL);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	TEST_ASSERT_EQUAL(LTE_LC_SYSTEM_MODE_LTEM_GPS, mode);
}

void test_lte_lc_system_mode_get_at_scanf_fail(void)
{
	int ret;
	enum lte_lc_system_mode mode;
	enum lte_lc_system_mode_preference preference;

	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT%XSYSTEMMODE?", "%%XSYSTEMMODE: %d,%d,%d,%d,%d", 3);

	ret = lte_lc_system_mode_get(&mode, &preference);
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_system_mode_get_no_mode_fail(void)
{
	int ret;
	enum lte_lc_system_mode_preference preference;

	ret = lte_lc_system_mode_get(NULL, &preference);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

void test_lte_lc_system_mode_get_invalid_mode_fail(void)
{
	int ret;
	enum lte_lc_system_mode mode;
	enum lte_lc_system_mode_preference preference;

	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT%XSYSTEMMODE?", "%%XSYSTEMMODE: %d,%d,%d,%d,%d", 4);
	__mock_nrf_modem_at_scanf_ReturnVarg_int(0); /* ltem_mode */
	__mock_nrf_modem_at_scanf_ReturnVarg_int(0); /* nbiot_mode */
	__mock_nrf_modem_at_scanf_ReturnVarg_int(0); /* gps_mode */
	__mock_nrf_modem_at_scanf_ReturnVarg_int(0); /* mode_preference */

	ret = lte_lc_system_mode_get(&mode, &preference);
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_system_mode_get_invalid_pref_fail(void)
{
	int ret;
	enum lte_lc_system_mode mode;
	enum lte_lc_system_mode_preference preference;

	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT%XSYSTEMMODE?", "%%XSYSTEMMODE: %d,%d,%d,%d,%d", 4);
	__mock_nrf_modem_at_scanf_ReturnVarg_int(1); /* ltem_mode */
	__mock_nrf_modem_at_scanf_ReturnVarg_int(1); /* nbiot_mode */
	__mock_nrf_modem_at_scanf_ReturnVarg_int(1); /* gps_mode */
	__mock_nrf_modem_at_scanf_ReturnVarg_int(0x0000FFFF); /* mode_preference */

	ret = lte_lc_system_mode_get(&mode, &preference);
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_func_mode_set_all_modes(void)
{
	int ret;
	char at_cmd[12];

	enable_notifications();
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CFUN=2", EXIT_SUCCESS);
	ret = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_RX_ONLY);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	enable_notifications();
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CFUN=21", EXIT_SUCCESS);
	ret = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_ACTIVATE_LTE);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	enum lte_lc_func_mode modes[] = {
		LTE_LC_FUNC_MODE_POWER_OFF,
		LTE_LC_FUNC_MODE_OFFLINE,
		LTE_LC_FUNC_MODE_DEACTIVATE_LTE,
		LTE_LC_FUNC_MODE_DEACTIVATE_GNSS,
		LTE_LC_FUNC_MODE_ACTIVATE_GNSS,
		LTE_LC_FUNC_MODE_DEACTIVATE_UICC,
		LTE_LC_FUNC_MODE_ACTIVATE_UICC,
		LTE_LC_FUNC_MODE_OFFLINE_UICC_ON,
	};

	for (size_t i = 0; i < ARRAY_SIZE(modes); ++i) {
		sprintf(at_cmd, "AT+CFUN=%d", modes[i]);
		__mock_nrf_modem_at_printf_ExpectAndReturn(at_cmd, EXIT_SUCCESS);
		ret = lte_lc_func_mode_set(modes[i]);
		TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	}
}

void test_lte_lc_func_mode_set_inval_fail(void)
{
	int ret;

	ret = lte_lc_func_mode_set(0xFFFF0000); /* Unknown */
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

void test_lte_lc_func_mode_set_at_printf_fail(void)
{
	int ret;

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CEREG=5", EXIT_SUCCESS);
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CSCON=1", -NRF_ENOMEM);
	ret = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_ACTIVATE_LTE);
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_lte_mode_get_null(void)
{
	int ret;

	ret = lte_lc_lte_mode_get(NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

void test_lte_lc_lte_mode_get_nomem(void)
{
	int ret;
	enum lte_lc_lte_mode mode;

	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT+CEREG?", "+CEREG: %*u,%*u,%*[^,],%*[^,],%hu", -NRF_ENOMEM);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(1);

	ret = lte_lc_lte_mode_get(&mode);
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_lte_mode_get_at_badmsg(void)
{
	int ret;
	enum lte_lc_lte_mode mode;

	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT+CEREG?", "+CEREG: %*u,%*u,%*[^,],%*[^,],%hu", -NRF_EBADMSG);

	ret = lte_lc_lte_mode_get(&mode);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	TEST_ASSERT_EQUAL(LTE_LC_LTE_MODE_NONE, mode);
}

void test_lte_lc_lte_mode_get_invalid_value(void)
{
	int ret;
	enum lte_lc_lte_mode mode;

	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT+CEREG?", "+CEREG: %*u,%*u,%*[^,],%*[^,],%hu", 1);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(0xFF00);

	ret = lte_lc_lte_mode_get(&mode);
	TEST_ASSERT_EQUAL(-EBADMSG, ret);
}

void test_lte_lc_lte_mode_get_known_modes(void)
{
	int ret;
	enum lte_lc_lte_mode mode;
	enum lte_lc_lte_mode modes[] = {
		LTE_LC_LTE_MODE_NONE,
		LTE_LC_LTE_MODE_LTEM,
		LTE_LC_LTE_MODE_NBIOT
	};

	for (size_t i = 0; i < ARRAY_SIZE(modes); ++i) {
		__mock_nrf_modem_at_scanf_ExpectAndReturn(
			"AT+CEREG?", "+CEREG: %*u,%*u,%*[^,],%*[^,],%hu", 1);
		__mock_nrf_modem_at_scanf_ReturnVarg_uint16(modes[i]);

		ret = lte_lc_lte_mode_get(&mode);
		TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
		TEST_ASSERT_EQUAL(modes[i], mode);
	}
}

void test_lte_lc_cereg(void)
{
	strcpy(at_notif, "+CEREG: 1,\"4321\",\"12345678\",7,,,\"11100000\",\"00010011\"\r\n");

	lte_lc_callback_count_expected = 4;

	test_event_data[0].type = LTE_LC_EVT_NW_REG_STATUS;
	test_event_data[0].nw_reg_status = LTE_LC_NW_REG_REGISTERED_HOME;

	test_event_data[1].type = LTE_LC_EVT_CELL_UPDATE;
	test_event_data[1].cell.mcc = 0;
	test_event_data[1].cell.mnc = 0;
	test_event_data[1].cell.id = 0x12345678;
	test_event_data[1].cell.tac = 0x4321;
	test_event_data[1].cell.earfcn = 0;
	test_event_data[1].cell.timing_advance = 0;
	test_event_data[1].cell.timing_advance_meas_time = 0;
	test_event_data[1].cell.measurement_time = 0;
	test_event_data[1].cell.phys_cell_id = 0;
	test_event_data[1].cell.rsrp = 0;
	test_event_data[1].cell.rsrq = 0;

	test_event_data[2].type = LTE_LC_EVT_LTE_MODE_UPDATE;
	test_event_data[2].lte_mode = LTE_LC_LTE_MODE_LTEM;

	test_event_data[3].type = LTE_LC_EVT_PSM_UPDATE;
	test_event_data[3].psm_cfg.tau = 11400;
	test_event_data[3].psm_cfg.active_time = -1;

	at_monitor_dispatch(at_notif);
	k_sleep(K_MSEC(1));

	/* Same notification doesn't trigger new events */
	at_monitor_dispatch(at_notif);
	k_sleep(K_MSEC(1));
}

void test_lte_lc_cereg_with_xmonitor(void)
{
	strcpy(at_notif, "+CEREG: 5,\"4321\",\"87654321\",9,,,\"11100000\",\"11100000\"\r\n");

	static const char xmonitor_resp[] =
		"%XMONITOR: 5,\"Operator\",\"OP\",\"20065\",\"4321\",9,20,\"12345678\","
		"334,6200,66,44,\"\","
		"\"11100000\",\"11100000\",\"01001001\"";

	lte_lc_callback_count_expected = 4;

	test_event_data[0].type = LTE_LC_EVT_NW_REG_STATUS;
	test_event_data[0].nw_reg_status = LTE_LC_NW_REG_REGISTERED_ROAMING;

	test_event_data[1].type = LTE_LC_EVT_CELL_UPDATE;
	test_event_data[1].cell.mcc = 0;
	test_event_data[1].cell.mnc = 0;
	test_event_data[1].cell.id = 0x87654321;
	test_event_data[1].cell.tac = 0x4321;
	test_event_data[1].cell.earfcn = 0;
	test_event_data[1].cell.timing_advance = 0;
	test_event_data[1].cell.timing_advance_meas_time = 0;
	test_event_data[1].cell.measurement_time = 0;
	test_event_data[1].cell.phys_cell_id = 0;
	test_event_data[1].cell.rsrp = 0;
	test_event_data[1].cell.rsrq = 0;

	test_event_data[2].type = LTE_LC_EVT_LTE_MODE_UPDATE;
	test_event_data[2].lte_mode = LTE_LC_LTE_MODE_NBIOT;

	test_event_data[3].type = LTE_LC_EVT_PSM_UPDATE;
	test_event_data[3].psm_cfg.tau = 3240;
	test_event_data[3].psm_cfg.active_time = -1;

	__cmock_nrf_modem_at_cmd_ExpectAndReturn(NULL, 0, "AT%%XMONITOR", 0);
	__cmock_nrf_modem_at_cmd_IgnoreArg_buf();
	__cmock_nrf_modem_at_cmd_IgnoreArg_len();
	__cmock_nrf_modem_at_cmd_ReturnArrayThruPtr_buf(
		(char *)xmonitor_resp, sizeof(xmonitor_resp));

	at_monitor_dispatch(at_notif);
	k_sleep(K_MSEC(1));
}

/* Tests:
 * - <Active-Time> and <Periodic-TAU> missing from CEREG
 * - AT%XMONITOR failing
 */
void test_lte_lc_cereg_active_time_tau_missing_with_xmonitor_badmsg(void)
{
	strcpy(at_notif, "+CEREG: 1,\"5678\",\"87654321\",9,,\r\n");

	static const char xmonitor_resp[] =
		"%XMONITOR: 1,\"Operator\",\"OP\",\"20065\",\"5678\",9,20,\"87654321\","
		"334,6200,66,44,\"\","
		"\"11100000\",\"11100000\"";

	lte_lc_callback_count_expected = 2;

	test_event_data[0].type = LTE_LC_EVT_NW_REG_STATUS;
	test_event_data[0].nw_reg_status = LTE_LC_NW_REG_REGISTERED_HOME;

	test_event_data[1].type = LTE_LC_EVT_CELL_UPDATE;
	test_event_data[1].cell.mcc = 0;
	test_event_data[1].cell.mnc = 0;
	test_event_data[1].cell.id = 0x87654321;
	test_event_data[1].cell.tac = 0x5678;
	test_event_data[1].cell.earfcn = 0;
	test_event_data[1].cell.timing_advance = 0;
	test_event_data[1].cell.timing_advance_meas_time = 0;
	test_event_data[1].cell.measurement_time = 0;
	test_event_data[1].cell.phys_cell_id = 0;
	test_event_data[1].cell.rsrp = 0;
	test_event_data[1].cell.rsrq = 0;

	/* No LTE_LC_EVT_LTE_MODE_UPDATE because same values */
	/* No LTE_LC_EVT_PSM_UPDATE because XMONITOR doesn't have legacy TAU timer */

	__cmock_nrf_modem_at_cmd_ExpectAndReturn(NULL, 0, "AT%%XMONITOR", 0);
	__cmock_nrf_modem_at_cmd_IgnoreArg_buf();
	__cmock_nrf_modem_at_cmd_IgnoreArg_len();
	__cmock_nrf_modem_at_cmd_ReturnArrayThruPtr_buf(
		(char *)xmonitor_resp, sizeof(xmonitor_resp));

	at_monitor_dispatch(at_notif);
	k_sleep(K_MSEC(1));
}

/* Tests:
 * - <Periodic-TAU> missing from CEREG
 * - AT%XMONITOR failing
 */
void test_lte_lc_cereg_tau_missing_with_xmonitor_fail(void)
{
	strcpy(at_notif, "+CEREG: 5,\"5678\",\"87654321\",9,,,\"11100000\",\r\n");

	lte_lc_callback_count_expected = 1;

	test_event_data[0].type = LTE_LC_EVT_NW_REG_STATUS;
	test_event_data[0].nw_reg_status = LTE_LC_NW_REG_REGISTERED_ROAMING;

	/* No LTE_LC_EVT_CELL_UPDATE because same values */
	/* No LTE_LC_EVT_LTE_MODE_UPDATE because same values */
	/* No LTE_LC_EVT_PSM_UPDATE because XMONITOR fails */

	__cmock_nrf_modem_at_cmd_ExpectAndReturn(NULL, 0, "AT%%XMONITOR", -NRF_E2BIG);
	__cmock_nrf_modem_at_cmd_IgnoreArg_buf();
	__cmock_nrf_modem_at_cmd_IgnoreArg_len();

	at_monitor_dispatch(at_notif);
	k_sleep(K_MSEC(1));
}

void test_lte_lc_cereg_active_time_not_8_with_xmonitor_tau_ext_not_8_fail(void)
{
	strcpy(at_notif, "+CEREG: 1,\"5678\",\"87654321\",9,,,\"1110\",\"11100000\"\r\n");

	static const char xmonitor_resp[] =
		"%XMONITOR: 1,\"Operator\",\"OP\",\"20065\",\"5678\",9,20,\"87654321\","
		"334,6200,66,44,\"\","
		"\"11100000\",\"0010\",\"01001001\"";

	lte_lc_callback_count_expected = 1;

	test_event_data[0].type = LTE_LC_EVT_NW_REG_STATUS;
	test_event_data[0].nw_reg_status = LTE_LC_NW_REG_REGISTERED_HOME;

	/* No LTE_LC_EVT_CELL_UPDATE because same values */
	/* No LTE_LC_EVT_LTE_MODE_UPDATE because same values */
	/* No LTE_LC_EVT_PSM_UPDATE because XMONITOR parsing fails */

	__cmock_nrf_modem_at_cmd_ExpectAndReturn(NULL, 0, "AT%%XMONITOR", 0);
	__cmock_nrf_modem_at_cmd_IgnoreArg_buf();
	__cmock_nrf_modem_at_cmd_IgnoreArg_len();
	__cmock_nrf_modem_at_cmd_ReturnArrayThruPtr_buf(
		(char *)xmonitor_resp, sizeof(xmonitor_resp));

	at_monitor_dispatch(at_notif);
	k_sleep(K_MSEC(1));
}

void test_lte_lc_cereg_with_xmonitor_active_time_valid_tau_disabled_fail(void)
{
	strcpy(at_notif, "+CEREG: 5,\"5678\",\"87654321\",9,,,\"00001000\",\"11100000\"\r\n");

	static const char xmonitor_resp[] =
		"%XMONITOR: 1,\"Operator\",\"OP\",\"20065\",\"5678\",9,20,\"87654321\","
		"334,6200,66,44,\"\","
		"\"00001000\",\"11100000\",\"11100000\"";

	lte_lc_callback_count_expected = 1;

	test_event_data[0].type = LTE_LC_EVT_NW_REG_STATUS;
	test_event_data[0].nw_reg_status = LTE_LC_NW_REG_REGISTERED_ROAMING;

	/* No LTE_LC_EVT_CELL_UPDATE because same values */
	/* No LTE_LC_EVT_LTE_MODE_UPDATE because same values */
	/* No LTE_LC_EVT_PSM_UPDATE because XMONITOR parsing fails */

	__cmock_nrf_modem_at_cmd_ExpectAndReturn(NULL, 0, "AT%%XMONITOR", 0);
	__cmock_nrf_modem_at_cmd_IgnoreArg_buf();
	__cmock_nrf_modem_at_cmd_IgnoreArg_len();
	__cmock_nrf_modem_at_cmd_ReturnArrayThruPtr_buf(
		(char *)xmonitor_resp, sizeof(xmonitor_resp));

	at_monitor_dispatch(at_notif);
	k_sleep(K_MSEC(1));
}

/* Test the following CEREG syntax:
 * - Empty: <tac>, <ci>, <AcT>, <cause_type>, <reject_cause>
 * - Exists: <Active-Time>, <Periodic-TAU>
 *
 * Not having <tac> and <ci> is a bit abnormal but valid.
 */
void test_lte_lc_cereg_no_tac_but_tau(void)
{
	strcpy(at_notif, "+CEREG: 4,,,,,,\"11100000\",\"00010010\"\r\n");

	lte_lc_callback_count_expected = 3;

	test_event_data[0].type = LTE_LC_EVT_NW_REG_STATUS;
	test_event_data[0].nw_reg_status = LTE_LC_NW_REG_UNKNOWN;

	test_event_data[1].type = LTE_LC_EVT_CELL_UPDATE;
	test_event_data[1].cell.mcc = 0;
	test_event_data[1].cell.mnc = 0;
	test_event_data[1].cell.id = LTE_LC_CELL_EUTRAN_ID_INVALID;
	test_event_data[1].cell.tac = -1;
	test_event_data[1].cell.earfcn = 0;
	test_event_data[1].cell.timing_advance = 0;
	test_event_data[1].cell.timing_advance_meas_time = 0;
	test_event_data[1].cell.measurement_time = 0;
	test_event_data[1].cell.phys_cell_id = 0;
	test_event_data[1].cell.rsrp = 0;
	test_event_data[1].cell.rsrq = 0;

	/* Because previous test leaves the mode to LTE_LC_LTE_MODE_LTEM,
	 * there is a change to LTE_LC_LTE_MODE_NONE in this test
	 */
	test_event_data[2].type = LTE_LC_EVT_LTE_MODE_UPDATE;
	test_event_data[2].lte_mode = LTE_LC_LTE_MODE_NONE;

	at_monitor_dispatch(at_notif);
}

/* Test CEREG reject cause, which is not visible in events but there is only code to log it. */
void test_lte_lc_cereg_unknown_reject_cause(void)
{
	strcpy(at_notif, "+CEREG: 2,\"ABBA\",\"12345678\",7,0,13\r\n");

	lte_lc_callback_count_expected = 3;

	test_event_data[0].type = LTE_LC_EVT_NW_REG_STATUS;
	test_event_data[0].nw_reg_status = LTE_LC_NW_REG_SEARCHING;

	test_event_data[1].type = LTE_LC_EVT_CELL_UPDATE;
	test_event_data[1].cell.mcc = 0;
	test_event_data[1].cell.mnc = 0;
	test_event_data[1].cell.id = 0x12345678;
	test_event_data[1].cell.tac = 0xABBA;
	test_event_data[1].cell.earfcn = 0;
	test_event_data[1].cell.timing_advance = 0;
	test_event_data[1].cell.timing_advance_meas_time = 0;
	test_event_data[1].cell.measurement_time = 0;
	test_event_data[1].cell.phys_cell_id = 0;
	test_event_data[1].cell.rsrp = 0;
	test_event_data[1].cell.rsrq = 0;

	/* Because previous test leaves the mode to LTE_LC_LTE_MODE_NONE,
	 * there is a change to LTE_LC_LTE_MODE_LTEM in this test
	 */
	test_event_data[2].type = LTE_LC_EVT_LTE_MODE_UPDATE;
	test_event_data[2].lte_mode = LTE_LC_LTE_MODE_LTEM;

	at_monitor_dispatch(at_notif);
}

void test_lte_lc_cereg_3_registration_denied(void)
{
	strcpy(at_notif, "+CEREG: 3\r\n");

	lte_lc_callback_count_expected = 3;

	test_event_data[0].type = LTE_LC_EVT_NW_REG_STATUS;
	test_event_data[0].nw_reg_status = LTE_LC_NW_REG_REGISTRATION_DENIED;

	test_event_data[1].type = LTE_LC_EVT_CELL_UPDATE;
	test_event_data[1].cell.id = LTE_LC_CELL_EUTRAN_ID_INVALID;
	test_event_data[1].cell.tac = -1;

	test_event_data[2].type = LTE_LC_EVT_LTE_MODE_UPDATE;
	test_event_data[2].lte_mode = LTE_LC_LTE_MODE_NONE;

	at_monitor_dispatch(at_notif);
}

/* Test unknown CEREG reject cause type */
void test_lte_lc_cereg_unknown_reject_cause_unknown_type(void)
{
	strcpy(at_notif, "+CEREG: 2,\"ABBA\",\"87654321\",7,1,1\r\n");

	lte_lc_callback_count_expected = 3;

	test_event_data[0].type = LTE_LC_EVT_NW_REG_STATUS;
	test_event_data[0].nw_reg_status = LTE_LC_NW_REG_SEARCHING;

	test_event_data[1].type = LTE_LC_EVT_CELL_UPDATE;
	test_event_data[1].cell.mcc = 0;
	test_event_data[1].cell.mnc = 0;
	test_event_data[1].cell.id = 0x87654321;
	test_event_data[1].cell.tac = 0xABBA;
	test_event_data[1].cell.earfcn = 0;
	test_event_data[1].cell.timing_advance = 0;
	test_event_data[1].cell.timing_advance_meas_time = 0;
	test_event_data[1].cell.measurement_time = 0;
	test_event_data[1].cell.phys_cell_id = 0;
	test_event_data[1].cell.rsrp = 0;
	test_event_data[1].cell.rsrq = 0;

	/* Because previous test leaves the mode to LTE_LC_LTE_MODE_NONE,
	 * there is a change to LTE_LC_LTE_MODE_LTEM in this test
	 */
	test_event_data[2].type = LTE_LC_EVT_LTE_MODE_UPDATE;
	test_event_data[2].lte_mode = LTE_LC_LTE_MODE_LTEM;

	at_monitor_dispatch(at_notif);
}

void test_lte_lc_cereg_90_uicc_fail(void)
{
	strcpy(at_notif, "+CEREG: 90\r\n");

	lte_lc_callback_count_expected = 3;

	test_event_data[0].type = LTE_LC_EVT_NW_REG_STATUS;
	test_event_data[0].nw_reg_status = LTE_LC_NW_REG_UICC_FAIL;

	test_event_data[1].type = LTE_LC_EVT_CELL_UPDATE;
	test_event_data[1].cell.id = LTE_LC_CELL_EUTRAN_ID_INVALID;
	test_event_data[1].cell.tac = -1;

	test_event_data[2].type = LTE_LC_EVT_LTE_MODE_UPDATE;
	test_event_data[2].lte_mode = LTE_LC_LTE_MODE_NONE;

	at_monitor_dispatch(at_notif);
}

void test_lte_lc_cereg_unknown_reg_status_fail(void)
{
	strcpy(at_notif, "+CEREG: 99\r\n");
	at_monitor_dispatch(at_notif);
}

void test_lte_lc_cereg_no_number_fail(void)
{
	strcpy(at_notif, "+CEREG:\r\n");
	at_monitor_dispatch(at_notif);
}

void test_lte_lc_cereg_unknown_lte_mode_fail(void)
{
	strcpy(at_notif, "+CEREG: 1,\"5678\",\"87654321\",5,,,\"11100000\",\"11100000\"\r\n");

	/* Registration status and cell update are sent before LTE mode is checked to be invalid */
	lte_lc_callback_count_expected = 2;

	test_event_data[0].type = LTE_LC_EVT_NW_REG_STATUS;
	test_event_data[0].nw_reg_status = LTE_LC_NW_REG_REGISTERED_HOME;

	test_event_data[1].type = LTE_LC_EVT_CELL_UPDATE;
	test_event_data[1].cell.mcc = 0;
	test_event_data[1].cell.mnc = 0;
	test_event_data[1].cell.id = 0x87654321;
	test_event_data[1].cell.tac = 0x5678;
	test_event_data[1].cell.earfcn = 0;
	test_event_data[1].cell.timing_advance = 0;
	test_event_data[1].cell.timing_advance_meas_time = 0;
	test_event_data[1].cell.measurement_time = 0;
	test_event_data[1].cell.phys_cell_id = 0;
	test_event_data[1].cell.rsrp = 0;
	test_event_data[1].cell.rsrq = 0;

	at_monitor_dispatch(at_notif);
}

void test_lte_lc_cereg_malformed_fail(void)
{
	strcpy(at_notif, "+CEREG: 1 2,3\r\n");
	at_monitor_dispatch(at_notif);

	strcpy(at_notif, "+CEREG: 1,.\r\n");
	at_monitor_dispatch(at_notif);
}

void test_lte_lc_cereg_no_event_handler(void)
{
	int ret;

	ret = lte_lc_deregister_handler(lte_lc_event_handler);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	strcpy(at_notif, "+CEREG: 1\r\n");
	at_monitor_dispatch(at_notif);

	/* Register handler so that tearDown() doesn't cause unnecessary warning log */
	lte_lc_register_handler(lte_lc_event_handler);
}

void test_lte_lc_cscon(void)
{
	lte_lc_callback_count_expected = 2;

	test_event_data[0].type = LTE_LC_EVT_RRC_UPDATE;
	test_event_data[0].rrc_mode = LTE_LC_RRC_MODE_CONNECTED;

	test_event_data[1].type = LTE_LC_EVT_RRC_UPDATE;
	test_event_data[1].rrc_mode = LTE_LC_RRC_MODE_IDLE;

	strcpy(at_notif, "+CSCON: 1\r\n");
	at_monitor_dispatch(at_notif);

	strcpy(at_notif, "+CSCON: 0\r\n");
	at_monitor_dispatch(at_notif);

	/* Invalid RRC mode value does not trigger event */
	strcpy(at_notif, "+CSCON: 2\r\n");
	at_monitor_dispatch(at_notif);

	/* Missing RRC mode value does not trigger event */
	strcpy(at_notif, "+CSCON:\r\n");
	at_monitor_dispatch(at_notif);
}

void test_lte_lc_xt3412(void)
{
	lte_lc_callback_count_expected = 1;

	test_event_data[0].type = LTE_LC_EVT_TAU_PRE_WARNING;
	test_event_data[0].time = CONFIG_LTE_LC_TAU_PRE_WARNING_TIME_MS;

	sprintf(at_notif, "%%XT3412: %d\r\n", CONFIG_LTE_LC_TAU_PRE_WARNING_TIME_MS);
	at_monitor_dispatch(at_notif);

	/* Event not triggered in the following cases */

	/* TAU not equal to CONFIG_LTE_LC_TAU_PRE_WARNING_TIME_MS */
	strcpy(at_notif, "%XT3412: 1000\r\n");
	at_monitor_dispatch(at_notif);

	/* TAU with max value */
	strcpy(at_notif, "%XT3412: 35712000000\r\n");
	at_monitor_dispatch(at_notif);

	/* Too big TAU pre-warning time */
	strcpy(at_notif, "%XT3412: 35712000001\r\n");
	at_monitor_dispatch(at_notif);

	/* Time not included */
	strcpy(at_notif, "%XT3412:\r\n");
	at_monitor_dispatch(at_notif);

	/* Negative TAU */
	strcpy(at_notif, "%XT3412: -100\r\n");
	at_monitor_dispatch(at_notif);

	/* TAU not a number */
	strcpy(at_notif, "%XT3412: invalid\r\n");
	at_monitor_dispatch(at_notif);
}

/* Test failing neighbor cell measurement first to see that the semaphore is given properly */
void test_lte_lc_neighbor_cell_measurement_fail(void)
{
	int ret;

	struct lte_lc_ncellmeas_params params = {
		.search_type = LTE_LC_NEIGHBOR_SEARCH_TYPE_EXTENDED_COMPLETE,
		.gci_count = 0,
	};

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS=2", -NRF_ENOMEM);

	ret = lte_lc_neighbor_cell_measurement(&params);
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

/* Test that no callbacks are triggered by an %NCELLMEAS notification when
 * lte_lc_neighbor_cell_measurement() has not been called.
 */
void test_lte_lc_neighbor_cell_measurement_no_request(void)
{
	strcpy(at_notif,
	       "%NCELLMEAS: 0,\"00112233\",\"98712\",\"0AB9\",4800,7,63,31,"
	       "456,4800,8,60,29,4,3500,9,99,18,5,5300,11\r\n");

	at_monitor_dispatch(at_notif);
}

void test_lte_lc_neighbor_cell_measurement_normal(void)
{
	int ret;
	struct lte_lc_ncellmeas_params params = {
		.search_type = LTE_LC_NEIGHBOR_SEARCH_TYPE_DEFAULT,
		.gci_count = 0,
	};
	strcpy(at_notif,
	       "%NCELLMEAS:0,\"00112233\",\"98712\",\"0AB9\",4800,7,63,31,"
	       "456,4800,8,60,29,4,3500,9,99,18,5,5300,11\r\n");

	lte_lc_callback_count_expected = 1;

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS", EXIT_SUCCESS);

	ret = lte_lc_neighbor_cell_measurement(&params);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	test_event_data[0].type = LTE_LC_EVT_NEIGHBOR_CELL_MEAS;
	test_event_data[0].cells_info.current_cell.mcc = 987;
	test_event_data[0].cells_info.current_cell.mnc = 12;
	test_event_data[0].cells_info.current_cell.id = 0x00112233;
	test_event_data[0].cells_info.current_cell.tac = 0x0AB9;
	test_event_data[0].cells_info.current_cell.earfcn = 7;
	test_event_data[0].cells_info.current_cell.timing_advance = 4800;
	test_event_data[0].cells_info.current_cell.timing_advance_meas_time = 11;
	test_event_data[0].cells_info.current_cell.measurement_time = 4800;
	test_event_data[0].cells_info.current_cell.phys_cell_id = 63;
	test_event_data[0].cells_info.current_cell.rsrp = 31;
	test_event_data[0].cells_info.current_cell.rsrq = 456;
	test_event_data[0].cells_info.ncells_count = 2;
	test_event_data[0].cells_info.gci_cells_count = 0;
	test_neighbor_cells[0].earfcn = 8;
	test_neighbor_cells[0].time_diff = 3500;
	test_neighbor_cells[0].phys_cell_id = 60;
	test_neighbor_cells[0].rsrp = 29;
	test_neighbor_cells[0].rsrq = 4;
	test_neighbor_cells[1].earfcn = 9;
	test_neighbor_cells[1].time_diff = 5300;
	test_neighbor_cells[1].phys_cell_id = 99;
	test_neighbor_cells[1].rsrp = 18;
	test_neighbor_cells[1].rsrq = 5;

	at_monitor_dispatch(at_notif);
}

void test_lte_lc_neighbor_cell_measurement_normal_max_cells(void)
{
	int ret;
	struct lte_lc_ncellmeas_params params = {
		.search_type = LTE_LC_NEIGHBOR_SEARCH_TYPE_EXTENDED_COMPLETE,
		.gci_count = 0,
	};
	strcpy(at_notif,
	       "%NCELLMEAS:0,\"00112233\",\"98712\",\"0AB9\",4800,7,63,31,"
	       "456,4800,8,60,29,4,3500,9,99,18,5,5300,11\r\n");
	strcpy(at_notif,
	       /* Status */
	       "%NCELLMEAS: 0,"
	       /* Current cell */
	       "\"00112233\",\"98712\",\"0AB9\",4800,7,63,31,456,4800,"
	       /* Neighbor cells (20). Last 3 are extra items to be ignored. */
	       "333333,100,101,102,0,333333,103,104,105,0,"
	       "333333,106,107,108,0,333333,109,110,111,0,"
	       "444444,112,113,114,0,444444,115,116,117,0,"
	       "444444,118,119,120,0,444444,121,122,123,0,"
	       "555555,124,125,126,0,555555,127,128,129,0,"
	       "555555,130,131,132,0,555555,133,134,135,0,"
	       "666666,136,137,138,0,666666,139,140,141,0,"
	       "666666,142,143,144,0,666666,145,146,147,0,"
	       "777777,148,149,150,0,777777,151,152,153,0,"
	       "888888,154,155,156,0,888888,157,158,159,0,"
	       "11\r\n"); /* Timing advance for current cell */

	lte_lc_callback_count_expected = 1;

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS=2", EXIT_SUCCESS);

	ret = lte_lc_neighbor_cell_measurement(&params);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	test_event_data[0].type = LTE_LC_EVT_NEIGHBOR_CELL_MEAS;
	test_event_data[0].cells_info.current_cell.mcc = 987;
	test_event_data[0].cells_info.current_cell.mnc = 12;
	test_event_data[0].cells_info.current_cell.id = 0x00112233;
	test_event_data[0].cells_info.current_cell.tac = 0x0AB9;
	test_event_data[0].cells_info.current_cell.earfcn = 7;
	test_event_data[0].cells_info.current_cell.timing_advance = 4800;
	test_event_data[0].cells_info.current_cell.timing_advance_meas_time = 11;
	test_event_data[0].cells_info.current_cell.measurement_time = 4800;
	test_event_data[0].cells_info.current_cell.phys_cell_id = 63;
	test_event_data[0].cells_info.current_cell.rsrp = 31;
	test_event_data[0].cells_info.current_cell.rsrq = 456;
	test_event_data[0].cells_info.ncells_count = 17;
	test_event_data[0].cells_info.gci_cells_count = 0;

	/* Neighbor cells */
	test_neighbor_cells[0].earfcn = 333333;
	test_neighbor_cells[0].time_diff = 0;
	test_neighbor_cells[0].phys_cell_id = 100;
	test_neighbor_cells[0].rsrp = 101;
	test_neighbor_cells[0].rsrq = 102;

	test_neighbor_cells[1].earfcn = 333333;
	test_neighbor_cells[1].time_diff = 0;
	test_neighbor_cells[1].phys_cell_id = 103;
	test_neighbor_cells[1].rsrp = 104;
	test_neighbor_cells[1].rsrq = 105;

	test_neighbor_cells[2].earfcn = 333333;
	test_neighbor_cells[2].time_diff = 0;
	test_neighbor_cells[2].phys_cell_id = 106;
	test_neighbor_cells[2].rsrp = 107;
	test_neighbor_cells[2].rsrq = 108;

	test_neighbor_cells[3].earfcn = 333333;
	test_neighbor_cells[3].time_diff = 0;
	test_neighbor_cells[3].phys_cell_id = 109;
	test_neighbor_cells[3].rsrp = 110;
	test_neighbor_cells[3].rsrq = 111;

	test_neighbor_cells[4].earfcn = 444444;
	test_neighbor_cells[4].time_diff = 0;
	test_neighbor_cells[4].phys_cell_id = 112;
	test_neighbor_cells[4].rsrp = 113;
	test_neighbor_cells[4].rsrq = 114;

	test_neighbor_cells[5].earfcn = 444444;
	test_neighbor_cells[5].time_diff = 0;
	test_neighbor_cells[5].phys_cell_id = 115;
	test_neighbor_cells[5].rsrp = 116;
	test_neighbor_cells[5].rsrq = 117;

	test_neighbor_cells[6].earfcn = 444444;
	test_neighbor_cells[6].time_diff = 0;
	test_neighbor_cells[6].phys_cell_id = 118;
	test_neighbor_cells[6].rsrp = 119;
	test_neighbor_cells[6].rsrq = 120;

	test_neighbor_cells[7].earfcn = 444444;
	test_neighbor_cells[7].time_diff = 0;
	test_neighbor_cells[7].phys_cell_id = 121;
	test_neighbor_cells[7].rsrp = 122;
	test_neighbor_cells[7].rsrq = 123;

	test_neighbor_cells[8].earfcn = 555555;
	test_neighbor_cells[8].time_diff = 0;
	test_neighbor_cells[8].phys_cell_id = 124;
	test_neighbor_cells[8].rsrp = 125;
	test_neighbor_cells[8].rsrq = 126;

	test_neighbor_cells[9].earfcn = 555555;
	test_neighbor_cells[9].time_diff = 0;
	test_neighbor_cells[9].phys_cell_id = 127;
	test_neighbor_cells[9].rsrp = 128;
	test_neighbor_cells[9].rsrq = 129;

	test_neighbor_cells[10].earfcn = 555555;
	test_neighbor_cells[10].time_diff = 0;
	test_neighbor_cells[10].phys_cell_id = 130;
	test_neighbor_cells[10].rsrp = 131;
	test_neighbor_cells[10].rsrq = 132;

	test_neighbor_cells[11].earfcn = 555555;
	test_neighbor_cells[11].time_diff = 0;
	test_neighbor_cells[11].phys_cell_id = 133;
	test_neighbor_cells[11].rsrp = 134;
	test_neighbor_cells[11].rsrq = 135;

	test_neighbor_cells[12].earfcn = 666666;
	test_neighbor_cells[12].time_diff = 0;
	test_neighbor_cells[12].phys_cell_id = 136;
	test_neighbor_cells[12].rsrp = 137;
	test_neighbor_cells[12].rsrq = 138;

	test_neighbor_cells[13].earfcn = 666666;
	test_neighbor_cells[13].time_diff = 0;
	test_neighbor_cells[13].phys_cell_id = 139;
	test_neighbor_cells[13].rsrp = 140;
	test_neighbor_cells[13].rsrq = 141;

	test_neighbor_cells[14].earfcn = 666666;
	test_neighbor_cells[14].time_diff = 0;
	test_neighbor_cells[14].phys_cell_id = 142;
	test_neighbor_cells[14].rsrp = 143;
	test_neighbor_cells[14].rsrq = 144;

	test_neighbor_cells[15].earfcn = 666666;
	test_neighbor_cells[15].time_diff = 0;
	test_neighbor_cells[15].phys_cell_id = 145;
	test_neighbor_cells[15].rsrp = 146;
	test_neighbor_cells[15].rsrq = 147;

	test_neighbor_cells[16].earfcn = 777777;
	test_neighbor_cells[16].time_diff = 0;
	test_neighbor_cells[16].phys_cell_id = 148;
	test_neighbor_cells[16].rsrp = 149;
	test_neighbor_cells[16].rsrq = 150;

	at_monitor_dispatch(at_notif);
}

void test_lte_lc_neighbor_cell_measurement_normal_no_ncells(void)
{
	int ret;
	struct lte_lc_ncellmeas_params params = {
		.search_type = LTE_LC_NEIGHBOR_SEARCH_TYPE_DEFAULT,
		.gci_count = 0,
	};
	strcpy(at_notif,
	       "%NCELLMEAS: 0,\"1FFFFFFF\",\"98712\",\"0AB9\",4800,7,63,31,456,4800\r\n");

	lte_lc_callback_count_expected = 1;

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS", EXIT_SUCCESS);

	ret = lte_lc_neighbor_cell_measurement(&params);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	test_event_data[0].type = LTE_LC_EVT_NEIGHBOR_CELL_MEAS;
	test_event_data[0].cells_info.current_cell.mcc = 987;
	test_event_data[0].cells_info.current_cell.mnc = 12;
	test_event_data[0].cells_info.current_cell.id = LTE_LC_CELL_EUTRAN_ID_INVALID;
	test_event_data[0].cells_info.current_cell.tac = 0x0AB9;
	test_event_data[0].cells_info.current_cell.earfcn = 7;
	test_event_data[0].cells_info.current_cell.timing_advance = 4800;
	test_event_data[0].cells_info.current_cell.timing_advance_meas_time = 0;
	test_event_data[0].cells_info.current_cell.measurement_time = 4800;
	test_event_data[0].cells_info.current_cell.phys_cell_id = 63;
	test_event_data[0].cells_info.current_cell.rsrp = 31;
	test_event_data[0].cells_info.current_cell.rsrq = 456;
	test_event_data[0].cells_info.ncells_count = 0;
	test_event_data[0].cells_info.gci_cells_count = 0;

	/* Test that 2nd NCELLMEAS returns error */
	ret = lte_lc_neighbor_cell_measurement(NULL);
	TEST_ASSERT_EQUAL(-EINPROGRESS, ret);

	at_monitor_dispatch(at_notif);
}

void test_lte_lc_neighbor_cell_measurement_normal_status_failed(void)
{
	int ret;

	strcpy(at_notif,
	       "%NCELLMEAS: 1,\"00112233\",\"98712\",\"0AB9\",4800,7,63,31,"
	       "456,4800,8,60,29,4,3500,9,99,18,5,5300,11\r\n");

	lte_lc_callback_count_expected = 1;

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS", EXIT_SUCCESS);

	ret = lte_lc_neighbor_cell_measurement(NULL);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	test_event_data[0].type = LTE_LC_EVT_NEIGHBOR_CELL_MEAS;
	test_event_data[0].cells_info.current_cell.id = LTE_LC_CELL_EUTRAN_ID_INVALID;
	test_event_data[0].cells_info.ncells_count = 0;
	test_event_data[0].cells_info.gci_cells_count = 0;

	at_monitor_dispatch(at_notif);
}

void test_lte_lc_neighbor_cell_measurement_normal_status_incomplete(void)
{
	int ret;

	strcpy(at_notif,
	       "%NCELLMEAS: 2,\"00112233\",\"98712\",\"0AB9\",4800,7,63,31,"
	       "456,4800,8,60,29,4,3500,9,99,18,5,5300,11\r\n");

	lte_lc_callback_count_expected = 1;

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS", EXIT_SUCCESS);

	ret = lte_lc_neighbor_cell_measurement(NULL);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	test_event_data[0].type = LTE_LC_EVT_NEIGHBOR_CELL_MEAS;
	test_event_data[0].cells_info.current_cell.mcc = 987;
	test_event_data[0].cells_info.current_cell.mnc = 12;
	test_event_data[0].cells_info.current_cell.id = 0x00112233;
	test_event_data[0].cells_info.current_cell.tac = 0x0AB9;
	test_event_data[0].cells_info.current_cell.earfcn = 7;
	test_event_data[0].cells_info.current_cell.timing_advance = 4800;
	test_event_data[0].cells_info.current_cell.timing_advance_meas_time = 11;
	test_event_data[0].cells_info.current_cell.measurement_time = 4800;
	test_event_data[0].cells_info.current_cell.phys_cell_id = 63;
	test_event_data[0].cells_info.current_cell.rsrp = 31;
	test_event_data[0].cells_info.current_cell.rsrq = 456;
	test_event_data[0].cells_info.ncells_count = 2;
	test_event_data[0].cells_info.gci_cells_count = 0;
	test_neighbor_cells[0].earfcn = 8;
	test_neighbor_cells[0].time_diff = 3500;
	test_neighbor_cells[0].phys_cell_id = 60;
	test_neighbor_cells[0].rsrp = 29;
	test_neighbor_cells[0].rsrq = 4;
	test_neighbor_cells[1].earfcn = 9;
	test_neighbor_cells[1].time_diff = 5300;
	test_neighbor_cells[1].phys_cell_id = 99;
	test_neighbor_cells[1].rsrp = 18;
	test_neighbor_cells[1].rsrq = 5;

	at_monitor_dispatch(at_notif);
}

void test_lte_lc_neighbor_cell_measurement_normal_status_incomplete_no_cells(void)
{
	int ret;
	struct lte_lc_ncellmeas_params params = {
		.search_type = LTE_LC_NEIGHBOR_SEARCH_TYPE_DEFAULT,
		.gci_count = 0,
	};
	strcpy(at_notif,
	       "%NCELLMEAS: 2\r\n");

	lte_lc_callback_count_expected = 1;

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS", EXIT_SUCCESS);

	ret = lte_lc_neighbor_cell_measurement(&params);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	test_event_data[0].type = LTE_LC_EVT_NEIGHBOR_CELL_MEAS;
	test_event_data[0].cells_info.current_cell.mcc = 0;
	test_event_data[0].cells_info.current_cell.mnc = 0;
	test_event_data[0].cells_info.current_cell.id = LTE_LC_CELL_EUTRAN_ID_INVALID;
	test_event_data[0].cells_info.current_cell.tac = 0;
	test_event_data[0].cells_info.current_cell.earfcn = 0;
	test_event_data[0].cells_info.current_cell.timing_advance = 0;
	test_event_data[0].cells_info.current_cell.timing_advance_meas_time = 0;
	test_event_data[0].cells_info.current_cell.measurement_time = 0;
	test_event_data[0].cells_info.current_cell.phys_cell_id = 0;
	test_event_data[0].cells_info.current_cell.rsrp = 0;
	test_event_data[0].cells_info.current_cell.rsrq = 0;
	test_event_data[0].cells_info.ncells_count = 0;
	test_event_data[0].cells_info.gci_cells_count = 0;

	at_monitor_dispatch(at_notif);
}

void test_lte_lc_neighbor_cell_measurement_cell_id_missing_fail(void)
{
	int ret;

	strcpy(at_notif,
	       "%NCELLMEAS:0,,\"98712\",\"0AB9\",4800,7,63,31,456,4800,"
	       "8,60,29,4,3500,9,99,18,5,5300,11\r\n");

	lte_lc_callback_count_expected = 1;

	/* In case of an error, we're expected to receive an empty event. */
	test_event_data[0].type = LTE_LC_EVT_NEIGHBOR_CELL_MEAS;
	test_event_data[0].cells_info.current_cell.id = LTE_LC_CELL_EUTRAN_ID_INVALID;
	test_event_data[0].cells_info.ncells_count = 0;
	test_event_data[0].cells_info.gci_cells_count = 0;

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS", EXIT_SUCCESS);

	ret = lte_lc_neighbor_cell_measurement(NULL);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	at_monitor_dispatch(at_notif);
}

void test_lte_lc_neighbor_cell_measurement_cell_id_too_big_fail(void)
{
	int ret;

	strcpy(at_notif,
	       "%NCELLMEAS:0,\"FFFFFFFF\",\"98712\",\"0AB9\",4800,7,63,31,456,4800,"
	       "8,60,29,4,3500,9,99,18,5,5300,11\r\n");

	lte_lc_callback_count_expected = 1;

	/* In case of an error, we're expected to receive an empty event. */
	test_event_data[0].type = LTE_LC_EVT_NEIGHBOR_CELL_MEAS;
	test_event_data[0].cells_info.current_cell.id = LTE_LC_CELL_EUTRAN_ID_INVALID;
	test_event_data[0].cells_info.ncells_count = 0;
	test_event_data[0].cells_info.gci_cells_count = 0;

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS", EXIT_SUCCESS);

	ret = lte_lc_neighbor_cell_measurement(NULL);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	at_monitor_dispatch(at_notif);
}

void test_lte_lc_neighbor_cell_measurement_malformed_fail(void)
{
	int ret;

	strcpy(at_notif,
	       "%NCELLMEAS:0,\"00112233\",\"98712\",\"0AB9\",4800,7,63,31,456,4800,"
	       "8,60,29,4,35 00,9,99,18,5,5300,11\r\n");

	lte_lc_callback_count_expected = 1;

	/* In case of an error, we're expected to receive an empty event. */
	test_event_data[0].type = LTE_LC_EVT_NEIGHBOR_CELL_MEAS;
	test_event_data[0].cells_info.current_cell.id = LTE_LC_CELL_EUTRAN_ID_INVALID;
	test_event_data[0].cells_info.ncells_count = 0;
	test_event_data[0].cells_info.gci_cells_count = 0;

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS", EXIT_SUCCESS);

	ret = lte_lc_neighbor_cell_measurement(NULL);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	at_monitor_dispatch(at_notif);
}

void test_lte_lc_neighbor_cell_measurement_gci(void)
{
	int ret;
	struct lte_lc_ncellmeas_params params = {
		.search_type = LTE_LC_NEIGHBOR_SEARCH_TYPE_GCI_EXTENDED_LIGHT,
		.gci_count = 10,
	};
	strcpy(at_notif,
	       "%NCELLMEAS: 0,"
	       "\"1FFFFFFF\",\"11199\",\"1A2B\",64,20877,6200,110,53,22,189205,1,0,"
	       "\"00567812\",\"11198\",\"3C4D\",65535,4,1300,75,53,16,189241,0,0,"
	       "\"0011AABB\",\"11297\",\"5E6F\",65534,5,2300,449,51,11,189245,0,0\r\n");

	lte_lc_callback_count_expected = 1;

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS=4,10", EXIT_SUCCESS);

	ret = lte_lc_neighbor_cell_measurement(&params);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	test_event_data[0].type = LTE_LC_EVT_NEIGHBOR_CELL_MEAS;
	test_event_data[0].cells_info.current_cell.mcc = 111;
	test_event_data[0].cells_info.current_cell.mnc = 99;
	test_event_data[0].cells_info.current_cell.id = LTE_LC_CELL_EUTRAN_ID_INVALID;
	test_event_data[0].cells_info.current_cell.tac = 0x1A2B;
	test_event_data[0].cells_info.current_cell.earfcn = 6200;
	test_event_data[0].cells_info.current_cell.timing_advance = 64;
	test_event_data[0].cells_info.current_cell.timing_advance_meas_time = 20877;
	test_event_data[0].cells_info.current_cell.measurement_time = 189205;
	test_event_data[0].cells_info.current_cell.phys_cell_id = 110;
	test_event_data[0].cells_info.current_cell.rsrp = 53;
	test_event_data[0].cells_info.current_cell.rsrq = 22;
	test_event_data[0].cells_info.ncells_count = 0;
	test_event_data[0].cells_info.gci_cells_count = 2;

	test_gci_cells[0].mcc = 111;
	test_gci_cells[0].mnc = 98;
	test_gci_cells[0].id = 0x00567812;
	test_gci_cells[0].tac = 0x3C4D;
	test_gci_cells[0].earfcn = 1300;
	test_gci_cells[0].timing_advance = 65535;
	test_gci_cells[0].timing_advance_meas_time = 4;
	test_gci_cells[0].measurement_time = 189241;
	test_gci_cells[0].phys_cell_id = 75;
	test_gci_cells[0].rsrp = 53;
	test_gci_cells[0].rsrq = 16;

	test_gci_cells[1].mcc = 112;
	test_gci_cells[1].mnc = 97;
	test_gci_cells[1].id = 0x0011AABB;
	test_gci_cells[1].tac = 0x5E6F;
	test_gci_cells[1].earfcn = 2300;
	test_gci_cells[1].timing_advance = 65534;
	test_gci_cells[1].timing_advance_meas_time = 5;
	test_gci_cells[1].measurement_time = 189245;
	test_gci_cells[1].phys_cell_id = 449;
	test_gci_cells[1].rsrp = 51;
	test_gci_cells[1].rsrq = 11;

	at_monitor_dispatch(at_notif);
}

void test_lte_lc_neighbor_cell_measurement_gci_limit_results(void)
{
	int ret;
	struct lte_lc_ncellmeas_params params = {
		.search_type = LTE_LC_NEIGHBOR_SEARCH_TYPE_GCI_EXTENDED_LIGHT,
		.gci_count = 3,
	};
	strcpy(at_notif,
	       "%NCELLMEAS: 0,"
	       "\"1FFFFFFF\",\"11199\",\"1A2B\",64,20877,6200,110,53,22,189205,1,0,"
	       "\"00567812\",\"11198\",\"3C4D\",65535,4,1300,75,53,16,189241,0,0,"
	       "\"0011AABB\",\"11297\",\"5E6F\",65534,5,2300,449,51,11,189245,0,0,"
	       "\"0011CCDD\",\"11296\",\"5E6F\",65534,5,3300,449,51,11,189245,0,0,"
	       "\"0011EEFF\",\"11295\",\"5E6F\",65534,5,4300,449,51,11,189245,0,0"
	       "\r\n");

	lte_lc_callback_count_expected = 1;

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS=4,3", EXIT_SUCCESS);

	ret = lte_lc_neighbor_cell_measurement(&params);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	test_event_data[0].type = LTE_LC_EVT_NEIGHBOR_CELL_MEAS;
	test_event_data[0].cells_info.current_cell.mcc = 111;
	test_event_data[0].cells_info.current_cell.mnc = 99;
	test_event_data[0].cells_info.current_cell.id = LTE_LC_CELL_EUTRAN_ID_INVALID;
	test_event_data[0].cells_info.current_cell.tac = 0x1A2B;
	test_event_data[0].cells_info.current_cell.earfcn = 6200;
	test_event_data[0].cells_info.current_cell.timing_advance = 64;
	test_event_data[0].cells_info.current_cell.timing_advance_meas_time = 20877;
	test_event_data[0].cells_info.current_cell.measurement_time = 189205;
	test_event_data[0].cells_info.current_cell.phys_cell_id = 110;
	test_event_data[0].cells_info.current_cell.rsrp = 53;
	test_event_data[0].cells_info.current_cell.rsrq = 22;
	test_event_data[0].cells_info.ncells_count = 0;
	test_event_data[0].cells_info.gci_cells_count = 2;

	test_gci_cells[0].mcc = 111;
	test_gci_cells[0].mnc = 98;
	test_gci_cells[0].id = 0x00567812;
	test_gci_cells[0].tac = 0x3C4D;
	test_gci_cells[0].earfcn = 1300;
	test_gci_cells[0].timing_advance = 65535;
	test_gci_cells[0].timing_advance_meas_time = 4;
	test_gci_cells[0].measurement_time = 189241;
	test_gci_cells[0].phys_cell_id = 75;
	test_gci_cells[0].rsrp = 53;
	test_gci_cells[0].rsrq = 16;

	test_gci_cells[1].mcc = 112;
	test_gci_cells[1].mnc = 97;
	test_gci_cells[1].id = 0x0011AABB;
	test_gci_cells[1].tac = 0x5E6F;
	test_gci_cells[1].earfcn = 2300;
	test_gci_cells[1].timing_advance = 65534;
	test_gci_cells[1].timing_advance_meas_time = 5;
	test_gci_cells[1].measurement_time = 189245;
	test_gci_cells[1].phys_cell_id = 449;
	test_gci_cells[1].rsrp = 51;
	test_gci_cells[1].rsrq = 11;

	at_monitor_dispatch(at_notif);
}

void test_lte_lc_neighbor_cell_measurement_gci_status_failed(void)
{
	int ret;
	struct lte_lc_ncellmeas_params params = {
		.search_type = LTE_LC_NEIGHBOR_SEARCH_TYPE_GCI_EXTENDED_LIGHT,
		.gci_count = 10,
	};
	strcpy(at_notif,
	       "%NCELLMEAS: 1,"
	       "\"1FFFFFFF\",\"11199\",\"1A2B\",64,20877,6200,110,53,22,189205,1,0,"
	       "\"00567812\",\"11198\",\"3C4D\",65535,4,1300,75,53,16,189241,0,0,"
	       "\"0011AABB\",\"11297\",\"5E6F\",65534,5,2300,449,51,11,189245,0,0\r\n");

	lte_lc_callback_count_expected = 1;

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS=4,10", EXIT_SUCCESS);

	ret = lte_lc_neighbor_cell_measurement(&params);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	test_event_data[0].type = LTE_LC_EVT_NEIGHBOR_CELL_MEAS;
	test_event_data[0].cells_info.current_cell.id = LTE_LC_CELL_EUTRAN_ID_INVALID;
	test_event_data[0].cells_info.ncells_count = 0;
	test_event_data[0].cells_info.gci_cells_count = 0;
	at_monitor_dispatch(at_notif);
}

void test_lte_lc_neighbor_cell_measurement_gci_status_incomplete(void)
{
	int ret;
	struct lte_lc_ncellmeas_params params = {
		.search_type = LTE_LC_NEIGHBOR_SEARCH_TYPE_GCI_EXTENDED_LIGHT,
		.gci_count = 10,
	};
	strcpy(at_notif,
	       "%NCELLMEAS: 2,"
	       "\"1FFFFFFF\",\"11199\",\"1A2B\",64,20877,6200,110,53,22,189205,1,0,"
	       "\"00567812\",\"11198\",\"3C4D\",65535,4,1300,75,53,16,189241,0,0,"
	       "\"0011AABB\",\"11297\",\"5E6F\",65534,5,2300,449,51,11,189245,0,0\r\n");

	lte_lc_callback_count_expected = 1;

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS=4,10", EXIT_SUCCESS);

	ret = lte_lc_neighbor_cell_measurement(&params);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	test_event_data[0].type = LTE_LC_EVT_NEIGHBOR_CELL_MEAS;
	test_event_data[0].cells_info.current_cell.mcc = 111;
	test_event_data[0].cells_info.current_cell.mnc = 99;
	test_event_data[0].cells_info.current_cell.id = LTE_LC_CELL_EUTRAN_ID_INVALID;
	test_event_data[0].cells_info.current_cell.tac = 0x1A2B;
	test_event_data[0].cells_info.current_cell.earfcn = 6200;
	test_event_data[0].cells_info.current_cell.timing_advance = 64;
	test_event_data[0].cells_info.current_cell.timing_advance_meas_time = 20877;
	test_event_data[0].cells_info.current_cell.measurement_time = 189205;
	test_event_data[0].cells_info.current_cell.phys_cell_id = 110;
	test_event_data[0].cells_info.current_cell.rsrp = 53;
	test_event_data[0].cells_info.current_cell.rsrq = 22;
	test_event_data[0].cells_info.ncells_count = 0;
	test_event_data[0].cells_info.gci_cells_count = 2;

	test_gci_cells[0].mcc = 111;
	test_gci_cells[0].mnc = 98;
	test_gci_cells[0].id = 0x00567812;
	test_gci_cells[0].tac = 0x3C4D;
	test_gci_cells[0].earfcn = 1300;
	test_gci_cells[0].timing_advance = 65535;
	test_gci_cells[0].timing_advance_meas_time = 4;
	test_gci_cells[0].measurement_time = 189241;
	test_gci_cells[0].phys_cell_id = 75;
	test_gci_cells[0].rsrp = 53;
	test_gci_cells[0].rsrq = 16;

	test_gci_cells[1].mcc = 112;
	test_gci_cells[1].mnc = 97;
	test_gci_cells[1].id = 0x0011AABB;
	test_gci_cells[1].tac = 0x5E6F;
	test_gci_cells[1].earfcn = 2300;
	test_gci_cells[1].timing_advance = 65534;
	test_gci_cells[1].timing_advance_meas_time = 5;
	test_gci_cells[1].measurement_time = 189245;
	test_gci_cells[1].phys_cell_id = 449;
	test_gci_cells[1].rsrp = 51;
	test_gci_cells[1].rsrq = 11;

	at_monitor_dispatch(at_notif);
}

void test_lte_lc_neighbor_cell_measurement_gci_status_incomplete_no_cells(void)
{
	int ret;
	struct lte_lc_ncellmeas_params params = {
		.search_type = LTE_LC_NEIGHBOR_SEARCH_TYPE_GCI_DEFAULT,
		.gci_count = 2,
	};
	strcpy(at_notif,
	       "%NCELLMEAS: 2\r\n");

	lte_lc_callback_count_expected = 1;

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS=3,2", EXIT_SUCCESS);

	ret = lte_lc_neighbor_cell_measurement(&params);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	test_event_data[0].type = LTE_LC_EVT_NEIGHBOR_CELL_MEAS;
	test_event_data[0].cells_info.current_cell.mcc = 0;
	test_event_data[0].cells_info.current_cell.mnc = 0;
	test_event_data[0].cells_info.current_cell.id = LTE_LC_CELL_EUTRAN_ID_INVALID;
	test_event_data[0].cells_info.current_cell.tac = 0;
	test_event_data[0].cells_info.current_cell.earfcn = 0;
	test_event_data[0].cells_info.current_cell.timing_advance = 0;
	test_event_data[0].cells_info.current_cell.timing_advance_meas_time = 0;
	test_event_data[0].cells_info.current_cell.measurement_time = 0;
	test_event_data[0].cells_info.current_cell.phys_cell_id = 0;
	test_event_data[0].cells_info.current_cell.rsrp = 0;
	test_event_data[0].cells_info.current_cell.rsrq = 0;
	test_event_data[0].cells_info.ncells_count = 0;
	test_event_data[0].cells_info.gci_cells_count = 0;

	at_monitor_dispatch(at_notif);
}

void test_lte_lc_neighbor_cell_measurement_gci_max_length(void)
{
	int ret;
	struct lte_lc_ncellmeas_params params = {
		.search_type = LTE_LC_NEIGHBOR_SEARCH_TYPE_GCI_EXTENDED_COMPLETE,
		.gci_count = 15,
	};

	strcpy(at_notif,
	       /* Status */
	       "%NCELLMEAS: 0,"
	       /* Current cell */
	       "\"00123456\",\"555555\",\"0102\",65534,18446744073709551614,"
	       "999999,123,127,-127,18446744073709551614,1,20,"
	       /* Neighbor cells (20). Last 3 are extra items to be ignored. */
	       "333333,100,101,102,0,333333,103,104,105,0,"
	       "333333,106,107,108,0,333333,109,110,111,0,"
	       "444444,112,113,114,0,444444,115,116,117,0,"
	       "444444,118,119,120,0,444444,121,122,123,0,"
	       "555555,124,125,126,0,555555,127,128,129,0,"
	       "555555,130,131,132,0,555555,133,134,135,0,"
	       "666666,136,137,138,0,666666,139,140,141,0,"
	       "666666,142,143,144,0,666666,145,146,147,0,"
	       "777777,148,149,150,0,777777,151,152,153,0,"
	       "888888,154,155,156,0,888888,157,158,159,0,"
	       /* GCI (surrounding) cells (14) */
	       "\"01234567\",\"555555\",\"0102\",65534,18446744073709551614,999999,123,127,-127,18446744073709551614,0,0,"
	       "\"02345678\",\"555555\",\"0102\",65534,18446744073709551614,999999,123,127,-127,18446744073709551614,0,0,"
	       "\"03456789\",\"555555\",\"0102\",65534,18446744073709551614,999999,123,127,-127,18446744073709551614,0,0,"
	       "\"0456789A\",\"555555\",\"0102\",65534,18446744073709551614,999999,123,127,-127,18446744073709551614,0,0,"
	       "\"056789AB\",\"555555\",\"0102\",65534,18446744073709551614,999999,123,127,-127,18446744073709551614,0,0,"
	       "\"06789ABC\",\"555555\",\"0102\",65534,18446744073709551614,999999,123,127,-127,18446744073709551614,0,0,"
	       "\"0789ABCD\",\"555555\",\"0102\",65534,18446744073709551614,999999,123,127,-127,18446744073709551614,0,0,"
	       "\"089ABCDE\",\"555555\",\"0102\",65534,18446744073709551614,999999,123,127,-127,18446744073709551614,0,0,"
	       "\"09ABCDEF\",\"555555\",\"0102\",65534,18446744073709551614,999999,123,127,-127,18446744073709551614,0,0,"
	       "\"0ABCDEF0\",\"555555\",\"0102\",65534,18446744073709551614,999999,123,127,-127,18446744073709551614,0,0,"
	       "\"0BCDEF01\",\"555555\",\"0102\",65534,18446744073709551614,999999,123,127,-127,18446744073709551614,0,0,"
	       "\"0CDEF012\",\"555555\",\"0102\",65534,18446744073709551614,999999,123,127,-127,18446744073709551614,0,0,"
	       "\"0DEF0123\",\"555555\",\"0102\",65534,18446744073709551614,999999,123,127,-127,18446744073709551614,0,0,"
	       "\"0EF01234\",\"555555\",\"0102\",65534,18446744073709551614,999999,123,127,-127,18446744073709551614,0,0\r\n");

	lte_lc_callback_count_expected = 1;

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS=5,15", EXIT_SUCCESS);

	ret = lte_lc_neighbor_cell_measurement(&params);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	test_event_data[0].type = LTE_LC_EVT_NEIGHBOR_CELL_MEAS;
	test_event_data[0].cells_info.current_cell.mcc = 555;
	test_event_data[0].cells_info.current_cell.mnc = 555;
	test_event_data[0].cells_info.current_cell.id = 0x00123456;
	test_event_data[0].cells_info.current_cell.tac = 0x0102;
	test_event_data[0].cells_info.current_cell.earfcn = 999999;
	test_event_data[0].cells_info.current_cell.timing_advance = 65534;
	test_event_data[0].cells_info.current_cell.timing_advance_meas_time = 18446744073709551614U;
	test_event_data[0].cells_info.current_cell.measurement_time = 18446744073709551614U;
	test_event_data[0].cells_info.current_cell.phys_cell_id = 123;
	test_event_data[0].cells_info.current_cell.rsrp = 127;
	test_event_data[0].cells_info.current_cell.rsrq = -127;
	test_event_data[0].cells_info.ncells_count = 17;
	test_event_data[0].cells_info.gci_cells_count = 14;

	/* Neighbor cells */
	test_neighbor_cells[0].earfcn = 333333;
	test_neighbor_cells[0].time_diff = 0;
	test_neighbor_cells[0].phys_cell_id = 100;
	test_neighbor_cells[0].rsrp = 101;
	test_neighbor_cells[0].rsrq = 102;

	test_neighbor_cells[1].earfcn = 333333;
	test_neighbor_cells[1].time_diff = 0;
	test_neighbor_cells[1].phys_cell_id = 103;
	test_neighbor_cells[1].rsrp = 104;
	test_neighbor_cells[1].rsrq = 105;

	test_neighbor_cells[2].earfcn = 333333;
	test_neighbor_cells[2].time_diff = 0;
	test_neighbor_cells[2].phys_cell_id = 106;
	test_neighbor_cells[2].rsrp = 107;
	test_neighbor_cells[2].rsrq = 108;

	test_neighbor_cells[3].earfcn = 333333;
	test_neighbor_cells[3].time_diff = 0;
	test_neighbor_cells[3].phys_cell_id = 109;
	test_neighbor_cells[3].rsrp = 110;
	test_neighbor_cells[3].rsrq = 111;

	test_neighbor_cells[4].earfcn = 444444;
	test_neighbor_cells[4].time_diff = 0;
	test_neighbor_cells[4].phys_cell_id = 112;
	test_neighbor_cells[4].rsrp = 113;
	test_neighbor_cells[4].rsrq = 114;

	test_neighbor_cells[5].earfcn = 444444;
	test_neighbor_cells[5].time_diff = 0;
	test_neighbor_cells[5].phys_cell_id = 115;
	test_neighbor_cells[5].rsrp = 116;
	test_neighbor_cells[5].rsrq = 117;

	test_neighbor_cells[6].earfcn = 444444;
	test_neighbor_cells[6].time_diff = 0;
	test_neighbor_cells[6].phys_cell_id = 118;
	test_neighbor_cells[6].rsrp = 119;
	test_neighbor_cells[6].rsrq = 120;

	test_neighbor_cells[7].earfcn = 444444;
	test_neighbor_cells[7].time_diff = 0;
	test_neighbor_cells[7].phys_cell_id = 121;
	test_neighbor_cells[7].rsrp = 122;
	test_neighbor_cells[7].rsrq = 123;

	test_neighbor_cells[8].earfcn = 555555;
	test_neighbor_cells[8].time_diff = 0;
	test_neighbor_cells[8].phys_cell_id = 124;
	test_neighbor_cells[8].rsrp = 125;
	test_neighbor_cells[8].rsrq = 126;

	test_neighbor_cells[9].earfcn = 555555;
	test_neighbor_cells[9].time_diff = 0;
	test_neighbor_cells[9].phys_cell_id = 127;
	test_neighbor_cells[9].rsrp = 128;
	test_neighbor_cells[9].rsrq = 129;

	test_neighbor_cells[10].earfcn = 555555;
	test_neighbor_cells[10].time_diff = 0;
	test_neighbor_cells[10].phys_cell_id = 130;
	test_neighbor_cells[10].rsrp = 131;
	test_neighbor_cells[10].rsrq = 132;

	test_neighbor_cells[11].earfcn = 555555;
	test_neighbor_cells[11].time_diff = 0;
	test_neighbor_cells[11].phys_cell_id = 133;
	test_neighbor_cells[11].rsrp = 134;
	test_neighbor_cells[11].rsrq = 135;

	test_neighbor_cells[12].earfcn = 666666;
	test_neighbor_cells[12].time_diff = 0;
	test_neighbor_cells[12].phys_cell_id = 136;
	test_neighbor_cells[12].rsrp = 137;
	test_neighbor_cells[12].rsrq = 138;

	test_neighbor_cells[13].earfcn = 666666;
	test_neighbor_cells[13].time_diff = 0;
	test_neighbor_cells[13].phys_cell_id = 139;
	test_neighbor_cells[13].rsrp = 140;
	test_neighbor_cells[13].rsrq = 141;

	test_neighbor_cells[14].earfcn = 666666;
	test_neighbor_cells[14].time_diff = 0;
	test_neighbor_cells[14].phys_cell_id = 142;
	test_neighbor_cells[14].rsrp = 143;
	test_neighbor_cells[14].rsrq = 144;

	test_neighbor_cells[15].earfcn = 666666;
	test_neighbor_cells[15].time_diff = 0;
	test_neighbor_cells[15].phys_cell_id = 145;
	test_neighbor_cells[15].rsrp = 146;
	test_neighbor_cells[15].rsrq = 147;

	test_neighbor_cells[16].earfcn = 777777;
	test_neighbor_cells[16].time_diff = 0;
	test_neighbor_cells[16].phys_cell_id = 148;
	test_neighbor_cells[16].rsrp = 149;
	test_neighbor_cells[16].rsrq = 150;

	/* GCI (surrounding) cells */
	test_gci_cells[0].mcc = 555;
	test_gci_cells[0].mnc = 555;
	test_gci_cells[0].id = 0x01234567;
	test_gci_cells[0].tac = 0x0102;
	test_gci_cells[0].earfcn = 999999;
	test_gci_cells[0].timing_advance = 65534;
	test_gci_cells[0].timing_advance_meas_time = 18446744073709551614U;
	test_gci_cells[0].measurement_time = 18446744073709551614U;
	test_gci_cells[0].phys_cell_id = 123;
	test_gci_cells[0].rsrp = 127;
	test_gci_cells[0].rsrq = -127;

	test_gci_cells[1].mcc = 555;
	test_gci_cells[1].mnc = 555;
	test_gci_cells[1].id = 0x02345678;
	test_gci_cells[1].tac = 0x0102;
	test_gci_cells[1].earfcn = 999999;
	test_gci_cells[1].timing_advance = 65534;
	test_gci_cells[1].timing_advance_meas_time = 18446744073709551614U;
	test_gci_cells[1].measurement_time = 18446744073709551614U;
	test_gci_cells[1].phys_cell_id = 123;
	test_gci_cells[1].rsrp = 127;
	test_gci_cells[1].rsrq = -127;

	test_gci_cells[2].mcc = 555;
	test_gci_cells[2].mnc = 555;
	test_gci_cells[2].id = 0x03456789;
	test_gci_cells[2].tac = 0x0102;
	test_gci_cells[2].earfcn = 999999;
	test_gci_cells[2].timing_advance = 65534;
	test_gci_cells[2].timing_advance_meas_time = 18446744073709551614U;
	test_gci_cells[2].measurement_time = 18446744073709551614U;
	test_gci_cells[2].phys_cell_id = 123;
	test_gci_cells[2].rsrp = 127;
	test_gci_cells[2].rsrq = -127;

	test_gci_cells[3].mcc = 555;
	test_gci_cells[3].mnc = 555;
	test_gci_cells[3].id = 0x0456789A;
	test_gci_cells[3].tac = 0x0102;
	test_gci_cells[3].earfcn = 999999;
	test_gci_cells[3].timing_advance = 65534;
	test_gci_cells[3].timing_advance_meas_time = 18446744073709551614U;
	test_gci_cells[3].measurement_time = 18446744073709551614U;
	test_gci_cells[3].phys_cell_id = 123;
	test_gci_cells[3].rsrp = 127;
	test_gci_cells[3].rsrq = -127;

	test_gci_cells[4].mcc = 555;
	test_gci_cells[4].mnc = 555;
	test_gci_cells[4].id = 0x056789AB;
	test_gci_cells[4].tac = 0x0102;
	test_gci_cells[4].earfcn = 999999;
	test_gci_cells[4].timing_advance = 65534;
	test_gci_cells[4].timing_advance_meas_time = 18446744073709551614U;
	test_gci_cells[4].measurement_time = 18446744073709551614U;
	test_gci_cells[4].phys_cell_id = 123;
	test_gci_cells[4].rsrp = 127;
	test_gci_cells[4].rsrq = -127;

	test_gci_cells[5].mcc = 555;
	test_gci_cells[5].mnc = 555;
	test_gci_cells[5].id = 0x06789ABC;
	test_gci_cells[5].tac = 0x0102;
	test_gci_cells[5].earfcn = 999999;
	test_gci_cells[5].timing_advance = 65534;
	test_gci_cells[5].timing_advance_meas_time = 18446744073709551614U;
	test_gci_cells[5].measurement_time = 18446744073709551614U;
	test_gci_cells[5].phys_cell_id = 123;
	test_gci_cells[5].rsrp = 127;
	test_gci_cells[5].rsrq = -127;

	test_gci_cells[6].mcc = 555;
	test_gci_cells[6].mnc = 555;
	test_gci_cells[6].id = 0x0789ABCD;
	test_gci_cells[6].tac = 0x0102;
	test_gci_cells[6].earfcn = 999999;
	test_gci_cells[6].timing_advance = 65534;
	test_gci_cells[6].timing_advance_meas_time = 18446744073709551614U;
	test_gci_cells[6].measurement_time = 18446744073709551614U;
	test_gci_cells[6].phys_cell_id = 123;
	test_gci_cells[6].rsrp = 127;
	test_gci_cells[6].rsrq = -127;

	test_gci_cells[7].mcc = 555;
	test_gci_cells[7].mnc = 555;
	test_gci_cells[7].id = 0x089ABCDE;
	test_gci_cells[7].tac = 0x0102;
	test_gci_cells[7].earfcn = 999999;
	test_gci_cells[7].timing_advance = 65534;
	test_gci_cells[7].timing_advance_meas_time = 18446744073709551614U;
	test_gci_cells[7].measurement_time = 18446744073709551614U;
	test_gci_cells[7].phys_cell_id = 123;
	test_gci_cells[7].rsrp = 127;
	test_gci_cells[7].rsrq = -127;

	test_gci_cells[8].mcc = 555;
	test_gci_cells[8].mnc = 555;
	test_gci_cells[8].id = 0x09ABCDEF;
	test_gci_cells[8].tac = 0x0102;
	test_gci_cells[8].earfcn = 999999;
	test_gci_cells[8].timing_advance = 65534;
	test_gci_cells[8].timing_advance_meas_time = 18446744073709551614U;
	test_gci_cells[8].measurement_time = 18446744073709551614U;
	test_gci_cells[8].phys_cell_id = 123;
	test_gci_cells[8].rsrp = 127;
	test_gci_cells[8].rsrq = -127;

	test_gci_cells[9].mcc = 555;
	test_gci_cells[9].mnc = 555;
	test_gci_cells[9].id = 0x0ABCDEF0;
	test_gci_cells[9].tac = 0x0102;
	test_gci_cells[9].earfcn = 999999;
	test_gci_cells[9].timing_advance = 65534;
	test_gci_cells[9].timing_advance_meas_time = 18446744073709551614U;
	test_gci_cells[9].measurement_time = 18446744073709551614U;
	test_gci_cells[9].phys_cell_id = 123;
	test_gci_cells[9].rsrp = 127;
	test_gci_cells[9].rsrq = -127;

	test_gci_cells[10].mcc = 555;
	test_gci_cells[10].mnc = 555;
	test_gci_cells[10].id = 0x0BCDEF01;
	test_gci_cells[10].tac = 0x0102;
	test_gci_cells[10].earfcn = 999999;
	test_gci_cells[10].timing_advance = 65534;
	test_gci_cells[10].timing_advance_meas_time = 18446744073709551614U;
	test_gci_cells[10].measurement_time = 18446744073709551614U;
	test_gci_cells[10].phys_cell_id = 123;
	test_gci_cells[10].rsrp = 127;
	test_gci_cells[10].rsrq = -127;

	test_gci_cells[11].mcc = 555;
	test_gci_cells[11].mnc = 555;
	test_gci_cells[11].id = 0x0CDEF012;
	test_gci_cells[11].tac = 0x0102;
	test_gci_cells[11].earfcn = 999999;
	test_gci_cells[11].timing_advance = 65534;
	test_gci_cells[11].timing_advance_meas_time = 18446744073709551614U;
	test_gci_cells[11].measurement_time = 18446744073709551614U;
	test_gci_cells[11].phys_cell_id = 123;
	test_gci_cells[11].rsrp = 127;
	test_gci_cells[11].rsrq = -127;

	test_gci_cells[12].mcc = 555;
	test_gci_cells[12].mnc = 555;
	test_gci_cells[12].id = 0x0DEF0123;
	test_gci_cells[12].tac = 0x0102;
	test_gci_cells[12].earfcn = 999999;
	test_gci_cells[12].timing_advance = 65534;
	test_gci_cells[12].timing_advance_meas_time = 18446744073709551614U;
	test_gci_cells[12].measurement_time = 18446744073709551614U;
	test_gci_cells[12].phys_cell_id = 123;
	test_gci_cells[12].rsrp = 127;
	test_gci_cells[12].rsrq = -127;

	test_gci_cells[13].mcc = 555;
	test_gci_cells[13].mnc = 555;
	test_gci_cells[13].id = 0x0EF01234;
	test_gci_cells[13].tac = 0x0102;
	test_gci_cells[13].earfcn = 999999;
	test_gci_cells[13].timing_advance = 65534;
	test_gci_cells[13].timing_advance_meas_time = 18446744073709551614U;
	test_gci_cells[13].measurement_time = 18446744073709551614U;
	test_gci_cells[13].phys_cell_id = 123;
	test_gci_cells[13].rsrp = 127;
	test_gci_cells[13].rsrq = -127;

	at_monitor_dispatch(at_notif);
}

void test_lte_lc_neighbor_cell_measurement_cancel(void)
{
	int ret;
	struct lte_lc_ncellmeas_params params = {
		.search_type = LTE_LC_NEIGHBOR_SEARCH_TYPE_DEFAULT,
		.gci_count = 0,
	};
	strcpy(at_notif,
	       "%NCELLMEAS:0,\"00112233\",\"98712\",\"0AB9\",4800,7,63,31,"
	       "456,4800,8,60,29,4,3500,9,99,18,5,5300,11\r\n");

	lte_lc_callback_count_expected = 2;

	test_event_data[0].type = LTE_LC_EVT_NEIGHBOR_CELL_MEAS;
	test_event_data[0].cells_info.current_cell.mcc = 987;
	test_event_data[0].cells_info.current_cell.mnc = 12;
	test_event_data[0].cells_info.current_cell.id = 0x00112233;
	test_event_data[0].cells_info.current_cell.tac = 0x0AB9;
	test_event_data[0].cells_info.current_cell.earfcn = 7;
	test_event_data[0].cells_info.current_cell.timing_advance = 4800;
	test_event_data[0].cells_info.current_cell.timing_advance_meas_time = 11;
	test_event_data[0].cells_info.current_cell.measurement_time = 4800;
	test_event_data[0].cells_info.current_cell.phys_cell_id = 63;
	test_event_data[0].cells_info.current_cell.rsrp = 31;
	test_event_data[0].cells_info.current_cell.rsrq = 456;
	test_event_data[0].cells_info.ncells_count = 2;
	test_event_data[0].cells_info.gci_cells_count = 0;

	test_event_data[1].type = LTE_LC_EVT_NEIGHBOR_CELL_MEAS;
	test_event_data[1].cells_info.current_cell.mcc = 987;
	test_event_data[1].cells_info.current_cell.mnc = 12;
	test_event_data[1].cells_info.current_cell.id = 0x00112233;
	test_event_data[1].cells_info.current_cell.tac = 0x0AB9;
	test_event_data[1].cells_info.current_cell.earfcn = 7;
	test_event_data[1].cells_info.current_cell.timing_advance = 4800;
	test_event_data[1].cells_info.current_cell.timing_advance_meas_time = 11;
	test_event_data[1].cells_info.current_cell.measurement_time = 4800;
	test_event_data[1].cells_info.current_cell.phys_cell_id = 63;
	test_event_data[1].cells_info.current_cell.rsrp = 31;
	test_event_data[1].cells_info.current_cell.rsrq = 456;
	test_event_data[1].cells_info.ncells_count = 2;
	test_event_data[1].cells_info.gci_cells_count = 0;

	test_neighbor_cells[0].earfcn = 8;
	test_neighbor_cells[0].time_diff = 3500;
	test_neighbor_cells[0].phys_cell_id = 60;
	test_neighbor_cells[0].rsrp = 29;
	test_neighbor_cells[0].rsrq = 4;

	test_neighbor_cells[1].earfcn = 9;
	test_neighbor_cells[1].time_diff = 5300;
	test_neighbor_cells[1].phys_cell_id = 99;
	test_neighbor_cells[1].rsrp = 18;
	test_neighbor_cells[1].rsrq = 5;

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS", EXIT_SUCCESS);

	ret = lte_lc_neighbor_cell_measurement(&params);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEASSTOP", EXIT_SUCCESS);

	ret = lte_lc_neighbor_cell_measurement_cancel();
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	at_monitor_dispatch(at_notif);

	/* Start a new measurement immediately, to make sure the cancel timeout does not mess
	 * things up.
	 */

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS", EXIT_SUCCESS);

	ret = lte_lc_neighbor_cell_measurement(&params);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	/* Wait until the cancel timeout would expire. */
	k_sleep(K_MSEC(2100));

	at_monitor_dispatch(at_notif);
}

void test_lte_lc_neighbor_cell_measurement_cancel_no_notif(void)
{
	int ret;
	struct lte_lc_ncellmeas_params params = {
		.search_type = LTE_LC_NEIGHBOR_SEARCH_TYPE_DEFAULT,
		.gci_count = 0,
	};

	lte_lc_callback_count_expected = 1;

	test_event_data[0].type = LTE_LC_EVT_NEIGHBOR_CELL_MEAS;
	test_event_data[0].cells_info.current_cell.id = LTE_LC_CELL_EUTRAN_ID_INVALID;

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS", EXIT_SUCCESS);

	ret = lte_lc_neighbor_cell_measurement(&params);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEASSTOP", EXIT_SUCCESS);

	ret = lte_lc_neighbor_cell_measurement_cancel();
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	/* After lte_lc_neighbor_cell_measurement_cancel(), new measurement can not be started
	 * before a %NCELLMEAS notification is received or a timeout happens.
	 */
	ret = lte_lc_neighbor_cell_measurement(&params);
	TEST_ASSERT_EQUAL(-EINPROGRESS, ret);

	k_sleep(K_MSEC(2100));
}

void test_lte_lc_neighbor_cell_measurement_cancel_fail(void)
{
	int ret;

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEASSTOP", -NRF_ENOMEM);
	ret = lte_lc_neighbor_cell_measurement_cancel();
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_neighbor_cell_measurement_normal_invalid_field_format_fail(void)
{
	int ret;
	struct lte_lc_ncellmeas_params params = {
		.search_type = LTE_LC_NEIGHBOR_SEARCH_TYPE_EXTENDED_LIGHT,
		.gci_count = 0,
	};

	lte_lc_callback_count_expected = 16;

	/* In case of an error, we're expected to receive an empty event. */
	for (int i = 0; i < lte_lc_callback_count_expected; i++) {
		test_event_data[i].type = LTE_LC_EVT_NEIGHBOR_CELL_MEAS;
		test_event_data[i].cells_info.current_cell.id = LTE_LC_CELL_EUTRAN_ID_INVALID;
		test_event_data[i].cells_info.ncells_count = 0;
		test_event_data[i].cells_info.gci_cells_count = 0;
	}

	/* Syntax:
	 * %NCELLMEAS: status
	 * [,<cell_id>,<plmn>,<tac>,<ta>,<earfcn>,<phys_cell_id>,<rsrp>,<rsrq>,<meas_time>
	 * [,<n_earfcn>1,<n_phys_cell_id>1,<n_rsrp>1,<n_rsrq>1,<time_diff>1]
	 * [,<n_earfcn>2,<n_phys_cell_id>2,<n_rsrp>2,<n_rsrq>2,<time_diff>2]
	 * ...
	 * [,<n_earfcn>17,<n_phys_cell_id>17,<n_rsrp>17,<n_rsrq>17,<time_diff>17]
	 * [,<timing_advance_measurement_time>]
	 */

	/* status */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS", EXIT_SUCCESS);
	ret = lte_lc_neighbor_cell_measurement(NULL);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	strcpy(at_notif,
	       "%NCELLMEAS:invalid,\"00112233\",\"98712\",\"0AB9\",4800,7,63,31,456,4800,"
	       "8,60,29,4,3500,9,99,18,5,5300\r\n");
	at_monitor_dispatch(at_notif);

	/* <cell_id> */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS", EXIT_SUCCESS);
	ret = lte_lc_neighbor_cell_measurement(NULL);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	strcpy(at_notif,
	       "%NCELLMEAS: 0,invalid,\"98712\",\"0AB9\",4800,7,63,31,456,4800,"
	       "8,60,29,4,3500,9,99,18,5,5300\r\n");
	at_monitor_dispatch(at_notif);

	/* <plmn> */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS", EXIT_SUCCESS);
	ret = lte_lc_neighbor_cell_measurement(NULL);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	strcpy(at_notif,
	       "%NCELLMEAS: 0,\"00112233\",invalid,\"0AB9\",4800,7,63,31,456,4800,"
	       "8,60,29,4,3500,9,99,18,5,5300\r\n");
	at_monitor_dispatch(at_notif);

	/* <tac> */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS", EXIT_SUCCESS);
	ret = lte_lc_neighbor_cell_measurement(NULL);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	strcpy(at_notif,
	       "%NCELLMEAS: 0,\"00112233\",\"98712\",invalid,4800,7,63,31,456,4800,"
	       "8,60,29,4,3500,9,99,18,5,5300\r\n");
	at_monitor_dispatch(at_notif);

	/* <ta> */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS", EXIT_SUCCESS);
	ret = lte_lc_neighbor_cell_measurement(NULL);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	strcpy(at_notif,
	       "%NCELLMEAS: 0,\"00112233\",\"98712\",\"0AB9\",invalid,7,63,31,456,4800,"
	       "8,60,29,4,3500,9,99,18,5,5300\r\n");
	at_monitor_dispatch(at_notif);

	/* <earfcn> */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS", EXIT_SUCCESS);
	ret = lte_lc_neighbor_cell_measurement(NULL);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	strcpy(at_notif,
	       "%NCELLMEAS: 0,\"00112233\",\"98712\",\"0AB9\",4800,invalid,63,31,456,4800,"
	       "8,60,29,4,3500,9,99,18,5,5300\r\n");
	at_monitor_dispatch(at_notif);

	/* <phys_cell_id> */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS", EXIT_SUCCESS);
	ret = lte_lc_neighbor_cell_measurement(NULL);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	strcpy(at_notif,
	       "%NCELLMEAS: 0,\"00112233\",\"98712\",\"0AB9\",4800,7,invalid,31,456,4800,"
	       "8,60,29,4,3500,9,99,18,5,5300\r\n");
	at_monitor_dispatch(at_notif);

	/* <rsrp> */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS", EXIT_SUCCESS);
	ret = lte_lc_neighbor_cell_measurement(NULL);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	strcpy(at_notif,
	       "%NCELLMEAS: 0,\"00112233\",\"98712\",\"0AB9\",4800,7,63,invalid,456,4800,"
	       "8,60,29,4,3500,9,99,18,5,5300\r\n");
	at_monitor_dispatch(at_notif);

	/* <rsrq> */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS", EXIT_SUCCESS);
	ret = lte_lc_neighbor_cell_measurement(NULL);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	strcpy(at_notif,
	       "%NCELLMEAS: 0,\"00112233\",\"98712\",\"0AB9\",4800,7,63,31,invalid,4800,"
	       "8,60,29,4,3500,9,99,18,5,5300\r\n");
	at_monitor_dispatch(at_notif);

	/* <meas_time> */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS", EXIT_SUCCESS);
	ret = lte_lc_neighbor_cell_measurement(NULL);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	strcpy(at_notif,
	       "%NCELLMEAS: 0,\"00112233\",\"98712\",\"0AB9\",4800,7,63,31,456,invalid,"
	       "8,60,29,4,3500,9,99,18,5,5300\r\n");
	at_monitor_dispatch(at_notif);

	/* <timing_advance_measurement_time>: this is the last field after all neighbors */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS", EXIT_SUCCESS);
	ret = lte_lc_neighbor_cell_measurement(NULL);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	strcpy(at_notif,
	       "%NCELLMEAS: 0,\"00112233\",\"98712\",\"0AB9\",4800,7,63,31,456,4800,"
	       "8,60,29,4,3500,9,99,18,5,5300,invalid\r\n");
	at_monitor_dispatch(at_notif);

	/***** Neighbor cell fields *****/

	/* <n_earfcn> */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS=1", EXIT_SUCCESS);
	ret = lte_lc_neighbor_cell_measurement(&params);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	strcpy(at_notif,
	       "%NCELLMEAS: 0,\"00112233\",\"98712\",\"0AB9\",4800,7,63,31,456,4800,"
	       "8,60,29,4,3500,invalid,99,18,5,5300\r\n");
	at_monitor_dispatch(at_notif);

	/* <n_phys_cell_id> */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS=1", EXIT_SUCCESS);
	ret = lte_lc_neighbor_cell_measurement(&params);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	strcpy(at_notif,
	       "%NCELLMEAS: 0,\"00112233\",\"98712\",\"0AB9\",4800,7,63,31,456,4800,"
	       "8,60,29,4,3500,9,invalid,18,5,5300\r\n");
	at_monitor_dispatch(at_notif);

	/* <n_rsrp> */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS=1", EXIT_SUCCESS);
	ret = lte_lc_neighbor_cell_measurement(&params);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	strcpy(at_notif,
	       "%NCELLMEAS: 0,\"00112233\",\"98712\",\"0AB9\",4800,7,63,31,456,4800,"
	       "8,60,29,4,3500,9,99,invalid,5,5300\r\n");
	at_monitor_dispatch(at_notif);

	/* <n_rsrq> */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS=1", EXIT_SUCCESS);
	ret = lte_lc_neighbor_cell_measurement(&params);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	strcpy(at_notif,
	       "%NCELLMEAS: 0,\"00112233\",\"98712\",\"0AB9\",4800,7,63,31,456,4800,"
	       "8,60,29,4,3500,9,99,18,invalid,5300\r\n");
	at_monitor_dispatch(at_notif);

	/* <time_diff> */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS", EXIT_SUCCESS);
	ret = lte_lc_neighbor_cell_measurement(NULL);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	strcpy(at_notif,
	       "%NCELLMEAS: 0,\"00112233\",\"98712\",\"0AB9\",4800,7,63,31,456,4800,"
	       "8,60,29,4,3500,9,99,18,5,invalid\r\n");
	at_monitor_dispatch(at_notif);
}

void test_lte_lc_neighbor_cell_measurement_gci_invalid_field_format_fail(void)
{
	int ret;
	struct lte_lc_ncellmeas_params params = {
		.search_type = LTE_LC_NEIGHBOR_SEARCH_TYPE_GCI_DEFAULT,
		.gci_count = 10,
	};

	lte_lc_callback_count_expected = 18;

	/* In case of an error, we're expected to receive an empty event. */
	for (int i = 0; i < lte_lc_callback_count_expected; i++) {
		test_event_data[i].type = LTE_LC_EVT_NEIGHBOR_CELL_MEAS;
		test_event_data[i].cells_info.current_cell.id = LTE_LC_CELL_EUTRAN_ID_INVALID;
		test_event_data[i].cells_info.ncells_count = 0;
		test_event_data[i].cells_info.gci_cells_count = 0;
	}

	/* Syntax for GCI search types:
	 * High level:
	 * status[,
	 *      GCI_cell_info1,neighbor_count1[,neighbor_cell1_1,neighbor_cell1_2...],
	 *      GCI_cell_info2,neighbor_count2[,neighbor_cell2_1,neighbor_cell2_2...]...]
	 *
	 * Detailed:
	 * %NCELLMEAS: status
	 * [,<cell_id>,<plmn>,<tac>,<ta>,<ta_meas_time>,<earfcn>,<phys_cell_id>,<rsrp>,<rsrq>,
	 *              <meas_time>,<serving>,<neighbor_count>
	 *      [,<n_earfcn1>,<n_phys_cell_id1>,<n_rsrp1>,<n_rsrq1>,<time_diff1>]
	 *      [,<n_earfcn2>,<n_phys_cell_id2>,<n_rsrp2>,<n_rsrq2>,<time_diff2>]...],
	 *  <cell_id>,<plmn>,<tac>,<ta>,<ta_meas_time>,<earfcn>,<phys_cell_id>,<rsrp>,<rsrq>,
	 *              <meas_time>,<serving>,<neighbor_count>
	 *      [,<n_earfcn1>,<n_phys_cell_id1>,<n_rsrp1>,<n_rsrq1>,<time_diff1>]
	 *      [,<n_earfcn2>,<n_phys_cell_id2>,<n_rsrp2>,<n_rsrq2>,<time_diff2>]...]...
	 */

	/* status */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS=3,10", EXIT_SUCCESS);
	ret = lte_lc_neighbor_cell_measurement(&params);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	strcpy(at_notif,
	       "%NCELLMEAS: invalid,"
	       "\"00112233\",\"11199\",\"1A2B\",64,20877,6200,110,53,22,189205,1,0,"
	       "\"0011AABB\",\"11297\",\"5E6F\",65534,5,2300,449,51,11,189245,0,0\r\n");
	at_monitor_dispatch(at_notif);

	/* <cell_id> */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS=3,10", EXIT_SUCCESS);
	ret = lte_lc_neighbor_cell_measurement(&params);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	strcpy(at_notif,
	       "%NCELLMEAS: 0,"
	       "\"00112233\",\"11199\",\"1A2B\",64,20877,6200,110,53,22,189205,1,0,"
	       "invalid,\"11297\",\"5E6F\",65534,5,2300,449,51,11,189245,0,0\r\n");
	at_monitor_dispatch(at_notif);

	/* <plmn> */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS=3,10", EXIT_SUCCESS);
	ret = lte_lc_neighbor_cell_measurement(&params);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	strcpy(at_notif,
	       "%NCELLMEAS: 0,"
	       "\"00112233\",\"11199\",\"1A2B\",64,20877,6200,110,53,22,189205,1,0,"
	       "\"0011AABB\",invalid,\"5E6F\",65534,5,2300,449,51,11,189245,0,0\r\n");
	at_monitor_dispatch(at_notif);

	/* <tac> */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS=3,10", EXIT_SUCCESS);
	ret = lte_lc_neighbor_cell_measurement(&params);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	strcpy(at_notif,
	       "%NCELLMEAS: 0,"
	       "\"00112233\",\"11199\",\"1A2B\",64,20877,6200,110,53,22,189205,1,0,"
	       "\"0011AABB\",\"11297\",invalid,65534,5,2300,449,51,11,189245,0,0\r\n");
	at_monitor_dispatch(at_notif);

	/* <ta> */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS=3,10", EXIT_SUCCESS);
	ret = lte_lc_neighbor_cell_measurement(&params);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	strcpy(at_notif,
	       "%NCELLMEAS: 0,"
	       "\"00112233\",\"11199\",\"1A2B\",64,20877,6200,110,53,22,189205,1,0,"
	       "\"0011AABB\",\"11297\",\"5E6F\",invalid,5,2300,449,51,11,189245,0,0\r\n");
	at_monitor_dispatch(at_notif);

	/* <ta_meas_time> */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS=3,10", EXIT_SUCCESS);
	ret = lte_lc_neighbor_cell_measurement(&params);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	strcpy(at_notif,
	       "%NCELLMEAS: 0,"
	       "\"00112233\",\"11199\",\"1A2B\",64,20877,6200,110,53,22,189205,1,0,"
	       "\"0011AABB\",\"11297\",\"5E6F\",65534,invalid,2300,449,51,11,189245,0,0\r\n");
	at_monitor_dispatch(at_notif);

	/* <earfcn> */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS=3,10", EXIT_SUCCESS);
	ret = lte_lc_neighbor_cell_measurement(&params);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	strcpy(at_notif,
	       "%NCELLMEAS: 0,"
	       "\"00112233\",\"11199\",\"1A2B\",64,20877,6200,110,53,22,189205,1,0,"
	       "\"0011AABB\",\"11297\",\"5E6F\",65534,5,invalid,449,51,11,189245,0,0\r\n");
	at_monitor_dispatch(at_notif);

	/* <phys_cell_id> */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS=3,10", EXIT_SUCCESS);
	ret = lte_lc_neighbor_cell_measurement(&params);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	strcpy(at_notif,
	       "%NCELLMEAS: 0,"
	       "\"00112233\",\"11199\",\"1A2B\",64,20877,6200,110,53,22,189205,1,0,"
	       "\"0011AABB\",\"11297\",\"5E6F\",65534,5,2300,invalid,51,11,189245,0,0\r\n");
	at_monitor_dispatch(at_notif);

	/* <rsrp> */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS=3,10", EXIT_SUCCESS);
	ret = lte_lc_neighbor_cell_measurement(&params);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	strcpy(at_notif,
	       "%NCELLMEAS: 0,"
	       "\"00112233\",\"11199\",\"1A2B\",64,20877,6200,110,53,22,189205,1,0,"
	       "\"0011AABB\",\"11297\",\"5E6F\",65534,5,2300,449,invalid,11,189245,0,0\r\n");
	at_monitor_dispatch(at_notif);

	/* <rsrq> */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS=3,10", EXIT_SUCCESS);
	ret = lte_lc_neighbor_cell_measurement(&params);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	strcpy(at_notif,
	       "%NCELLMEAS: 0,"
	       "\"00112233\",\"11199\",\"1A2B\",64,20877,6200,110,53,22,189205,1,0,"
	       "\"0011AABB\",\"11297\",\"5E6F\",65534,5,2300,449,51,invalid,189245,0,0\r\n");
	at_monitor_dispatch(at_notif);

	/* <meas_time> */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS=3,10", EXIT_SUCCESS);
	ret = lte_lc_neighbor_cell_measurement(&params);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	strcpy(at_notif,
	       "%NCELLMEAS: 0,"
	       "\"00112233\",\"11199\",\"1A2B\",64,20877,6200,110,53,22,189205,1,0,"
	       "\"0011AABB\",\"11297\",\"5E6F\",65534,5,2300,449,51,11,invalid,0,0\r\n");
	at_monitor_dispatch(at_notif);

	/* <serving> */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS=3,10", EXIT_SUCCESS);
	ret = lte_lc_neighbor_cell_measurement(&params);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	strcpy(at_notif,
	       "%NCELLMEAS: 0,"
	       "\"00112233\",\"11199\",\"1A2B\",64,20877,6200,110,53,22,189205,1,0,"
	       "\"0011AABB\",\"11297\",\"5E6F\",65534,5,2300,449,51,11,189245,invalid,0\r\n");
	at_monitor_dispatch(at_notif);

	/* <neighbor_count> */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS=3,10", EXIT_SUCCESS);
	ret = lte_lc_neighbor_cell_measurement(&params);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	strcpy(at_notif,
	       "%NCELLMEAS: 0,"
	       "\"00112233\",\"11199\",\"1A2B\",64,20877,6200,110,53,22,189205,1,0,"
	       "\"0011AABB\",\"11297\",\"5E6F\",65534,5,2300,449,51,11,189245,0,invalid\r\n");
	at_monitor_dispatch(at_notif);

	/* Serving cell normal neighbors */

	/* <n_earfcn> */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS=3,10", EXIT_SUCCESS);
	ret = lte_lc_neighbor_cell_measurement(&params);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	strcpy(at_notif,
	       "%NCELLMEAS: 0,"
	       "\"00112233\",\"11199\",\"1A2B\",64,20877,6200,110,53,22,189205,1,2,"
	       "invalid,100,101,102,0,333333,109,110,111,0,"
	       "\"0011AABB\",\"11297\",\"5E6F\",65534,5,2300,449,51,11,189245,0,0\r\n");
	at_monitor_dispatch(at_notif);

	/* <n_phys_cell_id> */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS=3,10", EXIT_SUCCESS);
	ret = lte_lc_neighbor_cell_measurement(&params);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	strcpy(at_notif,
	       "%NCELLMEAS: 0,"
	       "\"00112233\",\"11199\",\"1A2B\",64,20877,6200,110,53,22,189205,1,2,"
	       "333333,invalid,101,102,0,333333,109,110,111,0,"
	       "\"0011AABB\",\"11297\",\"5E6F\",65534,5,2300,449,51,11,189245,0,0\r\n");
	at_monitor_dispatch(at_notif);

	/* <n_rsrp> */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS=3,10", EXIT_SUCCESS);
	ret = lte_lc_neighbor_cell_measurement(&params);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	strcpy(at_notif,
	       "%NCELLMEAS: 0,"
	       "\"00112233\",\"11199\",\"1A2B\",64,20877,6200,110,53,22,189205,1,2,"
	       "333333,100,invalid,102,0,333333,109,110,111,0,"
	       "\"0011AABB\",\"11297\",\"5E6F\",65534,5,2300,449,51,11,189245,0,0\r\n");
	at_monitor_dispatch(at_notif);

	/* <n_rsrq> */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS=3,10", EXIT_SUCCESS);
	ret = lte_lc_neighbor_cell_measurement(&params);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	strcpy(at_notif,
	       "%NCELLMEAS: 0,"
	       "\"00112233\",\"11199\",\"1A2B\",64,20877,6200,110,53,22,189205,1,2,"
	       "333333,100,101,invalid,0,333333,109,110,111,0,"
	       "\"0011AABB\",\"11297\",\"5E6F\",65534,5,2300,449,51,11,189245,0,0\r\n");
	at_monitor_dispatch(at_notif);

	/* <time_diff> */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS=3,10", EXIT_SUCCESS);
	ret = lte_lc_neighbor_cell_measurement(&params);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	strcpy(at_notif,
	       "%NCELLMEAS: 0,"
	       "\"00112233\",\"11199\",\"1A2B\",64,20877,6200,110,53,22,189205,1,2,"
	       "333333,100,101,102,invalid,333333,109,110,111,0,"
	       "\"0011AABB\",\"11297\",\"5E6F\",65534,5,2300,449,51,11,189245,0,0\r\n");
	at_monitor_dispatch(at_notif);
}

void test_lte_lc_neighbor_cell_measurement_normal_invalid_mccmnc_fail(void)
{
	int ret;

	lte_lc_callback_count_expected = 2;

	/* In case of an error, we're expected to receive an empty event. */
	for (int i = 0; i < lte_lc_callback_count_expected; i++) {
		test_event_data[i].type = LTE_LC_EVT_NEIGHBOR_CELL_MEAS;
		test_event_data[i].cells_info.current_cell.id = LTE_LC_CELL_EUTRAN_ID_INVALID;
		test_event_data[i].cells_info.ncells_count = 0;
		test_event_data[i].cells_info.gci_cells_count = 0;
	}

	/* Invalid MCC */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS", EXIT_SUCCESS);
	ret = lte_lc_neighbor_cell_measurement(NULL);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	strcpy(at_notif,
	       "%NCELLMEAS: 0,\"00112233\",\"mcc12\",\"0AB9\",4800,7,63,31,456,4800,"
	       "8,60,29,4,3500,9,99,18,5,5300\r\n");
	at_monitor_dispatch(at_notif);

	/* Invalid MNC */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS", EXIT_SUCCESS);
	ret = lte_lc_neighbor_cell_measurement(NULL);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	strcpy(at_notif,
	       "%NCELLMEAS: 0,\"00112233\",\"987mnc\",\"0AB9\",4800,7,63,31,456,4800,"
	       "8,60,29,4,3500,9,99,18,5,5300\r\n");
	at_monitor_dispatch(at_notif);
}

void test_lte_lc_neighbor_cell_measurement_gci_invalid_mccmnc_fail(void)
{
	int ret;
	struct lte_lc_ncellmeas_params params = {
		.search_type = LTE_LC_NEIGHBOR_SEARCH_TYPE_GCI_EXTENDED_COMPLETE,
		.gci_count = 2,
	};

	lte_lc_callback_count_expected = 2;

	/* In case of an error, we're expected to receive an empty event. */
	for (int i = 0; i < lte_lc_callback_count_expected; i++) {
		test_event_data[i].type = LTE_LC_EVT_NEIGHBOR_CELL_MEAS;
		test_event_data[i].cells_info.current_cell.id = LTE_LC_CELL_EUTRAN_ID_INVALID;
		test_event_data[i].cells_info.ncells_count = 0;
		test_event_data[i].cells_info.gci_cells_count = 0;
	}

	/* Invalid MCC */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS=5,2", EXIT_SUCCESS);
	ret = lte_lc_neighbor_cell_measurement(&params);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	strcpy(at_notif,
	       "%NCELLMEAS: 0,"
	       "\"00112233\",\"11199\",\"1A2B\",64,20877,6200,110,53,22,189205,1,0,"
	       "\"0011AABB\",\"mcc97\",\"5E6F\",65534,5,2300,449,51,11,189245,0,0\r\n");
	at_monitor_dispatch(at_notif);

	/* Invalid MNC */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS=5,2", EXIT_SUCCESS);
	ret = lte_lc_neighbor_cell_measurement(&params);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	strcpy(at_notif,
	       "%NCELLMEAS: 0,"
	       "\"00112233\",\"11199\",\"1A2B\",64,20877,6200,110,53,22,189205,1,0,"
	       "\"0011AABB\",\"112mnc\",\"5E6F\",65534,5,2300,449,51,11,189245,0,0\r\n");
	at_monitor_dispatch(at_notif);
}

void test_lte_lc_neighbor_cell_measurement_invalid_gci_cell_count_fail(void)
{
	int ret;
	struct lte_lc_ncellmeas_params params;

	params.search_type = LTE_LC_NEIGHBOR_SEARCH_TYPE_GCI_DEFAULT;
	params.gci_count = 0;
	ret = lte_lc_neighbor_cell_measurement(&params);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	params.search_type = LTE_LC_NEIGHBOR_SEARCH_TYPE_GCI_EXTENDED_LIGHT;
	params.gci_count = 1;
	ret = lte_lc_neighbor_cell_measurement(&params);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	params.search_type = LTE_LC_NEIGHBOR_SEARCH_TYPE_GCI_EXTENDED_COMPLETE;
	params.gci_count = 16;
	ret = lte_lc_neighbor_cell_measurement(&params);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

void test_lte_lc_neighbor_cell_measurement_no_event_handler(void)
{
	int ret;

	ret = lte_lc_deregister_handler(lte_lc_event_handler);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	strcpy(at_notif,
	       "%NCELLMEAS: 0,"
	       "\"00112233\",\"11199\",\"1A2B\",64,20877,6200,110,53,22,189205,1,0,"
	       "\"0011AABB\",\"112mnc\",\"5E6F\",65534,5,2300,449,51,11,189245,0,0\r\n");
	at_monitor_dispatch(at_notif);

	/* Register handler so that tearDown() doesn't cause unnecessary warning log */
	lte_lc_register_handler(lte_lc_event_handler);
}

void test_lte_lc_modem_sleep_event(void)
{
	lte_lc_callback_count_expected = 6;
	int index = 0;

	strcpy(at_notif, "%XMODEMSLEEP: 1,12345\r\n");
	test_event_data[index].type = LTE_LC_EVT_MODEM_SLEEP_ENTER;
	test_event_data[index].modem_sleep.type = LTE_LC_MODEM_SLEEP_PSM;
	test_event_data[index].modem_sleep.time = 12345;
	at_monitor_dispatch(at_notif);
	index++;

	strcpy(at_notif, "%XMODEMSLEEP: 2,5000\r\n");
	test_event_data[index].type = LTE_LC_EVT_MODEM_SLEEP_EXIT_PRE_WARNING;
	test_event_data[index].modem_sleep.type = LTE_LC_MODEM_SLEEP_RF_INACTIVITY;
	test_event_data[index].modem_sleep.time = 5000;
	at_monitor_dispatch(at_notif);
	index++;

	strcpy(at_notif, "%XMODEMSLEEP: 3,0\r\n");
	test_event_data[index].type = LTE_LC_EVT_MODEM_SLEEP_EXIT;
	test_event_data[index].modem_sleep.type = LTE_LC_MODEM_SLEEP_LIMITED_SERVICE;
	test_event_data[index].modem_sleep.time = 0;
	at_monitor_dispatch(at_notif);
	index++;

	strcpy(at_notif, "%XMODEMSLEEP: 7,35712000000\r\n");
	test_event_data[index].type = LTE_LC_EVT_MODEM_SLEEP_ENTER;
	test_event_data[index].modem_sleep.type = LTE_LC_MODEM_SLEEP_PROPRIETARY_PSM;
	test_event_data[index].modem_sleep.time = 35712000000;
	at_monitor_dispatch(at_notif);
	index++;

	strcpy(at_notif, "%XMODEMSLEEP: 4,0\r\n");
	test_event_data[index].type = LTE_LC_EVT_MODEM_SLEEP_EXIT;
	test_event_data[index].modem_sleep.type = LTE_LC_MODEM_SLEEP_FLIGHT_MODE;
	test_event_data[index].modem_sleep.time = 0;
	at_monitor_dispatch(at_notif);
	index++;

	strcpy(at_notif, "%XMODEMSLEEP: 4\r\n");
	test_event_data[index].type = LTE_LC_EVT_MODEM_SLEEP_ENTER;
	test_event_data[index].modem_sleep.type = LTE_LC_MODEM_SLEEP_FLIGHT_MODE;
	test_event_data[index].modem_sleep.time = -1;
	at_monitor_dispatch(at_notif);
}

void test_lte_lc_modem_sleep_event_ignore(void)
{
	/* 5: Sleep resumed not supported by lte_lc */
	strcpy(at_notif, "%%XMODEMSLEEP: 5\r\n");
	at_monitor_dispatch(at_notif);

	/* 6: Sleep resumed not supported by lte_lc */
	strcpy(at_notif, "%%XMODEMSLEEP: 6\r\n");
	at_monitor_dispatch(at_notif);

	/* No event for incorrect number format for time */
	strcpy(at_notif, "%XMODEMSLEEP: 4,\"string\"\r\n");
	at_monitor_dispatch(at_notif);

	/* No event for unknown type */
	strcpy(at_notif, "%XMODEMSLEEP: 99,0\r\n");
	at_monitor_dispatch(at_notif);

	/* No event for malformed notification */
	strcpy(at_notif, "%XMODEMSLEEP: 99,1 2\r\n");
	at_monitor_dispatch(at_notif);
}

void test_lte_lc_modem_events(void)
{
	int ret;
	int index = 0;

	lte_lc_callback_count_expected = 11;

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%MDMEV=2", EXIT_SUCCESS);
	ret = lte_lc_modem_events_enable();
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%MDMEV=2", EXIT_FAILURE);
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%MDMEV=1", EXIT_SUCCESS);
	ret = lte_lc_modem_events_enable();
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	strcpy(at_notif, "%MDMEV: ME OVERHEATED\r\n");
	test_event_data[index].type = LTE_LC_EVT_MODEM_EVENT;
	test_event_data[index].modem_evt.type = LTE_LC_MODEM_EVT_OVERHEATED;
	at_monitor_dispatch(at_notif);
	index++;

	strcpy(at_notif, "%MDMEV: NO IMEI\r\n");
	test_event_data[index].type = LTE_LC_EVT_MODEM_EVENT;
	test_event_data[index].modem_evt.type = LTE_LC_MODEM_EVT_NO_IMEI;
	at_monitor_dispatch(at_notif);
	index++;

	strcpy(at_notif, "%MDMEV: ME BATTERY LOW\r\n");
	test_event_data[index].type = LTE_LC_EVT_MODEM_EVENT;
	test_event_data[index].modem_evt.type = LTE_LC_MODEM_EVT_BATTERY_LOW;
	at_monitor_dispatch(at_notif);
	index++;

	strcpy(at_notif, "%MDMEV: RESET LOOP\r\n");
	test_event_data[index].type = LTE_LC_EVT_MODEM_EVENT;
	test_event_data[index].modem_evt.type = LTE_LC_MODEM_EVT_RESET_LOOP;
	at_monitor_dispatch(at_notif);
	index++;

	strcpy(at_notif, "%MDMEV: SEARCH STATUS 1\r\n");
	test_event_data[index].type = LTE_LC_EVT_MODEM_EVENT;
	test_event_data[index].modem_evt.type = LTE_LC_MODEM_EVT_LIGHT_SEARCH_DONE;
	at_monitor_dispatch(at_notif);
	index++;

	strcpy(at_notif, "%MDMEV: SEARCH STATUS 2\r\n");
	test_event_data[index].type = LTE_LC_EVT_MODEM_EVENT;
	test_event_data[index].modem_evt.type = LTE_LC_MODEM_EVT_SEARCH_DONE;
	at_monitor_dispatch(at_notif);
	index++;

	strcpy(at_notif, "%MDMEV: PRACH CE-LEVEL 2\r\n");
	test_event_data[index].type = LTE_LC_EVT_MODEM_EVENT;
	test_event_data[index].modem_evt.type = LTE_LC_MODEM_EVT_CE_LEVEL;
	test_event_data[index].modem_evt.ce_level = LTE_LC_CE_LEVEL_2;
	at_monitor_dispatch(at_notif);
	index++;

	strcpy(at_notif, "%MDMEV: RF CALIBRATION NOT DONE\r\n");
	test_event_data[index].type = LTE_LC_EVT_MODEM_EVENT;
	test_event_data[index].modem_evt.type = LTE_LC_MODEM_EVT_RF_CAL_NOT_DONE;
	at_monitor_dispatch(at_notif);
	index++;

	strcpy(at_notif, "%MDMEV: INVALID BAND CONFIGURATION 1 2\r\n");
	test_event_data[index].type = LTE_LC_EVT_MODEM_EVENT;
	test_event_data[index].modem_evt.type = LTE_LC_MODEM_EVT_INVALID_BAND_CONF;
	test_event_data[index].modem_evt.invalid_band_conf.status_ltem =
		LTE_LC_BAND_CONF_STATUS_INVALID;
	test_event_data[index].modem_evt.invalid_band_conf.status_nbiot =
		LTE_LC_BAND_CONF_STATUS_SYSTEM_NOT_SUPPORTED;
	test_event_data[index].modem_evt.invalid_band_conf.status_ntn_nbiot =
		LTE_LC_BAND_CONF_STATUS_SYSTEM_NOT_SUPPORTED;
	at_monitor_dispatch(at_notif);
	index++;

	strcpy(at_notif, "%MDMEV: INVALID BAND CONFIGURATION 0 1 0\r\n");
	test_event_data[index].type = LTE_LC_EVT_MODEM_EVENT;
	test_event_data[index].modem_evt.type = LTE_LC_MODEM_EVT_INVALID_BAND_CONF;
	test_event_data[index].modem_evt.invalid_band_conf.status_ltem =
		LTE_LC_BAND_CONF_STATUS_OK;
	test_event_data[index].modem_evt.invalid_band_conf.status_nbiot =
		LTE_LC_BAND_CONF_STATUS_INVALID;
	test_event_data[index].modem_evt.invalid_band_conf.status_ntn_nbiot =
		LTE_LC_BAND_CONF_STATUS_OK;
	at_monitor_dispatch(at_notif);
	index++;

	strcpy(at_notif, "%MDMEV: DETECTED COUNTRY 123\r\n");
	test_event_data[index].type = LTE_LC_EVT_MODEM_EVENT;
	test_event_data[index].modem_evt.type = LTE_LC_MODEM_EVT_DETECTED_COUNTRY;
	test_event_data[index].modem_evt.detected_country = 123;
	at_monitor_dispatch(at_notif);
	index++;

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%MDMEV=0", EXIT_SUCCESS);
	ret = lte_lc_modem_events_disable();
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_modem_events_fail(void)
{
	int ret;

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%MDMEV=2", EXIT_FAILURE);
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%MDMEV=1", -NRF_ENOMEM);
	ret = lte_lc_modem_events_enable();
	TEST_ASSERT_EQUAL(-EFAULT, ret);

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%MDMEV=0", -NRF_ENOMEM);
	ret = lte_lc_modem_events_disable();
	TEST_ASSERT_EQUAL(-EFAULT, ret);

	/* No lte_lc event generated for these notifications */

	strcpy(at_notif, "%MDMEV: SEARCH STATUS 3\r\n");
	at_monitor_dispatch(at_notif);

	strcpy(at_notif, "%MDMEV: SEARCH STATUS 1 and then some\r\n");
	at_monitor_dispatch(at_notif);
}

void test_lte_lc_periodic_search_request(void)
{
	int ret;

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%PERIODICSEARCHCONF=3", EXIT_SUCCESS);
	ret = lte_lc_periodic_search_request();
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%PERIODICSEARCHCONF=3", -NRF_ENOMEM);
	ret = lte_lc_periodic_search_request();
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_conn_eval_params_get_null_fail(void)
{
	int ret;

	ret = lte_lc_conn_eval_params_get(NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

void test_lte_lc_conn_eval_params_get_cfun_at_scanf_fail(void)
{
	int ret;
	struct lte_lc_conn_eval_params params;

	__mock_nrf_modem_at_scanf_ExpectAndReturn("AT+CFUN?", "+CFUN: %hu", -NRF_ENOMEM);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(LTE_LC_FUNC_MODE_NORMAL);

	ret = lte_lc_conn_eval_params_get(&params);
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_conn_eval_params_get_func_mode_wrong_fail(void)
{
	int ret;
	struct lte_lc_conn_eval_params params;

	__mock_nrf_modem_at_scanf_ExpectAndReturn("AT+CFUN?", "+CFUN: %hu", 1);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(LTE_LC_FUNC_MODE_OFFLINE);

	ret = lte_lc_conn_eval_params_get(&params);
	TEST_ASSERT_EQUAL(-EOPNOTSUPP, ret);
}

void test_lte_lc_conn_eval_params_get_at_scanf_fail(void)
{
	int ret;
	struct lte_lc_conn_eval_params params;

	__mock_nrf_modem_at_scanf_ExpectAndReturn("AT+CFUN?", "+CFUN: %hu", 1);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(LTE_LC_FUNC_MODE_NORMAL);

	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT%CONEVAL",
		"%%CONEVAL: %d,%hu,%hu,%hd,%hd,%hd,\"%x\",\"%6[^\"]\",%hd,%d,%hd,%hu,%hu,%hd,%hd,%hd,%hd",
		-NRF_ENOMEM);
	__mock_nrf_modem_at_scanf_ReturnVarg_int(0); /* result */

	ret = lte_lc_conn_eval_params_get(&params);
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_conn_eval_params_get_result_fail(void)
{
	int ret;
	struct lte_lc_conn_eval_params params;

	__mock_nrf_modem_at_scanf_ExpectAndReturn("AT+CFUN?", "+CFUN: %hu", 1);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(LTE_LC_FUNC_MODE_NORMAL);

	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT%CONEVAL",
		"%%CONEVAL: %d,%hu,%hu,%hd,%hd,%hd,\"%x\",\"%6[^\"]\",%hd,%d,%hd,%hu,%hu,%hd,%hd,%hd,%hd",
		1);
	__mock_nrf_modem_at_scanf_ReturnVarg_int(1); /* result */

	ret = lte_lc_conn_eval_params_get(&params);
	TEST_ASSERT_EQUAL(1, ret);
}

void test_lte_lc_conn_eval_params_get_param_count_not_17_fail(void)
{
	int ret;
	struct lte_lc_conn_eval_params params;

	__mock_nrf_modem_at_scanf_ExpectAndReturn("AT+CFUN?", "+CFUN: %hu", 1);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(LTE_LC_FUNC_MODE_NORMAL);

	/* Returns 10 instead of 17 parameters */
	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT%CONEVAL",
		"%%CONEVAL: %d,%hu,%hu,%hd,%hd,%hd,\"%x\",\"%6[^\"]\",%hd,%d,%hd,%hu,%hu,%hd,%hd,%hd,%hd",
		10);
	__mock_nrf_modem_at_scanf_ReturnVarg_int(0); /* result */

	ret = lte_lc_conn_eval_params_get(&params);
	TEST_ASSERT_EQUAL(-EBADMSG, ret);
}

void test_lte_lc_conn_eval_params_get_invalid_mcc_fail(void)
{
	int ret;
	struct lte_lc_conn_eval_params params;

	__mock_nrf_modem_at_scanf_ExpectAndReturn("AT+CFUN?", "+CFUN: %hu", 1);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(LTE_LC_FUNC_MODE_NORMAL);

	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT%CONEVAL",
		"%%CONEVAL: %d,%hu,%hu,%hd,%hd,%hd,\"%x\",\"%6[^\"]\",%hd,%d,%hd,%hu,%hu,%hd,%hd,%hd,%hd",
		17);
	__mock_nrf_modem_at_scanf_ReturnVarg_int(0); /* result */
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(1); /* rrc_state */
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(8); /* energy_estimate */
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(8); /* rsrp */
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(8); /* rsrq */
	__mock_nrf_modem_at_scanf_ReturnVarg_int16(8); /* snr */
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(8); /* cell_id */
	__mock_nrf_modem_at_scanf_ReturnVarg_string("mcc92"); /* plmn_str */
	__mock_nrf_modem_at_scanf_ReturnVarg_int16(8); /* phy_cid */
	__mock_nrf_modem_at_scanf_ReturnVarg_int(8); /* earfcn */
	__mock_nrf_modem_at_scanf_ReturnVarg_int16(8); /* band */
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(0); /* tau_trig */
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(0); /* ce_level */
	__mock_nrf_modem_at_scanf_ReturnVarg_int16(8); /* tx_power */
	__mock_nrf_modem_at_scanf_ReturnVarg_int16(8); /* tx_rep */
	__mock_nrf_modem_at_scanf_ReturnVarg_int16(8); /* rx_rep */
	__mock_nrf_modem_at_scanf_ReturnVarg_int16(8); /* dl_pathloss */

	ret = lte_lc_conn_eval_params_get(&params);
	TEST_ASSERT_EQUAL(-EBADMSG, ret);
}

void test_lte_lc_conn_eval_params_get_invalid_mnc_fail(void)
{
	int ret;
	struct lte_lc_conn_eval_params params;

	__mock_nrf_modem_at_scanf_ExpectAndReturn("AT+CFUN?", "+CFUN: %hu", 1);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(LTE_LC_FUNC_MODE_NORMAL);

	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT%CONEVAL",
		"%%CONEVAL: %d,%hu,%hu,%hd,%hd,%hd,\"%x\",\"%6[^\"]\",%hd,%d,%hd,%hu,%hu,%hd,%hd,%hd,%hd",
		17);
	__mock_nrf_modem_at_scanf_ReturnVarg_int(0); /* result */
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(1); /* rrc_state */
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(8); /* energy_estimate */
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(8); /* rsrp */
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(8); /* rsrq */
	__mock_nrf_modem_at_scanf_ReturnVarg_int16(8); /* snr */
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(8); /* cell_id */
	__mock_nrf_modem_at_scanf_ReturnVarg_string("24292m"); /* plmn_str */
	__mock_nrf_modem_at_scanf_ReturnVarg_int16(8); /* phy_cid */
	__mock_nrf_modem_at_scanf_ReturnVarg_int(8); /* earfcn */
	__mock_nrf_modem_at_scanf_ReturnVarg_int16(8); /* band */
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(0); /* tau_trig */
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(0); /* ce_level */
	__mock_nrf_modem_at_scanf_ReturnVarg_int16(8); /* tx_power */
	__mock_nrf_modem_at_scanf_ReturnVarg_int16(8); /* tx_rep */
	__mock_nrf_modem_at_scanf_ReturnVarg_int16(8); /* rx_rep */
	__mock_nrf_modem_at_scanf_ReturnVarg_int16(8); /* dl_pathloss */

	ret = lte_lc_conn_eval_params_get(&params);
	TEST_ASSERT_EQUAL(-EBADMSG, ret);
}

void test_lte_lc_conn_eval_params_get_coneval_success(void)
{
	int ret;
	struct lte_lc_conn_eval_params params;

	__mock_nrf_modem_at_scanf_ExpectAndReturn("AT+CFUN?", "+CFUN: %hu", 1);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(LTE_LC_FUNC_MODE_NORMAL);

	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT%CONEVAL",
		"%%CONEVAL: %d,%hu,%hu,%hd,%hd,%hd,\"%x\",\"%6[^\"]\",%hd,%d,%hd,%hu,%hu,%hd,%hd,%hd,%hd",
		17);
	__mock_nrf_modem_at_scanf_ReturnVarg_int(0); /* result */
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(1); /* rrc_state */
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(8); /* energy_estimate */
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(8); /* rsrp */
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(8); /* rsrq */
	__mock_nrf_modem_at_scanf_ReturnVarg_int16(8); /* snr */
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(8); /* cell_id */
	__mock_nrf_modem_at_scanf_ReturnVarg_string("26295"); /* plmn_str */
	__mock_nrf_modem_at_scanf_ReturnVarg_int16(8); /* phy_cid */
	__mock_nrf_modem_at_scanf_ReturnVarg_int(8); /* earfcn */
	__mock_nrf_modem_at_scanf_ReturnVarg_int16(8); /* band */
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(0); /* tau_trig */
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(0); /* ce_level */
	__mock_nrf_modem_at_scanf_ReturnVarg_int16(8); /* tx_power */
	__mock_nrf_modem_at_scanf_ReturnVarg_int16(8); /* tx_rep */
	__mock_nrf_modem_at_scanf_ReturnVarg_int16(8); /* rx_rep */
	__mock_nrf_modem_at_scanf_ReturnVarg_int16(8); /* dl_pathloss */

	ret = lte_lc_conn_eval_params_get(&params);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_periodic_search_set_null_fail(void)
{
	int ret;

	ret = lte_lc_periodic_search_set(NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

void test_lte_lc_periodic_search_set_patterns0_fail(void)
{
	int ret;
	struct lte_lc_periodic_search_cfg cfg = { 0 };

	cfg.pattern_count = 0;

	ret = lte_lc_periodic_search_set(&cfg);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

void test_lte_lc_periodic_search_set_patterns5_fail(void)
{
	int ret;
	struct lte_lc_periodic_search_cfg cfg = { 0 };

	cfg.pattern_count = 5;

	ret = lte_lc_periodic_search_set(&cfg);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

void test_lte_lc_periodic_search_set_patterns1_success(void)
{
	int ret;
	struct lte_lc_periodic_search_cfg cfg = { 0 };

	cfg.pattern_count = 1;
	cfg.loop = 0;
	cfg.return_to_pattern = 1;
	cfg.band_optimization = 1;
	cfg.patterns[0].type = 0;
	cfg.patterns[0].range.initial_sleep = 60;
	cfg.patterns[0].range.final_sleep = 3600;
	cfg.patterns[0].range.time_to_final_sleep = 300;
	cfg.patterns[0].range.pattern_end_point = 600;

	__mock_nrf_modem_at_printf_ExpectAndReturn(
		"AT%PERIODICSEARCHCONF=0,0,1,1,\"0,60,3600,300,600\"", EXIT_SUCCESS);

	ret = lte_lc_periodic_search_set(&cfg);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_periodic_search_set_patterns4_success(void)
{
	int ret;
	struct lte_lc_periodic_search_cfg cfg = { 0 };

	cfg.pattern_count = 4;
	cfg.loop = 1;
	cfg.return_to_pattern = 3;
	cfg.band_optimization = 5;
	cfg.patterns[0].type = LTE_LC_PERIODIC_SEARCH_PATTERN_RANGE;
	cfg.patterns[0].range.initial_sleep = 60;
	cfg.patterns[0].range.final_sleep = 3600;
	cfg.patterns[0].range.time_to_final_sleep = 300;
	cfg.patterns[0].range.pattern_end_point = 600;
	cfg.patterns[1].type = LTE_LC_PERIODIC_SEARCH_PATTERN_RANGE;
	cfg.patterns[1].range.initial_sleep = 120;
	cfg.patterns[1].range.final_sleep = 1800;
	cfg.patterns[1].range.time_to_final_sleep = -1;
	cfg.patterns[1].range.pattern_end_point = 48;
	cfg.patterns[2].type = LTE_LC_PERIODIC_SEARCH_PATTERN_TABLE;
	cfg.patterns[2].table.val_1 = 10;
	cfg.patterns[2].table.val_2 = 20;
	cfg.patterns[2].table.val_3 = 30;
	cfg.patterns[2].table.val_4 = 40;
	cfg.patterns[2].table.val_5 = 50;
	cfg.patterns[3].type = LTE_LC_PERIODIC_SEARCH_PATTERN_TABLE;
	cfg.patterns[3].table.val_1 = 11;
	cfg.patterns[3].table.val_2 = 21;
	cfg.patterns[3].table.val_3 = 31;
	cfg.patterns[3].table.val_4 = -1;
	cfg.patterns[3].table.val_5 = -1;

	__mock_nrf_modem_at_printf_ExpectAndReturn(
		"AT%PERIODICSEARCHCONF=0,1,3,5,\"0,60,3600,300,600\",\"0,120,1800,,48\","
		"\"1,10,20,30,40,50\",\"1,11,21,31\"",
		EXIT_SUCCESS);

	ret = lte_lc_periodic_search_set(&cfg);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_periodic_search_set_pattern_table_values_success(void)
{
	int ret;
	struct lte_lc_periodic_search_cfg cfg = { 0 };

	cfg.pattern_count = 4;
	cfg.loop = 1;
	cfg.return_to_pattern = 3;
	cfg.band_optimization = 5;
	cfg.patterns[0].type = LTE_LC_PERIODIC_SEARCH_PATTERN_TABLE;
	cfg.patterns[0].table.val_1 = 11;
	cfg.patterns[0].table.val_2 = -1;
	cfg.patterns[0].table.val_3 = -1;
	cfg.patterns[0].table.val_4 = -1;
	cfg.patterns[0].table.val_5 = -1;
	cfg.patterns[1].type = LTE_LC_PERIODIC_SEARCH_PATTERN_TABLE;
	cfg.patterns[1].table.val_1 = 21;
	cfg.patterns[1].table.val_2 = 22;
	cfg.patterns[1].table.val_3 = -1;
	cfg.patterns[1].table.val_4 = -1;
	cfg.patterns[1].table.val_5 = -1;
	cfg.patterns[2].type = LTE_LC_PERIODIC_SEARCH_PATTERN_TABLE;
	cfg.patterns[2].table.val_1 = 31;
	cfg.patterns[2].table.val_2 = 32;
	cfg.patterns[2].table.val_3 = 33;
	cfg.patterns[2].table.val_4 = -1;
	cfg.patterns[2].table.val_5 = -1;
	cfg.patterns[3].type = LTE_LC_PERIODIC_SEARCH_PATTERN_TABLE;
	cfg.patterns[3].table.val_1 = 41;
	cfg.patterns[3].table.val_2 = 42;
	cfg.patterns[3].table.val_3 = 43;
	cfg.patterns[3].table.val_4 = 44;
	cfg.patterns[3].table.val_5 = -1;

	__mock_nrf_modem_at_printf_ExpectAndReturn(
		"AT%PERIODICSEARCHCONF=0,1,3,5,\"1,11\",\"1,21,22\","
		"\"1,31,32,33\",\"1,41,42,43,44\"",
		EXIT_SUCCESS);

	ret = lte_lc_periodic_search_set(&cfg);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_periodic_search_set_max_values_fail(void)
{
	int ret;
	struct lte_lc_periodic_search_cfg cfg = { 0 };

	cfg.pattern_count = 1;
	cfg.loop = 0;
	cfg.return_to_pattern = 1;
	cfg.band_optimization = 1;
	cfg.patterns[0].type = 2; /* Unknown type */
	cfg.patterns[0].table.val_1 = 10;
	cfg.patterns[0].table.val_2 = 20;
	cfg.patterns[0].table.val_3 = 30;
	cfg.patterns[0].table.val_4 = 40;
	cfg.patterns[0].table.val_5 = 50;

	__mock_nrf_modem_at_printf_ExpectAndReturn(
		"AT%PERIODICSEARCHCONF=0,0,1,1,", 65536);

	ret = lte_lc_periodic_search_set(&cfg);
	TEST_ASSERT_EQUAL(-EBADMSG, ret);
}

void test_lte_lc_periodic_search_set_unknown_search_pattern_type_fail(void)
{
	int ret;
	struct lte_lc_periodic_search_cfg cfg = { 0 };

	cfg.pattern_count = 1;
	cfg.loop = 0;
	cfg.return_to_pattern = 1;
	cfg.band_optimization = 1;
	cfg.patterns[0].type = 1;
	cfg.patterns[0].table.val_1 = INT_MAX;
	cfg.patterns[0].table.val_2 = INT_MAX;
	cfg.patterns[0].table.val_3 = INT_MAX;
	cfg.patterns[0].table.val_4 = INT_MAX;
	cfg.patterns[0].table.val_5 = INT_MAX;

	__mock_nrf_modem_at_printf_ExpectAndReturn(
		"AT%PERIODICSEARCHCONF=0,0,1,1,", 65536);

	ret = lte_lc_periodic_search_set(&cfg);
	TEST_ASSERT_EQUAL(-EBADMSG, ret);
}

void test_lte_lc_periodic_search_set_at_fail(void)
{
	int ret;
	struct lte_lc_periodic_search_cfg cfg = { 0 };

	cfg.pattern_count = 1;

	__mock_nrf_modem_at_printf_ExpectAndReturn(
		"AT%PERIODICSEARCHCONF=0,0,0,0,\"0,0,0,0,0\"", -NRF_ENOMEM);

	ret = lte_lc_periodic_search_set(&cfg);
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_periodic_search_set_at_cme_fail(void)
{
	int ret;
	struct lte_lc_periodic_search_cfg cfg = { 0 };

	cfg.pattern_count = 1;

	__mock_nrf_modem_at_printf_ExpectAndReturn(
		"AT%PERIODICSEARCHCONF=0,0,0,0,\"0,0,0,0,0\"", 1);

	ret = lte_lc_periodic_search_set(&cfg);
	TEST_ASSERT_EQUAL(-EBADMSG, ret);
}

void test_lte_lc_periodic_search_clear_success(void)
{
	int ret;

	__mock_nrf_modem_at_printf_ExpectAndReturn(
		"AT%PERIODICSEARCHCONF=2", EXIT_SUCCESS);

	ret = lte_lc_periodic_search_clear();
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_periodic_search_clear_at_fail(void)
{
	int ret;

	__mock_nrf_modem_at_printf_ExpectAndReturn(
		"AT%PERIODICSEARCHCONF=2", -NRF_ENOMEM);

	ret = lte_lc_periodic_search_clear();
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_periodic_search_clear_cme(void)
{
	int ret;

	__mock_nrf_modem_at_printf_ExpectAndReturn(
		"AT%PERIODICSEARCHCONF=2", 1);

	ret = lte_lc_periodic_search_clear();
	TEST_ASSERT_EQUAL(-EBADMSG, ret);
}

void test_lte_lc_periodic_search_get_null(void)
{
	int ret;

	ret = lte_lc_periodic_search_get(NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

void test_lte_lc_periodic_search_get_at_fail(void)
{
	int ret;
	struct lte_lc_periodic_search_cfg cfg;

	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT%PERIODICSEARCHCONF=1",
		"%%PERIODICSEARCHCONF: %hu,%hu,%hu,\"%40[^\"]\",\"%40[^\"]\",\"%40[^\"]\",\"%40[^\"]\"",
		-NRF_ENOMEM);

	ret = lte_lc_periodic_search_get(&cfg);
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_periodic_search_get_at_badmsg_fail(void)
{
	int ret;
	struct lte_lc_periodic_search_cfg cfg;

	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT%PERIODICSEARCHCONF=1",
		"%%PERIODICSEARCHCONF: %hu,%hu,%hu,\"%40[^\"]\",\"%40[^\"]\",\"%40[^\"]\",\"%40[^\"]\"",
		-NRF_EBADMSG);

	ret = lte_lc_periodic_search_get(&cfg);
	TEST_ASSERT_EQUAL(-ENOENT, ret);
}

void test_lte_lc_periodic_search_get_no_pattern(void)
{
	int ret;
	struct lte_lc_periodic_search_cfg cfg;

	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT%PERIODICSEARCHCONF=1",
		"%%PERIODICSEARCHCONF: %hu,%hu,%hu,\"%40[^\"]\",\"%40[^\"]\",\"%40[^\"]\",\"%40[^\"]\"",
		0);

	ret = lte_lc_periodic_search_get(&cfg);
	TEST_ASSERT_EQUAL(-EBADMSG, ret);
}

void test_lte_lc_periodic_search_get_pattern_invalid_type_fail(void)
{
	int ret;
	struct lte_lc_periodic_search_cfg cfg;

	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT%PERIODICSEARCHCONF=1",
		"%%PERIODICSEARCHCONF: %hu,%hu,%hu,\"%40[^\"]\",\"%40[^\"]\",\"%40[^\"]\",\"%40[^\"]\"",
		4);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(0); /* loop */
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(0); /* return_to_pattern */
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(10); /* band_optimization */
	__mock_nrf_modem_at_scanf_ReturnVarg_string("2,60,3600,,600"); /* pattern_buf */

	ret = lte_lc_periodic_search_get(&cfg);
	TEST_ASSERT_EQUAL(-EBADMSG, ret);
}

void test_lte_lc_periodic_search_get_pattern1_invalid_syntax_fail(void)
{
	int ret;
	struct lte_lc_periodic_search_cfg cfg;

	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT%PERIODICSEARCHCONF=1",
		"%%PERIODICSEARCHCONF: %hu,%hu,%hu,\"%40[^\"]\",\"%40[^\"]\",\"%40[^\"]\",\"%40[^\"]\"",
		4);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(0); /* loop */
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(0); /* return_to_pattern */
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(10); /* band_optimization */
	__mock_nrf_modem_at_scanf_ReturnVarg_string("invalid,invalid2"); /* pattern_buf */

	ret = lte_lc_periodic_search_get(&cfg);
	TEST_ASSERT_EQUAL(-EBADMSG, ret);
}

void test_lte_lc_periodic_search_get_pattern2_range_invalid_param3_syntax_fail(void)
{
	int ret;
	struct lte_lc_periodic_search_cfg cfg;

	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT%PERIODICSEARCHCONF=1",
		"%%PERIODICSEARCHCONF: %hu,%hu,%hu,\"%40[^\"]\",\"%40[^\"]\",\"%40[^\"]\",\"%40[^\"]\"",
		5);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(0); /* loop */
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(0); /* return_to_pattern */
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(10); /* band_optimization */
	__mock_nrf_modem_at_scanf_ReturnVarg_string("0,60,3600,30,600"); /* pattern_buf */
	__mock_nrf_modem_at_scanf_ReturnVarg_string("0,60,3600,invalid,600"); /* pattern_buf */

	ret = lte_lc_periodic_search_get(&cfg);
	TEST_ASSERT_EQUAL(-EBADMSG, ret);
}

void test_lte_lc_periodic_search_get_pattern3_range_too_few_params_fail(void)
{
	int ret;
	struct lte_lc_periodic_search_cfg cfg;

	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT%PERIODICSEARCHCONF=1",
		"%%PERIODICSEARCHCONF: %hu,%hu,%hu,\"%40[^\"]\",\"%40[^\"]\",\"%40[^\"]\",\"%40[^\"]\"",
		6);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(0); /* loop */
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(0); /* return_to_pattern */
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(10); /* band_optimization */
	__mock_nrf_modem_at_scanf_ReturnVarg_string("0,60,3600,30,600"); /* pattern_buf */
	__mock_nrf_modem_at_scanf_ReturnVarg_string("0,60,3600,30,600"); /* pattern_buf */
	__mock_nrf_modem_at_scanf_ReturnVarg_string("0,60,3600"); /* pattern_buf */

	ret = lte_lc_periodic_search_get(&cfg);
	TEST_ASSERT_EQUAL(-EBADMSG, ret);
}

void test_lte_lc_periodic_search_get_pattern4_range_empty_fail(void)
{
	int ret;
	struct lte_lc_periodic_search_cfg cfg;

	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT%PERIODICSEARCHCONF=1",
		"%%PERIODICSEARCHCONF: %hu,%hu,%hu,\"%40[^\"]\",\"%40[^\"]\",\"%40[^\"]\",\"%40[^\"]\"",
		7);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(0); /* loop */
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(0); /* return_to_pattern */
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(10); /* band_optimization */
	__mock_nrf_modem_at_scanf_ReturnVarg_string("0,60,3600,30,600"); /* pattern_buf */
	__mock_nrf_modem_at_scanf_ReturnVarg_string("0,60,3600,30,600"); /* pattern_buf */
	__mock_nrf_modem_at_scanf_ReturnVarg_string("0,60,3600,30,600"); /* pattern_buf */
	__mock_nrf_modem_at_scanf_ReturnVarg_string("0"); /* pattern_buf */

	ret = lte_lc_periodic_search_get(&cfg);
	TEST_ASSERT_EQUAL(-EBADMSG, ret);
}

void test_lte_lc_periodic_search_get_pattern4_table_empty_fail(void)
{
	int ret;
	struct lte_lc_periodic_search_cfg cfg;

	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT%PERIODICSEARCHCONF=1",
		"%%PERIODICSEARCHCONF: %hu,%hu,%hu,\"%40[^\"]\",\"%40[^\"]\",\"%40[^\"]\",\"%40[^\"]\"",
		7);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(0); /* loop */
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(0); /* return_to_pattern */
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(10); /* band_optimization */
	__mock_nrf_modem_at_scanf_ReturnVarg_string("0,60,3600,30,600"); /* pattern_buf */
	__mock_nrf_modem_at_scanf_ReturnVarg_string("0,60,3600,30,600"); /* pattern_buf */
	__mock_nrf_modem_at_scanf_ReturnVarg_string("0,60,3600,30,600"); /* pattern_buf */
	__mock_nrf_modem_at_scanf_ReturnVarg_string("1"); /* pattern_buf */

	ret = lte_lc_periodic_search_get(&cfg);
	TEST_ASSERT_EQUAL(-EBADMSG, ret);
}

void test_lte_lc_periodic_search_get_patterns1_success(void)
{
	int ret;
	struct lte_lc_periodic_search_cfg cfg;

	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT%PERIODICSEARCHCONF=1",
		"%%PERIODICSEARCHCONF: %hu,%hu,%hu,\"%40[^\"]\",\"%40[^\"]\",\"%40[^\"]\",\"%40[^\"]\"",
		4);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(0); /* loop */
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(0); /* return_to_pattern */
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(10); /* band_optimization */
	__mock_nrf_modem_at_scanf_ReturnVarg_string("0,60,3600,,600"); /* pattern_buf */

	ret = lte_lc_periodic_search_get(&cfg);
	TEST_ASSERT_EQUAL(0, cfg.loop);
	TEST_ASSERT_EQUAL(0, cfg.return_to_pattern);
	TEST_ASSERT_EQUAL(10, cfg.band_optimization);
	TEST_ASSERT_EQUAL(LTE_LC_PERIODIC_SEARCH_PATTERN_RANGE, cfg.patterns[0].type);
	TEST_ASSERT_EQUAL(60, cfg.patterns[0].range.initial_sleep);
	TEST_ASSERT_EQUAL(3600, cfg.patterns[0].range.final_sleep);
	TEST_ASSERT_EQUAL(-1, cfg.patterns[0].range.time_to_final_sleep);
	TEST_ASSERT_EQUAL(600, cfg.patterns[0].range.pattern_end_point);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_periodic_search_get_patterns4_success(void)
{
	int ret;
	struct lte_lc_periodic_search_cfg cfg;

	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT%PERIODICSEARCHCONF=1",
		"%%PERIODICSEARCHCONF: %hu,%hu,%hu,\"%40[^\"]\",\"%40[^\"]\",\"%40[^\"]\",\"%40[^\"]\"",
		7);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(0); /* loop */
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(0); /* return_to_pattern */
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(10); /* band_optimization */
	__mock_nrf_modem_at_scanf_ReturnVarg_string("0,60,3600,,600"); /* pattern_buf */
	__mock_nrf_modem_at_scanf_ReturnVarg_string("0,30,1800,20,48"); /* pattern_buf */
	__mock_nrf_modem_at_scanf_ReturnVarg_string("1,10,20,30,40,50"); /* pattern_buf */
	__mock_nrf_modem_at_scanf_ReturnVarg_string("1,11"); /* pattern_buf */

	ret = lte_lc_periodic_search_get(&cfg);
	TEST_ASSERT_EQUAL(0, cfg.loop);
	TEST_ASSERT_EQUAL(0, cfg.return_to_pattern);
	TEST_ASSERT_EQUAL(10, cfg.band_optimization);
	TEST_ASSERT_EQUAL(LTE_LC_PERIODIC_SEARCH_PATTERN_RANGE, cfg.patterns[0].type);
	TEST_ASSERT_EQUAL(60, cfg.patterns[0].range.initial_sleep);
	TEST_ASSERT_EQUAL(3600, cfg.patterns[0].range.final_sleep);
	TEST_ASSERT_EQUAL(-1, cfg.patterns[0].range.time_to_final_sleep);
	TEST_ASSERT_EQUAL(600, cfg.patterns[0].range.pattern_end_point);
	TEST_ASSERT_EQUAL(LTE_LC_PERIODIC_SEARCH_PATTERN_RANGE, cfg.patterns[1].type);
	TEST_ASSERT_EQUAL(30, cfg.patterns[1].range.initial_sleep);
	TEST_ASSERT_EQUAL(1800, cfg.patterns[1].range.final_sleep);
	TEST_ASSERT_EQUAL(20, cfg.patterns[1].range.time_to_final_sleep);
	TEST_ASSERT_EQUAL(48, cfg.patterns[1].range.pattern_end_point);
	TEST_ASSERT_EQUAL(LTE_LC_PERIODIC_SEARCH_PATTERN_TABLE, cfg.patterns[2].type);
	TEST_ASSERT_EQUAL(10, cfg.patterns[2].table.val_1);
	TEST_ASSERT_EQUAL(20, cfg.patterns[2].table.val_2);
	TEST_ASSERT_EQUAL(30, cfg.patterns[2].table.val_3);
	TEST_ASSERT_EQUAL(40, cfg.patterns[2].table.val_4);
	TEST_ASSERT_EQUAL(50, cfg.patterns[2].table.val_5);
	TEST_ASSERT_EQUAL(LTE_LC_PERIODIC_SEARCH_PATTERN_TABLE, cfg.patterns[3].type);
	TEST_ASSERT_EQUAL(11, cfg.patterns[3].table.val_1);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_periodic_search_get_patterns4_table_values_success(void)
{
	int ret;
	struct lte_lc_periodic_search_cfg cfg;

	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT%PERIODICSEARCHCONF=1",
		"%%PERIODICSEARCHCONF: %hu,%hu,%hu,\"%40[^\"]\",\"%40[^\"]\",\"%40[^\"]\",\"%40[^\"]\"",
		7);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(0); /* loop */
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(0); /* return_to_pattern */
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(10); /* band_optimization */
	__mock_nrf_modem_at_scanf_ReturnVarg_string("1,11"); /* pattern_buf */
	__mock_nrf_modem_at_scanf_ReturnVarg_string("1,21,22"); /* pattern_buf */
	__mock_nrf_modem_at_scanf_ReturnVarg_string("1,31,32,33"); /* pattern_buf */
	__mock_nrf_modem_at_scanf_ReturnVarg_string("1,41,42,43,44"); /* pattern_buf */

	ret = lte_lc_periodic_search_get(&cfg);
	TEST_ASSERT_EQUAL(0, cfg.loop);
	TEST_ASSERT_EQUAL(0, cfg.return_to_pattern);
	TEST_ASSERT_EQUAL(10, cfg.band_optimization);
	TEST_ASSERT_EQUAL(LTE_LC_PERIODIC_SEARCH_PATTERN_TABLE, cfg.patterns[2].type);
	TEST_ASSERT_EQUAL(11, cfg.patterns[0].table.val_1);
	TEST_ASSERT_EQUAL(LTE_LC_PERIODIC_SEARCH_PATTERN_TABLE, cfg.patterns[2].type);
	TEST_ASSERT_EQUAL(21, cfg.patterns[1].table.val_1);
	TEST_ASSERT_EQUAL(22, cfg.patterns[1].table.val_2);
	TEST_ASSERT_EQUAL(LTE_LC_PERIODIC_SEARCH_PATTERN_TABLE, cfg.patterns[2].type);
	TEST_ASSERT_EQUAL(31, cfg.patterns[2].table.val_1);
	TEST_ASSERT_EQUAL(32, cfg.patterns[2].table.val_2);
	TEST_ASSERT_EQUAL(33, cfg.patterns[2].table.val_3);
	TEST_ASSERT_EQUAL(LTE_LC_PERIODIC_SEARCH_PATTERN_TABLE, cfg.patterns[3].type);
	TEST_ASSERT_EQUAL(41, cfg.patterns[3].table.val_1);
	TEST_ASSERT_EQUAL(42, cfg.patterns[3].table.val_2);
	TEST_ASSERT_EQUAL(43, cfg.patterns[3].table.val_3);
	TEST_ASSERT_EQUAL(44, cfg.patterns[3].table.val_4);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_rai_update(void)
{
	int index = 0;

	lte_lc_callback_count_expected = 2;

	test_event_data[index].type = LTE_LC_EVT_RAI_UPDATE;
	test_event_data[index].rai_cfg.cell_id = 0x12beef;
	test_event_data[index].rai_cfg.mcc = 100;
	test_event_data[index].rai_cfg.mnc = 99;
	test_event_data[index].rai_cfg.as_rai = false;
	test_event_data[index].rai_cfg.cp_rai = false;

	strcpy(at_notif, "%RAI: \"0x12BEEF\",\"10099\",0,0\r\n");
	at_monitor_dispatch(at_notif);
	index++;

	test_event_data[index].type = LTE_LC_EVT_RAI_UPDATE;
	test_event_data[index].rai_cfg.cell_id = 0xdead;
	test_event_data[index].rai_cfg.mcc = 999;
	test_event_data[index].rai_cfg.mnc = 111;
	test_event_data[index].rai_cfg.as_rai = false;
	test_event_data[index].rai_cfg.cp_rai = true;

	strcpy(at_notif, "%RAI: \"0x0000DEAD\",\"999111\",0,1\r\n");
	at_monitor_dispatch(at_notif);
}

void test_lte_lc_rai_update_fail(void)
{
	lte_lc_callback_count_expected = 0;

	/* Cell ID bigger than LTE_LC_CELL_EUTRAN_ID_MAX */
	strcpy(at_notif, "%RAI: \"0x1111BEEF\",\"10099\",0,0\r\n");
	at_monitor_dispatch(at_notif);

	/* Non integer Cell Id value */
	strcpy(at_notif, "%RAI: 0x0012BEEF,\"99910\",0,0\r\n");
	at_monitor_dispatch(at_notif);

	/* Non integer AS RAI configuration value */
	strcpy(at_notif, "%RAI: \"0x0000BEEF\",\"99910\",\"invalid_as_rai_type\",0\r\n");
	at_monitor_dispatch(at_notif);

	/* Non integer CP RAI configuration value */
	strcpy(at_notif, "%RAI: \"0x0000BEEF\",\"99910\",1,\"invalid_cp_rai_type\"\r\n");
	at_monitor_dispatch(at_notif);

	/* Non integer MCC value */
	strcpy(at_notif, "%RAI: \"0x0000BEEF\",\"Hey10\",0,0\r\n");
	at_monitor_dispatch(at_notif);

	/* Non integer MNC value */
	strcpy(at_notif, "%RAI: \"0x0000BEEF\",\"999Hi\",0,0\r\n");
	at_monitor_dispatch(at_notif);

	/* Too long PLMN string */
	strcpy(at_notif, "%RAI: \"0x0012BEEF\",\"1234567\",0,0\r\n");
	at_monitor_dispatch(at_notif);

	/* Missing parameters */
	strcpy(at_notif, "%RAI: \"0x0012BEEF\",\"123456\"\r\n");
	at_monitor_dispatch(at_notif);
}

void test_lte_lc_env_eval_invalid_params(void)
{
	int ret;
	struct lte_lc_env_eval_plmn plmn_list[CONFIG_LTE_LC_ENV_EVAL_MAX_PLMN_COUNT] = {
		{
			.mcc = 100,
			.mnc = 99
		}
	};
	struct lte_lc_env_eval_params params = {
		.eval_type = LTE_LC_ENV_EVAL_TYPE_DYNAMIC,
		.plmn_count = 0,
		.plmn_list = NULL
	};

	/* NULL params */
	ret = lte_lc_env_eval(NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	/* PLMN list NULL */
	ret = lte_lc_env_eval(&params);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	params.plmn_list = plmn_list;

	/* Zero PLMN count */
	ret = lte_lc_env_eval(&params);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	/* PLMN count exceeds CONFIG_LTE_LC_ENV_EVAL_MAX_PLMN_COUNT */
	params.plmn_count = CONFIG_LTE_LC_ENV_EVAL_MAX_PLMN_COUNT + 1;
	ret = lte_lc_env_eval(&params);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

void test_lte_lc_env_eval_invalid_funmode(void)
{
	int ret;
	struct lte_lc_env_eval_plmn plmn_list[CONFIG_LTE_LC_ENV_EVAL_MAX_PLMN_COUNT] = {
		{
			.mcc = 123,
			.mnc = 99
		}
	};
	struct lte_lc_env_eval_params params = {
		.eval_type = LTE_LC_ENV_EVAL_TYPE_DYNAMIC,
		.plmn_count = 1,
		.plmn_list = plmn_list
	};

	/* Reading functional mode fails */
	__mock_nrf_modem_at_scanf_ExpectAndReturn("AT+CFUN?", "+CFUN: %hu", -NRF_ENOMEM);

	ret = lte_lc_env_eval(&params);
	TEST_ASSERT_EQUAL(-EFAULT, ret);

	/* Functional mode is not RX only */
	__mock_nrf_modem_at_scanf_ExpectAndReturn("AT+CFUN?", "+CFUN: %hu", 1);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(LTE_LC_FUNC_MODE_NORMAL);

	ret = lte_lc_env_eval(&params);
	TEST_ASSERT_EQUAL(-EOPNOTSUPP, ret);
}

void test_lte_lc_env_eval_invalid_mccmnc(void)
{
	int ret;
	struct lte_lc_env_eval_plmn plmn_list[CONFIG_LTE_LC_ENV_EVAL_MAX_PLMN_COUNT] = {0};
	struct lte_lc_env_eval_params params = {
		.eval_type = LTE_LC_ENV_EVAL_TYPE_DYNAMIC,
		.plmn_count = 1,
		.plmn_list = plmn_list
	};

	plmn_list[0].mcc = -1;
	plmn_list[0].mnc = 0;

	__mock_nrf_modem_at_scanf_ExpectAndReturn("AT+CFUN?", "+CFUN: %hu", 1);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(LTE_LC_FUNC_MODE_RX_ONLY);

	ret = lte_lc_env_eval(&params);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	plmn_list[0].mcc = 1000;
	plmn_list[0].mnc = 0;

	__mock_nrf_modem_at_scanf_ExpectAndReturn("AT+CFUN?", "+CFUN: %hu", 1);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(LTE_LC_FUNC_MODE_RX_ONLY);

	ret = lte_lc_env_eval(&params);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	plmn_list[0].mcc = 0;
	plmn_list[0].mnc = -1;

	__mock_nrf_modem_at_scanf_ExpectAndReturn("AT+CFUN?", "+CFUN: %hu", 1);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(LTE_LC_FUNC_MODE_RX_ONLY);

	ret = lte_lc_env_eval(&params);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	plmn_list[0].mcc = 0;
	plmn_list[0].mnc = 1000;

	__mock_nrf_modem_at_scanf_ExpectAndReturn("AT+CFUN?", "+CFUN: %hu", 1);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(LTE_LC_FUNC_MODE_RX_ONLY);

	ret = lte_lc_env_eval(&params);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

void test_lte_lc_env_eval_fail(void)
{
	int ret;
	struct lte_lc_env_eval_plmn plmn_list[CONFIG_LTE_LC_ENV_EVAL_MAX_PLMN_COUNT] = {
		{
			.mcc = 123,
			.mnc = 99
		},
		{
			.mcc = 123,
			.mnc = 100
		}
	};
	struct lte_lc_env_eval_params params = {
		.eval_type = LTE_LC_ENV_EVAL_TYPE_DYNAMIC,
		.plmn_count = 2,
		.plmn_list = plmn_list
	};

	__mock_nrf_modem_at_scanf_ExpectAndReturn("AT+CFUN?", "+CFUN: %hu", 1);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(LTE_LC_FUNC_MODE_RX_ONLY);

	__mock_nrf_modem_at_printf_ExpectAndReturn(
		"AT%ENVEVAL=0,\"12399\",\"123100\"", -NRF_EFAULT);

	ret = lte_lc_env_eval(&params);
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_env_eval_no_plmns_found(void)
{
	int ret;
	int index = 0;
	struct lte_lc_env_eval_plmn plmn_list[CONFIG_LTE_LC_ENV_EVAL_MAX_PLMN_COUNT] = {
		{
			.mcc = 123,
			.mnc = 99
		},
		{
			.mcc = 123,
			.mnc = 100
		},
		{
			.mcc = 123,
			.mnc = 101
		}
	};
	struct lte_lc_env_eval_params params = {
		.eval_type = LTE_LC_ENV_EVAL_TYPE_FULL,
		.plmn_count = 3,
		.plmn_list = plmn_list
	};

	lte_lc_callback_count_expected = 1;

	test_event_data[index].type = LTE_LC_EVT_ENV_EVAL_RESULT;
	test_event_data[index].env_eval_result.status = 0;
	test_event_data[index].env_eval_result.result_count = 0;

	__mock_nrf_modem_at_scanf_ExpectAndReturn("AT+CFUN?", "+CFUN: %hu", 1);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(LTE_LC_FUNC_MODE_RX_ONLY);

	__mock_nrf_modem_at_printf_ExpectAndReturn(
		"AT%ENVEVAL=2,\"12399\",\"123100\",\"123101\"", EXIT_SUCCESS);

	ret = lte_lc_env_eval(&params);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	strcpy(at_notif, "%ENVEVAL: 0\r\n");
	at_monitor_dispatch(at_notif);

	k_sleep(K_MSEC(1));
}

void test_lte_lc_env_eval_success(void)
{
	int ret;
	int index = 0;
	struct lte_lc_env_eval_plmn plmn_list[CONFIG_LTE_LC_ENV_EVAL_MAX_PLMN_COUNT] = {
		{
			.mcc = 123,
			.mnc = 99
		},
		{
			.mcc = 123,
			.mnc = 100
		},
		{
			.mcc = 123,
			.mnc = 101
		}
	};
	struct lte_lc_env_eval_params params = {
		.eval_type = LTE_LC_ENV_EVAL_TYPE_FULL,
		.plmn_count = 3,
		.plmn_list = plmn_list
	};
	struct lte_lc_conn_eval_params results[CONFIG_LTE_LC_ENV_EVAL_MAX_PLMN_COUNT] = {0};

	lte_lc_callback_count_expected = 1;

	test_event_data[index].type = LTE_LC_EVT_ENV_EVAL_RESULT;
	test_event_data[index].env_eval_result.status = 0;
	test_event_data[index].env_eval_result.result_count = 3;
	test_event_data[index].env_eval_result.results = results;

	results[0].rrc_state = LTE_LC_RRC_MODE_IDLE;
	results[0].energy_estimate = LTE_LC_ENERGY_CONSUMPTION_NORMAL;
	results[0].tau_trig = LTE_LC_CELL_NOT_IN_TAI_LIST;
	results[0].ce_level = LTE_LC_CE_LEVEL_0;
	results[0].earfcn = 1300;
	results[0].dl_pathloss = 99;
	results[0].rsrp = 59;
	results[0].rsrq = 32;
	results[0].tx_rep = 1;
	results[0].rx_rep = 8;
	results[0].phy_cid = 123;
	results[0].band = 3;
	results[0].snr = 42;
	results[0].tx_power = -5;
	results[0].mcc = 123;
	results[0].mnc = 99;
	results[0].cell_id = 0xABBAABBA;

	results[1].rrc_state = LTE_LC_RRC_MODE_IDLE;
	results[1].energy_estimate = LTE_LC_ENERGY_CONSUMPTION_REDUCED;
	results[1].tau_trig = LTE_LC_CELL_NOT_IN_TAI_LIST;
	results[1].ce_level = LTE_LC_CE_LEVEL_0;
	results[1].earfcn = 1300;
	results[1].dl_pathloss = 99;
	results[1].rsrp = 59;
	results[1].rsrq = 32;
	results[1].tx_rep = 1;
	results[1].rx_rep = 8;
	results[1].phy_cid = 123;
	results[1].band = 3;
	results[1].snr = 42;
	results[1].tx_power = -5;
	results[1].mcc = 123;
	results[1].mnc = 100;
	results[1].cell_id = 0xEBBEEBBE;

	results[2].rrc_state = LTE_LC_RRC_MODE_IDLE;
	results[2].energy_estimate = LTE_LC_ENERGY_CONSUMPTION_EFFICIENT;
	results[2].tau_trig = LTE_LC_CELL_NOT_IN_TAI_LIST;
	results[2].ce_level = LTE_LC_CE_LEVEL_0;
	results[2].earfcn = 1300;
	results[2].dl_pathloss = 99;
	results[2].rsrp = 59;
	results[2].rsrq = 32;
	results[2].tx_rep = 1;
	results[2].rx_rep = 8;
	results[2].phy_cid = 123;
	results[2].band = 3;
	results[2].snr = 42;
	results[2].tx_power = -5;
	results[2].mcc = 123;
	results[2].mnc = 101;
	results[2].cell_id = 0xDABADABA;

	__mock_nrf_modem_at_scanf_ExpectAndReturn("AT+CFUN?", "+CFUN: %hu", 1);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(LTE_LC_FUNC_MODE_RX_ONLY);

	__mock_nrf_modem_at_printf_ExpectAndReturn(
		"AT%ENVEVAL=2,\"12399\",\"123100\",\"123101\"", EXIT_SUCCESS);

	ret = lte_lc_env_eval(&params);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	strcpy(at_notif,
		"%ENVEVAL: 0,0,7,59,32,42,\"ABBAABBA\",\"12399\",123,1300,3,1,0,-5,1,8,99\r\n");
	at_monitor_dispatch(at_notif);

	strcpy(at_notif,
		"%ENVEVAL: 0,0,8,59,32,42,\"EBBEEBBE\",\"123100\",123,1300,3,1,0,-5,1,8,99\r\n");
	at_monitor_dispatch(at_notif);

	strcpy(at_notif,
		"%ENVEVAL: 0,0,9,59,32,42,\"DABADABA\",\"123101\",123,1300,3,1,0,-5,1,8,99\r\n");
	at_monitor_dispatch(at_notif);

	/* Send one extra notification to make sure it is ignored. */
	strcpy(at_notif,
		"%ENVEVAL: 0,0,5,59,32,42,\"BADABADA\",\"123102\",123,1300,3,1,0,-5,1,8,99\r\n");
	at_monitor_dispatch(at_notif);

	strcpy(at_notif, "%ENVEVAL: 0\r\n");
	at_monitor_dispatch(at_notif);

	k_sleep(K_MSEC(1));
}

void test_lte_lc_env_eval_malformed(void)
{
	int ret;
	struct lte_lc_env_eval_plmn plmn_list[CONFIG_LTE_LC_ENV_EVAL_MAX_PLMN_COUNT] = {
		{
			.mcc = 123,
			.mnc = 99
		},
		{
			.mcc = 123,
			.mnc = 100
		},
		{
			.mcc = 123,
			.mnc = 101
		}
	};
	struct lte_lc_env_eval_params params = {
		.eval_type = LTE_LC_ENV_EVAL_TYPE_FULL,
		.plmn_count = 3,
		.plmn_list = plmn_list
	};

	/* No valid values in the notification */

	__mock_nrf_modem_at_scanf_ExpectAndReturn("AT+CFUN?", "+CFUN: %hu", 1);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(LTE_LC_FUNC_MODE_RX_ONLY);

	__mock_nrf_modem_at_printf_ExpectAndReturn(
		"AT%ENVEVAL=2,\"12399\",\"123100\",\"123101\"", EXIT_SUCCESS);

	ret = lte_lc_env_eval(&params);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	strcpy(at_notif, "%ENVEVAL: foobar\r\n");
	at_monitor_dispatch(at_notif);

	/* Too short PLMN string */

	__mock_nrf_modem_at_scanf_ExpectAndReturn("AT+CFUN?", "+CFUN: %hu", 1);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(LTE_LC_FUNC_MODE_RX_ONLY);

	__mock_nrf_modem_at_printf_ExpectAndReturn(
		"AT%ENVEVAL=2,\"12399\",\"123100\",\"123101\"", EXIT_SUCCESS);

	ret = lte_lc_env_eval(&params);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	strcpy(at_notif,
		"%ENVEVAL: 0,0,7,59,32,42,\"ABBAABBA\",\"1234\",123,1300,3,1,0,-5,1,8,99\r\n");
	at_monitor_dispatch(at_notif);

	/* Invalid MNC value in PLMN string */

	__mock_nrf_modem_at_scanf_ExpectAndReturn("AT+CFUN?", "+CFUN: %hu", 1);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(LTE_LC_FUNC_MODE_RX_ONLY);

	__mock_nrf_modem_at_printf_ExpectAndReturn(
		"AT%ENVEVAL=2,\"12399\",\"123100\",\"123101\"", EXIT_SUCCESS);

	ret = lte_lc_env_eval(&params);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	strcpy(at_notif,
		"%ENVEVAL: 0,0,7,59,32,42,\"ABBAABBA\",\"123aa\",123,1300,3,1,0,-5,1,8,99\r\n");
	at_monitor_dispatch(at_notif);

	/* Invalid MCC value in PLMN string */

	__mock_nrf_modem_at_scanf_ExpectAndReturn("AT+CFUN?", "+CFUN: %hu", 1);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(LTE_LC_FUNC_MODE_RX_ONLY);

	__mock_nrf_modem_at_printf_ExpectAndReturn(
		"AT%ENVEVAL=2,\"12399\",\"123100\",\"123101\"", EXIT_SUCCESS);

	ret = lte_lc_env_eval(&params);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	strcpy(at_notif,
		"%ENVEVAL: 0,0,7,59,32,42,\"ABBAABBA\",\"aaa12\",123,1300,3,1,0,-5,1,8,99\r\n");
	at_monitor_dispatch(at_notif);

	k_sleep(K_MSEC(1));
}

void test_lte_lc_env_eval_cancel_fail(void)
{
	int ret;

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%ENVEVALSTOP", -NRF_EFAULT);

	ret = lte_lc_env_eval_cancel();
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_env_eval_cancel(void)
{
	int ret;

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%ENVEVALSTOP", EXIT_SUCCESS);

	ret = lte_lc_env_eval_cancel();
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

/* It is required to be added to each test. That is because unity's
 * main may return nonzero, while zephyr's main currently must
 * return 0 in all cases (other values are reserved).
 */
extern int unity_main(void);

int main(void)
{
	(void)unity_main();

	return 0;
}

SYS_INIT(sys_init_helper, POST_KERNEL, 0);
