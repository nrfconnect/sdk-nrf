/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @defgroup	audio_regulator		Audio Regulator API
 * @{
 * @brief	Audio Regulator API for nRF Audio applications.
 *
 * This Audio Regulator provides generic functions to manage the audio synchronisation. It uses
 * the specified method given by the callback functions. Note that asynchronous regulators are not
 * supported, and the update function must be called in a synchronous manner.
 */

#ifndef _AUDIO_REGULATOR_H_
#define _AUDIO_REGULATOR_H_

/**
 * @brief	The state of the audio regulator.
 */
enum audio_regulator_state {
	AUDIO_REGULATOR_STATE_UNINITIALIZED = 0,
	AUDIO_REGULATOR_STATE_INITIALIZED,
};

/**
 * @brief	Audio regulator's opaque implementation context structure.
 */
struct audio_regulator_imp_ctx;

/**
 * @brief	Audio regulator's opaque configuration structure.
 */
struct audio_regulator_cfg;

/**
 * @brief	Audio regulator's API structure.
 */
struct audio_regulator_ops {
	/**
	 * @brief	Initialize a regulator module implementation.
	 *
	 * @note	This is an optional function for a regulator module.
	 *
	 * @param	ctx	[in/out]	Pointer to the context to the module instance.
	 *
	 * @return	0 if successful, error otherwise.
	 */
	int (*initialize)(struct audio_regulator_imp_ctx *const ctx);

	/**
	 * @brief	Uninitialize a regulator implementation.
	 *
	 * @note	This is an optional function for a regulator module.
	 *
	 * @param	ctx	[in/out]	Pointer to the context of the module implementation.
	 *
	 * @return	0 if successful, error otherwise.
	 */
	int (*uninitialize)(struct audio_regulator_imp_ctx *const ctx);

	/**
	 * @brief	Reset a regulator module implementation back to the last valid
	 *		configuration.
	 *
	 * @note	This is an optional function for a regulator module.
	 *
	 * @param	ctx	[in/out]	Pointer to the context of the module implementation.
	 *
	 * @return	0 if successful, error otherwise.
	 */
	int (*reset)(struct audio_regulator_imp_ctx *const ctx);

	/**
	 * @brief	Configure a regulator module after it has been initialized.
	 *
	 * @note	This is an optional function for a regulator module.
	 *
	 * @param	ctx	[in/out]	Pointer to the context of the module implementation.
	 * @param	cfg	[in]		Pointer to the desired configuration to set.
	 *
	 * @return	0 if successful, error otherwise.
	 */
	int (*cfg_set)(struct audio_regulator_imp_ctx *const ctx,
		       struct audio_regulator_cfg const *const cfg);

	/**
	 * @brief	Get the configuration of an active regulator implementation.
	 *
	 * @note	This is an optional function for a regulator module.
	 *
	 * @param	ctx	[in/out]	Pointer to the context of the module implementation.
	 * @param	cfg	[out]		Pointer to the module's current configuration.
	 *
	 * @return	0 if successful, error otherwise.
	 */
	int (*cfg_get)(struct audio_regulator_imp_ctx const *const ctx,
		       struct audio_regulator_cfg *cfg);

	/**
	 * @brief	Update the regulator implementation.
	 *
	 * @note	This is a mandatory function for a regulator module.
	 *		As of now, only a synchronous regulator is supported.
	 *		This means that this function must be called at regular intervals.
	 *		Furthermore, the underlying implementation must be able to correctly
	 *		handle variations in delta_t between calls to this function.
	 *
	 * @param	ctx		[in/out]	Pointer to the context of the module
	 *						implementation.
	 * @param	ref_val_r	[in]		Pointer to the reference value.
	 * @param	meas_val_y	[in]		Pointer to the measured value.
	 * @param	control_val_u	[out]		Pointer to the control value.
	 *
	 * @return	0 if successful, error otherwise.
	 */
	int (*update)(struct audio_regulator_imp_ctx *const ctx, void *const ref_val_r,
		      void *const meas_val_y, void *const control_val_u);

	/**
	 * @brief	Retrieve the last control_val_u calculated by the regulator implementation.
	 *
	 * @note	This is an optional function for a regulator module.
	 *
	 * @param	ctx		[in/out]	Pointer to the context of the module
	 *						implementation.
	 * @param	control_val_u	[out]		Pointer to the control value.
	 *
	 * @return	0 if successful, error otherwise.
	 */
	int (*control_get)(struct audio_regulator_imp_ctx *const ctx, void *const control_val_u);
};

/**
 * @brief	Audio regulator's private configuration.
 *
 * @note	This structure, when allocated, must remain valid for the
 *		lifetime of the instance of the module.
 */
struct audio_regulator_ctx {
	/* Opaque structure to the implemented regulator context
	 * The structure and the pointer must remain valid for
	 * the lifetime of the instance of the module.
	 */
	struct audio_regulator_imp_ctx *imp_ctx;

	/* Audio regulator state, that must remain valid for the
	 * lifetime of the instance of the module
	 */
	enum audio_regulator_state state;

	/* Callbacks for the regulator implementation */
	struct audio_regulator_ops cb;
};

/**
 * @brief	Initialize an instance of the audio regulator module with the
 *		specified initial configuration.
 *
 * @param	ctx	[in/out]	Pointer to the audio regulator's context.
 * @param	imp_ctx	[in]		Pointer to the implemented regulator's context.
 * @param	imp_cb	[in]		Pointer to the table of callbacks.
 *
 * @return	0 if successful, error otherwise.
 */
int audio_regulator_init(struct audio_regulator_ctx *const ctx,
			 struct audio_regulator_imp_ctx *const imp_ctx,
			 struct audio_regulator_ops const *const imp_cb);

/**
 * @brief	Uninitialize an audio regulator module.
 *
 * @note	It is the responsibility of the caller to release
 *		any memory associated with the context areas.
 *
 * @param	ctx	[in/out]	Pointer to the audio regulator's context.
 *
 * @return	0 if successful, error otherwise.
 */
int audio_regulator_uninit(struct audio_regulator_ctx *ctx);

/**
 * @brief	Reset an audio regulator module back to the last configuration.
 *
 * @param	ctx	[in/out]	Pointer to the audio regulator's context.
 *
 * @return	0 if successful, error otherwise.

 */
int audio_regulator_reset(struct audio_regulator_ctx *const ctx);

/**
 * @brief	Configure an audio regulator after it has been initialized.
 *
 * @param	ctx	[in/out]	Pointer to the audio regulator's context.
 * @param	cfg	[in]		Pointer to the desired configuration to set.
 *
 * @return	0 if successful, error otherwise.
 */
int audio_regulator_cfg_set(struct audio_regulator_ctx *const ctx,
			    struct audio_regulator_cfg const *const cfg);

/**
 * @brief	Get the configuration of an audio regulator module.
 *
 * @param	ctx	[in]		Pointer to the audio regulator's context.
 * @param	cfg	[out]		Pointer to the module's current configuration.
 *
 * @return	0 if successful, error otherwise.
 */
int audio_regulator_cfg_get(struct audio_regulator_ctx const *const ctx,
			    struct audio_regulator_cfg *const cfg);

/**
 * @brief	Update and get the new control_val_u between the reference value and the measured
 *		value for an audio regulator module.
 *
 * @param	ctx		[in/out]	Pointer to the audio regulator's context.
 * @param	ref_val_r	[in]		Pointer to the reference value.
 * @param	meas_val_y	[in]		Pointer to the measured value.
 * @param	control_val_u	[out]		Pointer to the control value.
 *
 * @return	0 if successful, error otherwise.
 */
int audio_regulator_update_control(struct audio_regulator_ctx const *const ctx,
				   void *const ref_val_r, void *const meas_val_y,
				   void *const control_val_u);

/**
 * @brief	Get the control_val_u of an audio regulator module.
 *
 * @param	ctx		[in/out]	Pointer to the audio regulator's context.
 * @param	control_val_u	[out]		Pointer to the control value.
 *
 * @return	0 if successful, error otherwise.
 */
int audio_regulator_control_get(struct audio_regulator_ctx const *const ctx,
				void *const control_val_u);

/**
 * @}
 */

#endif /* _AUDIO_REGULATOR_H_ */
