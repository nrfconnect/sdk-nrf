/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DEVICE_INFO_SERVICE_H__
#define DEVICE_INFO_SERVICE_H__

#include <stddef.h>
#include <stdint.h>

#include <sdfw/sdfw_services/ssf_errno.h>

/** .. include_startingpoint_device_info_header_rst */
/**
 * @brief       Read UUID value.
 *
 * @note        UUID is provided in words, byte order is the same as
 *              read from the memory and no endianness swapping is performed.
 *
 * @param[out]  uuid_words Local buffer for copying the UUID words into.
 * @param[in]   uuid_words_count Size of local buffer, should be at least 4 words.
 *
 * @return      0 on success, otherwise a negative errno.
 */
int ssf_device_info_get_uuid(uint32_t *uuid_words, size_t uuid_words_count);

/** .. include_endpoint_device_info_header_rst */

#endif /* DEVICE_INFO_SERVICE_H__ */
