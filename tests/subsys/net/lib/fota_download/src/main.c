/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
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

/* Stubs and mocks */
bool dfu_ctx_mcuboot_set_b1_file__s0_active;
static u32_t s0_version;
static u32_t s1_version;
const char *download_client_start_file;
char *dfu_ctx_mcuboot_set_b1_file__update;

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

int spm_firmware_info(u32_t fw_address, struct fw_info *info)
{
	zassert_true(info != NULL, NULL);

	if (fw_address == PM_S0_ADDRESS) {
		info->version = s0_version;
	} else if (fw_address == PM_S1_ADDRESS) {
		info->version = s1_version;
	} else {
		zassert_true(false, "Unexpected address");
	}

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

void client_callback(enum fota_download_evt_id evt_id) { }

static void init(void)
{
	int err;

	err = fota_download_init(client_callback);
	zassert_equal(err, 0, NULL);
}

static void test_fota_download_start(void)
{
	int err;
	bool expect_s0_active = false;

	init();

	/* Verify that correct argument for s0_active is given */
	s0_version = 0;
	s1_version = 1;
	expect_s0_active = false;
	strcpy(buf, S0_S1);
	err = fota_download_start("something.com", buf);
	zassert_equal(err, 0, NULL);
	zassert_equal(dfu_ctx_mcuboot_set_b1_file__s0_active, expect_s0_active,
		      "Incorrect param for s0_active");

	s0_version = 2;
	s1_version = 1;
	expect_s0_active = true;
	err = fota_download_start("something.com", buf);
	zassert_equal(err, 0, NULL);
	zassert_equal(dfu_ctx_mcuboot_set_b1_file__s0_active, expect_s0_active,
		      "Incorrect param for s0_active");


	/* Next, verify that the update path given by mcuboot_set_b1_file
	 * is used correctly.
	 */

	/* update set to null indicates to use original file param */
	dfu_ctx_mcuboot_set_b1_file__update = NULL;
	strcpy(buf, S0_S1);
	err = fota_download_start("something.com", buf);
	zassert_equal(err, 0, NULL);
	zassert_true(strcmp(download_client_start_file, S0_S1) == 0, NULL);

	/* update set to not null indicates to use update for file param */
	dfu_ctx_mcuboot_set_b1_file__update = S1;
	strcpy(buf, S0_S1);
	err = fota_download_start("something.com", buf);
	zassert_equal(err, 0, NULL);
	zassert_true(strcmp(download_client_start_file, S1) == 0, NULL);
}

void test_main(void)
{
	ztest_test_suite(lib_fota_download_test,
	     ztest_unit_test(test_fota_download_start)
	 );

	ztest_run_test_suite(lib_fota_download_test);
}
