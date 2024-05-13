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

#define TEST_EVENT_MAX_COUNT 10

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

static void at_notif_dispatch_work_fn(struct k_work *work)
{
	at_monitor_dispatch(at_notif);
}

static K_WORK_DELAYABLE_DEFINE(at_notif_dispatch_work, at_notif_dispatch_work_fn);

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
		TEST_ASSERT_EQUAL(test_event_data[index].modem_evt, evt->modem_evt);
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

void test_lte_lc_connect_home_already_registered(void)
{
	int ret;

	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT+CEREG?", "+CEREG: %*u,%hu,%*[^,],\"%x\",", 2);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(LTE_LC_NW_REG_REGISTERED_HOME);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(0);

	ret = lte_lc_connect();
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_connect_async_null(void)
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

void test_lte_lc_connect_success(void)
{
	int ret;

	/* AT commands triggered by lte_lc_connect() */
	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT+CEREG?", "+CEREG: %*u,%hu,%*[^,],\"%x\",", 1);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(LTE_LC_NW_REG_NOT_REGISTERED);

	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT+CFUN?", "+CFUN: %hu", 1);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(LTE_LC_FUNC_MODE_OFFLINE);

	__mock_nrf_modem_at_printf_ExpectAndReturn(
		"AT%XSYSTEMMODE=1,1,1,3", 0);

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CEREG=5", 0);
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CSCON=1", 0);
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CFUN=1", 0);

	/* AT commands triggered by lte_lc_offline() */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CFUN=4", 0);

	lte_lc_callback_count_expected = 7;

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

	/* Schedule +CEREG notification to be dispatched while lte_lc_connect() blocks */
	strcpy(at_notif, "+CEREG: 1,\"002F\",\"0012BEEF\",7,,,\"00000101\",\"00010011\"\r\n");
	k_work_schedule(&at_notif_dispatch_work, K_MSEC(1));

	ret = lte_lc_connect();
	TEST_ASSERT_EQUAL(0, ret);

	ret = lte_lc_offline();
	TEST_ASSERT_EQUAL(0, ret);

	/* Send +CEREG notification to trigger de-registration events */
	strcpy(at_notif, "+CEREG: 0\r\n");
	at_monitor_dispatch(at_notif);
}

void test_lte_lc_connect_fallback(void)
{
	int ret;

	__mock_nrf_modem_at_printf_ExpectAndReturn(
		"AT%XSYSTEMMODE=1,0,1,0", 0);

	/* AT commands triggered by lte_lc_connect() */
	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT+CEREG?", "+CEREG: %*u,%hu,%*[^,],\"%x\",", 1);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(LTE_LC_NW_REG_NOT_REGISTERED);

	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT+CFUN?", "+CFUN: %hu", 1);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(LTE_LC_FUNC_MODE_OFFLINE);

	__mock_nrf_modem_at_printf_ExpectAndReturn(
		"AT%XSYSTEMMODE=1,0,1,0", 0);

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CEREG=5", 0);
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CSCON=1", 0);
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CFUN=1", 0);

	/* Fallback to NB-IoT + GNSS */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CFUN=4", 0);

	__mock_nrf_modem_at_printf_ExpectAndReturn(
		"AT%XSYSTEMMODE=0,1,1,0", 0);

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CEREG=5", 0);
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CSCON=1", 0);
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CFUN=1", 0);

	static const char xmonitor_resp[] =
		"%XMONITOR: 5,\"Operator\",\"OP\",\"20065\",\"003F\",9,20,\"0013BEEF\","
		"334,6200,66,44,\"\","
		"\"11100000\",\"11100000\",\"00111000\"";
	__cmock_nrf_modem_at_cmd_ExpectAndReturn(NULL, 0, "AT%%XMONITOR", 0);
	__cmock_nrf_modem_at_cmd_IgnoreArg_buf();
	__cmock_nrf_modem_at_cmd_IgnoreArg_len();
	__cmock_nrf_modem_at_cmd_ReturnArrayThruPtr_buf(
		(char *)xmonitor_resp, sizeof(xmonitor_resp));

	/* AT commands triggered by lte_lc_offline() */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CFUN=4", 0);

	lte_lc_callback_count_expected = 7;

	/* NB-IoT registration */
	test_event_data[0].type = LTE_LC_EVT_NW_REG_STATUS;
	test_event_data[0].nw_reg_status = LTE_LC_NW_REG_REGISTERED_ROAMING;

	test_event_data[1].type = LTE_LC_EVT_CELL_UPDATE;
	test_event_data[1].cell.id = 0x13beef;
	test_event_data[1].cell.tac = 0x3f;

	test_event_data[2].type = LTE_LC_EVT_LTE_MODE_UPDATE;
	test_event_data[2].lte_mode = LTE_LC_LTE_MODE_NBIOT;

	test_event_data[3].type = LTE_LC_EVT_PSM_UPDATE;
	test_event_data[3].psm_cfg.tau = 1440;
	test_event_data[3].psm_cfg.active_time = -1;

	/* De-registration */
	test_event_data[4].type = LTE_LC_EVT_NW_REG_STATUS;
	test_event_data[4].nw_reg_status = LTE_LC_NW_REG_NOT_REGISTERED;

	test_event_data[5].type = LTE_LC_EVT_CELL_UPDATE;
	test_event_data[5].cell.id = LTE_LC_CELL_EUTRAN_ID_INVALID;
	test_event_data[5].cell.tac = LTE_LC_CELL_TAC_INVALID;

	test_event_data[6].type = LTE_LC_EVT_LTE_MODE_UPDATE;
	test_event_data[6].lte_mode = LTE_LC_LTE_MODE_NONE;

	/* Change system mode to LTE-M + GNSS, so that fallback can be used */
	ret = lte_lc_system_mode_set(LTE_LC_SYSTEM_MODE_LTEM_GPS,
				     LTE_LC_SYSTEM_MODE_PREFER_AUTO);
	TEST_ASSERT_EQUAL(0, ret);

	/* Schedule +CEREG notification to be dispatched after timeout to trigger fallback */
	strcpy(at_notif, "+CEREG: 5,\"003F\",\"0013BEEF\",9,,,\"11100000\",\"11100000\"\r\n");
	k_work_schedule(&at_notif_dispatch_work, K_MSEC(1100));

	ret = lte_lc_connect();
	TEST_ASSERT_EQUAL(0, ret);
	k_sleep(K_MSEC(1));
	ret = lte_lc_offline();
	TEST_ASSERT_EQUAL(0, ret);

	/* Send +CEREG notification to trigger de-registration events */
	strcpy(at_notif, "+CEREG: 0\r\n");
	at_monitor_dispatch(at_notif);
}

void test_lte_lc_connect_timeout(void)
{
	int ret;

	__mock_nrf_modem_at_printf_ExpectAndReturn(
		"AT%XSYSTEMMODE=1,1,0,0", 0);

	/* AT commands triggered by lte_lc_connect() */
	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT+CEREG?", "+CEREG: %*u,%hu,%*[^,],\"%x\",", 1);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(LTE_LC_NW_REG_NOT_REGISTERED);

	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT+CFUN?", "+CFUN: %hu", 1);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(LTE_LC_FUNC_MODE_POWER_OFF);

	__mock_nrf_modem_at_printf_ExpectAndReturn(
		"AT%XSYSTEMMODE=1,1,0,0", 0);

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CEREG=5", 0);
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CSCON=1", 0);
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CFUN=1", 0);

	/* Modem is switched into offline mode after the timeout */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CFUN=4", 0);
	/* Fallback not possible, modem switched back to original functional mode */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CFUN=0", 0);

	lte_lc_callback_count_expected = 1;

	test_event_data[0].type = LTE_LC_EVT_NW_REG_STATUS;
	test_event_data[0].nw_reg_status = LTE_LC_NW_REG_SEARCHING;

	/* Change system mode to LTE-M + NB-IoT, so that fallback can not be used */
	ret = lte_lc_system_mode_set(LTE_LC_SYSTEM_MODE_LTEM_NBIOT,
				     LTE_LC_SYSTEM_MODE_PREFER_AUTO);
	TEST_ASSERT_EQUAL(0, ret);

	/* Schedule +CEREG notification to be dispatched while lte_lc_connect() blocks */
	strcpy(at_notif, "+CEREG: 2\r\n");
	k_work_schedule(&at_notif_dispatch_work, K_MSEC(1));

	ret = lte_lc_connect();
	TEST_ASSERT_EQUAL(-ETIMEDOUT, ret);
}

void test_lte_lc_normal_success(void)
{
	int ret;

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CEREG=5", EXIT_SUCCESS);
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CSCON=1", EXIT_SUCCESS);
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CFUN=1", EXIT_SUCCESS);
	ret = lte_lc_normal();
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_normal_fail1(void)
{
	int ret;

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CEREG=5", -NRF_ENOMEM);
	ret = lte_lc_normal();
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_normal_fail2(void)
{
	int ret;

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CEREG=5", EXIT_SUCCESS);
	/* enable_notifications */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CSCON=1", -NRF_ENOMEM);

	ret = lte_lc_normal();
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_normal_fail3(void)
{
	int ret;

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CEREG=5", EXIT_SUCCESS);
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CSCON=1", EXIT_SUCCESS);
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

void test_lte_lc_psm_param_set_seconds_enable_both_set(void)
{
	int ret;

	ret = lte_lc_psm_param_set_seconds(60, 600);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	__mock_nrf_modem_at_printf_ExpectAndReturn(
		"AT+CPSMS=1,,,\"01111110\",\"00101010\"", EXIT_SUCCESS);
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

void test_lte_lc_psm_get_badmsg(void)
{
	int ret;
	int tau;
	int active_time;

	/* Missing T3412 legacy timer value */
	static const char xmonitor_resp[] =
		"%XMONITOR: 1,\"Operator\",\"OP\",\"20065\",\"002F\",7,20,\"0012BEEF\","
		"334,6200,66,44,\"\","
		"\"00000101\",\"00010011\"";
	__cmock_nrf_modem_at_cmd_ExpectAndReturn(NULL, 0, "AT%%XMONITOR", 0);
	__cmock_nrf_modem_at_cmd_IgnoreArg_buf();
	__cmock_nrf_modem_at_cmd_IgnoreArg_len();
	__cmock_nrf_modem_at_cmd_ReturnArrayThruPtr_buf(
		(char *)xmonitor_resp, sizeof(xmonitor_resp));

	ret = lte_lc_psm_get(&tau, &active_time);
	TEST_ASSERT_EQUAL(-EBADMSG, ret);
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

void test_lte_lc_ptw_wrong_mode(void)
{
	int ret;

	ret = lte_lc_ptw_set(LTE_LC_LTE_MODE_NONE, NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

void test_lte_lc_ptw_wrong_ptw(void)
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

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CEDRXS=3", EXIT_SUCCESS);
	ret = lte_lc_edrx_req(false);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_edrx_req_enable_fail1(void)
{
	int ret;

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CEDRXS=2,4,\"0000\"", -NRF_ENOMEM);
	ret = lte_lc_edrx_req(true);
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_edrx_req_enable_fail2(void)
{
	int ret;

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CEDRXS=2,4,\"0000\"", EXIT_SUCCESS);
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CEDRXS=2,5,\"1001\"", -NRF_ENOMEM);
	ret = lte_lc_edrx_req(true);
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_edrx_req_enable_success(void)
{
	int ret;
	strcpy(at_notif, "+CEDRXP: 4,\"1000\",\"0010\",\"1110\"\r\n");

	lte_lc_callback_count_expected = 2;

	test_event_data[0].type = LTE_LC_EVT_EDRX_UPDATE;
	test_event_data[0].edrx_cfg.mode = LTE_LC_LTE_MODE_LTEM;
	test_event_data[0].edrx_cfg.edrx = 20.48;
	test_event_data[0].edrx_cfg.ptw = 19.2;

	test_event_data[1].type = LTE_LC_EVT_EDRX_UPDATE;
	test_event_data[1].edrx_cfg.mode = LTE_LC_LTE_MODE_LTEM;
	test_event_data[1].edrx_cfg.edrx = 20.48;
	test_event_data[1].edrx_cfg.ptw = 5.12;

	ret = lte_lc_edrx_param_set(LTE_LC_LTE_MODE_LTEM, "0010");
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	ret = lte_lc_edrx_param_set(LTE_LC_LTE_MODE_NBIOT, "0100");
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	ret = lte_lc_ptw_set(LTE_LC_LTE_MODE_LTEM, "0011");
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CEDRXS=2,4,\"0010\"", EXIT_SUCCESS);
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CEDRXS=2,5,\"0100\"", EXIT_SUCCESS);
	ret = lte_lc_edrx_req(true);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%XPTW=4,\"0011\"", EXIT_SUCCESS);
	at_monitor_dispatch(at_notif);
	k_sleep(K_MSEC(1));

	strcpy(at_notif, "+CEDRXP: 4,\"1000\",\"0010\",\"0011\"\r\n");
	at_monitor_dispatch(at_notif);
	k_sleep(K_MSEC(1));
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

void test_lte_lc_system_mode_get_all_modes(void)
{
	int ret;
	enum lte_lc_system_mode mode;
	enum lte_lc_system_mode_preference preference;

	int modes[7][3] = {
		{1, 0, 0},
		{0, 1, 0},
		{0, 0, 1},
		{1, 0, 1},
		{0, 1, 1},
		{1, 1, 0},
		{1, 1, 1},
	};

	for (size_t i = 0; i < ARRAY_SIZE(modes); ++i) {
		__mock_nrf_modem_at_scanf_ExpectAndReturn(
			"AT%XSYSTEMMODE?", "%%XSYSTEMMODE: %d,%d,%d,%d", 4);
		__mock_nrf_modem_at_scanf_ReturnVarg_int(modes[i][0]); /* ltem_mode */
		__mock_nrf_modem_at_scanf_ReturnVarg_int(modes[i][1]); /* nbiot_mode */
		__mock_nrf_modem_at_scanf_ReturnVarg_int(modes[i][2]); /* gps_mode */
		__mock_nrf_modem_at_scanf_ReturnVarg_int(0); /* mode_preference */

		ret = lte_lc_system_mode_get(&mode, &preference);
		TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	}
}

void test_lte_lc_system_mode_get_all_preferences(void)
{
	int ret;
	enum lte_lc_system_mode mode;
	enum lte_lc_system_mode_preference preference;

	for (int i = 0; i < 5; ++i) {
		__mock_nrf_modem_at_scanf_ExpectAndReturn(
			"AT%XSYSTEMMODE?", "%%XSYSTEMMODE: %d,%d,%d,%d", 4);
		__mock_nrf_modem_at_scanf_ReturnVarg_int(1); /* ltem_mode */
		__mock_nrf_modem_at_scanf_ReturnVarg_int(1); /* nbiot_mode */
		__mock_nrf_modem_at_scanf_ReturnVarg_int(0); /* gps_mode */
		__mock_nrf_modem_at_scanf_ReturnVarg_int(i); /* mode_preference */

		ret = lte_lc_system_mode_get(&mode, &preference);
		TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	}
}

void test_lte_lc_func_mode_set_all_modes(void)
{
	int ret;
	char at_cmd[12];

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CEREG=5", EXIT_SUCCESS);
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CSCON=1", EXIT_SUCCESS);
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CFUN=21", EXIT_SUCCESS);
	ret = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_ACTIVATE_LTE);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	enum lte_lc_func_mode modes[] = {
		LTE_LC_FUNC_MODE_POWER_OFF,
		LTE_LC_FUNC_MODE_RX_ONLY,
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

void test_lte_lc_lte_mode_get_badmsg(void)
{
	int ret;
	enum lte_lc_lte_mode mode;

	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT+CEREG?", "+CEREG: %*u,%*u,%*[^,],%*[^,],%hu", -NRF_EBADMSG);

	ret = lte_lc_lte_mode_get(&mode);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	TEST_ASSERT_EQUAL(LTE_LC_LTE_MODE_NONE, mode);
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
}

void test_lte_lc_cereg_with_xmonitor(void)
{
	strcpy(at_notif, "+CEREG: 5,\"4321\",\"87654321\",9,,,\"11100000\",\"11100000\"\r\n");

	static const char xmonitor_resp[] =
		"%XMONITOR: 1,\"Operator\",\"OP\",\"20065\",\"4321\",9,20,\"12345678\","
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
	test_event_data[1].cell.id = -1;
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

void test_lte_lc_neighbor_cell_measurement_neighbors(void)
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

void test_lte_lc_neighbor_cell_measurement_gci(void)
{
	int ret;
	struct lte_lc_ncellmeas_params params = {
		.search_type = LTE_LC_NEIGHBOR_SEARCH_TYPE_GCI_EXTENDED_LIGHT,
		.gci_count = 10,
	};
	strcpy(at_notif,
		"%NCELLMEAS: 0,"
		"\"00112233\",\"11199\",\"1A2B\",64,20877,6200,110,53,22,189205,1,0,"
		"\"00567812\",\"11198\",\"3C4D\",65535,4,1300,75,53,16,189241,0,0,"
		"\"0011AABB\",\"11297\",\"5E6F\",65534,5,2300,449,51,11,189245,0,0\r\n");

	lte_lc_callback_count_expected = 1;

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS=4,10", EXIT_SUCCESS);

	ret = lte_lc_neighbor_cell_measurement(&params);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	test_event_data[0].type = LTE_LC_EVT_NEIGHBOR_CELL_MEAS;
	test_event_data[0].cells_info.current_cell.mcc = 111;
	test_event_data[0].cells_info.current_cell.mnc = 99;
	test_event_data[0].cells_info.current_cell.id = 0x00112233;
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

void test_lte_lc_neighbor_cell_measurement_gci_max_length(void)
{
	int ret;
	struct lte_lc_ncellmeas_params params = {
		.search_type = LTE_LC_NEIGHBOR_SEARCH_TYPE_GCI_EXTENDED_COMPLETE,
		.gci_count = 15,
	};

	/* 18446744073709551614 is the maximum value for timing_advance_meas_time and
	 * measurement_time in @ref lte_lc_cells_info.
	 * This value could be represented with uint64_t but cannot be stored by at_parser,
	 * which internally uses int64_t value for all integers.
	 * Hence, the maximum value for these fields is represented by 63 bits and is
	 * 9223372036854775807, which still represents millions of years.
	 */
	strcpy(at_notif,
		/* Status */
		"%NCELLMEAS: 0,"
		/* Current cell */
		"\"00123456\",\"555555\",\"0102\",65534,18446744073709551614,"
		"999999,123,127,-127,184467440737095516140,1,17,"
		/* Neighbor cells (17) */
		"333333,100,101,102,0,333333,103,104,105,0,"
		"333333,106,107,108,0,333333,109,110,111,0,"
		"444444,112,113,114,0,444444,115,116,117,0,"
		"444444,118,119,120,0,444444,121,122,123,0,"
		"555555,124,125,126,0,555555,127,128,129,0,"
		"555555,130,131,132,0,555555,133,134,135,0,"
		"666666,136,137,138,0,666666,139,140,141,0,"
		"666666,142,143,144,0,666666,145,146,147,0,"
		"777777,148,149,150,0,"
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
	test_event_data[0].cells_info.current_cell.timing_advance_meas_time = 9223372036854775807;
	test_event_data[0].cells_info.current_cell.measurement_time = 9223372036854775807;
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
	test_gci_cells[0].timing_advance_meas_time = 9223372036854775807;
	test_gci_cells[0].measurement_time = 9223372036854775807;
	test_gci_cells[0].phys_cell_id = 123;
	test_gci_cells[0].rsrp = 127;
	test_gci_cells[0].rsrq = -127;

	test_gci_cells[1].mcc = 555;
	test_gci_cells[1].mnc = 555;
	test_gci_cells[1].id = 0x02345678;
	test_gci_cells[1].tac = 0x0102;
	test_gci_cells[1].earfcn = 999999;
	test_gci_cells[1].timing_advance = 65534;
	test_gci_cells[1].timing_advance_meas_time = 9223372036854775807;
	test_gci_cells[1].measurement_time = 9223372036854775807;
	test_gci_cells[1].phys_cell_id = 123;
	test_gci_cells[1].rsrp = 127;
	test_gci_cells[1].rsrq = -127;

	test_gci_cells[2].mcc = 555;
	test_gci_cells[2].mnc = 555;
	test_gci_cells[2].id = 0x03456789;
	test_gci_cells[2].tac = 0x0102;
	test_gci_cells[2].earfcn = 999999;
	test_gci_cells[2].timing_advance = 65534;
	test_gci_cells[2].timing_advance_meas_time = 9223372036854775807;
	test_gci_cells[2].measurement_time = 9223372036854775807;
	test_gci_cells[2].phys_cell_id = 123;
	test_gci_cells[2].rsrp = 127;
	test_gci_cells[2].rsrq = -127;

	test_gci_cells[3].mcc = 555;
	test_gci_cells[3].mnc = 555;
	test_gci_cells[3].id = 0x0456789A;
	test_gci_cells[3].tac = 0x0102;
	test_gci_cells[3].earfcn = 999999;
	test_gci_cells[3].timing_advance = 65534;
	test_gci_cells[3].timing_advance_meas_time = 9223372036854775807;
	test_gci_cells[3].measurement_time = 9223372036854775807;
	test_gci_cells[3].phys_cell_id = 123;
	test_gci_cells[3].rsrp = 127;
	test_gci_cells[3].rsrq = -127;

	test_gci_cells[4].mcc = 555;
	test_gci_cells[4].mnc = 555;
	test_gci_cells[4].id = 0x056789AB;
	test_gci_cells[4].tac = 0x0102;
	test_gci_cells[4].earfcn = 999999;
	test_gci_cells[4].timing_advance = 65534;
	test_gci_cells[4].timing_advance_meas_time = 9223372036854775807;
	test_gci_cells[4].measurement_time = 9223372036854775807;
	test_gci_cells[4].phys_cell_id = 123;
	test_gci_cells[4].rsrp = 127;
	test_gci_cells[4].rsrq = -127;

	test_gci_cells[5].mcc = 555;
	test_gci_cells[5].mnc = 555;
	test_gci_cells[5].id = 0x06789ABC;
	test_gci_cells[5].tac = 0x0102;
	test_gci_cells[5].earfcn = 999999;
	test_gci_cells[5].timing_advance = 65534;
	test_gci_cells[5].timing_advance_meas_time = 9223372036854775807;
	test_gci_cells[5].measurement_time = 9223372036854775807;
	test_gci_cells[5].phys_cell_id = 123;
	test_gci_cells[5].rsrp = 127;
	test_gci_cells[5].rsrq = -127;

	test_gci_cells[6].mcc = 555;
	test_gci_cells[6].mnc = 555;
	test_gci_cells[6].id = 0x0789ABCD;
	test_gci_cells[6].tac = 0x0102;
	test_gci_cells[6].earfcn = 999999;
	test_gci_cells[6].timing_advance = 65534;
	test_gci_cells[6].timing_advance_meas_time = 9223372036854775807;
	test_gci_cells[6].measurement_time = 9223372036854775807;
	test_gci_cells[6].phys_cell_id = 123;
	test_gci_cells[6].rsrp = 127;
	test_gci_cells[6].rsrq = -127;

	test_gci_cells[7].mcc = 555;
	test_gci_cells[7].mnc = 555;
	test_gci_cells[7].id = 0x089ABCDE;
	test_gci_cells[7].tac = 0x0102;
	test_gci_cells[7].earfcn = 999999;
	test_gci_cells[7].timing_advance = 65534;
	test_gci_cells[7].timing_advance_meas_time = 9223372036854775807;
	test_gci_cells[7].measurement_time = 9223372036854775807;
	test_gci_cells[7].phys_cell_id = 123;
	test_gci_cells[7].rsrp = 127;
	test_gci_cells[7].rsrq = -127;

	test_gci_cells[8].mcc = 555;
	test_gci_cells[8].mnc = 555;
	test_gci_cells[8].id = 0x09ABCDEF;
	test_gci_cells[8].tac = 0x0102;
	test_gci_cells[8].earfcn = 999999;
	test_gci_cells[8].timing_advance = 65534;
	test_gci_cells[8].timing_advance_meas_time = 9223372036854775807;
	test_gci_cells[8].measurement_time = 9223372036854775807;
	test_gci_cells[8].phys_cell_id = 123;
	test_gci_cells[8].rsrp = 127;
	test_gci_cells[8].rsrq = -127;

	test_gci_cells[9].mcc = 555;
	test_gci_cells[9].mnc = 555;
	test_gci_cells[9].id = 0x0ABCDEF0;
	test_gci_cells[9].tac = 0x0102;
	test_gci_cells[9].earfcn = 999999;
	test_gci_cells[9].timing_advance = 65534;
	test_gci_cells[9].timing_advance_meas_time = 9223372036854775807;
	test_gci_cells[9].measurement_time = 9223372036854775807;
	test_gci_cells[9].phys_cell_id = 123;
	test_gci_cells[9].rsrp = 127;
	test_gci_cells[9].rsrq = -127;

	test_gci_cells[10].mcc = 555;
	test_gci_cells[10].mnc = 555;
	test_gci_cells[10].id = 0x0BCDEF01;
	test_gci_cells[10].tac = 0x0102;
	test_gci_cells[10].earfcn = 999999;
	test_gci_cells[10].timing_advance = 65534;
	test_gci_cells[10].timing_advance_meas_time = 9223372036854775807;
	test_gci_cells[10].measurement_time = 9223372036854775807;
	test_gci_cells[10].phys_cell_id = 123;
	test_gci_cells[10].rsrp = 127;
	test_gci_cells[10].rsrq = -127;

	test_gci_cells[11].mcc = 555;
	test_gci_cells[11].mnc = 555;
	test_gci_cells[11].id = 0x0CDEF012;
	test_gci_cells[11].tac = 0x0102;
	test_gci_cells[11].earfcn = 999999;
	test_gci_cells[11].timing_advance = 65534;
	test_gci_cells[11].timing_advance_meas_time = 9223372036854775807;
	test_gci_cells[11].measurement_time = 9223372036854775807;
	test_gci_cells[11].phys_cell_id = 123;
	test_gci_cells[11].rsrp = 127;
	test_gci_cells[11].rsrq = -127;

	test_gci_cells[12].mcc = 555;
	test_gci_cells[12].mnc = 555;
	test_gci_cells[12].id = 0x0DEF0123;
	test_gci_cells[12].tac = 0x0102;
	test_gci_cells[12].earfcn = 999999;
	test_gci_cells[12].timing_advance = 65534;
	test_gci_cells[12].timing_advance_meas_time = 9223372036854775807;
	test_gci_cells[12].measurement_time = 9223372036854775807;
	test_gci_cells[12].phys_cell_id = 123;
	test_gci_cells[12].rsrp = 127;
	test_gci_cells[12].rsrq = -127;

	test_gci_cells[13].mcc = 555;
	test_gci_cells[13].mnc = 555;
	test_gci_cells[13].id = 0x0EF01234;
	test_gci_cells[13].tac = 0x0102;
	test_gci_cells[13].earfcn = 999999;
	test_gci_cells[13].timing_advance = 65534;
	test_gci_cells[13].timing_advance_meas_time = 9223372036854775807;
	test_gci_cells[13].measurement_time = 9223372036854775807;
	test_gci_cells[13].phys_cell_id = 123;
	test_gci_cells[13].rsrp = 127;
	test_gci_cells[13].rsrq = -127;

	at_monitor_dispatch(at_notif);
}

void test_lte_lc_neighbor_cell_measurement_cancel(void)
{
	int ret;

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEASSTOP", EXIT_SUCCESS);
	ret = lte_lc_neighbor_cell_measurement_cancel();
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEASSTOP", -NRF_ENOMEM);
	ret = lte_lc_neighbor_cell_measurement_cancel();
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_modem_sleep_event(void)
{
	lte_lc_callback_count_expected = 5;

	strcpy(at_notif, "%XMODEMSLEEP: 1,12345\r\n");
	test_event_data[0].type = LTE_LC_EVT_MODEM_SLEEP_ENTER;
	test_event_data[0].modem_sleep.type = LTE_LC_MODEM_SLEEP_PSM;
	test_event_data[0].modem_sleep.time = 12345;
	at_monitor_dispatch(at_notif);

	strcpy(at_notif, "%XMODEMSLEEP: 2,5000\r\n");
	test_event_data[1].type = LTE_LC_EVT_MODEM_SLEEP_EXIT_PRE_WARNING;
	test_event_data[1].modem_sleep.type = LTE_LC_MODEM_SLEEP_RF_INACTIVITY;
	test_event_data[1].modem_sleep.time = 5000;
	at_monitor_dispatch(at_notif);

	strcpy(at_notif, "%XMODEMSLEEP: 3,0\r\n");
	test_event_data[2].type = LTE_LC_EVT_MODEM_SLEEP_EXIT;
	test_event_data[2].modem_sleep.type = LTE_LC_MODEM_SLEEP_LIMITED_SERVICE;
	test_event_data[2].modem_sleep.time = 0;
	at_monitor_dispatch(at_notif);

	strcpy(at_notif, "%XMODEMSLEEP: 7,123456789\r\n");
	test_event_data[3].type = LTE_LC_EVT_MODEM_SLEEP_ENTER;
	test_event_data[3].modem_sleep.type = LTE_LC_MODEM_SLEEP_PROPRIETARY_PSM;
	test_event_data[3].modem_sleep.time = 123456789;
	at_monitor_dispatch(at_notif);

	strcpy(at_notif, "%XMODEMSLEEP: 4,0\r\n");
	test_event_data[4].type = LTE_LC_EVT_MODEM_SLEEP_EXIT;
	test_event_data[4].modem_sleep.type = LTE_LC_MODEM_SLEEP_FLIGHT_MODE;
	test_event_data[4].modem_sleep.time = 0;
	at_monitor_dispatch(at_notif);
}

void test_lte_lc_modem_sleep_event_ignore(void)
{
	strcpy(at_notif, "%%XMODEMSLEEP: 5\r\n");
	at_monitor_dispatch(at_notif);

	strcpy(at_notif, "%%XMODEMSLEEP: 6\r\n");
	at_monitor_dispatch(at_notif);
}

void test_lte_lc_modem_events(void)
{
	int ret;
	int index = 0;

	lte_lc_callback_count_expected = 10;

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%MDMEV=2", EXIT_SUCCESS);
	ret = lte_lc_modem_events_enable();
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%MDMEV=2", EXIT_FAILURE);
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%MDMEV=1", EXIT_SUCCESS);
	ret = lte_lc_modem_events_enable();
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	strcpy(at_notif, "%MDMEV: ME OVERHEATED\r\n");
	test_event_data[index].type = LTE_LC_EVT_MODEM_EVENT;
	test_event_data[index].modem_evt = LTE_LC_MODEM_EVT_OVERHEATED;
	at_monitor_dispatch(at_notif);
	index++;

	strcpy(at_notif, "%MDMEV: NO IMEI\r\n");
	test_event_data[index].type = LTE_LC_EVT_MODEM_EVENT;
	test_event_data[index].modem_evt = LTE_LC_MODEM_EVT_NO_IMEI;
	at_monitor_dispatch(at_notif);
	index++;

	strcpy(at_notif, "%MDMEV: ME BATTERY LOW\r\n");
	test_event_data[index].type = LTE_LC_EVT_MODEM_EVENT;
	test_event_data[index].modem_evt = LTE_LC_MODEM_EVT_BATTERY_LOW;
	at_monitor_dispatch(at_notif);
	index++;

	strcpy(at_notif, "%MDMEV: RESET LOOP\r\n");
	test_event_data[index].type = LTE_LC_EVT_MODEM_EVENT;
	test_event_data[index].modem_evt = LTE_LC_MODEM_EVT_RESET_LOOP;
	at_monitor_dispatch(at_notif);
	index++;

	strcpy(at_notif, "%MDMEV: SEARCH STATUS 1\r\n");
	test_event_data[index].type = LTE_LC_EVT_MODEM_EVENT;
	test_event_data[index].modem_evt = LTE_LC_MODEM_EVT_LIGHT_SEARCH_DONE;
	at_monitor_dispatch(at_notif);
	index++;

	strcpy(at_notif, "%MDMEV: SEARCH STATUS 2\r\n");
	test_event_data[index].type = LTE_LC_EVT_MODEM_EVENT;
	test_event_data[index].modem_evt = LTE_LC_MODEM_EVT_SEARCH_DONE;
	at_monitor_dispatch(at_notif);
	index++;

	strcpy(at_notif, "%MDMEV: PRACH CE-LEVEL 0\r\n");
	test_event_data[index].type = LTE_LC_EVT_MODEM_EVENT;
	test_event_data[index].modem_evt = LTE_LC_MODEM_EVT_CE_LEVEL_0;
	at_monitor_dispatch(at_notif);
	index++;

	strcpy(at_notif, "%MDMEV: PRACH CE-LEVEL 1\r\n");
	test_event_data[index].type = LTE_LC_EVT_MODEM_EVENT;
	test_event_data[index].modem_evt = LTE_LC_MODEM_EVT_CE_LEVEL_1;
	at_monitor_dispatch(at_notif);
	index++;

	strcpy(at_notif, "%MDMEV: PRACH CE-LEVEL 2\r\n");
	test_event_data[index].type = LTE_LC_EVT_MODEM_EVENT;
	test_event_data[index].modem_evt = LTE_LC_MODEM_EVT_CE_LEVEL_2;
	at_monitor_dispatch(at_notif);
	index++;

	strcpy(at_notif, "%MDMEV: PRACH CE-LEVEL 3\r\n");
	test_event_data[index].type = LTE_LC_EVT_MODEM_EVENT;
	test_event_data[index].modem_evt = LTE_LC_MODEM_EVT_CE_LEVEL_3;
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

	strcpy(at_notif, "%MDMEV: PRACH CE-LEVEL 4\r\n");
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

void test_lte_lc_conn_eval_params_get_null(void)
{
	int ret;

	ret = lte_lc_conn_eval_params_get(NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

void test_lte_lc_conn_eval_params_get_func_fail(void)
{
	int ret;
	struct lte_lc_conn_eval_params params;

	__mock_nrf_modem_at_scanf_ExpectAndReturn("AT+CFUN?", "+CFUN: %hu", -NRF_ENOMEM);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(LTE_LC_FUNC_MODE_NORMAL);

	ret = lte_lc_conn_eval_params_get(&params);
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_conn_eval_params_wrong_func_mode(void)
{
	int ret;
	struct lte_lc_conn_eval_params params;

	__mock_nrf_modem_at_scanf_ExpectAndReturn("AT+CFUN?", "+CFUN: %hu", 1);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(LTE_LC_FUNC_MODE_OFFLINE);

	ret = lte_lc_conn_eval_params_get(&params);
	TEST_ASSERT_EQUAL(-EOPNOTSUPP, ret);
}

void test_lte_lc_conn_eval_params_at_fail(void)
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

void test_lte_lc_conn_eval_params_coneval_fail(void)
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

void test_lte_lc_conn_eval_params_coneval_success(void)
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

void test_lte_lc_periodic_search_set_null(void)
{
	int ret;

	ret = lte_lc_periodic_search_set(NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

void test_lte_lc_periodic_search_set_success(void)
{
	int ret;
	struct lte_lc_periodic_search_cfg cfg = { 0 };

	cfg.pattern_count = 1;
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

void test_lte_lc_periodic_search_set_at_cme(void)
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

void test_lte_lc_periodic_search_get_badmsg(void)
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

void test_lte_lc_periodic_search_get_success(void)
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
