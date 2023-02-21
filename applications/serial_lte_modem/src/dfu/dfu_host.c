/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include "dfu_host.h"
#include "slm_defines.h"

LOG_MODULE_REGISTER(dfu_req, CONFIG_SLM_LOG_LEVEL);

static uint16_t mtu;
static uint16_t pause;

/* global functions defined in different resources */
int dfu_drv_tx(const uint8_t *data, uint16_t length);

static int send_raw(const uint8_t *data, uint32_t size)
{
	return dfu_drv_tx(data, size);
}

int dfu_host_setup(uint16_t mtu_size, uint16_t pause_time)
{
	mtu = mtu_size;
	pause = pause_time;

	return 0;
}

int dfu_host_send_fw(const uint8_t *data, uint32_t data_size)
{
	int rc = 0;
	uint32_t max_size, stp_size, pos;
	uint32_t pos_start;
	uint32_t hold_count = SLM_NRF52_BLK_SIZE / mtu;
#if defined(CONFIG_SLM_LOG_LEVEL_DBG)
	uint32_t crc_32 = 0;
#endif

	LOG_DBG("Sending firmware file, size %d ...", data_size);

	if (data == NULL || !data_size) {
		LOG_ERR("Invalid firmware data!");
		return 1;
	}

	max_size = mtu;
	pos_start = 0;

	for (pos = pos_start; pos < data_size; pos += stp_size) {
		stp_size = MIN((data_size - pos), max_size);
		rc = send_raw(data + pos, stp_size);
		if (rc) {
			break;
		}

#if defined(CONFIG_SLM_LOG_LEVEL_DBG)
		if (crc_32 == 0) {
			crc_32 = crc32_ieee(data + pos, stp_size);
		} else {
			crc_32 = crc32_ieee_update(crc_32, data + pos, stp_size);
		}
		LOG_DBG("Send %d/%d bytes, CRC: 0x%08x",
			pos + stp_size, data_size, crc_32);
#endif
		hold_count--;
		if (hold_count == 0) {
			k_sleep(K_MSEC(SLM_NRF52_BLK_TIME));
			hold_count = SLM_NRF52_BLK_SIZE / mtu;
		} else {
			k_sleep(K_MSEC(pause));
		}
	}

	return rc;
}
