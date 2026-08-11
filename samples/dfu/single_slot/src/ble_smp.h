/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SINGLE_SLOT_BLE_SMP_H_
#define SINGLE_SLOT_BLE_SMP_H_

#if defined(CONFIG_MCUMGR_TRANSPORT_BT)

/** @brief Enable Bluetooth and start advertising the SMP service.
 *
 * @return 0 on success, negative error code otherwise.
 */
int ble_smp_init(void);

#else

static inline int ble_smp_init(void)
{
	return 0;
}

#endif /* CONFIG_MCUMGR_TRANSPORT_BT */

#endif /* SINGLE_SLOT_BLE_SMP_H_ */
