/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @defgroup	audio_rate_control		Audio rate control API
 * @{
 * @brief	Audio rate control API for nRF Audio applications.
 *
 * This Audio Rate Control provides generic functions to manage the
 * audio synchronization. On the nRF5340 SoC, the audio synchronization
 * is typically controlled with the Audio Phase-Locked Loop (PLL).
 * Although where this is not available, alternative methods can be
 * configured through the callback functions.
 */

#ifndef _AUDIO_RATE_CONTROL_H_
#define _AUDIO_RATE_CONTROL_H_

/**
 * @brief	The state of the audio rate control.
 */
enum audio_rate_control_state {
	AUDIO_RATE_CONTROL_STATE_UNINITIALIZED = 0,
	AUDIO_RATE_CONTROL_STATE_INITIALIZED,
};

/**
 * @brief	Audio rate control's opaque implementation context structure.
 */
struct audio_rate_control_imp_ctx;

/**
 * @brief	Audio rate control's opaque configuration structure.
 */
struct audio_rate_control_cfg;

/**
 * @brief	Audio rate control's callback structure.
 */
struct audio_rate_control_ops {
	/**
	 * @brief	Initialize a rate control module implementation.
	 *
	 * @note	This is an optional function for a rate control module.
	 *
	 * @param	ctx	[in/out]	Pointer to the context of the module implementation.
	 *
	 * @return	0 if successful, error otherwise.
	 */
	int (*initialize)(struct audio_rate_control_imp_ctx *const ctx);

	/**
	 * @brief	Uninitialize a rate control implementation.
	 *
	 * @note	This is an optional function for a rate control module.
	 *
	 * @param	ctx	[in/out]	Pointer to the context of the module implementation.
	 *
	 * @return	0 if successful, error otherwise.
	 */
	int (*uninitialize)(struct audio_rate_control_imp_ctx *ctx);

	/**
	 * @brief	Reset a rate control module implementation back to the last valid
	 *		configuration..
	 *
	 * @note	This is an optional function for a rate control module.
	 *
	 * @param	ctx	[in/out]	Pointer to the context of the module implementation.
	 *
	 * @return	0 if successful, error otherwise.
	 */
	int (*reset)(struct audio_rate_control_imp_ctx *const ctx);

	/**
	 * @brief	Configure a rate control module after it has been initialized.
	 *
	 * @note	This is an optional function for a rate control module.
	 *
	 * @param	ctx	[in/out]	Pointer to the context of the module implementation.
	 * @param	cfg	[in]		Pointer to the desired configuration to set.
	 *
	 * @return	0 if successful, error otherwise.
	 */
	int (*cfg_set)(struct audio_rate_control_imp_ctx *const ctx,
		       struct audio_rate_control_cfg const *const cfg);

	/**
	 * @brief	Get the configuration of an active rate control implementation.
	 *
	 * @note	This is an optional function for a rate control module.
	 *
	 * @param	ctx	[in/out]	Pointer to the context of the module implementation.
	 * @param	cfg	[out]		Pointer to the module's current configuration.
	 *
	 * @return	0 if successful, error otherwise.
	 */
	int (*cfg_get)(struct audio_rate_control_imp_ctx const *const ctx,
		       struct audio_rate_control_cfg *cfg);

	/**
	 * @brief	Update the rate control value implementation.
	 *
	 * @note	This is a mandatory function for a rate control module.
	 *
	 * @param	ctx		[in/out]	Pointer to the context of the module
	 *						implementation.
	 * @param	control_val_u	[in]		Pointer to the rate control value.
	 *
	 * @return	0 if successful, error otherwise.
	 */
	int (*update)(struct audio_rate_control_imp_ctx *const ctx, void *const control_val_u);
};

/**
 * @brief	Audio rate control's private context.
 *
 * @note	This structure, when allocated, must remain valid for the
 *		lifetime of the instance of the module.
 */
struct audio_rate_control_ctx {
	/* Opaque structure to the implemented rate control context.
	 * The structure and the pointer, must remain valid for the
	 * lifetime of the instance of the module.
	 */
	struct audio_rate_control_imp_ctx *imp_ctx;

	/* Audio rate_control state */
	enum audio_rate_control_state state;

	/* Callbacks for the rate control implementation */
	struct audio_rate_control_ops cb;
};

/**
 * @brief	Initialize an instance of the audio rate control module with the
 *		specified initial configuration.
 *
 * @param	ctx	[in/out]	Pointer to the audio rate control's context.
 * @param	imp_ctx	[in]		Pointer to the implemented rate control's context.
 * @param	imp_cb	[in]		Pointer to the table of callbacks.
 *
 * @return	0 if successful, error otherwise.
 */
int audio_rate_control_init(struct audio_rate_control_ctx *const ctx,
			    struct audio_rate_control_imp_ctx *const imp_ctx,
			    struct audio_rate_control_ops const *const imp_cb);

/**
 * @brief	Uninitialize an audio rate control module.
 *
 * @note	It is the responsibility of the caller to release any memory
 *		associated with the context areas.
 *
 * @param	ctx	[in/out]	Pointer to the audio rate control's context.
 *
 * @return	0 if successful, error otherwise.
 */
int audio_rate_control_uninit(struct audio_rate_control_ctx *ctx);

/**
 * @brief	Reset an audio rate control module back to the last valid configuration.
 *
 * @param	ctx	[in/out]	Pointer to the audio rate control's context.
 *
 * @return	0 if successful, error otherwise.
 */
int audio_rate_control_reset(struct audio_rate_control_ctx *const ctx);

/**
 * @brief	Configure an audio rate control after it has been initialized.
 *
 * @param	ctx	[in/out]	Pointer to the audio rate control's context.
 * @param	cfg	[in]		Pointer to the desired configuration to set.
 *
 * @return	0 if successful, error otherwise.
 */
int audio_rate_control_cfg_set(struct audio_rate_control_ctx *const ctx,
			       struct audio_rate_control_cfg const *const cfg);

/**
 * @brief	Get the configuration of an audio rate control module.
 *
 * @param	ctx	[in]		Pointer to the audio rate control's context.
 * @param	cfg	[out]		Pointer to the module's current configuration.
 *
 * @return	0 if successful, error otherwise.
 */
int audio_rate_control_cfg_get(struct audio_rate_control_ctx const *const ctx,
			       struct audio_rate_control_cfg *const cfg);

/**
 * @brief	Update and get the new error between timestamps for an audio rate control module.
 *
 * @param	ctx		[in/out]	Pointer to the audio rate control's context.
 * @param	control_val_u	[in]		The pointer of the rate control value.
 *						Note the data type is user defined and must align
 *						with the selected rate control APIs.
 *
 * @return	0 if successful, error otherwise.
 */
int audio_rate_control_update(struct audio_rate_control_ctx *const ctx, void *const control_val_u);

/**
 * @}
 */

#endif /* _AUDIO_RATE_CONTROL_H_ */
