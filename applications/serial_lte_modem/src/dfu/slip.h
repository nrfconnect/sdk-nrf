/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#ifndef SLIP_H__
#define SLIP_H__

#include <zephyr/kernel.h>

#define SLIP_SIZE_MAX 128

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define	SLIP_END	0300
#define	SLIP_ESC	0333
#define	SLIP_ESC_END	0334
#define	SLIP_ESC_ESC	0335

void encode_slip(uint8_t *dest_data, uint32_t *dest_size,
		 const uint8_t *src_data, uint32_t src_size);

int  decode_slip(uint8_t *dest_data, uint32_t *dest_size,
		 const uint8_t *src_data, uint32_t src_size);

#ifdef __cplusplus
}   /* ... extern "C" */
#endif  /* __cplusplus */


#endif /* SLIP_H__ */
