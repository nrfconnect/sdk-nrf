/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DEVICE_INFO_SERVICE_H__
#define DEVICE_INFO_SERVICE_H__

#include <stddef.h>
#include <stdint.h>
#include <errno.h>

#include <sdfw/sdfw_services/ssf_errno.h>

#define UUID_BYTES_LENGTH 16UL

/** .. include_startingpoint_device_info_header_rst */
/**
 * @brief       Read UUID value.
 *
 * @note        UUID is provided in words, byte order is the same as 
 *              read from the memory and no endianness swapping is performed.
 *
 * @param[out]  uuid_buff Local buffer for copying the UUID into.
 * @param[out]  uuid_buff_size UUID buffer size, use defined UUID_BYTES_LENGTH 
 *              to specify its size.
 *
 * @return      0 on success, otherwise a negative errno.
 */
int ssf_device_info_get_uuid(uint8_t* uuid_buff, const size_t uuid_buff_size);

/** .. include_endpoint_device_info_header_rst */

#endif /* DEVICE_INFO_SERVICE_H__ */
