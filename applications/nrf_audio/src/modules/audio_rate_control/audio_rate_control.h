/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @defgroup audio_rate_control Audio rate control API
 * @{
 * @brief Audio rate control API for nRF Audio applications.
 *
 * This Audio Rate Control provides generic functions to manage the
 * audio synchronisation. On the nRF5340 SoC, the audio synchronisation
 * is typically controlled with the Analog Phase-Locked Loop (APLL).
 * Although where this is not available alternative methods can be
 * configured through the callback functions.
 */

#ifndef _AUDIO_RATE_CONTROL_H_
#define _AUDIO_RATE_CONTROL_H_

/**
 * @brief The state of the audio rate control.
 */
enum audio_rate_control_state {
	AUDIO_RATE_CONTROL_STATE_UNINITIALIZED = 0,
	AUDIO_RATE_CONTROL_STATE_INITIALIZED,
};

/**
 * @brief Audio rate control's opaque implementation context structure.
 */
struct audio_rate_control_implementation_ctx;

/**
 * @brief Audio rate control's opaque configuration structure.
 */
struct audio_rate_control_configuration;

/**
 * @brief Audio rate control's callback structure.
 */
struct audio_rate_control_ops {
	/**
	 * @brief Initialize a rate control module implementation.
	 *
	 * @note This is an optional function for an rate control module.
	 *
	 * @param context  [in/out]  Pointer to the context of the module implementation.
	 *
	 * @return 0 if successful, error otherwise.
	 */
	int (*initialize)(struct audio_rate_control_implementation_ctx *const context);

	/**
	 * @brief Uninitialize a rate control implementation.
	 *
	 * @note This is an optional function for an rate control module.
	 *
	 * @param context  [in/out]  Pointer to the context of the module implementation.
	 *
	 * @return 0 if successful, error otherwise.
	 */
	int (*uninitialize)(struct audio_rate_control_implementation_ctx *const context);

	/**
	 * @brief Reset a rate control module implementation back to the last configured state.
	 *
	 * @note This is an optional function for an rate control module.
	 *
	 * @param context  [in/out]  Pointer to the context of the module implementation.
	 *
	 * @return 0 if successful, error otherwise.
	 */
	int (*reset)(struct audio_rate_control_implementation_ctx *const context);

	/**
	 * @brief Configure an rate control module after it has been initialized.
	 *
	 * @note This is an optional function for an rate control module.
	 *
	 * @param context        [in/out]  Pointer to the context of the module implementation.
	 * @param configuration  [in]      Pointer to the desired configuration to set.
	 *
	 * @return 0 if successful, error otherwise.
	 */
	int (*configuration_set)(
		struct audio_rate_control_implementation_ctx *const context,
		struct audio_rate_control_configuration const *const configuration);

	/**
	 * @brief Get the configuration of an active rate control implementation.
	 *
	 * @note This is an optional function for an rate control module.
	 *
	 * @param context        [in/out]  Pointer to the context of the module implementation.
	 * @param configuration  [out]  Pointer to the module's current configuration.
	 *
	 * @return 0 if successful, error otherwise.
	 */
	int (*configuration_get)(struct audio_rate_control_implementation_ctx const *const context,
				 struct audio_rate_control_configuration *configuration);

	/**
	 * @brief Update the rate control implementation.
	 *
	 * @note This is a mandatory function for an rate control module.
	 *
	 * @param context  [in/out]  Pointer to the context of the module implementation.
	 * @param error    [out]     Pointer to the error to adjust by.
	 *
	 * @return 0 if successful, error otherwise.
	 */
	int (*update)(struct audio_rate_control_implementation_ctx *const context,
		      void *const error);
};

/**
 * @brief Audio rate control's private context.
 */
struct audio_rate_control_context {
	/* Opaque structure to the implemented rate control context */
	struct audio_rate_control_implementation_ctx *imp_ctx;

	/* Audio rate_control state */
	enum audio_rate_control_state state;

	/* Callbacks for the rate control implementation */
	struct audio_rate_control_ops cb;
};

/**
 * @brief Initialize an instance of the audio rate control module with the
 *        specified initial configuration.
 *
 * @param context             [in/out]  Pointer to the audio rate control's context.
 * @param implementation_ctx  [in]      Pointer to the implemented rate control's context.
 * @param implementation_cb   [in]      Pointer to the table of callbacks.
 *
 * @return 0 if successful, error otherwise.
 */
int audio_rate_control_init(struct audio_rate_control_context *const context,
			    struct audio_rate_control_implementation_ctx *const implementation_ctx,
			    struct audio_rate_control_ops const *const implementation_cb);

/**
 * @brief Uninitialize an audio rate control module.
 *
 * @param context  [in/out]  Pointer to the audio rate control's context.
 *
 * @return 0 if successful, error otherwise.
 */
int audio_rate_control_uninit(struct audio_rate_control_context *const context);

/**
 * @brief Reset an audio rate control module back to the last configuration.
 *
 * @param context  [in/out]  Pointer to the audio rate control's context.
 *
 * @return 0 if successful, error otherwise.
 */
int audio_rate_control_reset(struct audio_rate_control_context *const context);

/**
 * @brief Configure an audio rate control after it has been initialized.
 *
 * @param context        [in/out]  Pointer to the audio rate control's context.
 * @param configuration  [in]      Pointer to the desired configuration to set.
 *
 * @return 0 if successful, error otherwise.
 */
int audio_rate_control_configuration_set(
	struct audio_rate_control_context *const context,
	struct audio_rate_control_configuration const *const configuration);

/**
 * @brief Get the configuration of an audio rate control module.
 *
 * @param context        [in]      Pointer to the audio rate control's context.
 * @param configuration  [out]     Pointer to the module's current configuration.
 *
 * @return 0 if successful, error otherwise.
 */
int audio_rate_control_configuration_get(
	struct audio_rate_control_context const *const context,
	struct audio_rate_control_configuration *const configuration);

/**
 * @brief Update and get the new error between PTSs for an audio rate control module.
 *
 * @param context  [in/out]  Pointer to the audio rate control's context.
 * @param error    [out]     The pointer of the error value.
 *                           Note the data type is user defined and must align with
 *                           the selected rate control and rate control APIs.
 *
 * @return 0 if successful, error otherwise.
 */
int audio_rate_control_update(struct audio_rate_control_context *const context, void *const error);

/**
 * @}
 */

#endif /* _AUDIO_RATE_CONTROL_H_ */
