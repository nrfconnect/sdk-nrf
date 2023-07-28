/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "sd_card_playback.h"

#include <stdint.h>
#include <math.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/shell/shell.h>

#include "sd_card.h"
#include "sw_codec_lc3.h"
#include "sw_codec_select.h"
#include "pcm_mix.h"
#include "audio_system.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sd_card_playback, CONFIG_MODULE_SD_CARD_PLAYBACK_LOG_LEVEL);

#define MAX_PATH_LEN	    (CONFIG_FS_FATFS_MAX_LFN)
#define LIST_FILES_BUF_SIZE 512
#define FRAME_DURATION_MS   (CONFIG_AUDIO_FRAME_DURATION_US / 1000)

#define WAV_FORMAT_PCM	    1
#define WAV_SAMPLE_RATE_48K 48000

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
} __packed;

/* LC3 header */
struct lc3_header {
	uint16_t file_id;	 /* Constant value, 0xCC1C */
	uint16_t hdr_size;	 /* Header size, 0x0012 */
	uint16_t sample_rate;	 /* Sample frequency / 100 */
	uint16_t bit_rate;	 /* Bit rate / 100 (total for all channels) */
	uint16_t channels;	 /* Number of channels */
	uint16_t frame_duration; /* Frame duration in ms * 100 */
	uint16_t rfu;		 /* Reserved for future use */
	uint16_t signal_len_lsb; /* Number of samples in signal, 16 LSB */
	uint16_t signal_len_msb; /* Number of samples in signal, 16 MSB (>> 16) */
} __packed;

struct lc3_playback_config {
	uint16_t lc3_frames_num;
	uint16_t lc3_frame_length_bytes;
};

enum audio_formats {
	SD_CARD_PLAYBACK_WAV,
	SD_CARD_PLAYBACK_LC3,
};

RING_BUF_DECLARE(m_ringbuf_audio_data_lc3, CONFIG_SD_CARD_PLAYBACK_RING_BUF_SIZE);
K_SEM_DEFINE(m_sem_ringbuf_space_available, 0, 1);
K_MUTEX_DEFINE(mtx_ringbuf);
K_SEM_DEFINE(m_sem_playback, 0, 1);
K_THREAD_STACK_DEFINE(sd_card_playback_thread_stack, CONFIG_SD_CARD_PLAYBACK_STACK_SIZE);

/* Thread */
static struct k_thread sd_card_playback_thread_data;
static k_tid_t sd_card_playback_thread_id;

/* Playback */
static bool sd_card_playback_active;
static char *playback_file_name;
static enum audio_formats playback_file_format;
static uint16_t pcm_frame_size;
static char playback_file_path[MAX_PATH_LEN] = "";
static struct lc3_header lc3_file_header;
static struct wav_header wav_file_header;
static struct lc3_playback_config lc3_playback_cfg;

static struct fs_file_t f_seg_read_entry;

static int sd_card_playback_ringbuf_read(uint8_t *buf, size_t *size)
{
	int ret;
	uint16_t read_size;

	ret = k_mutex_lock(&mtx_ringbuf, K_NO_WAIT);
	if (ret) {
		LOG_ERR("Unable to take mutex. Ret: %d", ret);
		return ret;
	}

	read_size = ring_buf_get(&m_ringbuf_audio_data_lc3, buf, *size);
	if (read_size != *size) {
		LOG_WRN("Read size (%d) not equal requested size (%d)", read_size, *size);
	}

	ret = k_mutex_unlock(&mtx_ringbuf);
	if (ret) {
		LOG_ERR("Mutex unlock err: %d", ret);
		return ret;
	}

	if (ring_buf_space_get(&m_ringbuf_audio_data_lc3) >= pcm_frame_size) {
		k_sem_give(&m_sem_ringbuf_space_available);
	}

	*size = read_size;

	return 0;
}

static int sd_card_playback_ringbuf_write(uint8_t *buffer, size_t numbytes)
{
	int ret;
	uint8_t *buf_ptr;

	/* The ringbuffer is read every 10 ms by audio datapath when SD card playback is enabled.
	 * Timeout value should therefore not be less than 10 ms
	 */
	ret = k_sem_take(&m_sem_ringbuf_space_available, K_MSEC(20));
	if (ret) {
		LOG_ERR("Sem take err: %d. Skipping frame", ret);
		return ret;
	}

	ret = k_mutex_lock(&mtx_ringbuf, K_NO_WAIT);
	if (ret) {
		LOG_ERR("Unable to take mutex. Ret: %d", ret);
		return ret;
	}

	numbytes = ring_buf_put_claim(&m_ringbuf_audio_data_lc3, &buf_ptr, numbytes);
	memcpy(buf_ptr, buffer, numbytes);
	ret = ring_buf_put_finish(&m_ringbuf_audio_data_lc3, numbytes);
	if (ret) {
		LOG_ERR("Ring buf put finish err: %d", ret);
		return ret;
	}

	ret = k_mutex_unlock(&mtx_ringbuf);
	if (ret) {
		LOG_ERR("Mutex unlock err: %d", ret);
		return ret;
	}

	return numbytes;
}

static int sd_card_playback_check_wav_header(struct wav_header wav_file_header)
{
	if (wav_file_header.audio_format != WAV_FORMAT_PCM) {
		LOG_ERR("This is not a PCM file");
		return -EPERM;
	}

	if (wav_file_header.num_channels != SW_CODEC_MONO) {
		LOG_ERR("This is not a MONO file");
		return -EPERM;
	}

	if (wav_file_header.sample_rate != WAV_SAMPLE_RATE_48K) {
		LOG_ERR("Unsupported sample rate: %d", wav_file_header.sample_rate);
		return -EPERM;
	}

	if (wav_file_header.bit_depth != CONFIG_AUDIO_BIT_DEPTH_BITS) {
		LOG_ERR("Bit depth in WAV file is not 16, but %d", wav_file_header.bit_depth);
		return -EPERM;
	}

	return 0;
}

static int sd_card_playback_play_wav(void)
{
	int ret;
	int ret_sd_card_close;
	size_t wav_read_size;
	size_t wav_file_header_size = sizeof(wav_file_header);
	int audio_length_bytes;
	int n_iter;

	ret = sd_card_open(playback_file_name, &f_seg_read_entry);
	if (ret) {
		LOG_ERR("Open SD card file err: %d", ret);
		return ret;
	}

	ret = sd_card_read((char *)&wav_file_header, &wav_file_header_size, &f_seg_read_entry);
	if (ret) {
		LOG_ERR("Read SD card err: %d", ret);
		ret_sd_card_close = sd_card_close(&f_seg_read_entry);
		if (ret_sd_card_close) {
			LOG_ERR("Close SD card err: %d", ret_sd_card_close);
			return ret_sd_card_close;
		}
		return ret;
	}

	/* Verify that there is support for playing the specified file */
	ret = sd_card_playback_check_wav_header(wav_file_header);
	if (ret) {
		LOG_ERR("WAV header check failed. Ret: %d", ret);
		ret_sd_card_close = sd_card_close(&f_seg_read_entry);
		if (ret_sd_card_close) {
			LOG_ERR("Close SD card err: %d", ret_sd_card_close);
			return ret_sd_card_close;
		}
		return ret;
	}

	/* Size corresponding to frame size of audio BT stream */
	pcm_frame_size = wav_file_header.byte_rate * FRAME_DURATION_MS / 1000;
	wav_read_size = pcm_frame_size;
	uint8_t pcm_mono_frame[wav_read_size];

	audio_length_bytes = wav_file_header.wav_size + 8 - sizeof(wav_file_header);
	n_iter = ceil((float)audio_length_bytes / (float)wav_read_size);

	for (int i = 0; i < n_iter; i++) {
		/* Read a chunk of audio data from file */
		ret = sd_card_read(pcm_mono_frame, &wav_read_size, &f_seg_read_entry);
		if (ret < 0) {
			LOG_ERR("SD card read err: %d", ret);
			break;
		}

		/* Write audio data to the ringbuffer */
		ret = sd_card_playback_ringbuf_write(pcm_mono_frame, wav_read_size);
		if (ret < 0) {
			LOG_ERR("Load ringbuf err: %d", ret);
			break;
		}

		if (i == 0) {
			/* Data can now be read from the ringbuffer */
			sd_card_playback_active = true;
		}
	}

	sd_card_playback_active = false;

	ret_sd_card_close = sd_card_close(&f_seg_read_entry);
	/* Check if something inside the for loop failed */
	if (ret < 0) {
		LOG_ERR("WAV playback err: %d", ret);
		return ret;
	}

	if (ret_sd_card_close) {
		LOG_ERR("SD card close err: %d", ret);
		return ret;
	}

	return 0;
}

static int sd_card_playback_play_lc3(void)
{
	int ret;
	int ret_sd_card_close;
	uint16_t pcm_mono_write_size;
	uint8_t decoder_num_ch = audio_system_decoder_num_ch_get();
	size_t lc3_file_header_size = sizeof(lc3_file_header);
	size_t lc3_frame_header_size = sizeof(uint16_t);

	ret = sd_card_open(playback_file_name, &f_seg_read_entry);
	if (ret) {
		LOG_ERR("Open SD card file err: %d", ret);
		return ret;
	}

	/* Read the file header */
	ret = sd_card_read((char *)&lc3_file_header, &lc3_file_header_size, &f_seg_read_entry);
	if (ret < 0) {
		LOG_ERR("Read SD card file err: %d", ret);
		ret_sd_card_close = sd_card_close(&f_seg_read_entry);
		if (ret_sd_card_close) {
			LOG_ERR("Close SD card err: %d", ret_sd_card_close);
			return ret_sd_card_close;
		}
		return ret;
	}

	pcm_frame_size = sizeof(uint16_t) * lc3_file_header.sample_rate *
			 lc3_file_header.frame_duration / 1000;
	lc3_playback_cfg.lc3_frames_num =
		sizeof(uint16_t) *
		((lc3_file_header.signal_len_msb << 16) + lc3_file_header.signal_len_lsb) /
		pcm_frame_size;

	uint8_t pcm_mono_frame[pcm_frame_size];

	for (int i = 0; i < lc3_playback_cfg.lc3_frames_num; i++) {
		/* Read the frame header */
		ret = sd_card_read((char *)&lc3_playback_cfg.lc3_frame_length_bytes,
				   &lc3_frame_header_size, &f_seg_read_entry);
		if (ret < 0) {
			LOG_ERR("SD card read err: %d", ret);
			break;
		}

		char lc3_frame[lc3_playback_cfg.lc3_frame_length_bytes];
		size_t lc3_fr_len = lc3_playback_cfg.lc3_frame_length_bytes;

		/* Read the audio data frame to be decoded */
		ret = sd_card_read(lc3_frame, &lc3_fr_len, &f_seg_read_entry);
		if (ret < 0) {
			LOG_ERR("SD card read err: %d", ret);
			break;
		}

		if (lc3_fr_len != lc3_playback_cfg.lc3_frame_length_bytes) {
			LOG_ERR("SD card read size (%d) not equal requested size (%d)", lc3_fr_len,
				lc3_playback_cfg.lc3_frame_length_bytes);
			ret = -EPERM;
			break;
		}

		/* Decode audio data frame */
		ret = sw_codec_lc3_dec_run(lc3_frame, lc3_playback_cfg.lc3_frame_length_bytes,
					   pcm_frame_size, decoder_num_ch - 1, pcm_mono_frame,
					   &pcm_mono_write_size, false);
		if (ret) {
			LOG_ERR("Decoding err: %d", ret);
			break;
		}

		ret = sd_card_playback_ringbuf_write((char *)pcm_mono_frame, pcm_mono_write_size);
		if (ret < 0) {
			LOG_ERR("Load ringbuf err: %d", ret);
			break;
		}

		if (i == 0) {
			sd_card_playback_active = true;
		}
	}

	sd_card_playback_active = false;
	ret_sd_card_close = sd_card_close(&f_seg_read_entry);
	if (ret < 0) {
		LOG_ERR("LC3 playback err: %d", ret);
		sd_card_playback_active = false;
		return ret;
	}

	if (ret_sd_card_close) {
		LOG_ERR("SD card close err: %d", ret);
		sd_card_playback_active = false;
		return ret;
	}

	return 0;
}

static void sd_card_playback_thread(void *arg1, void *arg2, void *arg3)
{
	int ret;

	while (1) {
		k_sem_take(&m_sem_playback, K_FOREVER);
		switch (playback_file_format) {
		case SD_CARD_PLAYBACK_WAV:
			ring_buf_reset(&m_ringbuf_audio_data_lc3);
			k_sem_reset(&m_sem_ringbuf_space_available);
			k_sem_give(&m_sem_ringbuf_space_available);
			ret = sd_card_playback_play_wav();
			if (ret) {
				LOG_ERR("Wav playback err: %d", ret);
			}

			break;

		case SD_CARD_PLAYBACK_LC3:
			ring_buf_reset(&m_ringbuf_audio_data_lc3);
			k_sem_reset(&m_sem_ringbuf_space_available);
			k_sem_give(&m_sem_ringbuf_space_available);
			ret = sd_card_playback_play_lc3();
			if (ret) {
				LOG_ERR("LC3 playback err: %d", ret);
			}

			break;
		}
	}
}

bool sd_card_playback_is_active(void)
{
	return sd_card_playback_active;
}

int sd_card_playback_wav(char *filename)
{
	if (!sw_codec_is_initialized()) {
		LOG_ERR("Sw codec not initialized");
		return -EACCES;
	}

	playback_file_format = SD_CARD_PLAYBACK_WAV;
	playback_file_name = filename;
	k_sem_give(&m_sem_playback);

	return 0;
}

int sd_card_playback_lc3(char *filename)
{
	if (!sw_codec_is_initialized()) {
		LOG_ERR("Sw codec not initialized");
		return -EACCES;
	}

	playback_file_format = SD_CARD_PLAYBACK_LC3;
	playback_file_name = filename;
	k_sem_give(&m_sem_playback);

	return 0;
}

int sd_card_playback_mix_with_stream(void *const pcm_a, size_t pcm_a_size)
{
	int ret;
	uint8_t pcm_b[pcm_frame_size];
	size_t read_size = pcm_frame_size;

	if (!sd_card_playback_active) {
		LOG_ERR("SD card playback is not active");
		return -EACCES;
	}

	ret = sd_card_playback_ringbuf_read(pcm_b, &read_size);
	if (ret) {
		LOG_ERR("Loading data into buffer err: %d", ret);
		return ret;
	}

	if (read_size > 0) {
		ret = pcm_mix(pcm_a, pcm_a_size, pcm_b, read_size, B_MONO_INTO_A_STEREO_L);
		if (ret) {
			LOG_ERR("Pcm mix err: %d", ret);
			return ret;
		}
	} else {
		LOG_WRN("Size read from ringbuffer: %d. Skipping", read_size);
	}

	return 0;
}

int sd_card_playback_init(void)
{
	int ret;

	sd_card_playback_thread_id = k_thread_create(
		&sd_card_playback_thread_data, sd_card_playback_thread_stack,
		CONFIG_SD_CARD_PLAYBACK_STACK_SIZE, (k_thread_entry_t)sd_card_playback_thread, NULL,
		NULL, NULL, K_PRIO_PREEMPT(CONFIG_SD_CARD_PLAYBACK_THREAD_PRIORITY), 0, K_NO_WAIT);
	ret = k_thread_name_set(sd_card_playback_thread_id, "sd_card_playback");
	if (ret) {
		return ret;
	}

	return 0;
}

/* Shell functions */
static int cmd_play_wav_file(const struct shell *shell, size_t argc, char **argv)
{
	int ret;

	char file_loc[MAX_PATH_LEN] = "";

	if (argc != 2) {
		shell_error(shell, "Incorrect number of args");
		return -EINVAL;
	}

	strcat(file_loc, playback_file_path);
	strcat(file_loc, argv[1]);
	ret = sd_card_playback_wav(file_loc);
	if (ret) {
		shell_error(shell, "WAV playback err: %d", ret);
		return ret;
	}

	return 0;
}

static int cmd_play_lc3_file(const struct shell *shell, size_t argc, char **argv)
{
	int ret;

	if (argc != 2) {
		shell_error(shell, "Incorrect number of args");
		return -EINVAL;
	}

	char file_loc[MAX_PATH_LEN] = "";

	strcat(file_loc, playback_file_path);
	strcat(file_loc, argv[1]);
	ret = sd_card_playback_lc3(file_loc);
	if (ret) {
		shell_error(shell, "LC3 playback err: %d", ret);
		return ret;
	}

	return 0;
}

static int cmd_change_dir(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_error(shell, "Incorrect number of args");
		return -EINVAL;
	}

	if (argv[1][0] == '/') {
		playback_file_path[0] = '\0';
		shell_print(shell, "Current directory: root");
	} else {
		strcat(playback_file_path, argv[1]);
		strcat(playback_file_path, "/");
		shell_print(shell, "Current directory: %s", playback_file_path);
	}

	return 0;
}

static int cmd_list_files(const struct shell *shell, size_t argc, char **argv)
{
	int ret;
	char buf[LIST_FILES_BUF_SIZE];
	size_t buf_size = LIST_FILES_BUF_SIZE;

	ret = sd_card_list_files(playback_file_path, buf, &buf_size);
	if (ret) {
		shell_error(shell, "List files err: %d", ret);
		return ret;
	}

	shell_print(shell, "%s", buf);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sd_card_playback_cmd,
	SHELL_COND_CMD(CONFIG_SHELL, play_lc3, NULL, "Play LC3 file", cmd_play_lc3_file),
	SHELL_COND_CMD(CONFIG_SHELL, play_wav, NULL, "Play WAV file", cmd_play_wav_file),
	SHELL_COND_CMD(CONFIG_SHELL, cd, NULL, "Change directory", cmd_change_dir),
	SHELL_COND_CMD(CONFIG_SHELL, list_files, NULL, "List files", cmd_list_files),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(sd_card_playback, &sd_card_playback_cmd, "Play audio files from SD card", NULL);
