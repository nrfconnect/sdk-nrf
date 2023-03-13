/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <stdio.h>
#include <string.h>
#include <nrf_modem_at.h>
#include <modem/nrf_modem_lib.h>
#include <nrf_errno.h>

static char response[64];

#define RES_OK "OK\r\n"
#define RES_ERROR "ERROR\r\n"

#define CMD_CPMS_E_TA "AT+CPMS=\"TA\",\"TA\""
#define RES_CPMS_E_0_3 "+CPMS: 0,3,0,3\r\nOK\r\n"
#define RES_CPMS_E_1_3 "+CPMS: 1,3,1,3\r\nOK\r\n"

#define CMD_CPMS_Q "AT+CPMS?"
#define RES_CPMS_Q_0_3 "+CPMS: \"TA\",0,3,\"TA\",0,3\r\nOK\r\n"

#define CMD_CPMS_EQ "AT+CPMS=?"
#define RES_CPMS_EQ "+CPMS: (\"TA\"),(\"TA\")\r\nOK\r\n"

#define CMD_CSMS_E_0 "AT+CSMS=0"
#define CMD_CSMS_E_1 "AT+CSMS=1"
#define RES_CSMS_E_0_0_0 "+CSMS: 0,0,0\r\nOK\r\n"
#define RES_CSMS_E_1_1_0 "+CSMS: 1,1,0\r\nOK\r\n"

#define CMD_CSMS_Q "AT+CSMS?"
#define RES_CSMS_Q_0_1_1_0 "+CSMS: 0,1,1,0\r\nOK\r\n"

#define CMD_CSMS_EQ "AT+CSMS=?"
#define RES_CSMS_EQ_0 "+CSMS: 0\r\nOK\r\n"

#define CMD_CSCA_E "AT+CSCA=\"+358501234567\""

#define CMD_CSCA_Q "AT+CSCA?"
#define RES_CSCA_Q "+CSMS: \"+358501234567\""

#define CMD_CMGD_E_0 "AT+CMGD=0"
#define CMD_CMGD_E_1 "AT+CMGD=1"
#define CMD_CMGD_E_2 "AT+CMGD=2"

#define CMD_CMGD_EQ "AT+CMGD=?"
#define RES_CMGD_EQ_OK "+CMGD: (0-2)\r\nOK\r\n"

#define CMD_CMGW_E "AT+CMGW=1,9<CR>010017116031621300<CR><ESC>"
#define RES_CMGW_E_0 "+CMGW: 0"
#define RES_CMGW_E_1 "+CMGW: 1"
#define RES_CMGW_E_2 "+CMGW: 2"

#define CMD_CMSS_E_1 "AT+CMSS=1"
#define RES_CMSS_E "+CMSS: "

#define CMD_CONCAT_1 "AT+CMGD=1;+CMGD=2"
#define RES_CONCAT_1 "OK\r\nOK\r\n"

#define CMD_CONCAT_2 "AT+CMGD=?;+CEREG?"
#define RES_CONCAT_2 "+CMGD: (0-2)\r\nOK\r\n+CEREG:"

static void *suite_setup(void)
{
	int err;

	err = nrf_modem_lib_init(NORMAL_MODE);
	zassert_ok(err, "Failed to initialize");

	return NULL;
}

static void test_after(void *f)
{
	/* Delete storage */
	nrf_modem_at_cmd(response, sizeof(response), CMD_CMGD_E_0);
	nrf_modem_at_cmd(response, sizeof(response), CMD_CMGD_E_1);
	nrf_modem_at_cmd(response, sizeof(response), CMD_CMGD_E_2);
}

static void suite_teardown(void *f)
{
	int err;

	err = nrf_modem_lib_shutdown();
	zassert_ok(err, NULL);
}

ZTEST(gcf_sms, test_gcf_sms_cpms)
{
	int err;

	err = nrf_modem_at_cmd(response, sizeof(response), CMD_CPMS_E_TA);

	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, RES_CPMS_E_0_3, strlen(RES_CPMS_E_0_3), NULL);

	err = nrf_modem_at_cmd(response, sizeof(response), CMD_CPMS_Q);
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, RES_CPMS_Q_0_3, strlen(RES_CPMS_Q_0_3), NULL);

	err = nrf_modem_at_cmd(response, sizeof(response), CMD_CPMS_EQ);
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, RES_CPMS_EQ, strlen(RES_CPMS_EQ), NULL);
}

ZTEST(gcf_sms, test_gcf_sms_csms)
{
	int err;

	err = nrf_modem_at_cmd(response, sizeof(response), CMD_CSMS_E_0);
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, RES_CSMS_E_1_1_0, strlen(RES_CSMS_E_1_1_0), NULL);

	err = nrf_modem_at_cmd(response, sizeof(response), CMD_CSMS_E_1);
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, RES_CSMS_E_0_0_0, strlen(RES_CSMS_E_0_0_0), NULL);

	err = nrf_modem_at_cmd(response, sizeof(response), CMD_CSMS_Q);
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, RES_CSMS_Q_0_1_1_0, strlen(RES_CSMS_Q_0_1_1_0), NULL);

	err = nrf_modem_at_cmd(response, sizeof(response), CMD_CSMS_EQ);
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, RES_CSMS_EQ_0, strlen(RES_CSMS_EQ_0), NULL);
}

ZTEST(gcf_sms, test_gcf_sms_csca)
{
	int err;

	err = nrf_modem_at_cmd(response, sizeof(response), CMD_CSCA_E);
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, RES_OK, strlen(RES_OK), NULL);

	err = nrf_modem_at_cmd(response, sizeof(response), CMD_CSCA_Q);
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, RES_CSCA_Q, strlen(RES_CSCA_Q), NULL);
}

ZTEST(gcf_sms, test_gcf_sms_cmgd)
{
	int err;

	err = nrf_modem_at_cmd(response, sizeof(response), CMD_CMGD_E_2);
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, RES_OK, strlen(RES_OK), NULL);

	err = nrf_modem_at_cmd(response, sizeof(response), CMD_CMGD_EQ);
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, RES_CMGD_EQ_OK, strlen(RES_CMGD_EQ_OK), NULL);
}

ZTEST(gcf_sms, test_gcf_sms_cmgw)
{
	int err;

	err = nrf_modem_at_cmd(response, sizeof(response), CMD_CMGW_E);
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, RES_CMGW_E_0, strlen(RES_CMGW_E_0), NULL);

	err = nrf_modem_at_cmd(response, sizeof(response), CMD_CMGW_E);
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, RES_CMGW_E_1, strlen(RES_CMGW_E_1), NULL);
}

ZTEST(gcf_sms, test_gcf_sms_cmss)
{
	int err;

	/* This test require a SIM supporting SMS and mfw 1.3.2(?) or newer. */
	Z_TEST_SKIP_IFNDEF(CONFIG_SIM_HAS_SMS_SUPPORT);

	err = nrf_modem_at_cmd(response, sizeof(response), CMD_CMSS_E_1);
	zassert_mem_equal(response, RES_CMSS_E, strlen(RES_CMSS_E), NULL);
}

ZTEST(gcf_sms, test_gcf_sms_cmgw_cmgd)
{
	int err;

	/* fill slot 0 */
	err = nrf_modem_at_cmd(response, sizeof(response), CMD_CMGW_E);
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, RES_CMGW_E_0, strlen(RES_CMGW_E_0), NULL);

	/* fill slot 1 */
	err = nrf_modem_at_cmd(response, sizeof(response), CMD_CMGW_E);
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, RES_CMGW_E_1, strlen(RES_CMGW_E_1), NULL);

	/* delete slot 1*/
	err = nrf_modem_at_cmd(response, sizeof(response), CMD_CMGD_E_1);
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, RES_OK, strlen(RES_OK), NULL);

	/* fill slot 1 again */
	err = nrf_modem_at_cmd(response, sizeof(response), CMD_CMGW_E);
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, RES_CMGW_E_1, strlen(RES_CMGW_E_1), NULL);

	/* fill slot 2 */
	err = nrf_modem_at_cmd(response, sizeof(response), CMD_CMGW_E);
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, RES_CMGW_E_2, strlen(RES_CMGW_E_2), NULL);

	/* no more slots */
	err = nrf_modem_at_cmd(response, sizeof(response), CMD_CMGW_E);
	zassert_equal(1<<16, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, RES_ERROR, strlen(RES_ERROR), NULL);
}

ZTEST(gcf_sms, test_gcf_sms_concatenated)
{
	int err;

	err = nrf_modem_at_cmd(response, sizeof(response), CMD_CMGW_E);
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, RES_CMGW_E_0, strlen(RES_CMGW_E_0), NULL);

	err = nrf_modem_at_cmd(response, sizeof(response), CMD_CMGW_E);
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, RES_CMGW_E_1, strlen(RES_CMGW_E_1), NULL);

	err = nrf_modem_at_cmd(response, sizeof(response), CMD_CMGW_E);
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, RES_CMGW_E_2, strlen(RES_CMGW_E_2), NULL);

	err = nrf_modem_at_cmd(response, sizeof(response), CMD_CONCAT_1);
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, RES_CONCAT_1, strlen(RES_CONCAT_1), NULL);

	err = nrf_modem_at_cmd(response, sizeof(response), CMD_CPMS_E_TA);
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, RES_CPMS_E_1_3, strlen(RES_CPMS_E_1_3), NULL);

	err = nrf_modem_at_cmd(response, sizeof(response), CMD_CONCAT_2);
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, RES_CONCAT_2, strlen(RES_CONCAT_2), NULL);
}

ZTEST_SUITE(gcf_sms, NULL, suite_setup, NULL, test_after, suite_teardown);
