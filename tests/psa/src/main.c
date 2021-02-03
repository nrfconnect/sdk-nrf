/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <ztest.h>
#include <limits.h>

#include <psa/internal_trusted_storage.h>

uint64_t data;
uint32_t data_length = sizeof(data);
uint32_t no_offset;
uint32_t small_data;
size_t out_data_length;
psa_status_t status;
psa_storage_uid_t uid;
psa_storage_uid_t not_existing_uid = 2;
struct psa_storage_info_t info;

void test_set_invalid_data_length_return_INVALID_ARGUMENT(void)
{
	uint32_t invalid_data_length = 0;

	status = psa_its_set(uid, invalid_data_length, &data,
			     PSA_STORAGE_FLAG_NONE);

	zassert_equal(PSA_ERROR_INVALID_ARGUMENT, status,
			"Unexpected status: %d", status);
}

void test_set_invalid_p_data_return_INVALID_ARGUMENT(void)
{
	const void *p_data = NULL;

	status = psa_its_set(uid, data_length, p_data, PSA_STORAGE_FLAG_NONE);

	zassert_equal(PSA_ERROR_INVALID_ARGUMENT, status,
			"Unexpected status: %d", status);
}

void test_set_invalid_flag_return_NOT_SUPPORTED(void)
{
	psa_storage_create_flags_t not_supported_flag = (1 << 31);

	status = psa_its_set(uid, data_length, &data, not_supported_flag);

	zassert_equal(PSA_ERROR_NOT_SUPPORTED, status,
			"Unexpected status: %d", status);
}

void test_set_exceeding_flash_return_STORAGE_FAILURE(void)
{
	status = psa_its_set(uid, INT_MAX, &data, PSA_STORAGE_FLAG_NONE);

	zassert_equal(PSA_ERROR_STORAGE_FAILURE, status,
			"Unexpected status: %d", status);
}

void test_set_correctly_return_SUCCESS(void)
{
	status = psa_its_set(uid, sizeof(data), &data, PSA_STORAGE_FLAG_NONE);

	zassert_equal(PSA_SUCCESS, status,
			"Unexpected status: %d", status);
}

void test_set_override_flag_write_once_return_NOT_PERMITTED(void)
{
	psa_storage_uid_t uid = 1;

	status = psa_its_set(uid, sizeof(data), &data,
			     PSA_STORAGE_FLAG_WRITE_ONCE);

	zassert_equal(PSA_SUCCESS, status,
			"Unexpected status: %d", status);

	status = psa_its_set(uid, sizeof(data), &data,
			     PSA_STORAGE_FLAG_WRITE_ONCE);

	zassert_equal(PSA_ERROR_NOT_PERMITTED, status,
			"Unexpected status: %d", status);
}

void test_get_invalid_data_length_return_INVALID_ARGUMENT(void)
{
	uint32_t invalid_data_length = 0;

	status = psa_its_get(uid, no_offset, invalid_data_length, &data,
			     &out_data_length);

	zassert_equal(PSA_ERROR_INVALID_ARGUMENT, status,
			"Unexpected status: %d", status);
}

void test_get_invalid_p_data_return_INVALID_ARGUMENT(void)
{
	void *p_data = NULL;

	status = psa_its_get(uid, no_offset, sizeof(data), p_data,
			     &out_data_length);

	zassert_equal(PSA_ERROR_INVALID_ARGUMENT, status,
			"Unexpected status: %d", status);
}

void test_get_not_existing_uid_return_DOES_NOT_EXIST(void)
{
	status = psa_its_get(not_existing_uid, no_offset, sizeof(data), &data,
			     &out_data_length);

	zassert_equal(PSA_ERROR_DOES_NOT_EXIST, status,
			"Unexpected status: %d", status);
}

void test_get_too_small_buff_return_BUFFER_TOO_SMALL(void)
{
	status = psa_its_set(uid, sizeof(data), &data, PSA_STORAGE_FLAG_NONE);
	zassert_equal(PSA_SUCCESS, status,
			"Unexpected status: %d", status);

	status = psa_its_get(uid, no_offset, sizeof(small_data), &small_data,
			     &out_data_length);

	zassert_equal(PSA_ERROR_BUFFER_TOO_SMALL, status,
			"Unexpected status: %d", status);
}

void test_get_correctly_return_SUCCESS(void)
{
	const uint64_t set_data = 0x0807060504030201;

	status = psa_its_set(uid, sizeof(set_data), &set_data,
			     PSA_STORAGE_FLAG_NONE);
	zassert_equal(PSA_SUCCESS, status,
			"Unexpected status: %d", status);

	status = psa_its_get(uid, no_offset, sizeof(data), &data,
			     &out_data_length);

	zassert_equal(PSA_SUCCESS, status,
			"Unexpected status: %d", status);

	zassert_equal(out_data_length, sizeof(data),
		      "Unexpected amount of data returned");

	zassert_equal(data, set_data, "Unexpected data returned");
}

void test_get_correct_data_offset_return_SUCCESS(void)
{
	const uint64_t set_data = 0x0807060504030201;
	uint32_t data_offset = sizeof(set_data) / 2;

	status = psa_its_set(uid, sizeof(set_data), &set_data,
			     PSA_STORAGE_FLAG_NONE);
	zassert_equal(PSA_SUCCESS, status,
			"Unexpected status: %d", status);

	status = psa_its_get(uid, data_offset, sizeof(small_data), &small_data,
			     &out_data_length);

	zassert_equal(PSA_SUCCESS, status,
			"Unexpected status: %d", status);
	zassert_equal(out_data_length, sizeof(small_data),
		      "Unexpected amount of data returned");
	zassert_equal(small_data, 0x08070605, "Unexpected data returned");
}

void test_get_invalid_data_offset_return_INVALID_ARGUMENT(void)
{
	const uint64_t set_data = 0x0807060504030201;
	uint32_t invalid_data_offset = sizeof(set_data);

	status = psa_its_set(uid, sizeof(set_data), &set_data,
			     PSA_STORAGE_FLAG_NONE);
	zassert_equal(PSA_SUCCESS, status,
			"Unexpected status: %d", status);

	status = psa_its_get(uid, invalid_data_offset, sizeof(data), &data,
			     &out_data_length);

	zassert_equal(PSA_ERROR_INVALID_ARGUMENT, status,
			"Unexpected status: %d", status);
}

void test_get_info_invalid_p_info_return_INVALID_ARGUMENT(void)
{
	struct psa_storage_info_t *info = NULL;

	status = psa_its_get_info(uid, info);

	zassert_equal(PSA_ERROR_INVALID_ARGUMENT, status,
			"Unexpected status: %d", status);
}

void test_get_info_not_existing_uid_return_DOES_NOT_EXIST(void)
{
	status = psa_its_get_info(not_existing_uid, &info);

	zassert_equal(PSA_ERROR_DOES_NOT_EXIST, status,
			"Unexpected status: %d", status);
}

void test_get_info_correctly_return_SUCCESS(void)
{
	psa_storage_uid_t uid = 3;
	psa_storage_create_flags_t flags =
		PSA_STORAGE_FLAG_NONE | PSA_STORAGE_FLAG_WRITE_ONCE;

	status = psa_its_set(uid, sizeof(data), &data, flags);
	zassert_equal(PSA_SUCCESS, status,
			"Unexpected status: %d", status);

	status = psa_its_get_info(uid, &info);

	zassert_equal(PSA_SUCCESS, status,
			"Unexpected status: %d", status);
	zassert_equal(info.size, sizeof(data), "Unexpected value");
	zassert_equal(info.flags, flags, "Unexpected value");
}

void test_remove_not_existing_uid_return_DOES_NOT_EXIST(void)
{
	status = psa_its_remove(not_existing_uid);

	zassert_equal(PSA_ERROR_DOES_NOT_EXIST, status,
			"Unexpected status: %d", status);
}

void test_remove_correctly_return_SUCCESS(void)
{
	status = psa_its_set(uid, sizeof(data), &data, PSA_STORAGE_FLAG_NONE);
	zassert_equal(PSA_SUCCESS, status,
			"Unexpected status: %d", status);

	status = psa_its_remove(uid);

	zassert_equal(PSA_SUCCESS, status,
			"Unexpected status: %d", status);
}

void test_remove_uid_flag_write_once_return_NOT_PERMITTED(void)
{
	psa_storage_uid_t uid = 0xffffffffffffffff;

	status = psa_its_set(uid, sizeof(data), &data,
			     PSA_STORAGE_FLAG_WRITE_ONCE);
	zassert_equal(PSA_SUCCESS, status,
			"Unexpected status: %d", status);

	status = psa_its_remove(uid);

	zassert_equal(PSA_ERROR_NOT_PERMITTED, status,
			"Unexpected status: %d", status);
}

void test_main(void)
{
	ztest_test_suite(
		test_psa_eits,
		ztest_unit_test(
			test_set_invalid_data_length_return_INVALID_ARGUMENT),
		ztest_unit_test(
			test_set_invalid_p_data_return_INVALID_ARGUMENT),
		ztest_unit_test(test_set_invalid_flag_return_NOT_SUPPORTED),
		ztest_unit_test(
			test_set_exceeding_flash_return_STORAGE_FAILURE),
		ztest_unit_test(test_set_correctly_return_SUCCESS),
		ztest_unit_test(
			test_set_override_flag_write_once_return_NOT_PERMITTED),
		ztest_unit_test(
			test_get_invalid_data_length_return_INVALID_ARGUMENT),
		ztest_unit_test(
			test_get_invalid_p_data_return_INVALID_ARGUMENT),
		ztest_unit_test(
			test_get_not_existing_uid_return_DOES_NOT_EXIST),
		ztest_unit_test(
			test_get_too_small_buff_return_BUFFER_TOO_SMALL),
		ztest_unit_test(test_get_correctly_return_SUCCESS),
		ztest_unit_test(test_get_correct_data_offset_return_SUCCESS),
		ztest_unit_test(
			test_get_invalid_data_offset_return_INVALID_ARGUMENT),
		ztest_unit_test(
			test_get_info_invalid_p_info_return_INVALID_ARGUMENT),
		ztest_unit_test(
			test_get_info_not_existing_uid_return_DOES_NOT_EXIST),
		ztest_unit_test(test_get_info_correctly_return_SUCCESS),
		ztest_unit_test(
			test_remove_not_existing_uid_return_DOES_NOT_EXIST),
		ztest_unit_test(test_remove_correctly_return_SUCCESS),
		ztest_unit_test(
			test_remove_uid_flag_write_once_return_NOT_PERMITTED));
	ztest_run_test_suite(test_psa_eits);
}
