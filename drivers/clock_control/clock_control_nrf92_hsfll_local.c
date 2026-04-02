/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

/*
* HSFLL clock driver for nrf92. Configures local HSFLL directly via registers.
* The HSFLL can only run in one frequency.
*/

#define DT_DRV_COMPAT nordic_nrf92_hsfll_local

#include "clock_control_nrf2_common.h"
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>

#include <hal/nrf_hsfll.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(clock_control_nrf2, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

/* TODO: add support for other HSFLLs */
BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "multiple instances not supported");


#define HSFLL_FREQ_HIGH	DT_INST_PROP(0, clock_frequency)

#define HSFLL_REF_CLK_FREQ DT_PROP(DT_INST_CLOCKS_CTLR(0), clock_frequency)

/* Clock options sorted from lowest to highest frequency */
static const struct clock_options {
	uint32_t frequency;
} clock_options[] = {
	{ .frequency = HSFLL_FREQ_HIGH   },
};

struct hsfll_dev_data {
	STRUCT_CLOCK_CONFIG(hsfll, ARRAY_SIZE(clock_options)) clk_cfg;
};

static void hsfll_work_handler(struct k_work *work)
{
	struct hsfll_dev_data *dev_data =
		CONTAINER_OF(work, struct hsfll_dev_data, clk_cfg.work);
	NRF_HSFLL_Type *hsfll = (NRF_HSFLL_Type *)DT_INST_REG_ADDR(0);
	uint8_t to_activate_idx;

	to_activate_idx = clock_config_update_begin(work);

	nrf_hsfll_clkctrl_mult_set(hsfll,
				   clock_options[to_activate_idx].frequency /
				   HSFLL_REF_CLK_FREQ);

	/* Trigger frequency change twice as required by the hardware. */
	nrf_hsfll_task_trigger(hsfll, NRF_HSFLL_TASK_FREQ_CHANGE);
	nrf_hsfll_task_trigger(hsfll, NRF_HSFLL_TASK_FREQ_CHANGE);

	/* Poll until the frequency change is complete. */
	while (!nrf_hsfll_event_check(hsfll, NRF_HSFLL_EVENT_FREQ_CHANGED)) {
	}
	nrf_hsfll_event_clear(hsfll, NRF_HSFLL_EVENT_FREQ_CHANGED);

	clock_config_update_end(&dev_data->clk_cfg, 0);
}

static int hsfll_resolve_spec_to_idx(const struct nrf_clock_spec *req_spec)
{
	uint32_t req_frequency;

	if (req_spec->accuracy || req_spec->precision) {
		LOG_ERR("invalid specification of accuracy or precision");
		return -EINVAL;
	}

	req_frequency = req_spec->frequency == NRF_CLOCK_CONTROL_FREQUENCY_MAX
		      ? HSFLL_FREQ_HIGH
		      : req_spec->frequency;

	for (int i = 0; i < ARRAY_SIZE(clock_options); ++i) {
		if (req_frequency > clock_options[i].frequency) {
			continue;
		}

		return i;
	}

	LOG_ERR("invalid frequency");
	return -EINVAL;
}

static void hsfll_get_spec_by_idx(uint8_t idx, struct nrf_clock_spec *spec)
{
	spec->frequency = clock_options[idx].frequency;
	spec->accuracy = 0;
	spec->precision = 0;
}

static struct onoff_manager *hsfll_get_mgr_by_idx(const struct device *dev, uint8_t idx)
{
	struct hsfll_dev_data *dev_data = dev->data;

	return &dev_data->clk_cfg.onoff[idx].mgr;
}

static struct onoff_manager *hsfll_find_mgr_by_spec(const struct device *dev,
						    const struct nrf_clock_spec *spec)
{
	int idx;

	if (!spec) {
		return hsfll_get_mgr_by_idx(dev, 0);
	}

	idx = hsfll_resolve_spec_to_idx(spec);
	return idx < 0 ? NULL : hsfll_get_mgr_by_idx(dev, idx);
}

static int api_request_hsfll(const struct device *dev,
			     const struct nrf_clock_spec *spec,
			     struct onoff_client *cli)
{
	struct onoff_manager *mgr = hsfll_find_mgr_by_spec(dev, spec);

	if (mgr) {
		return clock_config_request(mgr, cli);
	}

	return -EINVAL;
}

static int api_release_hsfll(const struct device *dev,
			     const struct nrf_clock_spec *spec)
{
	struct onoff_manager *mgr = hsfll_find_mgr_by_spec(dev, spec);

	if (mgr) {
		return onoff_release(mgr);
	}

	return -EINVAL;
}

static int api_cancel_or_release_hsfll(const struct device *dev,
				       const struct nrf_clock_spec *spec,
				       struct onoff_client *cli)
{
	struct onoff_manager *mgr = hsfll_find_mgr_by_spec(dev, spec);

	if (mgr) {
		return onoff_cancel_or_release(mgr, cli);
	}

	return -EINVAL;
}

static int api_resolve_hsfll(const struct device *dev,
			     const struct nrf_clock_spec *req_spec,
			     struct nrf_clock_spec *res_spec)
{
	int idx;

	idx = hsfll_resolve_spec_to_idx(req_spec);
	if (idx < 0) {
		return -EINVAL;
	}

	hsfll_get_spec_by_idx(idx, res_spec);
	return 0;
}

static int hsfll_init(const struct device *dev)
{
	struct hsfll_dev_data *dev_data = dev->data;
	int rc;

	rc = clock_config_init(&dev_data->clk_cfg,
			       ARRAY_SIZE(dev_data->clk_cfg.onoff),
			       hsfll_work_handler);
	if (rc < 0) {
		return rc;
	}

	clock_config_update_end(&dev_data->clk_cfg, rc);

	return 0;
}

static DEVICE_API(nrf_clock_control, hsfll_drv_api) = {
	.std_api = {
		.on = api_nosys_on_off,
		.off = api_nosys_on_off,
	},
	.request = api_request_hsfll,
	.release = api_release_hsfll,
	.cancel_or_release = api_cancel_or_release_hsfll,
	.resolve = api_resolve_hsfll,
};

static struct hsfll_dev_data hsfll_data;

DEVICE_DT_INST_DEFINE(0, hsfll_init, NULL,
		      &hsfll_data,
		      NULL,
		      PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		      &hsfll_drv_api);
