/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ESB_WORKAROUNDS_H__
#define ESB_WORKAROUNDS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include <nrfx.h>

/* nRF52 Workaround 143
 * esb_apply_nrf52_143(uint32_t base_address_mask)
 */

/* nRF52 Workaround 182
 * esb_apply_nrf52_182(void)
 */

/* nRF54H Workaround 84
 * esb_apply_nrf54h_84(void)
 * esb_revert_nrf54h_84(void)
 *
 * Not applied automatically. Enable CONFIG_ESB_CLOCK_INIT to have esb_glue.c apply and
 * revert this workaround automatically.
 */

/* nRF54H Workaround 103
 * esb_apply_nrf54h_103(void)
 */

/* nRF54H Workaround 216
 * esb_apply_nrf54h_216(bool on)
 * esb_is_nrf54h_216_enabled(void)
 */

/* nRF54H Workaround 229
 * esb_apply_nrf54h_229(void)
 */

/* nRF54L Workaround 20
 * esb_apply_nrf54l_20(void)
 * esb_revert_nrf54l_20(void)
 *
 * Not applied automatically. Enable CONFIG_ESB_CLOCK_INIT to have esb_glue.c apply and
 * revert this workaround automatically.
 */

/* nRF54L Workaround 39
 * esb_apply_nrf54l_39(void)
 * esb_revert_nrf54l_39(void)
 *
 * Not applied automatically. Enable CONFIG_ESB_CLOCK_INIT to have esb_glue.c apply and
 * revert this workaround automatically.
 */

#if NRF_ERRATA_STATIC_CHECK(52, 143)
void esb_apply_nrf52_143(uint32_t base_address_mask);
#else
static inline void esb_apply_nrf52_143(uint32_t base_address_mask)
{
	ARG_UNUSED(base_address_mask);
}
#endif /* NRF_ERRATA_STATIC_CHECK(52, 143) */

#if NRF_ERRATA_STATIC_CHECK(52, 182)
void esb_apply_nrf52_182(void);
#else
static inline void esb_apply_nrf52_182(void) {}
#endif /* NRF_ERRATA_STATIC_CHECK(52, 182) */

#if NRF_ERRATA_STATIC_CHECK(54H, 103)
void esb_apply_nrf54h_103(void);
#else
static inline void esb_apply_nrf54h_103(void) {}
#endif /* NRF_ERRATA_STATIC_CHECK(54H, 103) */

#if NRF_ERRATA_STATIC_CHECK(54H, 216)
void esb_apply_nrf54h_216(bool on);
bool esb_is_nrf54h_216_enabled(void);
#else
static inline void esb_apply_nrf54h_216(bool on)
{
	ARG_UNUSED(on);
}

static inline bool esb_is_nrf54h_216_enabled(void)
{
	return false;
}
#endif /* NRF_ERRATA_STATIC_CHECK(54H, 216)*/

#if NRF_ERRATA_STATIC_CHECK(54H, 84)
void esb_apply_nrf54h_84(void);
void esb_revert_nrf54h_84(void);
#else
static inline void esb_apply_nrf54h_84(void) {}
static inline void esb_revert_nrf54h_84(void) {}
#endif /* NRF_ERRATA_STATIC_CHECK(54H, 84) */

#if NRF_ERRATA_STATIC_CHECK(54H, 229)
void esb_apply_nrf54h_229(void);
#else
static inline void esb_apply_nrf54h_229(void) {}
#endif /* NRF_ERRATA_STATIC_CHECK(54H, 229) */

#if NRF_ERRATA_STATIC_CHECK(54L, 20) || defined(NRF54LS05B_XXAA)
void esb_apply_nrf54l_20(void);
void esb_revert_nrf54l_20(void);
#else
static inline void esb_apply_nrf54l_20(void) {}
static inline void esb_revert_nrf54l_20(void) {}
#endif /* NRF_ERRATA_STATIC_CHECK(54L, 20) || defined(NRF54LS05B_XXAA) */

#if NRF_ERRATA_STATIC_CHECK(54L, 39) || defined(NRF54LS05B_XXAA)
void esb_apply_nrf54l_39(void);
void esb_revert_nrf54l_39(void);
#else
static inline void esb_apply_nrf54l_39(void) {}
static inline void esb_revert_nrf54l_39(void) {}
#endif /* NRF_ERRATA_STATIC_CHECK(54L, 39) || defined(NRF54LS05B_XXAA) */

#ifdef __cplusplus
}
#endif

#endif /* ESB_WORKAROUNDS_H__ */
