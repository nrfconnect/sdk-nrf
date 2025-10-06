/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 */

#include <zephyr/device.h>
#include <zephyr/types.h>
#include <zephyr/net/offloaded_netdev.h>
#include <zephyr/net/conn_mgr_connectivity_impl.h>
#include <zephyr/logging/log.h>
#include <modem/nrf_modem_lib.h>
#include <modem/lte_lc.h>
#include <modem/pdn.h>

#include "lte_ip_addr_helper.h"

LOG_MODULE_REGISTER(nrf_modem_lib_netif_ntn, CONFIG_NRF_MODEM_LIB_NET_IF_NTN_LOG_LEVEL);

/* Forward declarations */
static int lte_net_if_disconnect_ntn(struct conn_mgr_conn_binding *const if_conn);

static void lte_net_if_init_ntn(struct conn_mgr_conn_binding *if_conn)
{
	LOG_DBG("Initializing NTN modem network interface");

        /* Init stuff goes here */
}

int lte_net_if_enable_ntn(void)
{
        LOG_DBG("Enabling NTN");

	if (!nrf_modem_is_initialized()) {
		return nrf_modem_lib_init();
	}

	return 0;
}

int lte_net_if_disable_ntn(void)
{
        LOG_DBG("Disabling NTN");

	if (IS_ENABLED(CONFIG_NRF_MODEM_LIB_NET_IF_NTN_DOWN_DEFAULT_LTE_DISCONNECT)) {
		return lte_net_if_disconnect_ntn(NULL);
	} else {
		return nrf_modem_lib_shutdown();
	}

        return 0;
}

static int lte_net_if_connect_ntn(struct conn_mgr_conn_binding *const if_conn)
{
	ARG_UNUSED(if_conn);

        LOG_DBG("Connecting to NTN");

        /* Connect stuff goes here */

	return 0;
}

static int lte_net_if_disconnect_ntn(struct conn_mgr_conn_binding *const if_conn)
{
	ARG_UNUSED(if_conn);

        LOG_DBG("Disconnecting from NTN");

        /* Disconnect stuff goes here */

	return 0;
}

/* Bind connectivity APIs for NTN.
 * extern in nrf9x_sockets.c
 */
struct conn_mgr_conn_api lte_net_if_conn_mgr_api_ntn = {
	.init = lte_net_if_init_ntn,
	.connect = lte_net_if_connect_ntn,
	.disconnect = lte_net_if_disconnect_ntn,
};
