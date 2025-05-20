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

#define UUID_BYTES_LENGTH 16UL

/**
 * @brief       Read UUID value.
 *
 * @note        UUID byte order is the same as read from the memory
 *              and no endianness swapping is performed.
 *
 * @param[out]  uuid_buff Local buffer for copying the UUID into.
 *              use defined UUID_BYTES_LENGTH to specify its size.
 *
 * @return      0 on success, otherwise a negative errno defined in ssf_errno.h.
 *              -SSF_ENOBUFS if invalid buffer.
 *              -SSF_EPROTO if server code resulted in an error.
 *              -SSF_EBADMSG if client received a bad response.
 */
int ssf_device_info_get_uuid(uint8_t *uuid_buff);

#endif /* DEVICE_INFO_SERVICE_H__ */
