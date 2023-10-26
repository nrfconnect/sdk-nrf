/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/lwm2m.h>
#include <modem/lte_lc.h>
#include <nrf_modem_at.h>
#include <zephyr/logging/log.h>
#include <net/lwm2m_client_utils.h>

LOG_MODULE_REGISTER(lwm2m_rai, CONFIG_LWM2M_CLIENT_UTILS_LOG_LEVEL);
static void lte_rrc_event_handler(const struct lte_lc_evt *const evt);

static bool rrc_connected;

static int set_rel14feat(void)
{
	int ret;

	ret = nrf_modem_at_printf("AT%%REL14FEAT=0,1,0,0,0");
	if (ret < 0) {
		LOG_ERR("AT command failed, error code: %d", ret);
		return -EFAULT;
	}

	return 0;
}

int lwm2m_rai_get(enum lwm2m_rai_mode *mode)
{
	uint16_t rai_tmp;
	int ret;

	if (mode == NULL) {
		return -EINVAL;
	}

	ret = nrf_modem_at_scanf("AT%RAI?", "%%RAI: %hu", &rai_tmp);
	if (ret != 1) {
		LOG_ERR("Failed to read RAI, error code: %d", ret);
		return ret;
	}

	LOG_DBG("AS RAI: %d", rai_tmp);
	if (rai_tmp) {
		*mode = LWM2M_RAI_MODE_ENABLED;
	} else {
		*mode = LWM2M_RAI_MODE_DISABLED;
	}

	return 0;
}

int lwm2m_rai_req(enum lwm2m_rai_mode mode)
{
	int ret;

	ret = nrf_modem_at_printf("AT%%RAI=%d", mode);
	if (ret) {
		LOG_ERR("AT command failed, error code: %d", ret);
		return -EFAULT;
	}

	return 0;
}

static void lwm2m_lte_handler_register(void)
{
	lte_lc_register_handler(lte_rrc_event_handler);
}

static inline const char *lte_lc_rrc_mode2str(enum lte_lc_rrc_mode rrc_mode)
{
#if CONFIG_LWM2M_CLIENT_UTILS_LOG_LEVEL >= LOG_LEVEL_DBG
	switch (rrc_mode) {
	case LTE_LC_RRC_MODE_IDLE:
		return "Idle";
	case LTE_LC_RRC_MODE_CONNECTED:
		return "Connected";
	default:
		break;
	}

	return "<Unknown>";
#else
	ARG_UNUSED(rrc_mode);

	return "";
#endif /* CONFIG_LWM2M_CLIENT_UTILS_LOG_LEVEL >= LOG_LEVEL_DBG */
}

static void lte_rrc_event_handler(const struct lte_lc_evt *const evt)
{
	switch (evt->type) {
	case LTE_LC_EVT_RRC_UPDATE:
		if (evt->rrc_mode == LTE_LC_RRC_MODE_CONNECTED) {
			rrc_connected = true;
		} else {
			rrc_connected = false;
		}
		LOG_DBG("LTE RRC mode: %s", lte_lc_rrc_mode2str(evt->rrc_mode));
		break;
	default:
		break;
	}
}

int lwm2m_rai_no_data(void)
{
	int ret;
	struct lwm2m_ctx *ctx;

	if (rrc_connected) {
		ctx = lwm2m_rd_client_ctx();
		if (!ctx) {
			LOG_ERR("No context");
			return -EPERM;
		}

		if (ctx->sock_fd >= 0) {
			int dummy = 1;
			LOG_DBG("Set socket option SO_RAI_NO_DATA");
			ret = setsockopt(ctx->sock_fd, SOL_SOCKET, SO_RAI_NO_DATA, &dummy,
					 sizeof(dummy));

			if (ret < 0) {
				ret = -errno;
				LOG_ERR("Failed to set RAI socket option, error code: %d", ret);
				return ret;
			}
		}
	}

	return 0;
}

int lwm2m_rai_last(void)
{
	int ret;
	struct lwm2m_ctx *ctx;

	if (rrc_connected) {
		ctx = lwm2m_rd_client_ctx();
		if (!ctx) {
			LOG_ERR("No context");
			return -EPERM;
		}

		if (ctx->sock_fd >= 0) {
			int dummy = 1;
			LOG_DBG("Set socket option SO_RAI_LAST");
			ret = setsockopt(ctx->sock_fd, SOL_SOCKET, SO_RAI_LAST, &dummy,
					 sizeof(dummy));

			if (ret < 0) {
				ret = -errno;
				LOG_ERR("Failed to set RAI socket option, error code: %d", ret);
				return ret;
			}
		}
	}

	return 0;
}

int lwm2m_init_rai(void)
{
	int ret;
	enum lwm2m_rai_mode mode;

	ret = set_rel14feat();
	if (ret) {
		LOG_ERR("set_rel14feat failed, error code: %d", ret);
		return ret;
	}

	ret = lwm2m_rai_req(LWM2M_RAI_MODE_ENABLED);
	if (ret) {
		LOG_ERR("RAI request failed, error code: %d", ret);
		return ret;
	}

	ret = lwm2m_rai_get(&mode);
	if (ret) {
		LOG_ERR("Get RAI mode failed, error code: %d", ret);
		return ret;
	}

	lwm2m_lte_handler_register();

	return 0;
}
