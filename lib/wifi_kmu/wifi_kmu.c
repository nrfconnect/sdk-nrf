/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <errno.h>
#include <nrfx.h>
#include <nrfx_kmu.h>
#include <nrfx_mramc.h>
#include <zephyr/sys/util.h>

#include "wifi_kmu.h"

#if defined(CONFIG_BUILD_WITH_TFM)
#include <ioctl.h>
#endif

/* Metadata version that doesn't collide with the one used by PSA crypto keys */
#define WIFI_KEY_METADATA_VERSION (15)

/* memset/memcpy friendly size of KMU slots on the platform */
#define KMU_KEYSLOTBYTES  (KMU_KEYSLOTBITS / 8)

/* Reserved key slot in KMU */
#define KMU_RESERVED_SLOT_MIN (180)

/* SICR page for KMU on nRF71 series devices */
#define MRAM_CONFIGNVR_SICR_PAGE 3

/** @brief Union to punt KMU_push metadata to integer value
 *
 * This type holds the metadata for KMU push driver and provides a direct
 * conversion via punting to an unsigned integer type.
 */
typedef union {
	uint32_t value;
	struct {
		uint32_t metadata_version: 4;	/**!< Metadata in the same location as CRACEN KMU metadata. */
		uint32_t num_slots: 8;		/**!< Size in number of KMU slots of this key. */
		uint32_t num_slots_left: 8;	/**!< Number of KMU slots before end of key. */
		uint32_t reserved: 12;		/**!< Reserved (unused) */
	};
} wifi_key_metadata_t;


static int provision_slots(uint32_t slot_id, size_t num_slots, uint32_t target_address,
			   const uint8_t *key_buffer, size_t key_size)
{
	int err = -EFAULT;
	nrfx_kmu_key_slot_data_t slot_data = {0};
	wifi_key_metadata_t key_metadata = {0};
	size_t slots_left = num_slots;
	size_t block_size = 0;
	size_t size_left = key_size;
	const uint8_t *cur_src;

	key_metadata.metadata_version = WIFI_KEY_METADATA_VERSION;
	key_metadata.num_slots = num_slots;
	slot_data.revoke_policy = NRFX_KMU_RPOLICY_ROTATING;

	for (size_t i = slot_id; i < (slot_id + num_slots); i++) {
		slots_left -= 1;
		key_metadata.num_slots_left = slots_left;

		block_size = size_left > KMU_KEYSLOTBYTES ?
			KMU_KEYSLOTBYTES : size_left;

		cur_src = key_buffer + ((i - slot_id) * KMU_KEYSLOTBYTES);

		slot_data.metadata.metadata = key_metadata.value;
		slot_data.keyslot_dest = target_address + ((i - slot_id) * KMU_KEYSLOTBYTES);
		memset(slot_data.keyslot_value, 0, KMU_KEYSLOTBYTES);
		memcpy(slot_data.keyslot_value, cur_src, block_size);

		err = nrfx_kmu_key_slot_provision(&slot_data, i);
		if (err != 0) {
			return err;
		}

		size_left -= block_size;
	}

	return 0;
}

static int push_slots(uint32_t slot_id, size_t num_slots)
{
	int err = -EFAULT;
	nrfx_kmu_key_slot_metadata_t metadata;
	wifi_key_metadata_t key_metadata;

	/* Verify that all slots are populated with correct info */
	for (size_t i = slot_id; i < (slot_id + num_slots); i++) {
		err = nrfx_kmu_key_slot_metadata_read(i, &metadata);
		if (err != 0) {
			return err;
		}

		key_metadata.value = metadata.metadata;
		if (key_metadata.metadata_version != WIFI_KEY_METADATA_VERSION ||
		    key_metadata.num_slots != num_slots ||
		    key_metadata.num_slots_left != num_slots - 1 - (i - slot_id)) {
			return -EFAULT;
		}
	}

	/* Push all slots */
	err = nrfx_kmu_key_slots_push(slot_id, num_slots);
	return err;
}

static int erase_slots(uint32_t slot_id, size_t num_slots)
{
	return nrfx_kmu_key_slots_revoke(slot_id, num_slots);
}

int wifi_kmu_write_key(uint32_t slot_id, uint32_t target_addr,
		       const uint8_t *key_buffer, size_t key_size)
{
	int err = -EINVAL;
	size_t num_slots = DIV_ROUND_UP(key_size, KMU_KEYSLOTBYTES);

	if (slot_id < CONFIG_NRF_WIFI_KMU_SLOT_MIN ||
		slot_id >= CONFIG_NRF_WIFI_KMU_SLOT_MIN + CONFIG_NRF_WIFI_KMU_NUM_SLOTS) {
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BUILD_WITH_TFM)) {
		/* WIFI KMU keys are transferred with IOCTL call */

	} else {
		while (!nrfx_mramc_ready_check()) {
			/* Wait until MRAMC is ready for the next operation */
		}

		/* Enable write and erase from KMU to SICR in MRAM */
		nrfx_mramc_confignvr_perm_set(true, MRAM_CONFIGNVR_SICR_PAGE);

		err = provision_slots(slot_id, num_slots, target_addr, key_buffer, key_size);
		if (err != 0) {
			goto error;
		}

		err = push_slots(slot_id, num_slots);
		if (err != 0) {
			goto error;
		}

		err = erase_slots(slot_id, num_slots);
		if (err != 0) {
			goto error;
		}

		nrfx_mramc_confignvr_perm_set(false, MRAM_CONFIGNVR_SICR_PAGE);
	}

	return err;
error:

	if (!IS_ENABLED(CONFIG_BUILD_WITH_TFM)) {
		/* Clean out any partial keys, swallowing result */
		(void) erase_slots(slot_id, num_slots);

		/* Disable write and erase from KMU to SICR in MRAM */
		nrfx_mramc_confignvr_perm_set(false, MRAM_CONFIGNVR_SICR_PAGE);
	}

	return err;
}

int wifi_kmu_erase_keys(void)
{
	int err = -EFAULT;
	bool is_empty;
	/* Check if any of the keys are populated */

	if (IS_ENABLED(CONFIG_BUILD_WITH_TFM)) {
		/* WIFI KMU key are transferred with IOCTL call */

	} else {
		err = nrfx_kmu_key_slots_empty_check(CONFIG_NRF_WIFI_KMU_SLOT_MIN,
						     CONFIG_NRF_WIFI_KMU_NUM_SLOTS,
						     &is_empty);
		if (err != 0) {
			return err;
		}

		if (is_empty) {
			return 0;
		}

		while (!nrfx_mramc_ready_check()) {
			/* Wait until MRAMC is ready for the next operation */
		}

		nrfx_mramc_confignvr_perm_set(true, MRAM_CONFIGNVR_SICR_PAGE);

		err = erase_slots(CONFIG_NRF_WIFI_KMU_SLOT_MIN, CONFIG_NRF_WIFI_KMU_NUM_SLOTS);

		nrfx_mramc_confignvr_perm_set(false, MRAM_CONFIGNVR_SICR_PAGE);

	}

	return err;
}
