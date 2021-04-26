/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <string.h>
#include <zephyr/types.h>
#include <stdbool.h>
#include <ztest.h>
#include <download_client.h>
#include <fw_info.h>
#include <pm_config.h>
#include <fota_download.h>

/* Create buffer which we will fill with strings to test with.
 * This is needed since 'dfu_ctx_Mcuboot_set_b1_file` will modify its
 * 'file' parameter, hence we can not provide it raw c strings as they
 * are stored in read-only memory
 */
char buf[1024];

#define S0_S1 "s0 s1"
#define S0 "s0"
#define S1 "s1"
#define NO_SPACE "s0s1"
#define NO_TLS -1
#define DEFAULT_APN NULL

/* Stubs and mocks */
bool dfu_ctx_mcuboot_set_b1_file__s0_active;
const char *download_client_start_file;
char *dfu_ctx_mcuboot_set_b1_file__update;
static bool spm_s0_active_retval;

int dfu_target_init(int img_type, size_t file_size)
{
	return 0;
}

int dfu_target_img_type(const void *const buf, size_t len)
{
	return 0;
}

int dfu_target_offset_get(size_t *offset)
{
	return 0;
}

int dfu_target_write(const void *const buf, size_t len)
{
	return 0;
}

int dfu_target_done(bool successful)
{
	return 0;
}

int download_client_disconnect(struct download_client *client)
{
	return 0;
}

int dfu_target_reset(void)
{
	return 0;
}

int download_client_start(struct download_client *client, const char *file,
			  size_t from)
{
	download_client_start_file = file;
	return 0;
}

int download_client_file_size_get(struct download_client *client, size_t *size)
{
	return 0;
}

int download_client_init(struct download_client *client,
			 download_client_callback_t callback)
{
	return 0;
}

int download_client_connect(struct download_client *client, const char *host,
			    const struct download_client_cfg *config)
{
	return 0;
}

int dfu_ctx_mcuboot_set_b1_file(char *file, bool s0_active, char **update)
{
	dfu_ctx_mcuboot_set_b1_file__s0_active = s0_active;
	*update = dfu_ctx_mcuboot_set_b1_file__update;
	return 0;
}

/* END stubs and mocks */

#ifdef CONFIG_TRUSTED_EXECUTION_NONSECURE

int spm_s0_active(uint32_t s0_address, uint32_t s1_address, bool *s0_active)
{
	*s0_active = spm_s0_active_retval;

	return 0;
}

void set_s0_active(bool s0_active)
{
	spm_s0_active_retval = s0_active;
}
#else /* ! CONFIG_TRUSTED_EXECUTION_NONSECURE */

/* The test is _NOT_ executed as non-secure. Hence 'spm_s0_active' will not
 * be called. This means that we cannot create a mock of that function to
 * modify whether or not S0 should be considered the active B1 slot.
 *
 * To solve this issue, we perform flash write operations to the locations
 * where 'fw_info.h' will look for the S0 and S1 metadata. This allows
 * us to dictate what B1 slot should be considered active
 */
#include <zephyr.h>
#include <drivers/flash.h>
#include <nrfx_nvmc.h>
#include <device.h>

#define FLASH_NAME DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL

static const struct device *fdev;
void set_s0_active(bool s0_active)
{
	int err;
	struct fw_info s0_info = { .magic = { FIRMWARE_INFO_MAGIC } };
	struct fw_info s1_info = { .magic = { FIRMWARE_INFO_MAGIC } };

	spm_s0_active_retval = s0_active;

	s1_info.version = 1;
	if (s0_active) {
		s0_info.version = s1_info.version + 1;
	} else {
		s0_info.version = s1_info.version - 1;
	}

	fdev = device_get_binding(FLASH_NAME);
	zassert_not_equal(fdev, 0, "Unable to get fdev");

	err = flash_erase(fdev, PM_S0_ADDRESS, nrfx_nvmc_flash_page_size_get());
	zassert_equal(err, 0, "flash_erase failed");

	err = flash_erase(fdev, PM_S1_ADDRESS, nrfx_nvmc_flash_page_size_get());
	zassert_equal(err, 0, "flash_erase failed");

	err = flash_write(fdev, PM_S0_ADDRESS, (void *)&s0_info,
			  sizeof(s0_info));
	zassert_equal(err, 0, "Unable to write to flash");

	err = flash_write(fdev, PM_S1_ADDRESS, (void *)&s1_info,
			  sizeof(s1_info));
	zassert_equal(err, 0, "Unable to write to flash");
}
#endif

void client_callback(const struct fota_download_evt *evt)
{
}

static void init(void)
{
	int err;

	err = fota_download_init(client_callback);
	zassert_equal(err, 0, NULL);
}

static void test_fota_download_start(void)
{
	int err;

	init();

	/* Verify that correct argument for s0_active is given */
	set_s0_active(false);
	strcpy(buf, S0_S1);
	err = fota_download_start("something.com", buf, NO_TLS, DEFAULT_APN, 0);
	zassert_equal(err, 0, NULL);
	zassert_equal(dfu_ctx_mcuboot_set_b1_file__s0_active,
		      spm_s0_active_retval, "Incorrect param for s0_active");

	set_s0_active(true);
	err = fota_download_start("something.com", buf, NO_TLS, DEFAULT_APN, 0);
	zassert_equal(err, 0, NULL);
	zassert_equal(dfu_ctx_mcuboot_set_b1_file__s0_active,
		      spm_s0_active_retval, "Incorrect param for s0_active");

	/* Next, verify that the update path given by mcuboot_set_b1_file
	 * is used correctly.
	 */

	/* update set to null indicates to use original file param */
	dfu_ctx_mcuboot_set_b1_file__update = NULL;
	strcpy(buf, S0_S1);
	err = fota_download_start("something.com", buf, NO_TLS, DEFAULT_APN, 0);
	zassert_equal(err, 0, NULL);
	zassert_true(strcmp(download_client_start_file, S0_S1) == 0, NULL);

	/* update set to not null indicates to use update for file param */
	dfu_ctx_mcuboot_set_b1_file__update = S1;
	strcpy(buf, S0_S1);
	err = fota_download_start("something.com", buf, NO_TLS, DEFAULT_APN, 0);
	zassert_equal(err, 0, NULL);
	zassert_true(strcmp(download_client_start_file, S1) == 0, NULL);
}

void test_main(void)
{
	ztest_test_suite(lib_fota_download_test,
			 ztest_unit_test(test_fota_download_start));

	ztest_run_test_suite(lib_fota_download_test);
}
