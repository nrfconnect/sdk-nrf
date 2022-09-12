/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <bluetooth/adv_prov.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_le_adv_prov, CONFIG_BT_ADV_PROV_LOG_LEVEL);

enum provider_set {
	PROVIDER_SET_AD,
	PROVIDER_SET_SD
};


static void get_section_ptrs(enum provider_set set,
			     const struct bt_le_adv_prov_provider **start,
			     const struct bt_le_adv_prov_provider **end)
{
	switch (set) {
	case PROVIDER_SET_AD:
		{
			extern const struct bt_le_adv_prov_provider _bt_le_adv_prov_ad_list_start[];
			extern const struct bt_le_adv_prov_provider _bt_le_adv_prov_ad_list_end[];

			*start = _bt_le_adv_prov_ad_list_start;
			*end = _bt_le_adv_prov_ad_list_end;
		}
		break;

	case PROVIDER_SET_SD:
		{
			extern const struct bt_le_adv_prov_provider _bt_le_adv_prov_sd_list_start[];
			extern const struct bt_le_adv_prov_provider _bt_le_adv_prov_sd_list_end[];

			*start = _bt_le_adv_prov_sd_list_start;
			*end = _bt_le_adv_prov_sd_list_end;
		}
		break;

	default:
		/* Should not happen. */
		__ASSERT_NO_MSG(false);
		*start = NULL;
		*end = NULL;
		break;
	}
}

static size_t get_section_size(enum provider_set set)
{
	const struct bt_le_adv_prov_provider *start;
	const struct bt_le_adv_prov_provider *end;

	get_section_ptrs(set, &start, &end);

	return end - start;
}

size_t bt_le_adv_prov_get_ad_prov_cnt(void)
{
	return get_section_size(PROVIDER_SET_AD);
}

size_t bt_le_adv_prov_get_sd_prov_cnt(void)
{
	return get_section_size(PROVIDER_SET_SD);
}

static void update_common_feedback(struct bt_le_adv_prov_feedback *common_fb,
				   const struct bt_le_adv_prov_feedback *fb)
{
	common_fb->grace_period_s = MAX(common_fb->grace_period_s, fb->grace_period_s);
}

static int get_providers_data(enum provider_set set, struct bt_data *d, size_t *d_len,
			      const struct bt_le_adv_prov_adv_state *state,
			      struct bt_le_adv_prov_feedback *fb)
{
	const struct bt_le_adv_prov_provider *start;
	const struct bt_le_adv_prov_provider *end;
	struct bt_le_adv_prov_feedback common_fb;

	size_t pos = 0;
	int err = 0;

	if (*d_len < get_section_size(set)) {
		return -EINVAL;
	}

	get_section_ptrs(set, &start, &end);
	memset(&common_fb, 0, sizeof(common_fb));

	for (const struct bt_le_adv_prov_provider *p = start; p < end; p++) {
		memset(fb, 0, sizeof(*fb));
		err = p->get_data(&d[pos], state, fb);

		if (!err) {
			pos++;
			update_common_feedback(&common_fb, fb);
		} else if (err == -ENOENT) {
			err = 0;
		} else {
			break;
		}
	}

	if (!err) {
		*d_len = pos;
		memcpy(fb, &common_fb, sizeof(common_fb));
	}

	return err;
}

int bt_le_adv_prov_get_ad(struct bt_data *ad, size_t *ad_len,
			  const struct bt_le_adv_prov_adv_state *state,
			  struct bt_le_adv_prov_feedback *fb)
{
	return get_providers_data(PROVIDER_SET_AD, ad, ad_len, state, fb);
}

int bt_le_adv_prov_get_sd(struct bt_data *sd, size_t *sd_len,
			  const struct bt_le_adv_prov_adv_state *state,
			  struct bt_le_adv_prov_feedback *fb)
{
	return get_providers_data(PROVIDER_SET_SD, sd, sd_len, state, fb);
}
