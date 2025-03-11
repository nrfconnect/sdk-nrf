/*
 * Copyright (c) 2025 Nordic Semiconductor
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MSPI_ERROR_CASES_H
#define MSPI_ERROR_CASES_H

struct mspi_test_results {
	int    test_01a_MSPI_OP_MODE_CONTROLLER;
	int    test_01b_MSPI_OP_MODE_PERIPHERAL;
	int    test_02a_MSPI_HALF_DUPLEX;
	int    test_02b_MSPI_FULL_DUPLEX;
	int    test_03a_dqs_support_true;
	int    test_04a_sw_multi_periph_true;
	int    test_05a_num_ce_gpios_max;
	int    test_06a_num_periph_max;
	int    test_07a_max_freq_max;
	int    test_08a_re_init_true;
	int    test_08b_re_init_false;
	int    test_09a_MSPI_DEVICE_CONFIG_NONE;
	int    test_10a_MSPI_DEVICE_CONFIG_CE_NUM_MAX;
	int    test_11a_MSPI_DEVICE_CONFIG_FREQUENCY_MAX;
	int    test_11b_MSPI_DEVICE_CONFIG_FREQUENCY_MIN;
	int    test_12a_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_SINGLE;
	int    test_12b_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_DUAL;
	int    test_12c_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_DUAL_1_1_2;
	int    test_12d_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_DUAL_1_2_2;
	int    test_12e_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_QUAD;
	int    test_12f_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_QUAD_1_1_4;
	int    test_12g_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_QUAD_1_4_4;
	int    test_12h_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_OCTAL;
	int    test_12i_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_OCTAL_1_1_8;
	int    test_12j_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_OCTAL_1_8_8;
	int    test_12k_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_HEX;
	int    test_12l_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_HEX_8_8_16;
	int    test_12m_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_HEX_8_16_16;
	int    test_13a_MSPI_DEVICE_CONFIG_DATA_RATE_eq_MSPI_DATA_RATE_SINGLE;
	int    test_13b_MSPI_DEVICE_CONFIG_DATA_RATE_eq_MSPI_DATA_RATE_S_S_D;
	int    test_13c_MSPI_DEVICE_CONFIG_DATA_RATE_eq_MSPI_DATA_RATE_S_D_D;
	int    test_13d_MSPI_DEVICE_CONFIG_DATA_RATE_eq_MSPI_DATA_RATE_DUAL;
	int    test_14a_MSPI_DEVICE_CONFIG_CPP_eq_MSPI_CPP_MODE_0;
	int    test_14b_MSPI_DEVICE_CONFIG_CPP_eq_MSPI_CPP_MODE_1;
	int    test_14c_MSPI_DEVICE_CONFIG_CPP_eq_MSPI_CPP_MODE_2;
	int    test_14d_MSPI_DEVICE_CONFIG_CPP_eq_MSPI_CPP_MODE_3;
	int    test_15a_MSPI_DEVICE_CONFIG_ENDIAN_eq_MSPI_XFER_LITTLE_ENDIAN;
	int    test_15b_MSPI_DEVICE_CONFIG_ENDIAN_eq_MSPI_XFER_BIG_ENDIAN;
	int    test_16a_MSPI_DEVICE_CONFIG_CE_POL_eq_MSPI_CE_ACTIVE_LOW;
	int    test_16b_MSPI_DEVICE_CONFIG_CE_POL_eq_MSPI_CE_ACTIVE_HIGH;
	int    test_17a_MSPI_DEVICE_CONFIG_DQS_true;
	int    test_17b_MSPI_DEVICE_CONFIG_DQS_false;
	int    test_18a_MSPI_DEVICE_CONFIG_RX_DUMMY_MAX;
	int    test_19a_MSPI_DEVICE_CONFIG_TX_DUMMY_MAX;
	int    test_20a_MSPI_DEVICE_CONFIG_MEM_BOUND_MAX;
	int    test_21a_MSPI_DEVICE_CONFIG_BREAK_TIME_MAX;
	int    test_22a_mspi_xfer_async_true;
	int    test_23a_mspi_xfer_MSPI_PIO;
	int    test_23b_mspi_xfer_MSPI_DMA;
	int    test_24a_mspi_xfer_tx_dummy_max;
	int    test_25a_mspi_xfer_rx_dummy_max;
	int    test_26a_mspi_xfer_cmd_length_max;
	int    test_27a_mspi_xfer_addr_length_max;
	int    test_28a_mspi_xfer_hold_ce_true;
	int    test_28b_mspi_xfer_hold_ce_false;
	int    test_30a_mspi_xfer_priority_0;
	int    test_30b_mspi_xfer_priority_1;
	int    test_31a_mspi_xfer_packets_NULL;
	int    test_32a_mspi_xfer_num_packet_0;
	int    test_33a_mspi_xfer_timeout_max;
	int    test_34a_mspi_xfer_packet_num_bytes_max;
};

#endif
