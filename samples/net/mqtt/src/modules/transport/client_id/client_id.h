/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/types.h>

/** @brief Get Client ID name. Either retrieved from the HW ID library or
 *         CONFIG_MQTT_SAMPLE_TRANSPORT_CLIENT_ID if set.
 *         For Native Sim builds a random uint32 bit value is returned.
 *
 *  @param buffer Pointer to buffer that the Client ID will be written to.
 *  @param buffer_size Size of buffer.
 *
 *  @return 0 If successful. Otherwise, a negative error code is returned.
 *  @retval -EMSGSIZE if the passed in buffer is too small.
 *  @retval -EINVAL   If the passed in pointer is NULL or buffer is too small.
 */
int client_id_get(char *const buffer, size_t buffer_size);
