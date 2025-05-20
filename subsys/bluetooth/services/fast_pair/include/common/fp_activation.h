/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef FP_ACTIVATION_H_
#define FP_ACTIVATION_H_

/**
 * @defgroup fp_activation Fast Pair activation module
 * @brief The subsystem manages enabling and disabling Fast Pair modules.
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include <zephyr/toolchain.h>

/** Default initialization priority. */
#define FP_ACTIVATION_INIT_PRIORITY_DEFAULT	5

/**
 * @typedef fp_activation_module_init
 * Callback used to initialize the Fast Pair module.
 *
 * @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
typedef int (*fp_activation_module_init)(void);

/**
 * @typedef fp_activation_module_uninit
 * Callback used to uninitialize the Fast Pair module.
 *
 * @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
typedef int (*fp_activation_module_uninit)(void);

/** Structure describing Fast Pair module. */
struct fp_activation_module {
	/** Function used to initialize Fast Pair module. */
	fp_activation_module_init module_init;

	/** Function used to uninitialize Fast Pair module. */
	fp_activation_module_uninit module_uninit;
};

/** Register Fast Pair module to Fast Pair activation module.
 *
 * The macro statically registers a Fast Pair module to Fast Pair activation module.
 * The module is controlled by the Fast Pair activation module to ensure proper initialization and
 * uninitialization of registered Fast Pair modules. The Fast Pair activation module uses provided
 * callbacks to interact with registered modules when @ref bt_fast_pair_enable or
 * @ref bt_fast_pair_disable is called.
 *
 * @param _name			Module name.
 * @param _priority		Module's priority number.
 *				Allowed priority number range is 0-9. The number must be single
 *				decimal digit. The lower priority number (higher priority),
 *				the earlier the module will be initialized in initialization chain.
 *				Uninitialization is performed in reversed order of
 *				the initialization chain.
 * @param _module_init_fn	Function used to initialize the module.
 * @param _module_uninit_fn	Function used to uninitialize the module.
 */
#define FP_ACTIVATION_MODULE_REGISTER(_name, _priority, _module_init_fn, _module_uninit_fn)	\
	BUILD_ASSERT((_priority >= 0) && (_priority <= 9));					\
	BUILD_ASSERT(_module_init_fn != NULL);							\
	BUILD_ASSERT(_module_uninit_fn != NULL);						\
	static const STRUCT_SECTION_ITERABLE(fp_activation_module,				\
					     _CONCAT(_CONCAT(_, _priority), _name)) = {		\
		.module_init = _module_init_fn,							\
		.module_uninit = _module_uninit_fn,						\
	}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* FP_ACTIVATION_H_ */
