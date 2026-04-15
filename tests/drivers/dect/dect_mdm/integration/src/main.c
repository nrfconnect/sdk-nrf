/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <unity.h>
#include <zephyr/kernel.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef CONFIG_BOARD_NATIVE_SIM
#include "posix_board_if.h"
#endif

/* Include test functions from test_dect_integration.c (failure tests first, then success init) */
extern void test_dect_net_mgmt_invalid_iface_errors(void);
extern void test_dect_configure_callback_failure(void);
extern void test_dect_activation_callback_failure(void);
extern void test_dect_stack_initialization(void);
extern void test_dect_settings_reset_to_defaults(void);
extern void test_dect_scan_request_band1(void);
extern void test_dect_pt_association_request(void);
extern void test_dect_pt_neighbor_list_info_req(void);
extern void test_dect_pt_status_info_req(void);
extern void test_dect_pt_global_address_assign(void);
extern void test_dect_pt_global_address_change(void);
extern void test_dect_pt_global_address_removal(void);
extern void test_dect_pt_association_release(void);
extern void test_dect_pt_conn_mgr_connect(void);
extern void test_dect_pt_conn_mgr_disconnect(void);
extern void test_dect_ft_activate(void);
extern void test_dect_ft_rssi_scan(void);
extern void test_dect_deactivate(void);
extern void test_dect_deactivated_requests_fail(void);
extern void test_dect_ft_configuration(void);
extern void test_dect_pt_complete_workflow(void);
extern void test_dect_ft_cluster_start(void);
extern void test_dect_ft_network_create(void);
extern void test_dect_ft_cluster_reconfigure(void);
extern void test_dect_ft_sink_global_address_assign(void);
extern void test_dect_ft_sink_router_del(void);
extern void test_dect_ft_sink_router_nbr_del(void);
extern void test_dect_ft_cluster_associate_child_with_global_address(void);
extern void test_dect_ft_cluster_associate_child_with_global_address_2(void);
extern void test_dect_ft_sink_global_address_change(void);
extern void test_dect_ft_sckt_packet_rx_tx(void);
extern void test_dect_ft_local_multicast_tx(void);
extern void test_dect_ft_network_remove(void);
extern void test_dect_ft_sink_down(void);
extern void test_dect_ft_conn_mgr_connect(void);
extern void test_dect_ft_conn_mgr_disconnect(void);
extern void test_dect_ft_cluster_info(void);
extern void test_dect_ft_status(void);
extern void test_dect_ft_cluster_stop_returns_enotsup(void);
extern void test_dect_ft_nw_beacon_start(void);
extern void test_dect_ft_nw_beacon_stop(void);
extern void test_dect_ft_cluster_associate_child(void);
extern void test_dect_ft_cluster_associated_icmp_ping(void);
extern void test_dect_ft_cluster_associated_child_dissociates(void);
extern void test_dect_settings_write_scopes_and_reset(void);
extern void test_dect_association_callback_failure(void);
extern void test_dect_association_rejected(void);

/* Test result tracking structure */
typedef struct {
	const char *function_name;
	bool passed;
} test_result_t;

/* Global test result tracking array (order = run order; failure tests before success init) */
static test_result_t test_results[] = {
	{"test_dect_net_mgmt_invalid_iface_errors", false},
	{"test_dect_configure_callback_failure", false},
	{"test_dect_activation_callback_failure", false},
	{"test_dect_stack_initialization", false},
	{"test_dect_settings_reset_to_defaults", false},
	{"test_dect_scan_request_band1", false},
	{"test_dect_pt_association_request", false},
	{"test_dect_pt_neighbor_list_info_req", false},
	{"test_dect_pt_status_info_req", false},
	{"test_dect_pt_global_address_assign", false},
	{"test_dect_pt_global_address_change", false},
	{"test_dect_pt_global_address_removal", false},
	{"test_dect_pt_association_release", false},
	{"test_dect_association_callback_failure", false},
	{"test_dect_association_rejected", false},
	{"test_dect_deactivate", false},
	{"test_dect_deactivated_requests_fail", false},
	{"test_dect_ft_configuration", false},
	{"test_dect_ft_activate", false},
	{"test_dect_ft_rssi_scan", false},
	{"test_dect_ft_cluster_start", false},
	{"test_dect_ft_cluster_info", false},
	{"test_dect_ft_cluster_stop_returns_enotsup", false},
	{"test_dect_ft_nw_beacon_start", false},
	{"test_dect_ft_nw_beacon_stop", false},
	{"test_dect_ft_cluster_associate_child", false},
	{"test_dect_ft_status", false},
	{"test_dect_ft_cluster_associated_icmp_ping", false},
	{"test_dect_ft_cluster_associated_child_dissociates", false},
	{"test_dect_settings_write_scopes_and_reset", false},
	{"test_dect_deactivate_rerun", false},
	{"test_dect_ft_configuration_rerun", false},
	{"test_dect_ft_activate_rerun", false},
	{"test_dect_ft_network_create", false},
	{"test_dect_ft_cluster_reconfigure", false},
	{"test_dect_ft_sink_global_address_assign", false},
	{"test_dect_ft_sink_router_del", false},
	{"test_dect_ft_sink_router_nbr_del", false},
	{"test_dect_ft_cluster_associate_child_with_global_address", false},
	{"test_dect_ft_cluster_associate_child_with_global_address_2", false},
	{"test_dect_ft_sink_global_address_change", false},
	{"test_dect_ft_sckt_packet_rx_tx", false},
	{"test_dect_ft_local_multicast_tx", false},
	{"test_dect_ft_network_remove", false},
	{"test_dect_ft_sink_down", false},
	{"test_dect_ft_conn_mgr_connect", false},
	{"test_dect_ft_conn_mgr_disconnect", false},
	{"test_dect_ft_sink_down_before_pt", false},
	{"test_dect_pt_conn_mgr_connect", false},
	{"test_dect_pt_conn_mgr_disconnect", false}};

#define NUM_TESTS ARRAY_SIZE(test_results)

/**
 * Helper function to run a test and track its result
 */
#define RUN_TEST_AND_TRACK(test_func, test_index)                                                  \
	do {                                                                                       \
		int tests_before = Unity.TestFailures;                                             \
		RUN_TEST(test_func);                                                               \
		test_results[test_index].passed = (Unity.TestFailures == tests_before);            \
	} while (0)

/**
 * Main function - runs all Unity tests
 */
int main(void)
{
	/* Initialize Unity */
	UNITY_BEGIN();

	printk("=== DECT NR+ Simple Integration Tests ===\n");
	printk("Target: native_sim\n");
	printk("Framework: Unity\n");
	printk("Scope: Basic DECT operations with mocked libmodem\n\n");

	/* Run DECT integration tests with result tracking */
	RUN_TEST_AND_TRACK(test_dect_net_mgmt_invalid_iface_errors, 0);
	RUN_TEST_AND_TRACK(test_dect_configure_callback_failure, 1);
	RUN_TEST_AND_TRACK(test_dect_activation_callback_failure, 2);
	RUN_TEST_AND_TRACK(test_dect_stack_initialization, 3);
	RUN_TEST_AND_TRACK(test_dect_settings_reset_to_defaults, 4);
	RUN_TEST_AND_TRACK(test_dect_scan_request_band1, 5);
	RUN_TEST_AND_TRACK(test_dect_pt_association_request, 6);
	RUN_TEST_AND_TRACK(test_dect_pt_neighbor_list_info_req, 7);
	RUN_TEST_AND_TRACK(test_dect_pt_status_info_req, 8);
	RUN_TEST_AND_TRACK(test_dect_pt_global_address_assign, 9);
	RUN_TEST_AND_TRACK(test_dect_pt_global_address_change, 10);
	RUN_TEST_AND_TRACK(test_dect_pt_global_address_removal, 11);
	RUN_TEST_AND_TRACK(test_dect_pt_association_release, 12);
	RUN_TEST_AND_TRACK(test_dect_association_callback_failure, 13);
	RUN_TEST_AND_TRACK(test_dect_association_rejected, 14);
	RUN_TEST_AND_TRACK(test_dect_deactivate, 15);
	RUN_TEST_AND_TRACK(test_dect_deactivated_requests_fail, 16);
	RUN_TEST_AND_TRACK(test_dect_ft_configuration, 17);
	RUN_TEST_AND_TRACK(test_dect_ft_activate, 18);
	RUN_TEST_AND_TRACK(test_dect_ft_rssi_scan, 19);
	RUN_TEST_AND_TRACK(test_dect_ft_cluster_start, 20);
	RUN_TEST_AND_TRACK(test_dect_ft_cluster_info, 21);
	RUN_TEST_AND_TRACK(test_dect_ft_cluster_stop_returns_enotsup, 22);
	RUN_TEST_AND_TRACK(test_dect_ft_nw_beacon_start, 23);
	RUN_TEST_AND_TRACK(test_dect_ft_nw_beacon_stop, 24);
	RUN_TEST_AND_TRACK(test_dect_ft_cluster_associate_child, 25);
	RUN_TEST_AND_TRACK(test_dect_ft_status, 26);
	RUN_TEST_AND_TRACK(test_dect_ft_cluster_associated_icmp_ping, 27);
	RUN_TEST_AND_TRACK(test_dect_ft_cluster_associated_child_dissociates, 28);
	RUN_TEST_AND_TRACK(test_dect_settings_write_scopes_and_reset, 29);
	/* Intentional reruns: same test functions (deactivate, ft_configuration, ft_activate)
	 * run again; results tracked under test_dect_*_rerun names for summary.
	 */
	RUN_TEST_AND_TRACK(test_dect_deactivate, 30);
	RUN_TEST_AND_TRACK(test_dect_ft_configuration, 31);
	RUN_TEST_AND_TRACK(test_dect_ft_activate, 32);
	RUN_TEST_AND_TRACK(test_dect_ft_network_create, 33);
	RUN_TEST_AND_TRACK(test_dect_ft_cluster_reconfigure, 34);
	RUN_TEST_AND_TRACK(test_dect_ft_sink_global_address_assign, 35);
	RUN_TEST_AND_TRACK(test_dect_ft_sink_router_del, 36);
	RUN_TEST_AND_TRACK(test_dect_ft_sink_router_nbr_del, 37);
	RUN_TEST_AND_TRACK(test_dect_ft_cluster_associate_child_with_global_address, 38);
	RUN_TEST_AND_TRACK(test_dect_ft_cluster_associate_child_with_global_address_2, 39);
	RUN_TEST_AND_TRACK(test_dect_ft_sink_global_address_change, 40);
	RUN_TEST_AND_TRACK(test_dect_ft_sckt_packet_rx_tx, 41);
	RUN_TEST_AND_TRACK(test_dect_ft_local_multicast_tx, 42);
	RUN_TEST_AND_TRACK(test_dect_ft_network_remove, 43);
	RUN_TEST_AND_TRACK(test_dect_ft_sink_down, 44);
	RUN_TEST_AND_TRACK(test_dect_ft_conn_mgr_connect, 45);
	RUN_TEST_AND_TRACK(test_dect_ft_conn_mgr_disconnect, 46);
	/* Rerun sink_down so L2 removes FT global from DECT iface
	 * (NET_EVENT_IF_DOWN → prefix removed);
	 * then PT conn_mgr_connect sees no stale global.
	 */
	RUN_TEST_AND_TRACK(test_dect_ft_sink_down, 47);
	RUN_TEST_AND_TRACK(test_dect_pt_conn_mgr_connect, 48);
	RUN_TEST_AND_TRACK(test_dect_pt_conn_mgr_disconnect, 49);
	/* TODO more tests & coverity */

	/* Capture Unity statistics before UNITY_END() */
	int total_tests = Unity.NumberOfTests;
	int failed_tests = Unity.TestFailures;
	int passed_tests = total_tests - failed_tests;
	int success_rate_int = total_tests > 0 ? (passed_tests * 100) / total_tests : 0;

	/* Complete and report test results */
	int result = UNITY_END();

	/* Print minimal test summary with detailed test case status */
	printk("\n");
	printk("=== DECT NR+ TEST SUMMARY ===\n");
	printk("Tests: %d | Passed: %d | Failed: %d | Rate: %d%%\n", total_tests, passed_tests,
	       failed_tests, success_rate_int);

	printk("\nTEST CASE RESULTS:\n");
	printk("------------------\n");

	/* Print results based on actual test outcomes */
	for (int i = 0; i < NUM_TESTS; i++) {
		if (test_results[i].passed) {
			printk("\033[32m[PASS]\033[0m %-35s\n", test_results[i].function_name);
		} else {
			printk("\033[31m[FAIL]\033[0m %-35s\n", test_results[i].function_name);
		}
	}

	if (result == 0) {
		printk("\n\033[32mALL TESTS PASSED!\033[0m\n");
		printk("\033[32mComplete DECT lifecycle: Init→Reset→Scan→Associate→"
		       "Release→Deactivate→FT config\033[0m\n");
		/* Twister expects this marker for non-ztest Unity apps. */
		printf("PROJECT EXECUTION SUCCESSFUL\n");
	} else {
		printk("\n\033[31mSOME TESTS FAILED!\033[0m\n");
		printk("\033[33mCheck individual test results above for details\033[0m\n");
		printf("PROJECT EXECUTION FAILED\n");
	}
	printk("==============================\n");

	/* Exit automatically for native_sim - prevents hanging after tests complete */
	printk("Tests completed with result: %d\n", result);

#ifdef CONFIG_BOARD_NATIVE_SIM
	/*
	 * Explicitly terminate the native_sim process so Twister does not wait for
	 * its timeout after tests finish. This also flushes coverage data when
	 * CONFIG_COVERAGE is enabled.
	 */
	fflush(stdout);
	fflush(stderr);
	k_msleep(20);
	posix_exit(result);

	/* Should be unreachable: */
	return 1;
#else
	return result;
#endif
}
