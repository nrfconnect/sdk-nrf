/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include <errno.h>

#include <init.h>
#include <device.h>
#include <drivers/gpio.h>
#include <sys/atomic.h>

#include <hal/nrf_gpio.h>
#include <hal/nrf_gpiote.h>
#include <hal/nrf_radio.h>
#include <helpers/nrfx_gppi.h>
#include <nrfx_gpiote.h>
#include <nrfx_timer.h>

#include "fem.h"
#include "fem_interface.h"

#if defined(CONFIG_HAS_HW_NRF_PPI)
#include <nrfx_ppi.h>
#define gppi_channel_t nrf_ppi_channel_t
#define gppi_channel_alloc nrfx_ppi_channel_alloc
#elif defined(CONFIG_HAS_HW_NRF_DPPIC)
#include <nrfx_dppi.h>
#define gppi_channel_t uint8_t
#define gppi_channel_alloc nrfx_dppi_channel_alloc
#else
#error "No PPI or DPPI"
#endif

#define GENERIC_FEM_TIMER_INSTANCE 2

#define GENERIC_FEM_NODE DT_NODELABEL(nrf_radio_fem)

#define CTX_REG DT_REG_ADDR(DT_PHANDLE(GENERIC_FEM_NODE, ctx_gpios))
#define CTX_PIN DT_GPIO_PIN(GENERIC_FEM_NODE, ctx_gpios)
#define CTX_FLAGS DT_GPIO_FLAGS(GENERIC_FEM_NODE, ctx_gpios)

#define CRX_REG DT_REG_ADDR(DT_PHANDLE(GENERIC_FEM_NODE, crx_gpios))
#define CRX_PIN DT_GPIO_PIN(GENERIC_FEM_NODE, crx_gpios)
#define CRX_FLAGS DT_GPIO_FLAGS(GENERIC_FEM_NODE, crx_gpios)

#define CTX_SETTLE_TIME DT_PROP(GENERIC_FEM_NODE, ctx_settle_time_us)
#define CRX_SETTLE_TIME DT_PROP(GENERIC_FEM_NODE, crx_settle_time_us)

#define GENERIC_FEM_GPIO_POLARITY_GET(_dt_property)                   \
	((DT_GPIO_FLAGS(DT_NODELABEL(nrf_radio_fem), _dt_property) & \
	GPIO_ACTIVE_LOW) ? false : true)

struct gpiote_pin {
	uint32_t abs_pin;
	uint8_t gpiote_channel;
	bool active_high;
};

static struct generic_fem {
	struct gpiote_pin ctx_pin;
	struct gpiote_pin crx_pin;
#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), ant_sel_gpios)
	const struct gpio_dt_spec ant_sel;
#endif
#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), csd_gpios)
	const struct gpio_dt_spec csd;
#endif
#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), cps_gpios)
	const struct gpio_dt_spec cps;
#endif
#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), chl_gpios)
	const struct gpio_dt_spec chl;
#endif
	gppi_channel_t timer_ch;
	gppi_channel_t pin_set_ch;
	gppi_channel_t pin_clr_ch;
	const nrfx_timer_t timer;
} generic_fem_cfg = {
	.timer = NRFX_TIMER_INSTANCE(GENERIC_FEM_TIMER_INSTANCE),
#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), ant_sel_gpios)
	.ant_sel = GPIO_DT_SPEC_GET(GENERIC_FEM_NODE, ant_sel_gpios),
#endif
#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), csd_gpios)
	.csd = GPIO_DT_SPEC_GET(GENERIC_FEM_NODE, csd_gpios),
#endif
#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), cps_gpios)
	.cps = GPIO_DT_SPEC_GET(GENERIC_FEM_NODE, cps_gpios),
#endif
#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), chl_gpios)
	.chl = GPIO_DT_SPEC_GET(GENERIC_FEM_NODE, chl_gpios)
#endif
};

enum generic_fem_trx {
	GENERIC_FEM_TX,
	GENERIC_FEM_RX
};

/* Generic Front-end module (FEM) state */
enum fem_state {
	/* Front-end module Power-Down state */
	FEM_STATE_OFF,

	/* Front-end module is powered and ready for PA/LNA operation
	 * (program state).
	 */
	FEM_STATE_READY,

	/* Front-end module is in PA/LNA state or processes command. */
	FEM_STATE_BUSY
};

static uint32_t activate_evt;
static uint32_t deactivate_evt;

static atomic_t state = ATOMIC_INIT(FEM_STATE_READY);

static uint32_t gpio_port_num_decode(NRF_GPIO_Type *reg)
{
	if (reg == NRF_P0) {
		return 0;
	}
#if CONFIG_HAS_HW_NRF_GPIO1
	else if (reg == NRF_P1) {
		return 1;
	}
#endif /* CONFIG_HAS_HW_NRF_GPIO1 */

	__ASSERT(false, "Unknown GPIO port");
	return 0;
}

static inline nrf_radio_task_t generic_fem_radio_task_get(enum generic_fem_trx dir)
{
	return (dir == GENERIC_FEM_TX) ? NRF_RADIO_TASK_TXEN : NRF_RADIO_TASK_RXEN;
}

static struct gpiote_pin *generic_fem_gpiote_pin_get(enum generic_fem_trx dir)
{
	return (dir == GENERIC_FEM_TX) ? &generic_fem_cfg.ctx_pin : &generic_fem_cfg.crx_pin;
}

static int gpio_config(void)
{
	int err = 0;

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), ant_sel_gpios)
	err = gpio_pin_configure(generic_fem_cfg.ant_sel.port, generic_fem_cfg.ant_sel.pin,
				 generic_fem_cfg.ant_sel.dt_flags | GPIO_OUTPUT_INACTIVE);
	if (err) {
		return err;
	}
#endif /* DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), ant_sel_gpios) */

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), csd_gpios)
	err = gpio_pin_configure(generic_fem_cfg.csd.port, generic_fem_cfg.csd.pin,
				 generic_fem_cfg.csd.dt_flags | GPIO_OUTPUT_INACTIVE);
	if (err) {
		return err;
	}
#endif /* DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), csd_gpios) */

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), cps_gpios)
	err = gpio_pin_configure(generic_fem_cfg.cps.port, generic_fem_cfg.cps.pin,
				 generic_fem_cfg.cps.dt_flags | GPIO_OUTPUT_INACTIVE);
	if (err) {
		return err;
	}

#endif /* DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), cps_gpios) */

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), chl_gpios)
	err = gpio_pin_configure(generic_fem_cfg.chl.port, generic_fem_cfg.chl.pin,
				 generic_fem_cfg.chl.dt_flags | GPIO_OUTPUT_INACTIVE);
	if (err) {
		return err;
	}
#endif /* DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), chl_gpios) */

	return err;
}

static int gpiote_config(void)
{
	/* Configure CTX Front-end module pin */
	generic_fem_cfg.ctx_pin.active_high = GENERIC_FEM_GPIO_POLARITY_GET(ctx-gpios);
	generic_fem_cfg.ctx_pin.abs_pin = NRF_GPIO_PIN_MAP(
						gpio_port_num_decode((NRF_GPIO_Type *) CTX_REG),
						CTX_PIN);

	if (nrfx_gpiote_channel_alloc(&generic_fem_cfg.ctx_pin.gpiote_channel) != NRFX_SUCCESS) {
		return -ENXIO;
	}

	nrf_gpiote_task_configure(NRF_GPIOTE,
			generic_fem_cfg.ctx_pin.gpiote_channel,
			generic_fem_cfg.ctx_pin.abs_pin,
			(nrf_gpiote_polarity_t)GPIOTE_CONFIG_POLARITY_None,
			(nrf_gpiote_outinit_t)!generic_fem_cfg.ctx_pin.active_high);
	nrf_gpiote_task_enable(NRF_GPIOTE, generic_fem_cfg.ctx_pin.gpiote_channel);

	/* Configure CRX Front-end module pin */
	generic_fem_cfg.crx_pin.active_high = GENERIC_FEM_GPIO_POLARITY_GET(crx-gpios);
	generic_fem_cfg.crx_pin.abs_pin = NRF_GPIO_PIN_MAP(
						gpio_port_num_decode((NRF_GPIO_Type *) CRX_REG),
						CRX_PIN);

	if (nrfx_gpiote_channel_alloc(&generic_fem_cfg.crx_pin.gpiote_channel) != NRFX_SUCCESS) {
		return -ENXIO;
	}

	nrf_gpiote_task_configure(NRF_GPIOTE,
			generic_fem_cfg.crx_pin.gpiote_channel,
			generic_fem_cfg.crx_pin.abs_pin,
			(nrf_gpiote_polarity_t)GPIOTE_CONFIG_POLARITY_None,
			(nrf_gpiote_outinit_t)!generic_fem_cfg.crx_pin.active_high);
	nrf_gpiote_task_enable(NRF_GPIOTE, generic_fem_cfg.crx_pin.gpiote_channel);

	return 0;
}

static int gppi_channel_config(void)
{
	if (gppi_channel_alloc(&generic_fem_cfg.pin_set_ch) != NRFX_SUCCESS) {
		return -ENXIO;
	}

	if (gppi_channel_alloc(&generic_fem_cfg.pin_clr_ch) != NRFX_SUCCESS) {
		return -ENXIO;
	}

	if (gppi_channel_alloc(&generic_fem_cfg.timer_ch) != NRFX_SUCCESS) {
		return -ENXIO;
	}

	return 0;
}

static void event_configure(enum generic_fem_trx dir, uint32_t event,
			    bool active,
			    uint32_t active_delay)
{
	const struct gpiote_pin *pin = generic_fem_gpiote_pin_get(dir);
	gppi_channel_t gppi_ch;
	uint32_t task_addr;
	nrf_radio_task_t radio_task = generic_fem_radio_task_get(dir);

	gppi_ch = active ? generic_fem_cfg.pin_set_ch : generic_fem_cfg.pin_clr_ch;
	task_addr = (pin->active_high ^ active) ?
		nrf_gpiote_task_address_get(NRF_GPIOTE,
			nrf_gpiote_clr_task_get(pin->gpiote_channel)) :
		nrf_gpiote_task_address_get(NRF_GPIOTE,
			nrf_gpiote_set_task_get(pin->gpiote_channel));

	if (active) {
		nrf_timer_event_clear(generic_fem_cfg.timer.p_reg,
				      NRF_TIMER_EVENT_COMPARE0);
		nrf_timer_cc_set(generic_fem_cfg.timer.p_reg,
				 NRF_TIMER_CC_CHANNEL0,
				 active_delay);
		nrf_timer_shorts_enable(generic_fem_cfg.timer.p_reg,
					NRF_TIMER_SHORT_COMPARE0_STOP_MASK |
					NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK);


		nrfx_gppi_channel_endpoints_setup(gppi_ch,
			nrf_timer_event_address_get(generic_fem_cfg.timer.p_reg,
						    NRF_TIMER_EVENT_COMPARE0),
			task_addr);
		nrfx_gppi_channels_enable(BIT(gppi_ch));

		if (event == FEM_EXECUTE_NOW) {

			nrf_timer_task_trigger(generic_fem_cfg.timer.p_reg,
					       NRF_TIMER_TASK_START);
			nrf_radio_task_trigger(NRF_RADIO, radio_task);
		} else {
			nrfx_gppi_channel_endpoints_setup(generic_fem_cfg.timer_ch,
				event,
				nrf_timer_task_address_get(
						generic_fem_cfg.timer.p_reg,
						NRF_TIMER_TASK_START));
			nrfx_gppi_fork_endpoint_setup(generic_fem_cfg.timer_ch,
				nrf_radio_task_address_get(NRF_RADIO,
							   radio_task));
			nrfx_gppi_channels_enable(BIT(generic_fem_cfg.timer_ch));
		}

	} else {
		nrfx_gppi_channel_endpoints_setup(gppi_ch, event, task_addr);
		nrfx_gppi_channels_enable(BIT(gppi_ch));
	}
}

static int generic_fem_init(void)
{
	int err;

	/* Configure timer to 1 us resolution. */
	nrf_timer_mode_set(generic_fem_cfg.timer.p_reg, NRF_TIMER_MODE_TIMER);
	nrf_timer_bit_width_set(generic_fem_cfg.timer.p_reg,
				NRF_TIMER_BIT_WIDTH_16);
	nrf_timer_frequency_set(generic_fem_cfg.timer.p_reg, NRF_TIMER_FREQ_1MHz);

	err = gppi_channel_config();
	if (err) {
		return err;
	}

	err = gpio_config();
	if (err) {
		return err;
	}

	return gpiote_config();
}

static int generic_fem_power_up(void)
{
	int err = 0;

	if (!atomic_cas(&state, FEM_STATE_OFF, FEM_STATE_BUSY)) {
		return -EBUSY;
	}


#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), csd_gpios)
	err = gpio_pin_set(generic_fem_cfg.csd.port, generic_fem_cfg.csd.pin, 1);
#endif /* DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), csd_gpios) */

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), cps_gpios)
	if (!err) {
		if (IS_ENABLED(CONFIG_SKYWORKS_BYPASS_MODE)) {
			err = gpio_pin_set(generic_fem_cfg.cps.port, generic_fem_cfg.cps.pin, 1);
		}
	}
#endif /* DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), cps_gpios) */

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), chl_gpios)
	if (!err) {
		if (IS_ENABLED(CONFIG_SKYWORKS_HIGH_POWER_MODE)) {
			err = gpio_pin_set(generic_fem_cfg.chl.port, generic_fem_cfg.chl.pin, 1);
		}
	}
#endif /* DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), chl_gpios) */

	atomic_set(&state, err ? FEM_STATE_OFF : FEM_STATE_READY);

	return err;
}

static int generic_fem_power_down(void)
{
	int err = 0;

	if (!atomic_cas(&state, FEM_STATE_READY, FEM_STATE_BUSY)) {
		return -EBUSY;
	}

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), cps_gpios)
	if (IS_ENABLED(CONFIG_SKYWORKS_BYPASS_MODE)) {
		err = gpio_pin_set(generic_fem_cfg.cps.port, generic_fem_cfg.cps.pin, 0);
	}
#endif /* DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), cps_gpios) */

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), chl_gpios)
	if (!err) {
		if (IS_ENABLED(CONFIG_SKYWORKS_HIGH_POWER_MODE)) {
			err = gpio_pin_set(generic_fem_cfg.chl.port, generic_fem_cfg.chl.pin, 0);
		}
	}
#endif /* DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), chl_gpios) */

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), csd_gpios)
	if (!err) {
		err = gpio_pin_set(generic_fem_cfg.csd.port, generic_fem_cfg.csd.pin, 0);
	}
#endif /* DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), csd_gpios) */

	atomic_set(&state, err ? FEM_STATE_READY : FEM_STATE_OFF);

	return err;
}

static int generic_fem_tx_configure(uint32_t activate_event, uint32_t deactivate_event,
				    uint32_t activation_delay)
{
	if (!atomic_cas(&state, FEM_STATE_READY, FEM_STATE_BUSY)) {
		return -EBUSY;
	}

	activate_evt = activate_event;
	deactivate_evt = deactivate_event;

	/* Configure pin activation event. */
	event_configure(GENERIC_FEM_TX, activate_evt, true, activation_delay);

	if (deactivate_evt != FEM_EVENT_UNUSED) {
		/* Configure pin deactivation event. */
		event_configure(GENERIC_FEM_TX, deactivate_evt, false,
				activation_delay);
	}

	return 0;
}

static int generic_fem_rx_configure(uint32_t activate_event, uint32_t deactivate_event,
				   uint32_t activation_delay)
{

	if (!atomic_cas(&state, FEM_STATE_READY, FEM_STATE_BUSY)) {
		return -EBUSY;
	}

	activate_evt = activate_event;
	deactivate_evt = deactivate_event;

	/* Configure pin activation event. */
	event_configure(GENERIC_FEM_RX, activate_evt, true, activation_delay);

	if (deactivate_evt != FEM_EVENT_UNUSED) {
		/* Configure pin deactivation event. */
		event_configure(GENERIC_FEM_RX, deactivate_evt, false,
				activation_delay);
	}

	return 0;
}

static int generic_fem_txrx_stop(void)
{
	nrf_gpiote_task_force(NRF_GPIOTE,
			      generic_fem_cfg.crx_pin.gpiote_channel,
			      generic_fem_cfg.crx_pin.active_high ?
				NRF_GPIOTE_INITIAL_VALUE_LOW :
				NRF_GPIOTE_INITIAL_VALUE_HIGH);

	nrf_gpiote_task_force(NRF_GPIOTE,
			      generic_fem_cfg.ctx_pin.gpiote_channel,
			      generic_fem_cfg.ctx_pin.active_high ?
				NRF_GPIOTE_INITIAL_VALUE_LOW :
				NRF_GPIOTE_INITIAL_VALUE_HIGH);

	return 0;
}

static void generic_fem_txrx_configuration_clear(void)
{
	nrfx_gppi_channels_disable(BIT(generic_fem_cfg.pin_set_ch));
	nrfx_gppi_event_endpoint_clear(generic_fem_cfg.pin_set_ch,
			nrf_timer_event_address_get(generic_fem_cfg.timer.p_reg,
						    NRF_TIMER_EVENT_COMPARE0));
	nrfx_gppi_task_endpoint_clear(generic_fem_cfg.pin_set_ch,
		nrf_gpiote_task_address_get(NRF_GPIOTE,
			nrf_gpiote_set_task_get(generic_fem_cfg.ctx_pin.gpiote_channel)));

	nrfx_gppi_task_endpoint_clear(generic_fem_cfg.pin_set_ch,
		nrf_gpiote_task_address_get(NRF_GPIOTE,
			nrf_gpiote_set_task_get(generic_fem_cfg.crx_pin.gpiote_channel)));

	if (activate_evt != FEM_EXECUTE_NOW) {
		nrfx_gppi_channels_disable(BIT(generic_fem_cfg.timer_ch));
		nrfx_gppi_event_endpoint_clear(generic_fem_cfg.timer_ch,
					       activate_evt);
		nrfx_gppi_task_endpoint_clear(generic_fem_cfg.timer_ch,
			nrf_timer_task_address_get(generic_fem_cfg.timer.p_reg,
						   NRF_TIMER_TASK_START));
		nrfx_gppi_fork_endpoint_clear(generic_fem_cfg.timer_ch,
			nrf_radio_task_address_get(NRF_RADIO, NRF_RADIO_TASK_RXEN));
		nrfx_gppi_fork_endpoint_clear(generic_fem_cfg.timer_ch,
			nrf_radio_task_address_get(NRF_RADIO, NRF_RADIO_TASK_TXEN));
	}

	if (deactivate_evt != FEM_EVENT_UNUSED) {
		nrfx_gppi_channels_disable(BIT(generic_fem_cfg.pin_clr_ch));
		nrfx_gppi_event_endpoint_clear(generic_fem_cfg.pin_clr_ch, deactivate_evt);
		nrfx_gppi_task_endpoint_clear(generic_fem_cfg.pin_clr_ch,
			nrf_gpiote_task_address_get(NRF_GPIOTE,
				nrf_gpiote_clr_task_get(generic_fem_cfg.ctx_pin.gpiote_channel)));

		nrfx_gppi_task_endpoint_clear(generic_fem_cfg.pin_clr_ch,
			nrf_gpiote_task_address_get(NRF_GPIOTE,
				nrf_gpiote_clr_task_get(generic_fem_cfg.crx_pin.gpiote_channel)));
	}

	atomic_set(&state, FEM_STATE_READY);
}

static int generic_fem_tx_gain_set(uint32_t gain)
{
	return -ENOTSUP;
}

static uint32_t generic_fem_active_delay_calculate(bool rx, nrf_radio_mode_t mode)
{
	bool fast_ramp_up = nrf_radio_modecnf0_ru_get(NRF_RADIO);

	return rx ? (fem_radio_rx_ramp_up_delay_get(fast_ramp_up, mode) -
		     CRX_SETTLE_TIME) :
		    (fem_radio_tx_ramp_up_delay_get(fast_ramp_up, mode) -
		     CTX_SETTLE_TIME);
}

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), ant_sel_gpios)
static int generic_fem_antenna_select(enum fem_antenna ant)
{
	int err = 0;
	uint32_t prev_state;

	prev_state = atomic_set(&state, FEM_STATE_BUSY);

	if (prev_state == FEM_STATE_BUSY) {
		return -EBUSY;
	}

	switch (ant) {
	case FEM_ANTENNA_1:
		err = gpio_pin_set(generic_fem_cfg.ant_sel.port,
				   generic_fem_cfg.ant_sel.pin, 0);
		break;

	case FEM_ANTENNA_2:
		err = gpio_pin_set(generic_fem_cfg.ant_sel.port,
				   generic_fem_cfg.ant_sel.pin, 1);

		break;

	default:
		err = -EINVAL;
		break;
	}

	atomic_set(&state, prev_state);

	return err;
}
#else
static int generic_fem_antenna_select(enum fem_antenna ant)
{
	return -ENOTSUP;
}
#endif /* DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), ant_sel_gpios) */

static const struct fem_interface_api generic_fem_api = {
	.power_up = generic_fem_power_up,
	.power_down = generic_fem_power_down,
	.tx_configure = generic_fem_tx_configure,
	.rx_configure = generic_fem_rx_configure,
	.txrx_configuration_clear = generic_fem_txrx_configuration_clear,
	.txrx_stop = generic_fem_txrx_stop,
	.tx_gain_set = generic_fem_tx_gain_set,
	.default_active_delay_calculate = generic_fem_active_delay_calculate,
	.antenna_select = generic_fem_antenna_select
};

static int generic_fem_setup(const struct device *dev)
{
	int err;

	err = generic_fem_init();
	if (err) {
		return err;
	}

	return fem_interface_api_set(&generic_fem_api);
}

SYS_INIT(generic_fem_setup, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
