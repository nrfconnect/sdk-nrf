/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NPM6001_TYPES_H
#define NPM6001_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "compiler_abstraction.h"

#ifndef __I
#ifdef __cplusplus
#define __I volatile
#else
#define __I volatile const
#endif
#endif
#ifndef __O
#define __O volatile
#endif
#ifndef __IO
#define __IO volatile
#endif

#ifndef __IM
#define __IM volatile const
#endif
#ifndef __OM
#define __OM volatile
#endif
#ifndef __IOM
#define __IOM volatile
#endif

#include "compiler_abstraction.h"

#if defined(__CC_ARM)
#pragma anon_unions
#elif defined(__ICCARM__)
#pragma language = extended
#elif defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc11-extensions"
#pragma clang diagnostic ignored "-Wreserved-id-macro"
#pragma clang diagnostic ignored "-Wgnu-anonymous-struct"
#pragma clang diagnostic ignored "-Wnested-anon-types"
#elif defined(__GNUC__)

#elif defined(__TMS470__)

#elif defined(__TASKING__)
#pragma warning 586
#elif defined(__CSMC__)

#else
#warning Unsupported compiler type
#endif

#if defined(__CC_ARM)
#pragma pop
#elif defined(__ICCARM__)

#elif defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
#pragma clang diagnostic pop
#elif defined(__GNUC__)

#elif defined(__TMS470__)

#elif defined(__TASKING__)
#pragma warning restore
#elif defined(__CSMC__)

#endif

#ifdef __cplusplus
}
#endif
#endif
