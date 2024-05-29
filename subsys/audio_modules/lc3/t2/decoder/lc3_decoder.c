/*
 * Copyright(c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "audio_modules/lc3_decoder.h"

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <errno.h>

#include "audio_defines.h"
#include "audio_module/audio_module.h"
#include "LC3API.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(t2_lc3_decoder, CONFIG_AUDIO_MODULE_LC3_DECODER_LOG_LEVEL);

/**
 * @brief Number of micro seconds in a second.
 *
 */
#define LC3_DECODER_US_IN_A_SECOND (1000000)

/**
 * @brief Interleave a channel into a buffer of N channels of PCM
 *
 * @param[in]	input			Pointer to the single channel input buffer.
 * @param[in]	input_size		Number of bytes in input. Must be divisible by two.
 * @param[in]	channel			Channel to interleave into.
 * @param[in]	pcm_bit_depth	Bit depth of PCM samples (8, 16, 24, or 32).
 * @param[out]	output			Pointer to the output start of the multi-channel output.
 * @param[in]	output_size		Number of bytes in output. Must be divisible by two and
 *                              at least (input_size * bytes_per_sample * output_channels).
 * @param[out]	output_channels	Number of output channels in the output buffer.
 *
 * @return 0 if successful, error value
 */
static int interleave(void const *const input, size_t input_size, uint8_t channel,
		      uint8_t pcm_bit_depth, void *output, size_t output_size,
		      uint8_t output_channels)
{
	uint32_t samples_per_channel;
	uint8_t bytes_per_sample = pcm_bit_depth / 8;
	size_t step;
	char *pointer_input;
	char *pointer_output;

	if (output_size < (input_size * output_channels)) {
		LOG_DBG("Output buffer too small to interleave input into");
		return -EINVAL;
	}

	samples_per_channel = input_size / bytes_per_sample;
	step = bytes_per_sample * (output_channels - 1);
	pointer_input = (char *)input;
	pointer_output = (char *)output + (bytes_per_sample * channel);

	for (uint32_t i = 0; i < samples_per_channel; i++) {
		for (uint8_t j = 0; j < bytes_per_sample; j++) {
			*pointer_output++ = *pointer_input++;
		}

		pointer_output += step;
	}

	return 0;
}

/**
 * @brief  Open the LC3 module.
 *
 * @param handle         A pointer to the audio module's handle.
 * @param configuration  A pointer to the audio module's configuration to set.
 *
 * @return 0 if successful, error value
 */
static int lc3_dec_t2_open(struct audio_module_handle_private *handle,
			   struct audio_module_configuration const *const configuration)
{
	struct audio_module_handle *hdl = (struct audio_module_handle *)handle;
	struct lc3_decoder_context *ctx = (struct lc3_decoder_context *)hdl->context;

	/* Clear the context */
	memset(ctx, 0, sizeof(struct lc3_decoder_context));

	LOG_DBG("Open LC3 decoder module");

	return 0;
}

/**
 * @brief  Close the LC3 audio module.
 *
 * @param handle  A pointer to the audio module's handle.
 *
 * @return 0 if successful, error value
 */
static int lc3_dec_t2_close(struct audio_module_handle_private *handle)
{
	struct audio_module_handle *hdl = (struct audio_module_handle *)handle;
	struct lc3_decoder_context *ctx = (struct lc3_decoder_context *)hdl->context;
	LC3DecoderHandle_t *dec_handles = (LC3DecoderHandle_t *)&ctx->lc3_dec_channel[0];
	int8_t number_channels;

	audio_module_number_channels_calculate(ctx->config.locations, &number_channels);

	/* Close decoder sessions */
	for (uint8_t i = 0; i < number_channels; i++) {
		if (dec_handles[i] != NULL) {
			LC3DecodeSessionClose(dec_handles[i]);
			dec_handles = NULL;
		}
	}

	LOG_DBG("Close LC3 decoder module");

	return 0;
}

/**
 * @brief  Set the configuration of an audio module.
 *
 * @param handle         A pointer to the audio module's handle.
 * @param configuration  A pointer to the audio module's configuration to set.
 *
 * @return 0 if successful, error value
 */
static int
lc3_dec_t2_configuration_set(struct audio_module_handle_private *handle,
			     struct audio_module_configuration const *const configuration)
{
	int ret;
	struct audio_module_handle *hdl = (struct audio_module_handle *)handle;
	struct lc3_decoder_context *ctx = (struct lc3_decoder_context *)hdl->context;
	struct lc3_decoder_configuration *config =
		(struct lc3_decoder_configuration *)configuration;
	LC3DecoderHandle_t *dec_handles = (LC3DecoderHandle_t *)&ctx->lc3_dec_channel[0];
	LC3FrameSize_t framesize;
	uint16_t coded_bytes_req;
	int8_t number_channels;

	/* Need to validate the config parameters here before proceeding */

	audio_module_number_channels_calculate(config->locations, &number_channels);

	/* Free previous decoder memory */
	for (uint8_t i = 0; i < number_channels; i++) {
		if (dec_handles[i] != NULL) {
			LC3DecodeSessionClose(dec_handles[i]);
			dec_handles[i] = NULL;
		}
	}

	switch (config->data_len_us) {
	case 7500:
		framesize = LC3FrameSize7_5Ms;
		break;
	case 10000:
		framesize = LC3FrameSize10Ms;
		break;
	default:
		LOG_ERR("Unsupported framesize: %d", config->data_len_us);
		return -EINVAL;
	}

	coded_bytes_req = LC3BitstreamBuffersize(config->sample_rate_hz, config->bitrate_bps_max,
						 framesize, &ret);
	if (coded_bytes_req == 0) {
		LOG_ERR("Required coded bytes to LC3 instance %s is zero", hdl->name);
		return -EPERM;
	}

	for (uint8_t i = 0; i < number_channels; i++) {
		dec_handles[i] =
			LC3DecodeSessionOpen(config->sample_rate_hz, config->bits_per_sample,
					     framesize, NULL, NULL, &ret);
		if (ret) {
			LOG_ERR("LC3 decoder channel %d failed to initialise for module %s", i,
				hdl->name);
			return ret;
		}

		LOG_DBG("LC3 decode module %s session %d: %dus %dbits", hdl->name, i,
			config->data_len_us, config->bits_per_sample);

		ctx->dec_handles_count += 1;
	}

	memcpy(&ctx->config, config, sizeof(struct lc3_decoder_configuration));

	ctx->coded_bytes_req = coded_bytes_req;
	ctx->sample_frame_bytes = ((ctx->config.data_len_us * ctx->config.sample_rate_hz) /
				   LC3_DECODER_US_IN_A_SECOND) *
				  (ctx->config.carried_bits_pr_sample / 8);

	LOG_DBG("LC3 decode module %s requires %d coded bytes to produce %d decoded sample bytes",
		hdl->name, ctx->coded_bytes_req, ctx->sample_frame_bytes);

	/* Configure decoder */
	LOG_DBG("LC3 decode module %s configuration: %d Hz %d bits (sample bits %d) "
		"%d us %d "
		"channel(s)",
		hdl->name, ctx->config.sample_rate_hz, ctx->config.carried_bits_pr_sample,
		ctx->config.bits_per_sample, ctx->config.data_len_us, number_channels);

	return 0;
}

/**
 * @brief  Get the current configuration of an audio module.
 *
 * @param handle         A pointer to the audio module's handle.
 * @param configuration  A pointer to the audio module's current configuration.
 *
 * @return 0 if successful, error value
 */
static int lc3_dec_t2_configuration_get(struct audio_module_handle_private const *const handle,
					struct audio_module_configuration *configuration)
{
	struct audio_module_handle *hdl = (struct audio_module_handle *)handle;
	struct lc3_decoder_context *ctx = (struct lc3_decoder_context *)hdl->context;
	struct lc3_decoder_configuration *config =
		(struct lc3_decoder_configuration *)configuration;
	memcpy(config, &ctx->config, sizeof(struct lc3_decoder_configuration));

	/* Configure decoder */
	LOG_DBG("LC3 decode module %s configuration: %dHz %dbits (sample bits %d) %dus channel(s) "
		"mapped as 0x%X",
		hdl->name, config->sample_rate_hz, config->carried_bits_pr_sample,
		config->bits_per_sample, config->data_len_us, config->locations);

	return 0;
}

/**
 * @brief This processes the input audio data into the output audio data.
 *
 * @param handle	      A handle to this audio module's instance
 * @param audio_data_in   Pointer to the input audio data or NULL for an input module
 * @param audio_data_out  Pointer to the output audio data or NULL for an output module
 *
 * @return 0 if successful, error value
 */
static int lc3_dec_t2_data_process(struct audio_module_handle_private *handle,
				   struct audio_data const *const audio_data_in,
				   struct audio_data *audio_data_out)
{
	int ret;
	struct audio_module_handle *hdl = (struct audio_module_handle *)handle;
	struct lc3_decoder_context *ctx = (struct lc3_decoder_context *)hdl->context;
	LC3DecoderHandle_t *dec_handles = (LC3DecoderHandle_t *)&ctx->lc3_dec_channel[0];
	LC3BFI_t frame_status;
	uint16_t plc_counter = 0;
	size_t data_out_size;
	size_t session_in_size;
	uint8_t *data_in;
	uint8_t *data_out;
	uint8_t temp_pcm[LC3_DECODER_PCM_NUM_BYTES_MONO];
	int8_t number_channels;

	LOG_DBG("LC3 decoder module %s start process", hdl->name);

	if (audio_data_in->meta.data_coding != LC3) {
		LOG_DBG("LC3 decoder module %s has incorrect input data type: %d", hdl->name,
			audio_data_in->meta.data_coding);
		return -EINVAL;
	}

	if (ctx->config.locations != audio_data_in->meta.locations) {
		LOG_DBG("LC3 decoder module %s has incorrect channel map in the new audio_data: %d",
			hdl->name, audio_data_in->meta.locations);
		return -EINVAL;
	}

	if (audio_data_in->meta.bad_data) {
		frame_status = BadFrame;
	} else {
		frame_status = GoodFrame;
		ctx->plc_count = 0;
	}

	audio_module_number_channels_calculate(ctx->config.locations, &number_channels);

	if (audio_data_in->data_size) {
		session_in_size = audio_data_in->data_size / number_channels;
		if (session_in_size < ctx->coded_bytes_req) {
			LOG_ERR("Too few coded bytes to decode. Bytes required %d, input framesize "
				"is %d",
				ctx->coded_bytes_req, session_in_size);
			return -EINVAL;
		}
	} else {
		session_in_size = 0;
	}

	if (audio_data_out->data_size < ctx->sample_frame_bytes * number_channels) {
		LOG_ERR("Output buffer too small. Bytes required %d, output buffer is %d",
			(ctx->sample_frame_bytes * number_channels), audio_data_out->data_size);
		return -EINVAL;
	}

	if (ctx->config.interleaved) {
		data_out = temp_pcm;
	} else {
		data_out = (uint8_t *)audio_data_out->data;
	}

	data_out_size = 0;

	/* Should be able to decode only the channel(s) of interest here.
	 * These will be put in the first channel or channels and the location
	 * will indicate which channel(s) they are. Prior to playout (I2S or TDM)
	 * all other channels can be zeroed.
	 */
	for (uint8_t chan = 0; chan < number_channels; chan++) {
		data_in = (uint8_t *)audio_data_in->data + (session_in_size * chan);

		LC3DecodeInput_t LC3DecodeInput = {
			.inputData = data_in, .inputDataLength = session_in_size, frame_status};
		LC3DecodeOutput_t LC3DecodeOutput = {.PCMData = data_out,
						     .PCMDataLength = ctx->sample_frame_bytes,
						     .bytesWritten = 0,
						     .PLCCounter = plc_counter};

		if (dec_handles[chan] == NULL) {
			LOG_DBG("LC3 dec ch: %d is not initialized", chan);
			return -EINVAL;
		}

		ret = LC3DecodeSessionData(dec_handles[chan], &LC3DecodeInput, &LC3DecodeOutput);
		if (ret) {
			/* handle error */
			LOG_DBG("Error in decoder, ret: %d", ret);
			return ret;
		}

		if (LC3DecodeOutput.bytesWritten != ctx->sample_frame_bytes) {
			/* handle error */
			LOG_DBG("Error in decoder, output incorrect size %d when should "
				"be %d",
				LC3DecodeOutput.bytesWritten, ctx->sample_frame_bytes);

			/* Clear this channel as it is not correct */
			memset(data_out, 0, LC3DecodeOutput.bytesWritten);

			return -EFAULT;
		}

		/* Could also perform the resampling here, while we are operating
		 * on this area of memory.
		 */

		if (ctx->config.interleaved) {
			uint8_t bits_per_sample =
				audio_data_in->meta.bits_per_sample <
						audio_data_in->meta.carried_bits_pr_sample
					? audio_data_in->meta.carried_bits_pr_sample
					: audio_data_in->meta.bits_per_sample;

			ret = interleave(LC3DecodeOutput.PCMData, LC3DecodeOutput.bytesWritten,
					 chan, bits_per_sample, audio_data_out->data,
					 audio_data_out->data_size, number_channels);

			if (ret) {
				LOG_DBG("Failed to interleave output");
				return ret;
			}

			LOG_DBG("Completed decoders PCM interleaving for ch: %d", chan);
		} else {
			data_out += LC3DecodeOutput.bytesWritten;
		}

		data_out_size += LC3DecodeOutput.bytesWritten;

		LOG_DBG("Completed LC3 decode of ch: %d", chan);
	}

	ctx->plc_count = plc_counter;

	audio_data_out->data_size = data_out_size;

	return 0;
}

/**
 * @brief Table of the LC3 decoder module functions.
 */
struct audio_module_functions lc3_dec_t2_functions = {
	/**
	 * @brief  Function to an open the LC3 decoder module.
	 */
	.open = lc3_dec_t2_open,

	/**
	 * @brief  Function to close the LC3 decoder module.
	 */
	.close = lc3_dec_t2_close,

	/**
	 * @brief  Function to set the configuration of the LC3 decoder module.
	 */
	.configuration_set = lc3_dec_t2_configuration_set,

	/**
	 * @brief  Function to get the configuration of the LC3 decoder module.
	 */
	.configuration_get = lc3_dec_t2_configuration_get,

	/**
	 * @brief Start a module processing data.
	 */
	.start = NULL,

	/**
	 * @brief Pause a module processing data.
	 */
	.stop = NULL,

	/**
	 * @brief The core data processing function in the LC3 decoder module.
	 */
	.data_process = lc3_dec_t2_data_process};

/**
 * @brief The set-up parameters for the LC3 decoder.
 */
struct audio_module_description lc3_dec_t2_dept = {
	.name = "LC3 Decoder (T2)",
	.type = AUDIO_MODULE_TYPE_IN_OUT,
	.functions = (struct audio_module_functions *)&lc3_dec_t2_functions};

/**
 * @brief A private pointer to the LC3 decoder set-up parameters.
 */
struct audio_module_description *lc3_decoder_description = &lc3_dec_t2_dept;
