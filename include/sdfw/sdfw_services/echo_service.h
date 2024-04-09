/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SSF_ECHO_SERVICE_H__
#define SSF_ECHO_SERVICE_H__

#include <stddef.h>

#include <sdfw/sdfw_services/ssf_errno.h>

/** .. include_startingpoint_echo_header_rst */
/**
 * @brief       Echo back a c string.
 *
 * @param[in]   str_in c string to be echoed back.
 * @param[out]  str_out Local buffer for copying the echoed c string into.
 * @param[in]   str_out_size Size of local buffer.
 *
 * @return      0 on success, otherwise a negative errno.
 */
int ssf_echo(char *str_in, char *str_out, size_t str_out_size);
/** .. include_endpoint_echo_header_rst */

#endif /* SSF_ECHO_SERVICE_H__ */
