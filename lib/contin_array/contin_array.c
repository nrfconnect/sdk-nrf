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

int contin_array_buf_create(struct net_buf *pcm_contin, void const *const pcm_finite,
			    uint16_t pcm_finite_size, uint32_t locations,
			    uint16_t *const finite_pos)
{
	struct audio_metadata *meta_contin;
	uint8_t num_ch;
	uint8_t count_ch;
	uint8_t *output;
	uint8_t carrier_bytes;
	uint16_t step;
	uint16_t frame_bytes;
	uint16_t finite_start_pos;
	uint32_t out_locs;

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

	if (meta_contin->locations == 0 && locations == 0) {
		out_locs = 0x00000001;
	} else {
		out_locs = meta_contin->locations & locations;
	}

	if (out_locs == 0) {
		LOG_ERR("locations error");
		return -EPERM;
	}

	carrier_bytes = meta_contin->carried_bits_per_sample / 8;
	if (carrier_bytes < 0 && carrier_bytes > 4) {
		LOG_ERR("carrier bytes out of range: %d", carrier_bytes);
		return -EINVAL;
	}

	num_ch = metadata_num_ch_get(meta_contin);

	if (pcm_contin->len == 0) {
		memset(pcm_contin->data, 0, meta_contin->bytes_per_location * num_ch);
		net_buf_add(pcm_contin, meta_contin->bytes_per_location * num_ch);
	}

	finite_start_pos = *finite_pos;

	if (meta_contin->interleaved) {
		step = carrier_bytes * (num_ch - 1);
		frame_bytes = carrier_bytes;
	} else {
		step = 0;
		frame_bytes = meta_contin->bytes_per_location;
	}

	count_ch = 0;

	while (out_locs) {
		if (out_locs & 0x01) {
			output = &((uint8_t *)pcm_contin->data)[frame_bytes * count_ch];

			*finite_pos = finite_start_pos;

			for (size_t j = 0; j < meta_contin->bytes_per_location;
			     j += carrier_bytes) {
				for (size_t k = 0; k < carrier_bytes; k++) {
					*output++ = ((uint8_t *)pcm_finite)[(*finite_pos)++];

					if (*finite_pos >= pcm_finite_size) {
						*finite_pos = 0;
					}
				}

				output += step;
			}

			count_ch++;
		}

		out_locs >>= 1;
	}

	return 0;
}

int contin_array_net_buf_create(struct net_buf *pcm_contin, struct net_buf *pcm_finite,
				uint32_t locations, uint16_t *const finite_pos)
{
	struct audio_metadata *meta_contin;
	struct audio_metadata *meta_finite;

	if (pcm_contin == NULL || pcm_finite == NULL) {
		LOG_ERR("error in input parameter");
		return -ENXIO;
	}

	if (pcm_finite->len == 0) {
		LOG_ERR("finite buffer size error");
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

	if (meta_contin->sample_rate_hz != meta_finite->sample_rate_hz ||
	    meta_contin->bits_per_sample != meta_finite->bits_per_sample ||
	    meta_contin->carried_bits_per_sample != meta_finite->carried_bits_per_sample ||
	    metadata_num_ch_get(meta_finite) > 1) {
		LOG_ERR("sample/carrier/finite locations mismatch");
		return -EINVAL;
	}

	return contin_array_buf_create(pcm_contin, (void const *const)pcm_finite->data,
				       meta_finite->bytes_per_location, locations, finite_pos);
}
