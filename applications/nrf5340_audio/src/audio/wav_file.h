/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _WAV_FILE_H_
#define _WAV_FILE_H_

#include <stdint.h>
#include <stddef.h>
#include <zephyr/fs/fs.h>

#define WAV_FORMAT_PCM 1

/* WAV header */
struct wav_header {
	/* RIFF Header */
	char riff_header[4];
	uint32_t wav_size;  /* File size excluding first eight bytes */
	char wav_header[4]; /* Contains "WAVE" */
	/* Format Header */
	char fmt_header[4];
	uint32_t wav_chunk_size; /* Should be 16 for PCM */
	short audio_format;	 /* Should be 1 for PCM */
	short num_channels;
	uint32_t sample_rate;
	uint32_t byte_rate;
	short block_alignment; /* num_channels * Bytes Per Sample */
	short bit_depth;

	/* Data */
	char data_header[4];
	uint32_t data_bytes; /* Number of bytes in data */
} __packed;		     // 44 bytes

int read_wav_header(struct fs_file_t *wav_file, struct wav_header *header);

int write_wav_header(struct fs_file_t *wav_file, uint32_t size, uint16_t sample_rate,
		     uint16_t bytes_per_sample, uint16_t num_channels);

int write_wav_data(struct fs_file_t *wav_file, const char *buffer, uint32_t size);

#endif