/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <zephyr.h>

#include "bsd.h"
#include "lte_lc.h"
#include "nrf_socket.h"
#include "dfu_client.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(app);


static int app_dfu_client_event_handler(struct dfu_client_object *const dfu, enum dfu_client_evt event,
				  u32_t status);


static struct dfu_client_object dfu = {
	.host = "s3.amazonaws.com",
	.resource = "/nordic-firmware-files/f81197c5-0353-4ac2-a961-3b2ce867329d",
	.callback = app_dfu_client_event_handler
};


/**@brief Recoverable BSD library error. */
void bsd_recoverable_error_handler(uint32_t err)
{
	LOG_ERR("bsd_recoverable_error_handler = %lu\n", err);

	// Read the fault register for details.
	volatile uint32_t fault_reason = *((volatile uint32_t *)0x4002A614);
	__ASSERT(false, "bsd_recoverable_error_handler, reason %lu, reset application\n", fault_reason);
}

/**@brief Irrecoverable BSD library error. */
void bsd_irrecoverable_error_handler(uint32_t err)
{
	LOG_ERR("bsd_irrecoverable_error_handler = %lu\n", err);

	// Read the fault register for details.
	volatile uint32_t fault_reason = *((volatile uint32_t *)0x4002A614);
	__ASSERT(false, "bsd_recoverable_error_handler, reason %lu, reset application\n", fault_reason);
}


/**@brief Initialize modem DFU. */
static void app_dfu_init(void)
{
	int retval = dfu_client_init(&dfu);
   __ASSERT(retval == 0, "dfu_init() failed, err %d", retval);
}


/**@brief Start transfer of the new firmware patch. */
static void app_dfu_transfer_start(void)
{

	int retval = dfu_client_connect(&dfu);
	__ASSERT(retval == 0, "dfu_client_connect() failed, err %d", retval);

	retval = dfu_client_download(&dfu);
	__ASSERT(retval != -1, "dfu_client_download() failed, err %d", retval);
}

static int app_dfu_client_event_handler(struct dfu_client_object *const dfu,
				  enum dfu_client_evt event,
				  u32_t error)
{
	int retval;

	switch (event) {
	case DFU_CLIENT_EVT_DOWNLOAD_INIT: {
		LOG_INF("Download started");
		break;
	}
	case DFU_CLIENT_EVT_DOWNLOAD_FRAG: {
		LOG_INF("Download fragment, size %d", dfu->fragment_size );
		break;
	}
	case DFU_CLIENT_EVT_DOWNLOAD_DONE: {
		LOG_INF("Download completed");
		dfu_client_disconnect(dfu);
		break;
	}
	case DFU_CLIENT_EVT_ERROR: {
		LOG_ERR("DFU error");
		dfu_client_abort(dfu);
		dfu_client_disconnect(dfu);
		__ASSERT(false, "Something went wrong, please restart the application\n");
		break;
	}
	default:
		break;
	}

	return 0;
}



int main(void)
{
	app_dfu_init();

	app_dfu_transfer_start();

	while (true) {
        if(dfu.status == DFU_CLIENT_STATUS_DOWNLOAD_COMPLETE) {
            LOG_INF("Download complete. Nothing more to do.");
            break;
        }
        k_sleep(1000);
		dfu_client_process(&dfu);
	}

	return 0;
}

