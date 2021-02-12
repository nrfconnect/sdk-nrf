/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <device.h>
#include <drivers/gpio.h>
#include <drivers/spi.h>
#include <soc.h>
#include <sys/__assert.h>

#include <hal/nrf_gpio.h>
#include <hal/nrf_gpiote.h>
#include <hal/nrf_radio.h>
#include <helpers/nrfx_gppi.h>
#include <nrfx_timer.h>

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

#include "radio_def.h"
#include "nrf21540.h"

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

#define TX_EN_REG DT_REG_ADDR(DT_PHANDLE(NRF21540_NODE, tx_en_gpios))
#define TX_EN_PIN DT_GPIO_PIN(NRF21540_NODE, tx_en_gpios)
#define TX_EN_FLAGS DT_GPIO_FLAGS(NRF21540_NODE, tx_en_gpios)

#define RX_EN_REG DT_REG_ADDR(DT_PHANDLE(NRF21540_NODE, rx_en_gpios))
#define RX_EN_PIN DT_GPIO_PIN(NRF21540_NODE, rx_en_gpios)
#define RX_EN_FLAGS DT_GPIO_FLAGS(NRF21540_NODE, rx_en_gpios)

#define PDN_PORT DT_GPIO_LABEL(NRF21540_NODE, pdn_gpios)
#define PDN_PIN DT_GPIO_PIN(NRF21540_NODE, pdn_gpios)
#define PDN_FLAGS DT_GPIO_FLAGS(NRF21540_NODE, pdn_gpios)

#define ANT_SEL_PORT DT_GPIO_LABEL(NRF21540_NODE, ant_sel_gpios)
#define ANT_SEL_PIN DT_GPIO_PIN(NRF21540_NODE, ant_sel_gpios)
#define ANT_SEL_FLAGS DT_GPIO_FLAGS(NRF21540_NODE, ant_sel_gpios)

#define NRF21540_SPI DT_PHANDLE(NRF21540_NODE, spi_if)
#define NRF21540_SPI_LABEL DT_LABEL(DT_BUS(NRF21540_SPI))
#define CS_GPIO_PORT DT_SPI_DEV_CS_GPIOS_LABEL(NRF21540_SPI)

#define T_NCS_SCLK 1

#define NRF21540_GPIO_POLARITY_GET(_dt_property)                     \
	((DT_GPIO_FLAGS(DT_NODELABEL(nrf_radio_fem), _dt_property) & \
	GPIO_ACTIVE_LOW) ? false : true)

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

struct gpio_data {
	const struct device *port;
	gpio_pin_t pin;
};

struct gpiote_pin {
	uint32_t abs_pin;
	uint8_t gpiote_channel;
	bool active_high;
};

static struct nrf21540 {
	const struct device *spi_dev;
	struct spi_cs_control spi_cs;
	const struct spi_config spi_cfg;
	struct gpiote_pin tx_en;
	struct gpiote_pin rx_en;
	struct gpio_data pdn;
	struct gpio_data ant_sel;
	gppi_channel_t timer_ch;
	gppi_channel_t pin_set_ch;
	gppi_channel_t pin_clr_ch;
	const nrfx_timer_t timer;
} nrf21540_cfg = {
	.spi_cs = {
		.gpio_pin = DT_SPI_DEV_CS_GPIOS_PIN(NRF21540_SPI),
		.gpio_dt_flags = DT_SPI_DEV_CS_GPIOS_FLAGS(NRF21540_SPI),
		.delay = T_NCS_SCLK
	},
	.spi_cfg = {
		.frequency = DT_PROP(NRF21540_SPI, spi_max_frequency),
		.operation = (SPI_OP_MODE_MASTER | SPI_WORD_SET(8) |
			      SPI_TRANSFER_MSB | SPI_LINES_SINGLE),
		.slave = DT_REG_ADDR(NRF21540_SPI),
	},
	.timer = NRFX_TIMER_INSTANCE(NRF21540_TIMER_INSTANCE)
};

enum nrf21540_reg {
	NRF21540_REG_CONFREG0 = 0x00,
} nrf21540_reg;

static uint32_t active_evt;
static uint32_t deactive_evt;

static atomic_t state = NRF21540_STATE_OFF;

static uint32_t radio_tx_ramp_up_delay_get(bool fast, nrf_radio_mode_t mode)
{
	uint32_t ramp_up;

	switch (mode) {
	case NRF_RADIO_MODE_BLE_1MBIT:
		ramp_up = fast ? RADIO_RAMP_UP_TX_1M_FAST_US :
				 RADIO_RAMP_UP_TX_1M_US;
		break;

	case NRF_RADIO_MODE_BLE_2MBIT:
		ramp_up = fast ? RADIO_RAMP_UP_TX_2M_FAST_US :
				 RADIO_RAMP_UP_TX_2M_US;
		break;

	case NRF_RADIO_MODE_BLE_LR125KBIT:
		ramp_up = fast ? RADIO_RAMP_UP_TX_S8_FAST_US :
				 RADIO_RAMP_UP_TX_S8_US;
		break;

	case NRF_RADIO_MODE_BLE_LR500KBIT:
		ramp_up = fast ? RADIO_RAMP_UP_TX_S2_FAST_US :
				 RADIO_RAMP_UP_TX_S2_US;
		break;

	case NRF_RADIO_MODE_IEEE802154_250KBIT:
		ramp_up = fast ? RADIO_RAMP_UP_IEEE_FAST_US :
				 RADIO_RAMP_UP_IEEE_US;
		break;

	default:
		ramp_up = fast ? RADIO_RAMP_UP_DEFAULT_FAST_US :
				 RADIO_RAMP_UP_DEFAULT_US;
		break;
	}

	return ramp_up;
}

static uint32_t radio_rx_ramp_up_delay_get(bool fast, nrf_radio_mode_t mode)
{
	uint32_t ramp_up;

	switch (mode) {
	case NRF_RADIO_MODE_BLE_1MBIT:
		ramp_up = fast ? RADIO_RAMP_UP_RX_1M_FAST_US :
				 RADIO_RAMP_UP_RX_1M_US;
		break;

	case NRF_RADIO_MODE_BLE_2MBIT:
		ramp_up = fast ? RADIO_RAMP_UP_RX_2M_FAST_US :
				 RADIO_RAMP_UP_RX_2M_US;
		break;

	case NRF_RADIO_MODE_BLE_LR125KBIT:
		ramp_up = fast ? RADIO_RAMP_UP_RX_S8_FAST_US :
				 RADIO_RAMP_UP_RX_S8_US;
		break;

	case NRF_RADIO_MODE_BLE_LR500KBIT:
		ramp_up = fast ? RADIO_RAMP_UP_RX_S2_FAST_US :
				 RADIO_RAMP_UP_RX_S2_US;
		break;

	case NRF_RADIO_MODE_IEEE802154_250KBIT:
		ramp_up = fast ? RADIO_RAMP_UP_IEEE_FAST_US :
				 RADIO_RAMP_UP_IEEE_US;
		break;

	default:
		ramp_up = fast ? RADIO_RAMP_UP_DEFAULT_FAST_US :
				 RADIO_RAMP_UP_DEFAULT_US;
		break;
	}

	return ramp_up;
}

static inline nrf_radio_task_t nrf21540_radio_task_get(enum nrf21540_trx dir)
{
	return (dir == NRF21540_TX) ? NRF_RADIO_TASK_TXEN : NRF_RADIO_TASK_RXEN;
}

static struct gpiote_pin *nrf21540_gpiote_pin_get(enum nrf21540_trx dir)
{
	return dir == NRF21540_TX ? &nrf21540_cfg.tx_en : &nrf21540_cfg.rx_en;
}

static uint32_t gpio_port_num_decode(NRF_GPIO_Type *reg)
{
	if (reg == NRF_P0) {
		return 0;
	} else if (reg == NRF_P1) {
		return 1;
	}

	__ASSERT(false, "Unknown GPIO port");
	return 0;
}

static void gpiote_configure(void)
{
	nrf21540_cfg.tx_en.gpiote_channel = CONFIG_NRF21540_FEM_GPIOTE_TX_EN;
	nrf21540_cfg.tx_en.abs_pin =
		NRF_GPIO_PIN_MAP(
			gpio_port_num_decode((NRF_GPIO_Type *) TX_EN_REG),
					     TX_EN_PIN);
	nrf21540_cfg.tx_en.active_high =
		NRF21540_GPIO_POLARITY_GET(tx_en_gpios);

	nrf21540_cfg.rx_en.gpiote_channel = CONFIG_NRF21540_FEM_GPIOTE_RX_EN;
	nrf21540_cfg.rx_en.abs_pin =
		NRF_GPIO_PIN_MAP(
			gpio_port_num_decode((NRF_GPIO_Type *) RX_EN_REG),
					     RX_EN_PIN);
	nrf21540_cfg.rx_en.active_high =
		NRF21540_GPIO_POLARITY_GET(rx_en_gpios);

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

		if (event == NRF21540_EXECUTE_NOW) {

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

static int gpio_output_pin_config(struct gpio_data *config,
				  const char *port_name,
				  gpio_pin_t pin, gpio_dt_flags_t flags)
{
	config->port = device_get_binding(port_name);
	if (!config->port) {
		return -ENXIO;
	}

	config->pin = pin;

	return gpio_pin_configure(config->port, config->pin,
				  (GPIO_OUTPUT_INACTIVE | flags));
}

static int gpio_configure(void)
{
	int err;

	/* Configure PDN pin */
	err = gpio_output_pin_config(&nrf21540_cfg.pdn, PDN_PORT,
				     PDN_PIN, PDN_FLAGS);
	if (err) {
		return err;
	}

	/* Configure Antenna select pin */
	return gpio_output_pin_config(&nrf21540_cfg.ant_sel, ANT_SEL_PORT,
				      ANT_SEL_PIN, ANT_SEL_FLAGS);
}

static int spi_config(void)
{
	int err;

	/* Configure CS pin */
	nrf21540_cfg.spi_cs.gpio_dev = device_get_binding(CS_GPIO_PORT);
	if (!nrf21540_cfg.spi_cs.gpio_dev) {
		return -ENXIO;
	}

	err = gpio_pin_configure(nrf21540_cfg.spi_cs.gpio_dev,
				 nrf21540_cfg.spi_cs.gpio_pin,
				(GPIO_OUTPUT_INACTIVE |
				 nrf21540_cfg.spi_cs.gpio_dt_flags));
	if (err) {
		return err;
	}

	nrf21540_cfg.spi_dev = device_get_binding(NRF21540_SPI_LABEL);
	if (!nrf21540_cfg.spi_dev) {
		return -ENXIO;
	}

	return 0;
}

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

int nrf21540_init(void)
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

	gpiote_configure();

	err = gppi_channel_config();
	if (err) {
		return err;
	}

	err = spi_config();
	if (err) {
		return err;
	}

	return 0;
}

int nrf21540_power_up(void)
{
	int err;

	if (!atomic_cas(&state, NRF21540_STATE_OFF, NRF21540_STATE_BUSY)) {
		return -EBUSY;
	}

	/* Power-up nRF21540 */
	err = gpio_pin_set(nrf21540_cfg.pdn.port, nrf21540_cfg.pdn.pin, 1);
	if (err) {
		goto error;
	}

	/* Set CSN Pin */
	err = gpio_pin_set(nrf21540_cfg.spi_cs.gpio_dev,
			   nrf21540_cfg.spi_cs.gpio_pin, 0);
	if (err) {
		goto error;
	}

	k_busy_wait(NRF21540_PD_PG_TIME_US);

error:
	atomic_set(&state, err ? NRF21540_STATE_OFF : NRF21540_STATE_READY);

	return err;
}

int nrf21540_power_down(void)
{
	int err;

	if (!atomic_cas(&state, NRF21540_STATE_READY, NRF21540_STATE_BUSY)) {
		return -EBUSY;
	}

	/* Power-down nRF21540 */
	err = gpio_pin_set(nrf21540_cfg.pdn.port, nrf21540_cfg.pdn.pin, 0);

	atomic_set(&state, err ? NRF21540_STATE_READY : NRF21540_STATE_OFF);

	return err;
}

int nrf21540_antenna_select(enum nrf21540_ant ant)
{
	int err = 0;
	uint32_t prev_state;

	prev_state = atomic_set(&state, NRF21540_STATE_BUSY);

	if (prev_state == NRF21540_STATE_BUSY) {
		return -EBUSY;
	}

	switch (ant) {
	case NRF21540_ANT1:
		err = gpio_pin_set(nrf21540_cfg.ant_sel.port,
				   nrf21540_cfg.ant_sel.pin, 0);
		break;

	case NRF21540_ANT2:
		err = gpio_pin_set(nrf21540_cfg.ant_sel.port,
				   nrf21540_cfg.ant_sel.pin, 1);

		break;

	default:
		err = -EINVAL;
		break;
	}

	atomic_set(&state, prev_state);

	return err;
}

int nrf21540_tx_gain_set(uint8_t gain)
{
	int err;
	uint8_t buf[NRF21540_SPI_LENGTH_BYTES];

	if (!atomic_cas(&state, NRF21540_STATE_READY, NRF21540_STATE_BUSY)) {
		return -EBUSY;
	}

	if (gain > NRF21540_TX_GAIN_Max) {
		err = -EINVAL;
		goto error;
	}

	err = gpio_pin_set(nrf21540_cfg.spi_cs.gpio_dev,
			   nrf21540_cfg.spi_cs.gpio_pin, 0);
	if (err) {
		goto error;
	}

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

	err = gpio_pin_set(nrf21540_cfg.spi_cs.gpio_dev,
			   nrf21540_cfg.spi_cs.gpio_pin, 1);
	if (err) {
		goto error;
	}

	err = spi_transceive(nrf21540_cfg.spi_dev, &nrf21540_cfg.spi_cfg,
			     &tx, NULL);

error:
	atomic_set(&state, NRF21540_STATE_READY);
	return err;
}

int nrf21540_tx_configure(uint32_t active_event, uint32_t deactive_event,
			  uint32_t active_delay)
{
	if (!atomic_cas(&state, NRF21540_STATE_READY, NRF21540_STATE_BUSY)) {
		return -EBUSY;
	}

	active_evt = active_event;
	deactive_evt = deactive_event;

	/* Configure pin active event. */
	event_configure(NRF21540_TX, active_event, true, active_delay);

	if (deactive_event != NRF21540_EVENT_UNUSED) {
		/* Configure pin deactive event. */
		event_configure(NRF21540_TX, deactive_event, false,
				active_delay);
	}

	return 0;
}

int nrf21540_rx_configure(uint32_t active_event, uint32_t deactive_event,
			  uint32_t active_delay)
{
	int err;

	if (!atomic_cas(&state, NRF21540_STATE_READY, NRF21540_STATE_BUSY)) {
		return -EBUSY;
	}

	/* Pull down the CSN pin before RX. */
	err = gpio_pin_set(nrf21540_cfg.spi_cs.gpio_dev,
			   nrf21540_cfg.spi_cs.gpio_pin, 1);
	if (err) {
		return err;
	}

	active_evt = active_event;
	deactive_evt = deactive_event;

	/* Configure pin active event. */
	event_configure(NRF21540_RX, active_event, true, active_delay);


	if (deactive_event != NRF21540_EVENT_UNUSED) {
		/* Configure pin deactive event. */
		event_configure(NRF21540_RX, deactive_event, false,
				active_delay);
	}

	return 0;
}

int nrf21540_txrx_stop(void)
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

	/* Set CSN pin */
	return gpio_pin_set(nrf21540_cfg.spi_cs.gpio_dev,
			    nrf21540_cfg.spi_cs.gpio_pin, 0);
}

void nrf21540_txrx_configuration_clear(void)
{
	nrfx_gppi_channels_disable(BIT(nrf21540_cfg.pin_set_ch));
	nrfx_gppi_channel_endpoints_setup(nrf21540_cfg.pin_set_ch, 0, 0);
	nrfx_gppi_channels_disable(BIT(nrf21540_cfg.timer_ch));
	nrfx_gppi_channel_endpoints_setup(nrf21540_cfg.timer_ch, 0, 0);
	nrfx_gppi_fork_endpoint_clear(nrf21540_cfg.timer_ch, 0);

	if (deactive_evt != NRF21540_EVENT_UNUSED) {
		nrfx_gppi_channels_disable(BIT(nrf21540_cfg.pin_clr_ch));
		nrfx_gppi_channel_endpoints_setup(nrf21540_cfg.pin_clr_ch,
						  0, 0);
	}

	atomic_set(&state, NRF21540_STATE_READY);
}

uint32_t nrf21540_default_active_delay_calculate(bool rx, nrf_radio_mode_t mode)
{
	bool fast_ramp_up = nrf_radio_modecnf0_ru_get(NRF_RADIO);

	return rx ? (radio_rx_ramp_up_delay_get(fast_ramp_up, mode) -
		     NRF21540_PG_RX_TIME_US) :
		    (radio_tx_ramp_up_delay_get(fast_ramp_up, mode) -
		     NRF21540_PG_TX_TIME_US);
}
