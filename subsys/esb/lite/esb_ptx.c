/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <esb.h>
#include "esb_common.h"

int esb_ptx_init(const struct esb_config *config)
{
	return -ENOSYS;
}

void esb_ptx_disable(void)
{
}

int esb_ptx_write_payload(const struct esb_payload *payload)
{
	return -ENOSYS;
}

int esb_ptx_read_rx_payload(struct esb_payload *payload)
{
	return -ENOSYS;
}

int esb_ptx_start_tx(void)
{
	return -ENOSYS;
}

int esb_ptx_flush_tx(void)
{
	return -ENOSYS;
}

int esb_ptx_pop_tx(void)
{
	return -ENOSYS;
}

bool esb_ptx_tx_full(void)
{
	return false;
}

int esb_ptx_flush_rx(void)
{
	return -ENOSYS;
}

int esb_ptx_set_retransmit_delay(uint16_t delay)
{
	return -ENOSYS;
}

int esb_ptx_set_retransmit_count(uint16_t count)
{
	return -ENOSYS;
}

int esb_ptx_reuse_pid(uint8_t pipe)
{
	return -ENOSYS;
}
