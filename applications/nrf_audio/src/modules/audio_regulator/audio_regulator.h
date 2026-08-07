/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @defgroup audio_regulator Audio Regulator API
 * @{
 * @brief Audio Regulator API for nRF Audio applications.
 *
 * This Audio Regulator provides generic functions to manage the audio synchronisation. It uses
 * the specified method given by the callback functions.
 */

#ifndef _AUDIO_REGULATOR_H_
#define _AUDIO_REGULATOR_H_

/**
 * @brief The state of the audio regulator.
 */
enum audio_regulator_state {
	AUDIO_REGULATOR_STATE_UNINITIALIZED = 0,
	AUDIO_REGULATOR_STATE_INITIALIZED,
};

/**
 * @brief Audio regulator's opaque implementation context structure.
 */
struct audio_regulator_implementation_ctx;

/**
 * @brief Audio regulator's opaque configuration structure.
 */
struct audio_regulator_configuration;

/**
 * @brief Audio regulator's callback structure.
 */
struct audio_regulator_ops {
	/**
	 * @brief Initialize a regulator module implementation.
	 *
	 * @note This is an optional function for a regulator module.
	 *
	 * @param context  [in/out]  Pointer to the context to the module instance.
	 *
	 * @return 0 if successful, error otherwise.
	 */
	int (*initialize)(struct audio_regulator_implementation_ctx *const context);

	/**
	 * @brief Uninitialize a regulator implementation.
	 *
	 * @note This is an optional function for a regulator module.
	 *
	 * @param context  [in/out]  Pointer to the context of the module implementation.
	 *
	 * @return 0 if successful, error otherwise.
	 */
	int (*uninitialize)(struct audio_regulator_implementation_ctx *const context);

	/**
	 * @brief Reset a regulator module implementation back to the last configured state.
	 *
	 * @note This is an optional function for a regulator module.
	 *
	 * @param context  [in/out]  Pointer to the context of the module implementation.
	 *
	 * @return 0 if successful, error otherwise.
	 */
	int (*reset)(struct audio_regulator_implementation_ctx *const context);

	/**
	 * @brief Configure a regulator module after it has been initialized.
	 *
	 * @note This is an optional function for a regulator module.
	 *
	 * @param context        [in/out]  Pointer to the context of the module implementation.
	 * @param configuration  [in]      Pointer to the desired configuration to set.
	 *
	 * @return 0 if successful, error otherwise.
	 */
	int (*configuration_set)(struct audio_regulator_implementation_ctx *const context,
				 struct audio_regulator_configuration const *const configuration);

	/**
	 * @brief Get the configuration of an active regulator implementation.
	 *
	 * @note This is an optional function for a regulator module.
	 *
	 * @param context        [in]   Pointer to the context of the module implementation.
	 * @param configuration  [out]  Pointer to the module's current configuration.
	 *
	 * @return 0 if successful, error otherwise.
	 */
	int (*configuration_get)(struct audio_regulator_implementation_ctx const *const context,
				 struct audio_regulator_configuration *configuration);

	/**
	 * @brief Update the regulator implementation.
	 *
	 * @note This is a mandatory function for a regulator module.
	 *
	 * @param context  [in/out]  Pointer to the context of the module implementation.
	 * @param pts_ref  [in]      Pointer to the reference PTS.
	 * @param pts      [in]      Pointer to the captured PTS.
	 * @param error    [out]     The pointer of the location to write the error value.
	 *
	 * @return 0 if successful, error otherwise.
	 */
	int (*update)(struct audio_regulator_implementation_ctx *const context, void *const pts_ref,
		      void *const pts, void *const error);

	/**
	 * @brief Retrieve the last error calculated by the regulator implementation.
	 *
	 * @note This is an optional function for a regulator module.
	 *
	 * @param context  [in/out]  Pointer to the context of the module implementation.
	 * @param error    [out]     Pointer to the error.
	 *
	 * @return 0 if successful, error otherwise.
	 */
	int (*error_get)(struct audio_regulator_implementation_ctx *const context,
			 void *const error);
};

/**
 * @brief Audio regulator's private configuration.
 */
struct audio_regulator_context {
	/* Opaque structure to the implemented regulator context */
	struct audio_regulator_implementation_ctx *imp_ctx;

	/* Audio regulator state */
	enum audio_regulator_state state;

	/* Callbacks for the regulator implementation */
	struct audio_regulator_ops cb;
};

/**
 * @brief Initialize an instance of the audio regulator module with the
 *        specified initial configuration.
 *
 * @param context            [in/out]  Pointer to the audio regulator's context.
 * @param implemented_ctx    [in]      Pointer to the implemented regulator's context.
 * @param implementation_cb  [in]      Pointer to the table of callbacks.
 *
 * @return 0 if successful, error otherwise.
 */
int audio_regulator_init(struct audio_regulator_context *const context,
			 struct audio_regulator_implementation_ctx const *const implementation_ctx,
			 struct audio_regulator_ops const *const implementation_cb);

/**
 * @brief Uninitialize an audio regulator module.
 *
 * @param context  [in/out]  Pointer to the audio regulator's context.
 *
 * @return 0 if successful, error otherwise.
 */
int audio_regulator_uninit(struct audio_regulator_context *const context);

/**
 * @brief Reset an audio regulator module back to the last configuration.
 *
 * @param context  [in/out]  Pointer to the audio regulator's context.
 *
 * @return 0 if successful, error otherwise.
 */
int audio_regulator_reset(struct audio_regulator_context *const context);

/**
 * @brief Set the audio regulator module's callback function pointers.
 *
 * @param context  [in/out]  Pointer to the audio regulator's context.
 * @param imp_cb   [out]     Pointer to a structure of callback functions.
 *
 * @return 0 if successful, error otherwise.
 */
int audio_regulator_cb_set(struct audio_regulator_context *const context,
			   struct audio_regulator_ops const *const imp_cb);

/**
 * @brief Configure an audio regulator after it has been initialized.
 *
 * @param context        [in/out]  Pointer to the audio regulator's context.
 * @param configuration  [in]      Pointer to the desired configuration to set.
 *
 * @return 0 if successful, error otherwise.
 */
int audio_regulator_configuration_set(
	struct audio_regulator_context *const context,
	struct audio_regulator_configuration const *const configuration);

/**
 * @brief Get the configuration of an audio regulator module.
 *
 * @param context        [in]      Pointer to the audio regulator's context.
 * @param configuration  [out]     Pointer to the module's current configuration.
 *
 * @return 0 if successful, error otherwise.
 */
int audio_regulator_configuration_get(struct audio_regulator_context const *const context,
				      struct audio_regulator_configuration *const configuration);

/**
 * @brief Update and get the new error between PTSs for an audio regulator module.
 *
 * @param context  [in/out]  Pointer to the audio regulator's context.
 * @param pts_ref  [in]      Pointer to the reference presentation time-stamp (PTS).
 * @param pts      [in]      Pointer to the captured presentation time-stamp (PTS).
 * @param error    [out]     Pointer to the location to write the error value.
 *
 * @return 0 if successful, error otherwise.
 */
int audio_regulator_update_error(struct audio_regulator_context const *const context,
				 void *const pts_ref, void *const pts, void *const error);

/**
 * @brief Get the error of an audio regulator module.
 *
 * @param context  [in/out]  Pointer to the audio regulator's context.
 * @param error    [out]     Pointer to the location to write the error value.
 *
 * @return 0 if successful, error otherwise.
 */
int audio_regulator_error_get(struct audio_regulator_context const *const context,
			      void *const error);

/**
 * @}
 */

#endif /* _AUDIO_REGULATOR_H_ */
