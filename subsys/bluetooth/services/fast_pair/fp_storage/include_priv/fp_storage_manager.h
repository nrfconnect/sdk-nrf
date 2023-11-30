/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef FP_STORAGE_MANAGER_H_
#define FP_STORAGE_MANAGER_H_

/**
 * @defgroup fp_storage_manager Fast Pair storage manager
 * @brief The subsystem manages Fast Pair storage modules.
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include <zephyr/toolchain.h>

/**
 * @typedef fp_storage_manager_module_reset_perform
 * Callback used to perform a reset of the Fast Pair storage module.
 *
 * @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
typedef int (*fp_storage_manager_module_reset_perform)(void);

/**
 * @typedef fp_storage_manager_module_reset_prepare
 * Callback used to inform the Fast Pair storage module that the reset operation is due to begin.
 */
typedef void (*fp_storage_manager_module_reset_prepare)(void);

/**
 * @typedef fp_storage_manager_module_init
 * Callback used to initialize the Fast Pair storage module.
 *
 * @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
typedef int (*fp_storage_manager_module_init)(void);

/**
 * @typedef fp_storage_manager_module_uninit
 * Callback used to uninitialize the Fast Pair storage module.
 *
 * @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
typedef int (*fp_storage_manager_module_uninit)(void);

/** Structure describing Fast Pair storage module. */
struct fp_storage_manager_module {
	/** Function used to perform a reset of the Fast Pair storage module. */
	fp_storage_manager_module_reset_perform module_reset_perform;

	/** Function used to inform the Fast Pair storage module that the reset operation is due to
	 * begin. This function is always called before @ref module_reset_perform.
	 */
	fp_storage_manager_module_reset_prepare module_reset_prepare;

	/** Function used to initialize Fast Pair storage module. */
	fp_storage_manager_module_init module_init;

	/** Function used to uninitialize Fast Pair storage module. */
	fp_storage_manager_module_uninit module_uninit;
};

/** Register Fast Pair storage module.
 *
 * The macro statically registers a Fast Pair storage module to Fast Pair storage manager.
 * The module is controlled by the Fast Pair storage manager to ensure storage data integrity among
 * all storage modules. The Fast Pair storage manager uses provided callbacks to interact with
 * storage modules.
 *
 * @param _name				Storage module name.
 * @param _module_reset_perform_fn	Function used to perform a reset of the storage module.
 * @param _module_reset_prepare_fn	Function used to inform the storage module that the reset
 *					operation is due to begin.
 * @param _module_init_fn		Function used to initialize the storage module
 *					(can be NULL).
 * @param _module_uninit_fn		Function used to uninitialize the storage module
 *					(can be NULL).
 */
#define FP_STORAGE_MANAGER_MODULE_REGISTER(_name, _module_reset_perform_fn,			\
					   _module_reset_prepare_fn, _module_init_fn,		\
					   _module_uninit_fn)					\
	BUILD_ASSERT(_module_reset_perform_fn != NULL);						\
	BUILD_ASSERT(_module_reset_prepare_fn != NULL);						\
	BUILD_ASSERT((_module_init_fn == NULL) == (_module_uninit_fn == NULL));			\
	static const STRUCT_SECTION_ITERABLE(fp_storage_manager_module, _name) = {		\
		.module_reset_perform = _module_reset_perform_fn,				\
		.module_reset_prepare = _module_reset_prepare_fn,				\
		.module_init = _module_init_fn,							\
		.module_uninit = _module_uninit_fn,						\
	}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* FP_STORAGE_MANAGER_H_ */
