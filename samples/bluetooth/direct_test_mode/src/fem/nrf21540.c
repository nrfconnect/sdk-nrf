/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <string.h>

#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/pm/device.h>
#include <soc.h>
#include <zephyr/sys/__assert.h>

#include <hal/nrf_gpio.h>
#include <hal/nrf_gpiote.h>
#include <hal/nrf_radio.h>
#include <hal/nrf_uarte.h>
#include <helpers/nrfx_gppi.h>
#include <nrfx_timer.h>
#include <nrfx_gpiote.h>

#if defined(NRF5340_XXAA_NETWORK)
#include <nrfx_spim.h>
#endif

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

#include "fem_interface.h"

/* nRF21540 Front-End-Module maximum gain register value */
#define NRF21540_TX_GAIN_Max 31

#define NRF21540_TIMER_INSTANCE 2

#define NRF21540_SPI_LENGTH_BYTES 2
#define NRF21540_SPI_COMMAND_ADDR_BYTE 0
#define NRF21540_SPI_DATA_BYTE 1
#define NRF21540_SPI_COMMAND_Pos 6
#define NRF21540_SPI_COMMAND_READ 0x02
#define NRF21540_SPI_COMMAND_WRITE 0x03

#define NRF21540_BITS_CONFREG0_TX_GAIN_Pos 2
#define NRF21540_BITS_CONFREG0_TX_GAIN_Msk \
	(0x1F << NRF21540_BITS_CONFREG0_TX_GAIN_Pos)

#define NRF21540_SPI_WRITE(_reg) \
	((NRF21540_SPI_COMMAND_WRITE << NRF21540_SPI_COMMAND_Pos) | (_reg))

#define NRF21540_SPI_READ(_reg) \
	((NRF21540_SPI_COMMAND_READ << NRF21540_SPI_COMMAND_Pos) | (_reg))

#define NRF21540_NODE DT_NODELABEL(nrf_radio_fem)

#define PDN_SETTLE_TIME DT_PROP(NRF21540_NODE, pdn_settle_time_us)
#define TX_EN_SETTLE_TIME DT_PROP(NRF21540_NODE, tx_en_settle_time_us)
#define RX_EN_SETTLE_TIME DT_PROP(NRF21540_NODE, rx_en_settle_time_us)

#if defined(NRF5340_XXAA_NETWORK) && CONFIG_NRF21540_FEM_SPI_SUPPORTED
/* NRF5340 Network Core has only one SPIM0 instance. */
static nrfx_spim_t spim = NRFX_SPIM_INSTANCE(0);
#endif /* defined(NRF5340_XXAA_NETWORK) && CONFIG_NRF21540_FEM_SPI_SUPPORTED */

#define NRF21540_SPI DT_PHANDLE(NRF21540_NODE, spi_if)
#define NRF21540_SPI_BUS DT_BUS(NRF21540_SPI)

#if defined(NRF5340_XXAA_NETWORK)
PINCTRL_DT_DEFINE(NRF21540_SPI_BUS);
#endif

#define T_NCS_SCLK 1

enum nrf21540_ant {
	/** Antenna 1 output. */
	NRF21540_ANT1,

	/** Antenna 2 output. */
	NRF21540_ANT2
};

enum nrf21540_trx {
	NRF21540_TX,
	NRF21540_RX
};

/* nRF21540 state */
enum nrf21540_state {
	/* nRF21540 Power-Down state */
	NRF21540_STATE_OFF,

	/* nRF21540 is powered and ready for PA/LNA operation
	 * (program state).
	 */
	NRF21540_STATE_READY,

	/* nRF21540 is in PA/LNA state or processes command. */
	NRF21540_STATE_BUSY
};

struct gpiote_pin {
	uint32_t abs_pin;
	uint8_t gpiote_channel;
	bool active_high;
};

static struct nrf21540 {
#if CONFIG_NRF21540_FEM_SPI_SUPPORTED
#if !defined(NRF5340_XXAA_NETWORK)
	const struct device *const spi_dev;
#endif /* !defined(NRF5340_XXAA_NETWORK) */
	const struct spi_config spi_cfg;

#if DT_SPI_DEV_HAS_CS_GPIOS(NRF21540_SPI)
	const struct spi_cs_control spi_cs;
#endif /* DT_SPI_DEV_HAS_CS_GPIOS(NRF21540_SPI) */
#endif /* CONFIG_NRF21540_FEM_SPI_SUPPORTED */

	struct gpiote_pin tx_en;
	struct gpiote_pin rx_en;
	struct gpio_dt_spec tx_en_gpio;
	struct gpio_dt_spec rx_en_gpio;

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), pdn_gpios)
	const struct gpio_dt_spec pdn;
#endif /* DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), pdn_gpios) */

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), ant_sel_gpios)
	const struct gpio_dt_spec ant_sel;
#endif /* DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), ant_sel_gpios) */

	gppi_channel_t timer_ch;
	gppi_channel_t pin_set_ch;
	gppi_channel_t pin_clr_ch;
	const nrfx_timer_t timer;
} nrf21540_cfg = {
#if CONFIG_NRF21540_FEM_SPI_SUPPORTED
#if !defined(NRF5340_XXAA_NETWORK)
	.spi_dev = DEVICE_DT_GET(NRF21540_SPI_BUS),
#endif /* !defined(NRF5340_XXAA_NETWORK) */
#if DT_SPI_DEV_HAS_CS_GPIOS(NRF21540_SPI)
	.spi_cs.gpio = SPI_CS_GPIOS_DT_SPEC_GET(NRF21540_SPI),
#endif /* DT_SPI_DEV_HAS_CS_GPIOS(NRF21540_SPI) */

	.spi_cfg = {
		.frequency = DT_PROP(NRF21540_SPI, spi_max_frequency),
		.operation = (SPI_OP_MODE_MASTER | SPI_WORD_SET(8) |
			      SPI_TRANSFER_MSB | SPI_LINES_SINGLE),
		.slave = DT_REG_ADDR(NRF21540_SPI),
	},
#endif /* CONFIG_NRF21540_FEM_SPI_SUPPORTED */

	.tx_en_gpio = GPIO_DT_SPEC_GET(NRF21540_NODE, tx_en_gpios),
	.rx_en_gpio = GPIO_DT_SPEC_GET(NRF21540_NODE, rx_en_gpios),
#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), pdn_gpios)
	.pdn = GPIO_DT_SPEC_GET(NRF21540_NODE, pdn_gpios),
#endif /* #if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), pdn_gpios) */

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), ant_sel_gpios)
	.ant_sel = GPIO_DT_SPEC_GET(NRF21540_NODE, ant_sel_gpios),
#endif /* DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), ant_sel_gpios) */

	.timer = NRFX_TIMER_INSTANCE(NRF21540_TIMER_INSTANCE)
};

enum nrf21540_reg {
	NRF21540_REG_CONFREG0 = 0x00,
} nrf21540_reg;

#if defined(NRF5340_XXAA_NETWORK) && CONFIG_NRF21540_FEM_SPI_SUPPORTED
struct nrf_uarte_config {
	uint32_t tx_pin;
	uint32_t rx_pin;
	uint32_t cts_pin;
	uint32_t rts_pin;
	uint32_t baudrate;
	uint32_t config;
	uint32_t rxd_maxcnt;
	uint32_t rxd_ptr;
	uint32_t txd_maxcnt;
	uint32_t txd_ptr;
	uint32_t inten;
};
#endif /* defined(NRF5340_XXAA_NETWORK) && CONFIG_NRF21540_FEM_SPI_SUPPORTED */

static uint32_t activate_evt;
static uint32_t deactivate_evt;

static atomic_t state = NRF21540_STATE_OFF;

static inline nrf_radio_task_t nrf21540_radio_task_get(enum nrf21540_trx dir)
{
	return (dir == NRF21540_TX) ? NRF_RADIO_TASK_TXEN : NRF_RADIO_TASK_RXEN;
}

static struct gpiote_pin *nrf21540_gpiote_pin_get(enum nrf21540_trx dir)
{
	return dir == NRF21540_TX ? &nrf21540_cfg.tx_en : &nrf21540_cfg.rx_en;
}

static uint32_t gpio_port_num_decode(const struct device *dev)
{
	if (dev == DEVICE_DT_GET(DT_NODELABEL(gpio0))) {
		return 0;
	}
#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio1), okay)
	else if (dev == DEVICE_DT_GET(DT_NODELABEL(gpio1))) {
		return 1;
	}
#endif

	__ASSERT(false, "Unknown GPIO port");
	return 0;
}

static int gpiote_configure(void)
{
	if (nrfx_gpiote_channel_alloc(&nrf21540_cfg.tx_en.gpiote_channel) != NRFX_SUCCESS) {
		return -ENXIO;
	}

	nrf21540_cfg.tx_en.abs_pin =
		NRF_GPIO_PIN_MAP(gpio_port_num_decode(nrf21540_cfg.tx_en_gpio.port),
				 nrf21540_cfg.tx_en_gpio.pin);
	nrf21540_cfg.tx_en.active_high =
		(nrf21540_cfg.tx_en_gpio.dt_flags & GPIO_ACTIVE_HIGH) == GPIO_ACTIVE_HIGH;

	if (nrfx_gpiote_channel_alloc(&nrf21540_cfg.rx_en.gpiote_channel) != NRFX_SUCCESS) {
		return -ENXIO;
	}

	nrf21540_cfg.rx_en.abs_pin =
		NRF_GPIO_PIN_MAP(gpio_port_num_decode(nrf21540_cfg.rx_en_gpio.port),
				 nrf21540_cfg.rx_en_gpio.pin);
	nrf21540_cfg.rx_en.active_high =
		(nrf21540_cfg.rx_en_gpio.dt_flags & GPIO_ACTIVE_HIGH) == GPIO_ACTIVE_HIGH;

	nrf_gpiote_task_configure(NRF_GPIOTE,
			nrf21540_cfg.tx_en.gpiote_channel,
			nrf21540_cfg.tx_en.abs_pin,
			(nrf_gpiote_polarity_t)GPIOTE_CONFIG_POLARITY_None,
			(nrf_gpiote_outinit_t)!nrf21540_cfg.tx_en.active_high);
	nrf_gpiote_task_enable(NRF_GPIOTE, nrf21540_cfg.tx_en.gpiote_channel);

	nrf_gpiote_task_configure(NRF_GPIOTE,
			nrf21540_cfg.rx_en.gpiote_channel,
			nrf21540_cfg.rx_en.abs_pin,
			(nrf_gpiote_polarity_t)GPIOTE_CONFIG_POLARITY_None,
			(nrf_gpiote_outinit_t)!nrf21540_cfg.rx_en.active_high);
	nrf_gpiote_task_enable(NRF_GPIOTE, nrf21540_cfg.rx_en.gpiote_channel);

	return 0;
}

static void event_configure(enum nrf21540_trx dir, uint32_t event,
			    bool active,
			    uint32_t active_delay)
{
	const struct gpiote_pin *pin = nrf21540_gpiote_pin_get(dir);
	gppi_channel_t gppi_ch;
	uint32_t task_addr;
	nrf_radio_task_t radio_task = nrf21540_radio_task_get(dir);

	gppi_ch = active ? nrf21540_cfg.pin_set_ch : nrf21540_cfg.pin_clr_ch;
	task_addr = (pin->active_high ^ active) ?
		nrf_gpiote_task_address_get(NRF_GPIOTE,
			nrf_gpiote_clr_task_get(pin->gpiote_channel)) :
		nrf_gpiote_task_address_get(NRF_GPIOTE,
			nrf_gpiote_set_task_get(pin->gpiote_channel));

	if (active) {
		nrf_timer_event_clear(nrf21540_cfg.timer.p_reg,
				      NRF_TIMER_EVENT_COMPARE0);
		nrf_timer_cc_set(nrf21540_cfg.timer.p_reg,
				 NRF_TIMER_CC_CHANNEL0,
				 active_delay);
		nrf_timer_shorts_enable(nrf21540_cfg.timer.p_reg,
					NRF_TIMER_SHORT_COMPARE0_STOP_MASK |
					NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK);


		nrfx_gppi_channel_endpoints_setup(gppi_ch,
			nrf_timer_event_address_get(nrf21540_cfg.timer.p_reg,
						    NRF_TIMER_EVENT_COMPARE0),
			task_addr);
		nrfx_gppi_channels_enable(BIT(gppi_ch));

		if (event == FEM_EXECUTE_NOW) {

			nrf_timer_task_trigger(nrf21540_cfg.timer.p_reg,
					       NRF_TIMER_TASK_START);
			nrf_radio_task_trigger(NRF_RADIO, radio_task);
		} else {
			nrfx_gppi_channel_endpoints_setup(nrf21540_cfg.timer_ch,
				event,
				nrf_timer_task_address_get(
						nrf21540_cfg.timer.p_reg,
						NRF_TIMER_TASK_START));
			nrfx_gppi_fork_endpoint_setup(nrf21540_cfg.timer_ch,
				nrf_radio_task_address_get(NRF_RADIO,
							   radio_task));
			nrfx_gppi_channels_enable(BIT(nrf21540_cfg.timer_ch));
		}

	} else {
		nrfx_gppi_channel_endpoints_setup(gppi_ch, event, task_addr);
		nrfx_gppi_channels_enable(BIT(gppi_ch));
	}
}

static int gpio_configure(void)
{
	int err = 0;

#if DT_NODE_HAS_PROP(NRF21540_NODE, pdn_gpios)
	/* Configure PDN pin */
	if (!device_is_ready(nrf21540_cfg.pdn.port)) {
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&nrf21540_cfg.pdn, GPIO_OUTPUT_INACTIVE);
	if (err) {
		return err;
	}
#endif /* DT_NODE_HAS_PROP(NRF21540_NODE, pdn_gpios) */

#if DT_NODE_HAS_PROP(NRF21540_NODE, ant_sel_gpios)
	/* Configure Antenna select pin */
	if (!device_is_ready(nrf21540_cfg.ant_sel.port)) {
		return -ENODEV;
	}

	/* Configure Antenna select pin */
	err = gpio_pin_configure_dt(&nrf21540_cfg.ant_sel, GPIO_OUTPUT_INACTIVE);
#endif /* DT_NODE_HAS_PROP(NRF21540_NODE, ant_sel_gpios) */

	return err;
}

#if CONFIG_NRF21540_FEM_SPI_SUPPORTED
static int spi_config(void)
{
	int err = 0;

#if DT_SPI_DEV_HAS_CS_GPIOS(NRF21540_SPI)
	/* Configure CS pin */
	if (!device_is_ready(nrf21540_cfg.spi_cs.gpio.port)) {
		return -ENODEV;
	}
	err = gpio_pin_configure_dt(&nrf21540_cfg.spi_cs.gpio, GPIO_OUTPUT_INACTIVE);
	if (err) {
		return err;
	}
#endif /* DT_SPI_DEV_HAS_CS_GPIOS(NRF21540_SPI)  */

#if !defined(NRF5340_XXAA_NETWORK)
	if (!device_is_ready(nrf21540_cfg.spi_dev)) {
		return -ENODEV;
	}
#endif

	return err;
}
#endif /* CONFIG_NRF21540_FEM_SPI_SUPPORTED */

static int gppi_channel_config(void)
{
	if (gppi_channel_alloc(&nrf21540_cfg.pin_set_ch) != NRFX_SUCCESS) {
		return -ENXIO;
	}

	if (gppi_channel_alloc(&nrf21540_cfg.pin_clr_ch) != NRFX_SUCCESS) {
		return -ENXIO;
	}

	if (gppi_channel_alloc(&nrf21540_cfg.timer_ch) != NRFX_SUCCESS) {
		return -ENXIO;
	}

	return 0;
}

static int nrf21540_init(void)
{
	int err;

	/* Configure timer to 1 us resolution. */
	nrf_timer_mode_set(nrf21540_cfg.timer.p_reg, NRF_TIMER_MODE_TIMER);
	nrf_timer_bit_width_set(nrf21540_cfg.timer.p_reg,
				NRF_TIMER_BIT_WIDTH_16);
	nrf_timer_frequency_set(nrf21540_cfg.timer.p_reg, NRF_TIMER_FREQ_1MHz);

	err = gpio_configure();
	if (err) {
		return err;
	}

	err = gpiote_configure();
	if (err) {
		return err;
	}

	err = gppi_channel_config();
	if (err) {
		return err;
	}

#if CONFIG_NRF21540_FEM_SPI_SUPPORTED
	err = spi_config();
	if (err) {
		return err;
	}
#endif /* CONFIG_NRF21540_FEM_SPI_SUPPORTED */

	return 0;
}

static int nrf21540_power_up(void)
{
	int err = 0;

	if (!atomic_cas(&state, NRF21540_STATE_OFF, NRF21540_STATE_BUSY)) {
		return -EBUSY;
	}

#if DT_NODE_HAS_PROP(NRF21540_NODE, pdn_gpios)
	/* Power-up nRF21540 */
	err = gpio_pin_set_dt(&nrf21540_cfg.pdn, 1);
#endif /* DT_NODE_HAS_PROP(NRF21540_NODE, pdn_gpios) */

#if DT_SPI_DEV_HAS_CS_GPIOS(NRF21540_SPI)
	/* Set CSN Pin */
	if (!err) {
		err = gpio_pin_set_dt(&nrf21540_cfg.spi_cs.gpio, 0);
	}

#endif /* DT_SPI_DEV_HAS_CS_GPIOS(NRF21540_SPI) */

	if (!err) {
		k_busy_wait(PDN_SETTLE_TIME);
	}

	atomic_set(&state, err ? NRF21540_STATE_OFF : NRF21540_STATE_READY);

	return err;
}

static int nrf21540_power_down(void)
{
	int err = 0;

	if (!atomic_cas(&state, NRF21540_STATE_READY, NRF21540_STATE_BUSY)) {
		return -EBUSY;
	}

#if DT_NODE_HAS_PROP(NRF21540_NODE, pdn_gpios)
	/* Power-down nRF21540 */
	err = gpio_pin_set_dt(&nrf21540_cfg.pdn, 0);
#endif /* DT_NODE_HAS_PROP(NRF21540_NODE, pdn_gpios) */

	atomic_set(&state, err ? NRF21540_STATE_READY : NRF21540_STATE_OFF);

	return err;
}

#if DT_NODE_HAS_PROP(NRF21540_NODE, ant_sel_gpios)
static int nrf21540_antenna_select(enum fem_antenna ant)
{
	int err = 0;
	uint32_t prev_state;

	prev_state = atomic_set(&state, NRF21540_STATE_BUSY);

	if (prev_state == NRF21540_STATE_BUSY) {
		return -EBUSY;
	}

	switch (ant) {
	case FEM_ANTENNA_1:
		err = gpio_pin_set_dt(&nrf21540_cfg.ant_sel, 0);
		break;

	case FEM_ANTENNA_2:
		err = gpio_pin_set_dt(&nrf21540_cfg.ant_sel, 1);

		break;

	default:
		err = -EINVAL;
		break;
	}

	atomic_set(&state, prev_state);

	return err;
}
#else
static int nrf21540_antenna_select(enum fem_antenna ant)
{
	return -ENOTSUP;
}
#endif /* DT_NODE_HAS_PROP(NRF21540_NODE, ant_sel_gpios) */

#if CONFIG_NRF21540_FEM_SPI_SUPPORTED
#if defined(NRF5340_XXAA_NETWORK)
static inline nrf_spim_frequency_t get_nrf_spim_frequency(uint32_t frequency)
{
	/* Get the highest supported frequency not exceeding the requested one.
	 */
	if (frequency < 250000) {
		return NRF_SPIM_FREQ_125K;
	} else if (frequency < 500000) {
		return NRF_SPIM_FREQ_250K;
	} else if (frequency < 1000000) {
		return NRF_SPIM_FREQ_500K;
	} else if (frequency < 2000000) {
		return NRF_SPIM_FREQ_1M;
	} else if (frequency < 4000000) {
		return NRF_SPIM_FREQ_2M;
	} else if (frequency < 8000000) {
		return NRF_SPIM_FREQ_4M;
	} else {
		return NRF_SPIM_FREQ_8M;
	}
}

static void uarte_events_clear(NRF_UARTE_Type *uarte)
{
	nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_ENDRX);
	nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_CTS);
	nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_NCTS);
	nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_RXDRDY);
	nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_TXDRDY);
	nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_ENDTX);
	nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_ERROR);
	nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_RXTO);
	nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_RXSTARTED);
	nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_TXSTARTED);
	nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_TXSTOPPED);
}

static void spim_events_clear(NRF_SPIM_Type *spim)
{
	nrf_spim_event_clear(spim, NRF_SPIM_EVENT_STOPPED);
	nrf_spim_event_clear(spim, NRF_SPIM_EVENT_ENDRX);
	nrf_spim_event_clear(spim, NRF_SPIM_EVENT_END);
	nrf_spim_event_clear(spim, NRF_SPIM_EVENT_ENDTX);
	nrf_spim_event_clear(spim, NRF_SPIM_EVENT_STARTED);
}

static void uarte_configuration_store(NRF_UARTE_Type const *uarte,
				      struct nrf_uarte_config *cfg)
{
	cfg->tx_pin = nrf_uarte_tx_pin_get(uarte);
	cfg->rx_pin = nrf_uarte_rx_pin_get(uarte);
	cfg->cts_pin = nrf_uarte_cts_pin_get(uarte);
	cfg->rts_pin = nrf_uarte_rts_pin_get(uarte);
	cfg->baudrate = uarte->BAUDRATE;
	cfg->config = uarte->CONFIG;
	cfg->rxd_maxcnt = uarte->RXD.MAXCNT;
	cfg->rxd_ptr = uarte->RXD.PTR;
	cfg->txd_maxcnt = uarte->TXD.MAXCNT;
	cfg->txd_ptr = uarte->TXD.PTR;
	cfg->inten = nrf_uarte_int_enable_check(uarte, 0xFFFFFFFF);
}

static void uarte_configuration_restore(NRF_UARTE_Type *uarte,
					const struct nrf_uarte_config *cfg)
{
	nrf_uarte_txrx_pins_set(uarte, cfg->tx_pin, cfg->rx_pin);
	nrf_uarte_hwfc_pins_set(uarte, cfg->rts_pin, cfg->cts_pin);
	uarte->CONFIG = cfg->config;
	nrf_uarte_baudrate_set(uarte, cfg->baudrate);
	nrf_uarte_rx_buffer_set(uarte, (uint8_t *) cfg->rxd_ptr,
				cfg->rxd_maxcnt);
	nrf_uarte_tx_buffer_set(uarte, (uint8_t *) cfg->txd_ptr,
				cfg->txd_maxcnt);
	nrf_uarte_int_enable(uarte, cfg->inten);
}

static int spim_gain_transfer(uint8_t gain)
{
	int err = 0;
	nrfx_err_t nrfx_err;
	uint8_t buf[NRF21540_SPI_LENGTH_BYTES];
	nrfx_spim_config_t spim_config = NRFX_SPIM_DEFAULT_CONFIG(
						NRFX_SPIM_PIN_NOT_USED,
						NRFX_SPIM_PIN_NOT_USED,
						NRFX_SPIM_PIN_NOT_USED,
						NRFX_SPIM_PIN_NOT_USED);
	spim_config.skip_gpio_cfg = true;
	spim_config.skip_psel_cfg = true;

	err = pinctrl_apply_state(PINCTRL_DT_DEV_CONFIG_GET(NRF21540_SPI_BUS),
				  PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		return err;
	}

	spim_config.frequency =
		get_nrf_spim_frequency(nrf21540_cfg.spi_cfg.frequency);

#if DT_SPI_DEV_HAS_CS_GPIOS(NRF21540_SPI)
	err = gpio_pin_set_dt(&nrf21540_cfg.spi_cs.gpio, 0);
	if (err) {
		return err;
	}
#endif /* DT_SPI_DEV_HAS_CS_GPIOS(NRF21540_SPI) */

	nrfx_err = nrfx_spim_init(&spim, &spim_config, NULL, NULL);
	if (nrfx_err != NRFX_SUCCESS) {
		return -ENXIO;
	}

	buf[NRF21540_SPI_COMMAND_ADDR_BYTE] =
		NRF21540_SPI_WRITE(NRF21540_REG_CONFREG0);
	buf[NRF21540_SPI_DATA_BYTE] =
		(gain << NRF21540_BITS_CONFREG0_TX_GAIN_Pos) &
		NRF21540_BITS_CONFREG0_TX_GAIN_Msk;

	/* Prepare transfer */
	nrfx_spim_xfer_desc_t xfer_desc = NRFX_SPIM_XFER_TX(buf, sizeof(buf));

#if DT_SPI_DEV_HAS_CS_GPIOS(NRF21540_SPI)
	err = gpio_pin_set_dt(&nrf21540_cfg.spi_cs.gpio, 1);
	if (err) {
		return err;
	}
#endif /* DT_SPI_DEV_HAS_CS_GPIOS(NRF21540_SPI) */

	nrfx_err = nrfx_spim_xfer(&spim, &xfer_desc,
				  NRFX_SPIM_FLAG_NO_XFER_EVT_HANDLER);
	if (nrfx_err != NRFX_SUCCESS) {
		return -ENXIO;
	}

	nrfx_spim_uninit(&spim);
	spim_events_clear(spim.p_reg);

	return err;
}

static int nrf21540_tx_gain_set(uint32_t gain)
{
	int err;
	NRF_UARTE_Type *uarte_inst = NRF_UARTE0;
	const struct device *uart;
	struct nrf_uarte_config uarte_cfg;
	bool uart_ready = false;
	bool uart_irq = false;
	unsigned int key;

	if (gain > NRF21540_TX_GAIN_Max) {
		return -EINVAL;
	}

	if (!atomic_cas(&state, NRF21540_STATE_READY, NRF21540_STATE_BUSY)) {
		return -EBUSY;
	}

	memset(&uarte_cfg, 0, sizeof(uarte_cfg));

	key = irq_lock();

	/* Disable UART_0 */
	uart = DEVICE_DT_GET(DT_NODELABEL(uart0));
	if (uart) {
		uart_ready = device_is_ready(uart);
	}

	uart_irq = irq_is_enabled(nrfx_get_irq_number(uarte_inst));
	if (uart_irq) {
		irq_disable(nrfx_get_irq_number(uarte_inst));
	}

	if (uart_ready) {
		err = pm_device_action_run(uart, PM_DEVICE_ACTION_SUSPEND);
		if (err) {
			goto error;
		}

		uarte_events_clear(uarte_inst);
	}

	uarte_configuration_store(uarte_inst, &uarte_cfg);
	nrf_uarte_int_disable(uarte_inst, 0xFFFFFFFF);

	/* Initialize SPI, make a transfer with it and deinitialize. */
	err = spim_gain_transfer(gain);
	if (err) {
		goto error;
	}

	/* Restore UART_0 */
	uarte_configuration_restore(uarte_inst, &uarte_cfg);

	if (uart_ready) {
		err = pm_device_action_run(uart, PM_DEVICE_ACTION_RESUME);
	}

	if (uart_irq) {
		NVIC_ClearPendingIRQ(nrfx_get_irq_number(uarte_inst));
		irq_enable(nrfx_get_irq_number(uarte_inst));
	}

error:
	irq_unlock(key);
	atomic_set(&state, NRF21540_STATE_READY);

	return err;
}

#else
static int nrf21540_tx_gain_set(uint32_t gain)
{
	int err = 0;
	uint8_t buf[NRF21540_SPI_LENGTH_BYTES];

	if (!atomic_cas(&state, NRF21540_STATE_READY, NRF21540_STATE_BUSY)) {
		return -EBUSY;
	}

	if (gain > NRF21540_TX_GAIN_Max) {
		err = -EINVAL;
		goto error;
	}

#if DT_SPI_DEV_HAS_CS_GPIOS(NRF21540_SPI)
	err = gpio_pin_set_dt(&nrf21540_cfg.spi_cs.gpio, 0);
	if (err) {
		goto error;
	}
#endif /* DT_SPI_DEV_HAS_CS_GPIOS(NRF21540_SPI) */

	buf[NRF21540_SPI_COMMAND_ADDR_BYTE] =
		NRF21540_SPI_WRITE(NRF21540_REG_CONFREG0);
	buf[NRF21540_SPI_DATA_BYTE] =
		(gain << NRF21540_BITS_CONFREG0_TX_GAIN_Pos) &
		NRF21540_BITS_CONFREG0_TX_GAIN_Msk;

	const struct spi_buf tx_buf = {
		.buf = buf,
		.len = sizeof(buf)
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};

#if DT_SPI_DEV_HAS_CS_GPIOS(NRF21540_SPI)
	err = gpio_pin_set_dt(&nrf21540_cfg.spi_cs.gpio, 1);
	if (err) {
		goto error;
	}
#endif /* DT_SPI_DEV_HAS_CS_GPIOS(NRF21540_SPI) */

	err = spi_transceive(nrf21540_cfg.spi_dev, &nrf21540_cfg.spi_cfg,
			     &tx, NULL);

error:
	atomic_set(&state, NRF21540_STATE_READY);
	return err;
}
#endif /* defined(NRF5340_XXAA_NETWORK) */
#else
static int nrf21540_tx_gain_set(uint32_t gain)
{
	ARG_UNUSED(gain);

	return -ENOTSUP;
}
#endif /* CONFIG_NRF21540_FEM_SPI_SUPPORTED */

static int nrf21540_tx_configure(uint32_t activate_event, uint32_t deactivate_event,
				 uint32_t activation_delay)
{
	if (!atomic_cas(&state, NRF21540_STATE_READY, NRF21540_STATE_BUSY)) {
		return -EBUSY;
	}

	activate_evt = activate_event;
	deactivate_evt = deactivate_event;

	/* Configure pin activation event. */
	event_configure(NRF21540_TX, activate_evt, true, activation_delay);

	if (deactivate_evt != FEM_EVENT_UNUSED) {
		/* Configure pin deactivation event. */
		event_configure(NRF21540_TX, deactivate_evt, false,
				activation_delay);
	}

	return 0;
}

static int nrf21540_rx_configure(uint32_t activate_event, uint32_t deactivate_event,
				 uint32_t activation_delay)
{
	int err = 0;

	if (!atomic_cas(&state, NRF21540_STATE_READY, NRF21540_STATE_BUSY)) {
		return -EBUSY;
	}


#if DT_SPI_DEV_HAS_CS_GPIOS(NRF21540_SPI)
	/* Pull down the CSN pin before RX. */
	err = gpio_pin_set_dt(&nrf21540_cfg.spi_cs.gpio, 1);
	if (err) {
		return err;
	}
#endif /* DT_SPI_DEV_HAS_CS_GPIOS(NRF21540_SPI) */

	activate_evt = activate_event;
	deactivate_evt = deactivate_event;

	/* Configure pin activation event. */
	event_configure(NRF21540_RX, activate_evt, true, activation_delay);


	if (deactivate_evt != FEM_EVENT_UNUSED) {
		/* Configure pin deactivation event. */
		event_configure(NRF21540_RX, deactivate_evt, false,
				activation_delay);
	}

	return err;
}

static int nrf21540_txrx_stop(void)
{
	nrf_gpiote_task_force(NRF_GPIOTE,
			      nrf21540_cfg.rx_en.gpiote_channel,
			      nrf21540_cfg.rx_en.active_high ?
				NRF_GPIOTE_INITIAL_VALUE_LOW :
				NRF_GPIOTE_INITIAL_VALUE_HIGH);

	nrf_gpiote_task_force(NRF_GPIOTE,
			      nrf21540_cfg.tx_en.gpiote_channel,
			      nrf21540_cfg.tx_en.active_high ?
				NRF_GPIOTE_INITIAL_VALUE_LOW :
				NRF_GPIOTE_INITIAL_VALUE_HIGH);

#if DT_SPI_DEV_HAS_CS_GPIOS(NRF21540_SPI)
	/* Set CSN pin */
	return gpio_pin_set_dt(&nrf21540_cfg.spi_cs.gpio, 0);

#else
	return 0;
#endif /* DT_SPI_DEV_HAS_CS_GPIOS(NRF21540_SPI) */
}

static void nrf21540_txrx_configuration_clear(void)
{
	nrfx_gppi_channels_disable(BIT(nrf21540_cfg.pin_set_ch));
	nrfx_gppi_event_endpoint_clear(nrf21540_cfg.pin_set_ch,
			nrf_timer_event_address_get(nrf21540_cfg.timer.p_reg,
						    NRF_TIMER_EVENT_COMPARE0));
	nrfx_gppi_task_endpoint_clear(nrf21540_cfg.pin_set_ch,
		nrf_gpiote_task_address_get(NRF_GPIOTE,
			nrf_gpiote_set_task_get(nrf21540_cfg.tx_en.gpiote_channel)));

	nrfx_gppi_task_endpoint_clear(nrf21540_cfg.pin_set_ch,
		nrf_gpiote_task_address_get(NRF_GPIOTE,
			nrf_gpiote_set_task_get(nrf21540_cfg.rx_en.gpiote_channel)));

	if (activate_evt != FEM_EXECUTE_NOW) {
		nrfx_gppi_channels_disable(BIT(nrf21540_cfg.timer_ch));
		nrfx_gppi_event_endpoint_clear(nrf21540_cfg.timer_ch,
					       activate_evt);
		nrfx_gppi_task_endpoint_clear(nrf21540_cfg.timer_ch,
			nrf_timer_task_address_get(nrf21540_cfg.timer.p_reg,
						   NRF_TIMER_TASK_START));
		nrfx_gppi_fork_endpoint_clear(nrf21540_cfg.timer_ch,
			nrf_radio_task_address_get(NRF_RADIO, NRF_RADIO_TASK_RXEN));
		nrfx_gppi_fork_endpoint_clear(nrf21540_cfg.timer_ch,
			nrf_radio_task_address_get(NRF_RADIO, NRF_RADIO_TASK_TXEN));
	}

	if (deactivate_evt != FEM_EVENT_UNUSED) {
		nrfx_gppi_channels_disable(BIT(nrf21540_cfg.pin_clr_ch));
		nrfx_gppi_event_endpoint_clear(nrf21540_cfg.pin_clr_ch, deactivate_evt);
		nrfx_gppi_task_endpoint_clear(nrf21540_cfg.pin_clr_ch,
			nrf_gpiote_task_address_get(NRF_GPIOTE,
				nrf_gpiote_clr_task_get(nrf21540_cfg.tx_en.gpiote_channel)));

		nrfx_gppi_task_endpoint_clear(nrf21540_cfg.pin_clr_ch,
			nrf_gpiote_task_address_get(NRF_GPIOTE,
				nrf_gpiote_clr_task_get(nrf21540_cfg.rx_en.gpiote_channel)));
	}

	atomic_set(&state, NRF21540_STATE_READY);
}

static uint32_t nrf21540_default_active_delay_calculate(bool rx, nrf_radio_mode_t mode)
{
	bool fast_ramp_up = nrf_radio_modecnf0_ru_get(NRF_RADIO);

	return rx ? (fem_radio_rx_ramp_up_delay_get(fast_ramp_up, mode) -
		     RX_EN_SETTLE_TIME) :
		    (fem_radio_tx_ramp_up_delay_get(fast_ramp_up, mode) -
		     TX_EN_SETTLE_TIME);
}

static const struct fem_interface_api nrf21540_api = {
	.power_up = nrf21540_power_up,
	.power_down = nrf21540_power_down,
	.tx_configure = nrf21540_tx_configure,
	.rx_configure = nrf21540_rx_configure,
	.txrx_configuration_clear = nrf21540_txrx_configuration_clear,
	.txrx_stop = nrf21540_txrx_stop,
	.tx_gain_set = nrf21540_tx_gain_set,
	.default_active_delay_calculate = nrf21540_default_active_delay_calculate,
	.antenna_select = nrf21540_antenna_select
};

static int nrf21540_setup(const struct device *dev)
{
	int err;

	err = nrf21540_init();
	if (err) {
		return err;
	}

	return fem_interface_api_set(&nrf21540_api);
}

SYS_INIT(nrf21540_setup, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
