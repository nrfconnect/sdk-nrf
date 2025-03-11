/*
 * Copyright (c) 2025 Nordic Semiconductor
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mspi.h>
#include <zephyr/drivers/mspi_emul.h>
#include <zephyr/ztest.h>
#include "mspi_error_cases.h"

#if defined(CONFIG_SOC_SERIES_NRF54LX) || defined(CONFIG_SOC_NRF54H20)
const struct mspi_test_results expected_result = {
	.test_01a_MSPI_OP_MODE_CONTROLLER          = 0,
	.test_01b_MSPI_OP_MODE_PERIPHERAL          = -ENOTSUP,
	.test_02a_MSPI_HALF_DUPLEX                 = 0,
	.test_02b_MSPI_FULL_DUPLEX                 = 0,
	.test_03a_dqs_support_true                 = -ENOTSUP,
	.test_04a_sw_multi_periph_true             = 0,
	.test_05a_num_ce_gpios_max                 = 0,
	.test_06a_num_periph_max                   = 0,
	.test_07a_max_freq_max                     = -ENOTSUP,
	.test_08a_re_init_true                     = 0,
	.test_08b_re_init_false                    = 0,
	.test_09a_MSPI_DEVICE_CONFIG_NONE          = 0,
	.test_10a_MSPI_DEVICE_CONFIG_CE_NUM_MAX    = 0,
	.test_11a_MSPI_DEVICE_CONFIG_FREQUENCY_MAX = -EINVAL,
	.test_11b_MSPI_DEVICE_CONFIG_FREQUENCY_MIN = -EINVAL,
	.test_12a_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_SINGLE = 0,
	.test_12b_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_DUAL = -ENOTSUP,
	.test_12c_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_DUAL_1_1_2 = -ENOTSUP,
	.test_12d_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_DUAL_1_2_2 = -ENOTSUP,
	.test_12e_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_QUAD = 0,
	.test_12f_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_QUAD_1_1_4 = 0,
	.test_12g_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_QUAD_1_4_4 = 0,
	.test_12h_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_OCTAL = -ENOTSUP,
	.test_12i_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_OCTAL_1_1_8 = -ENOTSUP,
	.test_12j_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_OCTAL_1_8_8 = -ENOTSUP,
	.test_12k_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_HEX = -ENOTSUP,
	.test_12l_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_HEX_8_8_16 = -ENOTSUP,
	.test_12m_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_HEX_8_16_16 = -ENOTSUP,
	.test_13a_MSPI_DEVICE_CONFIG_DATA_RATE_eq_MSPI_DATA_RATE_SINGLE = 0,
	.test_13b_MSPI_DEVICE_CONFIG_DATA_RATE_eq_MSPI_DATA_RATE_S_S_D = -ENOTSUP,
	.test_13c_MSPI_DEVICE_CONFIG_DATA_RATE_eq_MSPI_DATA_RATE_S_D_D = -ENOTSUP,
	.test_13d_MSPI_DEVICE_CONFIG_DATA_RATE_eq_MSPI_DATA_RATE_DUAL = -ENOTSUP,
	.test_14a_MSPI_DEVICE_CONFIG_CPP_eq_MSPI_CPP_MODE_0 = 0,
	.test_14b_MSPI_DEVICE_CONFIG_CPP_eq_MSPI_CPP_MODE_1 = 0,
	.test_14c_MSPI_DEVICE_CONFIG_CPP_eq_MSPI_CPP_MODE_2 = 0,
	.test_14d_MSPI_DEVICE_CONFIG_CPP_eq_MSPI_CPP_MODE_3 = 0,
	.test_15a_MSPI_DEVICE_CONFIG_ENDIAN_eq_MSPI_XFER_LITTLE_ENDIAN = 0,
	.test_15b_MSPI_DEVICE_CONFIG_ENDIAN_eq_MSPI_XFER_BIG_ENDIAN = 0,
	.test_16a_MSPI_DEVICE_CONFIG_CE_POL_eq_MSPI_CE_ACTIVE_LOW = 0,
	.test_16b_MSPI_DEVICE_CONFIG_CE_POL_eq_MSPI_CE_ACTIVE_HIGH = 0,
	.test_17a_MSPI_DEVICE_CONFIG_DQS_true = -ENOTSUP,
	.test_17b_MSPI_DEVICE_CONFIG_DQS_false = 0,
	.test_18a_MSPI_DEVICE_CONFIG_RX_DUMMY_MAX = 0,
	.test_19a_MSPI_DEVICE_CONFIG_TX_DUMMY_MAX = 0,
	.test_20a_MSPI_DEVICE_CONFIG_MEM_BOUND_MAX = -ENOTSUP,
	.test_21a_MSPI_DEVICE_CONFIG_BREAK_TIME_MAX = -ENOTSUP,
	.test_22a_mspi_xfer_async_true = -ENOTSUP,
	.test_23a_mspi_xfer_MSPI_PIO = 0,
	.test_23b_mspi_xfer_MSPI_DMA = 0,
	.test_24a_mspi_xfer_tx_dummy_max = 0,
	.test_25a_mspi_xfer_rx_dummy_max = 0,
	.test_26a_mspi_xfer_cmd_length_max = 0,
	.test_27a_mspi_xfer_addr_length_max = 0,
	.test_28a_mspi_xfer_hold_ce_true = 0,
	.test_28b_mspi_xfer_hold_ce_false = 0,
	.test_30a_mspi_xfer_priority_0 = 0,
	.test_30b_mspi_xfer_priority_1 = 0,
	.test_31a_mspi_xfer_packets_NULL = -EFAULT,
	.test_32a_mspi_xfer_num_packet_0 = -EFAULT,
	.test_33a_mspi_xfer_timeout_max = -EFAULT,
	.test_34a_mspi_xfer_packet_num_bytes_max = -EINVAL,
};
#else
/* Default configuration - all supported. */
const struct mspi_test_results expected_result = {
	.test_01a_MSPI_OP_MODE_CONTROLLER          = 0,
	.test_01b_MSPI_OP_MODE_PERIPHERAL          = 0,
	.test_02a_MSPI_HALF_DUPLEX                 = 0,
	.test_02b_MSPI_FULL_DUPLEX                 = 0,
	.test_03a_dqs_support_true                 = 0,
	.test_04a_sw_multi_periph_true             = 0,
	.test_05a_num_ce_gpios_max                 = 0,
	.test_06a_num_periph_max                   = 0,
	.test_07a_max_freq_max                     = 0,
	.test_08a_re_init_true                     = 0,
	.test_08b_re_init_false                    = 0,
	.test_09a_MSPI_DEVICE_CONFIG_NONE          = 0,
	.test_10a_MSPI_DEVICE_CONFIG_CE_NUM_MAX    = 0,
	.test_11a_MSPI_DEVICE_CONFIG_FREQUENCY_MAX = 0,
	.test_11b_MSPI_DEVICE_CONFIG_FREQUENCY_MIN = 0,
	.test_12a_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_SINGLE = 0,
	.test_12b_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_DUAL = 0,
	.test_12c_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_DUAL_1_1_2 = 0,
	.test_12d_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_DUAL_1_2_2 = 0,
	.test_12e_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_QUAD = 0,
	.test_12f_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_QUAD_1_1_4 = 0,
	.test_12g_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_QUAD_1_4_4 = 0,
	.test_12h_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_OCTAL = 0,
	.test_12i_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_OCTAL_1_1_8 = 0,
	.test_12j_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_OCTAL_1_8_8 = 0,
	.test_12k_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_HEX = 0,
	.test_12l_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_HEX_8_8_16 = 0,
	.test_12m_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_HEX_8_16_16 = 0,
	.test_13a_MSPI_DEVICE_CONFIG_DATA_RATE_eq_MSPI_DATA_RATE_SINGLE = 0,
	.test_13b_MSPI_DEVICE_CONFIG_DATA_RATE_eq_MSPI_DATA_RATE_S_S_D = 0,
	.test_13c_MSPI_DEVICE_CONFIG_DATA_RATE_eq_MSPI_DATA_RATE_S_D_D = 0,
	.test_13d_MSPI_DEVICE_CONFIG_DATA_RATE_eq_MSPI_DATA_RATE_DUAL = 0,
	.test_14a_MSPI_DEVICE_CONFIG_CPP_eq_MSPI_CPP_MODE_0 = 0,
	.test_14b_MSPI_DEVICE_CONFIG_CPP_eq_MSPI_CPP_MODE_1 = 0,
	.test_14c_MSPI_DEVICE_CONFIG_CPP_eq_MSPI_CPP_MODE_2 = 0,
	.test_14d_MSPI_DEVICE_CONFIG_CPP_eq_MSPI_CPP_MODE_3 = 0,
	.test_15a_MSPI_DEVICE_CONFIG_ENDIAN_eq_MSPI_XFER_LITTLE_ENDIAN = 0,
	.test_15b_MSPI_DEVICE_CONFIG_ENDIAN_eq_MSPI_XFER_BIG_ENDIAN = 0,
	.test_16a_MSPI_DEVICE_CONFIG_CE_POL_eq_MSPI_CE_ACTIVE_LOW = 0,
	.test_16b_MSPI_DEVICE_CONFIG_CE_POL_eq_MSPI_CE_ACTIVE_HIGH = 0,
	.test_17a_MSPI_DEVICE_CONFIG_DQS_true = 0,
	.test_17b_MSPI_DEVICE_CONFIG_DQS_false = 0,
	.test_18a_MSPI_DEVICE_CONFIG_RX_DUMMY_MAX = 0,
	.test_19a_MSPI_DEVICE_CONFIG_TX_DUMMY_MAX = 0,
	.test_20a_MSPI_DEVICE_CONFIG_MEM_BOUND_MAX = 0,
	.test_21a_MSPI_DEVICE_CONFIG_BREAK_TIME_MAX = 0,
	.test_22a_mspi_xfer_async_true = 0,
	.test_23a_mspi_xfer_MSPI_PIO = 0,
	.test_23b_mspi_xfer_MSPI_DMA = 0,
	.test_24a_mspi_xfer_tx_dummy_max = 0,
	.test_25a_mspi_xfer_rx_dummy_max = 0,
	.test_26a_mspi_xfer_cmd_length_max = 0,
	.test_27a_mspi_xfer_addr_length_max = 0,
	.test_28a_mspi_xfer_hold_ce_true = 0,
	.test_28b_mspi_xfer_hold_ce_false = 0,
	.test_30a_mspi_xfer_priority_0 = 0,
	.test_30b_mspi_xfer_priority_1 = 0,
	.test_31a_mspi_xfer_packets_NULL = 0,
	.test_32a_mspi_xfer_num_packet_0 = 0,
	.test_33a_mspi_xfer_timeout_max = 0,
	.test_34a_mspi_xfer_packet_num_bytes_max = 0,
};
#endif

#define MSPI_BUS_NODE       DT_ALIAS(mspi0)

static struct gpio_dt_spec ce_gpios[] = MSPI_CE_GPIOS_DT_SPEC_GET(MSPI_BUS_NODE);

static struct mspi_dev_id dev_id[] = {
	DT_FOREACH_CHILD_STATUS_OKAY_SEP(MSPI_BUS_NODE, MSPI_DEVICE_ID_DT, (,))
};

static struct mspi_dev_cfg device_cfg[] = {
	DT_FOREACH_CHILD_STATUS_OKAY_SEP(MSPI_BUS_NODE, MSPI_DEVICE_CONFIG_DT, (,))
};

static const struct mspi_cfg default_hw_cfg = {
	.channel_num      = 0,
	.op_mode          = DT_ENUM_IDX_OR(MSPI_BUS_NODE, op_mode, MSPI_OP_MODE_CONTROLLER),
	.duplex           = MSPI_FULL_DUPLEX,
	.dqs_support      = DT_PROP_OR(MSPI_BUS_NODE, dqs_support, false),
	.sw_multi_periph  = DT_PROP_OR(MSPI_BUS_NODE, sw_multi_periph, false),
	.ce_group         = ce_gpios,
	.num_ce_gpios     = ARRAY_SIZE(ce_gpios),
	.num_periph       = DT_CHILD_NUM(MSPI_BUS_NODE),
	.max_freq         = DT_PROP(MSPI_BUS_NODE, clock_frequency),
	.re_init          = true,
};

uint8_t memc_write_buffer[256];

static struct mspi_xfer_packet packet1[] = {
	{
		.dir          = MSPI_TX,
		.cmd          = 0x12,
		.address      = 0,
		.num_bytes    = 256,
		.data_buf     = memc_write_buffer,
		.cb_mask      = MSPI_BUS_NO_CB,
	},
};

static struct mspi_xfer default_xfer = {
	.async            = false,
	.xfer_mode        = MSPI_DMA,
	.tx_dummy         = 0,
	.cmd_length       = 1,
	.addr_length      = 3,
	.priority         = 1,
	.packets          = (struct mspi_xfer_packet *)&packet1,
	.num_packet       = sizeof(packet1) / sizeof(struct mspi_xfer_packet),
};

ZTEST(mspi_error_cases, test_01a_MSPI_OP_MODE_CONTROLLER)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	struct mspi_cfg hw_cfg = default_hw_cfg;

	hw_cfg.op_mode = MSPI_OP_MODE_CONTROLLER;

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = hw_cfg,
	};

	ret = mspi_config(&spec);
	expected = expected_result.test_01a_MSPI_OP_MODE_CONTROLLER;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_01b_MSPI_OP_MODE_PERIPHERAL)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	struct mspi_cfg hw_cfg = default_hw_cfg;

	hw_cfg.op_mode = MSPI_OP_MODE_PERIPHERAL;

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = hw_cfg,
	};

	ret = mspi_config(&spec);
	expected = expected_result.test_01b_MSPI_OP_MODE_PERIPHERAL;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_02a_MSPI_HALF_DUPLEX)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	struct mspi_cfg hw_cfg = default_hw_cfg;

	hw_cfg.duplex = MSPI_HALF_DUPLEX;

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = hw_cfg,
	};

	ret = mspi_config(&spec);
	expected = expected_result.test_02a_MSPI_HALF_DUPLEX;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_02b_MSPI_FULL_DUPLEX)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	struct mspi_cfg hw_cfg = default_hw_cfg;

	hw_cfg.duplex = MSPI_FULL_DUPLEX;

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = hw_cfg,
	};

	ret = mspi_config(&spec);
	expected = expected_result.test_02b_MSPI_FULL_DUPLEX;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_03a_dqs_support_true)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	struct mspi_cfg hw_cfg = default_hw_cfg;

	hw_cfg.dqs_support = true;

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = hw_cfg,
	};

	ret = mspi_config(&spec);
	expected = expected_result.test_03a_dqs_support_true;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_04a_sw_multi_periph_true)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	struct mspi_cfg hw_cfg = default_hw_cfg;

	hw_cfg.sw_multi_periph = true;

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = hw_cfg,
	};

	ret = mspi_config(&spec);
	expected = expected_result.test_04a_sw_multi_periph_true;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_05a_num_ce_gpios_max)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	struct mspi_cfg hw_cfg = default_hw_cfg;

	hw_cfg.num_ce_gpios = 4294967295;

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = hw_cfg,
	};

	ret = mspi_config(&spec);
	expected = expected_result.test_05a_num_ce_gpios_max;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_06a_num_periph_max)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	struct mspi_cfg hw_cfg = default_hw_cfg;

	hw_cfg.num_periph = 4294967295;

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = hw_cfg,
	};

	ret = mspi_config(&spec);
	expected = expected_result.test_06a_num_periph_max;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_07a_max_freq_max)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	struct mspi_cfg hw_cfg = default_hw_cfg;

	hw_cfg.max_freq = 4294967295;

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = hw_cfg,
	};

	ret = mspi_config(&spec);
	expected = expected_result.test_07a_max_freq_max;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_08a_re_init_true)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	struct mspi_cfg hw_cfg = default_hw_cfg;

	hw_cfg.re_init = true;

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = hw_cfg,
	};

	ret = mspi_config(&spec);
	expected = expected_result.test_08a_re_init_true;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_08b_re_init_false)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	struct mspi_cfg hw_cfg = default_hw_cfg;

	hw_cfg.re_init = false;

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = hw_cfg,
	};

	ret = mspi_config(&spec);
	expected = expected_result.test_08b_re_init_false;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_09a_MSPI_DEVICE_CONFIG_NONE)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = default_hw_cfg,
	};

	ret = mspi_config(&spec);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_dev_config(mspi_bus, &dev_id[0], MSPI_DEVICE_CONFIG_NONE, &device_cfg[0]);
	expected = expected_result.test_09a_MSPI_DEVICE_CONFIG_NONE;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_10a_MSPI_DEVICE_CONFIG_CE_NUM_MAX)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = default_hw_cfg,
	};

	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.ce_num = 255;

	ret = mspi_config(&spec);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_dev_config(mspi_bus, &dev_id[0], MSPI_DEVICE_CONFIG_CE_NUM, &dev_cfg);
	expected = expected_result.test_10a_MSPI_DEVICE_CONFIG_CE_NUM_MAX;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_11a_MSPI_DEVICE_CONFIG_FREQUENCY_MAX)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = default_hw_cfg,
	};

	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.freq = 4294967295;

	ret = mspi_config(&spec);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_dev_config(mspi_bus, &dev_id[0], MSPI_DEVICE_CONFIG_FREQUENCY, &dev_cfg);
	expected = expected_result.test_11a_MSPI_DEVICE_CONFIG_FREQUENCY_MAX;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_11b_MSPI_DEVICE_CONFIG_FREQUENCY_MIN)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = default_hw_cfg,
	};

	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.freq = 1;

	ret = mspi_config(&spec);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_dev_config(mspi_bus, &dev_id[0], MSPI_DEVICE_CONFIG_FREQUENCY, &dev_cfg);
	expected = expected_result.test_11b_MSPI_DEVICE_CONFIG_FREQUENCY_MIN;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_12a_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_SINGLE)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = default_hw_cfg,
	};

	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.io_mode = MSPI_IO_MODE_SINGLE;

	ret = mspi_config(&spec);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_dev_config(mspi_bus, &dev_id[0], MSPI_DEVICE_CONFIG_IO_MODE, &dev_cfg);
	expected = expected_result.test_12a_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_SINGLE;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_12b_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_DUAL)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = default_hw_cfg,
	};

	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.io_mode = MSPI_IO_MODE_DUAL;

	ret = mspi_config(&spec);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_dev_config(mspi_bus, &dev_id[0], MSPI_DEVICE_CONFIG_IO_MODE, &dev_cfg);
	expected = expected_result.test_12b_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_DUAL;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_12c_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_DUAL_1_1_2)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = default_hw_cfg,
	};

	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.io_mode = MSPI_IO_MODE_DUAL_1_1_2;

	ret = mspi_config(&spec);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_dev_config(mspi_bus, &dev_id[0], MSPI_DEVICE_CONFIG_IO_MODE, &dev_cfg);
	expected = expected_result.test_12c_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_DUAL_1_1_2;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_12d_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_DUAL_1_2_2)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = default_hw_cfg,
	};

	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.io_mode = MSPI_IO_MODE_DUAL_1_2_2;

	ret = mspi_config(&spec);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_dev_config(mspi_bus, &dev_id[0], MSPI_DEVICE_CONFIG_IO_MODE, &dev_cfg);
	expected = expected_result.test_12d_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_DUAL_1_2_2;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_12e_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_QUAD)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = default_hw_cfg,
	};

	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.io_mode = MSPI_IO_MODE_QUAD;

	ret = mspi_config(&spec);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_dev_config(mspi_bus, &dev_id[0], MSPI_DEVICE_CONFIG_IO_MODE, &dev_cfg);
	expected = expected_result.test_12e_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_QUAD;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_12f_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_QUAD_1_1_4)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = default_hw_cfg,
	};

	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.io_mode = MSPI_IO_MODE_QUAD_1_1_4;

	ret = mspi_config(&spec);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_dev_config(mspi_bus, &dev_id[0], MSPI_DEVICE_CONFIG_IO_MODE, &dev_cfg);
	expected = expected_result.test_12f_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_QUAD_1_1_4;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_12g_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_QUAD_1_4_4)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = default_hw_cfg,
	};

	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.io_mode = MSPI_IO_MODE_QUAD_1_4_4;

	ret = mspi_config(&spec);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_dev_config(mspi_bus, &dev_id[0], MSPI_DEVICE_CONFIG_IO_MODE, &dev_cfg);
	expected = expected_result.test_12g_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_QUAD_1_4_4;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_12h_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_OCTAL)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = default_hw_cfg,
	};

	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.io_mode = MSPI_IO_MODE_OCTAL;

	ret = mspi_config(&spec);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_dev_config(mspi_bus, &dev_id[0], MSPI_DEVICE_CONFIG_IO_MODE, &dev_cfg);
	expected = expected_result.test_12h_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_OCTAL;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_12i_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_OCTAL_1_1_8)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = default_hw_cfg,
	};

	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.io_mode = MSPI_IO_MODE_OCTAL_1_1_8;

	ret = mspi_config(&spec);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_dev_config(mspi_bus, &dev_id[0], MSPI_DEVICE_CONFIG_IO_MODE, &dev_cfg);
	expected = expected_result.test_12i_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_OCTAL_1_1_8;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_12j_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_OCTAL_1_8_8)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = default_hw_cfg,
	};

	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.io_mode = MSPI_IO_MODE_OCTAL_1_8_8;

	ret = mspi_config(&spec);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_dev_config(mspi_bus, &dev_id[0], MSPI_DEVICE_CONFIG_IO_MODE, &dev_cfg);
	expected = expected_result.test_12j_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_OCTAL_1_8_8;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_12k_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_HEX)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = default_hw_cfg,
	};

	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.io_mode = MSPI_IO_MODE_HEX;

	ret = mspi_config(&spec);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_dev_config(mspi_bus, &dev_id[0], MSPI_DEVICE_CONFIG_IO_MODE, &dev_cfg);
	expected = expected_result.test_12k_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_HEX;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_12l_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_HEX_8_8_16)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = default_hw_cfg,
	};

	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.io_mode = MSPI_IO_MODE_HEX_8_8_16;

	ret = mspi_config(&spec);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_dev_config(mspi_bus, &dev_id[0], MSPI_DEVICE_CONFIG_IO_MODE, &dev_cfg);
	expected = expected_result.test_12l_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_HEX_8_8_16;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_12m_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_HEX_8_16_16)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = default_hw_cfg,
	};

	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.io_mode = MSPI_IO_MODE_HEX_8_16_16;

	ret = mspi_config(&spec);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_dev_config(mspi_bus, &dev_id[0], MSPI_DEVICE_CONFIG_IO_MODE, &dev_cfg);
	expected = expected_result.test_12m_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_HEX_8_16_16;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_13a_MSPI_DEVICE_CONFIG_DATA_RATE_eq_MSPI_DATA_RATE_SINGLE)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = default_hw_cfg,
	};

	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.data_rate = MSPI_DATA_RATE_SINGLE;

	ret = mspi_config(&spec);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_dev_config(mspi_bus, &dev_id[0], MSPI_DEVICE_CONFIG_DATA_RATE, &dev_cfg);
	expected = expected_result.test_13a_MSPI_DEVICE_CONFIG_DATA_RATE_eq_MSPI_DATA_RATE_SINGLE;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_13b_MSPI_DEVICE_CONFIG_DATA_RATE_eq_MSPI_DATA_RATE_S_S_D)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = default_hw_cfg,
	};

	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.data_rate = MSPI_DATA_RATE_S_S_D;

	ret = mspi_config(&spec);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_dev_config(mspi_bus, &dev_id[0], MSPI_DEVICE_CONFIG_DATA_RATE, &dev_cfg);
	expected = expected_result.test_13b_MSPI_DEVICE_CONFIG_DATA_RATE_eq_MSPI_DATA_RATE_S_S_D;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_13c_MSPI_DEVICE_CONFIG_DATA_RATE_eq_MSPI_DATA_RATE_S_D_D)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = default_hw_cfg,
	};

	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.data_rate = MSPI_DATA_RATE_S_D_D;

	ret = mspi_config(&spec);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_dev_config(mspi_bus, &dev_id[0], MSPI_DEVICE_CONFIG_DATA_RATE, &dev_cfg);
	expected = expected_result.test_13c_MSPI_DEVICE_CONFIG_DATA_RATE_eq_MSPI_DATA_RATE_S_D_D;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_13d_MSPI_DEVICE_CONFIG_DATA_RATE_eq_MSPI_DATA_RATE_DUAL)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = default_hw_cfg,
	};

	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.data_rate = MSPI_DATA_RATE_DUAL;

	ret = mspi_config(&spec);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_dev_config(mspi_bus, &dev_id[0], MSPI_DEVICE_CONFIG_DATA_RATE, &dev_cfg);
	expected = expected_result.test_13d_MSPI_DEVICE_CONFIG_DATA_RATE_eq_MSPI_DATA_RATE_DUAL;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_14a_MSPI_DEVICE_CONFIG_CPP_eq_MSPI_CPP_MODE_0)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = default_hw_cfg,
	};

	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.cpp = MSPI_CPP_MODE_0;

	ret = mspi_config(&spec);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_dev_config(mspi_bus, &dev_id[0], MSPI_DEVICE_CONFIG_CPP, &dev_cfg);
	expected = expected_result.test_14a_MSPI_DEVICE_CONFIG_CPP_eq_MSPI_CPP_MODE_0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_14b_MSPI_DEVICE_CONFIG_CPP_eq_MSPI_CPP_MODE_1)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = default_hw_cfg,
	};

	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.cpp = MSPI_CPP_MODE_1;

	ret = mspi_config(&spec);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_dev_config(mspi_bus, &dev_id[0], MSPI_DEVICE_CONFIG_CPP, &dev_cfg);
	expected = expected_result.test_14b_MSPI_DEVICE_CONFIG_CPP_eq_MSPI_CPP_MODE_1;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_14c_MSPI_DEVICE_CONFIG_CPP_eq_MSPI_CPP_MODE_2)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = default_hw_cfg,
	};

	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.cpp = MSPI_CPP_MODE_2;

	ret = mspi_config(&spec);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_dev_config(mspi_bus, &dev_id[0], MSPI_DEVICE_CONFIG_CPP, &dev_cfg);
	expected = expected_result.test_14c_MSPI_DEVICE_CONFIG_CPP_eq_MSPI_CPP_MODE_2;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_14d_MSPI_DEVICE_CONFIG_CPP_eq_MSPI_CPP_MODE_3)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = default_hw_cfg,
	};

	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.cpp = MSPI_CPP_MODE_3;

	ret = mspi_config(&spec);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_dev_config(mspi_bus, &dev_id[0], MSPI_DEVICE_CONFIG_CPP, &dev_cfg);
	expected = expected_result.test_14d_MSPI_DEVICE_CONFIG_CPP_eq_MSPI_CPP_MODE_3;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_15a_MSPI_DEVICE_CONFIG_ENDIAN_eq_MSPI_XFER_LITTLE_ENDIAN)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = default_hw_cfg,
	};

	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.endian = MSPI_XFER_LITTLE_ENDIAN;

	ret = mspi_config(&spec);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_dev_config(mspi_bus, &dev_id[0], MSPI_DEVICE_CONFIG_ENDIAN, &dev_cfg);
	expected = expected_result.test_15a_MSPI_DEVICE_CONFIG_ENDIAN_eq_MSPI_XFER_LITTLE_ENDIAN;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_15b_MSPI_DEVICE_CONFIG_ENDIAN_eq_MSPI_XFER_BIG_ENDIAN)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = default_hw_cfg,
	};

	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.endian = MSPI_XFER_BIG_ENDIAN;

	ret = mspi_config(&spec);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_dev_config(mspi_bus, &dev_id[0], MSPI_DEVICE_CONFIG_ENDIAN, &dev_cfg);
	expected = expected_result.test_15b_MSPI_DEVICE_CONFIG_ENDIAN_eq_MSPI_XFER_BIG_ENDIAN;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_16a_MSPI_DEVICE_CONFIG_CE_POL_eq_MSPI_CE_ACTIVE_LOW)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = default_hw_cfg,
	};

	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.ce_polarity = MSPI_CE_ACTIVE_LOW;

	ret = mspi_config(&spec);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_dev_config(mspi_bus, &dev_id[0], MSPI_DEVICE_CONFIG_CE_POL, &dev_cfg);
	expected = expected_result.test_16a_MSPI_DEVICE_CONFIG_CE_POL_eq_MSPI_CE_ACTIVE_LOW;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_16b_MSPI_DEVICE_CONFIG_CE_POL_eq_MSPI_CE_ACTIVE_HIGH)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = default_hw_cfg,
	};

	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.ce_polarity = MSPI_CE_ACTIVE_HIGH;

	ret = mspi_config(&spec);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_dev_config(mspi_bus, &dev_id[0], MSPI_DEVICE_CONFIG_CE_POL, &dev_cfg);
	expected = expected_result.test_16b_MSPI_DEVICE_CONFIG_CE_POL_eq_MSPI_CE_ACTIVE_HIGH;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_17a_MSPI_DEVICE_CONFIG_DQS_true)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = default_hw_cfg,
	};

	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.dqs_enable = true;

	ret = mspi_config(&spec);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_dev_config(mspi_bus, &dev_id[0], MSPI_DEVICE_CONFIG_DQS, &dev_cfg);
	expected = expected_result.test_17a_MSPI_DEVICE_CONFIG_DQS_true;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_17b_MSPI_DEVICE_CONFIG_DQS_false)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = default_hw_cfg,
	};

	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.dqs_enable = false;

	ret = mspi_config(&spec);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_dev_config(mspi_bus, &dev_id[0], MSPI_DEVICE_CONFIG_DQS, &dev_cfg);
	expected = expected_result.test_17b_MSPI_DEVICE_CONFIG_DQS_false;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_18a_MSPI_DEVICE_CONFIG_RX_DUMMY_MAX)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = default_hw_cfg,
	};

	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.rx_dummy = 65535;

	ret = mspi_config(&spec);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_dev_config(mspi_bus, &dev_id[0], MSPI_DEVICE_CONFIG_RX_DUMMY, &dev_cfg);
	expected = expected_result.test_18a_MSPI_DEVICE_CONFIG_RX_DUMMY_MAX;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_19a_MSPI_DEVICE_CONFIG_TX_DUMMY_MAX)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = default_hw_cfg,
	};

	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.tx_dummy = 65535;

	ret = mspi_config(&spec);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_dev_config(mspi_bus, &dev_id[0], MSPI_DEVICE_CONFIG_TX_DUMMY, &dev_cfg);
	expected = expected_result.test_19a_MSPI_DEVICE_CONFIG_TX_DUMMY_MAX;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_20a_MSPI_DEVICE_CONFIG_MEM_BOUND_MAX)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = default_hw_cfg,
	};

	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.mem_boundary = 4294967295;

	ret = mspi_config(&spec);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_dev_config(mspi_bus, &dev_id[0], MSPI_DEVICE_CONFIG_MEM_BOUND, &dev_cfg);
	expected = expected_result.test_20a_MSPI_DEVICE_CONFIG_MEM_BOUND_MAX;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_21a_MSPI_DEVICE_CONFIG_BREAK_TIME_MAX)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = default_hw_cfg,
	};

	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.time_to_break = 4294967295;

	ret = mspi_config(&spec);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_dev_config(mspi_bus, &dev_id[0], MSPI_DEVICE_CONFIG_BREAK_TIME, &dev_cfg);
	expected = expected_result.test_21a_MSPI_DEVICE_CONFIG_BREAK_TIME_MAX;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_22a_mspi_xfer_async_true)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = default_hw_cfg,
	};

	struct mspi_xfer xfer = default_xfer;

	xfer.async = true;

	ret = mspi_config(&spec);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_dev_config(mspi_bus, &dev_id[0], MSPI_DEVICE_CONFIG_ALL, &device_cfg[0]);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_transceive(mspi_bus, &dev_id[0], &xfer);
	expected = expected_result.test_22a_mspi_xfer_async_true;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_23a_mspi_xfer_MSPI_PIO)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = default_hw_cfg,
	};

	struct mspi_xfer xfer = default_xfer;

	xfer.xfer_mode = MSPI_PIO;

	ret = mspi_config(&spec);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_dev_config(mspi_bus, &dev_id[0], MSPI_DEVICE_CONFIG_ALL, &device_cfg[0]);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_transceive(mspi_bus, &dev_id[0], &xfer);
	expected = expected_result.test_23a_mspi_xfer_MSPI_PIO;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_23b_mspi_xfer_MSPI_DMA)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = default_hw_cfg,
	};

	struct mspi_xfer xfer = default_xfer;

	xfer.xfer_mode = MSPI_DMA;

	ret = mspi_config(&spec);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_dev_config(mspi_bus, &dev_id[0], MSPI_DEVICE_CONFIG_ALL, &device_cfg[0]);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_transceive(mspi_bus, &dev_id[0], &xfer);
	expected = expected_result.test_23b_mspi_xfer_MSPI_DMA;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_24a_mspi_xfer_tx_dummy_max)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = default_hw_cfg,
	};

	struct mspi_xfer xfer = default_xfer;

	xfer.tx_dummy = 65535;

	ret = mspi_config(&spec);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_dev_config(mspi_bus, &dev_id[0], MSPI_DEVICE_CONFIG_ALL, &device_cfg[0]);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_transceive(mspi_bus, &dev_id[0], &xfer);
	expected = expected_result.test_24a_mspi_xfer_tx_dummy_max;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_25a_mspi_xfer_rx_dummy_max)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = default_hw_cfg,
	};

	struct mspi_xfer xfer = default_xfer;

	xfer.rx_dummy = 65535;

	ret = mspi_config(&spec);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_dev_config(mspi_bus, &dev_id[0], MSPI_DEVICE_CONFIG_ALL, &device_cfg[0]);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_transceive(mspi_bus, &dev_id[0], &xfer);
	expected = expected_result.test_25a_mspi_xfer_rx_dummy_max;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_26a_mspi_xfer_cmd_length_max)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = default_hw_cfg,
	};

	struct mspi_xfer xfer = default_xfer;

	xfer.cmd_length = 255;

	ret = mspi_config(&spec);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_dev_config(mspi_bus, &dev_id[0], MSPI_DEVICE_CONFIG_ALL, &device_cfg[0]);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_transceive(mspi_bus, &dev_id[0], &xfer);
	expected = expected_result.test_26a_mspi_xfer_cmd_length_max;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_27a_mspi_xfer_addr_length_max)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = default_hw_cfg,
	};

	struct mspi_xfer xfer = default_xfer;

	xfer.addr_length = 255;

	ret = mspi_config(&spec);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_dev_config(mspi_bus, &dev_id[0], MSPI_DEVICE_CONFIG_ALL, &device_cfg[0]);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_transceive(mspi_bus, &dev_id[0], &xfer);
	expected = expected_result.test_27a_mspi_xfer_addr_length_max;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_28a_mspi_xfer_hold_ce_true)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = default_hw_cfg,
	};

	struct mspi_xfer xfer = default_xfer;

	xfer.hold_ce = true;

	ret = mspi_config(&spec);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_dev_config(mspi_bus, &dev_id[0], MSPI_DEVICE_CONFIG_ALL, &device_cfg[0]);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_transceive(mspi_bus, &dev_id[0], &xfer);
	expected = expected_result.test_28a_mspi_xfer_hold_ce_true;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_28b_mspi_xfer_hold_ce_false)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = default_hw_cfg,
	};

	struct mspi_xfer xfer = default_xfer;

	xfer.hold_ce = false;

	ret = mspi_config(&spec);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_dev_config(mspi_bus, &dev_id[0], MSPI_DEVICE_CONFIG_ALL, &device_cfg[0]);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_transceive(mspi_bus, &dev_id[0], &xfer);
	expected = expected_result.test_28b_mspi_xfer_hold_ce_false;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_30a_mspi_xfer_priority_0)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = default_hw_cfg,
	};

	struct mspi_xfer xfer = default_xfer;

	xfer.priority = 0;

	ret = mspi_config(&spec);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_dev_config(mspi_bus, &dev_id[0], MSPI_DEVICE_CONFIG_ALL, &device_cfg[0]);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_transceive(mspi_bus, &dev_id[0], &xfer);
	expected = expected_result.test_30a_mspi_xfer_priority_0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_30b_mspi_xfer_priority_1)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = default_hw_cfg,
	};

	struct mspi_xfer xfer = default_xfer;

	xfer.priority = 1;

	ret = mspi_config(&spec);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_dev_config(mspi_bus, &dev_id[0], MSPI_DEVICE_CONFIG_ALL, &device_cfg[0]);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_transceive(mspi_bus, &dev_id[0], &xfer);
	expected = expected_result.test_30b_mspi_xfer_priority_1;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_31a_mspi_xfer_packets_NULL)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = default_hw_cfg,
	};

	struct mspi_xfer xfer = default_xfer;

	xfer.packets = (struct mspi_xfer_packet *) NULL;

	ret = mspi_config(&spec);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_dev_config(mspi_bus, &dev_id[0], MSPI_DEVICE_CONFIG_ALL, &device_cfg[0]);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_transceive(mspi_bus, &dev_id[0], &xfer);
	expected = expected_result.test_31a_mspi_xfer_packets_NULL;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_32a_mspi_xfer_num_packet_0)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = default_hw_cfg,
	};

	struct mspi_xfer xfer = default_xfer;

	xfer.num_packet = 0;

	ret = mspi_config(&spec);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_dev_config(mspi_bus, &dev_id[0], MSPI_DEVICE_CONFIG_ALL, &device_cfg[0]);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_transceive(mspi_bus, &dev_id[0], &xfer);
	expected = expected_result.test_32a_mspi_xfer_num_packet_0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_33a_mspi_xfer_timeout_max)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = default_hw_cfg,
	};

	struct mspi_xfer xfer = default_xfer;

	xfer.timeout = 4294967295;

	ret = mspi_config(&spec);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_dev_config(mspi_bus, &dev_id[0], MSPI_DEVICE_CONFIG_ALL, &device_cfg[0]);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_transceive(mspi_bus, &dev_id[0], &xfer);
	expected = expected_result.test_33a_mspi_xfer_timeout_max;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_34a_mspi_xfer_packet_num_bytes_max)
{
	int ret, expected;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = default_hw_cfg,
	};

	struct mspi_xfer_packet test_packet[] = {
		{
			.dir                = MSPI_TX,
			.cmd                = 0x12,
			.address            = 0,
			.num_bytes          = 4294967295,
			.data_buf           = memc_write_buffer,
			.cb_mask            = MSPI_BUS_NO_CB,
		},
	};

	struct mspi_xfer xfer = default_xfer;

	xfer.packets = (struct mspi_xfer_packet *)&test_packet,

	ret = mspi_config(&spec);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_dev_config(mspi_bus, &dev_id[0], MSPI_DEVICE_CONFIG_ALL, &device_cfg[0]);
	expected = 0;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);

	ret = mspi_transceive(mspi_bus, &dev_id[0], &xfer);
	expected = expected_result.test_34a_mspi_xfer_packet_num_bytes_max;
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}


ZTEST_SUITE(mspi_error_cases, NULL, NULL, NULL, NULL, NULL);
