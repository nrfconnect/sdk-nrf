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
 * @brief  Reboot event handler registered with the module to handle asynchronous
 * reboot events from the module.
 *
 * @param[in]  status The reason for the reboot request.
 */
typedef void (*fota_reboot_handler_t)(enum nrf_cloud_fota_reboot_status status);

/**
 * @brief  Status event handler registered with the module to handle asynchronous
 * status events from the module.
 *
 * @param[in]  status Status event.
 * @param[in]  status_details Details about the event, NULL if no details are provided.
 */
typedef void (*nrf_cloud_fota_poll_handler_t)(enum nrf_cloud_fota_status status,
					      const char *const status_details);

struct nrf_cloud_fota_poll_ctx {
	/* Internal variables */
	struct nrf_cloud_rest_context *rest_ctx;
	struct k_work_delayable timeout_work;
	struct k_work_delayable cancel_work;
	bool is_nonblocking;
	bool full_modem_fota_supported;
	const char *device_id;
	enum dfu_target_image_type img_type;

	/* Public variables */

	/** HTTP fragment size. Sets the size of the requested payload in each HTTP request when
	 *  downloading the firmware image. Note that the header size is not included in this size.
	 *  It is important that the total size of each fragment (HTTP headers + payload) fits in
	 *  the buffers of the underlying network stack layers. Failing to do so, may result in
	 *  failure to download new firmware images.
	 *
	 *  If this value is set to 0, the Kconfig option
	 *  :kconfig:option:`CONFIG_NRF_CLOUD_FOTA_DOWNLOAD_FRAGMENT_SIZE` will be used.
	 */
	uint32_t fragment_size;

	/** User-provided callback function to handle reboots */
	fota_reboot_handler_t reboot_fn;

	/** Optional, user-provided callback function to handle FOTA status events.
	 *  If the function is provided, @ref nrf_cloud_fota_poll_process will be non-blocking and
	 *  the user will receive status events asynchronously.
	 *
	 * If the function is not provided, @ref nrf_cloud_fota_poll_process will be blocking.
	 */
	nrf_cloud_fota_poll_handler_t status_fn;

	/** Callback of type @ref dfu_target_reset_cb_t for resetting the SMP device to enter
	 * MCUboot recovery mode.
	 * Used if @kconfig{CONFIG_NRF_CLOUD_FOTA_SMP} is enabled.
	 */
	void *smp_reset_cb;
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
 *	  The function will be blocking during an update image download if the error_fn callback is
 *	  not provided.
 *	  If the error_fn callback is provided, the function will be non-blocking and the user
 *	  will receive error events asynchronously.
 *
 * @param[in] ctx Pointer to context used for FOTA polling operations.
 *
 * @retval 0		    Only applicable if in non-blocking mode. Indicates successful start of
 *			    FOTA processing.
 * @retval -EINVAL          Invalid ctx or module is not initialized.
 * @retval -ENOTRECOVERABLE Error performing FOTA action.
 * @retval -EBUSY           A reboot was requested but not performed by the application.
 * @retval -EFAULT          A FOTA job was not successful.
 * @retval -ENOENT          A FOTA job has finished and its status has been reported to the cloud.
 * @retval -EAGAIN          No FOTA job exists.
 * @retval -ETIMEDOUT       The FOTA job check timed out. Retry later.
 * @retval -ENETDOWN        The network is down.
 */
int nrf_cloud_fota_poll_process(struct nrf_cloud_fota_poll_ctx *ctx);

/**
 * @brief Apply downloaded image. For full modem FOTA this must be called after the network has
 *	  been disconnected. Only applicable in non-blocking mode.
 *
 * @param[in] ctx Pointer to context used for FOTA polling operations.
 *
 * @return 0 on success, negative value on failure.
 */
int nrf_cloud_fota_poll_update_apply(struct nrf_cloud_fota_poll_ctx *ctx);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* NRF_CLOUD_FOTA_POLL_H_ */
