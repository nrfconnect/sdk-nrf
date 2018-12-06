/*
 * Copyright (c) 2014 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/** @file coap_resource.h
 *
 * @defgroup iot_sdk_coap_resource CoAP Resource
 * @ingroup iot_sdk_coap
 * @{
 * @brief Private API of Nordic's CoAP Resource implementation.
 */

#ifndef COAP_RESOURCE_H__
#define COAP_RESOURCE_H__

#include <stdint.h>

#include <net/coap_api.h>
#include <net/coap_message.h>

#ifdef __cplusplus
extern "C" {
#endif

/**@brief Initialize the CoAP resource module.
 *
 * @details This function will initialize the root element pointer to NULL.
 *          This way, a new root can be assigned registered. The first
 *          resource added will be set as the new root.
 *
 * @retval 0 This function will always return success.
 */
u32_t coap_resource_init(void);

/**@brief Find a resource by traversing the resource names.
 *
 * @param[out] resource     Located resource.
 * @param[in]  uri_pointers Array of strings which forms the hierarchical path
 *                          to the resource.
 * @param[in]  num_of_uris  Number of URIs supplied through the path pointer
 *                          list.
 *
 * @retval 0      The resource was instance located.
 * @retval ENOENT The resource was not located or no resource has been
 *                registered.
 */
u32_t coap_resource_get(coap_resource_t **resource, u8_t **uri_pointers,
			u8_t num_of_uris);

/**@brief Process the request related to the resource.
 *
 * @details When a request is received and the resource has successfully been
 *          located it will pass on to this function. The method in the request
 *          will be matched against what the service provides of method handling
 *          callbacks. If the request expects a response this will be provided
 *          as output from this function. The memory provided for the response
 *          must be provided from outside.
 *
 * @param[in]    resource Resource that will handle the request.
 * @param[in]    request  The request to be handled.
 * @param[inout] response Response message which can be used by the resource
 *                        populate the response message.
 */
u32_t coap_resource_process_request(coap_resource_t *resource,
				    coap_message_t *request,
				    coap_message_t *response);

#ifdef __cplusplus
}
#endif

#endif /* COAP_MESSAGE_H__ */

/** @} */
