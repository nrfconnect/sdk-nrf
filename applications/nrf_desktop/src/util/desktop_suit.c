/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include "desktop_suit.h"
#include <sdfw/sdfw_services/suit_service.h>

int suit_get_installed_manifest_seq_num(unsigned int *seq_num)
{
	/* Manifest class ID
	 * nRF Desktop uses default SUIT manifest file (default namespace and name).
	 */
	const suit_manifest_class_id_t manifest_class_id = {{
		/* RFC4122 uuid5(uuid5(uuid.NAMESPACE_DNS, 'nordicsemi.com'),
		 *              'nRF54H20_sample_root')
		 */
		0x3f, 0x6a, 0x3a, 0x4d, 0xcd, 0xfa, 0x58, 0xc5,
		0xac, 0xce, 0xf9, 0xf5, 0x84, 0xc4, 0x11, 0x24
	}};

	int err = suit_get_installed_manifest_info((
				suit_manifest_class_id_t *)&manifest_class_id,
				seq_num, NULL, NULL, NULL, NULL);

	return err;
}
