/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @defgroup coap_utils CoAP communication BSD socket API based
 * @{
 */

#ifndef __COAP_UTILS_H__
#define __COAP_UTILS_H__

#include <zephyr/net/coap.h>
#include <zephyr/net/net_ip.h>

/** @brief Open socket and start the receiving thread.
 *
 * @param[in] ip_family Network IP protocol family (AF_INET or AF_INET6).
 * @param[in] addr Local address to bind for receiving data.
 *		   Pass NULL pointer if no address is provided.
 */
void coap_init(int ip_family, struct sockaddr *addr);

/** @brief Send CoAP non-confirmable request.
 *
 * @param[in] method           CoAP method type.
 * @param[in] addr             pointer to socket address struct for IPv6.
 * @param[in] uri_path_options pointer to CoAP URI schemes option.
 * @param[in] payload          pointer to the CoAP message payload.
 * @param[in] payload_size     size of the CoAP message payload.
 * @param[in] reply_cb         function to call when the response comes.
 *
 * @retval >= 0 On success.
 * @retval < 0 On failure.
 */
int coap_send_request(enum coap_method method, const struct sockaddr *addr,
		      const char *const *uri_path_options, uint8_t *payload,
		      uint16_t payload_size, coap_reply_t reply_cb);

#endif /* __COAP_UTILS_H__ */

/**
 * @}
 */
