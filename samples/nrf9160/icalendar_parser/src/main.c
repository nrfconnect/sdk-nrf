#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr.h>
#include <dk_buttons_and_leds.h>
#include <lte_lc.h>
#include <modem_key_mgmt.h>
#include <net/socket.h>
#include <net/http_client.h>
#include <net/tls_credentials.h>
#include <net/icalendar_parser.h>
#include <modem_info.h>

#define UI_BUTTON_1                     1
#define UI_BUTTON_2                     2
#define UI_SWITCH_1                     4
#define UI_SWITCH_2                     8

/* Interval to blink calendar state LED */
#define ICAL_STATE_LED_ON_PERIOD                500
#define ICAL_STATE_LED_OFF_PERIOD               500
/* Interval to check current date-time information */
#define ICAL_DATETIME_PERIOD                    1000
/* Interval to check current calendar event and set LED */
#define ICAL_EVENT_LED_PERIOD                   1000

/** HTTP socket. */
static int fd = -1;

/* Receiving buffer for HTTP client */
#define MAX_RECV_BUF_LEN 1024
static u8_t http_recv_buf[MAX_RECV_BUF_LEN];

static struct icalendar_parser ical;
static struct k_delayed_work ical_work;
static struct k_delayed_work datetime_update_work;

/* Current date-time information */
static u32_t current_date;
static u32_t current_time;
static u8_t timezone;

/** List of pointers to the allocated memory for parsed calendar components */
static struct ical_component *ical_coms[CONFIG_MAX_PARSED_COMPONENTS];
/** Total number of parsed calendar components */
static size_t component_cnt;

/* Certificate for calendar host */
static const char cert[] = {
	#include "../cert/GlobalSign-Root-CA-R2"
};

/**@brief Read current time from modem */
static void datetime_work_handler(struct k_work *unused)
{
	int err;
	char recv_buf[CONFIG_MODEM_INFO_BUFFER_SIZE] = { 0 };

	err = modem_info_string_get(MODEM_INFO_DATE_TIME, recv_buf);
	if (err <= 0) {
		printk("read modem info error\n");
	} else {
		int day = 0, month = 0, year = 0, hour = 0, min = 0, sec = 0;

		/* The format of date time modem info:
		 * yy/MM/dd,hh:mm:ss±zz
		 */
		year = atoi(recv_buf);
		month = atoi(recv_buf + strlen("yy/"));
		day = atoi(recv_buf + strlen("yy/MM/"));
		hour = atoi(recv_buf + strlen("yy/MM/dd,"));
		min = atoi(recv_buf + strlen("yy/MM/dd,hh:"));
		sec = atoi(recv_buf + strlen("yy/MM/dd,hh:mm:"));
		timezone = atoi(recv_buf + strlen("yy/MM/dd,hh:mm:ss"));
		current_date = 10000 * (2000 + year) + 100 * month + day;
		current_time = 10000 * hour + 100 * min + sec;
	}
	k_delayed_work_submit(&datetime_update_work, ICAL_DATETIME_PERIOD);
}

/**@brief Callback for button events from the DK buttons and LEDs library. */
static void button_handler(u32_t button_state, u32_t has_changed)
{
	u32_t button = button_state & has_changed;

	if (button & UI_BUTTON_1) {
		printk("Button presssed. Start download calendar.\n");
		k_delayed_work_submit(&ical_work, K_NO_WAIT);
	}
}

static int socket_sectag_set(int fd, int sec_tag)
{
	int err;
	int verify;
	sec_tag_t sec_tag_list[] = { sec_tag };
	nrf_sec_session_cache_t sec_session_cache;

	enum {
		NONE            = 0,
		OPTIONAL        = 1,
		REQUIRED        = 2,
	};

	verify = REQUIRED;

	err = setsockopt(fd, SOL_TLS, TLS_PEER_VERIFY, &verify, sizeof(verify));
	if (err) {
		printk("Failed to setup peer verification, errno %d\n", errno);
		return -1;
	}

	err = setsockopt(fd, SOL_TLS, TLS_SEC_TAG_LIST, sec_tag_list,
			 sizeof(sec_tag_t) * ARRAY_SIZE(sec_tag_list));
	if (err) {
		printk("Failed to set socket security tag, errno %d\n", errno);
		return -1;
	}

	err = setsockopt(fd, SOL_TLS, TLS_PEER_VERIFY, &verify, sizeof(verify));
	if (err) {
		printk("Failed to setup peer verification, errno %d\n", errno);
		return -1;
	}

	/* Disable TLE session cache */
	sec_session_cache = 0;
	err = nrf_setsockopt(fd, SOL_TLS, NRF_SO_SEC_SESSION_CACHE,
			     &sec_session_cache, sizeof(sec_session_cache));
	if (err) {
		printk("Failed to disable TLS cache, errno %d\n", errno);
		return -1;
	}

	return 0;
}

static int resolve_and_connect(int family, const char *host, int sec_tag)
{
	int fd;
	int err;
	int proto;
	u16_t port;
	struct addrinfo *addr;
	struct addrinfo *info;

	__ASSERT_NO_MSG(host);

	/* Set up port and protocol */
	if (sec_tag == -1) {
		/* HTTP, port 80 */
		proto = IPPROTO_TCP;
		port = htons(80);
	} else {
		/* HTTPS, port 443 */
		proto = IPPROTO_TLS_1_2;
		port = htons(443);
	}

	/* Lookup host */
	struct addrinfo hints = {
		.ai_family = family,
		.ai_socktype = SOCK_STREAM,
		.ai_protocol = proto,
	};

	err = getaddrinfo(host, NULL, &hints, &info);
	if (err) {
		printk("Failed to resolve hostname %s on %s\n",
			log_strdup(host),
			family == AF_INET ? "IPv4" : "IPv6");
		return -1;
	}

	printk("Attempting to connect over %s\n",
		family == AF_INET ? log_strdup("IPv4") : log_strdup("IPv6"));

	fd = socket(family, SOCK_STREAM, proto);
	if (fd < 0) {
		printk("Failed to create socket, errno %d\n", errno);
		goto cleanup;
	}

	if (proto == IPPROTO_TLS_1_2) {
		printk("Setting up TLS credentials\n");
		err = socket_sectag_set(fd, sec_tag);
		if (err) {
			goto cleanup;
		}
	}

	/* Not connected */
	err = -1;

	for (addr = info; addr != NULL; addr = addr->ai_next) {
		struct sockaddr *const sa = addr->ai_addr;

		switch (sa->sa_family) {
		case AF_INET6:
			((struct sockaddr_in6 *)sa)->sin6_port = port;
			break;
		case AF_INET:
			((struct sockaddr_in *)sa)->sin_port = port;
			break;
		}

		err = connect(fd, sa, addr->ai_addrlen);
		if (err) {
			/* Try next address */
			printk("Unable to connect, errno %d\n", errno);
		} else {
			/* Connected */
			break;
		}
	}

cleanup:
	freeaddrinfo(info);

	if (err) {
		/* Unable to connect, close socket */
		close(fd);
		fd = -1;
	}

	return fd;
}

static int socket_timeout_set(int fd)
{
	int err;

	const u32_t timeout_ms = K_FOREVER;

	struct timeval timeo = {
		.tv_sec = (timeout_ms / 1000),
		.tv_usec = (timeout_ms % 1000) * 1000,
	};

	printk("Configuring socket timeout (%ld s)", timeo.tv_sec);

	err = setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeo, sizeof(timeo));
	if (err) {
		printk("Failed to set socket timeout, errno %d\n", errno);
		return -errno;
	}

	return 0;
}

int server_connect(const char *host, int sec_tag)
{
	int err;

	if (host == NULL) {
		return -EINVAL;
	}

	if (!IS_ENABLED(CONFIG_ICAL_PARSER_TLS)) {
		printk("HTTP Connection\n");
		if (sec_tag != -1) {
			return -EINVAL;
		}
	} else {
		printk("HTTPS Connection\n");
		if (sec_tag == -1) {
			return -EINVAL;
		}
	}

	if (fd != -1) {
		/* Already connected */
		return 0;
	}

	/* Attempt IPv6 connection if configured, fallback to IPv4 */
	if (IS_ENABLED(CONFIG_ICAL_PARSER_IPV6)) {
		fd = resolve_and_connect(AF_INET6, host, sec_tag);
	}
	if (fd < 0) {
		fd = resolve_and_connect(AF_INET, host, sec_tag);
	}

	if (fd < 0) {
		printk("Fail to resolve and connect\n");
		return -EINVAL;
	}

	printk("Connected to %s\n", log_strdup(host));

	/* Set socket timeout, if configured */
	err = socket_timeout_set(fd);
	if (err) {
		return err;
	}

	return 0;
}

int server_disconnect(void)
{
	int err;

	if (fd < 0) {
		return -EINVAL;
	}

	err = close(fd);
	if (err) {
		printk("Failed to close socket, errno %d\n", errno);
		return -errno;
	}

	fd = -1;

	return 0;
}

static void response_cb(struct http_response *rsp,
			enum http_final_call final_data,
			void *user_data)
{
	int err;
	size_t ret = 0;
	static size_t data_received;

	data_received += rsp->data_len;
	if (final_data == HTTP_DATA_MORE) {
		printk("Partial data received (%zd bytes)\n", rsp->data_len);
		if (data_received == MAX_RECV_BUF_LEN) {
			ret = ical_parser_parse(&ical, rsp->recv_buf,
						MAX_RECV_BUF_LEN);
			printk("%d bytes parsed\n", ret);
			data_received = 0;
		}
	} else if (final_data == HTTP_DATA_FINAL) {
		printk("All the data received (%zd bytes)\n", rsp->data_len);
		ret = ical_parser_parse(&ical, rsp->recv_buf,
					data_received + rsp->data_len);
		data_received = 0;
		printk("Total %d pended event\n", component_cnt);
		err = server_disconnect();
		if (err != 0) {
			printk("server_disconnect fail. err:%d\n", err);
		}
		dk_set_led_on(DK_LED3);
		k_delayed_work_submit(&ical_work,
				      K_SECONDS(CONFIG_DOWNLOAD_INTERVAL));
	}
}

int download_start(void)
{
	int ret;
	struct http_request req;

	if (fd == -1) {
		/* Already connected */
		return -EINVAL;
	}

	memset(&req, 0, sizeof(req));

	req.method = HTTP_GET;
	req.url = CONFIG_DOWNLOAD_FILE;
	req.host = CONFIG_DOWNLOAD_HOST;
	req.protocol = "HTTP/1.1";
	req.response = response_cb;
	req.recv_buf = http_recv_buf;
	req.recv_buf_len = sizeof(http_recv_buf);

	ret = http_client_req(fd, &req, K_SECONDS(3), "");
	if (ret) {
		return 0;
	} else {
		return -EINVAL;
	}
}

/**@brief Initializes buttons and LEDs, using the DK buttons and LEDs
 * library.
 */
static void buttons_leds_init(void)
{
	int err;

	err = dk_buttons_init(button_handler);
	if (err) {
		printk("Could not initialize buttons, err code: %d\n", err);
	}

	err = dk_leds_init();
	if (err) {
		printk("Could not initialize leds, err code: %d\n", err);
	}

	err = dk_set_leds_state(0x00, DK_ALL_LEDS_MSK);
	if (err) {
		printk("Could not set leds state, err code: %d\n", err);
	}
}

/**@brief Configures modem to provide LTE link.
 *
 * Blocks until link is successfully established.
 */
static void modem_configure(void)
{
#if defined(CONFIG_LTE_LINK_CONTROL)
	if (IS_ENABLED(CONFIG_LTE_AUTO_INIT_AND_CONNECT)) {
		/* Do nothing, modem is already turned on
		 * and connected.
		 */
	} else {
		int err;

		printk("LTE Link Connecting ...\n");
		err = lte_lc_init_and_connect();
		__ASSERT(err == 0, "LTE link could not be established.");
		printk("LTE Link Connected!\n");
	}
#endif
}

static int icalendar_parser_callback(const struct ical_parser_evt *event,
				     struct ical_component *ical_com)
{
	int err = 0;

	if (event == NULL) {
		return -EINVAL;
	}

	switch (event->id) {

	case ICAL_PARSER_EVT_COMPONENT:
		if (ical_com != NULL) {
			if (component_cnt >= CONFIG_MAX_PARSED_COMPONENTS) {
				printk("Fail to store new calendar component. Increase CONFIG_MAX_PARSED_COMPONENTS!\n");
				break;
			}
			/* Compare current date-time and skip out-of-date event.
			 * The format of date time info from iCalendar:
			 * YYYYMMDDTHHMMSS[Z]
			 */
			u32_t dtend_date;
			u32_t dtend_time;

			dtend_date = atoi(ical_com->dtend);
			dtend_time = atoi(ical_com->dtend
					+ strlen("YYYYMMDDT"));
			if ((dtend_date > current_date) ||
			    ((dtend_date == current_date)
			     && (dtend_time > current_time))) {
				/* Allocate memory for new calendar component */
				struct ical_component *new_com;

				new_com = k_malloc(sizeof(*new_com));
				if (new_com) {
					memset(new_com,
					       0,
					       sizeof(struct ical_component));
					memcpy(new_com,
					       ical_com,
					       sizeof(struct ical_component));
					ical_coms[component_cnt] = new_com;
				} else {
					printk("Fail to allocate memory for parsed calendar component.\n");
					break;
				}
				component_cnt++;
			} else {
				printk("Skipped calendar event: out of date\n");
			}
		}
		break;

	default:
		break;
	}

	return err;
}


/**@brief Start downloading and parsing the iCalendar from calendar host */
static void ical_work_handler(struct k_work *unused)
{
	int err, sec_tag;

	/* Verify the host and file. */
	if (CONFIG_DOWNLOAD_HOST == NULL || CONFIG_DOWNLOAD_FILE == NULL) {
		printk("Wrong download host or file\n");
		return;
	}

	if (!IS_ENABLED(CONFIG_ICAL_PARSER_TLS)) {
		sec_tag = -1;
	} else {
		sec_tag = CONFIG_ICAL_SEC_TAG;
	}

	dk_set_led_off(DK_LED2);

	/* Connect to server if it is not connected yet. */
	if (fd == -1) {
		err = server_connect(CONFIG_DOWNLOAD_HOST, sec_tag);
		if (err != 0) {
			printk("ical_parser_connect fail. Reconnect after 3 seconds\n");
			k_delayed_work_submit(&ical_work, K_SECONDS(3));
			return;
		}
	} else {
		printk("Already connected.\n");
		return;
	}

	/* Initialize iCalendar parser */
	err = ical_parser_init(&ical, icalendar_parser_callback);
	if (err != 0) {
		printk("ical_parser_init error, err %d\n", err);
		return;
	}

	/* Free the buffer allocated for old parsed calendar events */
	for (int i = 0; i < component_cnt; i++) {
		k_free(ical_coms[i]);
	}
	component_cnt = 0;

	/* Start download */
	err = download_start();
	if (err != 0) {
		printk("http download fail. Restart after 3 seconds\n");
		k_delayed_work_submit(&ical_work, K_SECONDS(3));
	}
}

/* Provision certificate to modem */
int cert_provision(void)
{
	int err;
	bool exists;
	u8_t unused;

	err = modem_key_mgmt_exists(CONFIG_ICAL_SEC_TAG,
				    MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
				    &exists, &unused);
	if (err) {
		printk("Failed to check for certificates err %d\n", err);
		return err;
	}

	if (exists) {
		/* For the sake of simplicity we delete what is provisioned
		 * with our security tag and reprovision our certificate.
		 */
		err = modem_key_mgmt_delete(CONFIG_ICAL_SEC_TAG,
					    MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN);
		if (err) {
			printk("Failed to delete existing certificate, err %d\n",
			       err);
		}
	}

	printk("Provisioning certificate\n");

	/*  Provision certificate to the modem */
	err = modem_key_mgmt_write(CONFIG_ICAL_SEC_TAG,
				   MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
				   cert, sizeof(cert) - 1);
	if (err) {
		printk("Failed to provision certificate, err %d\n", err);
		return err;
	}

	return 0;
}

/**@brief Look up calendar components. Return true if there is ongoint event. */
static bool calendar_lookup(u32_t given_date, u32_t given_time)
{
	struct ical_component *ical_com;
	u32_t dtstart_date;
	u32_t dtstart_time;
	u32_t dtend_date;
	u32_t dtend_time;

	/* Look up all calendar components */
	for (int i = 0; i < component_cnt; i++) {
		ical_com = ical_coms[i];
		if (ical_com) {
			dtstart_date = atoi(ical_com->dtstart);
			dtstart_time = atoi(ical_com->dtstart
					    + strlen("YYYYMMDDT"));
			dtend_date = atoi(ical_com->dtend);
			dtend_time = atoi(ical_com->dtend
					  + strlen("YYYYMMDDT"));
			/* Check date-time start of component */
			if ((dtstart_date < given_date) ||
			    ((dtstart_date == given_date)
			     && (dtstart_time < given_time))) {
				/* Check date-time end of component */
				if ((dtend_date > given_date) ||
				    ((dtend_date == given_date)
				     && (dtend_time > given_time))) {
					return true;
				}
			}
		}
	}

	return false;
}

/**@brief Update LEDs state. */
static void leds_update(void)
{
	static bool led_on;

	led_on = !led_on;

	if (fd != -1) {
		/* Blink LED3 while downloading new calendar */
		if (led_on) {
			dk_set_led_off(DK_LED3);
			k_sleep(ICAL_STATE_LED_OFF_PERIOD);
		} else {
			dk_set_led_on(DK_LED3);
			k_sleep(ICAL_STATE_LED_OFF_PERIOD);
		}
	} else {
		/* Update LED1/LED2 according to parsed calendar components. */
		if (component_cnt > 0) {
			/* Lit LED2 if some event hasn't heppen yet. */
			dk_set_led_on(DK_LED2);
			dk_set_led_off(DK_LED3);
			/* Lit LED1 if some event is happening. */
			if (calendar_lookup(current_date, current_time)) {
				dk_set_led_off(DK_LED2);
				dk_set_led_on(DK_LED1);
			} else {
				dk_set_led_off(DK_LED1);
			}
		} else {
			dk_set_led_off(DK_LED1);
			dk_set_led_off(DK_LED2);
		}
		k_sleep(ICAL_EVENT_LED_PERIOD);
	}
}

void main(void)
{
	int err;

	printk("iCalendar parser example started\n");
	buttons_leds_init();

	/* Provision certificate to the modem. */
	err = cert_provision();
	if (err) {
		return;
	}
	modem_configure();

	err = modem_info_init();
	if (err) {
		printk("Modem info could not be established: %d\n", err);
		return;
	}

	k_delayed_work_init(&datetime_update_work, datetime_work_handler);
	k_delayed_work_submit(&datetime_update_work, ICAL_DATETIME_PERIOD);
	k_delayed_work_init(&ical_work, ical_work_handler);
	k_delayed_work_submit(&ical_work, 1000);

	while (1) {
		leds_update();
	}
}
