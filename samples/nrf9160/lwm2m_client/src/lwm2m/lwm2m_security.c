/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <net/lwm2m.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(app_lwm2m_security, CONFIG_APP_LOG_LEVEL);

#if defined(CONFIG_LWM2M_DTLS_SUPPORT)
#include <net/tls_credentials.h>

#define TLS_TAG			35724861
#endif

#include "config.h"

#define SERVER_ADDR	CONFIG_APP_LWM2M_SERVER

#if defined(CONFIG_LWM2M_DTLS_SUPPORT)
static int load_credentials_dummy(struct lwm2m_ctx *client_ctx)
{
	return 0;
}
#endif

int lwm2m_init_security(struct lwm2m_ctx *ctx, char *endpoint)
{
	int ret;
	char *server_url;
	u16_t server_url_len;
	u8_t server_url_flags;

	/* setup SECURITY object */

	/* Server URL */
	ret = lwm2m_engine_get_res_data("0/0/0",
					(void **)&server_url, &server_url_len,
					&server_url_flags);
	if (ret < 0) {
		return ret;
	}

	snprintk(server_url, server_url_len, "coap%s//%s%s%s",
		 IS_ENABLED(CONFIG_LWM2M_DTLS_SUPPORT) ? "s:" : ":",
		 strchr(SERVER_ADDR, ':') ? "[" : "", SERVER_ADDR,
		 strchr(SERVER_ADDR, ':') ? "]" : "");

	/* Security Mode */
	lwm2m_engine_set_u8("0/0/2",
			    IS_ENABLED(CONFIG_LWM2M_DTLS_SUPPORT) ? 0 : 3);
#if defined(CONFIG_LWM2M_DTLS_SUPPORT)
	ctx->tls_tag = TLS_TAG;
	ctx->load_credentials = load_credentials_dummy;

	lwm2m_engine_set_string("0/0/3", endpoint);
	lwm2m_engine_set_opaque("0/0/5",
				(void *)client_psk, sizeof(client_psk));
#endif /* CONFIG_LWM2M_DTLS_SUPPORT */
	return ret;
}
