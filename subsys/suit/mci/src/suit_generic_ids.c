/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <suit_mci.h>

/* RFC4122 uuid5(uuid.NAMESPACE_DNS, 'nordicsemi.com')
 */
static const suit_uuid_t nordic_vendor_id = {{0x76, 0x17, 0xda, 0xa5, 0x71, 0xfd, 0x5a, 0x85, 0x8f,
					      0x94, 0xe2, 0x8d, 0x73, 0x5c, 0xe9, 0xf4}};

mci_err_t suit_mci_nordic_vendor_id_get(const suit_uuid_t **vendor_id)
{
	if (NULL == vendor_id) {
		return SUIT_PLAT_ERR_INVAL;
	}
	*vendor_id = &nordic_vendor_id;
	return SUIT_PLAT_SUCCESS;
}
