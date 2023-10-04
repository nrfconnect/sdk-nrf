/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef COMMON_H_
#define COMMON_H_


#ifdef __cplusplus
extern "C" {
#endif

#define CNT_POS      (0UL)                  /* Position of Counter field. */
#define CNT_MSK      (0xFFFF << CNT_POS)    /* Bit mask of Counter field. */
#define FLAGS_POS    (16UL)                 /* Position of Flags field. */
#define FLAGS_MSK    (0xFFFF << FLAGS_POS)  /* Bit mask of Flags field. */
#define FLAGS_RESET  BIT(0)                 /* Reset bit. */
#define CNT_INIT_VAL (0x0055)               /* Initialization value for counter. */

#define IPC_MEM_CNT_IDX  0      /** Index of the memory cell that stores the counter. */
#define IPC_MEM_REAS_IDX 1      /** Index of the memory cell that stores the reset reason. */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* COMMON_H_ */
