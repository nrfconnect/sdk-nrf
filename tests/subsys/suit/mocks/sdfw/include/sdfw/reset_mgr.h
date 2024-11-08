/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


#ifndef RESET_MGR_H__
#define RESET_MGR_H__

#include <nrfx.h>
#include <stddef.h>

/**
 * @brief Synchronously reset nonessential domains and the resources associated with them.
 *
 * @note This API can not be used from the system workqueue thread.
 *
 * @retval 0 if successful
 * @retval -EDEADLK if called from the system workqueue thread.
 * @retval -EBUSY if an asynchronous reset is ongoing
 * @return other negative errno on failure.
 */
int reset_mgr_reset_domains_sync(void);

/**
 * @brief Boot a processor from a reset state using new VTORs.
 *
 * @note The VTORs parameters are cached and reused for subsequent resets.
 *
 * @param[in] processor Target processor
 * @param[in] svtor Initial secure VTOR.
 * @param[in] nsvtor Initial non-secure VTOR.
 *
 * @return 0 on success, or non-zero error codes.
 */
int reset_mgr_init_and_boot_processor(nrf_processor_t processor, uintptr_t svtor, uintptr_t nsvtor);

#endif /* RESET_MGR_H__ */
