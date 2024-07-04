/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef LC3_FILE_FAKE_H__
#define LC3_FILE_FAKE_H__

#include <zephyr/fff.h>
#include <zephyr/fs/fs.h>
#include <zephyr/types.h>

DECLARE_FAKE_VALUE_FUNC(int, sd_card_init);
DECLARE_FAKE_VALUE_FUNC(int, sd_card_open, const char *, struct fs_file_t *);
DECLARE_FAKE_VALUE_FUNC(int, sd_card_close, struct fs_file_t *);
DECLARE_FAKE_VALUE_FUNC(int, sd_card_read, char *, size_t *, struct fs_file_t *);

/* List of fakes used by this unit tester */
#define DO_FOREACH_FAKE(FUNC)		\
	do {							\
		FUNC(sd_card_init)			\
		FUNC(sd_card_open)			\
		FUNC(sd_card_close)			\
		FUNC(sd_card_read)			\
	} while (0)

/**
 * @brief Reset the internal counter for the number of bytes read from the fake data.
 */
void sd_card_fake_reset_counter(void);

/**
 * @brief Fake function for reading valid LC3 file data.
 */
int sd_card_read_lc3_file_fake_valid(char *buf, size_t *size, struct fs_file_t *f_seg_read_entry);

/**
 * @brief Fake function for reading invalid header data.
 */
int sd_card_read_fake_invalid_header(char *buf, size_t *size, struct fs_file_t *f_seg_read_entry);

/**
 * @brief Fake function for reading an invalid LC3 file, where the header is valid but the frame
 *	  data is shorted than the frame header specifies.
 */
int sd_card_read_lc3_file_fake_invalid_frame(char *buf, size_t *size,
					     struct fs_file_t *f_seg_read_entry);

#endif /* LC3_FILE_FAKE_H__*/
