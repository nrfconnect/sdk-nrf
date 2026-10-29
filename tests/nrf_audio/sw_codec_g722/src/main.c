/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <sample_rate_converter.h>
#include "sw_codec_select.h"
#include "g722_encoder.h"
#include "g722_decoder.h"

/* One mono 10 ms frame: 160 int16 samples at 16 kHz, encoded at 64 kbit/s. */
#define PCM_BYTES   320
#define CODED_BYTES 80
#define POOL_BYTES  2048
#define LEFT	    BT_AUDIO_LOCATION_FRONT_LEFT
#define RIGHT	    BT_AUDIO_LOCATION_FRONT_RIGHT

/* Tests hold up to three frames at once (PCM, encoded, decoded); six leaves three spare buffers.
 * This is test-pool capacity, not a G.722 channel or frame-size requirement.
 */
NET_BUF_POOL_FIXED_DEFINE(frames, 6, POOL_BYTES, sizeof(struct audio_metadata), NULL);

static int allocations, live_encoders, live_decoders, fail_allocation;
static int converter_opens, fail_converter;

G722_ENC_CTX *__real_g722_encoder_new(int rate, int options);
G722_DEC_CTX *__real_g722_decoder_new(int rate, int options);
int __real_g722_encoder_destroy(G722_ENC_CTX *ctx);
int __real_g722_decoder_destroy(G722_DEC_CTX *ctx);
int __real_sample_rate_converter_open(struct sample_rate_converter_ctx *ctx);

/**
 * @brief Inject encoder allocation failures and track ownership.
 *
 * @param[in]	rate	Library bitrate in bit/s.
 * @param[in]	options	Library mode flags.
 *
 * @return	Library context, or NULL at the injected failure index.
 */
G722_ENC_CTX *__wrap_g722_encoder_new(int rate, int options)
{
	if (++allocations == fail_allocation) {
		return NULL;
	}

	G722_ENC_CTX *ctx = __real_g722_encoder_new(rate, options);

	live_encoders += ctx != NULL;

	return ctx;
}

/**
 * @brief Inject decoder allocation failures and track ownership.
 *
 * @param[in]	rate	Library bitrate in bit/s.
 * @param[in]	options	Library mode flags.
 *
 * @return	Library context, or NULL at the injected failure index.
 */
G722_DEC_CTX *__wrap_g722_decoder_new(int rate, int options)
{
	if (++allocations == fail_allocation) {
		return NULL;
	}

	G722_DEC_CTX *ctx = __real_g722_decoder_new(rate, options);

	live_decoders += ctx != NULL;

	return ctx;
}

/**
 * @brief Check encoder ownership before forwarding destruction.
 *
 * @param[in]	ctx	Non-NULL live encoder context.
 *
 * @return	Library destruction result.
 */
int __wrap_g722_encoder_destroy(G722_ENC_CTX *ctx)
{
	zassert_not_null(ctx);
	zassert_true(live_encoders > 0);
	live_encoders--;

	return __real_g722_encoder_destroy(ctx);
}

/**
 * @brief Check decoder ownership before forwarding destruction.
 *
 * @param[in]	ctx	Non-NULL live decoder context.
 *
 * @return Library destruction result.
 */
int __wrap_g722_decoder_destroy(G722_DEC_CTX *ctx)
{
	zassert_not_null(ctx);
	zassert_true(live_decoders > 0);
	live_decoders--;

	return __real_g722_decoder_destroy(ctx);
}

/**
 * @brief Inject converter initialization failures without replacing processing.
 *
 * @param[in,out]	ctx	Converter context passed to the real initializer.
 *
 * @return	-EIO at the injected index; otherwise the real initializer result.
 */
int __wrap_sample_rate_converter_open(struct sample_rate_converter_ctx *ctx)
{
	if (++converter_opens == fail_converter) {
		return -EIO;
	}

	return __real_sample_rate_converter_open(ctx);
}

/**
 * @brief Build a fixed-profile codec configuration for a test.
 *
 * @param[in]	encoder		Enable encoder direction.
 * @param[in]	decoder		Enable decoder direction.
 * @param[in]	channels	Requested contexts per enabled direction.
 *
 * @return Configuration with 16 kHz PCM and 64 kbit/s encoding.
 */
static struct sw_codec_config config(bool encoder, bool decoder, uint8_t channels)
{
	return (struct sw_codec_config){
		.sw_codec = SW_CODEC_G722,
		.encoder = {.enabled = encoder,
			    .num_ch = channels,
			    .sample_rate_hz = 16000,
			    .bitrate = 64000},
		.decoder = {.enabled = decoder, .num_ch = channels, .sample_rate_hz = 16000},
	};
}

/**
 * @brief Allocate an empty, zero-filled test frame with 10 ms metadata.
 *
 * @param[in]	coding		PCM or G722 payload type.
 * @param[in]	locations	Audio location mask; zero represents mono.
 * @param[in]	rate		Sample rate in Hz.
 *
 * @return	Test buffer owned by the caller; allocation failure fails the test.
 */
static struct net_buf *frame(enum coding coding, uint32_t locations, uint32_t rate)
{
	struct net_buf *buf = net_buf_alloc(&frames, K_NO_WAIT);
	struct audio_metadata *meta;

	zassert_not_null(buf);
	meta = net_buf_user_data(buf);
	*meta = (struct audio_metadata){
		.data_coding = coding,
		.data_len_us = 10000,
		.sample_rate_hz = rate,
		.bits_per_sample = 16,
		.carried_bits_per_sample = 16,
		.bitrate_bps = 64000,
		.bytes_per_location = coding == PCM ? rate / 100 * 2 : CODED_BYTES,
		.locations = locations,
	};

	memset(buf->data, 0, POOL_BYTES);

	return buf;
}

/**
 * @brief Populate planar PCM with distinct left/right square-wave patterns.
 * @param[in,out]	buf	Empty PCM frame with capacity for its declared channels.
 */
static void fill_pcm(struct net_buf *buf)
{
	struct audio_metadata *meta = net_buf_user_data(buf);
	size_t samples = meta->bytes_per_location / sizeof(int16_t);
	int16_t *pcm = (int16_t *)net_buf_add(buf, meta->bytes_per_location *
							   audio_metadata_num_loc_get(meta));

	for (size_t i = 0; i < samples; i++) {
		pcm[i] = (i % 32 < 16) ? 6000 : -6000;

		if (audio_metadata_num_loc_get(meta) == 2) {
			pcm[samples + i] = (i % 20 < 10) ? -3000 : 3000;
		}
	}
}

/**
 * @brief Release codec directions and assert that each test leaves no live contexts.
 * @param[in]	fixture	Unused ztest fixture.
 */
static void after(void *fixture)
{
	if (sw_codec_is_initialized()) {
		zassert_ok(sw_codec_uninit(config(live_encoders != 0, live_decoders != 0, 2)));
	}

	zassert_equal(live_encoders, 0, "Encoder leaked");
	zassert_equal(live_decoders, 0, "Decoder leaked");

	allocations = fail_allocation = converter_opens = fail_converter = 0;
}

/** @brief Verify incremental init and atomic teardown validation across repeated cycles. */
ZTEST(g722, test_repeated_lifecycle_and_directions)
{
	for (int i = 0; i < 10; i++) {
		zassert_ok(sw_codec_init(config(true, false, 2)));
		zassert_ok(sw_codec_init(config(false, true, 2)));
		zassert_equal(sw_codec_init(config(true, true, 2)), -EALREADY);

		zassert_ok(sw_codec_uninit(config(true, false, 2)));
		zassert_true(sw_codec_is_initialized());
		zassert_equal(sw_codec_uninit(config(true, true, 2)), -EALREADY);
		zassert_equal(live_decoders, 2, "Failed teardown must be atomic");

		zassert_ok(sw_codec_uninit(config(false, true, 2)));
		zassert_false(sw_codec_is_initialized());
	}
}

/** @brief Exercise each acquisition failure and preserve an existing opposite direction. */
ZTEST(g722, test_allocation_and_converter_rollback)
{
	for (int i = 1; i <= 4; i++) {
		allocations = 0;
		fail_allocation = i;

		zassert_equal(sw_codec_init(config(true, true, 2)), -ENOMEM);
		zassert_equal(live_encoders + live_decoders, 0);
		zassert_false(sw_codec_is_initialized());
	}

	fail_allocation = 0;

	for (int i = 1; i <= 4; i++) {
		converter_opens = 0;
		fail_converter = i;

		zassert_equal(sw_codec_init(config(true, true, 2)), -EIO);
		zassert_equal(live_encoders + live_decoders, 0);
		zassert_false(sw_codec_is_initialized());
	}

	fail_converter = 0;

	zassert_ok(sw_codec_init(config(false, true, 1)));

	allocations = 0;
	fail_allocation = 2;

	zassert_equal(sw_codec_init(config(true, false, 2)), -ENOMEM);
	zassert_equal(live_encoders, 0);
	zassert_equal(live_decoders, 1, "Existing direction must survive rollback");
}

/** @brief Reject invalid initialization before acquiring any codec contexts. */
ZTEST(g722, test_invalid_init_before_allocation)
{
	struct sw_codec_config cfg = config(true, true, 2);

	cfg.decoder.sample_rate_hz = 24000;
	zassert_equal(sw_codec_init(cfg), -EINVAL);

	cfg = config(true, true, 0);
	zassert_equal(sw_codec_init(cfg), -EINVAL);

	cfg = config(true, true, 3);
	zassert_equal(sw_codec_init(cfg), -EINVAL);

	cfg = config(true, true, 1);
	cfg.encoder.bitrate = 56000;
	zassert_equal(sw_codec_init(cfg), -EINVAL);
	zassert_equal(sw_codec_init(config(false, false, 1)), -EINVAL);
	zassert_equal(allocations, 0);

	cfg.sw_codec = SW_CODEC_NONE;
	zassert_equal(sw_codec_init(cfg), -ENODEV);
}

/** @brief Report independent setup errors without allocation or changes to existing directions. */
ZTEST(g722, test_multiple_init_errors_preserve_state)
{
	struct sw_codec_config cfg = config(true, true, 0);

	cfg.encoder.sample_rate_hz = 24000;
	cfg.encoder.bitrate = 56000;
	cfg.decoder.sample_rate_hz = 48000;

	zassert_equal(sw_codec_init(cfg), -EINVAL);
	zassert_equal(allocations, 0);
	zassert_equal(converter_opens, 0);
	zassert_false(sw_codec_is_initialized());

	zassert_ok(sw_codec_init(config(true, true, 1)));
	int init_allocations = allocations;
	int init_converter_opens = converter_opens;

	zassert_equal(sw_codec_init(cfg), -EINVAL);
	zassert_equal(sw_codec_init(config(true, true, 1)), -EALREADY);
	zassert_equal(allocations, init_allocations);
	zassert_equal(converter_opens, init_converter_opens);
	zassert_equal(live_encoders, 1);
	zassert_equal(live_decoders, 1);
	zassert_true(sw_codec_is_initialized());
}

/** @brief Compare consecutive frames to a real reference encoder without frame allocations. */
ZTEST(g722, test_exact_lengths_continuity_and_no_frame_allocations)
{
	zassert_ok(sw_codec_init(config(true, true, 1)));

	int init_allocations = allocations;
	G722_ENC_CTX *reference = __real_g722_encoder_new(64000, G722_DEFAULT);
	uint8_t expected[CODED_BYTES];

	zassert_not_null(reference);

	for (int i = 0; i < 20; i++) {
		struct net_buf *pcm = frame(PCM, LEFT, 16000);
		struct net_buf *coded = frame(G722, LEFT, 16000);
		struct net_buf *decoded = frame(PCM, LEFT, 16000);

		fill_pcm(pcm);
		zassert_equal(g722_encode(reference, (int16_t *)pcm->data, 160, expected), 80);

		zassert_ok(sw_codec_encode(pcm, coded));
		zassert_equal(coded->len, CODED_BYTES);
		zassert_mem_equal(coded->data, expected, CODED_BYTES);

		zassert_ok(sw_codec_decode(coded, decoded));
		zassert_equal(decoded->len, PCM_BYTES);
		zassert_equal(allocations, init_allocations);

		net_buf_unref(pcm);
		net_buf_unref(coded);
		net_buf_unref(decoded);
	}

	__real_g722_encoder_destroy(reference);
}

/** @brief Reject invalid PCM metadata and insufficient tailroom without touching output. */
ZTEST(g722, test_encode_validation_and_tailroom)
{
	struct net_buf *pcm = frame(PCM, LEFT, 16000);
	struct net_buf *coded = frame(G722, LEFT, 16000);
	struct audio_metadata *meta = net_buf_user_data(pcm);

	zassert_ok(sw_codec_init(config(true, false, 1)));
	fill_pcm(pcm);

	meta->bytes_per_location--;
	zassert_equal(sw_codec_encode(pcm, coded), -EINVAL);

	meta->bytes_per_location++;
	meta->sample_rate_hz = 24000;
	zassert_equal(sw_codec_encode(pcm, coded), -EINVAL);

	meta->sample_rate_hz = 16000;
	meta->carried_bits_per_sample = 32;
	zassert_equal(sw_codec_encode(pcm, coded), -EINVAL);

	meta->carried_bits_per_sample = 16;
	meta->data_len_us = 7500;
	zassert_equal(sw_codec_encode(pcm, coded), -EINVAL);

	meta->data_len_us = 10000;
	net_buf_reserve(coded, POOL_BYTES - CODED_BYTES + 1);
	memset(coded->data, 0xa5, CODED_BYTES - 1);
	zassert_equal(sw_codec_encode(pcm, coded), -EINVAL);

	for (int i = 0; i < CODED_BYTES - 1; i++) {
		zassert_equal(coded->data[i], 0xa5);
	}

	zassert_equal(coded->len, 0);

	net_buf_unref(pcm);
	net_buf_unref(coded);
}

/** @brief Report simultaneous metadata errors without writing encoded output. */
ZTEST(g722, test_multiple_metadata_errors)
{
	struct net_buf *pcm = frame(PCM, LEFT, 16000);
	struct net_buf *coded = frame(G722, LEFT, 16000);
	struct audio_metadata *pcm_meta = net_buf_user_data(pcm);
	struct audio_metadata *coded_meta = net_buf_user_data(coded);

	zassert_ok(sw_codec_init(config(true, false, 1)));
	fill_pcm(pcm);

	pcm_meta->data_len_us = 7500;
	pcm_meta->sample_rate_hz = 24000;
	pcm_meta->bits_per_sample = 24;
	pcm_meta->carried_bits_per_sample = 32;
	coded_meta->sample_rate_hz = 48000;
	coded_meta->bytes_per_location = 60;
	coded_meta->bitrate_bps = 32000;
	coded_meta->locations = LEFT | RIGHT;

	zassert_equal(sw_codec_encode(pcm, coded), -EINVAL);
	zassert_equal(coded->len, 0);

	net_buf_unref(pcm);
	net_buf_unref(coded);
}

/** @brief Reject invalid locations without diagnosing channel counts derived from them. */
ZTEST(g722, test_invalid_locations_skip_channel_counts)
{
	struct net_buf *pcm = frame(PCM, LEFT, 16000);
	struct net_buf *coded = frame(G722, LEFT, 16000);
	struct audio_metadata *pcm_meta = net_buf_user_data(pcm);
	struct audio_metadata *coded_meta = net_buf_user_data(coded);

	zassert_ok(sw_codec_init(config(true, false, 1)));
	fill_pcm(pcm);

	pcm_meta->locations = LEFT | RIGHT | BIT(31);
	coded_meta->locations = LEFT | RIGHT | BIT(31);
	zassert_equal(sw_codec_encode(pcm, coded), -EINVAL);
	zassert_equal(coded->len, 0);

	net_buf_unref(pcm);
	net_buf_unref(coded);
}

/** @brief Exercise mono-zero and sparse-location routing through encode/decode. */
ZTEST(g722, test_mono_zero_location_and_sparse_routing)
{
	const uint32_t locations[] = {0, RIGHT, BT_AUDIO_LOCATION_TOP_BACK_RIGHT};

	zassert_ok(sw_codec_init(config(true, true, 1)));

	for (int i = 0; i < ARRAY_SIZE(locations); i++) {
		struct net_buf *pcm = frame(PCM, locations[i], 16000);
		struct net_buf *coded = frame(G722, 0, 16000);
		struct net_buf *decoded = frame(PCM, 0, 16000);

		fill_pcm(pcm);
		zassert_ok(sw_codec_encode(pcm, coded));
		zassert_equal(coded->len, 80);

		zassert_ok(sw_codec_decode(coded, decoded));
		zassert_equal(decoded->len, 320);

		net_buf_unref(pcm);
		net_buf_unref(coded);
		net_buf_unref(decoded);
	}
}

/** @brief Duplicate zero-location mono decode to every active physical output channel. */
ZTEST(g722, test_zero_location_mono_duplicates_to_physical_outputs)
{
	struct net_buf *pcm = frame(PCM, LEFT, 16000);
	struct net_buf *coded = frame(G722, 0, 16000);
	struct net_buf *decoded = frame(PCM, LEFT | RIGHT, 16000);
	struct audio_metadata *coded_meta = net_buf_user_data(coded);
	struct audio_metadata *decoded_meta = net_buf_user_data(decoded);

	zassert_ok(sw_codec_init(config(true, true, 1)));
	fill_pcm(pcm);
	zassert_ok(sw_codec_encode(pcm, coded));

	zassert_ok(sw_codec_decode(coded, decoded));
	zassert_equal(decoded->len, 2 * PCM_BYTES);
	zassert_mem_equal(decoded->data, decoded->data + PCM_BYTES, PCM_BYTES);

	bool decoded_audio_nonzero = false;

	for (size_t i = 0; i < PCM_BYTES; i++) {
		decoded_audio_nonzero |= decoded->data[i] != 0;
	}

	zassert_true(decoded_audio_nonzero, "Expected decoded mono audio");

	net_buf_reset(decoded);
	decoded_meta->interleaved = true;
	zassert_ok(sw_codec_decode(coded, decoded));
	zassert_equal(decoded->len, 2 * PCM_BYTES);

	int16_t *interleaved = (int16_t *)decoded->data;

	for (size_t sample = 0; sample < PCM_BYTES / sizeof(int16_t); sample++) {
		zassert_equal(interleaved[2 * sample], interleaved[2 * sample + 1]);
	}

	net_buf_reset(decoded);
	decoded_meta->interleaved = false;
	coded_meta->bad_data = 1;
	coded->len = 0;
	zassert_ok(sw_codec_decode(coded, decoded));
	zassert_equal(decoded->len, 2 * PCM_BYTES);

	for (size_t i = 0; i < decoded->len; i++) {
		zassert_equal(decoded->data[i], 0);
	}

	net_buf_unref(pcm);
	net_buf_unref(coded);
	net_buf_unref(decoded);
}

/** @brief Select the second planar input channel while using encoder context zero. */
ZTEST(g722, test_right_channel_extraction)
{
	struct net_buf *pcm = frame(PCM, LEFT | RIGHT, 16000);
	struct net_buf *coded = frame(G722, RIGHT, 16000);
	uint8_t expected[80];
	G722_ENC_CTX *reference = __real_g722_encoder_new(64000, G722_DEFAULT);

	zassert_not_null(reference);
	zassert_ok(sw_codec_init(config(true, false, 1)));
	fill_pcm(pcm);
	g722_encode(reference, (int16_t *)(pcm->data + PCM_BYTES), 160, expected);

	zassert_ok(sw_codec_encode(pcm, coded));
	zassert_equal(coded->len, 80);
	zassert_mem_equal(coded->data, expected, 80);

	__real_g722_encoder_destroy(reference);
	net_buf_unref(pcm);
	net_buf_unref(coded);
}

/** @brief Exercise interleaved stereo and the configured mono-duplication policy. */
ZTEST(g722, test_stereo_independent_state_and_interleaving)
{
	struct net_buf *pcm = frame(PCM, LEFT | RIGHT, 16000);
	struct net_buf *coded = frame(G722, LEFT | RIGHT, 16000);
	struct net_buf *decoded = frame(PCM, LEFT | RIGHT, 16000);
	struct audio_metadata *out_meta = net_buf_user_data(decoded);
	int16_t planar[320];

	zassert_ok(sw_codec_init(config(true, true, 2)));
	fill_pcm(pcm);
	memcpy(planar, pcm->data, sizeof(planar));

	for (int i = 0; i < 160; i++) {
		((int16_t *)pcm->data)[2 * i] = planar[i];
		((int16_t *)pcm->data)[2 * i + 1] = planar[160 + i];
	}

	((struct audio_metadata *)net_buf_user_data(pcm))->interleaved = true;

	zassert_ok(sw_codec_encode(pcm, coded));
	zassert_equal(coded->len, 160);

	if (IS_ENABLED(CONFIG_MONO_TO_ALL_RECEIVERS)) {
		zassert_mem_equal(coded->data, coded->data + 80, 80);
	} else {
		zassert_not_equal(memcmp(coded->data, coded->data + 80, 80), 0);
	}

	out_meta->interleaved = true;
	zassert_ok(sw_codec_decode(coded, decoded));
	zassert_equal(decoded->len, 640);

	net_buf_unref(pcm);
	net_buf_unref(coded);
	net_buf_unref(decoded);
}

/** @brief Verify full-frame silence after SRC loss and successful subsequent decoding. */
ZTEST(g722, test_loss_after_src_is_exact_silence_and_recovers)
{
	struct net_buf *pcm = frame(PCM, LEFT, 48000);
	struct net_buf *coded = frame(G722, LEFT, 16000);
	struct net_buf *decoded = frame(PCM, LEFT, 48000);
	struct audio_metadata *meta = net_buf_user_data(coded);

	/* Match audio_datapath's I2S output metadata, initially one 1 ms block. */
	((struct audio_metadata *)net_buf_user_data(decoded))->bytes_per_location = 96;
	zassert_ok(sw_codec_init(config(true, true, 1)));
	fill_pcm(pcm);
	zassert_ok(sw_codec_encode(pcm, coded));
	zassert_ok(sw_codec_decode(coded, decoded));
	zassert_equal(decoded->len, 960);

	net_buf_reset(decoded);
	meta->bad_data = LEFT;
	coded->len = 0;

	zassert_ok(sw_codec_decode(coded, decoded));
	zassert_equal(decoded->len, 960);

	for (int i = 0; i < decoded->len; i++) {
		zassert_equal(decoded->data[i], 0);
	}

	net_buf_reset(decoded);
	meta->bad_data = 0;
	zassert_equal(sw_codec_decode(coded, decoded), -EINVAL);

	coded->len = 80;
	zassert_ok(sw_codec_decode(coded, decoded));
	zassert_equal(decoded->len, 960);

	net_buf_unref(pcm);
	net_buf_unref(coded);
	net_buf_unref(decoded);
}

/** @brief Decode a partial aggregate while preserving its received channel. */
ZTEST(g722, test_partial_stereo_aggregate_marks_missing_channel_bad)
{
	int16_t samples[160];
	const uint32_t channel_locations[] = {LEFT, RIGHT};
	G722_ENC_CTX *reference = __real_g722_encoder_new(64000, G722_DEFAULT);

	zassert_not_null(reference);
	zassert_ok(sw_codec_init(config(false, true, 2)));

	for (size_t i = 0; i < ARRAY_SIZE(samples); i++) {
		samples[i] = (i % 32 < 16) ? 6000 : -6000;
	}

	for (size_t received_channel = 0; received_channel < ARRAY_SIZE(channel_locations);
	     received_channel++) {
		size_t missing_channel = received_channel == 0 ? 1 : 0;
		size_t received_offset = received_channel * PCM_BYTES;
		size_t missing_offset = missing_channel * PCM_BYTES;
		bool received_channel_nonzero = false;
		struct net_buf *coded = frame(G722, LEFT | RIGHT, 16000);
		struct net_buf *decoded = frame(PCM, LEFT | RIGHT, 16000);
		struct audio_metadata *meta = net_buf_user_data(coded);

		zassert_equal(g722_encode(reference, samples, ARRAY_SIZE(samples),
					  coded->data + received_channel * CODED_BYTES),
			      CODED_BYTES);
		net_buf_add(coded, 2 * CODED_BYTES);
		meta->bad_data = channel_locations[missing_channel];

		zassert_ok(sw_codec_decode(coded, decoded));
		zassert_equal(decoded->len, 2 * PCM_BYTES);

		for (size_t i = 0; i < PCM_BYTES; i++) {
			received_channel_nonzero |= decoded->data[received_offset + i] != 0;
			zassert_equal(decoded->data[missing_offset + i], 0);
		}

		zassert_true(received_channel_nonzero, "Received channel audio must be retained");
		net_buf_unref(coded);
		net_buf_unref(decoded);
	}

	__real_g722_encoder_destroy(reference);

	struct net_buf *coded = frame(G722, LEFT | RIGHT, 16000);
	struct net_buf *decoded = frame(PCM, LEFT | RIGHT, 16000);
	struct audio_metadata *meta = net_buf_user_data(coded);

	net_buf_add(coded, CODED_BYTES);
	meta->bad_data = LEFT;
	zassert_equal(sw_codec_decode(coded, decoded), -EINVAL);

	meta->bytes_per_location = 81;
	zassert_equal(sw_codec_decode(coded, decoded), -EINVAL);

	net_buf_unref(coded);
	net_buf_unref(decoded);
}

/** @brief Reject invalid decode locations, lengths and tailroom without output corruption. */
ZTEST(g722, test_decode_capacity_and_channel_limits)
{
	struct net_buf *coded = frame(G722, LEFT, 16000);
	struct net_buf *decoded = frame(PCM, LEFT, 16000);
	struct audio_metadata *meta = net_buf_user_data(coded);

	zassert_ok(sw_codec_init(config(false, true, 1)));

	zassert_equal(sw_codec_decode(NULL, decoded), -EINVAL);

	net_buf_add(coded, 81);
	zassert_equal(sw_codec_decode(coded, decoded), -EINVAL);

	coded->len = 80;
	meta->locations = BIT(31);
	zassert_equal(sw_codec_decode(coded, decoded), -EINVAL);

	meta->locations = LEFT | RIGHT;
	zassert_equal(sw_codec_decode(coded, decoded), -EINVAL);

	meta->locations = LEFT;
	((struct audio_metadata *)net_buf_user_data(decoded))->bytes_per_location = 32;
	net_buf_reserve(decoded, POOL_BYTES - PCM_BYTES + 1);
	memset(decoded->data, 0x5a, PCM_BYTES - 1);

	zassert_equal(sw_codec_decode(coded, decoded), -EINVAL);

	for (int i = 0; i < PCM_BYTES - 1; i++) {
		zassert_equal(decoded->data[i], 0x5a);
	}

	zassert_equal(decoded->len, 0);

	net_buf_unref(coded);
	net_buf_unref(decoded);
}

/** @brief Bound mono duplication by requested output channels, not configured maximum. */
ZTEST(g722, test_duplicate_capacity_uses_requested_output_channels)
{
	struct net_buf *pcm = frame(PCM, LEFT, 16000);
	struct net_buf *coded = frame(G722, LEFT, 16000);

	zassert_ok(sw_codec_init(config(true, false, 2)));
	fill_pcm(pcm);
	net_buf_reserve(coded, POOL_BYTES - CODED_BYTES);

	zassert_ok(sw_codec_encode(pcm, coded));
	zassert_equal(coded->len, CODED_BYTES);

	net_buf_unref(pcm);
	net_buf_unref(coded);
}

ZTEST_SUITE(g722, NULL, NULL, NULL, after, NULL);
