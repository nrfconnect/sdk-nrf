/*
 * Copyright (c) 2016 - 2023 Nordic Semiconductor ASA
 * Copyright (c) since 2020 Oberon microsystems AG
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "oberon_helpers.h"


int oberon_ct_compare(const void *x, const void *y, size_t length)
{
    const uint8_t *px = (const uint8_t*)x;
    const uint8_t *py = (const uint8_t*)y;
    int diff = 0;
    do {
        diff |= (int)(*px++ ^ *py++);
    } while (--length);
    return diff;
}
    
int oberon_ct_compare_zero(const void *x, size_t length)
{
    const uint8_t *px = (const uint8_t*)x;
    int diff = 0;
    do {
        diff |= (int)(*px++);
    } while (--length);
    return diff;
}

void oberon_xor(void *r, const void *x, const void *y, size_t length)
{
    const uint8_t *px = (const uint8_t*)x;
    const uint8_t *py = (const uint8_t*)y;
    uint8_t *pr = (uint8_t*)r;
    do {
        *pr++ = (uint8_t)(*px++ ^ *py++);
    } while (--length);
}
