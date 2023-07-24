/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <zephyr/init.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/timeutil.h>
#include <zephyr/types.h>
#include <dfu/dfu_target.h>
#include <modem/at_cmd_parser.h>
#include <modem/lte_lc.h>
#include <modem/modem_jwt.h>
#include <modem/nrf_modem_lib.h>
#include "nrf_modem.h"
#include "ncs_version.h"

#include "slm_util.h"
#include "slm_settings.h"
#include "slm_at_host.h"
#include "slm_at_tcp_proxy.h"
#include "slm_at_udp_proxy.h"
#include "slm_at_socket.h"
#include "slm_at_icmp.h"
#include "slm_at_sms.h"
#include "slm_at_fota.h"
#if defined(CONFIG_SLM_NATIVE_TLS)
#include "slm_at_cmng.h"
#endif
#if defined(CONFIG_SLM_NRF_CLOUD)
#include "slm_at_nrfcloud.h"
#endif
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
#if defined(CONFIG_SLM_CARRIER)
#include "slm_at_carrier.h"
#endif

LOG_MODULE_REGISTER(slm_at, CONFIG_SLM_LOG_LEVEL);

/* This delay is necessary for at_host to send response message in low baud rate. */
#define SLM_UART_RESPONSE_DELAY 50

/** @brief Shutdown modes. */
enum sleep_modes {
	SLEEP_MODE_INVALID,
	SLEEP_MODE_DEEP,
	SLEEP_MODE_IDLE
};

/** @brief AT command handler type. */
typedef int (*slm_at_handler_t) (enum at_cmd_type);

static struct slm_work_info {
	struct k_work_delayable uart_work;
	struct k_work_delayable sleep_work;
	uint32_t data;
} slm_work;

/* global variable defined in different files */
extern struct at_param_list at_param_list;
extern uint16_t datamode_time_limit;
extern struct uart_config slm_uart;

/* global functions defined in different files */
void enter_idle(void);
void enter_sleep(void);
void enter_shutdown(void);
int slm_uart_configure(void);
int poweroff_uart(void);
bool verify_datamode_control(uint16_t time_limit, uint16_t *time_limit_min);

/** @return Whether the modem is in the given functional mode. */
static bool is_modem_functional_mode(enum lte_lc_func_mode mode)
{
	int cfun;
	int rc = nrf_modem_at_scanf("AT+CFUN?", "+CFUN: %d", &cfun);

	return (rc == 1 && cfun == mode);
}

static void modem_power_off(void)
{
	/* First check whether the modem has already been turned off by the MCU. */
	if (!is_modem_functional_mode(LTE_LC_FUNC_MODE_POWER_OFF)) {

		/* "[...] there may be a delay until modem is disconnected from the network."
		 * https://infocenter.nordicsemi.com/topic/ps_nrf9160/chapters/pmu/doc/operationmodes/system_off_mode.html
		 * This will return once the modem responds, which means it has actually
		 * stopped. This has been observed to take between 1 and 2 seconds.
		 */
		nrf_modem_at_printf("AT+CFUN=0");
	}
}

/** @brief Handles AT#XSLMVER command.
 *  AT#XSLMVER
 *  AT#XSLMVER? not supported
 *  AT#XSLMVER=? not supported
 */
static int handle_at_slmver(enum at_cmd_type type)
{
	int ret = -EINVAL;

	if (type == AT_CMD_TYPE_SET_COMMAND) {
		char *libmodem = nrf_modem_build_version();

		rsp_send("\r\n#XSLMVER: %s,\"%s\"\r\n", STRINGIFY(NCS_VERSION_STRING), libmodem);
		ret = 0;
	}

	return ret;
}

static void go_sleep_wk(struct k_work *work)
{
	ARG_UNUSED(work);

	if (slm_work.data == SLEEP_MODE_IDLE) {
		if (poweroff_uart() == 0) {
			enter_idle();
		} else {
			LOG_ERR("failed to power off UART");
		}
	} else if (slm_work.data == SLEEP_MODE_DEEP) {
		slm_at_host_uninit();

		/* Only power off the modem if it has not been put
		 * in flight mode to allow reducing NVM wear.
		 */
		if (!is_modem_functional_mode(LTE_LC_FUNC_MODE_OFFLINE)) {
			modem_power_off();
		}
		enter_sleep();
	}
}

/** @brief Handles AT#XSLEEP commands.
 *  AT#XSLEEP=<sleep_mode>
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
		if (slm_work.data == SLEEP_MODE_DEEP || slm_work.data == SLEEP_MODE_IDLE) {
			k_work_reschedule(&slm_work.sleep_work, K_MSEC(100));
		} else {
			ret = -EINVAL;
		}
	} else if (type == AT_CMD_TYPE_TEST_COMMAND) {
		rsp_send("\r\n#XSLEEP: (%d,%d)\r\n", SLEEP_MODE_DEEP, SLEEP_MODE_IDLE);
		ret = 0;
	}

	return ret;
}

/** @brief Handles AT#XSHUTDOWN command.
 *  AT#XSHUTDOWN
 *  AT#XSHUTDOWN? not supported
 *  AT#XSHUTDOWN=? not supported
 */
static int handle_at_shutdown(enum at_cmd_type type)
{
	int ret = -EINVAL;

	if (type == AT_CMD_TYPE_SET_COMMAND) {
		rsp_send_ok();
		k_sleep(K_MSEC(SLM_UART_RESPONSE_DELAY));
		slm_at_host_uninit();
		modem_power_off();
		enter_shutdown();
	}

	return ret;
}

/** @brief Handles AT#XRESET command.
 *  AT#XRESET
 *  AT#XRESET? not supported
 *  AT#XRESET=? not supported
 */
static int handle_at_reset(enum at_cmd_type type)
{
	int ret = -EINVAL;

	if (type == AT_CMD_TYPE_SET_COMMAND) {
		rsp_send_ok();
		k_sleep(K_MSEC(SLM_UART_RESPONSE_DELAY));
		slm_at_host_uninit();
		modem_power_off();
		LOG_PANIC();
		sys_reboot(SYS_REBOOT_COLD);
	}

	return ret;
}

/** @brief Handles AT#XMODEMRESET command.
 *  AT#XMODEMRESET
 *  AT#XMODEMRESET? not supported
 *  AT#XMODEMRESET=? not supported
 */
static int handle_at_modemreset(enum at_cmd_type type)
{
	if (type != AT_CMD_TYPE_SET_COMMAND) {
		return -EINVAL;
	}

	/* The modem must be put in minimal function mode before being shut down. */
	modem_power_off();

	unsigned int step = 1;
	int ret;

	do {
		ret = nrf_modem_lib_shutdown();
		if (ret != 0) {
			break;
		}
		++step;

		ret = nrf_modem_lib_init();
		if (ret < 0) {
			break;
		}
		++step;

		if (ret > 0 || (fota_stage != FOTA_STAGE_INIT
					&& fota_type == DFU_TARGET_IMAGE_TYPE_MODEM_DELTA)) {
			slm_finish_modem_fota(ret);
			slm_fota_post_process();
		}

		/* Success. */
		rsp_send("\r\n#XMODEMRESET: 0\r\n");
		return 0;
	} while (0);

	/* Error; print the step that failed and its error code. */
	rsp_send("\r\n#XMODEMRESET: %u,%d\r\n", step, ret);
	return 0;
}

/** @brief Handles AT#XUUID command.
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
		rsp_send("\r\n#XUUID: %s\r\n", dev.str);
	}

	return ret;
}

/** @brief Handles AT#XCCLK command.
 *  AT#XCCLK
 *  AT#XCCLK? not supported
 *  AT#XCCLK=? not supported
 */
static int handle_at_cclk(enum at_cmd_type type)
{
	int ret;
	struct tm tm_time = {
		.tm_sec = 0,
		.tm_min = 0,
		.tm_hour = 0,
		.tm_mday = 0,
		.tm_mon = 0,
		.tm_year = 0};
	time_t utc_time;
	int16_t time_zone = 0;
	uint16_t daylight_saving = 0;

	if (type != AT_CMD_TYPE_SET_COMMAND) {
		return -EINVAL;
	}

	/** parse %CCLK: <time>[,<daylight_saving_time>]
	 * <time> = "yy/MM/dd,hh:mm:ss+/-z"
	 * <daylight_saving_time> is 0, 1, 2
	 */
	ret = nrf_modem_at_scanf("AT%CCLK?", "%%CCLK: \"%2d/%2d/%2d,%2d:%2d:%2d%+2hd\",%hu",
				 &tm_time.tm_year, &tm_time.tm_mon, &tm_time.tm_mday,
				 &tm_time.tm_hour, &tm_time.tm_min, &tm_time.tm_sec,
				 &time_zone, &daylight_saving);
	if (ret <= 0) {
		return -EAGAIN;
	}

	/* calculate local datetime */
	utc_time = timeutil_timegm(&tm_time);
	if (time_zone != 99) {
		utc_time += time_zone * 15 * 60;
	}
	utc_time += daylight_saving * 60 * 60;
	(void)gmtime_r(&utc_time, &tm_time);

	rsp_send("\r\n#XCCLK: \"%02d/%02d/%02d,%02d:%02d:%02d\",%+2hd,%hu\r\n",
		 tm_time.tm_year, tm_time.tm_mon, tm_time.tm_mday,
		 tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec,
		 time_zone, daylight_saving);
	return 0;
}

static void set_uart_wk(struct k_work *work)
{
	int err;

	err = slm_uart_configure();
	if (err != 0) {
		LOG_ERR("slm_uart_configure: %d", err);
		return;
	}
	err = slm_settings_uart_save();
	if (err != 0) {
		LOG_ERR("slm_settings_uart_save: %d", err);
	}
}

/** @brief Handles AT#XSLMUART commands.
 *  AT#XSLMUART[=<baud_rate>]
 *  AT#XSLMUART?
 *  AT#XSLMUART=?
 */
static int handle_at_slmuart(enum at_cmd_type type)
{
	int ret = -EINVAL;

	if (type == AT_CMD_TYPE_SET_COMMAND) {
		uint32_t baudrate;

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

		ret = k_work_reschedule(&slm_work.uart_work, K_MSEC(SLM_UART_RESPONSE_DELAY));
		if (ret > 0) {
			ret = 0;
		}
	}
	if (type == AT_CMD_TYPE_READ_COMMAND) {
		rsp_send("\r\n#XSLMUART: %d,%d\r\n", slm_uart.baudrate, slm_uart.flow_ctrl);
		ret = 0;
	}
	if (type == AT_CMD_TYPE_TEST_COMMAND) {
		rsp_send("\r\n#XSLMUART: (1200,2400,4800,9600,14400,19200,38400,57600,"
			 "115200,230400,460800,921600,1000000)\r\n");
		ret = 0;
	}
	return ret;
}

/** @brief Handles AT#XDATACTRL commands.
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
		if (time_limit > 0 && verify_datamode_control(time_limit, NULL)) {
			datamode_time_limit = time_limit;
		} else {
			return -EINVAL;
		}
		break;

	case AT_CMD_TYPE_READ_COMMAND:
		(void)verify_datamode_control(datamode_time_limit, &time_limit_min);
		rsp_send("\r\n#XDATACTRL: %d,%d\r\n", datamode_time_limit, time_limit_min);
		break;

	case AT_CMD_TYPE_TEST_COMMAND:
		rsp_send("\r\n#XDATACTRL=<time_limit>\r\n");
		break;

	default:
		break;
	}

	return ret;
}

/** @brief Handles AT#XCLAC command.
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

#if defined(CONFIG_SLM_NRF_CLOUD)
int handle_at_nrf_cloud(enum at_cmd_type cmd_type);
#if defined(CONFIG_NRF_CLOUD_LOCATION)
int handle_at_cellpos(enum at_cmd_type cmd_type);
int handle_at_wifipos(enum at_cmd_type cmd_type);
#endif
#endif

#if defined(CONFIG_SLM_GNSS)
int handle_at_gps(enum at_cmd_type cmd_type);
int handle_at_gps_delete(enum at_cmd_type cmd_type);
#if defined(CONFIG_SLM_NRF_CLOUD) && defined(CONFIG_NRF_CLOUD_AGPS)
int handle_at_agps(enum at_cmd_type cmd_type);
#endif
#if defined(CONFIG_SLM_NRF_CLOUD) && defined(CONFIG_NRF_CLOUD_PGPS)
int handle_at_pgps(enum at_cmd_type cmd_type);
#endif
#endif

#if defined(CONFIG_SLM_FTPC)
int handle_at_ftp(enum at_cmd_type cmd_type);
#endif
#if defined(CONFIG_SLM_TFTPC)
int handle_at_tftp(enum at_cmd_type cmd_type);
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

#if defined(CONFIG_SLM_CARRIER)
int handle_at_carrier(enum at_cmd_type cmd_type);
#endif

static struct slm_at_cmd {
	char *string;
	slm_at_handler_t handler;
} slm_at_cmd_list[] = {
	/* Generic commands */
	{"AT#XSLMVER", handle_at_slmver},
	{"AT#XSLEEP", handle_at_sleep},
	{"AT#XSHUTDOWN", handle_at_shutdown},
	{"AT#XRESET", handle_at_reset},
	{"AT#XMODEMRESET", handle_at_modemreset},
	{"AT#XUUID", handle_at_uuid},
	{"AT#XCLAC", handle_at_clac},
	{"AT#XSLMUART", handle_at_slmuart},
	{"AT#XDATACTRL", handle_at_datactrl},
	{"AT#XCCLK", handle_at_cclk},

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

#if defined(CONFIG_SLM_NRF_CLOUD)
	{"AT#XNRFCLOUD", handle_at_nrf_cloud},
#if defined(CONFIG_NRF_CLOUD_LOCATION)
	{"AT#XCELLPOS", handle_at_cellpos},
	{"AT#XWIFIPOS", handle_at_wifipos},
#endif
#endif

#if defined(CONFIG_SLM_GNSS)
	/* GNSS commands */
	{"AT#XGPS", handle_at_gps},
	{"AT#XGPSDEL", handle_at_gps_delete},
#if defined(CONFIG_SLM_NRF_CLOUD) && defined(CONFIG_NRF_CLOUD_AGPS)
	{"AT#XAGPS", handle_at_agps},
#endif
#if defined(CONFIG_SLM_NRF_CLOUD) && defined(CONFIG_NRF_CLOUD_PGPS)
	{"AT#XPGPS", handle_at_pgps},
#endif
#endif

#if defined(CONFIG_SLM_FTPC)
	/* FTP commands */
	{"AT#XFTP", handle_at_ftp},
#endif
#if defined(CONFIG_SLM_TFTPC)
	/* TFTP commands */
	{"AT#XTFTP", handle_at_tftp},
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

#if defined(CONFIG_SLM_CARRIER)
	{"AT#XCARRIER", handle_at_carrier},
#endif

};

int handle_at_clac(enum at_cmd_type cmd_type)
{
	int ret = -EINVAL;

	if (cmd_type == AT_CMD_TYPE_SET_COMMAND) {
		int total = ARRAY_SIZE(slm_at_cmd_list);

		for (int i = 0; i < total; i++) {
			rsp_send("%s\r\n", slm_at_cmd_list[i].string);
		}
		ret = 0;
	}

	return ret;
}

int slm_at_parse(const char *at_cmd)
{
	int ret = UNKNOWN_AT_COMMAND_RET;
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
#if defined(CONFIG_SLM_NRF_CLOUD)
	err = slm_at_nrfcloud_init();
	if (err) {
		LOG_ERR("nRF Cloud could not be initialized: %d", err);
		return -EFAULT;
	}
#endif
#if defined(CONFIG_SLM_GNSS)
	err = slm_at_gnss_init();
	if (err) {
		LOG_ERR("GNSS could not be initialized: %d", err);
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
#if defined(CONFIG_SLM_CARRIER)
	err = slm_at_carrier_init();
	if (err) {
		LOG_ERR("LwM2M carrier could not be initialized: %d", err);
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
#if defined(CONFIG_SLM_NRF_CLOUD)
	err = slm_at_nrfcloud_uninit();
	if (err) {
		LOG_WRN("nRF Cloud could not be uninitialized: %d", err);
	}
#endif
#if defined(CONFIG_SLM_GNSS)
	err = slm_at_gnss_uninit();
	if (err) {
		LOG_WRN("GNSS could not be uninitialized: %d", err);
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
#if defined(CONFIG_SLM_CARRIER)
	err = slm_at_carrier_uninit();
	if (err) {
		LOG_ERR("LwM2M carrier could not be uninitialized: %d", err);
	}
#endif
}
