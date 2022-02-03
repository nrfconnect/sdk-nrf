/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @defgroup emds Emergency Data Storage
 *  @ingroup emds
 *  @{
 *  @brief Emergency Data Storage API
 */

#ifndef EMDS_H__
#define EMDS_H__

#include <stddef.h>
#include <sys/types.h>
#include <sys/util.h>
#include <sys/slist.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct emds_entry
 *
 * Information about entries used to store data in the emergency data storage.
 */
struct emds_entry {
	/** Unique id for each static and dynamic entry. */
	uint16_t id;
	/** Pointer to data that shall be stored. */
	uint8_t *data;
	/** Length of data that shall be stored. */
	size_t len;
}

/**
 * @struct emds_dynamic_entry
 *
 * Entries with a node element, used for dynamic entries. These are registered
 * using a call to emds_entry_add().
 */
struct emds_dynamic_entry {
	struct emds_entry entry;
	sys_snode_t node;
}

/**
 * @brief Define a static entry for emergency data storage items.
 *
 * @param name The entry name.
 * @param _id Unique id for entry. This value and not overlap with any other
 *            value.
 * @param _data Data pointer to be stored at emergency data store.
 * @param _len Length of data to be stored at emergency data store.
 *
 * This creates a variable _name prepended by emds_.
 */
#define EMDS_STATIC_ENTRY_DEFINE(_name, _id, _data, _len)                      \
	(const STRUCT_SECTION_ITERABLE(emds_entry, emds_##_name) = {           \
		.id = _id,                                                     \
		.data = (uint8_t *)_data,                                      \
		.len = _len,                                                   \
	})

/**
 * @brief Initialize the emergency data storage.
 *
 * Initializes the emergency data storage. This need to called before starting
 * to add entries to the emds_dynamic_entry and doing restore.
 *
 * @retval 0 Success
 * @retval -ERRNO errno code if error
 */
int emds_init(void);

/**
 *  @brief Add entry that should be saved/restored when emergency data storage is called.
 *
 * This will add the entry to the dynamic entry list, and will when the
 * 'emds_store' function is called take the given data pointer and store the
 * data to the emergency data storage.
 *
 * @param entry Entry to add to list.
 *
 * @retval 0 Success
 * @retval -ERRNO errno code if error
 */
int emds_entry_add(struct emds_dynamic_entry *entry);

/**
 * @brief Start the emergency data storage process.
 *
 * This will trigger the process of storing all data that has been regitster to
 * be stored. When it is finished with storing the data the data should not be
 * changed and the board should be halted or rebooted at the end. This is a time
 * critical process and will be processed as fast as possible. This will store
 * all data register either though the 'emds_entry_add' or 'EMDS_ENTRY_DEFINE'.
 *
 * @retval 0 Success
 * @retval -ERRNO errno code if error
 */
int emds_store(void);

/**
 * @brief Load all data from the emergency data storage.
 *
 * This need to be called after the static and dynamic entries are added, as
 * they are used to select data that should be loaded. This should also be
 * called before the 'emds_prepare' function is called as that will delete all
 * the stored data.
 *
 * @retval 0 Success
 * @retval -ERRNO errno code if error
 */
int emds_load(void);

/**
 * @brief Clear flash area from the emergency data storage.
 *
 * This function will clear the flash area for all previous stored data.
 *
 * @retval 0 Success
 * @retval -ERRNO errno code if error
 */
int emds_clear(void);

/**
 * @brief Prepare flash area for a next emergency data storage.
 *
 * This function will prepare the flash area for the next emergency data
 * storage. This delete the current entries if it enough space for the next
 * emergency data storage. It will clear the flash storage if it is not enough
 * space for the next storage. This has to be done after all the dynamic is
 * added.
 *
 * @retval 0 Success
 * @retval -ERRNO errno code if error
 */
int emds_prepare(void);

/**
 * @brief Calculate the time needed to store the registered data.
 *
 * This will calculate how much time it takes to store all registered data, both
 * dynamic and static data registered in the entries.
 *
 * @return Time in micro seconds that is needed to store all data.
 */
int emds_store_time_get(void);

/**
 * @brief Calculate the size needed to store the registered data.
 *
 * This will calculate how much size it takes to store all registered data, both
 * dynamic and static data registered in the entries.
 *
 * @return Byte size that is needed to store all data.
 */
int emds_store_size_get(void);

/**
 * @brief Check whether the store operation has finished.
 *
 * @return Whether the store operation is still running.
 */
bool emds_is_busy(void);

/**
 * @brief Calculate the amount of space left in the flash area.
 *
 * The flash area available should always be larger than the needed size for the
 * registered data. If not the flash needs to be cleared before emergency data
 * is stored.
 *
 * @return Bytes available in flash storage for emergency space.
 */
int emds_store_size_available(void);

#ifdef __cplusplus
}
#endif

#endif /* EMDS_H__ */

/** @} */
