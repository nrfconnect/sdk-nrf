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

/* %FEACONF feature IDs. */
enum feaconf_feat {
	FEACONF_FEAT_PROPRIETARY_PSM = 0,
	FEACONF_FEAT_PLMN_SELECTION_OPTIMIZATION = 3,
	FEACONF_FEAT_DISABLE_PAGING_NO_SERVICE = 4,
	FEACONF_FEAT_UICC_DEACTIVATION_NO_CELLS = 5,
};

/* Initialize the modem firmware type (read it from the modem). */
void mfw_type_init(void);

/* Get the modem firmware type. */
enum mfw_type mfw_type_get(void);

/* Enable or disable a modem feature using AT%FEACONF. */
int feaconf_write(enum feaconf_feat feature, bool enable);

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
