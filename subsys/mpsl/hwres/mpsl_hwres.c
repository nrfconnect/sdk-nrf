/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <mpsl_hwres_ppi.h>
#include <helpers/nrfx_gppi.h>

#if defined(DPPI_PRESENT) || defined(LUMOS_XXAA)
static bool mpsl_hwres_channel_alloc(uint32_t node_id, uint8_t *p_ch)
{
	int ch = nrfx_gppi_channel_alloc(node_id);

	if (ch < 0) {
		return false;
	}
	*p_ch = (uint8_t)ch;
	return true;
}
#endif

#if defined(DPPI_PRESENT)

bool mpsl_hwres_dppi_channel_alloc(NRF_DPPIC_Type *p_dppic, uint8_t *p_dppi_ch)
{
	return mpsl_hwres_channel_alloc(nrfx_gppi_domain_id_get((uint32_t)p_dppic), p_dppi_ch);
}

#endif /* DPPI_PRESENT */

#if defined(PPIB_PRESENT)

#if defined(LUMOS_XXAA)
#include <soc/interconnect/nrfx_gppi_lumos.h>
static uint32_t ppib_get_domain(NRF_PPIB_Type *p_ppib)
{
	switch ((uint32_t)p_ppib) {
	case (uint32_t)NRF_PPIB00:
		/* fall through */
	case (uint32_t)NRF_PPIB10:
		return NRFX_GPPI_NODE_PPIB00_10;
	case (uint32_t)NRF_PPIB11:
		/* fall through */
	case (uint32_t)NRF_PPIB21:
		return NRFX_GPPI_NODE_PPIB11_21;
	case (uint32_t)NRF_PPIB01:
		/* fall through */
	case (uint32_t)NRF_PPIB20:
		return NRFX_GPPI_NODE_PPIB01_20;
	case (uint32_t)NRF_PPIB22:
		/* fall through */
	case (uint32_t)NRF_PPIB30:
		return NRFX_GPPI_NODE_PPIB22_30;
	default:
		__ASSERT_NO_MSG("Unexpected PPIB");
		return 0;
	}
}
#endif

bool mpsl_hwres_ppib_channel_alloc(NRF_PPIB_Type *p_ppib, uint8_t *p_ppib_ch)
{
#if defined(LUMOS_XXAA)
	return mpsl_hwres_channel_alloc(ppib_get_domain(p_ppib), p_ppib_ch);
#else
	(void)p_ppib;
	(void)p_ppib_ch;
	return false;
#endif
}

#endif /* PPIB_PRESENT */
