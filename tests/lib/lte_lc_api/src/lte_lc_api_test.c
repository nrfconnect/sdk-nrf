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
#include "lte_lc_helpers.h"
#include <mock_nrf_modem_at.h>

enum nrf_modem_at_scanf_variant_t {
	INVALID = 0,
	CEREG_lte_lc_nw_reg_status_get,
	CEREG_lte_lc_lte_mode_get,
	XSYSTEMMODE_lte_lc_system_mode_get,
	CFUN_lte_lc_func_mode_get,
	CONEVAL_lte_lc_conn_eval_params_get,
	PERIODICSEARCHCONF_lte_lc_periodic_search_get,
	AT_SCANF_FAIL,
};

struct nrf_modem_at_scanf_return_t {
	int ltem_mode;
	int nbiot_mode;
	int gps_mode;
	int mode_preference;
	uint16_t status;
	uint32_t cell_id;
	uint16_t mode;
	int return_code;
	int coneval_result;
	const char *plmn_str;
	struct lte_lc_conn_eval_params coneval;
	uint16_t loop;
	uint16_t return_to_pattern;
	uint16_t band_optimization;
	char *pattern_buf[4];
};

static enum nrf_modem_at_scanf_variant_t nrf_modem_at_scanf_variant[5];
static struct nrf_modem_at_scanf_return_t nrf_modem_at_scanf_return = {
	.plmn_str = "",
	.pattern_buf = {"", "", "", ""},
};

int __wrap_nrf_modem_at_scanf(const char *cmd, const char *fmt, ...)
{
	va_list args;
	int result = -EPERM;

	switch (nrf_modem_at_scanf_variant[0]) {

	case XSYSTEMMODE_lte_lc_system_mode_get:
		TEST_ASSERT_EQUAL_STRING("AT%XSYSTEMMODE?", cmd);
		TEST_ASSERT_EQUAL_STRING("%%XSYSTEMMODE: %d,%d,%d,%d", fmt);

		va_start(args, fmt);

		int *ltem_mode = va_arg(args, int *);
		int *nbiot_mode = va_arg(args, int *);
		int *gps_mode = va_arg(args, int *);
		int *mode_preference = va_arg(args, int *);

		va_end(args);

		*ltem_mode = nrf_modem_at_scanf_return.ltem_mode;
		*nbiot_mode = nrf_modem_at_scanf_return.nbiot_mode;
		*gps_mode = nrf_modem_at_scanf_return.gps_mode;
		*mode_preference = nrf_modem_at_scanf_return.mode_preference;

		result = 4;
		break;
	case CEREG_lte_lc_nw_reg_status_get:
		TEST_ASSERT_EQUAL_STRING("AT+CEREG?", cmd);
		TEST_ASSERT_EQUAL_STRING("+CEREG: %*u,%hu,%*[^,],\"%x\",", fmt);

		va_start(args, fmt);

		uint16_t *status = va_arg(args, uint16_t *);
		uint32_t *cell_id = va_arg(args, uint32_t *);

		va_end(args);

		*status = nrf_modem_at_scanf_return.status;
		*cell_id = nrf_modem_at_scanf_return.cell_id;

		result = 2;
		break;
	case CFUN_lte_lc_func_mode_get:
		TEST_ASSERT_EQUAL_STRING("AT+CFUN?", cmd);
		TEST_ASSERT_EQUAL_STRING("+CFUN: %hu", fmt);

		va_start(args, fmt);

		uint16_t *mode = va_arg(args, uint16_t *);

		va_end(args);

		*mode = nrf_modem_at_scanf_return.mode;

		result = 1;
		break;

	case AT_SCANF_FAIL:
		result = nrf_modem_at_scanf_return.return_code;
		break;

	case CEREG_lte_lc_lte_mode_get:
		TEST_ASSERT_EQUAL_STRING("AT+CEREG?", cmd);
		TEST_ASSERT_EQUAL_STRING("+CEREG: %*u,%*u,%*[^,],%*[^,],%hu", fmt);

		va_start(args, fmt);
		mode = va_arg(args, uint16_t *);
		va_end(args);

		*mode = nrf_modem_at_scanf_return.mode;

		result = 1;
		break;

	case CONEVAL_lte_lc_conn_eval_params_get:
		TEST_ASSERT_EQUAL_STRING("AT%CONEVAL", cmd);
		TEST_ASSERT_EQUAL_STRING(
			"%%CONEVAL: "
			"%d,"
			"%hu,"
			"%hu,"
			"%hd,"
			"%hd,"
			"%hd,"
			"\"%x\","
			"\"%6[^\"]\","
			"%hd,"
			"%d,"
			"%hd,"
			"%hu,"
			"%hu,"
			"%hd,"
			"%hd,"
			"%hd,"
			"%hd",
		fmt);

		va_start(args, fmt);

		int *coneval_result = va_arg(args, int *);
		uint16_t *rrc_state = va_arg(args, uint16_t *);
		uint16_t *energy_estimate = va_arg(args, uint16_t *);
		int16_t *rsrp = va_arg(args, int16_t *);
		int16_t *rsrq = va_arg(args, int16_t *);
		int16_t *snr = va_arg(args, int16_t *);

		cell_id = va_arg(args, uint32_t *);

		char *plmn_str = va_arg(args, char *);
		int16_t *phy_cid = va_arg(args, int16_t *);
		int *earfcn = va_arg(args, int *);
		int16_t *band = va_arg(args, int16_t *);
		uint16_t *tau_trig = va_arg(args, uint16_t *);
		uint16_t *ce_level = va_arg(args, uint16_t *);
		int16_t *tx_power = va_arg(args, int16_t *);
		int16_t *tx_rep = va_arg(args, int16_t *);
		int16_t *rx_rep = va_arg(args, int16_t *);
		int16_t *dl_pathloss = va_arg(args, int16_t *);


		va_end(args);

		*coneval_result = nrf_modem_at_scanf_return.coneval_result;
		*rrc_state = nrf_modem_at_scanf_return.coneval.rrc_state;
		*energy_estimate = nrf_modem_at_scanf_return.coneval.energy_estimate;
		*rsrp = nrf_modem_at_scanf_return.coneval.rsrp;
		*rsrq = nrf_modem_at_scanf_return.coneval.rsrq;
		*snr = nrf_modem_at_scanf_return.coneval.snr;
		*cell_id = nrf_modem_at_scanf_return.coneval.cell_id;
		strcpy(plmn_str, nrf_modem_at_scanf_return.plmn_str);
		*phy_cid = nrf_modem_at_scanf_return.coneval.phy_cid;
		*earfcn = nrf_modem_at_scanf_return.coneval.earfcn;
		*band = nrf_modem_at_scanf_return.coneval.band;
		*tau_trig = nrf_modem_at_scanf_return.coneval.tau_trig;
		*ce_level = nrf_modem_at_scanf_return.coneval.ce_level;
		*tx_power = nrf_modem_at_scanf_return.coneval.tx_power;
		*tx_rep = nrf_modem_at_scanf_return.coneval.tx_rep;
		*rx_rep = nrf_modem_at_scanf_return.coneval.rx_rep;
		*dl_pathloss = nrf_modem_at_scanf_return.coneval.dl_pathloss;

		result = 17;
		break;

	case PERIODICSEARCHCONF_lte_lc_periodic_search_get:
		TEST_ASSERT_EQUAL_STRING("AT%PERIODICSEARCHCONF=1", cmd);
		TEST_ASSERT_EQUAL_STRING("%%PERIODICSEARCHCONF: "
		"%hu,%hu,%hu,"
		"\"%40[^\"]\",\"%40[^\"]\",\"%40[^\"]\",\"%40[^\"]\"", fmt);

		char *pattern_buf[4];

		va_start(args, fmt);

		uint16_t *loop = va_arg(args, uint16_t *);
		uint16_t *return_to_pattern = va_arg(args, uint16_t *);
		uint16_t *band_optimization = va_arg(args, uint16_t *);

		pattern_buf[0] = va_arg(args, char *);
		pattern_buf[1] = va_arg(args, char *);
		pattern_buf[2] = va_arg(args, char *);
		pattern_buf[3] = va_arg(args, char *);

		va_end(args);

		*loop = nrf_modem_at_scanf_return.loop;
		*return_to_pattern = nrf_modem_at_scanf_return.return_to_pattern;
		*band_optimization = nrf_modem_at_scanf_return.band_optimization;
		strcpy(pattern_buf[0], nrf_modem_at_scanf_return.pattern_buf[0]);
		strcpy(pattern_buf[1], nrf_modem_at_scanf_return.pattern_buf[1]);
		strcpy(pattern_buf[2], nrf_modem_at_scanf_return.pattern_buf[2]);
		strcpy(pattern_buf[3], nrf_modem_at_scanf_return.pattern_buf[3]);

		result = 4;
		break;
	default:
		/* function called more often than expected */
		TEST_FAIL_MESSAGE("nrf_modem_at_scanf called more times than expected");
		return -EPERM;
	}

	/* advance queue */
	for (size_t i = 1; i < ARRAY_SIZE(nrf_modem_at_scanf_variant); ++i) {
		nrf_modem_at_scanf_variant[i-1] = nrf_modem_at_scanf_variant[i];
	}
	return result;
}

void wrap_lc_init(void)
{
	nrf_modem_at_scanf_return.ltem_mode = 1;
	nrf_modem_at_scanf_return.gps_mode = 1;
	nrf_modem_at_scanf_return.nbiot_mode = 1;
	nrf_modem_at_scanf_return.mode_preference = 0;
	nrf_modem_at_scanf_variant[0] = XSYSTEMMODE_lte_lc_system_mode_get;
	nrf_modem_at_scanf_variant[1] = INVALID;

	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT%%XSYSTEMMODE=%s,%c", EXIT_SUCCESS);
	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT+CEREG=5", EXIT_SUCCESS);
	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT+CSCON=1", EXIT_SUCCESS);
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

	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT+CFUN=%d", EXIT_SUCCESS);

	TEST_ASSERT_EQUAL_MESSAGE(INVALID, nrf_modem_at_scanf_variant[0],
		"nrf_modem_at_scanf called fewer times than expected");
	ret = lte_lc_deinit();
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	mock_nrf_modem_at_Verify();
}

/* This is needed because AT Monitor library is initialized in SYS_INIT. */
static int sys_init_helper(const struct device *unused)
{
	__wrap_nrf_modem_at_notif_handler_set_ExpectAnyArgsAndReturn(0);

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

	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT+CFUN=%d", EXIT_SUCCESS);
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

	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT+CFUN=%d", EXIT_SUCCESS);
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

	nrf_modem_at_scanf_variant[0] = CEREG_lte_lc_nw_reg_status_get;
	nrf_modem_at_scanf_variant[1] = INVALID;
	nrf_modem_at_scanf_return.cell_id = 0;
	nrf_modem_at_scanf_return.status = LTE_LC_NW_REG_REGISTERED_HOME;

	ret = lte_lc_connect();
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_init_and_connect_home_already_registered(void)
{
	int ret;

	nrf_modem_at_scanf_variant[0] = CEREG_lte_lc_nw_reg_status_get;
	nrf_modem_at_scanf_variant[1] = INVALID;
	nrf_modem_at_scanf_return.cell_id = 0;
	nrf_modem_at_scanf_return.status = LTE_LC_NW_REG_REGISTERED_HOME;

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

	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT+CEREG=5", EXIT_SUCCESS);
	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT+CSCON=1", EXIT_SUCCESS);
	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT+CFUN=%d", EXIT_SUCCESS);
	ret = lte_lc_normal();
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_normal_fail1(void)
{
	int ret;

	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT+CEREG=5", -NRF_ENOMEM);
	ret = lte_lc_normal();
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_normal_fail2(void)
{
	int ret;

	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT+CEREG=5", EXIT_SUCCESS);
	/* enable_notifications */
	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT+CSCON=1", -NRF_ENOMEM);
	__wrap_nrf_modem_at_cmd_ExpectAnyArgsAndReturn(-NRF_ENOMEM);
	/* error is ignored */
	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT+CFUN=%d", EXIT_SUCCESS);


	ret = lte_lc_normal();
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_normal_fail3(void)
{
	int ret;

	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT+CEREG=5", EXIT_SUCCESS);
	/* enable_notifications */
	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT+CSCON=1", -NRF_ENOMEM);
	__wrap_nrf_modem_at_cmd_ExpectAnyArgsAndReturn(EXIT_SUCCESS);
	/* error is ignored */
	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT+CFUN=%d", EXIT_SUCCESS);


	ret = lte_lc_normal();
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_normal_fail4(void)
{
	int ret;

	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT+CEREG=5", EXIT_SUCCESS);
	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT+CSCON=1", EXIT_SUCCESS);
	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT+CFUN=%d", -NRF_ENOMEM);
	ret = lte_lc_normal();
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_offline_success(void)
{
	int ret;

	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT+CFUN=%d", EXIT_SUCCESS);
	ret = lte_lc_offline();
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_offline_fail(void)
{
	int ret;

	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT+CFUN=%d", -NRF_ENOMEM);
	ret = lte_lc_offline();
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_power_off_success(void)
{
	int ret;

	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT+CFUN=%d", EXIT_SUCCESS);
	ret = lte_lc_power_off();
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_power_off_fail(void)
{
	int ret;

	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT+CFUN=%d", -NRF_ENOMEM);
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

	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT+CPSMS=1", EXIT_SUCCESS);
	ret = lte_lc_psm_req(true);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_psm_req_enable_rptau_default(void)
{
	int ret;

	ret = lte_lc_psm_param_set(NULL, "01001001");
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT+CPSMS=1,,,,\"%s\"", EXIT_SUCCESS);
	ret = lte_lc_psm_req(true);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_psm_req_enable_rat_default(void)
{
	int ret;

	ret = lte_lc_psm_param_set("00000111", NULL);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT+CPSMS=1,,,\"%s\"", EXIT_SUCCESS);
	ret = lte_lc_psm_req(true);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_psm_req_enable_both_set(void)
{
	int ret;

	ret = lte_lc_psm_param_set("00000111", "01001001");
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT+CPSMS=1,,,\"%s\",\"%s\"", EXIT_SUCCESS);
	ret = lte_lc_psm_req(true);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_psm_req_enable_fail(void)
{
	int ret;

	ret = lte_lc_psm_param_set(NULL, NULL);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT+CPSMS=1", NRF_MODEM_AT_ERROR);
	ret = lte_lc_psm_req(true);
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_psm_req_disable_success(void)
{
	int ret;

	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT+CPSMS=", EXIT_SUCCESS);
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

	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT+CEDRXS=3", -NRF_ENOMEM);
	ret = lte_lc_edrx_req(false);
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_edrx_req_disable_success(void)
{
	int ret;

	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT+CEDRXS=3", EXIT_SUCCESS);
	ret = lte_lc_edrx_req(false);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_edrx_req_enable_fail1(void)
{
	int ret;

	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT+CEDRXS=2,%d,\"%s\"", -NRF_ENOMEM);
	ret = lte_lc_edrx_req(true);
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_edrx_req_enable_fail2(void)
{
	int ret;

	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT+CEDRXS=2,%d,\"%s\"", EXIT_SUCCESS);
	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT%%XPTW=%d,\"%s\"", -NRF_ENOMEM);
	ret = lte_lc_edrx_req(true);
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_edrx_req_enable_fail3(void)
{
	int ret;

	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT+CEDRXS=2,%d,\"%s\"", EXIT_SUCCESS);
	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT%%XPTW=%d,\"%s\"", EXIT_SUCCESS);
	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT+CEDRXS=2,%d,\"%s\"", -NRF_ENOMEM);
	ret = lte_lc_edrx_req(true);
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_edrx_req_enable_success(void)
{
	int ret;

	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT+CEDRXS=2,%d,\"%s\"", EXIT_SUCCESS);
	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT%%XPTW=%d,\"%s\"", EXIT_SUCCESS);
	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT+CEDRXS=2,%d,\"%s\"", EXIT_SUCCESS);
	ret = lte_lc_edrx_req(true);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_rai_req_fail1(void)
{
	int ret;

	nrf_modem_at_scanf_return.ltem_mode = 1;
	nrf_modem_at_scanf_return.gps_mode = 1;
	nrf_modem_at_scanf_return.nbiot_mode = 0;
	nrf_modem_at_scanf_return.mode_preference = 0;
	nrf_modem_at_scanf_variant[0] = XSYSTEMMODE_lte_lc_system_mode_get;
	nrf_modem_at_scanf_variant[1] = INVALID;

	ret = lte_lc_rai_req(true);
	TEST_ASSERT_EQUAL(-EOPNOTSUPP, ret);
}

void test_lte_lc_rai_req_fail2(void)
{
	int ret;

	nrf_modem_at_scanf_return.ltem_mode = 0;
	nrf_modem_at_scanf_return.gps_mode = 0;
	nrf_modem_at_scanf_return.nbiot_mode = 0;
	nrf_modem_at_scanf_return.mode_preference = 0;
	nrf_modem_at_scanf_variant[0] = XSYSTEMMODE_lte_lc_system_mode_get;
	nrf_modem_at_scanf_variant[1] = INVALID;

	ret = lte_lc_rai_req(true);
	TEST_ASSERT_EQUAL(-EOPNOTSUPP, ret);
}

void test_lte_lc_rai_req_enable_success(void)
{
	int ret;

	nrf_modem_at_scanf_return.ltem_mode = 1;
	nrf_modem_at_scanf_return.gps_mode = 1;
	nrf_modem_at_scanf_return.nbiot_mode = 1;
	nrf_modem_at_scanf_return.mode_preference = 0;
	nrf_modem_at_scanf_variant[0] = XSYSTEMMODE_lte_lc_system_mode_get;
	nrf_modem_at_scanf_variant[1] = INVALID;

	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT%%XRAI=%s", EXIT_SUCCESS);
	ret = lte_lc_rai_req(true);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_rai_req_disable_success(void)
{
	int ret;

	nrf_modem_at_scanf_return.ltem_mode = 1;
	nrf_modem_at_scanf_return.gps_mode = 1;
	nrf_modem_at_scanf_return.nbiot_mode = 1;
	nrf_modem_at_scanf_return.mode_preference = 0;
	nrf_modem_at_scanf_variant[0] = XSYSTEMMODE_lte_lc_system_mode_get;
	nrf_modem_at_scanf_variant[1] = INVALID;

	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT%%XRAI=0", EXIT_SUCCESS);
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

	nrf_modem_at_scanf_return.return_code = -NRF_ENOMEM;
	nrf_modem_at_scanf_variant[0] = AT_SCANF_FAIL;
	nrf_modem_at_scanf_variant[1] = INVALID;

	ret = lte_lc_func_mode_get(&mode);
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}


void test_lte_lc_func_mode_get_success(void)
{
	int ret;
	enum lte_lc_func_mode mode;

	nrf_modem_at_scanf_return.mode = 1;
	nrf_modem_at_scanf_variant[0] = CFUN_lte_lc_func_mode_get;
	nrf_modem_at_scanf_variant[1] = INVALID;


	ret = lte_lc_func_mode_get(&mode);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	TEST_ASSERT_EQUAL(nrf_modem_at_scanf_return.mode, mode);
}

void test_lte_lc_system_mode_get_all_modes(void)
{
	int ret;
	enum lte_lc_system_mode mode;
	enum lte_lc_system_mode_preference preference;

	nrf_modem_at_scanf_variant[1] = INVALID;

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
		nrf_modem_at_scanf_return.ltem_mode = modes[i][0];
		nrf_modem_at_scanf_return.nbiot_mode = modes[i][1];
		nrf_modem_at_scanf_return.gps_mode = modes[i][2];
		nrf_modem_at_scanf_return.mode_preference = 0;
		nrf_modem_at_scanf_variant[0] = XSYSTEMMODE_lte_lc_system_mode_get;
		ret = lte_lc_system_mode_get(&mode, &preference);
		TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	}
}

void test_lte_lc_system_mode_get_all_preferences(void)
{
	int ret;
	enum lte_lc_system_mode mode;
	enum lte_lc_system_mode_preference preference;

	nrf_modem_at_scanf_variant[1] = INVALID;

	for (int i = 0; i < 5; ++i) {
		nrf_modem_at_scanf_return.ltem_mode = 0;
		nrf_modem_at_scanf_return.nbiot_mode = 0;
		nrf_modem_at_scanf_return.gps_mode = 0;
		nrf_modem_at_scanf_return.mode_preference = i;
		nrf_modem_at_scanf_variant[0] = XSYSTEMMODE_lte_lc_system_mode_get;
		ret = lte_lc_system_mode_get(&mode, &preference);
		TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	}
}

void test_lte_lc_func_mode_set_all_modes(void)
{
	int ret;

	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT+CEREG=5", EXIT_SUCCESS);
	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT+CSCON=1", EXIT_SUCCESS);
	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT+CFUN=%d", EXIT_SUCCESS);
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
		__wrap_nrf_modem_at_printf_ExpectAndReturn("AT+CFUN=%d", EXIT_SUCCESS);
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

	nrf_modem_at_scanf_variant[1] = INVALID;
	nrf_modem_at_scanf_variant[0] = AT_SCANF_FAIL;
	nrf_modem_at_scanf_return.return_code = -NRF_ENOMEM;

	ret = lte_lc_lte_mode_get(&mode);
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_lte_mode_get_badmsg(void)
{
	int ret;
	enum lte_lc_lte_mode mode;

	nrf_modem_at_scanf_variant[1] = INVALID;
	nrf_modem_at_scanf_variant[0] = AT_SCANF_FAIL;
	nrf_modem_at_scanf_return.return_code = -NRF_EBADMSG;

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
		nrf_modem_at_scanf_variant[1] = INVALID;
		nrf_modem_at_scanf_variant[0] = CEREG_lte_lc_lte_mode_get;
		nrf_modem_at_scanf_return.mode = modes[i];

		ret = lte_lc_lte_mode_get(&mode);
		TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
		TEST_ASSERT_EQUAL(modes[i], mode);
	}
}

void test_lte_lc_neighbor_cell_measurement_cancel(void)
{
	int ret;

	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT%%NCELLMEASSTOP", EXIT_SUCCESS);
	ret = lte_lc_neighbor_cell_measurement_cancel();
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT%%NCELLMEASSTOP", -NRF_ENOMEM);
	ret = lte_lc_neighbor_cell_measurement_cancel();
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_modem_events_enable(void)
{
	int ret;

	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT%%MDMEV=1", EXIT_SUCCESS);
	ret = lte_lc_modem_events_enable();
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT%%MDMEV=1", -NRF_ENOMEM);
	ret = lte_lc_modem_events_enable();
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_modem_events_disable(void)
{
	int ret;

	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT%%MDMEV=0", EXIT_SUCCESS);
	ret = lte_lc_modem_events_disable();
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT%%MDMEV=0", -NRF_ENOMEM);
	ret = lte_lc_modem_events_disable();
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_periodic_search_request(void)
{
	int ret;

	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT%%PERIODICSEARCHCONF=3", EXIT_SUCCESS);
	ret = lte_lc_periodic_search_request();
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT%%PERIODICSEARCHCONF=3", -NRF_ENOMEM);
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

	nrf_modem_at_scanf_variant[0] = AT_SCANF_FAIL;
	nrf_modem_at_scanf_variant[1] = INVALID;

	nrf_modem_at_scanf_return.return_code = -NRF_ENOMEM;

	ret = lte_lc_conn_eval_params_get(&params);
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_conn_eval_params_wrong_func_mode(void)
{
	int ret;
	struct lte_lc_conn_eval_params params;

	nrf_modem_at_scanf_variant[0] = CFUN_lte_lc_func_mode_get;
	nrf_modem_at_scanf_variant[1] = INVALID;

	nrf_modem_at_scanf_return.mode = LTE_LC_FUNC_MODE_OFFLINE;

	ret = lte_lc_conn_eval_params_get(&params);
	TEST_ASSERT_EQUAL(-EOPNOTSUPP, ret);
}

void test_lte_lc_conn_eval_params_at_fail(void)
{
	int ret;
	struct lte_lc_conn_eval_params params;

	nrf_modem_at_scanf_variant[0] = CFUN_lte_lc_func_mode_get;
	nrf_modem_at_scanf_variant[1] = AT_SCANF_FAIL;
	nrf_modem_at_scanf_variant[2] = INVALID;

	nrf_modem_at_scanf_return.mode = LTE_LC_FUNC_MODE_NORMAL;
	nrf_modem_at_scanf_return.return_code = -NRF_ENOMEM;

	ret = lte_lc_conn_eval_params_get(&params);
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_conn_eval_params_coneval_fail(void)
{
	int ret;
	struct lte_lc_conn_eval_params params;

	nrf_modem_at_scanf_variant[0] = CFUN_lte_lc_func_mode_get;
	nrf_modem_at_scanf_variant[1] = CONEVAL_lte_lc_conn_eval_params_get;
	nrf_modem_at_scanf_variant[2] = INVALID;

	nrf_modem_at_scanf_return.mode = LTE_LC_FUNC_MODE_NORMAL;
	nrf_modem_at_scanf_return.coneval_result = 1;

	ret = lte_lc_conn_eval_params_get(&params);
	TEST_ASSERT_EQUAL(1, ret);
}

void test_lte_lc_conn_eval_params_coneval_success(void)
{
	int ret;
	struct lte_lc_conn_eval_params params;

	nrf_modem_at_scanf_variant[0] = CFUN_lte_lc_func_mode_get;
	nrf_modem_at_scanf_variant[1] = CONEVAL_lte_lc_conn_eval_params_get;
	nrf_modem_at_scanf_variant[2] = INVALID;

	nrf_modem_at_scanf_return.mode = LTE_LC_FUNC_MODE_NORMAL;
	nrf_modem_at_scanf_return.coneval_result = 0;
	nrf_modem_at_scanf_return.coneval.rrc_state = 1;
	nrf_modem_at_scanf_return.coneval.energy_estimate = 8;
	nrf_modem_at_scanf_return.coneval.tau_trig = 0;
	nrf_modem_at_scanf_return.coneval.ce_level = 0;
	nrf_modem_at_scanf_return.plmn_str = "26295";

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

	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT%%PERIODICSEARCHCONF=0,"
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

	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT%%PERIODICSEARCHCONF=0,"
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

	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT%%PERIODICSEARCHCONF=0,"
						   "%hu,%hu,%hu,%s%s%s%s%s%s%s",
						   1);

	ret = lte_lc_periodic_search_set(&cfg);
	TEST_ASSERT_EQUAL(-EBADMSG, ret);
}

void test_lte_lc_periodic_search_clear_success(void)
{
	int ret;

	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT%%PERIODICSEARCHCONF=2",
						   EXIT_SUCCESS);

	ret = lte_lc_periodic_search_clear();
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

void test_lte_lc_periodic_search_clear_at_fail(void)
{
	int ret;

	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT%%PERIODICSEARCHCONF=2",
						   -NRF_ENOMEM);

	ret = lte_lc_periodic_search_clear();
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_periodic_search_clear_cme(void)
{
	int ret;

	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT%%PERIODICSEARCHCONF=2",
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

	nrf_modem_at_scanf_variant[0] = AT_SCANF_FAIL;
	nrf_modem_at_scanf_variant[1] = INVALID;

	nrf_modem_at_scanf_return.return_code = -NRF_ENOMEM;

	ret = lte_lc_periodic_search_get(&cfg);
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_periodic_search_get_badmsg(void)
{
	int ret;
	struct lte_lc_periodic_search_cfg cfg;

	nrf_modem_at_scanf_variant[0] = AT_SCANF_FAIL;
	nrf_modem_at_scanf_variant[1] = INVALID;

	nrf_modem_at_scanf_return.return_code = -NRF_EBADMSG;

	ret = lte_lc_periodic_search_get(&cfg);
	TEST_ASSERT_EQUAL(-ENOENT, ret);
}

void test_lte_lc_periodic_search_get_no_pattern(void)
{
	int ret;
	struct lte_lc_periodic_search_cfg cfg;

	nrf_modem_at_scanf_variant[0] = AT_SCANF_FAIL;
	nrf_modem_at_scanf_variant[1] = INVALID;

	nrf_modem_at_scanf_return.return_code = 0;

	ret = lte_lc_periodic_search_get(&cfg);
	TEST_ASSERT_EQUAL(-EBADMSG, ret);
}

void test_lte_lc_periodic_search_get_success(void)
{
	int ret;
	struct lte_lc_periodic_search_cfg cfg;

	nrf_modem_at_scanf_variant[0] = PERIODICSEARCHCONF_lte_lc_periodic_search_get;
	nrf_modem_at_scanf_variant[1] = INVALID;

	nrf_modem_at_scanf_return.loop = 0;
	nrf_modem_at_scanf_return.return_to_pattern = 0;
	nrf_modem_at_scanf_return.band_optimization = 10;
	nrf_modem_at_scanf_return.pattern_buf[0] = "0,60,3600,,600";

	ret = lte_lc_periodic_search_get(&cfg);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

/* It is required to be added to each test. That is because unity is using
 * different main signature (returns int) and zephyr expects main which does
 * not return value.
 */
extern int unity_main(void);

void main(void)
{
	(void)unity_main();
}

SYS_INIT(sys_init_helper, POST_KERNEL, 0);
