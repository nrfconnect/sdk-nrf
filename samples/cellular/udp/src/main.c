/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <modem/lte_lc.h>
#include <zephyr/net/socket.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <nrf_modem_at.h>

#if defined(CONFIG_NRF_MODEM_LIB)
#include <modem/nrf_modem_lib.h>
#include <nrf_modem_at.h>
#endif

#define UDP_IP_HEADER_SIZE 28

static int client_fd;
static struct sockaddr_storage host_addr;
static struct k_work_delayable socket_transmission_work;
static struct k_work_delayable lte_set_connection_work;

static volatile enum lte_state_type {
	LTE_STATE_ONLINE,
	LTE_STATE_OFFLINE,
	LTE_STATE_BUSY
};
static volatile enum lte_state_type lte_current_state;
static volatile enum lte_state_type lte_target_state;

#define ENABLED	 true
#define DISABLED false
static volatile bool psm_target_state;
static volatile bool rai_target_state;
static volatile bool psm_current_state;
static volatile bool rai_current_state;
static int socket_connect(void);

#if defined(CONFIG_BOARD_NRF9160DK_NRF9160_NS) \
            || defined(CONFIG_BOARD_NRF9161DK_NRF9161_NS)

static const struct gpio_dt_spec buttons[] = {
	GPIO_DT_SPEC_GET_OR(DT_NODELABEL(button0), gpios, {0}),
	GPIO_DT_SPEC_GET_OR(DT_NODELABEL(button1), gpios, {0}),
	GPIO_DT_SPEC_GET_OR(DT_NODELABEL(button2), gpios, {0}),
	GPIO_DT_SPEC_GET_OR(DT_NODELABEL(button3), gpios, {0})};
static struct gpio_callback button_cb_data[4];
#elif defined(CONFIG_BOARD_THINGY91_NRF9160_NS)
static const struct gpio_dt_spec buttons[] = {
	GPIO_DT_SPEC_GET_OR(DT_NODELABEL(button0), gpios, {0}),
};
static struct gpio_callback button_cb_data[1];
#endif

#if defined(CONFIG_BOARD_NRF9160DK_NRF9160_NS)
static int sw1_previous_value;
static int sw1_current_value;
static int sw2_previous_value;
static int sw2_current_value;
#endif

void button_pressed(const struct device *dev, 
        struct gpio_callback *cb, uint32_t pins)
{
	printk("[%" PRIu32 "]:Button pressed!\n", k_cycle_get_32());
	/* Button1 pressed, send udp package */
	if (1 == gpio_pin_get_dt(&buttons[0])) {
		printk("Send UDP package!\n");
		k_work_reschedule(&socket_transmission_work, K_NO_WAIT);
	}
	/* Button2 pressed, LTE online/offline */
	if (1 == gpio_pin_get_dt(&buttons[1])) {
		if (lte_current_state == LTE_STATE_ONLINE) {
			lte_target_state = LTE_STATE_OFFLINE;
			k_work_reschedule(&lte_set_connection_work, K_NO_WAIT);
		} else if (lte_current_state == LTE_STATE_OFFLINE) {
			lte_target_state = LTE_STATE_ONLINE;
			k_work_reschedule(&lte_set_connection_work, K_NO_WAIT);
		}
	}

/* Switch 1 changed or Button3 pressed, PSM feature setting */
#if defined(CONFIG_UDP_PSM_ENABLE)

#if defined(CONFIG_BOARD_NRF9160DK_NRF9160_NS)
	/* set Switch1 to ground */
	sw1_current_value = gpio_pin_get_dt(&buttons[2]);
	if (sw1_current_value != sw1_previous_value) {
		sw1_previous_value = sw1_current_value;
		if (1 == sw1_current_value) {
			psm_target_state = ENABLED;
			printk("ENABLED PSM!\n");
		} else {
			psm_target_state = DISABLED;
			printk("DISABLE PSM!\n");
		}
		if (LTE_STATE_ONLINE == lte_current_state) {
			lte_target_state = LTE_STATE_ONLINE;
			printk("Reconfigure PSM and reconnect network!\n");
			k_work_reschedule(&lte_set_connection_work, K_NO_WAIT);
		}
	}
#elif defined(CONFIG_BOARD_NRF9161DK_NRF9161_NS)
	if (1 == gpio_pin_get_dt(&buttons[2])) {
		if (psm_current_state == ENABLED) {
			psm_target_state = ENABLED;
			printk("ENABLED PSM!\n");
		} else {
			psm_target_state = DISABLED;
			printk("DISABLE PSM!\n");
		}
		if (LTE_STATE_ONLINE == lte_current_state) {
			lte_target_state = LTE_STATE_ONLINE;
			printk("Reconfigure PSM and reconnect network!\n");
			k_work_reschedule(&lte_set_connection_work, K_NO_WAIT);
		}
	}
#endif
#else
	printk("CONFIG_UDP_PSM_ENABLE is not enabled in prj.conf!");
#endif

/* Switch 2 changed or Button4 pressed, PSM feature setting */
#if defined(CONFIG_UDP_RAI_ENABLE)

#if defined(CONFIG_BOARD_NRF9160DK_NRF9160_NS)
	/* set Switch1 to ground */
	sw2_current_value = gpio_pin_get_dt(&buttons[3]);
	if (sw2_current_value != sw2_previous_value) {
		sw2_previous_value = sw2_current_value;
		if (1 == sw2_current_value) {
			rai_target_state = ENABLED;
			printk("ENABLED RAI!\n");
		} else {
			rai_target_state = DISABLED;
			printk("DISABLE RAI!\n");
		}
		if (LTE_STATE_ONLINE == lte_current_state) {
			lte_target_state = LTE_STATE_ONLINE;
			printk("Reconfigure RAI and reconnect network!\n");
			k_work_reschedule(&lte_set_connection_work, K_NO_WAIT);
		}
	}
#elif defined(CONFIG_BOARD_NRF9161DK_NRF9161_NS)
	if (1 == gpio_pin_get_dt(&buttons[3])) {
		if (rai_current_state == ENABLED) {
			rai_target_state = ENABLED;
			printk("ENABLED RAI!\n");
		} else {
			rai_target_state = DISABLED;
			printk("DISABLE RAI!\n");
		}
		if (LTE_STATE_ONLINE == lte_current_state) {
			lte_target_state = LTE_STATE_ONLINE;
			printk("Reconfigure RAI and reconnect network!\n");
			k_work_reschedule(&lte_set_connection_work, K_NO_WAIT);
		}
	}
#endif
#else
	printk("CONFIG_UDP_RAI_ENABLE is not enabled in prj.conf!");
#endif
}

/* Initialize psm, rai and lte connection states. */
static void states_init()
{

#if defined(CONFIG_BOARD_NRF9160DK_NRF9160_NS)
	psm_target_state = gpio_pin_get_dt(&buttons[2]);
	rai_target_state = gpio_pin_get_dt(&buttons[3]);
#elif defined(CONFIG_BOARD_NRF9161DK_NRF9161_NS) \
             || defined(CONFIG_BOARD_THINGY91_NRF9160_NS)
	psm_target_state = ENABLED;
	rai_target_state = ENABLED;
#endif
	lte_target_state = LTE_STATE_ONLINE;
	lte_current_state = LTE_STATE_BUSY;
}

static int lte_lc_rel14feat_rai_req(bool enable)
{ /** Need to enable release 14 RAI feature at offline mode before run this command **/
	int err;
	enum lte_lc_system_mode mode;

	err = lte_lc_system_mode_get(&mode, NULL);
	if (err) {
		return -EFAULT;
	}

	switch (mode) {
	case LTE_LC_SYSTEM_MODE_LTEM:
	case LTE_LC_SYSTEM_MODE_LTEM_GPS:
	case LTE_LC_SYSTEM_MODE_NBIOT:
	case LTE_LC_SYSTEM_MODE_NBIOT_GPS:
	case LTE_LC_SYSTEM_MODE_LTEM_NBIOT:
	case LTE_LC_SYSTEM_MODE_LTEM_NBIOT_GPS:
		break;
	default:
		printk("Unknown system mode");
		printk("Cannot request RAI for unknown system mode");
		return -EOPNOTSUPP;
	}

	if (enable) {
		err = nrf_modem_at_printf("AT%%RAI=1");
	} else {
		err = nrf_modem_at_printf("AT%%RAI=0");
	}

	if (err) {
		printk("nrf_modem_at_printf failed, reported error: %d", err);
		return -EFAULT;
	}

	return 0;
}

static void button_init(void)
{
	int ret;
	for (size_t i = 0; i < ARRAY_SIZE(buttons); i++) {
		ret = gpio_pin_configure_dt(&buttons[i], GPIO_INPUT);
		if (ret != 0) {
			printk("Error %d: failed to configure %s pin %d\n", ret,
			       buttons[i].port->name, buttons[i].pin);
			return;
		}

		ret = gpio_pin_interrupt_configure_dt(&buttons[i],
                                                      GPIO_INT_EDGE_TO_ACTIVE);
		if (ret != 0) {
			printk("Error %d: failed to configure interrupt",ret);
                        printk("on %s pin %d\n",buttons[i].port->name, 
                                buttons[i].pin);
			return;
		}

		gpio_init_callback(&button_cb_data[i], button_pressed,
                                    BIT(buttons[i].pin));
		gpio_add_callback(buttons[i].port, &button_cb_data[i]);
		printk("Set up button at %s pin %d\n", buttons[i].port->name,
                         buttons[i].pin);
#if defined(CONFIG_BOARD_NRF9160DK_NRF9160_NS)
		/* set Switch1 to ground */
		sw1_current_value = gpio_pin_get_dt(&buttons[2]);
		sw1_previous_value = sw1_current_value;
		sw2_current_value = gpio_pin_get_dt(&buttons[3]);
		sw2_previous_value = sw2_current_value;
#endif
	}
}

static void socket_transmission_work_fn(struct k_work *work)
{
	int err;
	char buffer[CONFIG_UDP_DATA_UPLOAD_SIZE_BYTES] = {"\0"};

	socket_connect();
	printk("Transmitting UDP/IP payload of %d bytes to the ",
	       CONFIG_UDP_DATA_UPLOAD_SIZE_BYTES + UDP_IP_HEADER_SIZE);
	printk("IP address %s, port number %d\n",
	       CONFIG_UDP_SERVER_ADDRESS_STATIC,
	       CONFIG_UDP_SERVER_PORT);

	if (rai_current_state == ENABLED) {
		/* Let the modem know that this is the last packet for now and we do not
		 * wait for a response.
		 */
		err = setsockopt(client_fd, SOL_SOCKET, SO_RAI_LAST, NULL, 0);
		if (err) {
			printk("Failed to set socket option, error: %d\n",
                                 errno);
		}
	}

	err = send(client_fd, buffer, sizeof(buffer), 0);
	if (err < 0) {
		printk("Failed to transmit UDP packet, error: %d\n", errno);
	}
	close(client_fd);
	k_work_schedule(&socket_transmission_work,
			K_SECONDS(CONFIG_UDP_DATA_UPLOAD_FREQUENCY_SECONDS));
}

static void lte_set_connection_work_fn(struct k_work *work)
{
	int err;

	lte_current_state = LTE_STATE_BUSY;
	printk("Set LTE to: %s\n", lte_target_state ? "OFFLINE" : "ONLINE");
	if (LTE_STATE_OFFLINE == lte_target_state) {
		lte_lc_offline();
	} else if (LTE_STATE_ONLINE == lte_target_state) {
		lte_lc_offline();
#if defined(CONFIG_UDP_PSM_ENABLE)
		err = lte_lc_psm_req(psm_target_state);
		if (err) {
			printk("lte_lc_psm_req, error: %d\n", err);
		}
		psm_current_state = psm_target_state;
		printk("Set PSM mode to: %s\n",
                         psm_current_state ? "ENABLED" : "DISABLED");
#endif

#if defined(CONFIG_UDP_EDRX_ENABLE)
		/** enhanced Discontinuous Reception */
		err = lte_lc_edrx_req(true);
		if (err) {
			printk("lte_lc_edrx_req, error: %d\n", err);
		}
#else
		err = lte_lc_edrx_req(false);
		if (err) {
			printk("lte_lc_edrx_req, error: %d\n", err);
		}
#endif

#if defined(CONFIG_UDP_RAI_ENABLE)
#if defined(CONFIG_SOC_NRF9160)
		/** Enable release 14 RAI feature **/
		err = nrf_modem_at_printf("AT%%REL14FEAT=0,1,0,0,0");
		if (err) {
			printk("Release 14 RAI feature enable failed, err %d",
                               err);
		}
#endif
		/** Release Assistance Indication  */
		err = lte_lc_rel14feat_rai_req(rai_target_state);
		if (err) {
			printk("lte_lc_rel14feat_rai_req, error: %d\n", err);
		}
		rai_current_state = rai_target_state;
		printk("Set RAI to: %s\n", 
                        rai_current_state ? "ENABLED" : "DISABLED");

#endif
		lte_lc_normal();
	}
}

static void work_init(void)
{
	k_work_init_delayable(&socket_transmission_work,
			      socket_transmission_work_fn);
        k_work_init_delayable(&lte_set_connection_work,
                              lte_set_connection_work_fn);
}

#if defined(CONFIG_NRF_MODEM_LIB)
static void lte_handler(const struct lte_lc_evt *const evt)
{
	switch (evt->type) {
	case LTE_LC_EVT_NW_REG_STATUS:
		if ((evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME) ||
		    (evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_ROAMING)) {
			printk("Network registration status: %s\n",
			       evt->nw_reg_status == 
                                       LTE_LC_NW_REG_REGISTERED_HOME
				       ? "Connected - home"
				       : "Connected - roaming");
			lte_current_state = LTE_STATE_ONLINE;
		} else if (evt->nw_reg_status == LTE_LC_NW_REG_NOT_REGISTERED) {
			printk("Network registration status: ");
                        printk("Not registered / Offline\n");
			lte_current_state = LTE_STATE_OFFLINE;
		}
		break;
	case LTE_LC_EVT_PSM_UPDATE:
		printk("PSM parameter update: TAU: %d s, Active time: %d s\n",
			evt->psm_cfg.tau, evt->psm_cfg.active_time);
		break;
	case LTE_LC_EVT_EDRX_UPDATE:
		printf("eDRX parameter update: eDRX: %.2f s, PTW: %.2f s\n",
		       evt->edrx_cfg.edrx, evt->edrx_cfg.ptw);
		break;
	case LTE_LC_EVT_RRC_UPDATE:
		printk("RRC mode: %s\n",
			evt->rrc_mode == LTE_LC_RRC_MODE_CONNECTED ?
			"Connected" : "Idle\n");
		break;
	case LTE_LC_EVT_CELL_UPDATE:
		printk("LTE cell changed: Cell ID: %d, Tracking area: %d\n",
		       evt->cell.id, evt->cell.tac);
		break;
	default:
		break;
	}
}

static int modem_init(void)
{
	int err;

	err = nrf_modem_lib_init();
	if (err) {
		printk("Failed to initialize modem library, error: %d\n", err);
		return err;
	}

	err = lte_lc_init();
	if (err) {
		printk("Failed to initialize LTE link control, error: %d\n",
                 err);
		return err;
	}

	return 0;
}

#endif /* CONFIG_NRF_MODEM_LIB */

static void host_addr_init(void)
{
	struct sockaddr_in *server4 = ((struct sockaddr_in *)&host_addr);

	server4->sin_family = AF_INET;
	server4->sin_port = htons(CONFIG_UDP_SERVER_PORT);

	inet_pton(AF_INET, CONFIG_UDP_SERVER_ADDRESS_STATIC, &server4->sin_addr);
}

static int socket_connect(void)
{
	int err;

	host_addr_init();

	client_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (client_fd < 0) {
		printk("Failed to create UDP socket, error: %d\n", errno);
		err = -errno;
		return err;
	}

	err = connect(client_fd, (struct sockaddr *)&host_addr,
		      sizeof(struct sockaddr_in));
	if (err < 0) {
		printk("Failed to connect socket, error: %d\n", errno);
		close(client_fd);
		return err;
	}

	return 0;
}

int main(void)
{
	int err;

	printk("UDP sample has started\n");

	button_init();
	states_init();
	work_init();

#if defined(CONFIG_NRF_MODEM_LIB)
	err = modem_init();
	if (err) {
		return -1;
	}
	lte_lc_register_handler(lte_handler);
	k_work_schedule(&lte_set_connection_work, K_NO_WAIT);
	while (LTE_STATE_BUSY == lte_current_state) {
		printk("lte_set_connection BUSY!\n");
		k_sleep(K_SECONDS(3));
	}
#endif
	k_work_schedule(&socket_transmission_work, K_NO_WAIT);

	return 0;
}
