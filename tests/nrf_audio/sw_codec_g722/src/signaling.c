/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <zephyr/bluetooth/audio/lc3.h>
#include <../subsys/bluetooth/host/conn_internal.h>
#include "le_audio.h"
#include "server_store.h"

/**
 * @brief Create explicit capabilities for the test vendor identity and fixed profile.
 * @return Complete G.722 capability record.
 */
static struct bt_audio_codec_cap capability(void)
{
	return (struct bt_audio_codec_cap)BT_AUDIO_CODEC_CAP(
		BT_HCI_CODING_FORMAT_VS, 0, 1, LE_AUDIO_G722_CAP_DATA,
		BT_AUDIO_CODEC_CAP_LC3_META(BT_AUDIO_CONTEXT_TYPE_MEDIA));
}

/**
 * @brief Adapt a 16_2_1 preset into a complete mono G.722 wire configuration.
 * @return Configuration with test vendor identity, 80 octets and one frame per SDU.
 */
static struct bt_audio_codec_cfg configuration(void)
{
	struct bt_bap_lc3_preset preset = BT_BAP_LC3_UNICAST_PRESET_16_2_1(
		BT_AUDIO_LOCATION_FRONT_LEFT, BT_AUDIO_CONTEXT_TYPE_MEDIA);

	preset.codec_cfg.id = BT_HCI_CODING_FORMAT_VS;
	preset.codec_cfg.cid = 0;
	preset.codec_cfg.vid = 1;

	zassert_true(bt_audio_codec_cfg_set_octets_per_frame(&preset.codec_cfg, 80) >= 0);
	zassert_true(bt_audio_codec_cfg_set_frame_blocks_per_sdu(&preset.codec_cfg, 1) >= 0);

	return preset.codec_cfg;
}

/** @brief Reject mismatched identity/profile fields while accepting mono-zero allocation. */
ZTEST(g722_signaling, test_config_profile_and_identity)
{
	struct bt_audio_codec_cfg cfg = configuration();

	zassert_ok(le_audio_g722_config_check(&cfg));

	cfg.cid++;
	zassert_equal(le_audio_g722_config_check(&cfg), -EINVAL);

	cfg = configuration();
	cfg.vid++;
	zassert_equal(le_audio_g722_config_check(&cfg), -EINVAL);

	cfg = configuration();
	cfg.id = BT_HCI_CODING_FORMAT_LC3;
	zassert_equal(le_audio_g722_config_check(&cfg), -EINVAL);

	cfg = configuration();
	bt_audio_codec_cfg_set_frame_blocks_per_sdu(&cfg, 2);
	zassert_equal(le_audio_g722_config_check(&cfg), -EINVAL);

	cfg = configuration();
	bt_audio_codec_cfg_set_freq(&cfg, BT_AUDIO_CODEC_CFG_FREQ_48KHZ);
	zassert_equal(le_audio_g722_config_check(&cfg), -EINVAL);

	cfg = configuration();
	bt_audio_codec_cfg_set_frame_dur(&cfg, BT_AUDIO_CODEC_CFG_DURATION_7_5);
	zassert_equal(le_audio_g722_config_check(&cfg), -EINVAL);

	cfg = configuration();
	bt_audio_codec_cfg_set_octets_per_frame(&cfg, 60);
	zassert_equal(le_audio_g722_config_check(&cfg), -EINVAL);

	cfg = configuration();
	bt_audio_codec_cfg_set_chan_allocation(&cfg, BT_AUDIO_LOCATION_FRONT_LEFT |
							     BT_AUDIO_LOCATION_FRONT_RIGHT);
	zassert_equal(le_audio_g722_config_check(&cfg), -EINVAL);

	cfg = configuration();
	bt_audio_codec_cfg_set_chan_allocation(&cfg, BT_AUDIO_LOCATION_MONO_AUDIO);
	zassert_ok(le_audio_g722_config_check(&cfg));
}

/** @brief Check all independently invalid profile fields in a single configuration. */
ZTEST(g722_signaling, test_config_multiple_invalid_fields)
{
	struct bt_audio_codec_cfg cfg = configuration();

	cfg.cid++;
	zassert_true(bt_audio_codec_cfg_set_freq(&cfg, BT_AUDIO_CODEC_CFG_FREQ_48KHZ) >= 0);
	zassert_true(bt_audio_codec_cfg_set_frame_dur(&cfg, BT_AUDIO_CODEC_CFG_DURATION_7_5) >= 0);
	zassert_true(bt_audio_codec_cfg_set_octets_per_frame(&cfg, 60) >= 0);
	zassert_true(bt_audio_codec_cfg_set_frame_blocks_per_sdu(&cfg, 2) >= 0);
	zassert_true(bt_audio_codec_cfg_set_chan_allocation(
			     &cfg, BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT) >=
		     0);

	zassert_equal(le_audio_g722_config_check(&cfg), -EINVAL);
}

/** @brief Reject every truncated prefix and malformed LTV length. */
ZTEST(g722_signaling, test_malformed_ltv_and_missing_fields)
{
	struct bt_audio_codec_cfg cfg = configuration();
	struct bt_audio_codec_cap cap = capability();

	for (int len = 0; len < cfg.data_len; len++) {
		struct bt_audio_codec_cfg truncated = cfg;

		truncated.data_len = len;
		zassert_equal(le_audio_g722_config_check(&truncated), -EINVAL);
	}

	for (int len = 0; len < cap.data_len; len++) {
		struct bt_audio_codec_cap truncated = cap;

		truncated.data_len = len;
		zassert_equal(le_audio_g722_cap_check(&truncated), -EINVAL);
	}

	cfg.data[cfg.data_len++] = 1;
	zassert_equal(le_audio_g722_config_check(&cfg), -EINVAL);

	cfg = configuration();
	cfg.data[0] = 0;
	zassert_equal(le_audio_g722_config_check(&cfg), -EINVAL);

	cap.data[0] = 1;
	zassert_equal(le_audio_g722_cap_check(&cap), -EINVAL);
}

/** @brief Require actual peer support instead of repairing incompatible capability fields. */
ZTEST(g722_signaling, test_capabilities_cannot_be_overwritten_to_match)
{
	struct bt_audio_codec_cap cap = capability();
	struct bt_audio_codec_octets_per_codec_frame octets = {.min = 40, .max = 60};

	zassert_ok(le_audio_g722_cap_check(&cap));

	bt_audio_codec_cap_set_octets_per_frame(&cap, &octets);
	zassert_equal(le_audio_g722_cap_check(&cap), -EINVAL);

	cap = capability();
	bt_audio_codec_cap_set_frame_dur(&cap, BT_AUDIO_CODEC_CAP_DURATION_7_5);
	zassert_equal(le_audio_g722_cap_check(&cap), -EINVAL);

	cap = capability();
	bt_audio_codec_cap_set_freq(&cap, BT_AUDIO_CODEC_CAP_FREQ_48KHZ);
	zassert_equal(le_audio_g722_cap_check(&cap), -EINVAL);

	cap = capability();
	bt_audio_codec_cap_set_supported_audio_chan_counts(
		&cap, BT_AUDIO_CODEC_CAP_CHAN_COUNT_SUPPORT(2));
	zassert_equal(le_audio_g722_cap_check(&cap), -EINVAL);
}

/** @brief Report independent capability mismatches after structural validation. */
ZTEST(g722_signaling, test_capabilities_multiple_invalid_fields)
{
	struct bt_audio_codec_cap cap = capability();
	struct bt_audio_codec_octets_per_codec_frame octets = {.min = 40, .max = 60};

	cap.cid++;
	zassert_true(bt_audio_codec_cap_set_freq(&cap, BT_AUDIO_CODEC_CAP_FREQ_48KHZ) >= 0);
	zassert_true(bt_audio_codec_cap_set_frame_dur(&cap, BT_AUDIO_CODEC_CAP_DURATION_7_5) >= 0);
	zassert_true(bt_audio_codec_cap_set_supported_audio_chan_counts(
			     &cap, BT_AUDIO_CODEC_CAP_CHAN_COUNT_SUPPORT(2)) >= 0);
	zassert_true(bt_audio_codec_cap_set_octets_per_frame(&cap, &octets) >= 0);
	zassert_true(bt_audio_codec_cap_set_max_codec_frames_per_sdu(&cap, 0) >= 0);

	zassert_equal(le_audio_g722_cap_check(&cap), -EINVAL);
}

/** @brief Stop before parsing absent or oversized capability/configuration data. */
ZTEST(g722_signaling, test_invalid_structure_stops_field_parsing)
{
	struct bt_audio_codec_cap cap = capability();
	struct bt_audio_codec_cfg cfg = configuration();

	zassert_equal(le_audio_g722_cap_check(NULL), -EINVAL);
	zassert_equal(le_audio_g722_config_check(NULL), -EINVAL);

	cap.data_len = sizeof(cap.data) + 1;
	cfg.data_len = sizeof(cfg.data) + 1;
	zassert_equal(le_audio_g722_cap_check(&cap), -EINVAL);
	zassert_equal(le_audio_g722_config_check(&cfg), -EINVAL);
}

/** @brief Verify fixed-profile preset adaptation for both sink and source capabilities. */
ZTEST(g722_signaling, test_preset_adaptation_both_directions)
{
	struct bt_conn conn = {.type = BT_CONN_TYPE_LE, .state = BT_CONN_STATE_CONNECTED};
	struct bt_audio_codec_cap cap = capability();
	struct server_store *server;
	uint32_t valid;
	const enum bt_audio_dir dirs[] = {BT_AUDIO_DIR_SINK, BT_AUDIO_DIR_SOURCE};

	/* Multi-rate peers must still select the fixed 16 kHz G.722 profile. */
	bt_audio_codec_cap_set_freq(&cap,
				    BT_AUDIO_CODEC_CAP_FREQ_16KHZ | BT_AUDIO_CODEC_CAP_FREQ_48KHZ);
	zassert_ok(srv_store_lock(K_NO_WAIT));
	zassert_ok(srv_store_init());
	zassert_ok(srv_store_add_by_conn(&conn));

	for (int i = 0; i < ARRAY_SIZE(dirs); i++) {
		zassert_ok(srv_store_codec_cap_set(&conn, dirs[i], &cap));
		zassert_ok(srv_store_valid_codec_cap_check(&conn, dirs[i], &valid, NULL, 0));
		zassert_equal(valid, 1);
		zassert_ok(srv_store_from_conn_get(&conn, &server));

		struct bt_bap_lc3_preset *preset =
			i == 0 ? &server->snk.lc3_preset[0] : &server->src.lc3_preset[0];

		/* The client assigns the endpoint's location when configuring its stream. */
		struct bt_audio_codec_cfg wire_cfg = preset->codec_cfg;

		bt_audio_codec_cfg_set_chan_allocation(&wire_cfg, BT_AUDIO_LOCATION_FRONT_LEFT);
		zassert_ok(le_audio_g722_config_check(&wire_cfg));
		zassert_equal(preset->qos.sdu, 80);
		zassert_equal(preset->qos.interval, 10000);
	}

	zassert_ok(srv_store_remove_all(true));
	srv_store_unlock();
}

/** @brief Ensure zero-location mono loss produces a nonzero bad-data mask. */
ZTEST(g722_signaling, test_mono_loss_metadata_is_not_zero)
{
	struct bt_audio_codec_cfg cfg = configuration();
	struct bt_bap_stream stream = {.codec_cfg = &cfg};
	struct bt_iso_recv_info info = {0};
	struct net_buf buf = {0};
	struct audio_metadata meta = {0};

	bt_audio_codec_cfg_set_chan_allocation(&cfg, BT_AUDIO_LOCATION_MONO_AUDIO);
	zassert_ok(le_audio_metadata_populate(&meta, &stream, &info, &buf));
	zassert_equal(meta.bad_data, 1);
	zassert_equal(meta.bytes_per_location, 80);
	zassert_equal(meta.data_coding, G722);
}

ZTEST_SUITE(g722_signaling, NULL, NULL, NULL, NULL, NULL);
