#ifndef CBKPROXY_H
#define CBKPROXY_H

#include <stdint.h>

/** @brief Get output callback proxy.
 *
 * This function creates an output callback proxy. Calling proxy will call
 * handler with untouched parameters on stack and registers. Because of that
 * type of handler must be the same as callback proxy. This function is
 * type independed, so function pointer are void* and caller is responsible
 * for correct type casting. Handler is called with one additional information
 * (obtained by cbkproxy_out_start_handler) which is a slot number. It can be
 * used to recognize with callback proxy was called in single handler.
 *
 * @param index    Slot index.
 * @param handler  Pointer to handler function. The function has to start with
 *                 cbkproxy_out_start_handler() and end with
 *                 cbkproxy_out_end_handler().
 * @returns        Pointer to function that calls provided handler using
 *                 provided slot index. The pointer has to be casted to
 *                 the same type as handler. NULL when index is too high or
 *                 the function was called again with the same index, but
 *                 different handler.
 */
void *cbkproxy_out_get(int index, void *handler);

/** @brief Sets input callback proxy.
 *
 * Calling the function again with the same callback parameter will not
 * allocate new slot, but it will return previously allocated.
 *
 * @param callback Callback function.
 *
 * @returns Slot number or -1 if no more slots are available or the function
 *          was called with the same callback, but different handler.
 */
int cbkproxy_in_set(void *callback);

/** @brief Gets input callback proxy.
 *
 * @param      index     Slot index.
 * @param[out] callback  Callback function.
 *
 * @returns Proxy handler function or NULL if slot index is invalid.
 */
void *cbkproxy_in_get(int index);

#endif /* CBKPROXY_H */
