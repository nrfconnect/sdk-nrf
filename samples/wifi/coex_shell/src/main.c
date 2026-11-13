/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Wi-Fi and Short-Range coexistence sample
 */

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_config.h>
#include <zephyr/net/net_if.h>
#include <zephyr/sys/util.h>

#if defined(CONFIG_NRFX_CLOCK_HFCLK) && \
	(defined(CLOCK_FEATURE_HFCLK_DIVIDE_PRESENT) || NRF_CLOCK_HAS_HFCLK192M)
#include <nrfx_clock_hfclk.h>
#endif

#ifdef CONFIG_SR_COEX_BRINGUP_IN_MAIN
/* Wi-Fi FMAC coexistence transport, used to wait until CM commands can be sent. */
#include <drivers/wifi/nrf71/nrf71_wifi_coex.h>
/* Coexistence driver SR notification API and its CM configuration state. */
#include <drivers/nrf71_sr_coex/nrf71_sr_coex_sr.h>
#include "nrf71_sr_coex_internal.h"
#endif /* CONFIG_SR_COEX_BRINGUP_IN_MAIN */

LOG_MODULE_REGISTER(sr_coex, CONFIG_LOG_DEFAULT_LEVEL);

#ifdef CONFIG_SR_COEX_BRINGUP_IN_MAIN
/** Interval between polls while waiting for the Wi-Fi FMAC transport, in milliseconds. */
#define COEX_POLL_INTERVAL_MS 100U

/** Consecutive ready polls required before the transport is trusted. */
#define COEX_TRANSPORT_STABLE_POLLS 5U

/** Give up waiting for the Wi-Fi transport or the network interface after this long. */
#define COEX_TRANSPORT_TIMEOUT_MS 60000U

/** Poll until the Wi-Fi FMAC coexistence transport reports ready. */
static int coex_wait_transport_ready(void)
{
	uint32_t elapsed_ms = 0U;
	unsigned int stable_polls = 0U;

	while (stable_polls < COEX_TRANSPORT_STABLE_POLLS) {
		if (elapsed_ms >= COEX_TRANSPORT_TIMEOUT_MS) {
			LOG_ERR("Coex: Wi-Fi transport not ready after %u ms (%d)", elapsed_ms,
				-ETIMEDOUT);
			return -ETIMEDOUT;
		}

		if (nrf71_wifi_coex_is_ready()) {
			stable_polls++;
		} else {
			stable_polls = 0U;
		}

		k_msleep(COEX_POLL_INTERVAL_MS);
		elapsed_ms += COEX_POLL_INTERVAL_MS;
	}

	LOG_INF("Coex: Wi-Fi transport stable after %u ms", elapsed_ms);

	return 0;
}

/*
 * Wait for interface up only. The nRF71 driver keeps the interface dormant
 * until a Wi-Fi association, so net_if_is_up() stays false while unassociated.
 */
static int coex_wait_net_if_up(void)
{
	uint32_t elapsed_ms = 0U;

	while (elapsed_ms < COEX_TRANSPORT_TIMEOUT_MS) {
		struct net_if *iface = net_if_get_default();

		if ((iface != NULL) && net_if_is_admin_up(iface)) {
			LOG_INF("Coex: network interface admin up after %u ms", elapsed_ms);
			return 0;
		}

		k_msleep(COEX_POLL_INTERVAL_MS);
		elapsed_ms += COEX_POLL_INTERVAL_MS;
	}

	LOG_ERR("Coex: network interface not admin up after %u ms (%d)",
		COEX_TRANSPORT_TIMEOUT_MS, -ETIMEDOUT);

	return -ETIMEDOUT;
}

/*
 * The driver runs the CM bring-up itself when the Wi-Fi interface comes up, so
 * wait for it to report the CM as active.
 */
static int coex_wait_cm_active(void)
{
	uint32_t elapsed_ms = 0U;

	while (elapsed_ms < COEX_TRANSPORT_TIMEOUT_MS) {
		const char *state = cd_wifi_state_name();

		if (strcmp(state, "active") == 0) {
			LOG_INF("Coex: CM active after %u ms", elapsed_ms);
			return 0;
		}

		if (strcmp(state, "failed") == 0) {
			LOG_ERR("Coex: CM configuration failed (%d)", -EIO);
			return -EIO;
		}

		k_msleep(COEX_POLL_INTERVAL_MS);
		elapsed_ms += COEX_POLL_INTERVAL_MS;
	}

	LOG_ERR("Coex: CM not active after %u ms (%d)", COEX_TRANSPORT_TIMEOUT_MS,
		-ETIMEDOUT);

	return -ETIMEDOUT;
}

/**
 * Wait until the coexistence driver has configured the CM, then report the SR
 * radio as ready.
 *
 * On Wi-Fi interface up the driver runs the full CM bring-up sequence: COEXC
 * tables, priority ranges, user params, coex params and finally enable, each
 * waiting for its completion event. This waits for the Wi-Fi FMAC transport,
 * the network interface and then for that sequence to finish.
 *
 * The SR notification, when enabled with CONFIG_SR_COEX_SR_NOTIFY_IN_MAIN,
 * enables coexistence in the SR driver and opens the driver's runtime
 * coexistence gate. The sample also sends it when Bluetooth is enabled, so it is
 * harmless to send it twice.
 */
static int coex_init_and_enable(void)
{
	int ret;

	ret = coex_wait_transport_ready();
	if (ret != 0) {
		return ret;
	}

	ret = coex_wait_net_if_up();
	if (ret != 0) {
		return ret;
	}

	ret = coex_wait_cm_active();
	if (ret != 0) {
		return ret;
	}

#ifdef CONFIG_SR_COEX_SR_NOTIFY_IN_MAIN
	ret = nrf71_sr_coex_sr_power_notify(NRF71_SR_COEX_SR_POWERED_UP_READY);
	if (ret != 0) {
		LOG_ERR("Coex: SR power notify failed: %d", ret);
		return ret;
	}

	LOG_INF("Coex: SR power notify accepted");
#endif /* CONFIG_SR_COEX_SR_NOTIFY_IN_MAIN */

	LOG_INF("Coex: radios reported ready");

	return 0;
}
#endif /* CONFIG_SR_COEX_BRINGUP_IN_MAIN */

int main(void)
{
#if defined(CONFIG_NRFX_CLOCK_HFCLK) && \
	(defined(CLOCK_FEATURE_HFCLK_DIVIDE_PRESENT) || NRF_CLOCK_HAS_HFCLK192M)
	/* Run the CPU at the full HFCLK frequency, without a divider. */
	nrfx_clock_hfclk_divider_set(NRF_CLOCK_HFCLK_DIV_1);
#endif
	LOG_INF("Starting %s with CPU frequency: %u MHz", CONFIG_BOARD,
		(unsigned int)(SystemCoreClock / MHZ(1)));

#ifdef CONFIG_SR_COEX_BRINGUP_IN_MAIN
	/*
	 * The coexistence driver initialises itself at boot (SYS_INIT), so there is no
	 * init call here. A failure is not fatal: the shell stays usable, but
	 * coexistence stays disabled.
	 */
	if (coex_init_and_enable() != 0) {
		LOG_WRN("Coex: bring-up failed, coexistence is not enabled");
	}
#endif

#if defined(CONFIG_NET_CONFIG_SETTINGS) && defined(CONFIG_WIFI)
	/*
	 * Without this, DHCPv4 starts on first interface and if that is not Wi-Fi or
	 * only supports IPv6, then it is an issue. (E.g., OpenThread)
	 *
	 * So, we start DHCPv4 on Wi-Fi interface always, independent of the ordering.
	 */
	const struct device *dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_wifi));
	struct net_if *wifi_iface = net_if_lookup_by_dev(dev);

	if (!wifi_iface) {
		LOG_ERR("No network interface for the Wi-Fi device");
		return -ENODEV;
	}

	/* As both are Ethernet, we need to set specific interface */
	net_if_set_default(wifi_iface);

	net_config_init_app(dev, "Initializing network");
#endif

	return 0;
}
