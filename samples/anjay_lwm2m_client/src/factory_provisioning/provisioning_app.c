/*
 * Copyright 2020-2023 AVSystem <avsystem@avsystem.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/shell/shell_uart.h>

#include <anjay/anjay.h>
#include <anjay/access_control.h>
#include <anjay/factory_provisioning.h>
#include <anjay/security.h>
#include <anjay/server.h>

#include <anjay_zephyr/config.h>
#include <anjay_zephyr/factory_provisioning.h>

#include "factory_flash.h"

LOG_MODULE_REGISTER(provisioning_app);

static anjay_t *initialize_anjay(void)
{
	anjay_t *anjay = anjay_new(&(const anjay_configuration_t){
		.endpoint_name = anjay_zephyr_config_default_ep_name() });

	if (!anjay) {
		LOG_ERR("Could not create Anjay object");
		return NULL;
	}

	if (anjay_security_object_install(anjay) || anjay_server_object_install(anjay) ||
	    anjay_access_control_install(anjay)) {
		anjay_delete(anjay);
		LOG_ERR("Failed to install necessary modules");
		return NULL;
	}

	return anjay;
}

static void factory_provision(void)
{
	anjay_t *anjay = initialize_anjay();

	if (!anjay) {
		LOG_ERR("Couldn't initialize Anjay. Rebooting.");
		abort();
	}

	if (anjay_zephyr_is_factory_provisioning_info_present()) {
		LOG_INF("Factory provisioning information already present. "
			"Please flash production firmware. Halting.");
	} else {
		LOG_WRN("NOTE: No more log messages will be displayed. Please use "
			"mcumgr to check provisioning results");

		LOG_INF("Device ready for provisioning.");

		z_shell_log_backend_disable(shell_backend_uart_get_ptr()->log_backend);

		avs_stream_t *stream = factory_flash_input_stream_init();

		assert(stream);

		avs_error_t err = anjay_factory_provision(anjay, stream);
		// NOTE: Not calling avs_stream_cleanup() because stream is *NOT* heap-allocated

		if (avs_is_ok(err) && anjay_zephyr_persist_factory_provisioning_info(anjay)) {
			err = avs_errno(AVS_EIO);
		}
		factory_flash_finished(avs_is_ok(err) ? 0 : -1);
	}

	while (true) {
		k_sleep(K_SECONDS(1));
	}
}

void main(void)
{
	LOG_PANIC();

	if (anjay_zephyr_persistence_init()) {
		LOG_ERR("Can't initialize persistence");
	}

	factory_provision();
}
