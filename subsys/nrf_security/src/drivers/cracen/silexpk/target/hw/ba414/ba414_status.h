/**
 * \brief BA414ep functions to translate status codes
 * \file
 */
/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BA414_STATUS_HEADERFILE
#define BA414_STATUS_HEADERFILE

#include <stdint.h>

/** Translate BA414ep status bits into a Silexpk status code. */
int convert_ba414_status(int code);

struct sx_pk_capabilities;

/** Translate BA414ep "PK_HwConfigReg" value into capabitities struct */
void convert_ba414_capabilities(uint32_t ba414epfeatures, struct sx_pk_capabilities *caps);

#endif
