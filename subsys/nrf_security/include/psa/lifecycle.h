/*
 * Copyright (c) 2020, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __PSA_LIFECYCLE_H__
#define __PSA_LIFECYCLE_H__

#ifdef __cplusplus
extern "C" {
#endif

#define PSA_LIFECYCLE_PSA_STATE_MASK            (0xff00u)
#define PSA_LIFECYCLE_IMP_STATE_MASK            (0x00ffu)
#define PSA_LIFECYCLE_UNKNOWN                   (0x0000u)
#define PSA_LIFECYCLE_ASSEMBLY_AND_TEST         (0x1000u)
#define PSA_LIFECYCLE_PSA_ROT_PROVISIONING      (0x2000u)
#define PSA_LIFECYCLE_SECURED                   (0x3000u)
#define PSA_LIFECYCLE_NON_PSA_ROT_DEBUG         (0x4000u)
#define PSA_LIFECYCLE_RECOVERABLE_PSA_ROT_DEBUG (0x5000u)
#define PSA_LIFECYCLE_DECOMMISSIONED            (0x6000u)

/*
 * \brief This function retrieves the current PSA RoT lifecycle state.
 *
 * \return state                The current security lifecycle state of the PSA
 *                              RoT. The PSA state and implementation state are
 *                              encoded as follows:
 * \arg                           state[15:8] – PSA lifecycle state
 * \arg                           state[7:0] – IMPLEMENTATION DEFINED state
 */
uint32_t psa_rot_lifecycle_state(void);

#ifdef __cplusplus
}
#endif

#endif /* __PSA_LIFECYCLE_H__ */
