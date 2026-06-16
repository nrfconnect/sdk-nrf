/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#define DT_DRV_COMPAT nordic_nrf_scan

#include <string.h>

#include <zephyr/drivers/can.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/sys/util.h>

#include <nrf_config_scan.h>
#include <nrf_scan.h>
#include <hal/nrf_spu.h>
#include <hal/nrf_memconf.h>
#include <softperipheral_regif.h>

LOG_MODULE_REGISTER(can_nrf_scan, CONFIG_CAN_LOG_LEVEL);

#define VPR_NODE DT_NODELABEL(cpuflpr_vpr)

#define RX_FILTER_COUNT NRF_SCAN_RXFILTER_MAX_BUFFER_SIZE

#define BITRATE_MIN 10000U
#define BITRATE_MAX 1000000U

struct driver_rx_filter {
	can_rx_callback_t cb;
	void *user_data;
	struct can_filter filter;
	bool active;
};

struct driver_data {
	struct can_driver_data common;
	const struct device *dev;
	nrf_scan_context_t scan_context;
	nrf_scan_timing_t timing;
	struct k_sem ctx_lock;
	struct k_sem tx_sem;
	bool timing_set;
	enum can_state last_state;
	can_tx_callback_t tx_cb;
	void *tx_cb_data;
	struct driver_rx_filter filters[RX_FILTER_COUNT];
};

struct driver_config {
	struct can_driver_config common;
	nrf_scan_t scan;
	const struct pinctrl_dev_config *pcfg;
};

static int scan_err_to_errno(nrf_scan_error_t err)
{
	switch (err) {
	case NRF_SCAN_SUCCESS:
		return 0;
	case NRF_SCAN_ERROR_BUSY:
		return -EBUSY;
	case NRF_SCAN_ERROR_INVALID_PARAM:
		return -EINVAL;
	case NRF_SCAN_ERROR_UNSUPPORTED:
		return -ENOTSUP;
	case NRF_SCAN_ERROR_INVALID_STATE:
		return -EIO;
	case NRF_SCAN_ERROR_ACK_ERROR:
		return -EIO;
	default:
		return -EIO;
	}
}

static enum can_state scan_state_to_zephyr(nrf_scan_state_t state, bool started)
{
	if (!started) {
		return CAN_STATE_STOPPED;
	}

	switch (state) {
	case NRF_SCAN_STATE_ERROR_WARNING:
		return CAN_STATE_ERROR_WARNING;
	case NRF_SCAN_STATE_ERROR_PASSIVE:
		return CAN_STATE_ERROR_PASSIVE;
	case NRF_SCAN_STATE_BUS_OFF:
		return CAN_STATE_BUS_OFF;
	case NRF_SCAN_STATE_STOPPED:
		return CAN_STATE_STOPPED;
	case NRF_SCAN_STATE_ERROR_ACTIVE:
	default:
		return CAN_STATE_ERROR_ACTIVE;
	}
}

static void scan_frame_to_zephyr(const nrf_scan_frame_t *src, struct can_frame *dst)
{
	memset(dst, 0, sizeof(*dst));

	dst->id = src->identifier;
	dst->dlc = src->data_length;

	if (src->ide != 0) {
		dst->flags |= CAN_FRAME_IDE;
	}

	if (src->rtr != 0) {
		dst->flags |= CAN_FRAME_RTR;
	} else {
		memcpy(dst->data, src->data, src->data_length);
	}
}

static void zephyr_frame_to_scan(const struct can_frame *src, nrf_scan_frame_t *dst)
{
	memset(dst, 0, sizeof(*dst));

	dst->identifier = src->id;
	dst->data_length = src->dlc;
	dst->ide = (src->flags & CAN_FRAME_IDE) ? 1U : 0U;
	dst->rtr = (src->flags & CAN_FRAME_RTR) ? 1U : 0U;

	if (dst->rtr == 0U) {
		memcpy(dst->data, src->data, can_dlc_to_bytes(src->dlc));
	}
}

static int tx_status_get(const struct device *dev)
{
	const struct driver_config *dev_config = dev->config;
	struct driver_data *dev_data = dev->data;
	nrf_scan_status_t status;

	status = nrf_scan_get_status(&dev_config->scan);

	if (status.state == NRF_SCAN_STATE_BUS_OFF) {
		return -ENETUNREACH;
	}

	if (status.error == NRF_SCAN_SUCCESS) {
		return 0;
	}

	if (dev_data->common.mode & CAN_MODE_ONE_SHOT) {
		if (status.error == NRF_SCAN_ERROR_ACK_ERROR) {
			return -EIO;
		}

		return -EBUSY;
	}

	return 0;
}

static void update_stats(const struct device *dev, nrf_scan_error_t err)
{
#ifdef CONFIG_CAN_STATS
	switch (err) {
	case NRF_SCAN_ERROR_BIT_STUFFING_ERROR:
		CAN_STATS_STUFF_ERROR_INC(dev);
		break;
	case NRF_SCAN_ERROR_BIT0_ERROR:
		CAN_STATS_BIT0_ERROR_INC(dev);
		break;
	case NRF_SCAN_ERROR_BIT1_ERROR:
		CAN_STATS_BIT1_ERROR_INC(dev);
		break;
	case NRF_SCAN_ERROR_FORM_ERROR:
		CAN_STATS_FORM_ERROR_INC(dev);
		break;
	case NRF_SCAN_ERROR_ACK_ERROR:
		CAN_STATS_ACK_ERROR_INC(dev);
		break;
	case NRF_SCAN_ERROR_CRC_ERROR:
		CAN_STATS_CRC_ERROR_INC(dev);
		break;
	default:
		break;
	}
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(err);
#endif
}

static void notify_state_change(const struct device *dev)
{
	struct driver_data *dev_data = dev->data;
	const struct driver_config *dev_config = dev->config;
	enum can_state state;
	nrf_scan_status_t status;

	if (dev_data->common.state_change_cb == NULL) {
		return;
	}

	status = nrf_scan_get_status(&dev_config->scan);
	state = scan_state_to_zephyr(status.state, dev_data->common.started);

	if (state == dev_data->last_state) {
		return;
	}

	dev_data->last_state = state;
	dev_data->common.state_change_cb(dev, state, (struct can_bus_err_cnt){0},
					 dev_data->common.state_change_cb_user_data);
}

#ifdef CONFIG_CAN_MANUAL_RECOVERY_MODE
static int abort_scan(const struct device *dev)
{
	const struct driver_config *dev_config = dev->config;

	return scan_err_to_errno(nrf_scan_abort(&dev_config->scan));
}
#endif

static int enable_scan(const struct device *dev)
{
	const struct driver_config *dev_config = dev->config;

	return scan_err_to_errno(nrf_scan_enable(&dev_config->scan));
}

static int disable_scan(const struct device *dev)
{
	const struct driver_config *dev_config = dev->config;

	return scan_err_to_errno(nrf_scan_disable(&dev_config->scan));
}

static int set_scan_mode(const struct device *dev, nrf_scan_mode_type_t *mode)
{
	const struct driver_config *dev_config = dev->config;
	nrf_scan_error_t err;

	err = nrf_scan_mode(&dev_config->scan, mode);
	return scan_err_to_errno(err);
}

static int apply_scan_mode(const struct device *dev)
{
	struct driver_data *dev_data = dev->data;
	nrf_scan_mode_type_t mode;

	mode = NRF_SCAN_MODE_NORMAL;

	if (dev_data->common.mode & CAN_MODE_LOOPBACK) {
		mode = NRF_SCAN_MODE_LOOPBACK;
	} else if (dev_data->common.mode & CAN_MODE_LISTENONLY) {
		mode = NRF_SCAN_MODE_LISTENONLY;
	} else if (dev_data->common.mode & CAN_MODE_ONE_SHOT) {
		mode = NRF_SCAN_MODE_ONESHOT;
	}

	return set_scan_mode(dev, &mode);
}

static int apply_timing(const struct device *dev)
{
	const struct driver_config *dev_config = dev->config;
	struct driver_data *dev_data = dev->data;
	nrf_scan_error_t err;

	err = nrf_scan_timing(&dev_config->scan, &dev_data->timing);
	if (err != NRF_SCAN_SUCCESS) {
		return scan_err_to_errno(err);
	}

	return 0;
}

static int apply_rx_filter(const struct device *dev, int filter_id)
{
	const struct driver_config *dev_config = dev->config;
	struct driver_data *dev_data = dev->data;
	struct driver_rx_filter *filter;
	nrf_scan_rx_filter_t scan_filter;
	nrf_scan_error_t err;

	filter = &dev_data->filters[filter_id];

	scan_filter.id = filter->filter.id;
	scan_filter.mask = filter->filter.mask;
	scan_filter.extended = filter->filter.flags & CAN_FILTER_IDE;

	err = nrf_scan_set_rx_filter(&dev_config->scan, &scan_filter, (uint8_t)filter_id);
	if (err != NRF_SCAN_SUCCESS) {
		return scan_err_to_errno(err);
	}

	return 0;
}

static int apply_rx_filters(const struct device *dev)
{
	int ret;
	int i;

	for (i = 0; i < RX_FILTER_COUNT; i++) {
		struct driver_data *dev_data = dev->data;

		if (!dev_data->filters[i].active) {
			continue;
		}

		ret = apply_rx_filter(dev, i);
		if (ret) {
			return ret;
		}
	}

	return 0;
}

static void handle_rx_complete(const struct device *dev)
{
	const struct driver_config *dev_config = dev->config;
	struct driver_data *dev_data = dev->data;
	struct driver_rx_filter *filter;
	struct can_frame frame;
	uint8_t mailbox;

	mailbox = dev_data->scan_context.last_updated_mailbox;

	if (mailbox >= RX_FILTER_COUNT) {
		return;
	}

	filter = &dev_data->filters[mailbox];

	if (!filter->active || filter->cb == NULL) {
		(void)nrf_scan_unlock_rx_mailbox(&dev_config->scan, mailbox);
		return;
	}

	if (!m_scan_rx_mailbox[mailbox].ready) {
		return;
	}

	scan_frame_to_zephyr(&m_scan_rx_mailbox[mailbox].rx_frame, &frame);

#ifndef CONFIG_CAN_ACCEPT_RTR
	if ((frame.flags & CAN_FRAME_RTR) != 0U) {
		return;
	}
#endif

	filter->cb(dev, &frame, filter->user_data);
	(void)nrf_scan_unlock_rx_mailbox(&dev_config->scan, mailbox);
}

static void scan_event_handler(nrf_scan_event_type_t const *event, void *context)
{
	struct driver_data *dev_data = CONTAINER_OF(context, struct driver_data, scan_context);
	const struct device *dev = dev_data->dev;
	can_tx_callback_t tx_cb;
	void *tx_cb_data;
	int tx_err;

	switch (*event) {
	case NRF_SCAN_EVT_TX_COMPLETE:
		tx_err = tx_status_get(dev);
		tx_cb = dev_data->tx_cb;
		tx_cb_data = dev_data->tx_cb_data;
		dev_data->tx_cb = NULL;
		dev_data->tx_cb_data = NULL;
		if (tx_cb != NULL) {
			tx_cb(dev, tx_err, tx_cb_data);
		}

		k_sem_give(&dev_data->tx_sem);
		break;
	case NRF_SCAN_EVT_RX_COMPLETE:
		handle_rx_complete(dev);
		break;
	case NRF_SCAN_EVT_ERROR: {
		const struct driver_config *dev_config = dev->config;
		nrf_scan_status_t status;

		status = nrf_scan_get_status(&dev_config->scan);
		update_stats(dev, status.error);
		notify_state_change(dev);
	} break;
	case NRF_SCAN_EVT_STATE_CHANGED:
		notify_state_change(dev);
		break;
	default:
		break;
	}
}

static can_mode_t get_capabilities(void)
{
	can_mode_t cap = CAN_MODE_NORMAL |
			  CAN_MODE_LOOPBACK |
			  CAN_MODE_LISTENONLY |
			  CAN_MODE_ONE_SHOT;

	if (IS_ENABLED(CONFIG_CAN_MANUAL_RECOVERY_MODE)) {
		cap |= CAN_MODE_MANUAL_RECOVERY;
	}

	return cap;
}

static int api_get_capabilities(const struct device *dev, can_mode_t *cap)
{
	ARG_UNUSED(dev);

	*cap = get_capabilities();

	return 0;
}

static int api_get_core_clock(const struct device *dev, uint32_t *rate)
{
	ARG_UNUSED(dev);

	*rate = SP_VPR_BASE_FREQ_HZ;

	return 0;
}

static int api_get_max_filters(const struct device *dev, bool ide)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(ide);

	return RX_FILTER_COUNT;
}

static int api_set_timing(const struct device *dev, const struct can_timing *timing)
{
	struct driver_data *dev_data = dev->data;

	__ASSERT_NO_MSG(timing != NULL);

	if (dev_data->common.started) {
		return -EBUSY;
	}

	memcpy(&dev_data->timing, timing, sizeof(*timing));
	dev_data->timing_set = true;

	return 0;
}

static int api_set_mode(const struct device *dev, can_mode_t mode)
{
	struct driver_data *dev_data = dev->data;
	can_mode_t cap;

	if (dev_data->common.started) {
		return -EBUSY;
	}

	cap = get_capabilities();

	if (mode & ~cap) {
		return -ENOTSUP;
	}

	if ((mode & CAN_MODE_LOOPBACK) && (mode & CAN_MODE_LISTENONLY)) {
		return -ENOTSUP;
	}

	dev_data->common.mode = mode;

	return 0;
}

static int api_start(const struct device *dev)
{
	const struct driver_config *dev_config = dev->config;
	struct driver_data *dev_data = dev->data;
	struct can_timing timing;
	int ret;

	if (dev_data->common.started) {
		return -EALREADY;
	}

	CAN_STATS_RESET(dev);

	if (!dev_data->timing_set) {
		ret = can_calc_timing(dev,
				      &timing,
				      dev_config->common.bitrate,
				      dev_config->common.sample_point);
		if (ret < 0) {
			return ret;
		}

		memcpy(&dev_data->timing, &timing, sizeof(timing));
		dev_data->timing_set = true;
	}

	ret = pm_device_runtime_get(dev);
	if (ret) {
		return ret;
	}

	ret = enable_scan(dev);
	if (ret) {
		return ret;
	}

	ret = apply_rx_filters(dev);
	if (ret) {
		(void)pm_device_runtime_put(dev);
		return ret;
	}

	ret = apply_scan_mode(dev);
	if (ret) {
		(void)pm_device_runtime_put(dev);
		return ret;
	}

	ret = apply_timing(dev);
	if (ret) {
		(void)pm_device_runtime_put(dev);
		return ret;
	}

	dev_data->common.started = true;
	dev_data->last_state = CAN_STATE_ERROR_ACTIVE;

	return 0;
}

static int api_stop(const struct device *dev)
{
	struct driver_data *dev_data = dev->data;
	int ret;

	if (!dev_data->common.started) {
		return -EALREADY;
	}

	ret = disable_scan(dev);
	if (ret) {
		return ret;
	}

	ret = pm_device_runtime_put(dev);
	if (ret) {
		return ret;
	}

	dev_data->common.started = false;
	dev_data->last_state = CAN_STATE_STOPPED;

	if (dev_data->tx_cb != NULL) {
		dev_data->tx_cb = NULL;
		dev_data->tx_cb_data = NULL;
		k_sem_give(&dev_data->tx_sem);
	}

	return 0;
}

static int api_send(const struct device *dev, const struct can_frame *frame,
		    k_timeout_t timeout, can_tx_callback_t callback, void *user_data)
{
	const struct driver_config *dev_config = dev->config;
	struct driver_data *dev_data = dev->data;
	nrf_scan_frame_t scan_frame;
	nrf_scan_error_t err;
	int ret;

	__ASSERT_NO_MSG(frame != NULL);

	if (frame->flags & ~(CAN_FRAME_IDE | CAN_FRAME_RTR)) {
		return -ENOTSUP;
	}

	if (frame->dlc > CAN_MAX_DLC) {
		return -EINVAL;
	}

	if (!dev_data->common.started) {
		return -ENETDOWN;
	}

	if (dev_data->common.mode & CAN_MODE_LISTENONLY) {
		return -ENOTSUP;
	}

	ret = k_sem_take(&dev_data->tx_sem, timeout);
	if (ret) {
		return -EAGAIN;
	}

	dev_data->tx_cb = callback;
	dev_data->tx_cb_data = user_data;

	zephyr_frame_to_scan(frame, &scan_frame);

	err = nrf_scan_send(&dev_config->scan, &scan_frame);
	if (err != NRF_SCAN_SUCCESS) {
		dev_data->tx_cb = NULL;
		dev_data->tx_cb_data = NULL;
		k_sem_give(&dev_data->tx_sem);
		return scan_err_to_errno(err);
	}

	return 0;
}

static int api_add_rx_filter(const struct device *dev, can_rx_callback_t callback,
			     void *user_data, const struct can_filter *filter)
{
	struct driver_data *dev_data = dev->data;
	int filter_id;
	int ret;

	__ASSERT_NO_MSG(callback != NULL);
	__ASSERT_NO_MSG(filter != NULL);

	for (filter_id = 0; filter_id < RX_FILTER_COUNT; filter_id++) {
		if (!dev_data->filters[filter_id].active) {
			break;
		}
	}

	if (filter_id >= RX_FILTER_COUNT) {
		return -ENOSPC;
	}

	dev_data->filters[filter_id].cb = callback;
	dev_data->filters[filter_id].user_data = user_data;
	dev_data->filters[filter_id].filter = *filter;
	dev_data->filters[filter_id].active = true;

	if (dev_data->common.started) {
		ret = apply_rx_filter(dev, filter_id);
		if (ret) {
			dev_data->filters[filter_id].active = false;
			return ret;
		}
	}

	return filter_id;
}

static void api_remove_rx_filter(const struct device *dev, int filter_id)
{
	const struct driver_config *dev_config = dev->config;
	struct driver_data *dev_data = dev->data;

	if (filter_id < 0 || filter_id >= RX_FILTER_COUNT) {
		return;
	}

	if (!dev_data->filters[filter_id].active) {
		return;
	}

	if (dev_data->common.started) {
		(void)nrf_scan_disable_rx_filter(&dev_config->scan, (uint8_t)filter_id);
	}

	dev_data->filters[filter_id].active = false;
	dev_data->filters[filter_id].cb = NULL;
}

static int api_get_state(const struct device *dev, enum can_state *state,
			 struct can_bus_err_cnt *err_cnt)
{
	const struct driver_config *dev_config = dev->config;
	struct driver_data *dev_data = dev->data;
	nrf_scan_status_t status;

	if (!dev_data->common.started) {
		if (state != NULL) {
			*state = CAN_STATE_STOPPED;
		}
	} else {
		status = nrf_scan_get_status(&dev_config->scan);

		if (state != NULL) {
			*state = scan_state_to_zephyr(status.state, true);
		}
	}

	if (err_cnt != NULL) {
		err_cnt->tx_err_cnt = 0;
		err_cnt->rx_err_cnt = 0;
	}

	return 0;
}

#ifdef CONFIG_CAN_MANUAL_RECOVERY_MODE
static int api_recover(const struct device *dev, k_timeout_t timeout)
{
	struct driver_data *dev_data = dev->data;
	enum can_state state;
	int ret;

	ARG_UNUSED(timeout);

	if (!dev_data->common.started) {
		return -ENETDOWN;
	}

	if (!(dev_data->common.mode & CAN_MODE_MANUAL_RECOVERY)) {
		return -ENOTSUP;
	}

	ret = api_get_state(dev, &state, NULL);
	if (ret) {
		return ret;
	}

	if (state != CAN_STATE_BUS_OFF) {
		return 0;
	}

	ret = abort_scan(dev);
	if (ret) {
		return ret;
	}

	ret = enable_scan(dev);
	if (ret) {
		return ret;
	}

	ret = apply_rx_filters(dev);
	if (ret) {
		return ret;
	}

	ret = apply_timing(dev);
	if (ret) {
		return ret;
	}

	ret = apply_scan_mode(dev);
	if (ret) {
		return ret;
	}

	dev_data->last_state = CAN_STATE_ERROR_ACTIVE;

	return 0;
}
#endif /* CONFIG_CAN_MANUAL_RECOVERY_MODE */

static void api_set_state_change_callback(const struct device *dev,
					  can_state_change_callback_t callback,
					  void *user_data)
{
	struct driver_data *dev_data = dev->data;

	dev_data->common.state_change_cb = callback;
	dev_data->common.state_change_cb_user_data = user_data;
}

static int init_scan(const struct device *dev)
{
	struct driver_data *dev_data = dev->data;
	const struct driver_config *dev_config = dev->config;
	nrf_scan_error_t err;

	err = nrf_scan_init(&dev_config->scan, scan_event_handler, &dev_data->scan_context);
	return scan_err_to_errno(err);
}

static void deinit_scan(const struct device *dev)
{
	const struct driver_config *dev_config = dev->config;

	nrf_scan_uninit(&dev_config->scan);
}

static int resume(const struct device *dev)
{
	const struct driver_config *dev_config = dev->config;
	int ret;

	ret = pinctrl_apply_state(dev_config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		return ret;
	}

	ret = init_scan(dev);
	if (ret) {
		return ret;
	}

	nrf_memconf_ramblock_ret_mask_enable_set(NRF_MEMCONF, 1, BIT(0), true);

	return 0;
}

static int suspend(const struct device *dev)
{
	const struct driver_config *dev_config = dev->config;
	int ret;

	ret = pinctrl_apply_state(dev_config->pcfg, PINCTRL_STATE_SLEEP);
	if (ret) {
		return ret;
	}

	deinit_scan(dev);

	nrf_memconf_ramblock_ret_mask_enable_set(NRF_MEMCONF, 1, BIT(0), false);

	return 0;
}

static int driver_pm_action(const struct device *dev, enum pm_device_action action)
{
	int ret;

	switch(action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = resume(dev);
		break;

	case PM_DEVICE_ACTION_SUSPEND:
		ret = suspend(dev);
		break;

	default:
		ret = -ENOTSUP;
	}

	return ret;
}

static int driver_init(const struct device *dev)
{
	struct driver_data *dev_data = dev->data;

	k_sem_init(&dev_data->ctx_lock, 1, 1);
	k_sem_init(&dev_data->tx_sem, 1, 1);

	dev_data->dev = dev;
	dev_data->last_state = CAN_STATE_STOPPED;

#ifndef CONFIG_TRUSTED_EXECUTION_NONSECURE
	nrf_spu_periph_perm_secattr_set(NRF_SPU00,
					nrf_address_slave_get(DT_REG_ADDR(VPR_NODE)),
					true);
#endif

	IRQ_CONNECT(DT_IRQN(VPR_NODE),
		    DT_IRQ(VPR_NODE, priority),
		    nrfx_isr,
		    nrf_scan_irq_handler,
		    0);

	irq_enable(DT_IRQN(VPR_NODE));

	return pm_device_driver_init(dev, driver_pm_action);
}

static DEVICE_API(can, driver_api) = {
	.get_capabilities = api_get_capabilities,
	.start = api_start,
	.stop = api_stop,
	.set_mode = api_set_mode,
	.set_timing = api_set_timing,
	.send = api_send,
	.add_rx_filter = api_add_rx_filter,
	.remove_rx_filter = api_remove_rx_filter,
	.get_state = api_get_state,
	.set_state_change_callback = api_set_state_change_callback,
	.get_core_clock = api_get_core_clock,
	.get_max_filters = api_get_max_filters,
#ifdef CONFIG_CAN_MANUAL_RECOVERY_MODE
	.recover = api_recover,
#endif
	.timing_min = {
		.sjw = 1,
		.prop_seg = 1,
		.phase_seg1 = 1,
		.phase_seg2 = 1,
		.prescaler = 8,
	},
	.timing_max = {
		.sjw = 4,
		.prop_seg = 8,
		.phase_seg1 = 8,
		.phase_seg2 = 16,
		.prescaler = UINT16_MAX,
	},
};

#define DRIVER_DEFINE(inst)									\
	PINCTRL_DT_DEFINE(VPR_NODE);								\
												\
	static struct driver_data CONCAT(data, inst);						\
	static const struct driver_config CONCAT(config, inst) = {				\
		.common = CAN_DT_DRIVER_CONFIG_INST_GET(					\
			inst,									\
			BITRATE_MIN,								\
			BITRATE_MAX								\
		),										\
		.scan = {									\
			.p_reg = (void *)DT_INST_REG_ADDR(inst),				\
			.drv_inst_idx = 0,							\
		},										\
		.pcfg = PINCTRL_DT_DEV_CONFIG_GET(VPR_NODE),					\
	};											\
												\
	PM_DEVICE_DT_INST_DEFINE(inst, driver_pm_action);					\
												\
	CAN_DEVICE_DT_INST_DEFINE(								\
		inst,										\
		driver_init,									\
		PM_DEVICE_DT_INST_GET(inst),							\
		&CONCAT(data, inst),								\
		&CONCAT(config, inst),								\
		POST_KERNEL,									\
		CONFIG_CAN_INIT_PRIORITY,							\
		&driver_api									\
	);

DT_INST_FOREACH_STATUS_OKAY(DRIVER_DEFINE)
