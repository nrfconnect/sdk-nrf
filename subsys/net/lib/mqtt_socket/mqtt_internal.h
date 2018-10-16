/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/** @file mqtt_internal.h
 *
 * @brief Function and data structures internal to MQTT module.
 */

#ifndef MQTT_INTERNAL_H_
#define MQTT_INTERNAL_H_

#include <stdint.h>
#include <string.h>

#include <net/mqtt_socket.h>

#ifdef __cplusplus
extern "C" {
#endif

/**@brief Maximum number of clients that can be managed by the module. */
#define MQTT_MAX_CLIENTS CONFIG_MQTT_MAX_CLIENTS

/**@brief Keep alive time for MQTT (in seconds). Sending of Ping Requests to
 *        keep the connection alive are governed by this value.
 */
#define MQTT_KEEPALIVE CONFIG_MQTT_KEEPALIVE

/**@brief Maximum MQTT packet size that can be sent (including the fixed and
 *        variable header).
 */
#define MQTT_MAX_PACKET_LENGTH CONFIG_MQTT_MAX_PACKET_LENGTH

/**@brief Fixed header minimum size. Remaining length size is 1 in this case. */
#define MQTT_FIXED_HEADER_SIZE 2

/**@brief Maximum size of the fixed header. Remaining length size is 4 in this
 *        case.
 */
#define MQTT_FIXED_HEADER_EXTENDED_SIZE 5

/**@brief MQTT Control Packet Types. */
#define MQTT_PKT_TYPE_CONNECT     0x10
#define MQTT_PKT_TYPE_CONNACK     0x20
#define MQTT_PKT_TYPE_PUBLISH     0x30
#define MQTT_PKT_TYPE_PUBACK      0x40
#define MQTT_PKT_TYPE_PUBREC      0x50
#define MQTT_PKT_TYPE_PUBREL      0x60
#define MQTT_PKT_TYPE_PUBCOMP     0x70
#define MQTT_PKT_TYPE_SUBSCRIBE   0x82 /* QoS 1 for subscribe */
#define MQTT_PKT_TYPE_SUBACK      0x90
#define MQTT_PKT_TYPE_UNSUBSCRIBE 0xA2
#define MQTT_PKT_TYPE_UNSUBACK    0xB0
#define MQTT_PKT_TYPE_PINGREQ     0xC0
#define MQTT_PKT_TYPE_PINGRSP     0xD0
#define MQTT_PKT_TYPE_DISCONNECT  0xE0

/**@brief Masks for MQTT header flags. */
#define MQTT_HEADER_DUP_MASK     0x08
#define MQTT_HEADER_QOS_MASK     0x06
#define MQTT_HEADER_RETAIN_MASK  0x01


/**@brief Masks for MQTT header flags. */
#define MQTT_CONNECT_FLAG_CLEAN_SESSION   0x02
#define MQTT_CONNECT_FLAG_WILL_TOPIC      0x04
#define MQTT_CONNECT_FLAG_WILL_RETAIN     0x20
#define MQTT_CONNECT_FLAG_PASSWORD        0x40
#define MQTT_CONNECT_FLAG_USERNAME        0x80

#define MQTT_CONNACK_FLAG_SESSION_PRESENT 0x01

/**@brief Size of mandatory header of MQTT packet. */
#define MQTT_PKT_HEADER_SIZE 2

/**@brief Maximum payload size of MQTT packet. */
#define MQTT_MAX_PAYLOAD_SIZE 0x0FFFFFFF

/**@brief Maximum size of variable and payload in the packet. */
#define MQTT_MAX_VARIABLE_HEADER_N_PAYLOAD \
	(MQTT_MAX_PACKET_LENGTH - MQTT_FIXED_HEADER_EXTENDED_SIZE)

/**@brief Computes total size needed to pack a UTF8 string. */
#define GET_UT8STR_BUFFER_SIZE(STR) (sizeof(u16_t) + (STR)->size)

/**@brief Computes total size needed to pack a binary stream. */
#define GET_BINSTR_BUFFER_SIZE(STR) ((STR)->len)

/**@brief Unsubscribe packet size. */
#define MQTT_UNSUBSCRIBE_PKT_SIZE 4

/**@brief Sets MQTT Client's state with one indicated in 'STATE'. */
#define MQTT_SET_STATE(CLIENT, STATE) ((CLIENT)->state |= (STATE))

/**@brief Sets MQTT Client's state exclusive to 'STATE'. */
#define MQTT_SET_STATE_EXCLUSIVE(CLIENT, STATE) ((CLIENT)->state = (STATE))

/**@brief Verifies if MQTT Client's state is set with one indicated in 'STATE'.
 */
#define MQTT_VERIFY_STATE(CLIENT, STATE) ((CLIENT)->state & (STATE))

/**@brief Reset 'STATE' in MQTT Client's state. */
#define MQTT_RESET_STATE(CLIENT, STATE) ((CLIENT)->state &= ~(STATE))

/**@brief Initialize MQTT Client's state. */
#define MQTT_STATE_INIT(CLIENT) ((CLIENT)->state = MQTT_STATE_IDLE)

/**@brief Computes the first byte of MQTT message header based on message type,
 *        duplication flag, QoS and  the retain flag.
 */
#define MQTT_MESSAGES_OPTIONS(TYPE, DUP, QOS, RETAIN) \
	(((TYPE)      & 0xF0)  | \
	 (((DUP) << 3) & 0x08) | \
	 (((QOS) << 1) & 0x06) | \
	 ((RETAIN) & 0x01))


#define MQTT_EVT_FLAG_NONE 0x00000000
#define MQTT_EVT_FLAG_INSTANCE_RESET 0x00000001

#define MQTT_LENGTH_VALUE_MASK 0x7F
#define MQTT_LENGTH_CONTINUATION_BIT 0x80
#define MQTT_LENGTH_SHIFT 7

/**@brief Check if the input pointer is NULL, if so it returns -EINVAL. */
#define NULL_PARAM_CHECK(param) \
	do { \
		if ((param) == NULL) { \
			return -EINVAL; \
		} \
	} while (0)

#define NULL_PARAM_CHECK_VOID(param) \
	do { \
		if ((param) == NULL) { \
			return; \
		} \
	} while (0)

/**@brief MQTT States. */
enum mqtt_state {
	/** Idle state, implying the client entry in the table is unused/free.
	 */
	MQTT_STATE_IDLE                 = 0x00000000,

	/** TCP Connection has been requested, awaiting result of the request.
	 */
	MQTT_STATE_TCP_CONNECTING       = 0x00000001,

	/** TCP Connection successfully established. */
	MQTT_STATE_TCP_CONNECTED        = 0x00000002,

	/** MQTT Connection successful. */
	MQTT_STATE_CONNECTED            = 0x00000004,

	/** State that indicates write callback is awaited for an issued
	 *  request.
	 */
	MQTT_STATE_PENDING_WRITE        = 0x00000008,

	/** TCP Disconnect has been requested, awaiting result of the request.
	 */
	MQTT_STATE_DISCONNECTING        = 0x00000010
};

/**@brief Notify application about MQTT event.
 *
 * @param[in] client Identifies the client for which event occurred.
 * @param[in] evt MQTT event.
 * @param[in] flags MQTT event flags.
 */
void event_notify(struct mqtt_client *client, const struct mqtt_evt *evt,
		  u32_t flags);

/**@brief Handles MQTT messages received from the peer. For TLS, this routine
 *        is evoked to handle decrypted application data. For TCP, this routine
 *        is evoked to handle TCP data.
 *
 * @param[in] client Identifies the client for which the data was received.
 * @param[in] data MQTT data received.
 * @param[in] datalen Length of data received.
 *
 * @retval Number of bytes processed from the incoming packet.
 */
u32_t mqtt_handle_rx_data(struct mqtt_client *client, u8_t *data,
			  u32_t datalen);

/**@brief Constructs/encodes Connect packet.
 *
 * @param[in] client Identifies the client for which the procedure is requested.
 *                   All information required for creating the packet like
 *                   client id, clean session flag, retain session flag etc are
 *                   assumed to be populated for the client instance when this
 *                   procedure is requested.
 * @param[out] packet Pointer to the MQTT connect message.
 * @param[out] packet_length Length of the connect request.
 *
 * @return 0 if the procedure is successful, an error code otherwise.
 */
int connect_request_encode(const struct mqtt_client *client,
			   const u8_t **packet, u32_t *packet_length);

/**@brief Constructs/encodes Publish packet.
 *
 * @param[in] client Identifies the client for which packet is encoded.
   @param[in] param Publish message parameters.
 * @param[out] packet Pointer to the MQTT Publish message.
 * @param[out] packet_length Length of the Publish message.
 *
 * @return 0 if the procedure is successful, an error code otherwise.
 */
int publish_encode(const struct mqtt_client *client,
		   const struct mqtt_publish_param *param,
		   const u8_t **packet, u32_t *packet_length);

/**@brief Constructs/encodes Publish Ack packet.
 *
 * @param[in] client Identifies the client for which packet is encoded.
   @param[in] param Publish Ack message parameters.
 * @param[out] packet Pointer to the MQTT Publish Ack message.
 * @param[out] packet_length Length of the Publish Ack message.
 *
 * @return 0 if the procedure is successful, an error code otherwise.
 */
int publish_ack_encode(const struct mqtt_client *client,
		       const struct mqtt_puback_param *param,
		       const u8_t **packet, u32_t *packet_length);

/**@brief Constructs/encodes Publish Receive packet.
 *
 * @param[in] client Identifies the client for which packet is encoded.
   @param[in] param Publish Receive message parameters.
 * @param[out] packet Pointer to the MQTT Publish Receive message.
 * @param[out] packet_length Length of the Publish Receive message.
 *
 * @return 0 if the procedure is successful, an error code otherwise.
 */
int publish_receive_encode(const struct mqtt_client *client,
			   const struct mqtt_pubrec_param *param,
			   const u8_t **packet, u32_t *packet_length);

/**@brief Constructs/encodes Publish Release packet.
 *
 * @param[in] client Identifies the client for which packet is encoded.
   @param[in] param Publish Release message parameters.
 * @param[out] packet Pointer to the MQTT Publish Release message.
 * @param[out] packet_length Length of the Publish Relase message.
 *
 * @return 0 if the procedure is successful, an error code otherwise.
 */
int publish_release_encode(const struct mqtt_client *client,
			   const struct mqtt_pubrel_param *param,
			   const u8_t **packet, u32_t *packet_length);

/**@brief Constructs/encodes Publish Complete packet.
 *
 * @param[in] client Identifies the client for which packet is encoded.
   @param[in] param Publish Complete message parameters.
 * @param[out] packet Pointer to the MQTT Publish Complete message.
 * @param[out] packet_length Length of the Publish Complete message.
 *
 * @return 0 if the procedure is successful, an error code otherwise.
 */
int publish_complete_encode(const struct mqtt_client *client,
			    const struct mqtt_pubcomp_param *param,
			    const u8_t **packet, u32_t *packet_length);

/**@brief Constructs/encodes Disconnect packet.
 *
 * @param[in] client Identifies the client for which packet is encoded.
 * @param[out] packet Pointer to the MQTT Disconnect message.
 * @param[out] packet_length Length of the Disconnect message.
 *
 * @return 0 if the procedure is successful, an error code otherwise.
 */
int disconnect_encode(const struct mqtt_client *client,
		      const u8_t **packet, u32_t *packet_length);

/**@brief Constructs/encodes Subscribe packet.
 *
 * @param[in] client Identifies the client for which packet is encoded.
   @param[in] param Subscribe message parameters.
 * @param[out] packet Pointer to the MQTT Subscribe message.
 * @param[out] packet_length Length of the Subscribe message.
 *
 * @return 0 if the procedure is successful, an error code otherwise.
 */
int subscribe_encode(const struct mqtt_client *client,
		     const struct mqtt_subscription_list *param,
		     const u8_t **packet, u32_t *packet_length);

/**@brief Constructs/encodes Unsubscribe packet.
 *
 * @param[in] client Identifies the client for which packet is encoded.
   @param[in] param Unsubscribe message parameters.
 * @param[out] packet Pointer to the MQTT Unsubscribe message.
 * @param[out] packet_length Length of the Unsubscribe message.
 *
 * @return 0 if the procedure is successful, an error code otherwise.
 */
int unsubscribe_encode(const struct mqtt_client *client,
		       const struct mqtt_subscription_list *param,
		       const u8_t **packet, u32_t *packet_length);

/**@brief Constructs/encodes Ping Request packet.
 *
 * @param[in] client Identifies the client for which packet is encoded.
   @param[in] param Ping Request message parameters.
 * @param[out] packet Pointer to the MQTT Ping Request message.
 * @param[out] packet_length Length of the Ping Request message.
 *
 * @return 0 if the procedure is successful, an error code otherwise.
 */
int ping_request_encode(const struct mqtt_client *client,
			const u8_t **packet, u32_t *packet_length);

/**@brief Decode MQTT Packet Length in the MQTT fixed header.
 *
 * @param[in] buffer Buffer where the length is to be decoded.
 * @param[in] buffer_len Length of buffer.
 * @param[out] remaining_length Length of variable header and payload in the
 *                              MQTT message.
 * @param[inout] offset Offset on the buffer from where the length is to be
 *                      unpacked.
 *
 * @retval 0 if the procedure is successful.
 * @retval -EINVAL if the offset is greater than or equal to the buffer length.
 */
int packet_length_decode(u8_t *buffer, u32_t buffer_len,
			 u32_t *remaining_length, u32_t *offset);

/**@brief Decode MQTT Connect Ack packet.
 *
 * @param[in] client MQTT client for which packet is decoded.
 * @param[in] data Buffer containing message to decode.
 * @param[in] datalen Length of the message.
 * @param[in] offset Offset of the first byte after MQTT fixed header.
 * @param[out] param Pointer to buffer for decoded Connect Ack parameters.
 *
 * @return 0 if the procedure is successful, an error code otherwise.
 */
int connect_ack_decode(const struct mqtt_client *client, u8_t *data,
		       u32_t datalen, u32_t offset,
		       struct mqtt_connack_param *param);

/**@brief Decode MQTT Publish packet.
 *
 * @param[in] data Buffer containing message to decode.
 * @param[in] datalen Length of the message.
 * @param[in] offset Offset of the first byte after MQTT fixed header.
 * @param[out] param Pointer to buffer for decoded Publish parameters.
 *
 * @return 0 if the procedure is successful, an error code otherwise.
 */
int publish_decode(u8_t *data, u32_t datalen, u32_t offset,
		   struct mqtt_publish_param *param);

/**@brief Decode MQTT Publish Ack packet.
 *
 * @param[in] data Buffer containing message to decode.
 * @param[in] datalen Length of the message.
 * @param[in] offset Offset of the first byte after MQTT fixed header.
 * @param[out] param Pointer to buffer for decoded Publish Ack parameters.
 *
 * @return 0 if the procedure is successful, an error code otherwise.
 */
int publish_ack_decode(u8_t *data, u32_t datalen, u32_t offset,
		       struct mqtt_puback_param *param);

/**@brief Decode MQTT Publish Receive packet.
 *
 * @param[in] data Buffer containing message to decode.
 * @param[in] datalen Length of the message.
 * @param[in] offset Offset of the first byte after MQTT fixed header.
 * @param[out] param Pointer to buffer for decoded Publish Receive parameters.
 *
 * @return 0 if the procedure is successful, an error code otherwise.
 */
int publish_receive_decode(u8_t *data, u32_t datalen, u32_t offset,
			   struct mqtt_pubrec_param *param);

/**@brief Decode MQTT Publish Release packet.
 *
 * @param[in] data Buffer containing message to decode.
 * @param[in] datalen Length of the message.
 * @param[in] offset Offset of the first byte after MQTT fixed header.
 * @param[out] param Pointer to buffer for decoded Publish Release parameters.
 *
 * @return 0 if the procedure is successful, an error code otherwise.
 */
int publish_release_decode(u8_t *data, u32_t datalen, u32_t offset,
			   struct mqtt_pubrel_param *param);

/**@brief Decode MQTT Publish Complete packet.
 *
 * @param[in] data Buffer containing message to decode.
 * @param[in] datalen Length of the message.
 * @param[in] offset Offset of the first byte after MQTT fixed header.
 * @param[out] param Pointer to buffer for decoded Publish Complete parameters.
 *
 * @return 0 if the procedure is successful, an error code otherwise.
 */
int publish_complete_decode(u8_t *data, u32_t datalen, u32_t offset,
			    struct mqtt_pubcomp_param *param);

/**@brief Decode MQTT Subscribe packet.
 *
 * @param[in] data Buffer containing message to decode.
 * @param[in] datalen Length of the message.
 * @param[in] offset Offset of the first byte after MQTT fixed header.
 * @param[out] param Pointer to buffer for decoded Subscribe parameters.
 *
 * @return 0 if the procedure is successful, an error code otherwise.
 */
int subscribe_ack_decode(u8_t *data, u32_t datalen, u32_t offset,
			 struct mqtt_suback_param *param);

/**@brief Decode MQTT Unsubscribe packet.
 *
 * @param[in] data Buffer containing message to decode.
 * @param[in] datalen Length of the message.
 * @param[in] offset Offset of the first byte after MQTT fixed header.
 * @param[out] param Pointer to buffer for decoded Unsubscribe parameters.
 *
 * @return 0 if the procedure is successful, an error code otherwise.
 */
int unsubscribe_ack_decode(u8_t *data, u32_t datalen, u32_t offset,
			   struct mqtt_unsuback_param *param);

#ifdef __cplusplus
}
#endif

#endif /* MQTT_INTERNAL_H_ */
