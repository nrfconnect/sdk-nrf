/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __SECURE_STORAGE_INIT_H_
#define __SECURE_STORAGE_INIT_H_

/* In case a HUK ist used for the secure_storage key */
#ifdef CONFIG_SECURE_STORAGE_BACKEND_AEAD_KEY_DERIVE_FROM_HUK
int write_huk(void);
#endif /* CONFIG_SECURE_STORAGE_BACKEND_AEAD_KEY_DERIVE_FROM_HUK */

#endif /* __SECURE_STORAGE_INIT_H_ */
