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

void wrap_lc_init(void)
{
	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT%XSYSTEMMODE?", "%%XSYSTEMMODE: %d,%d,%d,%d", 4);
	__mock_nrf_modem_at_scanf_ReturnVarg_int(1); /* ltem_mode */
	__mock_nrf_modem_at_scanf_ReturnVarg_int(1); /* nbiot_mode */
	__mock_nrf_modem_at_scanf_ReturnVarg_int(1); /* gps_mode */
	__mock_nrf_modem_at_scanf_ReturnVarg_int(0); /* mode_preference */

	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT%%XSYSTEMMODE=%s,%c", EXIT_SUCCESS);
	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT+CEREG=5", EXIT_SUCCESS);
	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT+CSCON=1", EXIT_SUCCESS);
	int ret = lte_lc_init();

	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void setUp(void)
{
	mock_nrf_modem_at_Init();
	wrap_lc_init();
}

void tearDown(void)
{
	int ret;

	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT+CFUN=%d", EXIT_SUCCESS);

	ret = lte_lc_deinit();
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

void test_lte_lc_deregister_handler_not_initialized(void)
{
	int ret;

	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT+CFUN=%d", EXIT_SUCCESS);
	ret = lte_lc_deinit();
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	ret = lte_lc_deregister_handler(NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	/* this is just to make the tearDown work again */
	wrap_lc_init();
}

void test_lte_lc_deinit_twice(void)
{
	int ret;

	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT+CFUN=%d", EXIT_SUCCESS);
	ret = lte_lc_deinit();
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	ret = lte_lc_deinit();
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	/* this is just to make the tearDown work again */
	wrap_lc_init();
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

void test_lte_lc_init_and_connect_home_already_registered(void)
{
	int ret;

	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT+CEREG?", "+CEREG: %*u,%hu,%*[^,],\"%x\",", 2);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(LTE_LC_NW_REG_REGISTERED_HOME);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(0);

	ret = lte_lc_init_and_connect();
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_init_and_connect_async_null(void)
{
	int ret;

	ret = lte_lc_init_and_connect_async(NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

void test_lte_lc_normal_success(void)
{
	int ret;

	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT+CEREG=5", EXIT_SUCCESS);
	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT+CSCON=1", EXIT_SUCCESS);
	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT+CFUN=%d", EXIT_SUCCESS);
	ret = lte_lc_normal();
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_normal_fail1(void)
{
	int ret;

	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT+CEREG=5", -NRF_ENOMEM);
	ret = lte_lc_normal();
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_normal_fail2(void)
{
	int ret;

	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT+CEREG=5", EXIT_SUCCESS);
	/* enable_notifications */
	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT+CSCON=1", -NRF_ENOMEM);
	__cmock_nrf_modem_at_cmd_ExpectAnyArgsAndReturn(-NRF_ENOMEM);
	/* error is ignored */
	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT+CFUN=%d", EXIT_SUCCESS);


	ret = lte_lc_normal();
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_normal_fail3(void)
{
	int ret;
	char modem_ver[] = "MODEM_FW_VER_1.0\r\nOK";

	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT+CEREG=5", EXIT_SUCCESS);
	/* enable_notifications */
	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT+CSCON=1", -NRF_ENOMEM);

	/* error is ignored but modem FW version is checked */
	__cmock_nrf_modem_at_cmd_ExpectAndReturn(NULL, 0, "AT+CGMR", EXIT_SUCCESS);
	__cmock_nrf_modem_at_cmd_IgnoreArg_buf();
	__cmock_nrf_modem_at_cmd_IgnoreArg_len();
	__cmock_nrf_modem_at_cmd_ReturnArrayThruPtr_buf(modem_ver, sizeof(modem_ver));

	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT+CFUN=%d", EXIT_SUCCESS);
	ret = lte_lc_normal();
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_normal_fail4(void)
{
	int ret;

	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT+CEREG=5", EXIT_SUCCESS);
	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT+CSCON=1", EXIT_SUCCESS);
	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT+CFUN=%d", -NRF_ENOMEM);
	ret = lte_lc_normal();
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_offline_success(void)
{
	int ret;

	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT+CFUN=%d", EXIT_SUCCESS);
	ret = lte_lc_offline();
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_offline_fail(void)
{
	int ret;

	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT+CFUN=%d", -NRF_ENOMEM);
	ret = lte_lc_offline();
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_power_off_success(void)
{
	int ret;

	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT+CFUN=%d", EXIT_SUCCESS);
	ret = lte_lc_power_off();
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_power_off_fail(void)
{
	int ret;

	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT+CFUN=%d", -NRF_ENOMEM);
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

	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT+CPSMS=1", EXIT_SUCCESS);
	ret = lte_lc_psm_req(true);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_psm_req_enable_rptau_default(void)
{
	int ret;

	ret = lte_lc_psm_param_set(NULL, "01001001");
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT+CPSMS=1,,,,\"%s\"", EXIT_SUCCESS);
	ret = lte_lc_psm_req(true);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_psm_req_enable_rat_default(void)
{
	int ret;

	ret = lte_lc_psm_param_set("00000111", NULL);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT+CPSMS=1,,,\"%s\"", EXIT_SUCCESS);
	ret = lte_lc_psm_req(true);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_psm_req_enable_both_set(void)
{
	int ret;

	ret = lte_lc_psm_param_set("00000111", "01001001");
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT+CPSMS=1,,,\"%s\",\"%s\"", EXIT_SUCCESS);
	ret = lte_lc_psm_req(true);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_psm_req_enable_fail(void)
{
	int ret;

	ret = lte_lc_psm_param_set(NULL, NULL);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT+CPSMS=1", NRF_MODEM_AT_ERROR);
	ret = lte_lc_psm_req(true);
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_psm_req_disable_success(void)
{
	int ret;

	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT+CPSMS=", EXIT_SUCCESS);
	ret = lte_lc_psm_req(false);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
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

	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT+CEDRXS=3", -NRF_ENOMEM);
	ret = lte_lc_edrx_req(false);
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_edrx_req_disable_success(void)
{
	int ret;

	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT+CEDRXS=3", EXIT_SUCCESS);
	ret = lte_lc_edrx_req(false);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_edrx_req_enable_fail1(void)
{
	int ret;

	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT+CEDRXS=2,%d,\"%s\"", -NRF_ENOMEM);
	ret = lte_lc_edrx_req(true);
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_edrx_req_enable_fail2(void)
{
	int ret;

	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT+CEDRXS=2,%d,\"%s\"", EXIT_SUCCESS);
	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT%%XPTW=%d,\"%s\"", -NRF_ENOMEM);
	ret = lte_lc_edrx_req(true);
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_edrx_req_enable_fail3(void)
{
	int ret;

	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT+CEDRXS=2,%d,\"%s\"", EXIT_SUCCESS);
	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT%%XPTW=%d,\"%s\"", EXIT_SUCCESS);
	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT+CEDRXS=2,%d,\"%s\"", -NRF_ENOMEM);
	ret = lte_lc_edrx_req(true);
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_edrx_req_enable_success(void)
{
	int ret;

	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT+CEDRXS=2,%d,\"%s\"", EXIT_SUCCESS);
	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT%%XPTW=%d,\"%s\"", EXIT_SUCCESS);
	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT+CEDRXS=2,%d,\"%s\"", EXIT_SUCCESS);
	ret = lte_lc_edrx_req(true);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_rai_req_fail1(void)
{
	int ret;

	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT%XSYSTEMMODE?", "%%XSYSTEMMODE: %d,%d,%d,%d", 4);
	__mock_nrf_modem_at_scanf_ReturnVarg_int(1); /* ltem_mode */
	__mock_nrf_modem_at_scanf_ReturnVarg_int(0); /* nbiot_mode */
	__mock_nrf_modem_at_scanf_ReturnVarg_int(1); /* gps_mode */
	__mock_nrf_modem_at_scanf_ReturnVarg_int(0); /* mode_preference */

	ret = lte_lc_rai_req(true);
	TEST_ASSERT_EQUAL(-EOPNOTSUPP, ret);
}

void test_lte_lc_rai_req_fail2(void)
{
	int ret;

	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT%XSYSTEMMODE?", "%%XSYSTEMMODE: %d,%d,%d,%d", 4);
	__mock_nrf_modem_at_scanf_ReturnVarg_int(0); /* ltem_mode */
	__mock_nrf_modem_at_scanf_ReturnVarg_int(0); /* nbiot_mode */
	__mock_nrf_modem_at_scanf_ReturnVarg_int(0); /* gps_mode */
	__mock_nrf_modem_at_scanf_ReturnVarg_int(0); /* mode_preference */

	ret = lte_lc_rai_req(true);
	TEST_ASSERT_EQUAL(-EOPNOTSUPP, ret);
}

void test_lte_lc_rai_req_enable_success(void)
{
	int ret;

	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT%XSYSTEMMODE?", "%%XSYSTEMMODE: %d,%d,%d,%d", 4);
	__mock_nrf_modem_at_scanf_ReturnVarg_int(1); /* ltem_mode */
	__mock_nrf_modem_at_scanf_ReturnVarg_int(1); /* nbiot_mode */
	__mock_nrf_modem_at_scanf_ReturnVarg_int(1); /* gps_mode */
	__mock_nrf_modem_at_scanf_ReturnVarg_int(0); /* mode_preference */

	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT%%XRAI=%s", EXIT_SUCCESS);
	ret = lte_lc_rai_req(true);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_rai_req_disable_success(void)
{
	int ret;

	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT%XSYSTEMMODE?", "%%XSYSTEMMODE: %d,%d,%d,%d", 4);
	__mock_nrf_modem_at_scanf_ReturnVarg_int(1); /* ltem_mode */
	__mock_nrf_modem_at_scanf_ReturnVarg_int(1); /* nbiot_mode */
	__mock_nrf_modem_at_scanf_ReturnVarg_int(1); /* gps_mode */
	__mock_nrf_modem_at_scanf_ReturnVarg_int(0); /* mode_preference */

	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT%%XRAI=0", EXIT_SUCCESS);
	ret = lte_lc_rai_req(false);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_rai_param_set_null(void)
{
	int ret;

	ret = lte_lc_rai_param_set(NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

void test_lte_lc_rai_param_wrong_value(void)
{
	int ret;

	ret = lte_lc_rai_param_set("1");
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

void test_lte_lc_rai_param_success(void)
{
	int ret;

	ret = lte_lc_rai_param_set("3");
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
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

	int modes[8][3] = {
		{0, 0, 0},
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
		__mock_nrf_modem_at_scanf_ReturnVarg_int(0); /* ltem_mode */
		__mock_nrf_modem_at_scanf_ReturnVarg_int(0); /* nbiot_mode */
		__mock_nrf_modem_at_scanf_ReturnVarg_int(0); /* gps_mode */
		__mock_nrf_modem_at_scanf_ReturnVarg_int(i); /* mode_preference */

		ret = lte_lc_system_mode_get(&mode, &preference);
		TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	}
}

void test_lte_lc_func_mode_set_all_modes(void)
{
	int ret;

	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT+CEREG=5", EXIT_SUCCESS);
	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT+CSCON=1", EXIT_SUCCESS);
	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT+CFUN=%d", EXIT_SUCCESS);
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
		__cmock_nrf_modem_at_printf_ExpectAndReturn("AT+CFUN=%d", EXIT_SUCCESS);
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

void test_lte_lc_neighbor_cell_measurement_cancel(void)
{
	int ret;

	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT%%NCELLMEASSTOP", EXIT_SUCCESS);
	ret = lte_lc_neighbor_cell_measurement_cancel();
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT%%NCELLMEASSTOP", -NRF_ENOMEM);
	ret = lte_lc_neighbor_cell_measurement_cancel();
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_modem_events_enable(void)
{
	int ret;

	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT%%MDMEV=1", EXIT_SUCCESS);
	ret = lte_lc_modem_events_enable();
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT%%MDMEV=1", -NRF_ENOMEM);
	ret = lte_lc_modem_events_enable();
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_modem_events_disable(void)
{
	int ret;

	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT%%MDMEV=0", EXIT_SUCCESS);
	ret = lte_lc_modem_events_disable();
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT%%MDMEV=0", -NRF_ENOMEM);
	ret = lte_lc_modem_events_disable();
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_periodic_search_request(void)
{
	int ret;

	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT%%PERIODICSEARCHCONF=3", EXIT_SUCCESS);
	ret = lte_lc_periodic_search_request();
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT%%PERIODICSEARCHCONF=3", -NRF_ENOMEM);
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
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(0);

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
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(1); /* result */

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
	struct lte_lc_periodic_search_cfg cfg;

	cfg.pattern_count = 1;
	cfg.patterns[0].type = 0;
	cfg.patterns[0].range.initial_sleep = 60;
	cfg.patterns[0].range.final_sleep = 3600;
	cfg.patterns[0].range.time_to_final_sleep = 300;
	cfg.patterns[0].range.pattern_end_point = 600;

	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT%%PERIODICSEARCHCONF=0,"
						   "%hu,%hu,%hu,%s%s%s%s%s%s%s",
						   EXIT_SUCCESS);

	ret = lte_lc_periodic_search_set(&cfg);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_periodic_search_set_at_fail(void)
{
	int ret;
	struct lte_lc_periodic_search_cfg cfg;

	cfg.pattern_count = 1;

	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT%%PERIODICSEARCHCONF=0,"
						   "%hu,%hu,%hu,%s%s%s%s%s%s%s",
						   -NRF_ENOMEM);

	ret = lte_lc_periodic_search_set(&cfg);
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_periodic_search_set_at_cme(void)
{
	int ret;
	struct lte_lc_periodic_search_cfg cfg;

	cfg.pattern_count = 1;

	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT%%PERIODICSEARCHCONF=0,"
						   "%hu,%hu,%hu,%s%s%s%s%s%s%s",
						   1);

	ret = lte_lc_periodic_search_set(&cfg);
	TEST_ASSERT_EQUAL(-EBADMSG, ret);
}

void test_lte_lc_periodic_search_clear_success(void)
{
	int ret;

	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT%%PERIODICSEARCHCONF=2",
						   EXIT_SUCCESS);

	ret = lte_lc_periodic_search_clear();
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_periodic_search_clear_at_fail(void)
{
	int ret;

	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT%%PERIODICSEARCHCONF=2",
						   -NRF_ENOMEM);

	ret = lte_lc_periodic_search_clear();
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_periodic_search_clear_cme(void)
{
	int ret;

	__cmock_nrf_modem_at_printf_ExpectAndReturn("AT%%PERIODICSEARCHCONF=2",
						   1);

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
	TEST_ASSERT_EQUAL(cfg.loop, 0);
	TEST_ASSERT_EQUAL(cfg.return_to_pattern, 0);
	TEST_ASSERT_EQUAL(cfg.band_optimization, 10);
	TEST_ASSERT_EQUAL(cfg.patterns[0].type, LTE_LC_PERIODIC_SEARCH_PATTERN_RANGE);
	TEST_ASSERT_EQUAL(cfg.patterns[0].range.initial_sleep, 60);
	TEST_ASSERT_EQUAL(cfg.patterns[0].range.final_sleep, 3600);
	TEST_ASSERT_EQUAL(cfg.patterns[0].range.time_to_final_sleep, -1);
	TEST_ASSERT_EQUAL(cfg.patterns[0].range.pattern_end_point, 600);
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
