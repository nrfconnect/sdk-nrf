/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef HELPERS_H__
#define HELPERS_H__

#include <string.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <modem/at_parser.h>

#ifdef __cplusplus
extern "C" {
#endif

enum mfw_type {
	MFW_TYPE_UNKNOWN,
	MFW_TYPE_NRF9160,
	MFW_TYPE_NRF91X1,
	MFW_TYPE_NRF9151_NTN
};

/* Initialize the modem firmware type (read it from the modem). */
void mfw_type_init(void);

/* Get the modem firmware type. */
enum mfw_type mfw_type_get(void);

/* Converts integer as string to integer type. */
int string_to_int(const char *str_buf, int base, int *output);

/* Converts integer on string format to integer type. */
int string_param_to_int(struct at_parser *parser, size_t idx, int *output, int base);

/* Converts PLMN string to integer type MCC and MNC. */
int plmn_param_string_to_mcc_mnc(struct at_parser *parser, size_t idx, int *mcc, int *mnc);

#ifdef __cplusplus
}
#endif

#endif /* HELPERS_H__ */
