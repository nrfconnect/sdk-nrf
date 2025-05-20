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
 * @brief Get raw channel value from UICR
 */
uint8_t uicr_channel_get(void);

/**
 * @brief Write raw channel value to UICR
 *
 * @param channel Channel value
 *
 * @return 0 if successful
 * @return -EROFS if different channel is already written
 * @return -EIO if channel failed to be written
 */
int uicr_channel_set(uint8_t channel);

/**
 * @brief Get Segger serial number value from UICR
 */
uint64_t uicr_snr_get(void);

#endif /* _UICR_H_ */
