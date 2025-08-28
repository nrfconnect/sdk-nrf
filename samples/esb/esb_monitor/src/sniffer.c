/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>

#include <hal/nrf_radio.h>
#include <esb.h>

#include <SEGGER_RTT.h>

#include "sniffer.h"

LOG_MODULE_REGISTER(esb_sniffer, CONFIG_ESB_MONITOR_APP_LOG_LEVEL);

static uint8_t rtt_data_buff[CONFIG_ESB_SNIFFER_RTT_DATA_BUF_SZ];
static uint8_t rtt_comm_ret_buff[CONFIG_ESB_SNIFFER_RTT_CMD_RET_BUF_SZ];
static uint8_t rtt_comm_buff[CONFIG_ESB_SNIFFER_RTT_CMD_BUF_SZ];

static k_tid_t rtt_comm_thread_id;
static K_THREAD_STACK_DEFINE(rtt_comm_thread_stack, CONFIG_ESB_SNIFFER_THREAD_STACK_SZ);
static struct k_thread rtt_comm_thread;

static struct esb_sniffer_cfg sniffer_cfg;

enum esb_sniffer_rtt_comm {
	SNIFFER_START  = 1,
	SNIFFER_STOP   = 2,
	SNIFFER_STATUS = 3
};

/**
 * @brief Convert radio register value into array
 *
 * This function takes value of address or prefix register and converts it
 * into human readable array containing currently configured address or prefix
 *
 * @param reg value from base_addr0 or base_addr1 or prefix0 or prefix1 register
 * @param array pointer to destination of the converted value
 */
static void radio_reg_to_array(uint32_t reg, uint8_t *array)
{
#if __CORTEX_M == (0x04U)
	uint32_t inp = reg;

	*((uint32_t *)array) = sys_cpu_to_be32((uint32_t)__RBIT(inp));
#else
	uint32_t inp = sys_cpu_to_le32(reg);

	/* bytewise bit-swap */
	inp = (inp & 0xF0F0F0F0) >> 4 | (inp & 0x0F0F0F0F) << 4;
	inp = (inp & 0xCCCCCCCC) >> 2 | (inp & 0x33333333) << 2;
	inp = (inp & 0xAAAAAAAA) >> 1 | (inp & 0x55555555) << 1;
	memcpy(array, &inp, sizeof(uint32_t));
#endif
}

static void rtt_comm_exec(enum esb_sniffer_rtt_comm command)
{
	int err;

	switch (command) {
	case SNIFFER_START:
		LOG_INF("Start sniffer");
		if (!sniffer_cfg.is_running) {
			err = esb_start_rx();
		} else {
			err = 0;
		}

		if (err != 0) {
			LOG_ERR("Failed to start sniffer, err: %d", err);
		} else {
			sniffer_cfg.is_running = true;
		}

		SEGGER_RTT_Write(CONFIG_ESB_SNIFFER_RTT_CMD_RET_CHANNEL,
								&err, sizeof(int));
		break;
	case SNIFFER_STOP:
		LOG_INF("Stop sniffer");
		if (sniffer_cfg.is_running) {
			err = esb_stop_rx();
		} else {
			err = 0;
		}

		if (err != 0) {
			LOG_ERR("Failed to stop sniffer, err: %d", err);
		} else {
			sniffer_cfg.is_running = false;
		}

		SEGGER_RTT_Write(CONFIG_ESB_SNIFFER_RTT_CMD_RET_CHANNEL,
								&err, sizeof(int));
		break;
	case SNIFFER_STATUS:
		err = sniffer_cfg.is_running;
		SEGGER_RTT_Write(CONFIG_ESB_SNIFFER_RTT_CMD_RET_CHANNEL,
								&err, sizeof(int));
		break;
	default:
		LOG_ERR("Unrecognized rtt command: %d", command);
	}
}

static void rtt_comm_thread_fn(void)
{
	enum esb_sniffer_rtt_comm comm;
	uint32_t read_len;

	/* Send packet length to backend */
	read_len = sizeof(struct rtt_frame);
	read_len = sys_cpu_to_be32(read_len);

	SEGGER_RTT_Write(CONFIG_ESB_SNIFFER_RTT_CMD_RET_CHANNEL,
						&read_len, sizeof(uint32_t));
	while (1) {
		read_len = SEGGER_RTT_Read(CONFIG_ESB_SNIFFER_RTT_CMD_CHANNEL,
						&comm, sizeof(enum esb_sniffer_rtt_comm));
		if (read_len == sizeof(enum esb_sniffer_rtt_comm)) {
			rtt_comm_exec(comm);
		}

		k_sleep(K_MSEC(500));
	}
}

int sniffer_init(void)
{
	int err;

	BUILD_ASSERT(
		CONFIG_ESB_SNIFFER_RTT_DATA_CHANNEL != CONFIG_ESB_SNIFFER_RTT_CMD_RET_CHANNEL,
		"RTT channels with the same direction must have different indexes!"
	);

	err = SEGGER_RTT_ConfigUpBuffer(CONFIG_ESB_SNIFFER_RTT_DATA_CHANNEL,
					"Data", rtt_data_buff,
					CONFIG_ESB_SNIFFER_RTT_DATA_BUF_SZ,
					SEGGER_RTT_MODE_NO_BLOCK_SKIP);
	if (err < 0) {
		LOG_ERR("RTT initialization failed");
		return err;
	}

	err = SEGGER_RTT_ConfigUpBuffer(CONFIG_ESB_SNIFFER_RTT_CMD_RET_CHANNEL,
					 "Comm_ret", rtt_comm_ret_buff,
					 CONFIG_ESB_SNIFFER_RTT_CMD_RET_BUF_SZ,
					 SEGGER_RTT_MODE_NO_BLOCK_SKIP);
	if (err < 0) {
		LOG_ERR("RTT initialization failed");
		return err;
	}

	err = SEGGER_RTT_ConfigDownBuffer(CONFIG_ESB_SNIFFER_RTT_CMD_CHANNEL,
					   "Commands", rtt_comm_buff,
					   CONFIG_ESB_SNIFFER_RTT_CMD_BUF_SZ,
					   SEGGER_RTT_MODE_NO_BLOCK_SKIP);
	if (err < 0) {
		LOG_ERR("RTT initialization failed");
		return err;
	}

	/* Setup sniffer_cfg from already initialized ESB */
	sniffer_cfg.enabled_pipes = 0xFF;
	sniffer_cfg.bitrate = (enum esb_bitrate)nrf_radio_mode_get(NRF_RADIO);
	sniffer_cfg.is_running = false;

	radio_reg_to_array(nrf_radio_base0_get(NRF_RADIO), sniffer_cfg.base_addr0);
	radio_reg_to_array(nrf_radio_base1_get(NRF_RADIO), sniffer_cfg.base_addr1);
	radio_reg_to_array(nrf_radio_prefix0_get(NRF_RADIO), sniffer_cfg.pipe_prefix);
	radio_reg_to_array(nrf_radio_prefix1_get(NRF_RADIO), &sniffer_cfg.pipe_prefix[4]);

	rtt_comm_thread_id = k_thread_create(&rtt_comm_thread,
				rtt_comm_thread_stack,
				CONFIG_ESB_SNIFFER_THREAD_STACK_SZ,
				(k_thread_entry_t)rtt_comm_thread_fn,
				NULL, NULL, NULL,
				CONFIG_ESB_SNIFFER_THREAD_PRIORITY, 0, K_NO_WAIT);

	sniffer_shell_init(&sniffer_cfg);

	return 0;
}
