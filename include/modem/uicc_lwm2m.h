/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef UICC_LWM2M_H_
#define UICC_LWM2M_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file uicc_lwm2m.h
 *
 * @defgroup uicc_lwm2m UICC LwM2M
 *
 * @{
 *
 * @brief Public APIs of the UICC LwM2M library.
 */

/** UICC record max size is 256 bytes. The buffer size needed for the AT response is
 * (256 * 2) + 4 bytes for SW + 1 byte for NUL. Using 516 bytes is adequate to read
 * a full UICC record.
 */
#define UICC_RECORD_BUFFER_MAX ((256 * 2) + 4 + 1)

/**
 * @brief Read UICC LwM2M bootstrap record.
 *
 * @param[inout] buffer Buffer to store UICC LwM2M bootstrap record. This buffer is also
 *                      used internally by the function reading the AT response, so it must
 *                      be twice the size of expected LwM2M content + 4 bytes for UICC SW.
 * @param[in] buffer_size Total size of buffer.
 *
 * @return Length of UICC LwM2M bootstrap record, -errno on error.
 */
int uicc_lwm2m_bootstrap_read(uint8_t *buffer, int buffer_size);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* UICC_LWM2M_H_ */
