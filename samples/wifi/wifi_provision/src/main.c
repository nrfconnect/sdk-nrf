/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_config.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/conn_mgr_monitor.h>
#include <zephyr/net/conn_mgr_connectivity.h>
#include <dk_buttons_and_leds.h>
#include <net/wifi_provision.h>

LOG_MODULE_REGISTER(wifi_provision_sample, CONFIG_WIFI_PROVISION_SAMPLE_LOG_LEVEL);

/* Macro used to subscribe to specific Zephyr NET management events. */
#define L4_EVENT_MASK (NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED)
#define CONN_LAYER_EVENT_MASK (NET_EVENT_CONN_IF_FATAL_ERROR)

/* Macro called upon a fatal error, reboots the device. */
#define FATAL_ERROR()								\
	LOG_ERR("Fatal error!%s", IS_ENABLED(CONFIG_RESET_ON_FATAL_ERROR) ?	\
				  " Rebooting the device" : "");		\
	LOG_PANIC();								\
	IF_ENABLED(CONFIG_REBOOT, (sys_reboot(0)))

/* Callbacks for Zephyr NET management events. */
static struct net_mgmt_event_callback l4_cb;
static struct net_mgmt_event_callback conn_cb;

static void l4_event_handler(struct net_mgmt_event_callback *cb,
			     uint32_t event,
			     struct net_if *iface)
{
	switch (event) {
	case NET_EVENT_L4_CONNECTED:
		LOG_INF("Network connected");

		int ret = dk_set_led_on(DK_LED2);
		if (ret) {
			LOG_ERR("dk_set_led_on, error: %d", ret);
			FATAL_ERROR();
		}

		break;
	case NET_EVENT_L4_DISCONNECTED:
		LOG_INF("Network disconnected");
		break;
	default:
		/* Don't care */
		return;
	}
}

static void connectivity_event_handler(struct net_mgmt_event_callback *cb,
				       uint32_t event,
				       struct net_if *iface)
{
	if (event == NET_EVENT_CONN_IF_FATAL_ERROR) {
		LOG_ERR("NET_EVENT_CONN_IF_FATAL_ERROR");
		FATAL_ERROR();
		return;
	}
}

static void button_handler(uint32_t button_states, uint32_t has_changed)
{
	if (has_changed & DK_BTN1_MSK) {
		if (button_states & DK_BTN1_MSK) {
			LOG_DBG("Button 1 pressed");

			/* Reset the Wi-Fi provisioning library. */
			int ret = wifi_provision_reset();
			if (ret) {
				LOG_ERR("wifi_provision_reset, error: %d", ret);
				FATAL_ERROR();
			}
		}
	}
}

/* Callback for Wi-Fi provisioning events. */
static void wifi_provision_handler(const struct wifi_provision_evt *evt)
{
	int ret;

	switch (evt->type) {
	case WIFI_PROVISION_EVT_STARTED:
		LOG_INF("Provisioning started");

		ret = dk_set_led_on(DK_LED1);
		if (ret) {
			LOG_ERR("dk_set_led_on, error: %d", ret);
			FATAL_ERROR();
		}

		break;
	case WIFI_PROVISION_EVT_CLIENT_CONNECTED:
		LOG_INF("Client connected");
		break;
	case WIFI_PROVISION_EVT_CLIENT_DISCONNECTED:
		LOG_INF("Client disconnected");
		break;
	case WIFI_PROVISION_EVT_CREDENTIALS_RECEIVED:
		LOG_INF("Wi-Fi credentials received");
		break;
	case WIFI_PROVISION_EVT_COMPLETED:
		LOG_INF("Provisioning completed");
		break;
	case WIFI_PROVISION_EVT_REBOOT:
		LOG_INF("Unprovisioning completed, rebooting");

		LOG_INF("Rebooting...");
		LOG_PANIC();
		IF_ENABLED(CONFIG_REBOOT, (sys_reboot(0)));

		break;
	case WIFI_PROVISION_EVT_FATAL_ERROR:
		LOG_ERR("Provisioning failed");
		FATAL_ERROR();
		break;
	default:
		/* Don't care */
		return;
	}
}

static int wifi_power_saving_disable(void)
{
	int ret;
	struct net_if *iface = net_if_get_first_wifi();
		struct wifi_ps_params params = {
		.enabled = WIFI_PS_DISABLED
	};

	ret = net_mgmt(NET_REQUEST_WIFI_PS, iface, &params, sizeof(params));
	if (ret) {
		LOG_ERR("Failed to disable power save, error: %d", ret);
		FATAL_ERROR();
		return ret;
	}

	return 0;
}

int main(void)
{
	int ret;

	LOG_INF("Wi-Fi provision sample started");

	ret = dk_buttons_init(button_handler);
	if (ret) {
		LOG_ERR("dk_buttons_init, error: %d", ret);
		FATAL_ERROR();
		return ret;
	}

	ret = dk_leds_init();
	if (ret) {
		LOG_ERR("dk_leds_init, error: %d", ret);
		FATAL_ERROR();
		return ret;
	}

	ret = conn_mgr_all_if_up(true);
	if (ret) {
		LOG_ERR("conn_mgr_all_if_up, error: %d", ret);
		FATAL_ERROR();
		return ret;
	}

	LOG_INF("Network interface brought up");

	/* Start the Wi-Fi provisioning library.
	 * This function blocks until provisioning has finished.
	 */
	ret = wifi_provision_start(wifi_provision_handler);
	if (ret) {
		LOG_ERR("wifi_provision_init, error: %d", ret);
		FATAL_ERROR();
		return ret;
	}

	/* Setup handler for Zephyr NET Connection Manager events. */
	net_mgmt_init_event_callback(&l4_cb, l4_event_handler, L4_EVENT_MASK);
	net_mgmt_add_event_callback(&l4_cb);

	/* Setup handler for Zephyr NET Connection Manager Connectivity layer. */
	net_mgmt_init_event_callback(&conn_cb, connectivity_event_handler, CONN_LAYER_EVENT_MASK);
	net_mgmt_add_event_callback(&conn_cb);

	/* Disable power save in STA mode. Temporary fix - In PS mode the device is not discoverable
	 * using mDNS.
	 */
	ret = wifi_power_saving_disable();
	if (ret) {
		LOG_ERR("wifi_power_saving_disable, error: %d", ret);
		FATAL_ERROR();
		return ret;
	}

	ret = conn_mgr_all_if_connect(true);
	if (ret) {
		LOG_ERR("conn_mgr_all_if_connect, error: %d", ret);
		FATAL_ERROR();
		return ret;
	}

	return 0;
}
