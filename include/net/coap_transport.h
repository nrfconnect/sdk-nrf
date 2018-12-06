/*
 * Copyright (c) 2014 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/** @file coap_transport.h
 *
 * @defgroup iot_sdk_coap_transport CoAP transport abstraction
 * @ingroup iot_sdk_coap
 * @{
 * @brief The transport interface that the CoAP depends on for sending and
 *        receiving CoAP messages.
 *
 * @details While the interface is well defined and should not be altered, the
 *          implementation of the interface depends on the choice of IP stack.
 *          The only exception to this is the \ref coap_transport_read API.
 *          This API is implemented in the CoAP, and the transport layer is
 *          expected to call this function when data is received on one of the
 *          CoAP ports.
 */

#ifndef COAP_TRANSPORT_H__
#define COAP_TRANSPORT_H__

#include <stdint.h>

#include <net/socket.h>
#include <net/tls_credentials.h>

#ifdef __cplusplus
extern "C" {
#endif


/**@brief  Identifies the local transport handle for data exchange. */
typedef u32_t coap_transport_handle_t;

/**@brief TLS configuration for secure MQTT transports. */
typedef struct {
	/** Indicates the role. */
	int role;

	/** Indicates the preference for peer verification. */
	int peer_verify;

	/** Indicates the number of entries in the cipher list. */
	u32_t cipher_count;

	/** Indicates the list of ciphers to be used for the session.
	 *  May be NULL to use the default ciphers.
	 */
	int *cipher_list;

	/** Indicates the number of entries in the sec tag list. */
	u32_t sec_tag_count;

	/** Indicates the list of security tags to be used for the session. */
	sec_tag_t *sec_tag_list;

	/** Peer hostname for ceritificate verification.
	 *  May be NULL to skip hostname verification.
	 */
	char *hostname;
} coap_sec_config_t;

/**@brief Local endpoint - address, port and associated transport (if any). */
typedef struct {
	/** Local address and port information. */
	struct sockaddr *addr;

	/** Protocol to be used, shall be one of IPPROTO_UDP or SPROTO_DTLS1v2.
	 *  0 implies IPROTO_UDP.
	 */
	int protocol;

	/** Transport associated with the local endpoint. */
	coap_transport_handle_t *transport;

	/** Security parameters in case the protocol is SPROTO_DTLS1v2.
	 *  Otherwise ignored.
	 */
	coap_sec_config_t *setting;
} coap_local_t;

/**@brief Transport initialization information. */
typedef struct {
	/** Information about the ports being registered. Count is assumed to
	 *  be COAP_PORT_COUNT.
	 */
	coap_local_t *port_table;

	/** Public. Miscellaneous pointer to application provided data that
	 *  should be passed to the transport.
	 */
	void *arg;
} coap_transport_init_t;

/**@brief Initializes the transport layer to have the data ports set up for
 *        CoAP.
 *
 * @param[in] param Port count and port numbers.
 *
 * @return 0 if initialization was successful. Otherwise, an error code that
 *         indicates the reason for the failure is returned.
 */
u32_t coap_transport_init(coap_transport_init_t *param);

/**@brief Sends data on a CoAP endpoint or port.
 *
 * @param[in] handle  Identifies the transport on which the data is to be sent.
 * @param[in] remote  Remote endpoint to which the data is targeted.
 * @param[in] data    Pointer to the data to be sent.
 * @param[in] datalen Length of the data to be sent.
 *
 * @return 0 if the data was sent successfully. Otherwise, an error code that
 *         indicates the reason for the failure is returned.
 */
u32_t coap_transport_write(const coap_transport_handle_t *handle,
			   const struct sockaddr *remote, const u8_t *data,
			   u16_t datalen);

/**@brief Handles data received on a CoAP endpoint or port.
 *
 * This API is not implemented by the transport layer, but assumed to exist.
 * This approach avoids unnecessary registering of callback and remembering
 * it in the transport layer.
 *
 * @param[in] handle  Transport on which the data was received.
 * @param[in] remote  Remote endpoint from which the data is received.
 * @param[in] local   Local endpoint on which the data is received.
 * @param[in] result  Indicates if the data was processed successfully by lower
 *                    layers.
 * @param[in] data    Pointer to the data received.
 * @param[in] datalen Length of the data received.
 *
 * @return 0 if the data was handled successfully. Otherwise, an error code
 *         that indicates the reason for the failure is returned.
 *
 */
u32_t coap_transport_read(const coap_transport_handle_t *handle,
			  const struct sockaddr *remote,
			  const struct sockaddr *local, u32_t result,
			  const u8_t *data, u16_t datalen);

/**@brief Process loop to handle DTLS processing.
 *
 * @details The function handles any processing of encrypted packets.
 *          Some encryption libraries requires to be run in a processing
 *          loop. This function is called by the CoAP library every time
 *          \ref coap_time_tick is issued from the library user. Any other
 *          process specific routines that should be done regularly could be
 *          added in this function.
 */
void coap_transport_process(void);

/**@brief Process loop when using CoAP BSD socket transport implementation.
 *
 * @details This is blocking call. The function unblock is only triggered upon
 *          an socket event registered to select() by CoAP transport. This
 *          function must be called as often as possible in order to dispatch
 *          incoming socket events. Preferred to be put in the application's
 *          main loop or similar.
 */
void coap_transport_input(void);

#ifdef __cplusplus
}
#endif

#endif /* COAP_TRANSPORT_H__ */

/** @} */
