/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "wav_file.h"

LOG_MODULE_REGISTER(audio_wavfile, CONFIG_WAV_FILE_LOG_LEVEL);

int write_wav_header(struct fs_file_t *wav_file, uint32_t size, uint16_t sample_rate,
		     uint16_t bytes_per_sample, uint16_t num_channels)
{
	struct wav_header wav_file_header;
	wav_file_header.riff_header[0] = 'R';
	wav_file_header.riff_header[1] = 'I';
	wav_file_header.riff_header[2] = 'F';
	wav_file_header.riff_header[3] = 'F';
	wav_file_header.wav_size = size + 0x24;
	wav_file_header.wav_header[0] = 'W';
	wav_file_header.wav_header[1] = 'A';
	wav_file_header.wav_header[2] = 'V';
	wav_file_header.wav_header[3] = 'E';
	wav_file_header.fmt_header[0] = 'f';
	wav_file_header.fmt_header[1] = 'm';
	wav_file_header.fmt_header[2] = 't';
	wav_file_header.fmt_header[3] = ' ';
	wav_file_header.wav_chunk_size = 16;
	wav_file_header.audio_format = WAV_FORMAT_PCM;
	wav_file_header.num_channels = 1;
	wav_file_header.sample_rate = sample_rate;
	wav_file_header.byte_rate = sample_rate * bytes_per_sample * num_channels;
	wav_file_header.block_alignment = bytes_per_sample * num_channels;
	wav_file_header.bit_depth = bytes_per_sample * 8;
	wav_file_header.data_header[0] = 'd';
	wav_file_header.data_header[1] = 'a';
	wav_file_header.data_header[2] = 't';
	wav_file_header.data_header[3] = 'a';
	wav_file_header.data_bytes = size;

	off_t position = fs_tell(wav_file);

	// seek back to the beginning to rewrite the header.
	// BUGBUG: this seek doesn't seem to be working?
	int ret = fs_seek(wav_file, 0, FS_SEEK_SET);
	if (ret) {
		LOG_ERR("Seek file pointer failed");
		return ret;
	}

	ret = write_wav_data(wav_file, (char *)&wav_file_header, sizeof(wav_file_header));

	// seek back to where we were.
	fs_seek(wav_file, position, FS_SEEK_SET);
	return ret;
}

int read_wav_header(struct fs_file_t *wav_file, struct wav_header *header)
{
	int ret = fs_read(wav_file, header, sizeof(struct wav_header));
	if (ret < 0) {
		LOG_ERR("Read file failed: %d", ret);
	}
	return ret;
}

int write_wav_data(struct fs_file_t *wav_file, const char *buffer, uint32_t size)
{
	int ret = fs_write(wav_file, buffer, size);
	if (ret < 0) {
		LOG_ERR("Write file failed: %d", ret);
		return ret;
	}
	return 0;
}