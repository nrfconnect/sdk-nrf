/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "nrfx_twim.h"
#include <zephyr/sys/__assert.h>

nrfx_err_t nrfx_twim_init(nrfx_twim_t const *p_instance,
			nrfx_twim_config_t const *p_config,
			nrfx_twim_evt_handler_t event_handler,
			void *p_context)
{
	ARG_UNUSED(p_instance);
	ARG_UNUSED(p_config);
	ARG_UNUSED(event_handler);
	ARG_UNUSED(p_context);

	__ASSERT_NO_MSG(false);

	return NRFX_ERROR_NOT_SUPPORTED;
}

void nrfx_twim_enable(nrfx_twim_t const *p_instance)
{
	ARG_UNUSED(p_instance);

	__ASSERT_NO_MSG(false);
}

void nrfx_twim_disable(nrfx_twim_t const *p_instance)
{
	ARG_UNUSED(p_instance);

	__ASSERT_NO_MSG(false);
}

nrfx_err_t nrfx_twim_xfer(nrfx_twim_t const *p_instance,
			nrfx_twim_xfer_desc_t const *p_xfer_desc,
			uint32_t flags)
{
	ARG_UNUSED(p_instance);
	ARG_UNUSED(p_xfer_desc);
	ARG_UNUSED(flags);

	__ASSERT_NO_MSG(false);

	return NRFX_ERROR_NOT_SUPPORTED;
}
