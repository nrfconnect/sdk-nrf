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

#define ARBITRARY_IMAGE_OFFSET 512

/* Stubs and mocks */
bool dfu_ctx_mcuboot_set_b1_file__s0_active;
const char *download_client_start_file;
char *dfu_ctx_mcuboot_set_b1_file__update;
static bool spm_s0_active_retval;

K_SEM_DEFINE(download_with_offset_sem, 0, 1);
static bool start_with_offset;
static bool fail_on_offset_get;
static bool fail_on_connect;
static bool fail_on_start;
static bool download_with_offset_success;
static download_client_callback_t download_client_event_handler;

int dfu_target_init(int img_type, size_t file_size, dfu_target_callback_t cb)
{
	return 0;
}

int dfu_target_img_type(const void *const buf, size_t len)
{
	return 0;
}

int dfu_target_offset_get(size_t *offset)
{
	if (fail_on_offset_get == true) {
		return -1;
	}

	if (start_with_offset == true) {
		*offset = ARBITRARY_IMAGE_OFFSET;
	} else {
		*offset = 0;
	}

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

	if (fail_on_start == true) {
		return -1;
	}

	if (from == ARBITRARY_IMAGE_OFFSET) {
		download_with_offset_success = true;
		k_sem_give(&download_with_offset_sem);
	}

	return 0;
}

int download_client_file_size_get(struct download_client *client, size_t *size)
{
	return 0;
}

int download_client_init(struct download_client *client,
			 download_client_callback_t callback)
{
	download_client_event_handler = callback;
	return 0;
}

int download_client_connect(struct download_client *client, const char *host,
			    const struct download_client_cfg *config)
{
	if (fail_on_connect == true) {
		return -1;
	}

	return 0;
}

int dfu_ctx_mcuboot_set_b1_file(char *file, bool s0_active, char **update)
{
	dfu_ctx_mcuboot_set_b1_file__s0_active = s0_active;
	*update = dfu_ctx_mcuboot_set_b1_file__update;
	return 0;
}

int z_impl_zsock_inet_pton(sa_family_t family, const char *src, void *dst)
{
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
	switch (evt->id) {
	case FOTA_DOWNLOAD_EVT_ERROR:
		if (fail_on_offset_get == true) {
			zassert_equal(evt->cause, FOTA_DOWNLOAD_ERROR_CAUSE_DOWNLOAD_FAILED, NULL);
			fail_on_offset_get = false;
			k_sem_give(&download_with_offset_sem);
		}
		if (fail_on_connect == true) {
			zassert_equal(evt->cause, FOTA_DOWNLOAD_ERROR_CAUSE_DOWNLOAD_FAILED, NULL);
			fail_on_connect = false;
			k_sem_give(&download_with_offset_sem);
		}
		if (fail_on_start == true) {
			zassert_equal(evt->cause, FOTA_DOWNLOAD_ERROR_CAUSE_DOWNLOAD_FAILED, NULL);
			fail_on_start = false;
			k_sem_give(&download_with_offset_sem);
		}
		break;

	default:
		break;
	}
}

static void init(void)
{
	int err;

	k_sem_init(&download_with_offset_sem, 0, 1);
	start_with_offset = false;
	fail_on_offset_get = false;
	fail_on_connect = false;
	fail_on_start = false;

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
	err = fota_download_start("something.com", buf, NO_TLS, 0, 0);
	zassert_equal(err, 0, NULL);
	zassert_equal(dfu_ctx_mcuboot_set_b1_file__s0_active,
		      spm_s0_active_retval, "Incorrect param for s0_active");

	err = fota_download_cancel();
	zassert_equal(err, 0, NULL);
	set_s0_active(true);
	err = fota_download_start("something.com", buf, NO_TLS, 0, 0);
	zassert_equal(err, 0, NULL);
	zassert_equal(dfu_ctx_mcuboot_set_b1_file__s0_active,
		      spm_s0_active_retval, "Incorrect param for s0_active");
	err = fota_download_cancel();
	zassert_equal(err, 0, NULL);

	/* Next, verify that the update path given by mcuboot_set_b1_file
	 * is used correctly.
	 */

	/* update set to null indicates to use original file param */
	dfu_ctx_mcuboot_set_b1_file__update = NULL;
	strcpy(buf, S0_S1);
	err = fota_download_start("something.com", buf, NO_TLS, 0, 0);
	zassert_equal(err, 0, NULL);
	zassert_true(strcmp(download_client_start_file, S0_S1) == 0, NULL);
	err = fota_download_cancel();
	zassert_equal(err, 0, NULL);

	/* update set to not null indicates to use update for file param */
	dfu_ctx_mcuboot_set_b1_file__update = S1;
	strcpy(buf, S0_S1);
	err = fota_download_start("something.com", buf, NO_TLS, 0, 0);
	zassert_equal(err, 0, NULL);
	zassert_true(strcmp(download_client_start_file, S1) == 0, NULL);

	/* Check if double call returns EALREADY */
	err = fota_download_start("something.com", buf, NO_TLS, 0, 0);
	zassert_equal(err, -EALREADY, "No failure for double call");

	err = fota_download_cancel();
	zassert_equal(err, 0, NULL);
}

static void test_download_with_offset(void)
{
	int err;

	uint8_t fragment_buf[1] = {0};
	size_t  fragment_len = 1;
	const struct download_client_evt evt = {
		.id = DOWNLOAD_CLIENT_EVT_FRAGMENT,
		.fragment = {
			.buf = fragment_buf,
			.len = fragment_len,
		}
	};

	start_with_offset = true;


	/* Check that application is being notified when
	 * download_with_offset fails to get fw image offset
	 */
	err = fota_download_start("something.com", buf, NO_TLS, 0, 0);
	zassert_ok(err, NULL);

	err = download_client_event_handler(&evt);
	zassert_equal(err, -1, NULL);

	fail_on_offset_get = true;
	k_sem_take(&download_with_offset_sem, K_SECONDS(2));
	zassert_false(fail_on_offset_get, NULL);

	err = fota_download_cancel();
	zassert_ok(err, NULL);


	/* Check that application is being notified when
	 * download_with_offset fails to connect
	 */
	err = fota_download_start("something.com", buf, NO_TLS, 0, 0);
	zassert_ok(err, NULL);

	err = download_client_event_handler(&evt);
	zassert_equal(err, -1, NULL);

	fail_on_connect = true;
	k_sem_take(&download_with_offset_sem, K_SECONDS(2));
	zassert_false(fail_on_connect, NULL);

	err = fota_download_cancel();
	zassert_ok(err, NULL);


	/* Check that application is being notified when
	 * download_with_offset fails to start download
	 */
	err = fota_download_start("something.com", buf, NO_TLS, 0, 0);
	zassert_ok(err, NULL);

	err = download_client_event_handler(&evt);
	zassert_equal(err, -1, NULL);

	fail_on_start = true;
	k_sem_take(&download_with_offset_sem, K_SECONDS(2));
	zassert_false(fail_on_start, NULL);

	err = fota_download_cancel();
	zassert_ok(err, NULL);


	/* Successfully restart download with offset */
	err = fota_download_start("something.com", buf, NO_TLS, 0, 0);
	zassert_ok(err, NULL);

	err = download_client_event_handler(&evt);
	zassert_equal(err, -1, NULL);

	download_with_offset_success = false;
	k_sem_take(&download_with_offset_sem, K_SECONDS(2));
	zassert_true(download_with_offset_success, NULL);

	err = fota_download_cancel();
	zassert_ok(err, NULL);


	start_with_offset = false;
}

void test_main(void)
{
	ztest_test_suite(lib_fota_download_test,
			 ztest_unit_test(test_fota_download_start),
			 ztest_unit_test(test_download_with_offset));

	ztest_run_test_suite(lib_fota_download_test);
}
