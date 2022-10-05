/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef HW_ID_H__
#define HW_ID_H__

#include <zephyr/types.h>

/**
 * @defgroup hw_id Device ID Library
 * @{
 * @brief Library that provides a unified interface for fetching a unique device ID from hardware.
 */

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_HW_ID_LIBRARY_SOURCE_BLE_MAC)
#define HW_ID_LEN (12 + 1)
#elif defined(CONFIG_HW_ID_LIBRARY_SOURCE_DEVICE_ID)
#define HW_ID_LEN (16 + 1)
#elif defined(CONFIG_HW_ID_LIBRARY_SOURCE_UUID)
#include <modem/modem_jwt.h>
#define HW_ID_LEN (NRF_UUID_V4_STR_LEN + 1)
#elif defined(CONFIG_HW_ID_LIBRARY_SOURCE_IMEI)
#define HW_ID_LEN (15 + 1)
#elif defined(CONFIG_HW_ID_LIBRARY_SOURCE_NET_MAC)
#define HW_ID_LEN (12 + 1)
#endif

/** @brief Fetch unique device ID
 *  @note The buffer should be at least HW_ID_LEN bytes long.
 *
 *  @param[out] buf destination buffer
 *  @param[in] buf_len length of destination buffer
 *  @return 0         If the operation was successful.
 *  @return -EINVAL   If the passed in pointer is NULL or buffer is too small.
 *  @return -EIO      If requesting platform ID failed.
 */
int hw_id_get(char *buf, size_t buf_len);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* HW_ID_H__ */
