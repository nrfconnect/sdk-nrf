/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_CLOUD_FOTA_POLL_H_
#define NRF_CLOUD_FOTA_POLL_H_

/** @file nrf_cloud_fota_poll.h
 * @brief Module to provide nRF Cloud FOTA assistance for applications
 *        connecting to nRF Cloud using REST or CoAP.
 */

#include <net/nrf_cloud_rest.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup nrf_cloud_fota_poll nRF Cloud FOTA poll
 * @{
 */

enum nrf_cloud_fota_reboot_status {
	FOTA_REBOOT_REQUIRED,	/** Reboot to install update. */
	FOTA_REBOOT_SUCCESS,	/** Reboot to complete FOTA job. */
	FOTA_REBOOT_FAIL,	/** Reboot because FOTA job failed. */
	FOTA_REBOOT_SYS_ERROR	/** Reboot because fatal system error occurred. */
};

/**
 * @brief  Event handler registered with the module to handle asynchronous
 * events from the module.
 *
 * @param[in]  status The reason for the reboot request.
 */
typedef void (*fota_reboot_handler_t)(enum nrf_cloud_fota_reboot_status status);

struct nrf_cloud_fota_poll_ctx {
	struct nrf_cloud_rest_context *rest_ctx;
	const char *device_id;
	fota_reboot_handler_t reboot_fn;
	bool full_modem_fota_supported;
};

/**
 * @brief Initialize nRF Cloud FOTA polling assistance.
 *        Must be called before any other module functions.
 *
 * @param[in] ctx Pointer to context used for FOTA polling operations.
 *
 * @retval 0       If successful.
 * @retval -EINVAL Invalid ctx.
 * @return         A negative value indicates an error.
 */
int nrf_cloud_fota_poll_init(struct nrf_cloud_fota_poll_ctx *ctx);

/**
 * @brief Process/validate a pending FOTA update job.
 *        This may initiate a reboot through the context's reboot function.
 *
 * @param[in] ctx Pointer to context used for FOTA polling operations.
 *
 * @retval -EINVAL Invalid ctx or module is not initialized.
 * @return         If successful, the pending FOTA job type is returned @ref nrf_cloud_fota_type.
 *                 NRF_CLOUD_FOTA_TYPE__INVALID indicates no pending FOTA job.
 */
int nrf_cloud_fota_poll_process_pending(struct nrf_cloud_fota_poll_ctx *ctx);

/**
 * @brief Perform the following FOTA tasks:
 *        Report the status of an in progress FOTA job.
 *        Check for a queued FOTA job.
 *        Execute FOTA job.
 *        Save status and request reboot.
 *
 * @param[in] ctx Pointer to context used for FOTA polling operations.
 *
 * @retval -EINVAL          Invalid ctx or module is not initialized.
 * @retval -ENOTRECOVERABLE Error performing FOTA action.
 * @retval -EBUSY           A reboot was requested but not performed by the application.
 * @retval -EFAULT          A FOTA job was not successful.
 * @retval -ENOENT          A FOTA job has finished and its status has been reported to the cloud.
 * @retval -EAGAIN          No FOTA job exists.
 */
int nrf_cloud_fota_poll_process(struct nrf_cloud_fota_poll_ctx *ctx);

/** @} */

#ifdef __cplusplus
#endif

#endif /* NRF_CLOUD_FOTA_POLL_H_ */
