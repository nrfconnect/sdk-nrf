/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _UICR_H_
#define _UICR_H_

#include <stdint.h>

// TODO: Discuss better alternative for UICR storage. This memory range is not documented
#define UICR_APP_BASE_ADDR (NRF_UICR_S_BASE + 0xF0)

/**
 * @brief Get raw location value from UICR as bitfield
 */
uint32_t uicr_location_get(void);

/**
 * @brief Write raw location bitfield value to UICR
 *
 * @param location Location bitfield in accordance with LE Audio specification.
 *
 * @return 0 if successful
 * @return -EROFS if different location is already written
 * @return -EIO if location failed to be written
 */
int uicr_location_set(uint32_t location);

/**
 * @brief Get Segger serial number value from UICR
 */
uint64_t uicr_snr_get(void);

#endif /* _UICR_H_ */
