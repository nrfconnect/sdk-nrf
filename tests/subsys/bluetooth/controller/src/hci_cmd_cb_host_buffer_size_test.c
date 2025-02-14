/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <unity.h>
#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include <zephyr/sys/util.h>

#include "hci_internal_wrappers.h"
#include "cmock_sdc_hci_cmd_controller_baseband.h"

#define TEST_HOST_ACL_DATA_PCKT_LNGTH  55
#define TEST_HOST_SYNC_DATA_PCKT_LNGTH 55
#define TEST_HOST_NUM_SYNC_DATA_PCKTS  5555

/* The unity_main is not declared in any header file. It is only defined in the generated test
 * runner because of ncs' unity configuration. It is therefore declared here to avoid a compiler
 * warning.
 */
extern int unity_main(void);

typedef struct {
	uint16_t acl_input_pckts;
	uint16_t exp_acl_pckts;
} test_vector_t;

static const test_vector_t test_vectors[] = {
	{5, 5},					  /* Input within range, no adjustment needed */
	{15, (CONFIG_BT_BUF_CMD_TX_COUNT - 1)},	  /* Input exceeds TX buffer count - 1 limit */
	{0xFF, (CONFIG_BT_BUF_CMD_TX_COUNT - 1)}, /* Maximum input value, corner case */
};

void test_sdc_hci_cmd_cb_host_buffer_size_wrapper(void)
{
	sdc_hci_cmd_cb_host_buffer_size_t cmd_params;

	cmd_params.host_acl_data_packet_length = TEST_HOST_ACL_DATA_PCKT_LNGTH;
	cmd_params.host_sync_data_packet_length = TEST_HOST_SYNC_DATA_PCKT_LNGTH;
	cmd_params.host_total_num_sync_data_packets = TEST_HOST_NUM_SYNC_DATA_PCKTS;

	for (size_t i = 0; i < ARRAY_SIZE(test_vectors); i++) {
		const test_vector_t *v = &test_vectors[i];

		sdc_hci_cmd_cb_host_buffer_size_t exp_cmd_params = cmd_params;

		exp_cmd_params.host_total_num_acl_data_packets = v->exp_acl_pckts;
		__cmock_sdc_hci_cmd_cb_host_buffer_size_ExpectAndReturn(&exp_cmd_params, 0);

		cmd_params.host_total_num_acl_data_packets = v->acl_input_pckts;
		sdc_hci_cmd_cb_host_buffer_size_wrapper(&cmd_params);
	}
}

int main(void)
{
	(void)unity_main();

	return 0;
}
