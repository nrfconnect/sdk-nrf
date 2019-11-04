/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef PROVISION_H_
#define PROVISION_H_

/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/types.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Function for reading address of slot 0.
 *
 * @return Address of slot 0.
 */
u32_t s0_address_read(void);

/**
 * @brief Function for reading address of slot 1.
 *
 * @return Address of slot 1.
 */
u32_t s1_address_read(void);

/**
 * @brief Function for reading number of public key data slots.
 *
 * @return Number of public key data slots.
 */
u32_t num_public_keys_read(void);

/**
 * @brief Function for reading public key data.
 *
 * @param[in]  key_idx  Index of key.
 * @param[out] p_buf    Pointer to area where key data will be stored.
 * @param[in]  buf_size Size of buffer, in bytes, provided in p_buf.
 *
 * @return Number of bytes written to p_buf is successful.
 * @retval -EINVAL  Key has been invalidated.
 * @retval -ENOMEM  The provided buffer is too small.
 * @retval -EFAULT  key_idx is too large. There is no key with that index.
 */
int public_key_data_read(u32_t key_idx, u8_t *p_buf, size_t buf_size);


/**
 * @brief Function for invalidating a public key.
 *
 * The public key will no longer be returned by @ref public_key_data_read.
 *
 * @param[in]  key_idx  Index of key.
 */
void invalidate_public_key(u32_t key_idx);

#ifdef __cplusplus
}
#endif

#endif /* PROVISION_H_ */
