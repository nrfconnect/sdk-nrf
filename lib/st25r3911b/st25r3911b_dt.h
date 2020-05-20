/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef ST25R3911B_DT_H_
#define ST25R3911B_DT_H_

/*
 * Internal, shared header to access the the ST25R3911B devicetree node.
 */

#include <devicetree.h>

/*
 * Devicetree API node identifier for the ST25R3911B NFC reader.
 *
 * This is the first status "okay" node with compatible "st,st25r3911b",
 * as long as at least one such node exists in the devicetree.
 */
#define ST25R3911B_NODE		DT_INST(0, st_st25r3911b)

#endif /* ST25R3911B_DT_H_ */
