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
 * @return Number of bytes written to p_buf is successful. Otherwise negative
 *         error code.
 */
int public_key_data_read(u32_t key_idx, u32_t *p_buf, size_t buf_size);

#ifdef __cplusplus
}
#endif

#endif /* PROVISION_H_ */
