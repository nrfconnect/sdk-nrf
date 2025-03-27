/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <suit_ipuc.h>
#include <suit_metadata.h>
#include <suit_plat_decode_util.h>
#include <suit_components_state.h>

void suit_components_state_report(void)
{
	suit_plat_err_t err = SUIT_PLAT_SUCCESS;
	size_t count = 0;

	err = suit_ipuc_get_count(&count);
	if (err == SUIT_PLAT_SUCCESS) {

		printk("In-place updateable components:\n");
		for (size_t idx = 0; idx < count; idx++) {
			struct zcbor_string component_id;
			uint8_t component_id_data[32];
			suit_manifest_role_t role = SUIT_MANIFEST_UNKNOWN;

			component_id.value = component_id_data;
			component_id.len = sizeof(component_id_data);
			err = suit_ipuc_get_info(idx, &component_id, &role);
			if (err == SUIT_PLAT_SUCCESS) {
				suit_component_type_t component_type =
					SUIT_COMPONENT_TYPE_UNSUPPORTED;

				suit_plat_decode_component_type(&component_id, &component_type);
				if (component_type == SUIT_COMPONENT_TYPE_MEM) {
					uint8_t core_id = 0;
					intptr_t run_address = 0;
					size_t slot_size = 0;

					suit_plat_decode_component_id(&component_id, &core_id,
								      &run_address, &slot_size);

					printk("MEM, 0x%08X, %d bytes, core: %d, mfst role: "
					       "0x%02X\n",
					       (unsigned int)run_address, slot_size, core_id, role);

				} else {
					printk("component_type: %d, mfst role: 0x%02X\n",
					       component_type, role);
				}
			}
		}
	}
}
