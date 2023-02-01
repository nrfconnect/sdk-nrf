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

/* Stub used to register test local PDN event handler used to invoke PDN event in UUT. */
static int pdn_default_ctx_cb_reg_stub(pdn_event_handler_t cb, int num_of_calls)
{
	pdn_event_handler_callback = cb;
	return 0;
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

static void ipv4_address_add(void)
{
	struct net_if *net_if = net_if_get_default();

	__mock_nrf_modem_at_scanf_ExpectAndReturn(
			"AT+CGPADDR=0", "+CGPADDR: %*d,\"%46[.:0-9A-F]\",\"%46[:0-9A-F]\"", 1);
	__mock_nrf_modem_at_scanf_ReturnVarg_string("192.9.201.39");

	pdn_event_handler_callback(0, PDN_EVENT_ACTIVATED, 0);

	TEST_ASSERT_FALSE(net_if_flag_is_set(net_if, NET_IF_DORMANT));
	TEST_ASSERT_TRUE(net_if_ipv4_get_global_addr(net_if, NET_ADDR_PREFERRED));
}

static void ipv6_address_add(void)
{
	struct net_if *net_if = net_if_get_default();

	__mock_nrf_modem_at_scanf_ExpectAndReturn(
			"AT+CGPADDR=0", "+CGPADDR: %*d,\"%46[.:0-9A-F]\",\"%46[:0-9A-F]\"", 2);
	__mock_nrf_modem_at_scanf_ReturnVarg_string("");
	__mock_nrf_modem_at_scanf_ReturnVarg_string("2001:8c0:5140:801:1b4d:4ce8:6eed:7b69");

	pdn_event_handler_callback(0, PDN_EVENT_IPV6_UP, 0);

	TEST_ASSERT_TRUE(net_if_ipv6_get_global_addr(NET_ADDR_PREFERRED, &net_if));
}

void setUp(void)
{
	mock_nrf_modem_at_Init();
}

void tearDown(void)
{
	uint8_t option;
	size_t len;
	struct net_if *net_if = net_if_get_default();
	enum lte_lc_func_mode mode_expected = LTE_LC_FUNC_MODE_DEACTIVATE_LTE;

	int ret = conn_mgr_if_get_opt(net_if, LTE_CONNECTIVITY_IF_DOWN, (void *)&option, &len);

	TEST_ASSERT_EQUAL(0, ret);

	/* Do teardown depending on the LTE_CONNECTIVITY_IF_DOWN option. */
	if (net_if_flag_is_set(net_if, NET_IF_UP)) {
		if (option == LTE_CONNECTIVITY_IF_DOWN_LTE_DISCONNECT) {
			__cmock_lte_lc_func_mode_set_ExpectAndReturn(mode_expected, 0);
			TEST_ASSERT_EQUAL(0, net_if_down(net_if));
		} else if (option == LTE_CONNECTIVITY_IF_DOWN_MODEM_SHUTDOWN) {
			__cmock_nrf_modem_lib_shutdown_ExpectAndReturn(0);
			TEST_ASSERT_EQUAL(0, net_if_down(net_if));
		} else {
			TEST_ASSERT_TRUE(false);
		}
	}

	/* Remove IP addresses if any */
	if (net_if_ipv6_get_global_addr(NET_ADDR_PREFERRED, &net_if)) {
		TEST_ASSERT_EQUAL(0, net_if_config_ipv6_put(net_if));
	}

	if (net_if_ipv4_get_global_addr(net_if, NET_ADDR_PREFERRED)) {
		TEST_ASSERT_EQUAL(0, net_if_config_ipv4_put(net_if));
	}

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

void test_init_should_set_connection_persistent_flag(void)
{
	struct net_if *net_if = net_if_get_default();

	TEST_ASSERT_TRUE(conn_mgr_if_get_flag(net_if, CONN_MGR_IF_PERSISTENT));
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

void test_disable_should_shutdown_modem(void)
{
	bring_network_interface_up();

	__cmock_nrf_modem_lib_shutdown_ExpectAndReturn(0);

	TEST_ASSERT_EQUAL(0, net_if_down(net_if_get_default()));
}

void test_disable_should_return_error_if_shutdown_of_modem_fails(void)
{
	bring_network_interface_up();

	__cmock_nrf_modem_lib_shutdown_ExpectAndReturn(-1);

	TEST_ASSERT_EQUAL(-1, net_if_down(net_if_get_default()));
}

void test_disable_should_disconnect_lte(void)
{
	bring_network_interface_up();
	network_interface_down_option_set(LTE_CONNECTIVITY_IF_DOWN_LTE_DISCONNECT, 0);

	__cmock_lte_lc_func_mode_set_ExpectAndReturn(LTE_LC_FUNC_MODE_DEACTIVATE_LTE, 0);

	TEST_ASSERT_EQUAL(0, net_if_down(net_if_get_default()));
}

void test_disable_should_return_error_if_lte_disconnect_fails(void)
{
	bring_network_interface_up();
	network_interface_down_option_set(LTE_CONNECTIVITY_IF_DOWN_LTE_DISCONNECT, 0);

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
	size_t len;

	int ret = conn_mgr_if_get_opt(net_if, LTE_CONNECTIVITY_IF_DOWN, (void *)&option, &len);

	TEST_ASSERT_EQUAL(0, ret);
}

void test_options_get_should_return_correct_option_value(void)
{
	struct net_if *net_if = net_if_get_default();
	uint8_t option = LTE_CONNECTIVITY_IF_DOWN_MODEM_SHUTDOWN;
	size_t len;
	int ret;

	ret = conn_mgr_if_set_opt(net_if,
				  LTE_CONNECTIVITY_IF_DOWN,
				  (const void *)&option,
				  sizeof(option));

	TEST_ASSERT_EQUAL(0, ret);

	ret = conn_mgr_if_get_opt(net_if, LTE_CONNECTIVITY_IF_DOWN, (void *)&option, &len);

	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(LTE_CONNECTIVITY_IF_DOWN_MODEM_SHUTDOWN, option);
}

void test_options_get_should_return_error_if_wrong_option_name_is_used(void)
{
	struct net_if *net_if = net_if_get_default();
	uint8_t option;
	size_t len;
	int ret;

	ret = conn_mgr_if_get_opt(net_if, UINT8_MAX, (void *)&option, &len);

	TEST_ASSERT_EQUAL(-ENOPROTOOPT, ret);
}

/* PDN notifications */

/* IPv4 */

void test_pdn_ipv4_address_should_be_added_on_activated(void)
{
	ipv4_address_add();
}

void test_pdn_ipv4_fails_to_be_added_on_activated(void)
{
	struct net_if *net_if = net_if_get_default();

	__mock_nrf_modem_at_scanf_ExpectAndReturn(
			"AT+CGPADDR=0", "+CGPADDR: %*d,\"%46[.:0-9A-F]\",\"%46[:0-9A-F]\"", 0);
	__cmock_lte_lc_func_mode_set_ExpectAndReturn(LTE_LC_FUNC_MODE_DEACTIVATE_LTE, 0);

	pdn_event_handler_callback(0, PDN_EVENT_ACTIVATED, 0);

	TEST_ASSERT_FALSE(net_if_ipv4_get_global_addr(net_if, NET_ADDR_PREFERRED));
}

void test_pdn_ipv4_address_should_be_removed_on_deactivated(void)
{
	ipv4_address_add();

	struct net_if *net_if = net_if_get_default();

	__cmock_nrf_modem_is_initialized_ExpectAndReturn(1);

	pdn_event_handler_callback(0, PDN_EVENT_DEACTIVATED, 0);

	TEST_ASSERT_TRUE(net_if_flag_is_set(net_if, NET_IF_DORMANT));
	TEST_ASSERT_FALSE(net_if_ipv4_get_global_addr(net_if, NET_ADDR_PREFERRED));
}

/* IPv6 */

void test_pdn_ipv6_address_should_be_added_on_ipv6_up(void)
{
	ipv6_address_add();
}

void test_pdn_ipv6_fails_to_be_added_on_ipv6_up(void)
{
	struct net_if *net_if = net_if_get_default();

	__mock_nrf_modem_at_scanf_ExpectAndReturn(
			"AT+CGPADDR=0", "+CGPADDR: %*d,\"%46[.:0-9A-F]\",\"%46[:0-9A-F]\"", 0);
	__cmock_lte_lc_func_mode_set_ExpectAndReturn(LTE_LC_FUNC_MODE_DEACTIVATE_LTE, 0);

	pdn_event_handler_callback(0, PDN_EVENT_ACTIVATED, 0);

	TEST_ASSERT_FALSE(net_if_ipv6_get_global_addr(NET_ADDR_PREFERRED, &net_if));
}

void test_pdn_ipv6_address_should_be_removed_on_ipv6_down(void)
{
	struct net_if *net_if = net_if_get_default();

	ipv6_address_add();

	pdn_event_handler_callback(0, PDN_EVENT_IPV6_DOWN, 0);

	TEST_ASSERT_FALSE(net_if_ipv6_get_global_addr(NET_ADDR_PREFERRED, &net_if));
}

void test_pdn_should_call_lte_disconnect_on_pdn_deactivated(void)
{
	struct net_if *net_if = net_if_get_default();

	__cmock_nrf_modem_is_initialized_ExpectAndReturn(0);

	TEST_ASSERT_EQUAL(0, conn_mgr_if_set_flag(net_if, CONN_MGR_IF_PERSISTENT, false));

	pdn_event_handler_callback(0, PDN_EVENT_DEACTIVATED, 0);

	TEST_ASSERT_TRUE(net_if_flag_is_set(net_if, NET_IF_DORMANT));
}

int main(void)
{
	(void)unity_main();

	return 0;
}
