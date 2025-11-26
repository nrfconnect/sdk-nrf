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

static void copy_samples(uint8_t *pcm_contin, uint32_t pcm_contin_size,
			 uint8_t const *const pcm_finite, uint32_t pcm_finite_size,
			 uint16_t *const _finite_pos, uint16_t step, uint8_t carrier_bytes)
{
	for (size_t j = 0; j < pcm_contin_size; j += carrier_bytes) {
		for (size_t k = 0; k < carrier_bytes; k++) {
			*pcm_contin++ = pcm_finite[(*_finite_pos)++];

			if (*_finite_pos >= pcm_finite_size) {
				*_finite_pos = 0;
			}
		}

		pcm_contin += step;
	}
}

int contin_array_create(void *const pcm_cont, uint32_t pcm_cont_size, void const *const pcm_finite,
			uint32_t pcm_finite_size, uint32_t *const _finite_pos)
{
	LOG_DBG("pcm_cont_size: %d pcm_finite_size %d", pcm_cont_size, pcm_finite_size);

	if (pcm_cont == NULL || pcm_finite == NULL) {
		return -ENXIO;
	}

	if (!pcm_cont_size || !pcm_finite_size) {
		LOG_ERR("Size cannot be zero");
		return -EPERM;
	}

	for (uint32_t i = 0; i < pcm_cont_size; i++) {
		if (*_finite_pos > (pcm_finite_size - 1)) {
			*_finite_pos = 0;
		}
		((char *)pcm_cont)[i] = ((char *)pcm_finite)[*_finite_pos];
		(*_finite_pos)++;
	}

	return 0;
}

int contin_array_buf_create(struct net_buf *pcm_contin, void const *const pcm_finite,
			    uint16_t pcm_finite_size, uint32_t locations, uint16_t *_finite_pos)
{
	struct audio_metadata *meta_contin;
	uint8_t num_loc;
	uint8_t count_ch;
	uint8_t *output;
	uint8_t carrier_bytes;
	uint16_t step;
	uint16_t frame_bytes;
	uint16_t finite_start_pos;
	uint32_t out_locs;

	if (pcm_contin == NULL || pcm_finite == NULL || _finite_pos == NULL) {
		LOG_ERR("Invalid parameter");
		return -ENXIO;
	}

	if ((pcm_contin->size == 0) || (pcm_finite_size == 0) ||
	    (*_finite_pos >= pcm_finite_size)) {
		LOG_ERR("Size or finite position out of range");
		return -EPERM;
	}

	meta_contin = net_buf_user_data(pcm_contin);
	if (meta_contin == NULL) {
		LOG_ERR("No metadata");
		return -ENXIO;
	}

	/* Here the number of common output locations is determined */
	if (meta_contin->locations == 0 && locations == 0) {
		/* Both are mono buffers and hence have a single output location in common */
		out_locs = 0x01;
	} else {
		out_locs = meta_contin->locations & locations;
	}

	if (out_locs == 0) {
		LOG_ERR("Locations error");
		return -EPERM;
	}

	if ((meta_contin->carried_bits_per_sample == 0) ||
	    (meta_contin->carried_bits_per_sample > PCM_CONT_MAX_CARRIER_BIT_DEPTH) ||
	    (meta_contin->carried_bits_per_sample % 8)) {
		LOG_ERR("Carrier bits invalid: %d", meta_contin->carried_bits_per_sample);
		return -EINVAL;
	}

	carrier_bytes = meta_contin->carried_bits_per_sample / 8;

	num_loc = audio_metadata_num_loc_get(meta_contin);

	if (pcm_contin->len == 0) {
		memset(pcm_contin->data, 0, meta_contin->bytes_per_location * num_loc);
		net_buf_add(pcm_contin, meta_contin->bytes_per_location * num_loc);
	}

	finite_start_pos = *_finite_pos;

	if (meta_contin->interleaved) {
		step = carrier_bytes * (num_loc - 1);
		frame_bytes = carrier_bytes;
	} else {
		step = 0;
		frame_bytes = meta_contin->bytes_per_location;
	}

	count_ch = 0;

	/* While there are common output locations */
	while (out_locs) {
		if (out_locs & 0x01) {
			output = &((uint8_t *)pcm_contin->data)[frame_bytes * count_ch];

			*_finite_pos = finite_start_pos;

			copy_samples(output, meta_contin->bytes_per_location, (uint8_t *)pcm_finite,
				     pcm_finite_size, _finite_pos, step, carrier_bytes);

			count_ch++;
		}

		out_locs >>= 1;
	}

	return 0;
}

int contin_array_net_buf_create(struct net_buf *pcm_contin, struct net_buf const *const pcm_finite,
				uint32_t locations, uint16_t *const _finite_pos)
{
	struct audio_metadata *meta_contin;
	struct audio_metadata *meta_finite;

	if (pcm_contin == NULL || pcm_finite == NULL) {
		LOG_ERR("Error in input parameter");
		return -ENXIO;
	}

	if (pcm_finite->len == 0) {
		LOG_ERR("Finite buffer size error");
		return -EPERM;
	}

	meta_contin = net_buf_user_data(pcm_contin);
	if (meta_contin == NULL) {
		LOG_ERR("No metadata");
		return -ENXIO;
	}

	meta_finite = net_buf_user_data(pcm_finite);
	if (meta_finite == NULL) {
		LOG_ERR("No metadata");
		return -ENXIO;
	}

	if ((meta_contin->sample_rate_hz != meta_finite->sample_rate_hz) ||
	    (meta_contin->bits_per_sample != meta_finite->bits_per_sample) ||
	    (meta_contin->carried_bits_per_sample != meta_finite->carried_bits_per_sample) ||
	    audio_metadata_num_loc_get(meta_finite) > 1) {
		LOG_ERR("Sample/Carrier/Finite locations mismatch");
		return -EINVAL;
	}

	return contin_array_buf_create(pcm_contin, (void const *const)pcm_finite->data,
				       meta_finite->bytes_per_location, locations, _finite_pos);
}
