/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/entropy.h>
#include <mbedtls/entropy.h>
#include <entropy_poll.h>

int mbedtls_hardware_poll(void *data,
                          unsigned char *output,
                          size_t len,
                          size_t *olen )
{
    const struct device *dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_entropy));
    size_t chunk_size;

    (void)data;

    if (output == NULL)
    {
        return -1;
    }

    if (olen == NULL)
    {
        return -1;
    }

    if (len == 0)
    {
        return -1;
    }

    if (!device_is_ready(dev))
    {
        return MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
    }

    while (len > 0)
    {
        chunk_size = MIN(MBEDTLS_ENTROPY_MAX_GATHER, len);

        if (entropy_get_entropy(dev, output, chunk_size) < 0)
        {
            return MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
        }

        *olen += chunk_size;
        output += chunk_size;
        len -= chunk_size;
    }

    return 0;
}
