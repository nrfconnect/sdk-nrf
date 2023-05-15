/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _DFU_LOCK_H_
#define _DFU_LOCK_H_

#include <zephyr/types.h>

/**
 * @defgroup dfu_lock DFU Lock API
 * @brief DFU Lock API
 *
 * @{
 */

/** @brief DFU lock owner descriptor. */
struct dfu_lock_owner {
	/** Owner name. */
	const char *name;

	/** Notify the previous DFU owner that another DFU owner claimed the lock.
	 *
	 * This information can be used to perform the cleanup operations before
	 * the subsequent DFU attempts of the previous DFU owner. Typically, the
	 * previous DFU owner should reset its DFU progress and make sure that
	 * the DFU flash memory is erased before writing new image content. Please
	 * note that the flash erase operation can only be done once the previous
	 * owner reclaims the DFU lock.
	 *
	 * This callback is executed from the context used by the new DFU owner to
	 * claim lock. During the callback execution, mutex of the DFU lock module
	 * is locked. It is recommended to perform simple operations within the
	 * context of this callback.
	 *
	 * @param new_owner New DFU owner descriptor.
	 */
	void (*owner_changed)(const struct dfu_lock_owner *new_owner);
};

/** Claim the DFU lock.
 *
 * Claim the DFU lock for the provided owner instance. You can start
 * to interact with the DFU flash memory once this function returns
 * with success.
 *
 * Claiming the DFU lock that you already claimed with the same owner
 * instance has no effect and does not change the module state.
 *
 * @param new_owner New DFU owner descriptor.
 *
 * @retval 0 on success.
 * @retval -EPERM if the DFU lock has already been claimed by another owner.
 */
int dfu_lock_claim(const struct dfu_lock_owner *new_owner);

/** Release the DFU lock.
 *
 * Release the DFU lock for the provided owner instance to allow other owners
 * to lock it. You must stop interacting with the DFU flash memory once you
 * release the DFU lock.
 *
 * @param owner DFU owner descriptor.
 *
 * @retval 0 on success.
 * @retval -EPERM if the DFU lock has not been claimed by the owner.
 */
int dfu_lock_release(const struct dfu_lock_owner *owner);

/**
 * @}
 */

#endif /* _DFU_LOCK_H_ */
