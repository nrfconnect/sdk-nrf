/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#ifdef CONFIG_BOARD_NRF54H20DK
#include <nrfs_backend_ipc_service.h>
#include <nrfs_gdpwr.h>

static void gdpwr_handler(nrfs_gdpwr_evt_t const *p_evt, void *context)
{
}

static void clear_global_power_domains_requests(void)
{
	int service_status;
	int tst_ctx = 1;

	service_status = nrfs_gdpwr_init(gdpwr_handler);
	service_status = nrfs_gdpwr_power_request(GDPWR_POWER_DOMAIN_ACTIVE_SLOW,
						  GDPWR_POWER_REQUEST_CLEAR, (void *)tst_ctx++);
	service_status = nrfs_gdpwr_power_request(GDPWR_POWER_DOMAIN_ACTIVE_FAST,
						  GDPWR_POWER_REQUEST_CLEAR, (void *)tst_ctx++);
	service_status = nrfs_gdpwr_power_request(GDPWR_POWER_DOMAIN_MAIN_SLOW,
						  GDPWR_POWER_REQUEST_CLEAR, (void *)tst_ctx);
}
#endif

int main(void)
{
#ifdef CONFIG_BOARD_NRF54H20DK
	nrfs_backend_wait_for_connection(K_FOREVER);
	clear_global_power_domains_requests();
#endif

	k_sleep(K_FOREVER);

	return 0;
}
