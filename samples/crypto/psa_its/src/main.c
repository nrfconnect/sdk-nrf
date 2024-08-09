/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <psa/crypto.h>
#include <psa/internal_trusted_storage.h>
#include <zephyr/logging/log.h>

#include "trusted_storage_init.h"

#define APP_SUCCESS	    (0)
#define APP_ERROR	    (-1)
#define APP_SUCCESS_MESSAGE "Example finished successfully!"

LOG_MODULE_REGISTER(psa_its, LOG_LEVEL_DBG);

int write_wrong_uid(void)
{
	psa_status_t status;

	LOG_INF("Verify that reading the wrong UID will fail");

	psa_storage_uid_t non_existent_uid = 0x5EB01234;

	/* Read from the start of the entry */
	uint32_t data_offset = 0;

	/* Read 4 bytes */
	uint32_t data_length = 4;

	/* Write the result from psa_its_get to p_data */
	uint8_t p_data[4];

	/* Number of bytes written to p_data */
	size_t p_data_length = 0x5EB0;

	status = psa_its_get(non_existent_uid, data_offset, data_length, p_data, &p_data_length);

	if (status == 0) {
		LOG_ERR("Expected psa_its_get with the wrong UID to fail, but it "
			"succeeded.");
		return APP_ERROR;
	}

	LOG_INF("psa_its_get correctly returned an error code when we intentionally read "
		"the wrong UID");

	return APP_SUCCESS;
}

int write_and_read(void)
{
	psa_status_t status;

	LOG_INF("Write to ITS and then check that we can read it back");

	psa_storage_uid_t new_uid = 0x5EB00000;

	/* Data to be written to ITS */
	uint8_t p_data_write[1] = {42};

	/* permission flags for ITS entry */
	psa_storage_create_flags_t create_flags = PSA_STORAGE_FLAG_NONE;

	status = psa_its_set(new_uid, sizeof(p_data_write), p_data_write, create_flags);

	if (status) {
		LOG_ERR("Unable to write to ITS");
		LOG_ERR("psa_its_get returned psa_status_t: %d", status);
		return APP_ERROR;
	}

	/* Write the 'new_uid' entry in ITS to this buffer */
	uint8_t p_data_read[1];

	/* Read from the start of the entry */
	uint32_t data_offset = 0;

	/* Read 1 byte */
	uint32_t data_length = 1;

	/* Number of bytes written to p_data */
	size_t p_data_length = 0xc0ffe;

	status = psa_its_get(new_uid, data_offset, data_length, p_data_read, &p_data_length);
	if (status) {
		LOG_ERR("Unable to read back from ITS");
		LOG_ERR("psa_its_get returned psa_status_t: %d", status);
		return APP_ERROR;
	}

	if (p_data_length != 1) {
		LOG_ERR("Unable to read the correct amount of bytes from ITS");
	}

	if (p_data_read[0] != 42) {
		LOG_ERR("Read the wrong value back from ITS");
	}

	LOG_INF("Successfully wrote to ITS and read back what was written");

	return APP_SUCCESS;
}

int main(void)
{
	LOG_INF("Starting PSA ITS example...");

#ifdef CONFIG_TRUSTED_STORAGE_BACKEND_AEAD_KEY_DERIVE_FROM_HUK
	write_huk();
#endif /* CONFIG_TRUSTED_STORAGE_BACKEND_AEAD_KEY_DERIVE_FROM_HUK */

	psa_status_t status = psa_crypto_init();

	if (status) {
		LOG_INF("psa_crypto_init failed");
		return 1;
	}

	status = write_wrong_uid();
	if (status) {
		return APP_ERROR;
	}

	status = write_and_read();
	if (status) {
		return APP_ERROR;
	}

	LOG_INF(APP_SUCCESS_MESSAGE);

	return APP_SUCCESS;
}
