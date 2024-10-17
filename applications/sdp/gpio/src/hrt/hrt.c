/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <hal/nrf_vpr_csr_vio.h>

void set_direction(void)
{
	nrf_vpr_csr_vio_dir_set(0xA);
	nrf_vpr_csr_vio_dir_set(0xB);
	nrf_vpr_csr_vio_dir_set(0xC);
}

void set_output(void)
{
	nrf_vpr_csr_vio_out_set(0xA);
	nrf_vpr_csr_vio_dir_set(0xB);
	nrf_vpr_csr_vio_dir_set(0xC);
}
