/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <zephyr/types.h>
#include <stdbool.h>
#include <zephyr/ztest.h>
#include <downloader.h>
#include <fw_info.h>
#include <pm_config.h>
#include <fota_download.h>
#include "test_fota_download_common.h"
#include <fota_download_util.h>

/* fota_download_start may modify the resource locator passed to it, so we cannot directly pass
 * test constants. Instead, we copy them to a modifiable buffer first.
 */
static char buf[1024];

#define BASE_DOMAIN "something.com"
#define NO_TLS NULL
#define ARBITRARY_IMAGE_OFFSET 512

/* Stubs and mocks */
static const char *downloader_get_file;
static bool spm_s0_active_retval;

K_SEM_DEFINE(download_with_offset_sem, 0, 1);
static bool start_with_offset;
static bool fail_on_offset_get;
static bool fail_on_connect;
static bool fail_on_start;
static bool fail_on_proto;
static bool download_with_offset_success;
static downloader_callback_t downloader_event_handler;
K_SEM_DEFINE(stop_sem, 0, 1);

int dfu_target_init(int img_type, int img_num, size_t file_size, dfu_target_callback_t cb)
{
	return 0;
}

enum dfu_target_image_type dfu_target_img_type(const void *const buf, size_t len)
{
	return DFU_TARGET_IMAGE_TYPE_MCUBOOT;
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

int dfu_target_reset(void)
{
	return 0;
}

int dfu_target_schedule_update(int img_num)
{
	return 0;
}

int downloader_file_size_get(struct downloader *client, size_t *size)
{
	return 0;
}

int downloader_init(struct downloader *client, struct downloader_cfg *dl_cfg)
{
	downloader_event_handler = dl_cfg->callback;
	return 0;
}

enum dfu_target_image_type dfu_target_smp_img_type_check(const void *const buf, size_t len)
{
	return DFU_TARGET_IMAGE_TYPE_SMP;
}

int downloader_get_with_host_and_file(struct downloader *dl,
				      const struct downloader_host_cfg *dl_host_cfg,
				      const char *host, const char *file, size_t from)
{
	if (fail_on_proto == true) {
		return -EPROTONOSUPPORT;
	}
	if (fail_on_connect == true) {
		return -1;
	}

	downloader_get_file = file;

	if (fail_on_start == true) {
		return -1;
	}

	if (from == ARBITRARY_IMAGE_OFFSET) {
		download_with_offset_success = true;
		k_sem_give(&download_with_offset_sem);
	}

	return 0;
}

int downloader_cancel(struct downloader *client)
{
	const struct downloader_evt evt = {
		.id = DOWNLOADER_EVT_STOPPED,
	};

	downloader_event_handler(&evt);
	return 0;
}


int z_impl_zsock_inet_pton(net_sa_family_t family, const char *src, void *dst)
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

int tfm_platform_s0_active(uint32_t s0_address, uint32_t s1_address, bool *s0_active)
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
#include <zephyr/kernel.h>
#include <zephyr/drivers/flash.h>
#include <nrfx_nvmc.h>
#include <zephyr/device.h>

void set_s0_active(bool s0_active)
{
	int err;
	const struct device *fdev = DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller));
	struct fw_info s0_info = { .magic = { FIRMWARE_INFO_MAGIC } };
	struct fw_info s1_info = { .magic = { FIRMWARE_INFO_MAGIC } };

	spm_s0_active_retval = s0_active;

	s1_info.version = 1;
	if (s0_active) {
		s0_info.version = s1_info.version + 1;
	} else {
		s0_info.version = s1_info.version - 1;
	}

	zassert_true(device_is_ready(fdev), "Flash device not ready");

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
	printk("Event: %d\n", evt->id);
	switch (evt->id) {
	case FOTA_DOWNLOAD_EVT_ERROR:
		if (fail_on_proto) {
			zassert_equal(evt->cause, FOTA_DOWNLOAD_ERROR_CAUSE_PROTO_NOT_SUPPORTED);
			fail_on_proto = false;
			k_sem_give(&download_with_offset_sem);
		}
		if (fail_on_offset_get == true) {
			zassert_equal(evt->cause, FOTA_DOWNLOAD_ERROR_CAUSE_INTERNAL, NULL);
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
		k_sem_give(&stop_sem);
		break;

	case FOTA_DOWNLOAD_EVT_CANCELLED:
	case FOTA_DOWNLOAD_EVT_FINISHED:
		k_sem_give(&stop_sem);
		break;

	default:
		break;
	}
}


/**
 * @brief Reset mocks and initialize the fota_download library
 *
 * Call this before every test
 */
static void init(void)
{
	int err;

	start_with_offset = false;
	fail_on_offset_get = false;
	fail_on_connect = false;
	fail_on_start = false;
	fail_on_proto = false;
	downloader_get_file = NULL;
	spm_s0_active_retval = false;

	k_sem_reset(&stop_sem);
	k_sem_reset(&download_with_offset_sem);

	err = fota_download_init(client_callback);
	zassert_equal(err, 0, NULL);
}


/**
 * @brief Generic test of fota_download
 * Not to be called directly by the test suite. Performs a parameterized test of fota_download_start
 *
 * @param resource_locator - The resource locator to be fed to fota_download_start
 * @param expected_selection - The resource locator that fota_download_start is expected to select
 * @param s0_active - If true, pretend that s0 is active. Otherwise, pretend that s1 is active.
 */
static void test_fota_download_any_generic(const char * const resource_locator,
					     const char * const expected_selection, bool s0_active)
{
	int err;
	/* Init */
	init();

	/* Set the active slot */
	set_s0_active(s0_active);

	/* Start the download, check that it succeeds */
	strcpy(buf, resource_locator);
	err = fota_download_any(BASE_DOMAIN, buf, NO_TLS, 0, 0, 0);
	zassert_equal(err, 0, NULL);

	/* Verify that the correct resource was selected */
	zassert_true(strcmp(downloader_get_file, expected_selection) == 0, NULL);

	/* Verify that the download can be canceled with no isse */
	err = fota_download_cancel();
	zassert_equal(err, 0, NULL);

	k_sem_take(&stop_sem, K_FOREVER);
}


ZTEST_SUITE(fota_download_tests, NULL, NULL, NULL, NULL, NULL);

ZTEST(fota_download_tests, test_fota_download_cancel_before_init)
{
	/* Verify that calling fota_download_cancel without initializing results in an error. */
	int err = fota_download_cancel();

	zassert_equal(err, -EAGAIN);
}

ZTEST(fota_download_tests, test_download_single)
{
	test_fota_download_any_generic(S0_A, S0_A, S1_ACTIVE);
}

ZTEST(fota_download_tests, test_download_dual_s0_active)
{
	test_fota_download_any_generic(S0_B " " S1_B, S1_B, S0_ACTIVE);
}

ZTEST(fota_download_tests, test_download_dual_s1_active)
{
	test_fota_download_any_generic(S0_C " " S1_C, S0_C, S1_ACTIVE);
}

ZTEST(fota_download_tests, test_download_with_offset)
{
	int err;

	/* Init */
	init();

	/* Cause subsequent downloads to start with an offset */
	start_with_offset = true;

	uint8_t fragment_buf[1] = {0};
	size_t  fragment_len = 1;
	const struct downloader_evt evt = {
		.id = DOWNLOADER_EVT_FRAGMENT,
		.fragment = {
			.buf = fragment_buf,
			.len = fragment_len,
		}
	};

	/* Check that application is being notified when
	 * download_with_offset fails to get fw image offset
	 */
	err = fota_download_any(BASE_DOMAIN, buf, NO_TLS, 0, 0, 0);
	zassert_ok(err, NULL);

	err = downloader_event_handler(&evt);
	zassert_equal(err, -1, NULL);

	fail_on_offset_get = true;
	k_sem_take(&download_with_offset_sem, K_SECONDS(2));
	zassert_false(fail_on_offset_get, NULL);
	k_sem_take(&stop_sem, K_FOREVER);

	/* Expect the fota_download_cancel to fail as the download_with_offset() failed. */
	err = fota_download_cancel();
	zassert_equal(err, -EAGAIN);


	/* Check that application is being notified when
	 * download_with_offset fails to connect
	 */
	err = fota_download_any(BASE_DOMAIN, buf, NO_TLS, 0, 0, 0);
	zassert_ok(err, NULL);

	err = downloader_event_handler(&evt);
	zassert_equal(err, -1, NULL);

	fail_on_connect = true;
	k_sem_take(&download_with_offset_sem, K_SECONDS(2));
	zassert_false(fail_on_connect, NULL);

	/* Expect the fota_download_cancel to fail as the download_with_offset() failed. */
	err = fota_download_cancel();
	zassert_equal(err, -EAGAIN);


	/* Check that application is being notified when
	 * download_with_offset fails to start download
	 */
	err = fota_download_any(BASE_DOMAIN, buf, NO_TLS, 0, 0, 0);
	zassert_ok(err, NULL);

	err = downloader_event_handler(&evt);
	zassert_equal(err, -1, NULL);

	fail_on_start = true;
	k_sem_take(&download_with_offset_sem, K_SECONDS(2));
	zassert_false(fail_on_start, NULL);

	/* Expect the fota_download_cancel to fail as the download_with_offset() failed. */
	err = fota_download_cancel();
	zassert_equal(err, -EAGAIN);


	/* Successfully restart download with offset */
	err = fota_download_any(BASE_DOMAIN, buf, NO_TLS, 0, 0, 0);
	zassert_ok(err, NULL);

	err = downloader_event_handler(&evt);
	zassert_equal(err, -1, NULL);

	download_with_offset_success = false;
	k_sem_take(&download_with_offset_sem, K_SECONDS(2));
	zassert_true(download_with_offset_success, NULL);

	err = fota_download_cancel();
	zassert_ok(err, NULL);

	/* Try cancelling again and expect error. */
	err = fota_download_cancel();
	zassert_equal(err, -EAGAIN);
}


ZTEST(fota_download_tests, test_download_invalid_protocol)
{
	init();

	fail_on_proto = true;
	int err = fota_download_any("ftp://" BASE_DOMAIN, buf, NO_TLS, 0, 0, 0);

	zassert_equal(err, -EPROTONOSUPPORT, "%d", err);
	fota_download_cancel();

}

ZTEST_SUITE(fota_download_util, NULL, NULL, NULL, NULL, NULL);

ZTEST(fota_download_util, test_download_util_invalid_protocol)
{
	init();

	fail_on_proto = true;
	int err = fota_download_util_download_start("ftp://host.com/file.bin",
				      DFU_TARGET_IMAGE_TYPE_ANY, 0,
				      client_callback);

	zassert_ok(err, NULL);
	k_sem_take(&download_with_offset_sem, K_SECONDS(2));
	k_sem_take(&stop_sem, K_FOREVER);
}

ZTEST(fota_download_util, test_download_util_start)
{
	init();

	uint8_t fragment_buf[1] = {0};
	size_t  fragment_len = 1;
	struct downloader_evt evt = {
		.id = DOWNLOADER_EVT_FRAGMENT,
		.fragment = {
			.buf = fragment_buf,
			.len = fragment_len,
		}
	};

	start_with_offset = true;
	int err = fota_download_util_download_start("http://host.com/file.bin",
				      DFU_TARGET_IMAGE_TYPE_ANY, 0,
				      client_callback);

	zassert_ok(err);

	err = downloader_event_handler(&evt);
	zassert_ok(err);

	evt.id = DOWNLOADER_EVT_DONE;
	err = downloader_event_handler(&evt);
	zassert_ok(err);

	k_sem_take(&stop_sem, K_FOREVER);
}
