#ifndef __OT_COAP_UTILS_H__
#define __OT_COAP_UTILS_H__

#include <openthread/instance.h>

#include <coap_server_client_interface.h>

/**@brief Type definition of the function used to handle light resource change.
 */
typedef void (*light_request_callback_t)(u8_t cmd);
typedef void (*provisioning_request_callback_t)();

int ot_coap_init(otStateChangedCallback on_state_changed,
		 provisioning_request_callback_t on_provisioning_request,
		 light_request_callback_t on_light_request);

void ot_coap_activate_provisioning(void);

void ot_coap_deactivate_provisioning(void);

bool ot_coap_is_provisioning_active(void);

#endif
