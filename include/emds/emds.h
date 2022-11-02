/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file emds.h
 *
 *  @defgroup emds Emergency Data Storage Library
 *  @{
 *  @brief Emergency Data Storage API
 */

#ifndef EMDS_H__
#define EMDS_H__

#include <stddef.h>
#include <sys/types.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/slist.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct emds_entry
 *
 * Information about entries used to store data in the emergency data storage.
 */
struct emds_entry {
	/** Unique ID for each static and dynamic entry. */
	uint16_t id;
	/** Pointer to data that will be stored. */
	uint8_t *data;
	/** Length of data that will be stored. */
	size_t len;
};

/**
 * @struct emds_dynamic_entry
 *
 * Entries with a node element, used for dynamic entries. These are registered
 * using a call to @ref emds_entry_add.
 */
struct emds_dynamic_entry {
	struct emds_entry entry;
	sys_snode_t node;
};

/**
 * @brief Define a static entry for emergency data storage items.
 *
 * @param _name The entry name.
 * @param _id Unique ID for the entry. This value and not an overlap with any
 *            other value.
 * @param _data Data pointer to be stored at emergency data store.
 * @param _len Length of data to be stored at emergency data store.
 *
 * This creates a variable _name prepended by emds_.
 */
#define EMDS_STATIC_ENTRY_DEFINE(_name, _id, _data, _len)                      \
	static const STRUCT_SECTION_ITERABLE(emds_entry, emds_##_name) = {     \
		.id = _id,                                                     \
		.data = (uint8_t *)_data,                                      \
		.len = _len,                                                   \
	}

/**
 * @typedef emds_store_cb_t
 * @brief Callback for application commands when storing has been executed.
 *
 * This can be used to perform other actions or execute busy-waiting until the
 * power supply is discharged. It will be run by @ref emds_store,
 * and called from the same context the @ref emds_store is called from.
 * Interrupts are unlocked before triggering this callback. Therefore, this
 * callback may not be reached before available backup power runs out.
 * If the application needs interrupts to be locked during this callback,
 * application can do so by calling irq_lock before calling @ref emds_store.
 */
typedef void (*emds_store_cb_t)(void);

/**
 * @brief Initialize the emergency data storage.
 *
 * Initializes the emergency data storage. This needs to be called before adding
 * entries to the @ref emds_dynamic_entry and loading data.
 *
 * @param cb Callback to notify application that the store operation has been
 *           executed or NULL if no notification is needed.
 *
 * @retval 0 Success
 * @retval -ERRNO errno code if error
 */
int emds_init(emds_store_cb_t cb);

/**
 *  @brief Add entry to be saved/restored when emergency data storage is called.
 *
 * Adds the entry to the dynamic entry list. When the @ref emds_store function
 * is called, takes the given data pointer and stores the data to the emergency
 * data storage.
 *
 * @note EMDS does not make a local copy of the dynamic entry structure.
 *
 * @param entry Entry to add to list and load data into.
 *
 * @retval 0 Success
 * @retval -ERRNO errno code if error
 */
int emds_entry_add(struct emds_dynamic_entry *entry);

/**
 * @brief Start the emergency data storage process.
 *
 * Triggers the process of storing all data registered to be stored. All data
 * registered either through @ref emds_entry_add function or the
 * @ref EMDS_STATIC_ENTRY_DEFINE macro is stored. It locks all interrupts until
 * the write is finished. Once the data storage is completed, the data should
 * not be changed, and the device should be halted. The device must not be
 * allowed to reboot when operating on a backup supply, since reboot will
 * trigger data load from the EMDS storage and clear the storage area. The reboot
 * should only be allowed when the main power supply is available.
 *
 * This is a time-critical operation and will be processed as fast as possible.
 * To achieve better time predictability, this function must be called from an
 * interrupt context with the highest priority.
 *
 * When writing to flash, it bypasses the flash driver. Therefore, when running
 * with MPSL, make sure to uninitialize the MPSL before this function is called.
 * Otherwise, an assertion may be triggered by the exit of the function.
 *
 * @retval 0 Success
 * @retval -ERRNO errno code if error
 */
int emds_store(void);

/**
 * @brief Load all static data from the emergency data storage.
 *
 * This function needs to be called after the static entries are added, as they
 * are used to select the data to be loaded. The function also needs to be
 * called before the @ref emds_prepare function which will delete all the
 * previously stored data.
 *
 * @retval 0 Success
 * @retval -ERRNO errno code if error
 */
int emds_load(void);

/**
 * @brief Clear flash area from the emergency data storage.
 *
 * This function clears the flash area for all previously stored data.
 *
 * @retval 0 Success
 * @retval -ERRNO errno code if error
 */
int emds_clear(void);

/**
 * @brief Prepare flash area for the next emergency data storage.
 *
 * This function prepares the flash area for the next emergency data storage. It
 * deletes the current entries if there is enough space for the next emergency
 * data storage, and clears the flash storage if there is not enough space for
 * the next storage. This has to be done after all the dynamic entries are
 * added. After this has been called emergency data storage should be ready to
 * store.
 *
 * @retval 0 Success
 * @retval -ERRNO errno code if error
 */
int emds_prepare(void);

/**
 * @brief Estimate the time needed to store the registered data.
 *
 * Estimate how much time it takes to store all dynamic and static data
 * registered in the entries. This value is dependent on the chip used, and
 * should be checked against the chip datasheet.
 *
 * @return Time needed to store all data (in microseconds).
 */
uint32_t emds_store_time_get(void);

/**
 * @brief Calculate the size needed to store the registered data.
 *
 * Calculates the size it takes to store all dynamic and static data registered
 * in the entries.
 *
 * @return Byte size that is needed to store all data.
 */
uint32_t emds_store_size_get(void);

/**
 * @brief Check if the store operation can be run.
 *
 * When the data store operation has completed, the store operation is no longer
 * ready and this function will then return false.
 *
 * @return True if the store operation can be started, otherwise false.
 */
bool emds_is_ready(void);

#ifdef __cplusplus
}
#endif

#endif /* EMDS_H__ */

/** @} */
