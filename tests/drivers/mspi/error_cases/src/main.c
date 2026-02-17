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

#define MSPI_BUS_NODE       DT_ALIAS(mspi0)

static const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

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
	.duplex           = MSPI_HALF_DUPLEX,
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
	.xfer_mode        = MSPI_PIO,
	.tx_dummy         = 0,
	.cmd_length       = 1,
	.addr_length      = 3,
	.priority         = 1,
	.packets          = (struct mspi_xfer_packet *)&packet1,
	.num_packet       = sizeof(packet1) / sizeof(struct mspi_xfer_packet),
};

static int ret;

/* Helper function. */
void check_mspi_config(struct mspi_cfg *hw_cfg, int expected)
{

	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = *hw_cfg,
	};

	ret = mspi_config(&spec);
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_01a_MSPI_OP_MODE_CONTROLLER)
{
	struct mspi_cfg hw_cfg = default_hw_cfg;

	hw_cfg.op_mode = MSPI_OP_MODE_CONTROLLER;

	check_mspi_config(&hw_cfg, 0);
}

ZTEST(mspi_error_cases, test_01b_MSPI_OP_MODE_PERIPHERAL)
{
	struct mspi_cfg hw_cfg = default_hw_cfg;

	hw_cfg.op_mode = MSPI_OP_MODE_PERIPHERAL;

	check_mspi_config(&hw_cfg, -ENOTSUP);
}

ZTEST(mspi_error_cases, test_02a_MSPI_HALF_DUPLEX)
{
	struct mspi_cfg hw_cfg = default_hw_cfg;

	hw_cfg.duplex = MSPI_HALF_DUPLEX;

	check_mspi_config(&hw_cfg, 0);
}

ZTEST(mspi_error_cases, test_02b_MSPI_FULL_DUPLEX)
{
	struct mspi_cfg hw_cfg = default_hw_cfg;

	hw_cfg.duplex = MSPI_FULL_DUPLEX;

	check_mspi_config(&hw_cfg, -ENOTSUP);
}

ZTEST(mspi_error_cases, test_03a_dqs_support_true)
{
	struct mspi_cfg hw_cfg = default_hw_cfg;

	hw_cfg.dqs_support = true;

	check_mspi_config(&hw_cfg, -ENOTSUP);
}

ZTEST(mspi_error_cases, test_03b_dqs_support_false)
{
	struct mspi_cfg hw_cfg = default_hw_cfg;

	hw_cfg.dqs_support = false;

	check_mspi_config(&hw_cfg, 0);
}

ZTEST(mspi_error_cases, test_04a_sw_multi_periph_true)
{
	struct mspi_cfg hw_cfg = default_hw_cfg;

	hw_cfg.sw_multi_periph = true;

	check_mspi_config(&hw_cfg, 0);
}

ZTEST(mspi_error_cases, test_05a_num_ce_gpios_max)
{
	struct mspi_cfg hw_cfg = default_hw_cfg;

	hw_cfg.num_ce_gpios = UINT32_MAX;

	check_mspi_config(&hw_cfg, -EINVAL);
}

ZTEST(mspi_error_cases, test_06a_num_periph_max)
{
	struct mspi_cfg hw_cfg = default_hw_cfg;

	hw_cfg.num_periph = UINT32_MAX;

	check_mspi_config(&hw_cfg, -EINVAL);
}

ZTEST(mspi_error_cases, test_07a_max_freq_max)
{
	struct mspi_cfg hw_cfg = default_hw_cfg;

	hw_cfg.max_freq = UINT32_MAX;

	check_mspi_config(&hw_cfg, -ENOTSUP);
}

ZTEST(mspi_error_cases, test_08a_re_init_true)
{
	struct mspi_cfg hw_cfg = default_hw_cfg;

	hw_cfg.re_init = true;

	check_mspi_config(&hw_cfg, 0);
}

ZTEST(mspi_error_cases, test_08b_re_init_false)
{
	struct mspi_cfg hw_cfg = default_hw_cfg;

	hw_cfg.re_init = false;

	check_mspi_config(&hw_cfg, 0);
}

void check_mspi_dev_config(const enum mspi_dev_cfg_mask param_mask,
		const struct mspi_dev_cfg *cfg,
		int expected)
{
	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = default_hw_cfg,
	};

	ret = mspi_config(&spec);
	zassert_equal(ret, 0, "Got unexpected %d instead of 0", ret);

	ret = mspi_dev_config(mspi_bus, &dev_id[0], param_mask, cfg);
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_09a_MSPI_DEVICE_CONFIG_NONE)
{
	check_mspi_dev_config(MSPI_DEVICE_CONFIG_NONE, &device_cfg[0], 0);
}

ZTEST(mspi_error_cases, test_10a_MSPI_DEVICE_CONFIG_CE_NUM_MAX)
{
	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.ce_num = UINT8_MAX;

	check_mspi_dev_config(MSPI_DEVICE_CONFIG_CE_NUM, &dev_cfg, -EINVAL);
}

ZTEST(mspi_error_cases, test_11a_MSPI_DEVICE_CONFIG_FREQUENCY_MAX)
{
	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.freq = UINT32_MAX;

	check_mspi_dev_config(MSPI_DEVICE_CONFIG_FREQUENCY, &dev_cfg, -EINVAL);
}

ZTEST(mspi_error_cases, test_11b_MSPI_DEVICE_CONFIG_FREQUENCY_MIN)
{
	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.freq = 1;

	check_mspi_dev_config(MSPI_DEVICE_CONFIG_FREQUENCY, &dev_cfg, -EINVAL);
}

ZTEST(mspi_error_cases, test_12a_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_SINGLE)
{
	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.io_mode = MSPI_IO_MODE_SINGLE;

	check_mspi_dev_config(MSPI_DEVICE_CONFIG_IO_MODE, &dev_cfg, 0);
}

ZTEST(mspi_error_cases, test_12b_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_DUAL)
{
	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.io_mode = MSPI_IO_MODE_DUAL;

	check_mspi_dev_config(MSPI_DEVICE_CONFIG_IO_MODE, &dev_cfg, -ENOTSUP);
}

ZTEST(mspi_error_cases, test_12c_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_DUAL_1_1_2)
{
	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.io_mode = MSPI_IO_MODE_DUAL_1_1_2;

	check_mspi_dev_config(MSPI_DEVICE_CONFIG_IO_MODE, &dev_cfg, -ENOTSUP);
}

ZTEST(mspi_error_cases, test_12d_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_DUAL_1_2_2)
{
	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.io_mode = MSPI_IO_MODE_DUAL_1_2_2;

	check_mspi_dev_config(MSPI_DEVICE_CONFIG_IO_MODE, &dev_cfg, -ENOTSUP);
}

ZTEST(mspi_error_cases, test_12e_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_QUAD)
{
	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.io_mode = MSPI_IO_MODE_QUAD;

	check_mspi_dev_config(MSPI_DEVICE_CONFIG_IO_MODE, &dev_cfg, 0);
}

ZTEST(mspi_error_cases, test_12f_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_QUAD_1_1_4)
{
	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.io_mode = MSPI_IO_MODE_QUAD_1_1_4;

	check_mspi_dev_config(MSPI_DEVICE_CONFIG_IO_MODE, &dev_cfg, 0);
}

ZTEST(mspi_error_cases, test_12g_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_QUAD_1_4_4)
{
	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.io_mode = MSPI_IO_MODE_QUAD_1_4_4;

	check_mspi_dev_config(MSPI_DEVICE_CONFIG_IO_MODE, &dev_cfg, 0);
}

ZTEST(mspi_error_cases, test_12h_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_OCTAL)
{
	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.io_mode = MSPI_IO_MODE_OCTAL;

	check_mspi_dev_config(MSPI_DEVICE_CONFIG_IO_MODE, &dev_cfg, -ENOTSUP);
}

ZTEST(mspi_error_cases, test_12i_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_OCTAL_1_1_8)
{
	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.io_mode = MSPI_IO_MODE_OCTAL_1_1_8;

	check_mspi_dev_config(MSPI_DEVICE_CONFIG_IO_MODE, &dev_cfg, -ENOTSUP);
}

ZTEST(mspi_error_cases, test_12j_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_OCTAL_1_8_8)
{
	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.io_mode = MSPI_IO_MODE_OCTAL_1_8_8;

	check_mspi_dev_config(MSPI_DEVICE_CONFIG_IO_MODE, &dev_cfg, -ENOTSUP);
}

ZTEST(mspi_error_cases, test_12k_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_HEX)
{
	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.io_mode = MSPI_IO_MODE_HEX;

	check_mspi_dev_config(MSPI_DEVICE_CONFIG_IO_MODE, &dev_cfg, -ENOTSUP);
}

ZTEST(mspi_error_cases, test_12l_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_HEX_8_8_16)
{
	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.io_mode = MSPI_IO_MODE_HEX_8_8_16;

	check_mspi_dev_config(MSPI_DEVICE_CONFIG_IO_MODE, &dev_cfg, -ENOTSUP);
}

ZTEST(mspi_error_cases, test_12m_MSPI_DEVICE_CONFIG_IO_MODE_eq_MSPI_IO_MODE_HEX_8_16_16)
{
	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.io_mode = MSPI_IO_MODE_HEX_8_16_16;

	check_mspi_dev_config(MSPI_DEVICE_CONFIG_IO_MODE, &dev_cfg, -ENOTSUP);
}

ZTEST(mspi_error_cases, test_13a_MSPI_DEVICE_CONFIG_DATA_RATE_eq_MSPI_DATA_RATE_SINGLE)
{
	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.data_rate = MSPI_DATA_RATE_SINGLE;

	check_mspi_dev_config(MSPI_DEVICE_CONFIG_DATA_RATE, &dev_cfg, 0);
}

ZTEST(mspi_error_cases, test_13b_MSPI_DEVICE_CONFIG_DATA_RATE_eq_MSPI_DATA_RATE_S_S_D)
{
	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.data_rate = MSPI_DATA_RATE_S_S_D;

	check_mspi_dev_config(MSPI_DEVICE_CONFIG_DATA_RATE, &dev_cfg, -ENOTSUP);
}

ZTEST(mspi_error_cases, test_13c_MSPI_DEVICE_CONFIG_DATA_RATE_eq_MSPI_DATA_RATE_S_D_D)
{
	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.data_rate = MSPI_DATA_RATE_S_D_D;

	check_mspi_dev_config(MSPI_DEVICE_CONFIG_DATA_RATE, &dev_cfg, -ENOTSUP);
}

ZTEST(mspi_error_cases, test_13d_MSPI_DEVICE_CONFIG_DATA_RATE_eq_MSPI_DATA_RATE_DUAL)
{
	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.data_rate = MSPI_DATA_RATE_DUAL;

	check_mspi_dev_config(MSPI_DEVICE_CONFIG_DATA_RATE, &dev_cfg, -ENOTSUP);
}

ZTEST(mspi_error_cases, test_14a_MSPI_DEVICE_CONFIG_CPP_eq_MSPI_CPP_MODE_0)
{
	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.cpp = MSPI_CPP_MODE_0;

	check_mspi_dev_config(MSPI_DEVICE_CONFIG_CPP, &dev_cfg, 0);
}

ZTEST(mspi_error_cases, test_14b_MSPI_DEVICE_CONFIG_CPP_eq_MSPI_CPP_MODE_1)
{
	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.cpp = MSPI_CPP_MODE_1;

	check_mspi_dev_config(MSPI_DEVICE_CONFIG_CPP, &dev_cfg, -ENOTSUP);
}

ZTEST(mspi_error_cases, test_14c_MSPI_DEVICE_CONFIG_CPP_eq_MSPI_CPP_MODE_2)
{
	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.cpp = MSPI_CPP_MODE_2;

	check_mspi_dev_config(MSPI_DEVICE_CONFIG_CPP, &dev_cfg, -ENOTSUP);
}

ZTEST(mspi_error_cases, test_14d_MSPI_DEVICE_CONFIG_CPP_eq_MSPI_CPP_MODE_3)
{
	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.cpp = MSPI_CPP_MODE_3;

	check_mspi_dev_config(MSPI_DEVICE_CONFIG_CPP, &dev_cfg, 0);
}

ZTEST(mspi_error_cases, test_15a_MSPI_DEVICE_CONFIG_ENDIAN_eq_MSPI_XFER_LITTLE_ENDIAN)
{
	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.endian = MSPI_XFER_LITTLE_ENDIAN;

	check_mspi_dev_config(MSPI_DEVICE_CONFIG_ENDIAN, &dev_cfg, -ENOTSUP);
}

ZTEST(mspi_error_cases, test_15b_MSPI_DEVICE_CONFIG_ENDIAN_eq_MSPI_XFER_BIG_ENDIAN)
{
	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.endian = MSPI_XFER_BIG_ENDIAN;

	check_mspi_dev_config(MSPI_DEVICE_CONFIG_ENDIAN, &dev_cfg, 0);
}

ZTEST(mspi_error_cases, test_16a_MSPI_DEVICE_CONFIG_CE_POL_eq_MSPI_CE_ACTIVE_LOW)
{
	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.ce_polarity = MSPI_CE_ACTIVE_LOW;

	check_mspi_dev_config(MSPI_DEVICE_CONFIG_CE_POL, &dev_cfg, 0);
}

ZTEST(mspi_error_cases, test_16b_MSPI_DEVICE_CONFIG_CE_POL_eq_MSPI_CE_ACTIVE_HIGH)
{
	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.ce_polarity = MSPI_CE_ACTIVE_HIGH;

	check_mspi_dev_config(MSPI_DEVICE_CONFIG_CE_POL, &dev_cfg, 0);
}

ZTEST(mspi_error_cases, test_17a_MSPI_DEVICE_CONFIG_DQS_true)
{
	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.dqs_enable = true;

	check_mspi_dev_config(MSPI_DEVICE_CONFIG_DQS, &dev_cfg, -ENOTSUP);
}

ZTEST(mspi_error_cases, test_17b_MSPI_DEVICE_CONFIG_DQS_false)
{
	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.dqs_enable = false;

	check_mspi_dev_config(MSPI_DEVICE_CONFIG_DQS, &dev_cfg, 0);
}

ZTEST(mspi_error_cases, test_18a_MSPI_DEVICE_CONFIG_RX_DUMMY_MAX)
{
	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.rx_dummy = UINT16_MAX;

	check_mspi_dev_config(MSPI_DEVICE_CONFIG_RX_DUMMY, &dev_cfg, -ENOTSUP);
}

ZTEST(mspi_error_cases, test_19a_MSPI_DEVICE_CONFIG_TX_DUMMY_MAX)
{
	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.tx_dummy = UINT16_MAX;

	check_mspi_dev_config(MSPI_DEVICE_CONFIG_TX_DUMMY, &dev_cfg, -ENOTSUP);
}

ZTEST(mspi_error_cases, test_20a_MSPI_DEVICE_CONFIG_MEM_BOUND_MAX)
{
	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.mem_boundary = UINT32_MAX;

	check_mspi_dev_config(MSPI_DEVICE_CONFIG_MEM_BOUND, &dev_cfg, -ENOTSUP);
}

ZTEST(mspi_error_cases, test_21a_MSPI_DEVICE_CONFIG_BREAK_TIME_MAX)
{
	struct mspi_dev_cfg dev_cfg = device_cfg[0];

	dev_cfg.time_to_break = UINT32_MAX;

	check_mspi_dev_config(MSPI_DEVICE_CONFIG_BREAK_TIME, &dev_cfg, -ENOTSUP);
}

void check_mspi_transceive(const struct mspi_xfer *xfer, int expected)
{
	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = default_hw_cfg,
	};

	ret = mspi_config(&spec);
	zassert_equal(ret, 0, "Got unexpected %d instead of 0", ret);

	ret = mspi_dev_config(mspi_bus, &dev_id[0], MSPI_DEVICE_CONFIG_ALL, &device_cfg[0]);
	zassert_equal(ret, 0, "Got unexpected %d instead of 0", ret);

	ret = mspi_transceive(mspi_bus, &dev_id[0], xfer);
	zassert_equal(ret, expected, "Got unexpected %d instead of %d", ret, expected);
}

ZTEST(mspi_error_cases, test_22a_mspi_xfer_async_true)
{
	struct mspi_xfer xfer = default_xfer;

	xfer.async = true;

	check_mspi_transceive(&xfer, -ENOTSUP);
}

ZTEST(mspi_error_cases, test_22a_mspi_xfer_async_false)
{
	struct mspi_xfer xfer = default_xfer;

	xfer.async = false;

	check_mspi_transceive(&xfer, 0);
}

ZTEST(mspi_error_cases, test_23a_mspi_xfer_MSPI_PIO)
{
	struct mspi_xfer xfer = default_xfer;

	xfer.xfer_mode = MSPI_PIO;

	check_mspi_transceive(&xfer, 0);
}

ZTEST(mspi_error_cases, test_23b_mspi_xfer_MSPI_DMA)
{
	struct mspi_xfer xfer = default_xfer;

	xfer.xfer_mode = MSPI_DMA;

	check_mspi_transceive(&xfer, -ENOTSUP);
}

ZTEST(mspi_error_cases, test_24a_mspi_xfer_tx_dummy_max)
{
	struct mspi_xfer xfer = default_xfer;

	xfer.tx_dummy = UINT16_MAX;

	check_mspi_transceive(&xfer, -ENOTSUP);
}

ZTEST(mspi_error_cases, test_25a_mspi_xfer_rx_dummy_max)
{
	struct mspi_xfer xfer = default_xfer;

	xfer.rx_dummy = UINT16_MAX;

	check_mspi_transceive(&xfer, -ENOTSUP);
}

ZTEST(mspi_error_cases, test_26a_mspi_xfer_cmd_length_max)
{
	struct mspi_xfer xfer = default_xfer;

	xfer.cmd_length = UINT8_MAX;

	check_mspi_transceive(&xfer, 0);
}

ZTEST(mspi_error_cases, test_27a_mspi_xfer_addr_length_max)
{
	struct mspi_xfer xfer = default_xfer;

	xfer.addr_length = UINT8_MAX;

	check_mspi_transceive(&xfer, 0);
}

ZTEST(mspi_error_cases, test_28a_mspi_xfer_hold_ce_true)
{
	struct mspi_xfer xfer = default_xfer;

	xfer.hold_ce = true;

	check_mspi_transceive(&xfer, 0);
}

ZTEST(mspi_error_cases, test_28b_mspi_xfer_hold_ce_false)
{
	struct mspi_xfer xfer = default_xfer;

	xfer.hold_ce = false;

	check_mspi_transceive(&xfer, 0);
}

ZTEST(mspi_error_cases, test_30a_mspi_xfer_priority_0)
{
	struct mspi_xfer xfer = default_xfer;

	xfer.priority = 0;

	check_mspi_transceive(&xfer, 0);
}

ZTEST(mspi_error_cases, test_30b_mspi_xfer_priority_1)
{
	struct mspi_xfer xfer = default_xfer;

	xfer.priority = 1;

	check_mspi_transceive(&xfer, 0);
}

ZTEST(mspi_error_cases, test_31a_mspi_xfer_packets_NULL)
{
	struct mspi_xfer xfer = default_xfer;

	xfer.packets = (struct mspi_xfer_packet *) NULL;

	check_mspi_transceive(&xfer, -EFAULT);
}

ZTEST(mspi_error_cases, test_32a_mspi_xfer_num_packet_0)
{
	struct mspi_xfer xfer = default_xfer;

	xfer.num_packet = 0;

	check_mspi_transceive(&xfer, -EFAULT);
}

ZTEST(mspi_error_cases, test_33a_mspi_xfer_timeout_max)
{
	struct mspi_xfer xfer = default_xfer;

	xfer.timeout = UINT32_MAX;

	check_mspi_transceive(&xfer, -EFAULT);
}

ZTEST(mspi_error_cases, test_34a_mspi_xfer_packet_num_bytes_max)
{
	struct mspi_xfer_packet test_packet[] = {
		{
			.dir                = MSPI_TX,
			.cmd                = 0x12,
			.address            = 0,
			.num_bytes          = UINT32_MAX,
			.data_buf           = memc_write_buffer,
			.cb_mask            = MSPI_BUS_NO_CB,
		},
	};

	struct mspi_xfer xfer = default_xfer;

	xfer.packets = (struct mspi_xfer_packet *)&test_packet,

	check_mspi_transceive(&xfer, -EINVAL);
}

ZTEST_SUITE(mspi_error_cases, NULL, NULL, NULL, NULL, NULL);
