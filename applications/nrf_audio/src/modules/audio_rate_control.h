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

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief	The state of the audio rate control.
 */
enum audio_rate_control_state {
	AUDIO_RATE_CONTROL_STATE_EMPTY = 0,
	AUDIO_RATE_CONTROL_STATE_UNINITIALIZED,
	AUDIO_RATE_CONTROL_STATE_INITIALIZED,
};

/**
 * @brief	Enumeration of possible audio rate control sources.
 */
enum audio_rate_control_type {
	AUDIO_PLL = 0,
	USB = 1,
	I2S = 2,
	AUDIO_RATE_CONTROL_TYPE_COUNT,
};

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
	 * @return	0 if successful, error otherwise.
	 */
	int (*initialize)(void);

	/**
	 * @brief	Uninitialize a rate control implementation.
	 *
	 * @note	This is an optional function for a rate control module.
	 *
	 * @return	0 if successful, error otherwise.
	 */
	int (*uninitialize)(void);

	/**
	 * @brief	Reset a rate control module implementation back to the last valid
	 *		configuration..
	 *
	 * @note	This is an optional function for a rate control module.
	 *
	 * @return	0 if successful, error otherwise.
	 */
	int (*reset)(void);

	/**
	 * @brief	Configure a rate control module after it has been initialized.
	 *
	 * @note	This is an optional function for a rate control module.
	 *
	 * @param	cfg	[in]		Pointer to the desired configuration to set.
	 *
	 * @return	0 if successful, error otherwise.
	 */
	int (*cfg_set)(struct audio_rate_control_cfg const *const cfg);

	/**
	 * @brief	Get the configuration of an active rate control implementation.
	 *
	 * @note	This is an optional function for a rate control module.
	 *
	 * @param	cfg	[out]		Pointer to the module's current configuration.
	 *
	 * @return	0 if successful, error otherwise.
	 */
	int (*cfg_get)(struct audio_rate_control_cfg *cfg);

	/**
	 * @brief	Update the rate control value implementation.
	 *
	 * @note	This is a mandatory function for a rate control module.
	 *
	 * @param	control_val_u	[in]		Pointer to the rate control value.
	 * @param	calibrate	[in]		Indicates whether the rate control should be
	 *						recalibrated if necessary.
	 *
	 * @return	0 if successful, error otherwise.
	 */
	int (*update)(void *const control_val_u, bool calibrate);
};

/**
 * @brief	Audio rate control's private context.
 *
 * @note	This structure, when allocated, must remain valid for the
 *		lifetime of the instance of the module.
 */
struct audio_rate_control_ctx {
	/* Audio rate_control state */
	enum audio_rate_control_state state;

	/* Callbacks for the rate control implementation */
	struct audio_rate_control_ops ops;
};

/**
 * @brief	Reset an audio rate control module back to the last valid configuration.
 *
 * @param	type	[in]		The type of the rate control source.
 *
 * @return	0 if successful, error otherwise.
 */
int audio_rate_control_reset(uint8_t type);

/**
 * @brief	Configure an audio rate control after it has been initialized.
 *
 * @param	type	[in]		The type of the rate control source.
 * @param	cfg	[in]		Pointer to the desired configuration to set.
 *
 * @return	0 if successful, error otherwise.
 */
int audio_rate_control_cfg_set(uint8_t type, struct audio_rate_control_cfg const *const cfg);

/**
 * @brief	Get the configuration of an audio rate control module.
 *
 * @param	type	[in]		The type of the rate control source.
 * @param	cfg	[out]		Pointer to the module's current configuration.
 *
 * @return	0 if successful, error otherwise.
 */
int audio_rate_control_cfg_get(uint8_t type, struct audio_rate_control_cfg *const cfg);

/**
 * @brief	Update and get the new error between timestamps for an audio rate control module.
 *
 * @param	type		[in]		The type of the rate control source.
 * @param	control_val_u	[in]		The pointer of the rate control value.
 *						Note the data type is user defined and must align
 *						with the selected rate control APIs.
 * @param	calibrate	[in]		Indicates whether the rate control should be
 *						recalibrated if necessary.
 *
 * @return	0 if successful, error otherwise.
 */
int audio_rate_control_update(uint8_t type, void *const control_val_u, bool calibrate);

/**
 * @brief	Register an audio rate control implementation for a specific type.
 *
 * @param	type		[in]		The type of the rate control source.
 * @param	imp_ops		[in]		Pointer to the implementation operations.
 *
 * @return	0 if successful, error otherwise.
 */
int audio_rate_control_register(uint8_t type, struct audio_rate_control_ops const *const imp_ops);

/**
 * @brief	Uninitialize an audio rate control module.
 *
 * @note	It is the responsibility of the caller to release any memory
 *		associated with the context areas.
 *
 * @param	type	[in]		The type of the rate control source.
 *
 * @return	0 if successful, error otherwise.
 */
int audio_rate_control_uninit(uint8_t type);

/**
 * @brief	Initialize an instance of the audio rate control module with the
 *		specified initial configuration.
 *
 * @param	type	[in]		The type of the rate control source.
 *
 * @return	0 if successful, error otherwise.
 */
int audio_rate_control_init(uint8_t type);

/**
 * @}
 */

#endif /* _AUDIO_RATE_CONTROL_H_ */
