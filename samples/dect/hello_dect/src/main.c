/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/conn_mgr_connectivity.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/dns_resolve.h>
#include <zephyr/net/hostname.h>
#include <zephyr/posix/netdb.h>
#include <zephyr/posix/unistd.h>
#include <zephyr/posix/arpa/inet.h>
#include <zephyr/posix/sys/socket.h>

#if defined(CONFIG_DK_LIBRARY)
#include <dk_buttons_and_leds.h>
#endif

#include <modem/nrf_modem_lib.h>
#include <nrf_modem.h>

#include <net/dect/dect_net_l2_mgmt.h>
#include <net/dect/dect_net_l2.h>

LOG_MODULE_REGISTER(hello_dect, CONFIG_HELLO_DECT_MAC_LOG_LEVEL);

/* Modem fault handler */
void nrf_modem_fault_handler(struct nrf_modem_fault_info *fault_info)
{
	LOG_ERR("Modem fault: reason=%d, program_counter=0x%x",
		fault_info->reason, fault_info->program_counter);

	__ASSERT(false, "Modem crash detected, halting application");
}

/* Network interface */
static struct net_if *dect_iface;

/* Network management callback */
static struct net_mgmt_event_callback net_conn_mgr_cb;
static struct net_mgmt_event_callback net_if_cb;

/* Application state */
static bool dect_connected;
static uint32_t message_counter;
static struct sockaddr_in6 peer_addr;
static bool peer_resolved;
static atomic_t recv_socket_atomic = ATOMIC_INIT(-1);

/* Socket receive timeout in seconds */
#define SOCKET_RECV_TIMEOUT_SEC 5

/* Device configuration based on Kconfig */
#if defined(CONFIG_DECT_DEFAULT_DEV_TYPE_FT)
#define DEVICE_TYPE_STR "FT"
static char *local_hostname = "dect-nr+-ft-device";
static char *peer_hostname = "dect-nr+-pt-device.local";
#elif defined(CONFIG_DECT_DEFAULT_DEV_TYPE_PT)
#define DEVICE_TYPE_STR "PT"
static char *local_hostname = "dect-nr+-pt-device";
static char *peer_hostname = "dect-nr+-ft-device.local";
#else
#error "Either CONFIG_DECT_DEFAULT_DEV_TYPE_FT or _PT must be defined."
#endif

/* Forward declarations */
static void hello_dect_max_tx_work_handler(struct k_work *work);
static void hello_dect_mac_tx_demo_message(void);
static void hello_dect_mac_resolve_peer_address(void);
static void hello_dect_mac_set_hostname(void);
static void hello_dect_mac_rx_thread(void);

/* Demo work definition */
static K_WORK_DELAYABLE_DEFINE(tx_work, hello_dect_max_tx_work_handler);

/* LED 2 turn-off work definition */
#if defined(CONFIG_DK_LIBRARY)
static void hello_dect_led2_off_work_handler(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(led2_off_work, hello_dect_led2_off_work_handler);
#endif

static void hello_dect_max_tx_work_handler(struct k_work *work)
{
	if (dect_connected) {
		/* Try to resolve peer if not already resolved */
		if (!peer_resolved) {
			hello_dect_mac_resolve_peer_address();
		}
		hello_dect_mac_tx_demo_message();

		/* Reschedule work */
		k_work_schedule(&tx_work,
				K_SECONDS(CONFIG_HELLO_DECT_MAC_DEMO_INTERVAL));
	}
}

#if defined(CONFIG_DK_LIBRARY)
static void hello_dect_led2_off_work_handler(struct k_work *work)
{
	/* Turn off LED 2 after 1 second delay */
	dk_set_led_off(DK_LED2);
}
#endif

static void hello_dect_mac_resolve_peer_address(void)
{
	int ret;
	struct addrinfo hints = {0};
	struct addrinfo *result;
	char addr_str[NET_IPV6_ADDR_LEN];

	hints.ai_family = AF_INET6;
	hints.ai_flags = AI_ADDRCONFIG;

	LOG_INF("Resolving peer hostname: %s", peer_hostname);

	ret = getaddrinfo(peer_hostname, NULL, &hints, &result);
	if (ret == 0 && result != NULL) {
		memcpy(&peer_addr, result->ai_addr, sizeof(peer_addr));
		peer_addr.sin6_port = htons(12345);
		peer_resolved = true;

		net_addr_ntop(AF_INET6, &peer_addr.sin6_addr, addr_str, sizeof(addr_str));
		LOG_INF("Peer resolved to: %s", addr_str);

		freeaddrinfo(result);
	} else {
		LOG_WRN("Failed to resolve peer hostname (%s): %d", peer_hostname, ret);
		peer_resolved = false;
	}
}

static void hello_dect_mac_tx_demo_message(void)
{
	int sock, ret;
	char message[512];
	char addr_str[NET_IPV6_ADDR_LEN];

	snprintf(message, sizeof(message),
		"Hello DECT NR+ from %s (name: %s) device! Message #%u",
		DEVICE_TYPE_STR, local_hostname, ++message_counter);

	sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) {
		LOG_ERR("Failed to create socket: %d", errno);
		return;
	}

	if (peer_resolved) {
		/* Send to discovered peer */
		net_addr_ntop(AF_INET6, &peer_addr.sin6_addr, addr_str, sizeof(addr_str));
		LOG_INF("Sending to peer: %s", addr_str);

		ret = sendto(sock, message, strlen(message), 0,
			     (struct sockaddr *)&peer_addr, sizeof(peer_addr));
	} else {
		/* Fallback to multicast */
		struct sockaddr_in6 mcast_addr = {0};

		mcast_addr.sin6_family = AF_INET6;
		mcast_addr.sin6_port = htons(12345);

		/* Well known IPv6 link local scope all nodes multicast address (FF02::1) */
		net_ipv6_addr_create_ll_allnodes_mcast((struct in6_addr *)&mcast_addr.sin6_addr);

		LOG_INF("Sending to multicast (peer not resolved)");

		ret = sendto(sock, message, strlen(message), 0,
			     (struct sockaddr *)&mcast_addr, sizeof(mcast_addr));
	}

	if (ret < 0) {
		LOG_ERR("Failed to send message: %d", errno);
	} else {
		LOG_INF("Sent: %s", message);

#if defined(CONFIG_DK_LIBRARY)
		/* Cancel any pending LED 2 turn-off work */
		k_work_cancel_delayable(&led2_off_work);
		/* Turn on LED 2 to indicate successful transmission */
		dk_set_led_on(DK_LED2);
		/* Schedule LED 2 to turn off after 1 second */
		k_work_schedule(&led2_off_work, K_SECONDS(1));
#endif
	}

	close(sock);
}

static void hello_dect_mac_print_network_info(struct net_if *iface)
{
	LOG_INF("=== Network Interface Information ===");
	LOG_INF("Interface: %s", net_if_get_device(iface)->name);
	LOG_INF("Device type: %s", DEVICE_TYPE_STR);
}

static int hello_dect_mac_start_udp_listener(void)
{
	struct sockaddr_in6 addr = {0};
	struct timeval timeout = {
		.tv_sec = SOCKET_RECV_TIMEOUT_SEC,
		.tv_usec = 0
	};
	int ret;
	int sock;

	if (atomic_get(&recv_socket_atomic) >= 0) {
		return 0;  /* Already listening */
	}

	sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) {
		LOG_ERR("Failed to create receive socket: %d", errno);
		return -errno;
	}

	/* Set receive timeout to avoid blocking forever */
	ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
	if (ret < 0) {
		LOG_WRN("Failed to set socket receive timeout: %d", errno);
		/* Continue anyway - socket will block indefinitely */
	}

	addr.sin6_family = AF_INET6;
	addr.sin6_port = htons(12345);
	addr.sin6_addr = in6addr_any;

	ret = bind(sock, (struct sockaddr *)&addr, sizeof(addr));
	if (ret < 0) {
		LOG_ERR("Failed to bind receive socket: %d", errno);
		close(sock);
		return -errno;
	}

	atomic_set(&recv_socket_atomic, sock);
	LOG_INF("UDP listener started on port 12345 (timeout: %ds)", SOCKET_RECV_TIMEOUT_SEC);
	return 0;
}

static void hello_dect_mac_stop_udp_listener(void)
{
	int sock = atomic_get(&recv_socket_atomic);

	if (sock >= 0) {
		atomic_set(&recv_socket_atomic, -1);
		close(sock);
		LOG_INF("UDP listener stopped");
	}
}

static void hello_dect_mac_rx_thread(void)
{
	char buffer[256];
	struct sockaddr_in6 src_addr;
	socklen_t addr_len;
	int ret;
	int sock;
	char addr_str[NET_IPV6_ADDR_LEN];

	while (1) {
		sock = atomic_get(&recv_socket_atomic);
		if (sock < 0) {
			k_sleep(K_SECONDS(1));
			continue;
		}

		addr_len = sizeof(src_addr);
		ret = recvfrom(sock, buffer, sizeof(buffer) - 1, 0,
			       (struct sockaddr *)&src_addr, &addr_len);

		if (ret < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				/* Timeout - check if socket is still valid and continue */
				continue;
			}
			/* Check if socket was closed by another thread */
			if (atomic_get(&recv_socket_atomic) < 0) {
				LOG_DBG("Socket closed, waiting for reconnect");
				continue;
			}
			LOG_ERR("recvfrom failed: %d", errno);
			k_sleep(K_SECONDS(1));
			continue;
		}

		if (ret > 0) {
			buffer[ret] = '\0';
			net_addr_ntop(AF_INET6, &src_addr.sin6_addr,
				      addr_str, sizeof(addr_str));
			LOG_INF("Received %d bytes from %s: %s",
				ret, addr_str, buffer);
		}
	}
}

static void net_conn_mgr_event_handler(struct net_mgmt_event_callback *cb,
				       uint64_t mgmt_event, struct net_if *iface)
{
	/* Only handle events for our DECT interface */
	if (iface != dect_iface) {
		return;
	}
	if (mgmt_event == NET_EVENT_L4_CONNECTED) {
		LOG_INF("NET_EVENT_L4_CONNECTED");
	} else if (mgmt_event == NET_EVENT_L4_DISCONNECTED) {
		LOG_INF("NET_EVENT_L4_DISCONNECTED");
	}
}

static void net_if_event_handler(struct net_mgmt_event_callback *cb,
				 uint64_t mgmt_event, struct net_if *iface)
{
	/* Only handle events for our DECT interface */
	if (iface != dect_iface) {
		return;
	}
	if (mgmt_event == NET_EVENT_IF_UP) {
		LOG_INF("DECT NR+ interface is UP with local IPv6 addressing");
		dect_connected = true;
		hello_dect_mac_print_network_info(iface);

		/* Start UDP listener */
		hello_dect_mac_start_udp_listener();

		/* Start demo work and schedule peer resolution */
		k_work_schedule(&tx_work, K_SECONDS(5));  /* First run after 5 seconds */

#if defined(CONFIG_DK_LIBRARY)
		/* Turn on LED 1 to indicate connection */
		dk_set_led_on(DK_LED1);
#endif

	} else if (mgmt_event == NET_EVENT_IF_DOWN) {
		LOG_INF("DECT NR+ interface is DOWN");
		dect_connected = false;
		peer_resolved = false;
		/* Reset message counter for new session */
		message_counter = 0;
		hello_dect_mac_stop_udp_listener();
		k_work_cancel_delayable(&tx_work);

#if defined(CONFIG_DK_LIBRARY)
		/* Turn off LED 1 to indicate disconnection */
		dk_set_led_off(DK_LED1);
#endif
	}
}

#if defined(CONFIG_DK_LIBRARY)
static void button_handler(uint32_t button_states, uint32_t has_changed)
{
	int ret;

	if (has_changed & button_states & DK_BTN1_MSK) {
		LOG_INF("Button 1 pressed - initiating connection");
		ret = conn_mgr_if_connect(dect_iface);
		if (ret < 0) {
			LOG_ERR("Failed to initiate connection: %d", ret);
		} else {
			LOG_INF("Connection initiated");
		}
	}

	if (has_changed & button_states & DK_BTN2_MSK) {
		LOG_INF("Button 2 pressed - disconnecting");
		ret = conn_mgr_if_disconnect(dect_iface);
		if (ret < 0) {
			LOG_ERR("Failed to disconnect: %d", ret);
		} else {
			LOG_INF("Disconnect initiated");
		}
	}
}
#endif

static void hello_dect_mac_set_hostname(void)
{
	if (net_hostname_set(local_hostname, strlen(local_hostname))) {
		LOG_ERR("Failed to set hostname %s", local_hostname);
	} else {
		LOG_INF("Hostname set to: %s", local_hostname);
	}
}

int main(void)
{
	int err;

	LOG_INF("=== Hello DECT NR+ Sample Application ===");
	LOG_INF("Device type: %s", DEVICE_TYPE_STR);

	/* Set hostname based on device type */
	hello_dect_mac_set_hostname();

	/* Setup network management callbacks for L4 connected/disconnected events */
	net_mgmt_init_event_callback(&net_conn_mgr_cb, net_conn_mgr_event_handler,
				     NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED);
	net_mgmt_add_event_callback(&net_conn_mgr_cb);

	net_mgmt_init_event_callback(&net_if_cb,
				     net_if_event_handler,
				     (NET_EVENT_IF_UP |
				      NET_EVENT_IF_DOWN));
	net_mgmt_add_event_callback(&net_if_cb);

	/* Get the DECT network interface */
	dect_iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DECT));
	if (!dect_iface) {
		LOG_ERR("No DECT interface found");
		return -ENODEV;
	}

	/* Initialize modem library and this triggers DECT NR+ stack initialization */
#if defined(CONFIG_NRF_MODEM_LIB)
	err = nrf_modem_lib_init();
	if (err) {
		LOG_ERR("Failed to initialize modem library: %d", err);
		return err;
	}
#endif

#if defined(CONFIG_DK_LIBRARY)
	/* Initialize DK library for buttons and LEDs */
	err = dk_buttons_init(button_handler);
	if (err) {
		LOG_WRN("Failed to initialize buttons: %d", err);
	}

	err = dk_leds_init();
	if (err) {
		LOG_WRN("Failed to initialize LEDs: %d", err);
	} else {
		/* Initialize LEDs to OFF state */
		dk_set_led_off(DK_LED1);
		dk_set_led_off(DK_LED2);
	}
#endif

#if !defined(CONFIG_NET_L2_DECT_CONN_MGR_AUTO_CONNECT)
	/* Initiate connection using connection manager */
	LOG_INF("Initiating DECT connection...");
	err = conn_mgr_if_connect(dect_iface);
	if (err) {
		LOG_ERR("Failed to initiate connection: %d", err);
		return err;
	}
#endif
	LOG_INF("Hello DECT application started successfully");
#if defined(CONFIG_DK_LIBRARY)
	LOG_INF("Press button 1 to connect, button 2 to disconnect");
#endif

	/* Main application loop - run UDP receive in main thread */
	hello_dect_mac_rx_thread();

	return 0;
}
