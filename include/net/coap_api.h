/*
 * Copyright (c) 2014 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/** @file coap_api.h
 *
 * @defgroup iot_sdk_coap_api CoAP Application Programming Interface
 * @ingroup iot_sdk_coap
 * @{
 * @brief Public API of Nordic's CoAP implementation.
 *
 */

#ifndef COAP_API_H__
#define COAP_API_H__

#include <stdint.h>

#include <net/coap_codes.h>
#include <net/coap_transport.h>

#ifdef __cplusplus
extern "C" {
#endif

#define COAP_VERSION CONFIG_NRF_COAP_VERSION
#define COAP_ENABLE_OBSERVE_SERVER CONFIG_NRF_COAP_ENABLE_OBSERVE_SERVER
#define COAP_ENABLE_OBSERVE_CLIENT CONFIG_NRF_COAP_ENABLE_OBSERVE_CLIENT
#define COAP_OBSERVE_MAX_NUM_OBSERVERS CONFIG_NRF_COAP_OBSERVE_MAX_NUM_OBSERVERS
#define COAP_OBSERVE_MAX_NUM_OBSERVABLES \
				CONFIG_NRF_COAP_OBSERVE_MAX_NUM_OBSERVABLES
#define COAP_MAX_NUMBER_OF_OPTIONS CONFIG_NRF_COAP_MAX_NUMBER_OF_OPTIONS
#define COAP_RESOURCE_MAX_NAME_LEN CONFIG_NRF_COAP_RESOURCE_MAX_NAME_LEN
#define COAP_MESSAGE_DATA_MAX_SIZE CONFIG_NRF_COAP_MESSAGE_DATA_MAX_SIZE
#define COAP_MESSAGE_QUEUE_SIZE CONFIG_NRF_COAP_MESSAGE_QUEUE_SIZE
#define COAP_RESOURCE_MAX_DEPTH CONFIG_NRF_COAP_RESOURCE_MAX_DEPTH
#define COAP_SESSION_COUNT CONFIG_NRF_COAP_SESSION_COUNT
#define COAP_PORT_COUNT CONFIG_NRF_COAP_PORT_COUNT
#define COAP_ACK_TIMEOUT CONFIG_NRF_COAP_ACK_TIMEOUT
#define COAP_ACK_RANDOM_FACTOR CONFIG_NRF_COAP_ACK_RANDOM_FACTOR
#define COAP_MAX_TRANSMISSION_SPAN CONFIG_NRF_COAP_MAX_TRANSMISSION_SPAN
#define COAP_MAX_RETRANSMIT_COUNT CONFIG_NRF_COAP_MAX_RETRANSMIT_COUNT

/**@defgroup COAP_CONTENT_TYPE_MASK Resource content type bitmask values
 * @{
 */
/** Content type Plain text supported in the endpoint resource. */
#define COAP_CT_MASK_PLAIN_TEXT 0x01

/** Content type Charset-UTF8 supported in the endpoint resource. */
#define COAP_CT_MASK_CHARSET_UTF8 0x02

/** Content type Application/link-format supported in the endpoint resource. */
#define COAP_CT_MASK_APP_LINK_FORMAT 0x04

/** Content type Application/xml supported in the endpoint resource. */
#define COAP_CT_MASK_APP_XML 0x08

/** Content type Application/octet-stream supported in the endpoint resource. */
#define COAP_CT_MASK_APP_OCTET_STREAM 0x10

/** Content type Application/exi supported in the endpoint resource. */
#define COAP_CT_MASK_APP_EXI 0x20

/** Content type Application/json supported in the endpoint resource. */
#define COAP_CT_MASK_APP_JSON 0x40

/** Content type Application/vnd.oma.lwm2m+tlv supported in the endpoint
 *  resource.
 */
#define COAP_CT_MASK_APP_LWM2M_TLV 0x80
/**@} */

/**@defgroup COAP_METHOD_PERMISSION Resource method permission bitmask values
 * @{
 */
/** Permission by default. Do not allow any method in the CoAP/CoAPS endpoint
 *  resource.
 */
#define COAP_PERM_NONE 0x0000

/** Permission to allow GET method in the CoAP endpoint resource. */
#define COAP_PERM_GET 0x0001

/** Permission to allow POST method in the CoAP endpoint resource.  */
#define COAP_PERM_POST 0x0002

/** Permission to allow PUT method in the CoAP endpoint resource.  */
#define COAP_PERM_PUT 0x0004

/** Permission to allow DELETE method in the CoAP endpoint resource.  */
#define COAP_PERM_DELETE 0x0008

/** Permission to allow GET method in the CoAPS endpoint resource.  */
#define COAPS_PERM_GET 0x0010

/** Permission to allow POST method in the CoAPS endpoint resource.  */
#define COAPS_PERM_POST 0x0020

/** Permission to allow PUT method in the CoAPS endpoint resource.  */
#define COAPS_PERM_PUT 0x0040

/** Permission to allow DELETE method in the CoAPS endpoint resource.  */
#define COAPS_PERM_DELETE 0x0080

/** Permission to allow OBSERVE of the endpoint resource. */
#define COAP_PERM_OBSERVE 0x0100
/**@} */

/**@cond */
/* Forward declare structs. */
typedef struct coap_message_t coap_message_t;
typedef struct coap_resource_t coap_resource_t;
/**@endcond */


/**@brief Signature of function registered by the application
 *        with CoAP to allocate memory for internal module use.
 *
 * @param[in] size Size of memory to be used.
 *
 * @retval A valid memory address on success, else, NULL.
 */
typedef void * (*coap_alloc_t) (size_t size);

/**@brief Signature of function registered by the application with
 *        module to free the memory allocated by the module.
 *
 * @param[in] memory Address of memory to be freed.
 */
typedef void (*coap_free_t)(void *memory);

/**@brief Callback function to call upon undefined behavior.
 *
 * @param[in] error_code Error code from CoAP module.
 * @param[in] message    CoAP message processed when error occurred.
 *                       Could be NULL.
 */
typedef void (*coap_error_callback_t)(u32_t error_code,
				      coap_message_t *message);

/**@brief Callback function to be registered with CoAP messages.
 *
 * @param[in] status  Response status. Possible status codes:
 *                    0 If response was successfully received,
 *                    ECONNRESET if a reset response was received or,
 *                    ETIMEDOUT if transmission
 * @param[in] arg     Miscellaneous pointer to application provided data that
 *                    is associated with the message.
 * @param[in] message Pointer to a CoAP Response message.
 */
typedef void (*coap_response_callback_t)(u32_t status, void *arg,
					 coap_message_t *message);

/**@brief Handler function for manually handling all incoming requests.
 *
 * @details If the function is set, the error code given back will trigger error
 *          messages to be sent back by CoAP to indicate failure. Default error
 *          message will be 4.00 BAD REQUEST. On success, it is expected that
 *          the callback has already sent a response message.
 *
 * @param[in] request Pointer to a CoAP Request message.
 *
 * @retval 0      If the message was successfully has been handled.
 * @retval ENOENT If the message did not match any recognized resources, and a
 *                4.04 NOT FOUND error message should be sent back to the
 *                requester.
 * @retval EINVAL If the message resolved the resource and operation not
 *                permitted, and a 4.05 METHOD NOT ALLOWED error message should
 *                be sent back to the requester.
 */
typedef u32_t (*coap_request_handler_t)(coap_message_t *request);

#ifdef COAP_AUTOMODE

/**@brief Callback function to be registered with CoAP endpoint resources in
 *        auto-mode.
 *
 * @details The callback needs to implement any action based on the request.
 *          The response message will automatically be sent as response when the
 *          callback function returns. The memory is allocated by the caller, so
 *          the application does not have to free up the memory used for the
 *          response.
 *
 * @param[in]  resource Pointer to the request message's target resource.
 * @param[in]  request  Pointer to the request message.
 * @param[out] response Pointer to the prepared response message. The
 *                      application can override its values.
 */
typedef void (*coap_method_callback_t) (coap_resource_t *resource,
					coap_message_t *request,
					coap_message_t *response);

#else /* COAP_AUTOMODE */

/**@brief Callback function to be registered with CoAP endpoint resources in
 *        auto-mode.
 *
 * @details The callback needs to implement any action based on the request.
 *          The callback is responsible of handling the sending of any response
 *          back to the requester. The memory for request will be freed up by
 *          the CoAP module after the callback has been completed.
 *
 * @param[in] resource Pointer to the request message's target resource.
 * @param[in] request  Pointer to the request message.
 */
typedef void (*coap_method_callback_t) (coap_resource_t *resource,
					coap_message_t *request);

#endif /* COAP_AUTOMODE */

/**@brief Enumeration of CoAP content types. */
typedef enum {
	/** Plain text content format number. Default. */
	COAP_CT_PLAIN_TEXT = 0,

	/** Application/link-format content format number. */
	COAP_CT_APP_LINK_FORMAT  = 40,

	/** Application/xml content format number. */
	COAP_CT_APP_XML = 41,

	/** Application/octet-stream content format number. */
	COAP_CT_APP_OCTET_STREAM = 42,

	/** Application/exi content format number. */
	COAP_CT_APP_EXI = 47,

	/** Application/json content format number. */
	COAP_CT_APP_JSON = 50,

	/** Application/vnd.oma.lwm2m+tlv content format number. */
	COAP_CT_APP_LWM2M_TLV = 11542
} coap_content_type_t;

/**@brief Enumeration of CoAP options numbers. */
#define COAP_OPT_RESERVED0      0   /**< Reserved option number. */
#define COAP_OPT_IF_MATCH       1   /**< If-Match option number. */
#define COAP_OPT_URI_HOST       3   /**< URI-Host option number. */
#define COAP_OPT_ETAG           4   /**< ETag option number. */
#define COAP_OPT_IF_NONE_MATCH  5   /**< If-None-Match option number. */
#define COAP_OPT_URI_PORT       7   /**< URI-Port option number. */
#define COAP_OPT_LOCATION_PATH  8   /**< Location-Path option number. */
#define COAP_OPT_URI_PATH       11  /**< URI-Path option number. */
#define COAP_OPT_CONTENT_FORMAT 12  /**< Content-Format option number. */
#define COAP_OPT_MAX_AGE        14  /**< Max-Age option number. */
#define COAP_OPT_URI_QUERY      15  /**< URI-Query option number. */
#define COAP_OPT_ACCEPT         17  /**< Accept option number. */
#define COAP_OPT_LOCATION_QUERY 20  /**< Location-Query option number. */
#define COAP_OPT_BLOCK2         23  /**< Block2 option number. */
#define COAP_OPT_BLOCK1         27  /**< Block1 option number. */
#define COAP_OPT_SIZE2          28  /**< Size2 option number. */
#define COAP_OPT_PROXY_URI      35  /**< Proxy-URI option number. */
#define COAP_OPT_PROXY_SCHEME   39  /**< Proxy-Scheme option number. */
#define COAP_OPT_SIZE1          60  /**< Size1 option number. */
#define COAP_OPT_RESERVED1      128 /**< Reserved option number. */
#define COAP_OPT_RESERVED2      132 /**< Reserved option number. */
#define COAP_OPT_RESERVED3      136 /**< Reserved option number. */
#define COAP_OPT_RESERVED4      140 /**< Reserved option number. */


/**@brief Enumeration of CoAP message types. */
typedef enum {
	COAP_TYPE_CON = 0, /**< Confirmable Message type. */
	COAP_TYPE_NON,     /**< Non-Confirmable Message type. */
	COAP_TYPE_ACK,     /**< Acknowledge Message type. */
	COAP_TYPE_RST      /**< Reset Message type. */
} coap_msg_type_t;

/**@brief Structure to hold a CoAP option. */
typedef struct {
	/** Option number (including the extended delta value if any). */
	u16_t number;

	/** Option length (including the extended length value in any). */
	u16_t length;

	/** Pointer to the memory where the data of the option is located. */
	u8_t *data;
} coap_option_t;


/**@brief Structure to hold a CoAP message configuration.
 *
 * @details The structure is used when calling the \ref coap_message_new API
 *          function. All data supplied will be copied to the created message.
 */
typedef struct {
	/** Callback function to be called when a response matching the token
	 *  is identified.
	 */
	coap_response_callback_t response_callback;

	/** Message token. token_len must be set to indicate how many of the
	 *  bytes should be used in the token.
	 */
	u8_t token[8];

	/** Token size in bytes. */
	u8_t token_len;

	/** Message ID. If 0 is given, the library will replace this number
	 *  with an autogenerated value.
	 */
	u16_t id;

	/** Message type: COAP_TYPE_CON, COAP_TYPE_NON, COAP_TYPE_ACK, or
	 *  COAP_TYPE_RST.
	 */
	coap_msg_type_t type;

	/** Message code (definitions found in coap_msg_code_t). */
	coap_msg_code_t code;

	/** Transport layer variable to associate the message with an
	 *  underlying Transport Layer socket descriptor.
	 */
	coap_transport_handle_t transport;
} coap_message_conf_t;

/**@brief Structure to hold a CoAP message header.
 *
 * @details This structure holds the 4-byte mandatory CoAP header.
 *          The structure uses bitfields to save memory footprint.
 */
typedef struct {
	/** CoAP version number. The current specification RFC7252 mandates this
	 *  to be version 1.
	 */
	u8_t version : 2;

	/** Message type: COAP_TYPE_CON, COAP_TYPE_NON, COAP_TYPE_ACK, or
	 *  COAP_TYPE_RST.
	 */
	u8_t type : 2;

	/** Length of the message token. */
	u8_t token_len : 4;

	/** Message code (definitions found in @ref coap_msg_code_t). */
	u8_t code;

	/** Message ID in little-endian format. Conversion to Network Byte Order
	 *  will be handled by the library.
	 */
	u16_t id;
} coap_message_header_t;

/**@brief Structure to hold a CoAP message.
 *
 * @details The CoAP message structure contains both internal and public members
 *          to serialize and deserialize data supplied from the application to
 *          a byte buffer sent over UDP. The message structure is used both in
 *          transmission and reception, which makes it easy to handle in an
 *          application. Updating the message should be done using the provided
 *          APIs, not by manually assigning new values to the members directly.
 *          Reading the members, on the other hand, is fine.
 */
struct coap_message_t {
	/** Public. Structure containing address information and port number
	 *  to the remote.
	 */
	struct sockaddr *remote;

	/** Public. Structure containing local destination address information
	 *  and port number.
	 */
	struct sockaddr *local;

	/** Public. Header structure containing the mandatory CoAP 4-byte header
	 *  fields.
	 */
	coap_message_header_t header;

	/** Public. Pointer to the payload buffer in the message. */
	u8_t *payload;

	/** Public. Size of the payload in the message. */
	u16_t payload_len;

	/** Public. The number of options in the message. */
	u8_t options_count;

	/** Public. Array options in the message. */
	coap_option_t options[COAP_MAX_NUMBER_OF_OPTIONS];

	/** Public. Miscellaneous pointer to application provided data that
	 *  is associated with the message.
	 */
	void *arg;

	/** Internal. Function callback set by the application to be called when
	 *  a response to this message is received. Should be set by the
	 *  application through a configuration parameter.
	 */
	coap_response_callback_t response_callback;

	/** Internal. Array holding the variable-sized message token. Should
	 *  be set by the application through a configuration parameter.
	 */
	u8_t token[8];

	/** Internal. Transport layer variable to associate the message with
	 *  an underlying Transport Layer socket descriptor.
	 */
	coap_transport_handle_t transport;

	/** Internal. Length of the options including the mandatory header
	 *  with extension bytes and option data. Accumulated every time a
	 *  new options is added.
	 */
	u16_t options_len;

	/** Internal. Index to where the next option or payload can be added
	 *  in the message's data buffer
	 */
	u16_t options_offset;

	/** Internal. Current option number. Used to calculate the next option
	 *  delta when adding new options to the message.
	 */
	u16_t options_delta;

	/** Internal. Data buffer for adding dynamically sized options
	 *  and payload.
	 */
	u8_t *data;

	/** Internal. Length of the provided data buffer for options and
	 *  payload.
	 */
	u16_t data_len;
};


/**@brief Structure to hold a CoAP endpoint resource. */
struct coap_resource_t {
	/** Number of children in the linked list. */
	u8_t child_count;

	/** Bitmask to tell which methods are permitted on the resource.
	 *  Bit values available can be seen in \ref COAP_METHOD_PERMISSION.
	 */
	u16_t permission;

	/** Sibling pointer to the next element in the list. */
	coap_resource_t *sibling;

	/** Pointer to the beginning of the linked list. */
	coap_resource_t *front;

	/** Pointer to the last added child in the list. */
	coap_resource_t *tail;

	/** Callback to the resource handler. */
	coap_method_callback_t callback;

	/** Bitmask to tell which content types are supported by the resource.
	 *  Bit values available can be seen in \ref COAP_CONTENT_TYPE_MASK.
	 */
	u32_t ct_support_mask;

	/** Max age of resource endpoint value. */
	u32_t max_age;

	/** Number of seconds until expire. */
	u32_t expire_time;

	/** Name of the resource. Must be zero terminated. */
	char name[COAP_RESOURCE_MAX_NAME_LEN + 1];
};

/**@brief Initializes the CoAP module.
 *
 * @details Initializes the library module and resets internal queues and
 *          resource registrations.
 *
 * @param[in] token_rand_seed  Random number seed to be used to generate the
 *                             token numbers.
 * @param[in] transport_params Pointer to transport parameters. Providing the
 *                             list of ports to be used by CoAP.
 * @param[in] alloc_fn         Function registered with the module to allocate
 *                             memory. Shall not be NULL.
 * @param[in] free_fn          Function registered with the module to free
 *                             allocated memory. Shall not be NULL.
 *
 * @retval 0 If initialization succeeds.
 */
u32_t coap_init(u32_t token_rand_seed, coap_transport_init_t *transport_params,
		coap_alloc_t alloc_fn, coap_free_t free_fn);

/**@brief Register error handler callback to the CoAP module.
 *
 * @param[in] callback Function to be called upon unknown messages and
 *                     failure.
 *
 * @retval 0 If registration was successful.
 */
u32_t coap_error_handler_register(coap_error_callback_t callback);

/**@brief Register request handler which should handle all incoming requests.
 *
 * @details Setting this request handler redirects all requests to the
 *          application provided callback routine. The callback handler might
 *          be cleared by NULL, making CoAP module handle the requests and do
 *          resource lookup in order to process the requests.
 *
 * @param[in] handler Function pointer to the provided request handler.
 *
 * @retval 0 If registration was successful.
 */
u32_t coap_request_handler_register(coap_request_handler_t handler);

/**@brief Sends a CoAP message.
 *
 * @details Sends out a request using the underlying transport layer. Before
 *          sending, the \ref coap_message_t structure is serialized and added
 *          to an internal message queue in the library. The handle returned
 *          can be used to abort the message from being retransmitted at any
 *          time.
 *
 * @param[out] handle  Handle to the message if CoAP CON/ACK messages has been
 *                     used. Returned by reference.
 * @param[in]  message Message to be sent.
 *
 * @retval 0 If the message was successfully encoded and scheduled for
 *           transmission.
 */
u32_t coap_message_send(u32_t *handle, coap_message_t *message);

/**@brief Abort a CoAP message.
 *
 * @details Aborts an ongoing transmission. If the message has not yet been
 *          sent, it will be deleted from the message queue as well as stop
 *          any ongoing re-transmission of the message.
 *
 * @param[in] handle Handle of the message to abort.
 *
 * @retval 0      If the message was successfully aborted and removed from the
 *                message queue.
 * @retval ENOENT If the message with the given handle was not located in the
 *                message queue.
 */
u32_t coap_message_abort(u32_t handle);

/**@brief Creates CoAP message, initializes, and allocates the needed memory.
 *
 * @details Creates a CoAP message. This is an intermediate representation of
 *          the message, because the message will be serialized by the library
 *          before it is transmitted. The structure is verbose to facilitate
 *          configuring the message. Options, payload, and remote address
 *          information can be added using API function calls.
 *
 * @param[inout] request Pointer to be filled by the allocated CoAP message
 *                       structures.
 * @param[in]    config  Configuration for the message to be created. Manual
 *                       configuration can be performed after the message
 *                       creation, except for the CLIENT port association.
 *
 * @retval 0      If the request was successfully allocated and initialized.
 * @retval EINVAL If local port number was not configured.
 */
u32_t coap_message_new(coap_message_t **request, coap_message_conf_t *config);

/**@brief Deletes the CoAP request message.
 *
 * @details Frees up memory associated with the request message.
 *
 * @param[in] message Pointer to the request message to delete.
 */
u32_t coap_message_delete(coap_message_t *message);

/**@brief Adds a payload to a CoAP message.
 *
 * @details Sets a data payload to a request or response message.
 *
 * This function must be called after all CoAP options have been added.
 * Due to internal buffers in the library, the payload will be added after
 * any options in the buffer. If an option is added after the payload,
 * this option will over-write the payload in the internal buffer.
 *
 * @param[inout] message     Pointer to the message to add the payload to.
 * @param[in]    payload     Pointer to the payload to be added.
 * @param[in]    payload_len Size of the payload to be added.
 *
 * @retval 0      If the payload was successfully added to the message.
 * @retval ENOMEM If the payload could not fit within the allocated payload
 *                memory defined by CONFIG_NRF_COAP_MESSAGE_DATA_MAX_SIZE.
 */
u32_t coap_message_payload_set(coap_message_t *message, void *payload,
			       u16_t payload_len);

/**@brief Adds an empty CoAP option to the message.
 *
 * Option numbers must be in ascending order, adding the one with the smallest
 * number first and greatest last. If the order is incorrect, the delta
 * number calculation will result in an invalid or wrong delta number
 * for the option.
 *
 * @param[inout] message    Pointer to the message to add the option to.
 *                          Should not be NULL.
 * @param[in]    option_num The option number to add to the message.
 *
 * @retval 0      If the empty option was successfully added to the message.
 * @retval ENOMEM If the maximum number of options that can be added to a
 *                message has been reached.
 */
u32_t coap_message_opt_empty_add(coap_message_t *message, u16_t option_num);

/**@brief Adds a u CoAP option to the message.
 *
 * Option numbers must be in ascending order, adding the one with the smallest
 * number first and greatest last. If the order is incorrect, the delta number
 * calculation will result in an invalid or wrong delta number for the option.
 *
 * @param[inout] message    Pointer to the message to add the option to.
 *                          Should not be NULL.
 * @param[in]    option_num The option number to add to the message.
 * @param[in]    data       An unsigned value (8-bit, 16-bit, or 32-bit) casted
 *                          to u32_t. The value of the data is used to determine
 *                          how many bytes CoAP must use to represent this
 *                          option value.
 *
 * @retval 0        If the unsigned integer option was successfully added to
 *                  the message.
 * @retval EMSGSIZE If the data exceeds the available message buffer data size.
 * @retval ENOMEM   If the maximum number of options that can be added to a
 *                  message has been reached.
 */
u32_t coap_message_opt_uint_add(coap_message_t *message, u16_t option_num,
				u32_t data);

/**@brief Adds a string CoAP option to the message.
 *
 * Option numbers must be in ascending order, adding the one with the smallest
 * number first and greatest last. If the order is incorrect, the delta number
 * calculation will result in an invalid or wrong delta number for the option.
 *
 * @param[inout] message    Pointer to the message to add the option to.
 *                          Should not be NULL.
 * @param[in]    option_num The option number to add to the message.
 * @param[in]    data       Pointer to a string buffer to be used as value for
 *                          the option. Should not be NULL.
 * @param[in]    length     Length of the string buffer provided.
 *
 * @retval 0        If the string option was successfully added to the message.
 * @retval EMSGSIZE If the data exceeds the available message buffer data size.
 * @retval ENOMEM   If the maximum number of options that can be added to a
 *                  message has been reached.
 */
u32_t coap_message_opt_str_add(coap_message_t *message, u16_t option_num,
			       u8_t *data, u16_t length);

/**@brief Adds an opaque CoAP option to the message.
 *
 * Option numbers must be in ascending order, adding the one with the smallest
 * number first and greatest last. If the order is incorrect, the delta number
 * calculation will result in an invalid or wrong delta number for the option.
 *
 * @param[inout] message    Pointer to the message to add the option to.
 *                          Should not be NULL.
 * @param[in]    option_num The option number to add to the message.
 * @param[in]    data       Pointer to an opaque buffer to be used as value for
 *                          the option. Should not be NULL.
 * @param[in]    length     Length of the opaque buffer provided.
 *
 * @retval 0        If the opaque option was successfully added to the message.
 * @retval EMSGSIZE If the data exceeds the available message buffer data size.
 * @retval ENOMEM   If the maximum number of options that can be added to a
 *                  message has been reached.
 */
u32_t coap_message_opt_opaque_add(coap_message_t *message, u16_t option_num,
				  u8_t *data, u16_t length);

/**@brief Sets a remote address and port number to a CoAP message.
 *
 * @details Copies the content of the provided pointer into the CoAP message.
 *
 * @param[inout] message Pointer to the message to add the address information
 *                       to. Should not be NULL.
 * @param[in]    address Pointer to a structure holding the address information
 *                       for the remote server or client. Should not be NULL.
 *
 * @retval 0 When copying the content has finished.
 */
u32_t coap_message_remote_addr_set(coap_message_t *message,
				   struct sockaddr *address);

/**@brief Creates a CoAP endpoint resource.
 *
 * @details Initializes the \ref coap_resource_t members.
 *
 * The first resource that is created will be set as the root of the resource
 * hierarchy.
 *
 * @param[in] resource Pointer to coap_resource_t passed in by reference.
 *                     This variable must be stored in non-volatile memory.
 *                     Should not be NULL.
 * @param[in] name     Verbose name of the service (zero-terminated
 *                     string). The maximum length of a name is defined
 *                     by CONFIG_NRF_COAP_RESOURCE_MAX_NAME_LEN.
 *                     and can be adjusted if needed. Should not be NULL.
 *
 * @retval EINVAL If the provided name is larger than the available name buffer
 *                or the pointer to the resource or the provided name buffer
 *                is NULL.
 */
u32_t coap_resource_create(coap_resource_t *resource, const char *name);

/**@brief Adds a child resource.
 *
 * @details The hierarchy is constructed as a linked list with a maximum number
 *          of children. CONFIG_NRF_COAP_RESOURCE_MAX_DEPTH sets the maximum
 *          depth. The maximum number of children can be adjusted if more
 *          levels are needed.
 *
 * @param[in] parent Resource to attach the child to. Should not be NULL.
 * @param[in] child  Child resource to attach. Should not be NULL.
 *
 * @retval 0 If the child was successfully added.
 */
u32_t coap_resource_child_add(coap_resource_t *parent, coap_resource_t *child);

/**@brief Generates .well-known/core string.
 *
 * @details This is a helper function for generating a CoRE link-format encoded
 *          string used for CoAP discovery. The function traverse the resource
 *          hierarchy recursively. The result will be resources listed in
 *          link-format. This function can be called when all resources have
 *          been added by the application.
 *
 * @param[inout] string Buffer to use for the .well-known/core string.
 *                      Should not be NULL.
 * @param[inout] length Length of the string buffer. Returns the used number
 *                      of bytes from the provided buffer.
 *
 * @retval 0      If string generation was successful.
 * @retval EINVAL If the string buffer was a NULL pointer.
 * @retval ENOMEM If the size of the generated string exceeds the given buffer
 *                size.
 * @retval ENOENT If no resource has been registered.
 */
u32_t coap_resource_well_known_generate(u8_t *string, u16_t *length);

/**@brief Get the root resource pointer.
 *
 * @param[out] resource Pointer to be filled with pointer to the root resource.
 *
 * @retval 0      If root resource was located.
 * @retval ENOENT If root resource was not located.
 * @retval EINVAL If output pointer was NULL.
 */
u32_t coap_resource_root_get(coap_resource_t **resource);

/**@brief Check whether a message contains a given CoAP Option.
 *
 * @param[in] message Pointer to the to check for the CoAP Option.
 *                    Should not be NULL.
 * @param[in] option  CoAP Option to check for in the CoAP message.
 *
 * @retval 0      If the CoAP Option is present in the message.
 * @retval EINVAL If the pointer to the message is NULL.
 * @retval ENOENT If the CoAP Option is not present in the message.
 */
u32_t coap_message_opt_present(coap_message_t *message, u16_t option);

/**@brief Check whether a message contains a given CoAP Option and return the
 *        index of the entry in the message option list.
 *
 * @param[in] index   Value by reference to fill the resolved index into.
 *                    Should not be NULL.
 * @param[in] message Pointer to the to check for the CoAP Option.
 *                    Should not be NULL.
 * @param[in] option  CoAP Option to check for in the CoAP message.
 *
 * @retval 0      If the CoAP Option is present in the message.
 * @retval EINVAL If the pointer to the message or the index is NULL.
 * @retval ENOENT If the CoAP Option is not present in the message.
 */
u32_t coap_message_opt_index_get(u8_t *index, coap_message_t *message,
				 u16_t option);

/**@brief Find common content type between the CoAP message and the resource.
 *
 * @details The return value will be the first match between the ACCEPT options
 *          and the supported content types in the resource. The priority is by
 *          content-format ID starting going from the lowest value to the
 *          highest.
 *
 * @param[out] ct       Resolved content type given by reference.
 *                      Should not be NULL.
 * @param[in]  message  Pointer to the message. Should not be NULL.
 * @param[in]  resource Pointer to the resource. Should not be NULL.
 *
 * @retval 0      If match was found.
 * @retval ENOENT If no match was found.
 */
u32_t coap_message_ct_match_select(coap_content_type_t *ct,
				   coap_message_t *message,
				   coap_resource_t *resource);

/**@brief CoAP time tick used for retransmitting any message in the queue
 *        if needed.
 *
 * @retval 0 If time tick update was successfully handled.
 */
u32_t coap_time_tick(void);

/**@brief Setup secure DTLS session.
 *
 * @details For the client role, this API triggers a DTLS handshake. Until the
 *          handshake is complete with the remote, \ref coap_message_send will
 *          fail. For the server role, this API does not create any DTLS
 *          session. A DTLS session is created each time a new client remote
 *          endpoint sends a request on the local port of the server.
 *
 * @note The success of this function does not imply that the DTLS handshake
 *       is successful.
 *
 * @note Only one DTLS session is permitted between a local and remote endpoint.
 *       Therefore, in case a DTLS session was established between the local
 *       and remote endpoint, the existing DTLS session will be reused
 *       irrespective of the role and number of times this API was called. In
 *       case the application desires a fresh security setup, it must first
 *       call the \ref coap_security_destroy to tear down the existing setup.
 *
 * @param[inout] local  Identifies the local IP address and port on which the
 *                      setup is requested. Also indicates the security
 *                      parameters to be used for the setup. In case the
 *                      procedure succeeds, then the transport identifier is
 *                      provided in the tranasport field. This transport pointer
 *                      must be used for all subsequent communication on the
 *                      endpoint.
 * @param[in]    remote Pointer to a structure holding the address information
 *                      for the remote endpoint. If a the device is acting as a
 *                      server, a NULL pointer shall be given as a parameter.
 *                      Rationale: The server is not envisioned to be bound to
 *                      a pre-known client endpoint. Therefore, security server
 *                      settings shall be setup irrespective of the remote
 *                      client.
 *
 * @retval 0 If setup of the secure DTLS session was successful.
 */
u32_t coap_security_setup(coap_local_t *local, struct sockaddr const *remote);

/**@brief Destroy a secure DTLS session.
 *
 * @details Terminate and clean up any session associated with the local port
 *          and the remote.
 *
 * @param[in] local_port Local port to unbind the session from.
 * @param[in] remote     Pointer to a structure holding the address information
 *                       for the remote endpoint. Providing a NULL as remote
 *                       will clean up all DTLS sessions associated with the
 *                       local port.
 *
 * @retval 0 If the destruction of the secure DTLS session was successful.
 */
u32_t coap_security_destroy(coap_transport_handle_t handle);


/**@brief Process loop when using CoAP BSD socket transport implementation.
 *
 * @details This is blocking call. The function unblock is only triggered upon
 *          an socket event registered to select() by CoAP transport. This
 *          function must be called as often as possible in order to dispatch
 *          incoming socket events. Preferred to be put in the application's
 *          main loop or similar.
 **/
void coap_input(void);

#ifdef __cplusplus
}
#endif

#endif /* COAP_API_H__ */

/** @} */
