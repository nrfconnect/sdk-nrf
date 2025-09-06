/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <contin_array.h>

#include <zephyr/kernel.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(contin_array, CONFIG_CONTIN_ARRAY_LOG_LEVEL);

int contin_array_create(void *const pcm_cont, uint32_t pcm_cont_size, void const *const pcm_finite,
			uint32_t pcm_finite_size, uint32_t *const finite_pos)
{
	LOG_DBG("pcm_cont_size: %d pcm_finite_size %d", pcm_cont_size, pcm_finite_size);

	if (pcm_cont == NULL || pcm_finite == NULL) {
		return -ENXIO;
	}

	if (!pcm_cont_size || !pcm_finite_size) {
		LOG_ERR("size cannot be zero");
		return -EPERM;
	}

	for (uint32_t i = 0; i < pcm_cont_size; i++) {
		if (*finite_pos > (pcm_finite_size - 1)) {
			*finite_pos = 0;
		}
		((char *)pcm_cont)[i] = ((char *)pcm_finite)[*finite_pos];
		(*finite_pos)++;
	}

	return 0;
}

int contin_array_single_chan_buf_create(struct net_buf *pcm_contin, void const *const pcm_finite,
					uint16_t pcm_finite_size, uint8_t out_ch,
					uint16_t *const finite_pos)
{
	uint16_t step;
	struct audio_metadata *meta_contin;
	uint8_t num_ch;
	uint8_t *output;
	uint8_t carrier_bytes;
	uint16_t chan_start;

	if (pcm_contin == NULL || pcm_finite == NULL || finite_pos == NULL) {
		LOG_ERR("invalid parameter");
		return -ENXIO;
	}

	if (pcm_contin->size == 0 || pcm_contin->len == 0 || pcm_finite_size == 0 ||
	    *finite_pos >= pcm_finite_size) {
		LOG_ERR("size or finite position out of range");
		return -EPERM;
	}

	meta_contin = net_buf_user_data(pcm_contin);
	if (meta_contin == NULL) {
		LOG_ERR("no metadata");
		return -ENXIO;
	}

	num_ch = metadata_num_ch_get(meta_contin);
	if (num_ch < out_ch) {
		LOG_ERR("out_ch out of range");
		return -EPERM;
	}

	carrier_bytes = meta_contin->carried_bits_per_sample / 8;

	if (meta_contin->interleaved) {
		step = carrier_bytes * (num_ch - 1);
		chan_start = carrier_bytes * (out_ch - 1);
	} else {
		step = 0;
		chan_start = meta_contin->bytes_per_location * (out_ch - 1);
	}

	output = &((uint8_t *)pcm_contin->data)[chan_start];

	for (size_t j = 0; j < meta_contin->bytes_per_location; j += carrier_bytes) {
		for (size_t k = 0; k < carrier_bytes; k++) {
			*output++ = ((uint8_t *)pcm_finite)[(*finite_pos)++];

			if (*finite_pos >= pcm_finite_size) {
				*finite_pos = 0;
			}
		}

		output += step;
	}

	return 0;
}

int contin_array_chans_buf_create(struct net_buf *pcm_contin, void const *const pcm_finite,
				  uint16_t pcm_finite_size, uint16_t *const finite_pos)
{
	uint16_t chan_step;
	struct audio_metadata *meta_contin;
	uint8_t num_ch;
	uint8_t *output;
	uint8_t carrier_bytes;
	uint16_t finite_start_pos;
	uint16_t chan_start;

	if (pcm_contin == NULL || pcm_finite == NULL || finite_pos == NULL) {
		LOG_ERR("invalid parameter");
		return -ENXIO;
	}

	if (pcm_contin->size == 0 || pcm_finite_size == 0 || *finite_pos >= pcm_finite_size) {
		LOG_ERR("size or finite position out of range");
		return -EPERM;
	}

	meta_contin = net_buf_user_data(pcm_contin);
	if (meta_contin == NULL) {
		LOG_ERR("no metadata");
		return -ENXIO;
	}

	num_ch = metadata_num_ch_get(meta_contin);
	carrier_bytes = meta_contin->carried_bits_per_sample / 8;
	finite_start_pos = *finite_pos;

	if (meta_contin->interleaved) {
		chan_step = carrier_bytes * (num_ch - 1);
		chan_start = carrier_bytes;
	} else {
		chan_step = 0;
		chan_start = meta_contin->bytes_per_location;
	}

	for (size_t i = 0; i < num_ch; i++) {
		output = &pcm_contin->data[i * chan_start];
		*finite_pos = finite_start_pos;

		for (size_t j = 0; j < meta_contin->bytes_per_location; j += carrier_bytes) {
			for (size_t k = 0; k < carrier_bytes; k++) {
				*output++ = ((uint8_t *)pcm_finite)[(*finite_pos)++];

				if (*finite_pos >= pcm_finite_size) {
					*finite_pos = 0;
				}
			}

			output += chan_step;
		}
	}

	if (pcm_contin->len == 0) {
		net_buf_add(pcm_contin, meta_contin->bytes_per_location * num_ch);
	} else if (pcm_contin->len < meta_contin->bytes_per_location * num_ch) {
		net_buf_add(pcm_contin,
			    (meta_contin->bytes_per_location * num_ch) - pcm_contin->len);
	} else if (pcm_contin->len > meta_contin->bytes_per_location * num_ch) {
		net_buf_remove_mem(pcm_contin,
				   (meta_contin->bytes_per_location * num_ch) - pcm_contin->len);
	}

	return 0;
}

int contin_array_single_chan_create(struct net_buf *pcm_contin, struct net_buf *pcm_finite,
				    uint8_t out_ch, uint16_t *const finite_pos)
{
	uint16_t step;
	struct audio_metadata *meta_contin;
	struct audio_metadata *meta_finite;
	uint8_t num_ch;
	uint8_t *output;
	uint8_t carrier_bytes;
	uint16_t chan_start;

	if (pcm_contin == NULL || pcm_finite == NULL || finite_pos == NULL) {
		LOG_ERR("error in input parameter");
		return -ENXIO;
	}

	if (pcm_contin->size == 0 || pcm_contin->len == 0 || pcm_finite->len == 0 ||
	    *finite_pos >= pcm_finite->len) {
		LOG_ERR("buffer size error");
		return -EPERM;
	}

	meta_contin = net_buf_user_data(pcm_contin);
	if (meta_contin == NULL) {
		LOG_ERR("no metadata");
		return -ENXIO;
	}

	meta_finite = net_buf_user_data(pcm_finite);
	if (meta_finite == NULL) {
		LOG_ERR("no metadata");
		return -ENXIO;
	}

	if (meta_contin->bits_per_sample != meta_finite->bits_per_sample ||
	    meta_contin->carried_bits_per_sample != meta_finite->carried_bits_per_sample ||
	    meta_contin->sample_rate_hz != meta_finite->sample_rate_hz) {
		LOG_ERR("sample/carrier/sample rate miss match");
		return -EINVAL;
	}

	num_ch = metadata_num_ch_get(meta_contin);
	if (num_ch < out_ch) {
		LOG_ERR("channels meismatch");
		return -EPERM;
	}

	carrier_bytes = meta_contin->carried_bits_per_sample / 8;

	if (meta_contin->interleaved) {
		step = carrier_bytes * (num_ch - 1);
		chan_start = carrier_bytes * (out_ch - 1);
	} else {
		step = 0;
		chan_start = meta_contin->bytes_per_location * (out_ch - 1);
	}

	output = &((uint8_t *)pcm_contin->data)[chan_start];

	for (size_t j = 0; j < meta_contin->bytes_per_location; j += carrier_bytes) {
		for (size_t k = 0; k < carrier_bytes; k++) {
			*output++ = ((uint8_t *)pcm_finite->data)[(*finite_pos)++];

			if (*finite_pos >= pcm_finite->len) {
				*finite_pos = 0;
			}
		}

		output += step;
	}

	return 0;
}

int contin_array_chans_create(struct net_buf *pcm_contin, struct net_buf *pcm_finite,
			      uint16_t *const finite_pos)
{
	uint16_t chan_step;
	struct audio_metadata *meta_contin;
	struct audio_metadata *meta_finite;
	uint8_t num_ch;
	uint8_t *output;
	uint8_t carrier_bytes;
	uint16_t finite_start_pos;
	uint16_t chan_start;

	if (pcm_contin == NULL || pcm_finite == NULL || finite_pos == NULL) {
		LOG_ERR("error in input parameter");
		return -ENXIO;
	}

	if (pcm_contin->size == 0 || pcm_finite->len == 0 || *finite_pos >= pcm_finite->len) {
		LOG_ERR("buffer size error");
		return -EPERM;
	}

	meta_contin = net_buf_user_data(pcm_contin);
	if (meta_contin == NULL) {
		LOG_ERR("no metadata");
		return -ENXIO;
	}

	meta_finite = net_buf_user_data(pcm_finite);
	if (meta_finite == NULL) {
		LOG_ERR("no metadata");
		return -ENXIO;
	}

	if (meta_contin->bits_per_sample != meta_finite->bits_per_sample ||
	    meta_contin->carried_bits_per_sample != meta_finite->carried_bits_per_sample ||
	    meta_contin->sample_rate_hz != meta_finite->sample_rate_hz) {
		LOG_ERR("sample/carrier/sample rate miss match");
		return -EINVAL;
	}

	num_ch = metadata_num_ch_get(meta_contin);
	carrier_bytes = meta_contin->carried_bits_per_sample / 8;
	finite_start_pos = *finite_pos;

	if (meta_contin->interleaved) {
		chan_step = carrier_bytes * (num_ch - 1);
		chan_start = carrier_bytes;
	} else {
		chan_step = 0;
		chan_start = meta_contin->bytes_per_location;
	}

	for (size_t i = 0; i < num_ch; i++) {
		output = &pcm_contin->data[i * chan_start];
		*finite_pos = finite_start_pos;

		for (size_t j = 0; j < meta_contin->bytes_per_location; j += carrier_bytes) {
			for (size_t k = 0; k < carrier_bytes; k++) {
				*output++ = ((uint8_t *)pcm_finite->data)[(*finite_pos)++];

				if (*finite_pos >= pcm_finite->len) {
					*finite_pos = 0;
				}
			}

			output += chan_step;
		}
	}

	if (pcm_contin->len == 0) {
		net_buf_add(pcm_contin, meta_contin->bytes_per_location * num_ch);
	} else if (pcm_contin->len < meta_contin->bytes_per_location * num_ch) {
		net_buf_add(pcm_contin,
			    (meta_contin->bytes_per_location * num_ch) - pcm_contin->len);
	} else if (pcm_contin->len > meta_contin->bytes_per_location * num_ch) {
		net_buf_remove_mem(pcm_contin,
				   (meta_contin->bytes_per_location * num_ch) - pcm_contin->len);
	}

	return 0;
}
