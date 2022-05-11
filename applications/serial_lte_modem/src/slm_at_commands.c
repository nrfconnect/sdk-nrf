/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/types.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <stdio.h>
#include <ctype.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/uart.h>
#include <string.h>
#include <zephyr/init.h>
#include <modem/at_cmd_parser.h>
#include <modem/modem_jwt.h>
#include <zephyr/sys/reboot.h>
#include "ncs_version.h"

#include "slm_util.h"
#include "slm_at_host.h"
#include "slm_at_tcp_proxy.h"
#include "slm_at_udp_proxy.h"
#include "slm_at_socket.h"
#if defined(CONFIG_SLM_NATIVE_TLS)
#include "slm_at_cmng.h"
#endif
#include "slm_at_icmp.h"
#include "slm_at_sms.h"
#include "slm_at_fota.h"
#if defined(CONFIG_SLM_GNSS)
#include "slm_at_gnss.h"
#endif
#if defined(CONFIG_SLM_FTPC)
#include "slm_at_ftp.h"
#endif
#if defined(CONFIG_SLM_MQTTC)
#include "slm_at_mqtt.h"
#endif
#if defined(CONFIG_SLM_HTTPC)
#include "slm_at_httpc.h"
#endif
#if defined(CONFIG_SLM_TWI)
#include "slm_at_twi.h"
#endif
#if defined(CONFIG_SLM_GPIO)
#include "slm_at_gpio.h"
#endif
#if defined(CONFIG_SLM_NRF52_DFU)
#include "slm_at_dfu.h"
#endif

LOG_MODULE_REGISTER(slm_at, CONFIG_SLM_LOG_LEVEL);

/* This delay is necessary for at_host to send response message in low baud rate. */
#define SLM_UART_RESPONSE_DELAY 50

/**@brief Shutdown modes. */
enum shutdown_modes {
	SHUTDOWN_MODE_INVALID,
	SHUTDOWN_MODE_SLEEP,
	SHUTDOWN_MODE_IDLE
};

/**@brief AT command handler type. */
typedef int (*slm_at_handler_t) (enum at_cmd_type);

static struct slm_work_info {
	struct k_work_delayable uart_work;
	struct k_work_delayable sleep_work;
	uint32_t data;
} slm_work;

/* global variable defined in different files */
extern struct at_param_list at_param_list;
extern char rsp_buf[SLM_AT_CMD_RESPONSE_MAX_LEN];
extern uint16_t datamode_time_limit;
extern struct uart_config slm_uart;

/* global functions defined in different files */
void enter_idle(void);
void enter_sleep(void);
int slm_uart_configure(void);
int poweroff_uart(void);
bool verify_datamode_control(uint16_t time_limit, uint16_t *time_limit_min);
extern int slm_setting_uart_save(void);

static void modem_power_off(void)
{
	/*
	 * The LTE modem also needs to be stopped by issuing AT command
	 * through the modem API, before entering System OFF mode.
	 * Once the command is issued, one should wait for the modem
	 * to respond that it actually has stopped as there may be a
	 * delay until modem is disconnected from the network.
	 * Refer to https://infocenter.nordicsemi.com/topic/ps_nrf9160/
	 * pmu.html?cp=2_0_0_4_0_0_1#system_off_mode
	 */
	(void)nrf_modem_at_printf("AT+CFUN=0");
	k_sleep(K_SECONDS(1));
}

/**@brief handle AT#XSLMVER commands
 *  #XSLMVER
 *  AT#XSLMVER? not supported
 *  AT#XSLMVER=? not supported
 */
static int handle_at_slmver(enum at_cmd_type type)
{
	int ret = -EINVAL;

	if (type == AT_CMD_TYPE_SET_COMMAND) {
#if defined(CONFIG_SLM_CUSTOMIZED)
		sprintf(rsp_buf, "\r\n#XSLMVER: %s-CUSTOMIZED\r\n",
			STRINGIFY(NCS_VERSION_STRING));
#else
		sprintf(rsp_buf, "\r\n#XSLMVER: %s\r\n",
			STRINGIFY(NCS_VERSION_STRING));
#endif
		rsp_send(rsp_buf, strlen(rsp_buf));
		ret = 0;
	}

	return ret;
}
static void go_sleep_wk(struct k_work *work)
{
	ARG_UNUSED(work);

	if (slm_work.data == SHUTDOWN_MODE_IDLE) {
		if (poweroff_uart() == 0) {
			enter_idle();
		} else {
			LOG_ERR("failed to power off UART");
		}
	} else if (slm_work.data == SHUTDOWN_MODE_SLEEP) {
		slm_at_host_uninit();
		modem_power_off();
		enter_sleep();
	}
}

/**@brief handle AT#XSLEEP commands
 *  AT#XSLEEP=<shutdown_mode>
 *  AT#XSLEEP? not supported
 *  AT#XSLEEP=?
 */
static int handle_at_sleep(enum at_cmd_type type)
{
	int ret = -EINVAL;

	if (type == AT_CMD_TYPE_SET_COMMAND) {
		ret = at_params_unsigned_int_get(&at_param_list, 1, &slm_work.data);
		if (ret) {
			return -EINVAL;
		}
		if (slm_work.data == SHUTDOWN_MODE_SLEEP || slm_work.data == SHUTDOWN_MODE_IDLE) {
			k_work_reschedule(&slm_work.sleep_work, K_MSEC(100));
		} else {
			ret = -EINVAL;
		}
	} else if (type == AT_CMD_TYPE_TEST_COMMAND) {
		sprintf(rsp_buf, "\r\n#XSLEEP: (%d,%d)\r\n", SHUTDOWN_MODE_SLEEP,
			SHUTDOWN_MODE_IDLE);
		rsp_send(rsp_buf, strlen(rsp_buf));
		ret = 0;
	}

	return ret;
}

/**@brief handle AT#XRESET commands
 *  AT#XRESET
 *  AT#XRESET? not supported
 *  AT#XRESET=? not supported
 */
static int handle_at_reset(enum at_cmd_type type)
{
	int ret = -EINVAL;
	char ok_str[] = "\r\nOK\r\n";

	if (type == AT_CMD_TYPE_SET_COMMAND) {
		rsp_send(ok_str, strlen(ok_str));
		k_sleep(K_MSEC(SLM_UART_RESPONSE_DELAY));
		slm_at_host_uninit();
		modem_power_off();
		sys_reboot(SYS_REBOOT_COLD);
	}

	return ret;
}

/**@brief handle AT#XUUID commands
 *  AT#XUUID
 *  AT#XUUID? not supported
 *  AT#XUUID=? not supported
 */
static int handle_at_uuid(enum at_cmd_type type)
{
	int ret;

	if (type != AT_CMD_TYPE_SET_COMMAND) {
		return -EINVAL;
	}

	struct nrf_device_uuid dev = {0};

	ret = modem_jwt_get_uuids(&dev, NULL);
	if (ret) {
		LOG_ERR("Get device UUID error: %d", ret);
	} else {
		sprintf(rsp_buf, "\r\n#XUUID: %s\r\n", dev.str);
		rsp_send(rsp_buf, strlen(rsp_buf));
	}

	return ret;
}

static void set_uart_wk(struct k_work *work)
{
	int err;

	err = slm_uart_configure();
	if (err != 0) {
		LOG_ERR("slm_uart_configure: %d", err);
		return;
	}
	err = slm_setting_uart_save();
	if (err != 0) {
		LOG_ERR("uart_config_set: %d", err);
	}
}

/**@brief handle AT#XSLMUART commands
 *  AT#XSLMUART[=<baud_rate>,<hwfc>]
 *  AT#XSLMUART?
 *  AT#XSLMUART=?
 */
static int handle_at_slmuart(enum at_cmd_type type)
{
	int ret = -EINVAL;

	if (type == AT_CMD_TYPE_SET_COMMAND) {
		uint32_t baudrate;
		uint16_t hwfc;

		ret = at_params_unsigned_int_get(&at_param_list, 1, &baudrate);

		if (ret == 0) {
			switch (baudrate) {
			case 1200:
			case 2400:
			case 4800:
			case 9600:
			case 14400:
			case 19200:
			case 38400:
			case 57600:
			case 115200:
			case 230400:
			case 460800:
			case 921600:
			case 1000000:
				slm_uart.baudrate = baudrate;
				break;
			default:
				LOG_ERR("Invalid uart baud rate provided. %d", baudrate);
				return -EINVAL;
			}
		}
#if defined(CONFIG_SLM_UART_HWFC_RUNTIME)
		ret = at_params_unsigned_short_get(&at_param_list, 2, &hwfc);
		if (ret == 0) {
			if ((hwfc != UART_CFG_FLOW_CTRL_RTS_CTS) &&
				(hwfc != UART_CFG_FLOW_CTRL_NONE)) {
				LOG_ERR("Invalid uart hwfc provided.");
				return -EINVAL;
			}
		}
#else
		hwfc = UART_CFG_FLOW_CTRL_NONE;
#endif
		slm_uart.flow_ctrl = hwfc;

		ret = k_work_reschedule(&slm_work.uart_work, K_MSEC(SLM_UART_RESPONSE_DELAY));
		if (ret > 0) {
			ret = 0;
		}
	}
	if (type == AT_CMD_TYPE_READ_COMMAND) {
		sprintf(rsp_buf, "\r\n#XSLMUART: %d,%d\r\n", slm_uart.baudrate, slm_uart.flow_ctrl);
		rsp_send(rsp_buf, strlen(rsp_buf));
		ret = 0;
	}
	if (type == AT_CMD_TYPE_TEST_COMMAND) {
#if defined(CONFIG_SLM_UART_HWFC_RUNTIME)
		sprintf(rsp_buf, "\r\n#XSLMUART: (1200,2400,4800,9600,14400,19200,38400,57600,"
				 "115200,230400,460800,921600,1000000),(0,1)\r\n");
#else
		sprintf(rsp_buf, "\r\n#XSLMUART: (1200,2400,4800,9600,14400,19200,38400,57600,"
				 "115200,230400,460800,921600,1000000)\r\n");
#endif
		rsp_send(rsp_buf, strlen(rsp_buf));
		ret = 0;
	}
	return ret;
}

/**@brief handle AT#XDATACTRL commands
 *  AT#XDATACTRL=<time_limit>
 *  AT#XDATACTRL?
 *  AT#XDATACTRL=?
 */
static int handle_at_datactrl(enum at_cmd_type cmd_type)
{
	int ret = 0;
	uint16_t time_limit, time_limit_min;

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		ret = at_params_unsigned_short_get(&at_param_list, 1, &time_limit);
		if (ret) {
			return ret;
		}
		if (verify_datamode_control(time_limit, NULL)) {
			datamode_time_limit = time_limit;
		} else {
			return -EINVAL;
		}
		break;

	case AT_CMD_TYPE_READ_COMMAND:
		(void)verify_datamode_control(datamode_time_limit, &time_limit_min);
		sprintf(rsp_buf, "\r\n#XDATACTRL: %d,%d\r\n", datamode_time_limit, time_limit_min);
		rsp_send(rsp_buf, strlen(rsp_buf));
		break;

	case AT_CMD_TYPE_TEST_COMMAND:
		sprintf(rsp_buf, "\r\n#XDATACTRL=<time_limit>\r\n");
		rsp_send(rsp_buf, strlen(rsp_buf));
		break;

	default:
		break;
	}

	return ret;
}

/**@brief handle AT#XCLAC commands
 *  AT#XCLAC
 *  AT#XCLAC? not supported
 *  AT#XCLAC=? not supported
 */
int handle_at_clac(enum at_cmd_type cmd_type);

/* TCP proxy commands */
int handle_at_tcp_server(enum at_cmd_type cmd_type);
int handle_at_tcp_client(enum at_cmd_type cmd_type);
int handle_at_tcp_send(enum at_cmd_type cmd_type);
int handle_at_tcp_hangup(enum at_cmd_type cmd_type);

/* UDP proxy commands */
int handle_at_udp_server(enum at_cmd_type cmd_type);
int handle_at_udp_client(enum at_cmd_type cmd_type);
int handle_at_udp_send(enum at_cmd_type cmd_type);

/* Socket-type TCPIP commands */
int handle_at_socket(enum at_cmd_type cmd_type);
int handle_at_secure_socket(enum at_cmd_type cmd_type);
int handle_at_socket_select(enum at_cmd_type cmd_type);
int handle_at_socketopt(enum at_cmd_type cmd_type);
int handle_at_secure_socketopt(enum at_cmd_type cmd_type);
int handle_at_bind(enum at_cmd_type cmd_type);
int handle_at_connect(enum at_cmd_type cmd_type);
int handle_at_listen(enum at_cmd_type cmd_type);
int handle_at_accept(enum at_cmd_type cmd_type);
int handle_at_send(enum at_cmd_type cmd_type);
int handle_at_recv(enum at_cmd_type cmd_type);
int handle_at_sendto(enum at_cmd_type cmd_type);
int handle_at_recvfrom(enum at_cmd_type cmd_type);
int handle_at_poll(enum at_cmd_type cmd_type);
int handle_at_getaddrinfo(enum at_cmd_type cmd_type);

#if defined(CONFIG_SLM_NATIVE_TLS)
int handle_at_xcmng(enum at_cmd_type cmd_type);
#endif

/* ICMP commands */
int handle_at_icmp_ping(enum at_cmd_type cmd_type);

#if defined(CONFIG_SLM_SMS)
/* SMS commands */
int handle_at_sms(enum at_cmd_type cmd_type);
#endif

/* FOTA commands */
int handle_at_fota(enum at_cmd_type cmd_type);

#if defined(CONFIG_SLM_GNSS)
int handle_at_gps(enum at_cmd_type cmd_type);
int handle_at_nrf_cloud(enum at_cmd_type cmd_type);
int handle_at_agps(enum at_cmd_type cmd_type);
int handle_at_pgps(enum at_cmd_type cmd_type);
int handle_at_cellpos(enum at_cmd_type cmd_type);
#endif

#if defined(CONFIG_SLM_FTPC)
int handle_at_ftp(enum at_cmd_type cmd_type);
#endif

#if defined(CONFIG_SLM_MQTTC)
int handle_at_mqtt_connect(enum at_cmd_type cmd_type);
int handle_at_mqtt_publish(enum at_cmd_type cmd_type);
int handle_at_mqtt_subscribe(enum at_cmd_type cmd_type);
int handle_at_mqtt_unsubscribe(enum at_cmd_type cmd_type);
#endif

#if defined(CONFIG_SLM_HTTPC)
int handle_at_httpc_connect(enum at_cmd_type cmd_type);
int handle_at_httpc_request(enum at_cmd_type cmd_type);
#endif

#if defined(CONFIG_SLM_TWI)
int handle_at_twi_list(enum at_cmd_type cmd_type);
int handle_at_twi_write(enum at_cmd_type cmd_type);
int handle_at_twi_read(enum at_cmd_type cmd_type);
int handle_at_twi_write_read(enum at_cmd_type cmd_type);
#endif

#if defined(CONFIG_SLM_GPIO)
int handle_at_gpio_configure(enum at_cmd_type cmd_type);
int handle_at_gpio_operate(enum at_cmd_type cmd_type);
#endif

#if defined(CONFIG_SLM_NRF52_DFU)
int handle_at_dfu_get(enum at_cmd_type cmd_type);
int handle_at_dfu_run(enum at_cmd_type cmd_type);
#endif

static struct slm_at_cmd {
	char *string;
	slm_at_handler_t handler;
} slm_at_cmd_list[] = {
	/* Generic commands */
	{"AT#XSLMVER", handle_at_slmver},
	{"AT#XSLEEP", handle_at_sleep},
	{"AT#XRESET", handle_at_reset},
	{"AT#XUUID", handle_at_uuid},
	{"AT#XCLAC", handle_at_clac},
	{"AT#XSLMUART", handle_at_slmuart},
	{"AT#XDATACTRL", handle_at_datactrl},

	/* TCP proxy commands */
	{"AT#XTCPSVR", handle_at_tcp_server},
	{"AT#XTCPCLI", handle_at_tcp_client},
	{"AT#XTCPSEND", handle_at_tcp_send},
	{"AT#XTCPHANGUP", handle_at_tcp_hangup},

	/* UDP proxy commands */
	{"AT#XUDPSVR", handle_at_udp_server},
	{"AT#XUDPCLI", handle_at_udp_client},
	{"AT#XUDPSEND", handle_at_udp_send},

	/* Socket-type TCPIP commands */
	{"AT#XSOCKET", handle_at_socket},
	{"AT#XSSOCKET", handle_at_secure_socket},
	{"AT#XSOCKETSELECT", handle_at_socket_select},
	{"AT#XSOCKETOPT", handle_at_socketopt},
	{"AT#XSSOCKETOPT", handle_at_secure_socketopt},
	{"AT#XBIND", handle_at_bind},
	{"AT#XCONNECT", handle_at_connect},
	{"AT#XLISTEN", handle_at_listen},
	{"AT#XACCEPT", handle_at_accept},
	{"AT#XSEND", handle_at_send},
	{"AT#XRECV", handle_at_recv},
	{"AT#XSENDTO", handle_at_sendto},
	{"AT#XRECVFROM", handle_at_recvfrom},
	{"AT#XPOLL", handle_at_poll},
	{"AT#XGETADDRINFO", handle_at_getaddrinfo},

#if defined(CONFIG_SLM_NATIVE_TLS)
	{"AT#XCMNG", handle_at_xcmng},
#endif
	/* ICMP commands */
	{"AT#XPING", handle_at_icmp_ping},

#if defined(CONFIG_SLM_SMS)
	/* SMS commands */
	{"AT#XSMS", handle_at_sms},
#endif

	/* FOTA commands */
	{"AT#XFOTA", handle_at_fota},

#if defined(CONFIG_SLM_GNSS)
	/* GNSS commands */
	{"AT#XGPS", handle_at_gps},
	{"AT#XNRFCLOUD", handle_at_nrf_cloud},
#if defined(CONFIG_SLM_AGPS)
	{"AT#XAGPS", handle_at_agps},
#endif
#if defined(CONFIG_SLM_PGPS)
	{"AT#XPGPS", handle_at_pgps},
#endif
#if defined(CONFIG_SLM_CELL_POS)
	{"AT#XCELLPOS", handle_at_cellpos},
#endif
#endif

#if defined(CONFIG_SLM_FTPC)
	/* FTP commands */
	{"AT#XFTP", handle_at_ftp},
#endif

#if defined(CONFIG_SLM_MQTTC)
	{"AT#XMQTTCON", handle_at_mqtt_connect},
	{"AT#XMQTTPUB", handle_at_mqtt_publish},
	{"AT#XMQTTSUB", handle_at_mqtt_subscribe},
	{"AT#XMQTTUNSUB", handle_at_mqtt_unsubscribe},
#endif

#if defined(CONFIG_SLM_HTTPC)
	{"AT#XHTTPCCON", handle_at_httpc_connect},
	{"AT#XHTTPCREQ", handle_at_httpc_request},
#endif

#if defined(CONFIG_SLM_TWI)
	{"AT#XTWILS", handle_at_twi_list},
	{"AT#XTWIW", handle_at_twi_write},
	{"AT#XTWIR", handle_at_twi_read},
	{"AT#XTWIWR", handle_at_twi_write_read},
#endif

#if defined(CONFIG_SLM_GPIO)
	{"AT#XGPIOCFG", handle_at_gpio_configure},
	{"AT#XGPIO", handle_at_gpio_operate},
#endif

#if defined(CONFIG_SLM_NRF52_DFU)
	{"AT#XDFUGET", handle_at_dfu_get},
	{"AT#XDFURUN", handle_at_dfu_run},
#endif
};

int handle_at_clac(enum at_cmd_type cmd_type)
{
	int ret = -EINVAL;

	if (cmd_type == AT_CMD_TYPE_SET_COMMAND) {
		int total = ARRAY_SIZE(slm_at_cmd_list);

		for (int i = 0; i < total; i++) {
			sprintf(rsp_buf, "%s\r\n", slm_at_cmd_list[i].string);
			rsp_send(rsp_buf, strlen(rsp_buf));
		}
		ret = 0;
	}

	return ret;
}

int slm_at_parse(const char *at_cmd)
{
	int ret = -ENOENT;
	int total = ARRAY_SIZE(slm_at_cmd_list);

	for (int i = 0; i < total; i++) {
		if (slm_util_cmd_casecmp(at_cmd, slm_at_cmd_list[i].string)) {
			enum at_cmd_type type = at_parser_cmd_type_get(at_cmd);

			at_params_list_clear(&at_param_list);
			ret = at_parser_params_from_str(at_cmd, NULL, &at_param_list);
			if (ret) {
				LOG_ERR("Failed to parse AT command %d", ret);
				return -EINVAL;
			}
			ret = slm_at_cmd_list[i].handler(type);
			break;
		}
	}

	return ret;
}

int slm_at_init(void)
{
	int err;

	k_work_init_delayable(&slm_work.uart_work, set_uart_wk);
	k_work_init_delayable(&slm_work.sleep_work, go_sleep_wk);

	err = slm_at_tcp_proxy_init();
	if (err) {
		LOG_ERR("TCP Server could not be initialized: %d", err);
		return -EFAULT;
	}
	err = slm_at_udp_proxy_init();
	if (err) {
		LOG_ERR("UDP Server could not be initialized: %d", err);
		return -EFAULT;
	}
	err = slm_at_socket_init();
	if (err) {
		LOG_ERR("TCPIP could not be initialized: %d", err);
		return -EFAULT;
	}
#if defined(CONFIG_SLM_NATIVE_TLS)
	err = slm_at_cmng_init();
	if (err) {
		LOG_ERR("TLS could not be initialized: %d", err);
		return -EFAULT;
	}
#endif
	err = slm_at_icmp_init();
	if (err) {
		LOG_ERR("ICMP could not be initialized: %d", err);
		return -EFAULT;
	}
#if defined(CONFIG_SLM_SMS)
	err = slm_at_sms_init();
	if (err) {
		LOG_ERR("SMS could not be initialized: %d", err);
		return -EFAULT;
	}
#endif
	err = slm_at_fota_init();
	if (err) {
		LOG_ERR("FOTA could not be initialized: %d", err);
		return -EFAULT;
	}
#if defined(CONFIG_SLM_GNSS)
	err = slm_at_gnss_init();
	if (err) {
		LOG_ERR("GPS could not be initialized: %d", err);
		return -EFAULT;
	}
#endif
#if defined(CONFIG_SLM_FTPC)
	err = slm_at_ftp_init();
	if (err) {
		LOG_ERR("FTP could not be initialized: %d", err);
		return -EFAULT;
	}
#endif
#if defined(CONFIG_SLM_MQTTC)
	err = slm_at_mqtt_init();
	if (err) {
		LOG_ERR("MQTT could not be initialized: %d", err);
		return -EFAULT;
	}
#endif
#if defined(CONFIG_SLM_HTTPC)
	err = slm_at_httpc_init();
	if (err) {
		LOG_ERR("HTTP could not be initialized: %d", err);
		return -EFAULT;
	}
#endif
#if defined(CONFIG_SLM_GPIO)
	err = slm_at_gpio_init();
	if (err) {
		LOG_ERR("GPIO could not be initialized: %d", err);
		return -EFAULT;
	}
#endif
#if defined(CONFIG_SLM_TWI)
	err = slm_at_twi_init();
	if (err) {
		LOG_ERR("TWI could not be initialized: %d", err);
		return -EFAULT;
	}
#endif
#if defined(CONFIG_SLM_NRF52_DFU)
	err = slm_at_dfu_init();
	if (err) {
		LOG_ERR("DFU could not be initialized: %d", err);
		return -EFAULT;
	}
#endif

	return err;
}

void slm_at_uninit(void)
{
	int err;

	err = slm_at_tcp_proxy_uninit();
	if (err) {
		LOG_WRN("TCP Server could not be uninitialized: %d", err);
	}
	err = slm_at_udp_proxy_uninit();
	if (err) {
		LOG_WRN("UDP Server could not be uninitialized: %d", err);
	}
	err = slm_at_socket_uninit();
	if (err) {
		LOG_WRN("TCPIP could not be uninitialized: %d", err);
	}
#if defined(CONFIG_SLM_NATIVE_TLS)
	err = slm_at_cmng_uninit();
	if (err) {
		LOG_WRN("TLS could not be uninitialized: %d", err);
	}
#endif
	err = slm_at_icmp_uninit();
	if (err) {
		LOG_WRN("ICMP could not be uninitialized: %d", err);
	}
#if defined(CONFIG_SLM_SMS)
	err = slm_at_sms_uninit();
	if (err) {
		LOG_WRN("SMS could not be uninitialized: %d", err);
	}
#endif
	err = slm_at_fota_uninit();
	if (err) {
		LOG_WRN("FOTA could not be uninitialized: %d", err);
	}
#if defined(CONFIG_SLM_GNSS)
	err = slm_at_gnss_uninit();
	if (err) {
		LOG_WRN("GPS could not be uninitialized: %d", err);
	}
#endif
#if defined(CONFIG_SLM_FTPC)
	err = slm_at_ftp_uninit();
	if (err) {
		LOG_WRN("FTP could not be uninitialized: %d", err);
	}
#endif
#if defined(CONFIG_SLM_MQTTC)
	err = slm_at_mqtt_uninit();
	if (err) {
		LOG_WRN("MQTT could not be uninitialized: %d", err);
	}
#endif
#if defined(CONFIG_SLM_HTTPC)
	err = slm_at_httpc_uninit();
	if (err) {
		LOG_WRN("HTTP could not be uninitialized: %d", err);
	}
#endif
#if defined(CONFIG_SLM_TWI)
	err = slm_at_twi_uninit();
	if (err) {
		LOG_ERR("TWI could not be uninit: %d", err);
	}
#endif
#if defined(CONFIG_SLM_GPIO)
	err = slm_at_gpio_uninit();
	if (err) {
		LOG_ERR("GPIO could not be uninit: %d", err);
	}
#endif
#if defined(CONFIG_SLM_NRF52_DFU)
	err = slm_at_dfu_uninit();
	if (err) {
		LOG_ERR("DFU could not be uninitialized: %d", err);
	}
#endif
}
