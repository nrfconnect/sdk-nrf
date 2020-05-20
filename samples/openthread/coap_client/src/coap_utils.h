/**
 * @file
 * @defgroup coap_utils CoAP Zephyr socket based API
 * @{
 */

/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#ifndef __COAP_UTILS_H__
#define __COAP_UTILS_H__

#include <net/coap.h>

/** @brief Initialize CoAP utilities.
 */
void coap_init(void);

/** @brief Send CoAP non-confirmable request.
 *
 * @param[in] method           CoAP method type.
 * @param[in] addr6            pointer to socket address struct for IPv6.
 * @param[in] uri_path_options pointer to CoAP URI schemes option.
 * @param[in] payload          pointer to the CoAP message payload.
 * @param[in] payload_size     size of the CoAP message payload.
 * @param[in] reply_cb         function to call when the response comes.
 *
 * @retval 0    On success.
 * @retval != 0 On failure.
 */
int coap_send_request(enum coap_method method, const struct sockaddr_in6 *addr6,
		      const char *const *uri_path_options, u8_t *payload,
		      u16_t payload_size, coap_reply_t reply_cb);

#endif

/**
 * @}
 */
