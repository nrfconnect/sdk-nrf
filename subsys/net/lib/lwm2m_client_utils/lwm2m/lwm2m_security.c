/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <net/lwm2m.h>
#include <net/lwm2m_client_utils.h>
#include <lwm2m_engine.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(lwm2m_security, CONFIG_LWM2M_CLIENT_UTILS_LOG_LEVEL);

/* LWM2M_OBJECT_SECURITY_ID */
#define SECURITY_CLIENT_PK_ID 3

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
	uint16_t server_url_len;
	uint8_t server_url_flags;

	/* setup SECURITY object */

	/* Server URL */
	ret = lwm2m_engine_get_res_data("0/0/0",
					(void **)&server_url, &server_url_len,
					&server_url_flags);
	if (ret < 0) {
		return ret;
	}

	snprintk(server_url, server_url_len, "%s", CONFIG_LWM2M_CLIENT_UTILS_SERVER);

	/* Security Mode */
	lwm2m_engine_set_u8("0/0/2",
			    IS_ENABLED(CONFIG_LWM2M_DTLS_SUPPORT) ? 0 : 3);
#if defined(CONFIG_LWM2M_DTLS_SUPPORT)
	ctx->tls_tag = IS_ENABLED(CONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP) ?
			       CONFIG_LWM2M_CLIENT_UTILS_BOOTSTRAP_TLS_TAG :
				     CONFIG_LWM2M_CLIENT_UTILS_SERVER_TLS_TAG;
	ctx->load_credentials = load_credentials_dummy;

	lwm2m_engine_set_string(LWM2M_PATH(LWM2M_OBJECT_SECURITY_ID, 0, SECURITY_CLIENT_PK_ID),
				endpoint);
#endif /* CONFIG_LWM2M_DTLS_SUPPORT */

#if defined(CONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP)
	/* Mark 1st instance of security object as a bootstrap server */
	lwm2m_engine_set_u8("0/0/1", 1);
#else
	/* Security and Server object need matching Short Server ID value. */
	lwm2m_engine_set_u16("0/0/10", 101);
	lwm2m_engine_set_u16("1/0/0", 101);
#endif

	return ret;
}
