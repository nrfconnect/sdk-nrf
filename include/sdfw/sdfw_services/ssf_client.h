/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SSF_CLIENT_H__
#define SSF_CLIENT_H__

#include <stddef.h>
#include <stdint.h>

#include <sdfw/sdfw_services/ssf_errno.h>

/**
 * @brief       Structure to hold a raw request or response.
 */
struct ssf_client_raw_data {
	uint8_t *data;
	size_t len;
};

/**
 * @brief       SSF request encode function prototype. Function of this type are
 *              typically generated from cddl with zcbor.
 */
typedef int (*request_encoder)(uint8_t *payload, size_t payload_len, void *input,
			       size_t *payload_len_out);

/**
 * @brief       SSF response decode function prototype. Function of this type are
 *              typically generated from cddl with zcbor.
 */
typedef int (*response_decoder)(const uint8_t *payload, size_t payload_len, void *result,
				size_t *payload_len_out);

/**
 * @brief       SSF service definition (client).
 */
struct ssf_client_srvc {
	/* Service ID (unique) */
	uint16_t id;
	/* Client's version number */
	uint16_t version;
	/* Request encoder */
	request_encoder req_encode;
	/* Response decoder */
	response_decoder rsp_decode;
	/* Request buffer size */
	size_t req_buf_size;
};

/**
 * @brief       Define a read-only service definition object.
 *
 * @param       _name  Name of service definition object.
 * @param[in]   _srvc_name  Short uppercase service name. Must match the service_name variable used
 *                          when specifying the service with the Kconfig.template.service template.
 * @param[in]   _req_encode  Function of type @ref request_encoder. Used when encoding requests.
 * @param[in]   _rsp_decode  Function of type @ref response_decoder. Used when decoding responses.
 */
#define SSF_CLIENT_SERVICE_DEFINE(_name, _srvc_name, _req_encode, _rsp_decode)                     \
	static const struct ssf_client_srvc _name = {                                              \
		.id = (CONFIG_SSF_##_srvc_name##_SERVICE_ID),                                      \
		.version = (CONFIG_SSF_##_srvc_name##_SERVICE_VERSION),                            \
		.req_encode = (request_encoder)_req_encode,                                        \
		.rsp_decode = (response_decoder)_rsp_decode,                                       \
		.req_buf_size = (CONFIG_SSF_##_srvc_name##_SERVICE_BUFFER_SIZE)                    \
	}

/**
 * @brief       Initialize SDFW Service Framework. This will initialize underlying transport
 *              and wait for remote domains to establish connection.
 *
 * @return      0 on success
 *              -SSF_EINVAL if transport failed to initialize.
 */
int ssf_client_init(void);

/**
 * @brief       Send a request and wait for a response.
 *
 * @note        The req structure is encoded with the service definition's req_encode function
 *              before it is passed to the underlying transport. The response is decoded with
 *              the service definition's rsp_decode function.
 *
 * @param[in]   srvc  A pointer to a service definition object.
 * @param[in]   req  A pointer to the (zcbor) structure holding the request to be sent.
 * @param[out]  decoded_rsp  A pointer to a (zcbor) structure, allocated by the caller, that
 *                           the response will be decoded into.
 * @param[out]  rsp_pkt  Holds the address of the response packet upon return. Used
 *                       with @ref ssf_client_decode_done to free the response packet.
 *                       This is only necessary if the response contains a CBOR bstr or tstr.
 *                       If the response does not contain a CBOR bstr or tstr, set this to NULL
 *                       to have the function free the response before returning.
 *
 * @return      0 on success
 *              -SSF_EINVAL if parameters are invalid.
 *              -SSF_EBUSY if transport is not initialized.
 *              -SSF_EPROTO if encode or decode fails. Either on client or server side.
 *              -SSF_ENOMEM if allocation of transport layer buffer fails.
 *              -SSF_EIO if sending with underlying transport fails.
 *              -SSF_EMSGSIZE if message is too long.
 *              -SSF_EPERM if mismatching domain id on server side.
 *              -SSF_ENOTSUP if service version number on client and server side does not match.
 *              -SSF_EPROTONOSUPPORT if the service is not found on server side.
 */
int ssf_client_send_request(const struct ssf_client_srvc *srvc, void *req, void *decoded_rsp,
			    const uint8_t **rsp_pkt);

/**
 * @brief       Send a raw byte string request and wait for a response.
 *
 * @param[in]   srvc  A pointer to a service definition object.
 * @param[in]   raw_req  Raw request data to be sent.
 * @param[out]  raw_rsp  Buffer to copy the raw response into and size of the buffer.
 *
 * @return      0 on success
 *              -SSF_EINVAL if parameters are invalid.
 *              -SSF_EBUSY if transport is not initialized.
 *              -SSF_EPROTO if zcbor encode or decode fails. Either on client or server side.
 *              -SSF_ENOMEM if allocation of transport layer buffer fails.
 *              -SSF_EIO if sending with underlying transport fails.
 *              -SSF_EMSGSIZE if message is too long.
 *              -SSF_EPERM if mismatching domain id on server side.
 *              -SSF_ENOTSUP if service version number on client and server side does not match.
 *              -SSF_EPROTONOSUPPORT if the service is not found on server side.
 */
int ssf_client_send_raw_request(const struct ssf_client_srvc *srvc,
				struct ssf_client_raw_data raw_req,
				struct ssf_client_raw_data *raw_rsp);

/**
 * @brief       Free transport layer response buffer when decoding of response is finished.
 *
 * @note        When decoding CBOR bstr and tstr with zcbor, the zcbor structure will hold
 *              a pointer into the response buffer and length of the string. Therefore, make
 *              sure to not call @ref ssf_client_decode_done before the bstr/tstr have been
 *              processed or copied elsewhere.
 *
 * @param[in]   rsp_pkt  A pointer to the response packet to free.
 */
void ssf_client_decode_done(const uint8_t *rsp_pkt);

#endif /* SSF_CLIENT_H__ */
