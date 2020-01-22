/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef AT_NOTIF_H_
#define AT_NOTIF_H_

/**
 * @file at_notif.h
 *
 * @defgroup at_notif AT command notification manager
 *
 * @{
 *
 * @brief Public APIs for the AT command notification manager.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/types.h>

/**
 * @typedefs at_notif_handler_t
 *
 * Because this driver let multiple threads share the same socket, it must make
 * sure that the correct thread gets the correct data returned from the AT
 * interface. Notifications will be dispatched to handlers registered using
 * the @ref at_notif_register_handler() function. Handlers can be de-registered
 * using the @ref at_notif_deregister_handler() function.
 *
 * @param context      Pointer to context provided by the module which has
 *                     registered the handler.
 * @param response     Null terminated string containing the modem message
 *
 */
typedef void (*at_notif_handler_t)(void *context, const char *response);

/**@brief Initialize AT command notification manager.
 *
 * @return Zero on success, non-zero otherwise.
 */
int at_notif_init(void);

/**
 * @brief Function to register AT command notification handler
 *
 * @note  If the same combination of context and handler exists in the memory,
 *        then the request will be ignored and command execution will be
 *        regarded as finished successfully.
 *
 * @param context Pointer to context provided by the module which has
 *                registered the handler.
 * @param handler Pointer to a received notification handler function of type
 *                @ref at_notif_handler_t.
 *
 * @retval 0            If command execution was successful.
 * @retval -ENOBUFS     If memory cannot be allocated.
 * @retval -EINVAL      If handler is a NULL pointer.
 */
int at_notif_register_handler(void *context, at_notif_handler_t handler);

/**
 * @brief Function to de-register AT command notification handler
 *
 * @param context Pointer to context provided by the module which has
 *                registered the handler.
 * @param handler Pointer to a received notification handler function of type
 *                @ref at_notif_handler_t.
 *
 * @retval 0            If command execution was successful.
 * @retval -ENXIO       If the combination of context and handler cannot be
 *                      found.
 * @retval -EINVAL      If handler is a NULL pointer.
 */
int at_notif_deregister_handler(void *context, at_notif_handler_t handler);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* AT_NOTIF_H_ */
