/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "./backend/backend.h"
#include "./hrt/hrt.h"

#include <stdio.h>
#include <zephyr/kernel.h>
#include <hal/nrf_vpr_csr.h>
#include <hal/nrf_vpr_csr_vio.h>
#include <haly/nrfy_gpio.h>

#include <zephyr/drivers/mspi.h>

#define MAX_DATA_LEN	256

#define HRT_IRQ_PRIORITY          	2
#define HRT_VEVIF_IDX_WRITE_SINGLE  17
#define HRT_VEVIF_IDX_WRITE_QUAD    18

#define VEVIF_IRQN(vevif) VEVIF_IRQN_1(vevif)
#define VEVIF_IRQN_1(vevif) VPRCLIC_##vevif##_IRQn

struct mspi_config {
	uint8_t* data;
	uint8_t data_len;
	uint8_t word_size;
};

struct mspi_dev_config {
	enum mspi_io_mode 		io_mode;
	enum mspi_ce_polarity   ce_polarity;
	uint32_t                read_cmd;
	uint32_t                write_cmd;
	uint8_t                 cmd_length; /* Command length in bits. */
	uint8_t                 addr_length; /* Address length in bits. */
};

static struct mspi_dev_config mspi_dev_configs;

uint32_t data_buffer[MAX_DATA_LEN + 2];

volatile uint8_t counter_top;
volatile uint8_t word_size;
volatile uint32_t* data_to_send;
volatile uint8_t data_len;
volatile uint8_t ce_hold;

void configure_clock(enum mspi_cpp_mode cpp_mode)
{
	nrf_vpr_csr_vio_config_t vio_config = {
		.input_sel = 0,
		.stop_cnt = 0,
	};

    nrf_vpr_csr_vio_dir_set(PIN_DIR_OUT_MASK(SCLK_PIN));

	switch (cpp_mode)
	{
		case MSPI_CPP_MODE_0:
		{
			vio_config.clk_polarity = 0;
    		nrf_vpr_csr_vio_out_set(PIN_OUT_LOW_MASK(SCLK_PIN));
			break;
		}
		case MSPI_CPP_MODE_1:
		{
			vio_config.clk_polarity = 1;
    		nrf_vpr_csr_vio_out_set(PIN_OUT_LOW_MASK(SCLK_PIN));
			break;
		}
		case MSPI_CPP_MODE_2:
		{
			vio_config.clk_polarity = 1;
    		nrf_vpr_csr_vio_out_set(PIN_OUT_HIGH_MASK(SCLK_PIN));
			break;
		}
		case MSPI_CPP_MODE_3:
		{
			vio_config.clk_polarity = 0;
    		nrf_vpr_csr_vio_out_set(PIN_OUT_HIGH_MASK(SCLK_PIN));
			break;
		}
	}

	nrf_vpr_csr_vio_config_set(&vio_config);
}

void prepare_and_send_data(uint8_t* data, uint8_t data_length)
{
	/* Use for-loop instead of memcpy to make sure that unnecessary zeros are not trasmitted. Can be optimized later. */
	for (uint8_t i = 0; i < data_length; i++)
	{
		data_buffer[i + 2] = data[i];
	}

	/* Send command */
	ce_hold = true;
	word_size = mspi_dev_configs.cmd_length;
	data_len = 1;
	data_to_send = data_buffer;

	if (mspi_dev_configs.io_mode == MSPI_IO_MODE_QUAD)
	{
		nrf_vpr_clic_int_pending_set(NRF_VPRCLIC, VEVIF_IRQN(HRT_VEVIF_IDX_WRITE_QUAD));
	}
	else
	{
		nrf_vpr_clic_int_pending_set(NRF_VPRCLIC, VEVIF_IRQN(HRT_VEVIF_IDX_WRITE_SINGLE));
	}

	/* Send address */
	word_size = mspi_dev_configs.addr_length;
	data_to_send = data_buffer + 1;
	nrf_vpr_clic_int_pending_set(NRF_VPRCLIC, VEVIF_IRQN(HRT_VEVIF_IDX_WRITE_SINGLE));

	if (mspi_dev_configs.io_mode == MSPI_IO_MODE_SINGLE || mspi_dev_configs.io_mode == MSPI_IO_MODE_QUAD_1_1_4)
	{
		nrf_vpr_clic_int_pending_set(NRF_VPRCLIC, VEVIF_IRQN(HRT_VEVIF_IDX_WRITE_SINGLE));
	}
	else
	{
		nrf_vpr_clic_int_pending_set(NRF_VPRCLIC, VEVIF_IRQN(HRT_VEVIF_IDX_WRITE_QUAD));
	}

	/* Send data */
	ce_hold = false;
	word_size = 8;
	data_len = data_length;
	data_to_send = data_buffer + 2;

	if (mspi_dev_configs.io_mode == MSPI_IO_MODE_SINGLE)
	{
		nrf_vpr_clic_int_pending_set(NRF_VPRCLIC, VEVIF_IRQN(HRT_VEVIF_IDX_WRITE_SINGLE));
	}
	else
	{
		nrf_vpr_clic_int_pending_set(NRF_VPRCLIC, VEVIF_IRQN(HRT_VEVIF_IDX_WRITE_QUAD));
	}
}

void process_packet(const void *data, size_t len)
{
	(void)len;
	nrfe_mspi_flpr_response_t response;
	uint8_t *buffer = (uint8_t *)data;
	uint8_t opcode = *buffer++;

	switch (opcode) {
		case NRFE_MSPI_CTRL_CONFIG: {
			response.opcode = NRFE_MSPI_CTRL_CONFIG_DONE;
			struct mspi_cfg *cfg = (struct mspi_cfg *)data;

			/* Not supported options. Some may be added later, for example, sw_multi_periph. */
			if (cfg->op_mode == MSPI_OP_MODE_PERIPHERAL ||
				cfg->duplex == MSPI_FULL_DUPLEX ||
				cfg->dqs_support ||
				cfg->sw_multi_periph ||
				cfg->num_ce_gpios > 1)
			{
				return;
			}

			for (uint8_t i = 0; i < cfg->num_ce_gpios; i++)
			{
				/* Do not support CS on other ports than PORT2, so that VIO could be used. */
				if (cfg->ce_group[i].port->config->port_num != 2)
				{
					return;
				}
			}

			/* Ignore freq_max and re_init for now. */

			break;
		}
		case NRFE_MSPI_DEV_CONFIG: {
			response.opcode = NRFE_MSPI_DEV_CONFIG_DONE;
			struct mspi_dev_cfg *cfg = (struct mspi_dev_cfg *)data;

			/* Not supported options. */
			if (cfg->io_mode == MSPI_IO_MODE_DUAL ||
				cfg->io_mode == MSPI_IO_MODE_DUAL_1_1_2 ||
				cfg->io_mode == MSPI_IO_MODE_DUAL_1_2_2 ||
				cfg->io_mode == MSPI_IO_MODE_OCTAL ||
				cfg->io_mode == MSPI_IO_MODE_OCTAL_1_1_8 ||
				cfg->io_mode == MSPI_IO_MODE_OCTAL_1_8_8 ||
				cfg->io_mode == MSPI_IO_MODE_HEX ||
				cfg->io_mode == MSPI_IO_MODE_HEX_8_8_16 ||
				cfg->io_mode == MSPI_IO_MODE_HEX_8_16_16 ||
				cfg->data_rate == MSPI_DATA_RATE_S_S_D ||
				cfg->data_rate == MSPI_DATA_RATE_S_D_D ||
				cfg->data_rate == MSPI_DATA_RATE_DUAL ||
				cfg->endian != MSPI_XFER_BIG_ENDIAN ||
				cfg->dqs_enable)
			{
				return;
			}
			/* TODO: Process device config data */

			mspi_dev_configs.io_mode = cfg->io_mode;

			configure_clock(cfg->cpp);

			mspi_dev_configs.ce_polarity = cfg->ce_polarity;

    		nrf_vpr_csr_vio_dir_set(PIN_DIR_OUT_MASK(CS_PIN));

			if (mspi_dev_configs.ce_polarity == MSPI_CE_ACTIVE_LOW)
			{
    			nrf_vpr_csr_vio_out_set(PIN_OUT_HIGH_MASK(CS_PIN));
			}
			else
			{
    			nrf_vpr_csr_vio_out_set(PIN_OUT_LOW_MASK(CS_PIN));
			}

			mspi_dev_configs.read_cmd = cfg->read_cmd;
			mspi_dev_configs.write_cmd = cfg->write_cmd;

			mspi_dev_configs.cmd_length = cfg->cmd_length;
			mspi_dev_configs.addr_length = cfg->addr_length;

			/* Ignore ce_num, freq, rx_dummy, tx_dummy, mem_boundary and time_to_break for now. */
			break;
		}
		case NRFE_MSPI_XFER_CONFIG: {
			response.opcode = NRFE_MSPI_XFER_CONFIG_DONE;
			struct mspi_xfer *cfg = (struct mspi_xfer *)data;

			mspi_dev_configs.cmd_length = cfg->cmd_length;
			mspi_dev_configs.addr_length = cfg->addr_length;

			/* Ignore async, tx_dummy, rx_dummy, hold_ce, ce_sw_ctrl, priority, timeout for now. */

			break;
		}
		case NRFE_MSPI_XFER: {
			struct mspi_xfer_packet *cfg = (struct mspi_xfer_packet *)data;

			if (packet->dir == MSPI_TX) {

				counter_top = 4;

				data_buffer[0] = cfg->command;
				data_buffer[1] = cfg->address;

				prepare_and_send_data(cfg->data_buf, cfg->num_bytes);

				response.opcode = NRFE_MSPI_TX_DONE;
			}
			else if (packet->dir == MSPI_RX) {
				response.opcode = NRFE_MSPI_RX_DONE;
			}

			/* Ignore cb_mask for now. */

			break;
		}
		default:
			break;
	}

	int ret = ipc_service_send(&ep, (const void *)&response.opcode, sizeof(response));
	if (ret < 0) {
		// printf("IPC send error: %d\n", ret);
	}
}

#define HRT_CONNECT(vevif, handler)                                            \
	IRQ_DIRECT_CONNECT(vevif, HRT_IRQ_PRIORITY, handler, 0);               \
	nrf_vpr_clic_int_enable_set(NRF_VPRCLIC, VEVIF_IRQN(vevif), true)


__attribute__ ((interrupt)) void hrt_handler_write_single(void)
{
	uint8_t ce_enable_state = (mspi_dev_configs.ce_polarity == MSPI_CE_ACTIVE_LOW) ? 0 : 1;
	write_single_by_word(data_to_send, data_len, counter_top, word_size, ce_enable_state, ce_hold);
}

__attribute__ ((interrupt)) void hrt_handler_write_quad(void)
{
	uint8_t ce_enable_state = (mspi_dev_configs.ce_polarity == MSPI_CE_ACTIVE_LOW) ? 0 : 1;
	write_quad_by_word(data_to_send, data_len, counter_top, word_size, ce_enable_state, ce_hold);
}

int main(void)
{
	int ret = 0;

	ret = backend_init(process_packet);
	if (ret < 0) {
		return 0;
	}

	HRT_CONNECT(HRT_VEVIF_IDX_WRITE_SINGLE, hrt_handler_write_single);
	HRT_CONNECT(HRT_VEVIF_IDX_WRITE_QUAD, hrt_handler_write_quad);

	nrf_vpr_csr_rtperiph_enable_set(true);

	while (true) {
		k_cpu_idle();
	}

	return 0;
}
