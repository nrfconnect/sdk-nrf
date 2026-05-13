/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <string.h>
#include <zephyr/fff.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "lc3_file_data.h"
#include "sd_card_fake.h"

static int read_lc3_fake_bytes_read;

DEFINE_FFF_GLOBALS;

DEFINE_FAKE_VALUE_FUNC(int, sd_card_init);
DEFINE_FAKE_VALUE_FUNC(int, sd_card_open, const char *, struct fs_file_t *);
DEFINE_FAKE_VALUE_FUNC(int, sd_card_close, struct fs_file_t *);
DEFINE_FAKE_VALUE_FUNC(int, sd_card_read, char *, size_t *, struct fs_file_t *);

void sd_card_fake_reset_counter(void)
{
	read_lc3_fake_bytes_read = 0;
}

int sd_card_read_lc3_file_fake_valid(char *buf, size_t *size, struct fs_file_t *f_seg_read_entry)
{
	ARG_UNUSED(f_seg_read_entry);

	int read_size;

	if (lc3_file_dataset1_valid_size == read_lc3_fake_bytes_read) {
		/* No data left */
		*size = 0;
		return -EINVAL;
	} else if ((lc3_file_dataset1_valid_size - read_lc3_fake_bytes_read) < *size) {
		/* Not enough data, read less */
		read_size = lc3_file_dataset1_valid_size - read_lc3_fake_bytes_read;
	} else {
		read_size = *size;
	}

	memcpy(buf, &lc3_file_dataset1_valid[read_lc3_fake_bytes_read], read_size);

	read_lc3_fake_bytes_read += read_size;
	*size = read_size;

	return 0;
}

int sd_card_read_fake_invalid_header(char *buf, size_t *size, struct fs_file_t *f_seg_read_entry)
{
	ARG_UNUSED(f_seg_read_entry);

	zassert_equal(*size, lc3_file_dataset_invalid_header_size,
		      "Expected header size to be requested");

	memcpy(buf, lc3_file_dataset_invalid_header, lc3_file_dataset_invalid_header_size);
	*size = lc3_file_dataset_invalid_header_size;

	return 0;
}

int sd_card_read_lc3_file_fake_invalid_frame(char *buf, size_t *size,
					     struct fs_file_t *f_seg_read_entry)
{
	ARG_UNUSED(f_seg_read_entry);

	int read_size;

	if (lc3_file_dataset_invalid_frame_size == read_lc3_fake_bytes_read) {
		/* No data left */
		*size = 0;
		return -EINVAL;
	} else if ((lc3_file_dataset_invalid_frame_size - read_lc3_fake_bytes_read) < *size) {
		/* Not enough data, read less */
		read_size = lc3_file_dataset_invalid_frame_size - read_lc3_fake_bytes_read;
	} else {
		read_size = *size;
	}

	memcpy(buf, &lc3_file_dataset_invalid_frame[read_lc3_fake_bytes_read], read_size);

	read_lc3_fake_bytes_read += read_size;
	*size = read_size;

	return 0;
}
