#ifndef __COAP_UTILS_H__
#define __COAP_UTILS_H__

#include <net/coap.h>

void coap_init(void);

int coap_send_request(enum coap_method method, const struct sockaddr_in6 *addr6,
		      const char *const *uri_path_options, u8_t *payload,
		      u16_t payload_size, coap_reply_t reply_cb);

#endif
