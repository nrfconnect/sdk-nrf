/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __TRUSTED_STORAGE_INIT_H_
#define __TRUSTED_STORAGE_INIT_H_

/* In case a HUK ist used for the trusted_storage key */
#ifdef CONFIG_TRUSTED_STORAGE_BACKEND_AEAD_KEY_DERIVE_FROM_HUK
int write_huk(void);
#endif /* CONFIG_TRUSTED_STORAGE_BACKEND_AEAD_KEY_DERIVE_FROM_HUK */

#endif /* __TRUSTED_STORAGE_INIT_H_ */
