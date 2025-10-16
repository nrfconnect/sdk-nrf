/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <mpsl_dppi.h>
#if defined(DPPI_PRESENT)
#include <nrfx_dppi.h>
#endif
#if defined(LUMOS_XXAA)
#include <nrfx_ppib.h>
#endif

#if defined(DPPI_PRESENT)

bool mpsl_hwres_dppi_channel_alloc(NRF_DPPIC_Type *p_dppic, uint8_t *p_dppi_ch)
{
	nrfx_dppi_t dppi = {0};
	nrfx_err_t err = nrfx_dppi_periph_get((uintptr_t)p_dppic, &dppi);

	if (err != NRFX_SUCCESS) {
		return false;
	}

	return (nrfx_dppi_channel_alloc(&dppi, p_dppi_ch) == NRFX_SUCCESS);
}

#endif /* DPPI_PRESENT */

#if defined(PPIB_PRESENT)

#if defined(LUMOS_XXAA)
static const nrfx_ppib_interconnect_t *nrfx_ppib_interconnect_find_by_ptr(NRF_PPIB_Type *p_ppib)
{
	static const nrfx_ppib_interconnect_t interconnects[] = {
#if NRFX_CHECK(NRFX_PPIB00_ENABLED) && NRFX_CHECK(NRFX_PPIB10_ENABLED)
		NRFX_PPIB_INTERCONNECT_INSTANCE(00, 10),
#endif
#if NRFX_CHECK(NRFX_PPIB11_ENABLED) && NRFX_CHECK(NRFX_PPIB21_ENABLED)
		NRFX_PPIB_INTERCONNECT_INSTANCE(11, 21),
#endif
#if NRFX_CHECK(NRFX_PPIB22_ENABLED) && NRFX_CHECK(NRFX_PPIB30_ENABLED)
		NRFX_PPIB_INTERCONNECT_INSTANCE(22, 30),
#endif
#if NRFX_CHECK(NRFX_PPIB01_ENABLED) && NRFX_CHECK(NRFX_PPIB20_ENABLED)
		NRFX_PPIB_INTERCONNECT_INSTANCE(01, 20),
#endif
	};

	for (size_t i = 0U; i < NRFX_ARRAY_SIZE(interconnects); ++i) {
		const nrfx_ppib_interconnect_t *ith = &interconnects[i];

		if ((ith->left.p_reg == p_ppib) || (ith->right.p_reg == p_ppib)) {
			return ith;
		}
	}

	return NULL;
}
#endif

bool mpsl_hwres_ppib_channel_alloc(NRF_PPIB_Type *p_ppib, uint8_t *p_ppib_ch)
{
#if defined(LUMOS_XXAA)
	const nrfx_ppib_interconnect_t *ppib_interconnect =
		nrfx_ppib_interconnect_find_by_ptr(p_ppib);

	if (ppib_interconnect == NULL) {
		return false;
	}

	return (nrfx_ppib_channel_alloc(ppib_interconnect, p_ppib_ch) == NRFX_SUCCESS);
#else
	(void)p_ppib;
	(void)p_ppib_ch;
	return false;
#endif
}

#endif /* PPIB_PRESENT */
