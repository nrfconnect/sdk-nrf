/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <unity.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/conn_mgr_connectivity.h>
#include <zephyr/device.h>

/* UUT header file */
#include "lte_connectivity.h"

/* Mocked libraries */
#include "cmock_lte_lc.h"
#include "cmock_nrf_modem_lib.h"
#include "mock_nrf_modem_at.h"
#include "cmock_nrf_modem_at.h"
#include "cmock_nrf_modem.h"
#include "cmock_pdn.h"

extern int unity_main(void);

static pdn_event_handler_t pdn_event_handler_callback;
static lte_lc_evt_handler_t lte_lc_event_handler_callback;

/* Initial condition tracking */
static bool initial_conditions_observed;
static bool initial_persistence;
static bool initial_auto_connect;
static bool initial_auto_down;
static int initial_connect_timeout;

/* Stub used to register test local PDN event handler used to invoke PDN event in UUT. */
static int pdn_default_ctx_cb_reg_stub(pdn_event_handler_t cb, int num_of_calls)
{
	pdn_event_handler_callback = cb;
	return 0;
}

/* Stub used to register test local CEREG event handler used to invoke CEREG event in UUT. */
static void lte_lc_register_handler_stub(lte_lc_evt_handler_t cb, int num_of_calls)
{
	lte_lc_event_handler_callback = cb;
}

static void bring_network_interface_up(void)
{
	struct net_if *net_if = net_if_get_default();

	__cmock_nrf_modem_is_initialized_ExpectAndReturn(1);
	__cmock_lte_lc_init_ExpectAndReturn(0);

	TEST_ASSERT_EQUAL(0, net_if_up(net_if));
}

static void network_interface_down_option_set(enum lte_connectivity_if_down_options value,
					      int retval_expected)
{
	uint8_t option = value;

	int ret = conn_mgr_if_set_opt(net_if_get_default(),
				      LTE_CONNECTIVITY_IF_DOWN,
				      (const void *)&option,
				      sizeof(option));

	TEST_ASSERT_EQUAL(retval_expected, ret);
}

/* Ensure consistent starting conditions for each test. */
void setUp(void)
{
	struct net_if *net_if = net_if_get_default();

	/* Set up dummy CEREG event struct. */
	struct lte_lc_evt cereg_evt = {
		.type = LTE_LC_EVT_NW_REG_STATUS,
		.nw_reg_status = LTE_LC_NW_REG_UNKNOWN,
	};

	/* Observe initial conditions */
	if (!initial_conditions_observed) {
		initial_conditions_observed = true;
		initial_connect_timeout = conn_mgr_if_get_timeout(net_if);
		initial_persistence = conn_mgr_if_get_flag(net_if, CONN_MGR_IF_PERSISTENT);
		initial_auto_down = !conn_mgr_if_get_flag(net_if, CONN_MGR_IF_NO_AUTO_DOWN);
		initial_auto_connect = !conn_mgr_if_get_flag(net_if, CONN_MGR_IF_NO_AUTO_CONNECT);
	}

	/* Temporarily ignore and provide default values to mocked functions */
	__cmock_nrf_modem_is_initialized_IgnoreAndReturn(1);
	__cmock_lte_lc_func_mode_set_IgnoreAndReturn(0);
	__cmock_nrf_modem_lib_shutdown_IgnoreAndReturn(0);

	/* Lose PDN and CEREG */
	pdn_event_handler_callback(0, PDN_EVENT_DEACTIVATED, 0);
	lte_lc_event_handler_callback(&cereg_evt);

	/* Let events propagate */
	k_sleep(K_MSEC(10));

	/* Remove IP addresses if any */
	if (net_if_ipv6_get_global_addr(NET_ADDR_PREFERRED, &net_if)) {
		net_if_config_ipv6_put(net_if);
	}

	if (net_if_ipv4_get_global_addr(net_if, NET_ADDR_PREFERRED)) {
		net_if_config_ipv4_put(net_if);
	}

	/* Reset flags */
	conn_mgr_if_set_flag(net_if, CONN_MGR_IF_PERSISTENT, false);

	/* Reset options */
	uint8_t option = LTE_CONNECTIVITY_IF_DOWN_LTE_DISCONNECT;
	(void)conn_mgr_if_set_opt(net_if, LTE_CONNECTIVITY_IF_DOWN,
					  (const void *)&option,
					  sizeof(option));

	/* Take iface down */
	(void)net_if_down(net_if);

	/* Let events propagate */
	k_sleep(K_MSEC(10));

	/* Stop ignoring mocked functions*/
	__cmock_nrf_modem_is_initialized_StopIgnore();
	__cmock_lte_lc_func_mode_set_StopIgnore();
	__cmock_nrf_modem_lib_shutdown_StopIgnore();

	/* Prepare to track/mock modem_at_scanf calls for the upcoming test */
	mock_nrf_modem_at_Init();
}

void tearDown(void)
{
	/* Validate modem_at_scanf calls */
	mock_nrf_modem_at_Verify();
}

/* Verify lte_connectivity_init().
 * This function is called at SYS init by the Connection Manager.
 */

void test_init_should_set_network_interface_as_dormant(void)
{
	struct net_if *net_if = net_if_get_default();

	TEST_ASSERT_TRUE(net_if_flag_is_set(net_if, NET_IF_DORMANT));
}

void test_init_should_set_timeout(void)
{
	struct net_if *net_if = net_if_get_default();
	int timeout_desired = CONFIG_LTE_CONNECTIVITY_CONNECT_TIMEOUT_SECONDS;

	TEST_ASSERT_EQUAL(timeout_desired, conn_mgr_if_get_timeout(net_if));
}

/* Ensure that initial settings reflect what was selected in KConfig */
void test_initial_settings_should_match_kconfig(void)
{
	TEST_ASSERT_EQUAL(
		IS_ENABLED(CONFIG_LTE_CONNECTIVITY_CONNECTION_PERSISTENCE),
		initial_persistence
	);
	TEST_ASSERT_EQUAL(
		IS_ENABLED(CONFIG_LTE_CONNECTIVITY_AUTO_CONNECT),
		initial_auto_connect
	);
	TEST_ASSERT_EQUAL(
		IS_ENABLED(CONFIG_LTE_CONNECTIVITY_AUTO_DOWN),
		initial_auto_down
	);
	TEST_ASSERT_EQUAL(
		CONFIG_LTE_CONNECTIVITY_CONNECT_TIMEOUT_SECONDS,
		initial_connect_timeout
	);
}

/* Expect pdn_default_ctx_cb_reg() to be called at SYS init. We need to expect this at SYS init
 * due to the mock being called before the test runner.
 */
static int test_init_should_set_pdn_event_handler(void)
{
	__cmock_pdn_default_ctx_cb_reg_Stub(&pdn_default_ctx_cb_reg_stub);
	__cmock_pdn_default_ctx_cb_reg_ExpectAnyArgsAndReturn(0);
	return 0;
}
SYS_INIT(test_init_should_set_pdn_event_handler, POST_KERNEL, 0);

/* Expect lte_lc_register_handler() to be called at SYS init. We need to expect this at SYS init
 * due to the mock being called before the test runner.
 */
static int test_init_should_set_cereg_event_handler(void)
{
	__cmock_lte_lc_register_handler_Stub(&lte_lc_register_handler_stub);
	__cmock_lte_lc_register_handler_ExpectAnyArgs();
	return 0;
}
SYS_INIT(test_init_should_set_cereg_event_handler, POST_KERNEL, 0);

/* Verify lte_connectivity_enable() */

void test_enable_should_init_modem_and_link_controller(void)
{
	struct net_if *net_if = net_if_get_default();

	__cmock_nrf_modem_is_initialized_ExpectAndReturn(0);
	__cmock_nrf_modem_lib_init_ExpectAndReturn(0);
	__cmock_lte_lc_init_ExpectAndReturn(0);

	TEST_ASSERT_EQUAL(0, net_if_up(net_if));
}

void test_enable_should_init_modem_twice_upon_successful_dfu_result(void)
{
	struct net_if *net_if = net_if_get_default();

	__cmock_nrf_modem_is_initialized_ExpectAndReturn(0);
	__cmock_nrf_modem_lib_init_ExpectAndReturn(NRF_MODEM_DFU_RESULT_OK);
	__cmock_nrf_modem_lib_init_ExpectAndReturn(0);
	__cmock_lte_lc_init_ExpectAndReturn(0);

	TEST_ASSERT_EQUAL(0, net_if_up(net_if));
}

void test_enable_should_return_error_upon_dfu_error(void)
{
	struct net_if *net_if = net_if_get_default();

	__cmock_nrf_modem_is_initialized_ExpectAndReturn(0);
	__cmock_nrf_modem_lib_init_ExpectAndReturn(NRF_MODEM_DFU_RESULT_HARDWARE_ERROR);

	TEST_ASSERT_EQUAL(NRF_MODEM_DFU_RESULT_HARDWARE_ERROR, net_if_up(net_if));
}

/* Verify lte_connectivity_disable() */

/* Verify taking the iface down also shuts down the modem if
 * the IF_DOWN mode is configured as such
 */
void test_disable_should_shutdown_modem(void)
{
	/* Set the IF_DOWN mode to SHUTDOWN_MODEM */
	network_interface_down_option_set(LTE_CONNECTIVITY_IF_DOWN_MODEM_SHUTDOWN, 0);

	/* Bring the interface up */
	bring_network_interface_up();

	/* Expect that the modem will be shut down when interface is taken down.
	 * Schedule the shutdown to succeed.
	 */
	__cmock_nrf_modem_lib_shutdown_ExpectAndReturn(0);

	/* Take the interface down and verify that it succeeds */
	TEST_ASSERT_EQUAL(0, net_if_down(net_if_get_default()));
}

/* Verify taking the iface down fails if modem shutdown fails and
 * the IF_DOWN mode is set to SHUTDOWN_MODEM
 */
void test_disable_should_return_error_if_shutdown_of_modem_fails(void)
{
	/* Set the IF_DOWN mode to SHUTDOWN_MODEM */
	network_interface_down_option_set(LTE_CONNECTIVITY_IF_DOWN_MODEM_SHUTDOWN, 0);

	/* Bring the interface up */
	bring_network_interface_up();

	/* Expect that the modem will be shut down when interface is taken down.
	 * Schedule the shutdown to fail.
	 */
	__cmock_nrf_modem_lib_shutdown_ExpectAndReturn(-1);

	/* Take the interface down and verify that it fails */
	TEST_ASSERT_EQUAL(-1, net_if_down(net_if_get_default()));
}

void test_disable_should_disconnect_lte(void)
{
	bring_network_interface_up();

	__cmock_lte_lc_func_mode_set_ExpectAndReturn(LTE_LC_FUNC_MODE_DEACTIVATE_LTE, 0);

	TEST_ASSERT_EQUAL(0, net_if_down(net_if_get_default()));
}

void test_disable_should_return_error_if_lte_disconnect_fails(void)
{
	bring_network_interface_up();

	__cmock_lte_lc_func_mode_set_ExpectAndReturn(LTE_LC_FUNC_MODE_DEACTIVATE_LTE, -1);

	TEST_ASSERT_EQUAL(-1, net_if_down(net_if_get_default()));
}

/* Verify lte_connectivity_connect() */

void test_connect_should_set_functional_mode(void)
{
	bring_network_interface_up();

	__cmock_lte_lc_func_mode_set_ExpectAndReturn(LTE_LC_FUNC_MODE_ACTIVATE_LTE, 0);

	TEST_ASSERT_EQUAL(0, conn_mgr_if_connect(net_if_get_default()));
}

void test_connect_should_return_error_if_setting_of_functional_mode_fails(void)
{
	bring_network_interface_up();

	__cmock_lte_lc_func_mode_set_ExpectAndReturn(LTE_LC_FUNC_MODE_ACTIVATE_LTE, -1);

	TEST_ASSERT_EQUAL(-1, conn_mgr_if_connect(net_if_get_default()));
}

/* Verify lte_connectivity_disconnect() */

void test_disconnect_should_set_functional_mode(void)
{
	bring_network_interface_up();

	__cmock_lte_lc_func_mode_set_ExpectAndReturn(LTE_LC_FUNC_MODE_DEACTIVATE_LTE, 0);

	TEST_ASSERT_EQUAL(0, conn_mgr_if_disconnect(net_if_get_default()));
}

void test_disconnect_should_return_error_if_setting_of_functional_mode_fails(void)
{
	bring_network_interface_up();

	__cmock_lte_lc_func_mode_set_ExpectAndReturn(LTE_LC_FUNC_MODE_DEACTIVATE_LTE, -1);

	TEST_ASSERT_EQUAL(-1, conn_mgr_if_disconnect(net_if_get_default()));
}

/* Verify lte_connectivity_options_set() */

void test_options_set_should_return_success_when_setting_option_to_modem_shutdown(void)
{
	network_interface_down_option_set(LTE_CONNECTIVITY_IF_DOWN_MODEM_SHUTDOWN, 0);
}

void test_options_set_should_return_error_upon_unsupported_option_name(void)
{
	struct net_if *net_if = net_if_get_default();

	uint8_t option = LTE_CONNECTIVITY_IF_DOWN_LTE_DISCONNECT;

	int ret = conn_mgr_if_set_opt(net_if,
				      UINT8_MAX,
				      (const void *)&option,
				      sizeof(option));

	TEST_ASSERT_EQUAL(-ENOPROTOOPT, ret);
}

void test_options_set_should_return_error_upon_unsupported_option_value(void)
{
	network_interface_down_option_set(UINT8_MAX, -EBADF);
}

void test_options_set_should_return_error_on_too_large_input(void)
{
	struct net_if *net_if = net_if_get_default();

	uint32_t option = LTE_CONNECTIVITY_IF_DOWN_LTE_DISCONNECT;

	int ret = conn_mgr_if_set_opt(net_if,
				      LTE_CONNECTIVITY_IF_DOWN,
				      (const void *)&option,
				      sizeof(option));

	TEST_ASSERT_EQUAL(-ENOBUFS, ret);
}

/* Verify lte_connectivity_options_get() */

void test_options_get_should_return_success(void)
{
	struct net_if *net_if = net_if_get_default();
	uint8_t option;
	size_t len = sizeof(option);

	int ret = conn_mgr_if_get_opt(net_if, LTE_CONNECTIVITY_IF_DOWN, (void *)&option, &len);

	TEST_ASSERT_EQUAL(0, ret);
}

/* Verify that conn_mgr_if_set_opt successfully changes the IF_DOWN option from LTE_DISCONNECT
 * to MODEM_SHUTDOWN
 */
void test_options_set_should_change_connectivity_if_down_to_modem_shutdown(void)
{
	struct net_if *net_if = net_if_get_default();
	uint8_t option = 99;
	size_t len;
	int ret;

	/* Verify that the IF_DOWN option starts out as LTE_DISCONNECT */
	len = sizeof(option);
	ret = conn_mgr_if_get_opt(net_if, LTE_CONNECTIVITY_IF_DOWN, (void *)&option, &len);
	TEST_ASSERT_EQUAL(LTE_CONNECTIVITY_IF_DOWN_LTE_DISCONNECT, option);
	TEST_ASSERT_EQUAL(0, ret);

	/* Attempt to change the IF_DOWN option to MODEM_SHUTDOWN */
	network_interface_down_option_set(LTE_CONNECTIVITY_IF_DOWN_MODEM_SHUTDOWN, 0);

	/* Verify that the IF_DOWN option was successfully changed  */
	len = sizeof(option);
	ret = conn_mgr_if_get_opt(net_if, LTE_CONNECTIVITY_IF_DOWN, (void *)&option, &len);
	TEST_ASSERT_EQUAL(LTE_CONNECTIVITY_IF_DOWN_MODEM_SHUTDOWN, option);
	TEST_ASSERT_EQUAL(0, ret);
}

/* Verify that conn_mgr_if_set_opt successfully changes the IF_DOWN option from MODEM_SHUTDOWN
 * to LTE_DISCONNECT
 */
void test_options_set_should_change_connectivity_if_down_to_lte_disconnect(void)
{
	struct net_if *net_if = net_if_get_default();
	uint8_t option = 99;
	size_t len;
	int ret;

	/* Set up initial conditions by setting IF_DOWN option to MODEM_SHUTDOWN */
	network_interface_down_option_set(LTE_CONNECTIVITY_IF_DOWN_MODEM_SHUTDOWN, 0);

	/* Verify that the IF_DOWN option is MODEM_SHUTDOWN */
	len = sizeof(option);
	ret = conn_mgr_if_get_opt(net_if, LTE_CONNECTIVITY_IF_DOWN, (void *)&option, &len);
	TEST_ASSERT_EQUAL(LTE_CONNECTIVITY_IF_DOWN_MODEM_SHUTDOWN, option);
	TEST_ASSERT_EQUAL(0, ret);

	/* Attempt to change the IF_DOWN option to LTE_DISCONNECT */
	network_interface_down_option_set(LTE_CONNECTIVITY_IF_DOWN_LTE_DISCONNECT, 0);

	/* Verify that the IF_DOWN option was successfully changed  */
	len = sizeof(option);
	ret = conn_mgr_if_get_opt(net_if, LTE_CONNECTIVITY_IF_DOWN, (void *)&option, &len);
	TEST_ASSERT_EQUAL(LTE_CONNECTIVITY_IF_DOWN_LTE_DISCONNECT, option);
	TEST_ASSERT_EQUAL(0, ret);
}

void test_options_get_should_return_error_if_wrong_option_name_is_used(void)
{
	struct net_if *net_if = net_if_get_default();
	uint8_t option;
	size_t len = sizeof(option);
	int ret;

	ret = conn_mgr_if_get_opt(net_if, UINT8_MAX, (void *)&option, &len);

	TEST_ASSERT_EQUAL(-ENOPROTOOPT, ret);
}

/* PDN notifications */

/* IPv4 */

/* Ensure that PDN activation and deativation adds and removes an IPv4 address if provided */
void test_pdn_events_should_trigger_ipv4_address_changes(void)
{
	struct net_if *net_if = net_if_get_default();

	/* Expect lte_connectivity to check for an IPv4 address when PDN is activated */
	__mock_nrf_modem_at_scanf_ExpectAndReturn(
			"AT+CGPADDR=0", "+CGPADDR: %*d,\"%46[.:0-9A-F]\",\"%46[:0-9A-F]\"", 1);

	/* Provide one */
	__mock_nrf_modem_at_scanf_ReturnVarg_string("192.9.201.39");

	/* Activate PDN */
	pdn_event_handler_callback(0, PDN_EVENT_ACTIVATED, 0);

	/* Verify IPv4 was added */
	TEST_ASSERT_TRUE(net_if_ipv4_get_global_addr(net_if, NET_ADDR_PREFERRED));

	/* Let events fire */
	k_sleep(K_MSEC(10));

	/* Dectivate PDN */
	pdn_event_handler_callback(0, PDN_EVENT_DEACTIVATED, 0);

	/* Verify that IPv4 was removed */
	TEST_ASSERT_FALSE(net_if_ipv4_get_global_addr(net_if, NET_ADDR_PREFERRED));
}

/* Ensure that PDN activation does not add an IPv4 address if none is provided */
void test_pdn_act_should_not_add_nonexistant_ipv4(void)
{
	struct net_if *net_if = net_if_get_default();

	/* Expect lte_connectivity to check for an IPv4 address when PDN is activated */
	__mock_nrf_modem_at_scanf_ExpectAndReturn(
			"AT+CGPADDR=0", "+CGPADDR: %*d,\"%46[.:0-9A-F]\",\"%46[:0-9A-F]\"", 0);

	/* Do not provide one */
	__cmock_lte_lc_func_mode_set_ExpectAndReturn(LTE_LC_FUNC_MODE_DEACTIVATE_LTE, 0);

	/* Activate PDN */
	pdn_event_handler_callback(0, PDN_EVENT_ACTIVATED, 0);

	/* Verify IPv4 was not added */
	TEST_ASSERT_FALSE(net_if_ipv4_get_global_addr(net_if, NET_ADDR_PREFERRED));
}

/* IPv6 */

/* Verify that PDN activation and deativation adds and removes an IPv6 address if provided */
void test_pdn_events_should_trigger_ipv6_address_changes(void)
{
	struct net_if *net_if = net_if_get_default();

	/* Expect lte_connectivity to check for an IPv6 address when PDN is activated */
	__mock_nrf_modem_at_scanf_ExpectAndReturn(
			"AT+CGPADDR=0", "+CGPADDR: %*d,\"%46[.:0-9A-F]\",\"%46[:0-9A-F]\"", 2);

	/* Provide one */
	__mock_nrf_modem_at_scanf_ReturnVarg_string("");
	__mock_nrf_modem_at_scanf_ReturnVarg_string("2001:8c0:5140:801:1b4d:4ce8:6eed:7b69");

	/* Activate PDN */
	pdn_event_handler_callback(0, PDN_EVENT_IPV6_UP, 0);

	/* Verify IPv6 was added */
	TEST_ASSERT_TRUE(net_if_ipv6_get_global_addr(NET_ADDR_PREFERRED, &net_if));

	/* Let events fire */
	k_sleep(K_MSEC(10));

	/* Dectivate PDN */
	pdn_event_handler_callback(0, PDN_EVENT_DEACTIVATED, 0);

	/* Verify that IPv6 was removed */
	TEST_ASSERT_FALSE(net_if_ipv6_get_global_addr(NET_ADDR_PREFERRED, &net_if));
}


/* Verify that PDN activation does not add an IPv6 address if none is provided */
void test_pdn_act_should_not_add_nonexistant_ipv6(void)
{
	struct net_if *net_if = net_if_get_default();

	/* Expect lte_connectivity to check for an IPv4 address when PDN is activated */
	__mock_nrf_modem_at_scanf_ExpectAndReturn(
			"AT+CGPADDR=0", "+CGPADDR: %*d,\"%46[.:0-9A-F]\",\"%46[:0-9A-F]\"", 0);

	/* Do not provide one */
	__cmock_lte_lc_func_mode_set_ExpectAndReturn(LTE_LC_FUNC_MODE_DEACTIVATE_LTE, 0);

	/* Activate PDN */
	pdn_event_handler_callback(0, PDN_EVENT_ACTIVATED, 0);

	/* Verify IPv6 was not added */
	TEST_ASSERT_FALSE(net_if_ipv6_get_global_addr(NET_ADDR_PREFERRED, &net_if));
}

/* CEREG and PDN readiness */

/* Verify that PDN activation does not activate iface if cereg state is unregistered */
void test_pdn_act_without_cereg_should_not_activate_iface(void)
{
	struct net_if *net_if = net_if_get_default();

	/* We do not care about modem init, initialization checks, or func mode changes for this
	 * test. Set up some sane values, but ignore call counts.
	 */
	__cmock_nrf_modem_is_initialized_IgnoreAndReturn(1);
	__cmock_lte_lc_func_mode_set_IgnoreAndReturn(0);
	__cmock_lte_lc_init_IgnoreAndReturn(0);

	/* Take the iface admin-up */
	net_if_up(net_if);

	/* Check that the iface is still dormant. */
	TEST_ASSERT_TRUE(net_if_is_dormant(net_if));

	/* Prepare an IP address to be provided when PDN is set to active */
	__mock_nrf_modem_at_scanf_ExpectAndReturn(
			"AT+CGPADDR=0", "+CGPADDR: %*d,\"%46[.:0-9A-F]\",\"%46[:0-9A-F]\"", 1);
	__mock_nrf_modem_at_scanf_ReturnVarg_string("192.9.201.39");

	/* Fire PDN active */
	pdn_event_handler_callback(0, PDN_EVENT_ACTIVATED, 0);

	/* Let events fire */
	k_sleep(K_MSEC(10));

	/* Check that iface remains inactive */
	TEST_ASSERT_TRUE(net_if_is_dormant(net_if));
}

/* Verify that PDN activation activates iface if cereg state is registered */
void test_pdn_act_with_cereg_should_activate_iface(void)
{
	struct net_if *net_if = net_if_get_default();

	/* Set up dummy CEREG event struct. */
	struct lte_lc_evt cereg_evt = {
		.type = LTE_LC_EVT_NW_REG_STATUS
	};

	/* We do not care about modem init, initialization checks, or func mode changes for this
	 * test. Set up some sane values, but ignore call counts.
	 */
	__cmock_nrf_modem_is_initialized_IgnoreAndReturn(1);
	__cmock_lte_lc_func_mode_set_IgnoreAndReturn(0);
	__cmock_lte_lc_init_IgnoreAndReturn(0);

	/* Take the iface admin-up */
	net_if_up(net_if);

	/* Fire CEREG registered */
	cereg_evt.nw_reg_status = LTE_LC_NW_REG_REGISTERED_HOME;
	lte_lc_event_handler_callback(&cereg_evt);

	/* Let events fire */
	k_sleep(K_MSEC(10));

	/* Check that the iface is still dormant. */
	TEST_ASSERT_TRUE(net_if_is_dormant(net_if));

	/* Prepare an IP address to be provided when PDN is set to active */
	__mock_nrf_modem_at_scanf_ExpectAndReturn(
			"AT+CGPADDR=0", "+CGPADDR: %*d,\"%46[.:0-9A-F]\",\"%46[:0-9A-F]\"", 1);
	__mock_nrf_modem_at_scanf_ReturnVarg_string("192.9.201.39");

	/* Fire PDN active */
	pdn_event_handler_callback(0, PDN_EVENT_ACTIVATED, 0);

	/* Let events fire */
	k_sleep(K_MSEC(10));

	/* Check that iface becomes active */
	TEST_ASSERT_FALSE(net_if_is_dormant(net_if));
}


/* Verify that CEREG registration does not activate iface if PDN is inactive */
void test_cereg_registered_without_pdn_should_not_activate_iface(void)
{
	struct net_if *net_if = net_if_get_default();

	/* Set up dummy CEREG event struct. */
	struct lte_lc_evt cereg_evt = {
		.type = LTE_LC_EVT_NW_REG_STATUS
	};

	/* We do not care about modem init, initialization checks, or func mode changes for this
	 * test. Set up some sane values, but ignore call counts.
	 */
	__cmock_nrf_modem_is_initialized_IgnoreAndReturn(1);
	__cmock_lte_lc_func_mode_set_IgnoreAndReturn(0);
	__cmock_lte_lc_init_IgnoreAndReturn(0);

	/* Take the iface admin-up */
	net_if_up(net_if);

	/* Check that the iface is still dormant. */
	TEST_ASSERT_TRUE(net_if_is_dormant(net_if));

	/* Fire CEREG registered */
	cereg_evt.nw_reg_status = LTE_LC_NW_REG_REGISTERED_HOME;
	lte_lc_event_handler_callback(&cereg_evt);

	/* Let events fire */
	k_sleep(K_MSEC(10));

	/* Check that iface remains inactive */
	TEST_ASSERT_TRUE(net_if_is_dormant(net_if));
}

/* Verify that CEREG registration (home) does activate iface if PDN is active */
void test_cereg_registered_home_with_pdn_should_activate_iface(void)
{
	struct net_if *net_if = net_if_get_default();

	/* Set up dummy CEREG event struct. */
	struct lte_lc_evt cereg_evt = {
		.type = LTE_LC_EVT_NW_REG_STATUS
	};

	/* We do not care about modem init, initialization checks, or func mode changes for this
	 * test. Set up some sane values, but ignore call counts.
	 */
	__cmock_nrf_modem_is_initialized_IgnoreAndReturn(1);
	__cmock_lte_lc_func_mode_set_IgnoreAndReturn(0);
	__cmock_lte_lc_init_IgnoreAndReturn(0);

	/* Take the iface admin-up */
	net_if_up(net_if);

	/* Prepare an IP address to be provided when PDN is set to active */
	__mock_nrf_modem_at_scanf_ExpectAndReturn(
			"AT+CGPADDR=0", "+CGPADDR: %*d,\"%46[.:0-9A-F]\",\"%46[:0-9A-F]\"", 1);
	__mock_nrf_modem_at_scanf_ReturnVarg_string("192.9.201.39");

	/* Fire PDN active */
	pdn_event_handler_callback(0, PDN_EVENT_ACTIVATED, 0);

	/* Let events fire */
	k_sleep(K_MSEC(10));

	/* Check that the iface is still dormant. */
	TEST_ASSERT_TRUE(net_if_is_dormant(net_if));

	/* Fire CEREG registered (home) */
	cereg_evt.nw_reg_status = LTE_LC_NW_REG_REGISTERED_HOME;
	lte_lc_event_handler_callback(&cereg_evt);

	/* Let events fire */
	k_sleep(K_MSEC(10));

	/* Check that iface becomes active */
	TEST_ASSERT_FALSE(net_if_is_dormant(net_if));
}

/* Verify that CEREG registration (roaming) does activate iface if PDN is active */
void test_cereg_registered_roaming_with_pdn_should_activate_iface(void)
{
	struct net_if *net_if = net_if_get_default();

	/* Set up dummy CEREG event struct. */
	struct lte_lc_evt cereg_evt = {
		.type = LTE_LC_EVT_NW_REG_STATUS
	};

	/* We do not care about modem init, initialization checks, or func mode changes for this
	 * test. Set up some sane values, but ignore call counts.
	 */
	__cmock_nrf_modem_is_initialized_IgnoreAndReturn(1);
	__cmock_lte_lc_func_mode_set_IgnoreAndReturn(0);
	__cmock_lte_lc_init_IgnoreAndReturn(0);

	/* Take the iface admin-up */
	net_if_up(net_if);

	/* Prepare an IP address to be provided when PDN is set to active */
	__mock_nrf_modem_at_scanf_ExpectAndReturn(
			"AT+CGPADDR=0", "+CGPADDR: %*d,\"%46[.:0-9A-F]\",\"%46[:0-9A-F]\"", 1);
	__mock_nrf_modem_at_scanf_ReturnVarg_string("192.9.201.39");

	/* Fire PDN active */
	pdn_event_handler_callback(0, PDN_EVENT_ACTIVATED, 0);

	/* Let events fire */
	k_sleep(K_MSEC(10));

	/* Check that the iface is still dormant. */
	TEST_ASSERT_TRUE(net_if_is_dormant(net_if));

	/* Fire CEREG registered (roaming) */
	cereg_evt.nw_reg_status = LTE_LC_NW_REG_REGISTERED_ROAMING;
	lte_lc_event_handler_callback(&cereg_evt);

	/* Let events fire */
	k_sleep(K_MSEC(10));

	/* Check that iface becomes active */
	TEST_ASSERT_FALSE(net_if_is_dormant(net_if));
}

/* Verify that CEREG searching does not affect iface activation (from inactive to active) */
void test_cereg_searching_should_not_activate_iface(void)
{
	struct net_if *net_if = net_if_get_default();

	/* Set up dummy CEREG event struct. */
	struct lte_lc_evt cereg_evt = {
		.type = LTE_LC_EVT_NW_REG_STATUS
	};

	/* We do not care about modem init, initialization checks, or func mode changes for this
	 * test. Set up some sane values, but ignore call counts.
	 */
	__cmock_nrf_modem_is_initialized_IgnoreAndReturn(1);
	__cmock_lte_lc_func_mode_set_IgnoreAndReturn(0);
	__cmock_lte_lc_init_IgnoreAndReturn(0);

	/* Take the iface admin-up */
	net_if_up(net_if);

	/* Check that the iface is inactive. */
	TEST_ASSERT_TRUE(net_if_is_dormant(net_if));

	/* Fire CEREG searching */
	cereg_evt.nw_reg_status = LTE_LC_NW_REG_SEARCHING;
	lte_lc_event_handler_callback(&cereg_evt);

	/* Let events fire */
	k_sleep(K_MSEC(10));

	/* Check that iface remains active */
	TEST_ASSERT_TRUE(net_if_is_dormant(net_if));
}


/* Verify that CEREG searching does not affect iface activation (from active to inactive) */
void test_cereg_searching_should_not_deactivate_iface(void)
{
	struct net_if *net_if = net_if_get_default();

	/* Set up dummy CEREG event struct. */
	struct lte_lc_evt cereg_evt = {
		.type = LTE_LC_EVT_NW_REG_STATUS
	};

	/* We do not care about modem init, initialization checks, or func mode changes for this
	 * test. Set up some sane values, but ignore call counts.
	 */
	__cmock_nrf_modem_is_initialized_IgnoreAndReturn(1);
	__cmock_lte_lc_func_mode_set_IgnoreAndReturn(0);
	__cmock_lte_lc_init_IgnoreAndReturn(0);

	/* Take the iface admin-up */
	net_if_up(net_if);

	/* Prepare an IP address to be provided when PDN is set to active */
	__mock_nrf_modem_at_scanf_ExpectAndReturn(
			"AT+CGPADDR=0", "+CGPADDR: %*d,\"%46[.:0-9A-F]\",\"%46[:0-9A-F]\"", 1);
	__mock_nrf_modem_at_scanf_ReturnVarg_string("192.9.201.39");

	/* Fire PDN active */
	pdn_event_handler_callback(0, PDN_EVENT_ACTIVATED, 0);

	/* Fire CEREG registered */
	cereg_evt.nw_reg_status = LTE_LC_NW_REG_REGISTERED_HOME;
	lte_lc_event_handler_callback(&cereg_evt);

	/* Let events fire */
	k_sleep(K_MSEC(10));

	/* Check that the iface is active. */
	TEST_ASSERT_FALSE(net_if_is_dormant(net_if));

	/* Fire CEREG searching */
	cereg_evt.nw_reg_status = LTE_LC_NW_REG_SEARCHING;
	lte_lc_event_handler_callback(&cereg_evt);

	/* Let events fire */
	k_sleep(K_MSEC(10));

	/* Check that iface remains active */
	TEST_ASSERT_FALSE(net_if_is_dormant(net_if));
}

/* Verify that CEREG unregistered deactivates iface */
void test_cereg_unregistered_should_deactivate_iface(void)
{
	struct net_if *net_if = net_if_get_default();

	/* Set up dummy CEREG event struct. */
	struct lte_lc_evt cereg_evt = {
		.type = LTE_LC_EVT_NW_REG_STATUS
	};

	/* We do not care about modem init, initialization checks, or func mode changes for this
	 * test. Set up some sane values, but ignore call counts.
	 */
	__cmock_nrf_modem_is_initialized_IgnoreAndReturn(1);
	__cmock_lte_lc_func_mode_set_IgnoreAndReturn(0);
	__cmock_lte_lc_init_IgnoreAndReturn(0);

	/* Take the iface admin-up */
	net_if_up(net_if);

	/* Prepare an IP address to be provided when PDN is set to active */
	__mock_nrf_modem_at_scanf_ExpectAndReturn(
			"AT+CGPADDR=0", "+CGPADDR: %*d,\"%46[.:0-9A-F]\",\"%46[:0-9A-F]\"", 1);
	__mock_nrf_modem_at_scanf_ReturnVarg_string("192.9.201.39");

	/* Fire PDN active */
	pdn_event_handler_callback(0, PDN_EVENT_ACTIVATED, 0);

	/* Fire CEREG registered */
	cereg_evt.nw_reg_status = LTE_LC_NW_REG_REGISTERED_HOME;
	lte_lc_event_handler_callback(&cereg_evt);

	/* Let events fire */
	k_sleep(K_MSEC(10));

	/* Check that the iface is active. */
	TEST_ASSERT_FALSE(net_if_is_dormant(net_if));

	/* Fire CEREG unregistered */
	cereg_evt.nw_reg_status = LTE_LC_NW_REG_UNKNOWN;
	lte_lc_event_handler_callback(&cereg_evt);

	/* Let events fire */
	k_sleep(K_MSEC(10));

	/* Check that iface becomes inactive */
	TEST_ASSERT_TRUE(net_if_is_dormant(net_if));
}

/* Verify that PDN deactivation deactivates iface */
void test_pdn_deact_should_deactivate_iface(void)
{
	struct net_if *net_if = net_if_get_default();

	/* Set up dummy CEREG event struct. */
	struct lte_lc_evt cereg_evt = {
		.type = LTE_LC_EVT_NW_REG_STATUS
	};

	/* We do not care about modem init, initialization checks, or func mode changes for this
	 * test. Set up some sane values, but ignore call counts.
	 */
	__cmock_nrf_modem_is_initialized_IgnoreAndReturn(1);
	__cmock_lte_lc_func_mode_set_IgnoreAndReturn(0);
	__cmock_lte_lc_init_IgnoreAndReturn(0);

	/* Take the iface admin-up */
	net_if_up(net_if);

	/* Prepare an IP address to be provided when PDN is set to active */
	__mock_nrf_modem_at_scanf_ExpectAndReturn(
			"AT+CGPADDR=0", "+CGPADDR: %*d,\"%46[.:0-9A-F]\",\"%46[:0-9A-F]\"", 1);
	__mock_nrf_modem_at_scanf_ReturnVarg_string("192.9.201.39");

	/* Fire PDN active */
	pdn_event_handler_callback(0, PDN_EVENT_ACTIVATED, 0);

	/* Fire CEREG registered */
	cereg_evt.nw_reg_status = LTE_LC_NW_REG_REGISTERED_HOME;
	lte_lc_event_handler_callback(&cereg_evt);

	/* Let events fire */
	k_sleep(K_MSEC(10));

	/* Check that the iface is active. */
	TEST_ASSERT_FALSE(net_if_is_dormant(net_if));

	/* Fire PDN inactive */
	pdn_event_handler_callback(0, PDN_EVENT_DEACTIVATED, 0);

	/* Let events fire */
	k_sleep(K_MSEC(10));

	/* Check that iface becomes inactive */
	TEST_ASSERT_TRUE(net_if_is_dormant(net_if));
}

int main(void)
{
	(void)unity_main();

	return 0;
}
