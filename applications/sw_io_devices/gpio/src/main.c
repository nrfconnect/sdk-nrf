/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/dt-bindings/gpio/nordic-nrf-gpio.h>

#if defined(USE_ICMSG_BACKEND)
#include <zephyr/ipc/ipc_service.h>
#endif
#include <drivers/gpio/nrfe_gpio.h>

#include <hal/nrf_vpr_csr.h>
#include <hal/nrf_vpr_csr_vio.h>
#include <haly/nrfy_gpio.h>

#if defined(USE_ICMSG_BACKEND)
static struct ipc_ept ep;

volatile uint32_t bound_sem = 1;

static void ep_bound(void *priv)
{
	bound_sem = 0;
}
#elif defined(USE_STRUCT_COMMUNICATION)
#include <zephyr/drivers/mbox.h>
static const struct mbox_dt_spec rx_channel = MBOX_DT_SPEC_GET(DT_PATH(mbox_consumer), rx);
#else
#error "Define communication channel type"
#endif

static nrf_gpio_pin_pull_t get_pull(gpio_flags_t flags)
{
	if (flags & GPIO_PULL_UP) {
		return NRF_GPIO_PIN_PULLUP;
	} else if (flags & GPIO_PULL_DOWN) {
		return NRF_GPIO_PIN_PULLDOWN;
	}

	return NRF_GPIO_PIN_NOPULL;
}

static int gpio_nrfe_pin_configure(uint8_t port, uint16_t pin, uint32_t flags)
{
	if (port != 2) {
		return -EINVAL;
	}

	uint32_t abs_pin = NRF_GPIO_PIN_MAP(port, pin);
	nrf_gpio_pin_pull_t pull = get_pull(flags);
	nrf_gpio_pin_drive_t drive;

	switch (flags & (NRF_GPIO_DRIVE_MSK | GPIO_OPEN_DRAIN)) {
	case NRF_GPIO_DRIVE_S0S1:
		drive = NRF_GPIO_PIN_S0S1;
		break;
	case NRF_GPIO_DRIVE_S0H1:
		drive = NRF_GPIO_PIN_S0H1;
		break;
	case NRF_GPIO_DRIVE_H0S1:
		drive = NRF_GPIO_PIN_H0S1;
		break;
	case NRF_GPIO_DRIVE_H0H1:
		drive = NRF_GPIO_PIN_H0H1;
		break;
	case NRF_GPIO_DRIVE_S0 | GPIO_OPEN_DRAIN:
		drive = NRF_GPIO_PIN_S0D1;
		break;
	case NRF_GPIO_DRIVE_H0 | GPIO_OPEN_DRAIN:
		drive = NRF_GPIO_PIN_H0D1;
		break;
	case NRF_GPIO_DRIVE_S1 | GPIO_OPEN_SOURCE:
		drive = NRF_GPIO_PIN_D0S1;
		break;
	case NRF_GPIO_DRIVE_H1 | GPIO_OPEN_SOURCE:
		drive = NRF_GPIO_PIN_D0H1;
		break;
	default:
		return -EINVAL;
	}

	if (flags & GPIO_OUTPUT_INIT_HIGH) {
		uint16_t outs = nrf_vpr_csr_vio_out_get();

		nrf_vpr_csr_vio_out_set(outs | (BIT(pin)));
	} else if (flags & GPIO_OUTPUT_INIT_LOW) {
		uint16_t outs = nrf_vpr_csr_vio_out_get();

		nrf_vpr_csr_vio_out_set(outs & ~(BIT(pin)));
	}

	nrf_gpio_pin_dir_t dir =
		(flags & GPIO_OUTPUT) ? NRF_GPIO_PIN_DIR_OUTPUT : NRF_GPIO_PIN_DIR_INPUT;
	nrf_gpio_pin_input_t input =
		(flags & GPIO_INPUT) ? NRF_GPIO_PIN_INPUT_CONNECT : NRF_GPIO_PIN_INPUT_DISCONNECT;

	/* Reconfigure the GPIO pin with the specified pull-up/pull-down configuration and drive
	 * strength.
	 */
	nrfy_gpio_reconfigure(abs_pin, &dir, &input, &pull, &drive, NULL);

	if (dir == NRF_GPIO_PIN_DIR_OUTPUT) {
		nrf_vpr_csr_vio_dir_set(nrf_vpr_csr_vio_dir_get() | (BIT(pin)));
	}

	/* Take control of the pin */
	nrfy_gpio_pin_control_select(abs_pin, NRF_GPIO_PIN_SEL_VPR);

	return 0;
}

static void gpio_nrfe_port_set_bits_raw(uint16_t set_mask)
{
	uint16_t outs = nrf_vpr_csr_vio_out_get();

	nrf_vpr_csr_vio_out_set(outs | set_mask);
}

static void gpio_nrfe_port_clear_bits_raw(uint16_t clear_mask)
{
	uint16_t outs = nrf_vpr_csr_vio_out_get();

	nrf_vpr_csr_vio_out_set(outs & ~clear_mask);
}

static void gpio_nrfe_port_toggle_bits(uint16_t toggle_mask)
{
	nrf_vpr_csr_vio_out_toggle_set(toggle_mask);
}

static void process_packet(nrfe_gpio_data_packet_t *packet)
{
	if (packet->port != 2) {
		return;
	}

	switch (packet->opcode) {
	case NRFE_GPIO_PIN_CONFIGURE: {
		gpio_nrfe_pin_configure(packet->port, packet->pin, packet->flags);
		break;
	}
	case NRFE_GPIO_PIN_CLEAR: {
		gpio_nrfe_port_clear_bits_raw(packet->pin);
		break;
	}
	case NRFE_GPIO_PIN_SET: {
		gpio_nrfe_port_set_bits_raw(packet->pin);
		break;
	}
	case NRFE_GPIO_PIN_TOGGLE: {
		gpio_nrfe_port_toggle_bits(packet->pin);
		break;
	}
	default: {
		break;
	}
	}
}

#if defined(USE_ICMSG_BACKEND)
static void ep_recv(const void *data, size_t len, void *priv)
{
	(void)len;
	(void)priv;

	process_packet((nrfe_gpio_data_packet_t *)data);
}

static struct ipc_ept_cfg ep_cfg = {
	.cb = {
		.bound = ep_bound,
		.received = ep_recv,
	},
};

#elif defined(USE_STRUCT_COMMUNICATION)

/**
 * @brief Callback function for when a message is received from the mailbox
 * @param instance Pointer to the mailbox device instance
 * @param channel Unused, but required by the mailbox API
 * @param user_data Pointer to the received message
 * @param msg_data Unused, but required by the mailbox API
 *
 * This function is called when a message is received from the mailbox.
 * It is responsible for handling the received data and logging any errors.
 */
static void mbox_callback(const struct device *instance, uint32_t channel, void *user_data,
			  struct mbox_msg *msg_data)
{
	/* Unused parameters */
	(void)msg_data;

	/* Check for invalid arguments */
	if (user_data == NULL) {
		return;
	}

	shared_t *rx_data = (shared_t *)user_data;

	/* Try and get lock for the shared data structure */
	if (atomic_flag_test_and_set(&rx_data->lock)) {
		/* Return in case lock is not acquired (used by other core)*/
		return;
	}

	nrfe_gpio_data_packet_t *packet = (nrfe_gpio_data_packet_t *)&rx_data->data;

	process_packet(packet);

	/* Clear shared_data.buffer_size (there is no more data available)
	 * This is necessary so that the other core knows that the data has been read
	 */
	rx_data->size = 0;

	/* We are finished with the shared data structure, so we can release the lock */
	atomic_flag_clear(&rx_data->lock);
}

/**
 * @brief Initialize the mailbox driver.
 *
 * This function sets up the mailbox receive channel with the callback
 * function specified in the DT node.
 *
 * @return 0 on success, negative error code on failure.
 */
static int mbox_init(void *shared_data)
{
	int ret;

	/* Register the callback function for the mailbox receive channel */
	ret = mbox_register_callback_dt(&rx_channel, mbox_callback, shared_data);
	if (ret < 0) {
		return ret;
	}

	/* Enable the mailbox receive channel */
	return mbox_set_enabled_dt(&rx_channel, true);
}
#endif

int main(void)
{
	int ret = 0;
#if defined(USE_STRUCT_COMMUNICATION)

	static shared_t *rx_data = (shared_t *)((uint8_t *)(DT_REG_ADDR(DT_NODELABEL(sram_rx))));

	ret = mbox_init((void *)rx_data);
	if (ret < 0) {
		return 0;
	}

	/* clear the buffer locks and their size holders */
	atomic_flag_clear(&rx_data->lock);
	rx_data->size = 0;

#elif defined(USE_ICMSG_BACKEND)
	const struct device *ipc0_instance;
	volatile uint32_t delay = 0;

#if !defined(CONFIG_SYS_CLOCK_EXISTS)
	/* Wait a little bit for IPC service to be ready on APP side */
	while (delay < 1000) {
		delay++;
	}
#endif

	ipc0_instance = DEVICE_DT_GET(DT_NODELABEL(ipc0));

	ret = ipc_service_open_instance(ipc0_instance);
	if ((ret < 0) && (ret != -EALREADY)) {
		return ret;
	}

	ret = ipc_service_register_endpoint(ipc0_instance, &ep, &ep_cfg);
	if (ret < 0) {
		return ret;
	}

	/* Wait for endpoint to be bound */
	while (bound_sem != 0) {
	};
#endif

	if (!nrf_vpr_csr_rtperiph_enable_check()) {
		nrf_vpr_csr_rtperiph_enable_set(true);
	}

	while (true) {
		k_cpu_idle();
	}

	return 0;
}
