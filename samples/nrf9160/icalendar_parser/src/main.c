#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr.h>
#include <dk_buttons_and_leds.h>
#include <lte_lc.h>
#include <modem_key_mgmt.h>
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
				     struct ical_component *p_com)
{
	int err = 0;

	if (event == NULL) {
		return -EINVAL;
	}

	switch (event->id) {

	case ICAL_PARSER_EVT_CALENDAR:
		printk("Got calendar event\n");
		/* Free the buffer allocated before */
		for (int i = 0; i < component_cnt; i++) {
			k_free(ical_coms[i]);
		}
		component_cnt = 0;
		break;

	case ICAL_PARSER_EVT_COMPONENT:
		if (p_com != NULL) {
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

			dtend_date = atoi(p_com->dtend);
			dtend_time = atoi(p_com->dtend + strlen("YYYYMMDDT"));
			if ((dtend_date > current_date) ||
			    ((dtend_date == current_date)
			     && (dtend_time > current_time))) {
				/* Allocate memory for new calendar component */
				struct ical_component *p_new_com;

				p_new_com = k_malloc(sizeof(*p_new_com));
				if (p_new_com) {
					memset(p_new_com,
					       0,
					       sizeof(struct ical_component));
					memcpy(p_new_com,
					       p_com,
					       sizeof(struct ical_component));
					ical_coms[component_cnt] = p_new_com;
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

	case ICAL_PARSER_EVT_COMPLETE:
		printk("ICAL_PARSER_EVT_COMPLETE: %d event\n", component_cnt);
		dk_set_led_on(DK_LED3);
		ical_parser_disconnect(&ical);
		k_delayed_work_submit(&ical_work,
				      K_SECONDS(CONFIG_DOWNLOAD_INTERVAL));
		break;

	case ICAL_PARSER_EVT_ERROR: {
		component_cnt = 0;
		ical_parser_disconnect(&ical);
		printk("ICAL_PARSER_EVT_ERROR err number: %d.\n", event->error);
		err = event->error;
		printk("Reconnect after 3 seconds\n");
		k_delayed_work_submit(&ical_work, K_SECONDS(3));
		break;
	}
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

	/* Verify that a download is not already ongoing */
	if (ical.state == ICAL_PARSER_STATE_DOWNLOADING) {
		printk("There is ongoing calendar parsing task\n");
		return;
	}

	if (!IS_ENABLED(CONFIG_ICAL_PARSER_TLS)) {
		sec_tag = -1;
	} else {
		sec_tag = CONFIG_ICAL_SEC_TAG;
	}

	dk_set_led_off(DK_LED2);

	/* Connect to server if it is not connected yet. */
	if (ical.fd == -1) {
		err = ical_parser_connect(&ical, CONFIG_DOWNLOAD_HOST, sec_tag);
		if (err != 0) {
			printk("ical_parser_connect fail. Reconnect after 3 seconds\n");
			k_delayed_work_submit(&ical_work, K_SECONDS(3));
			return;
		}
	}

	/* Start download and parse thread. */
	err = ical_parser_start(&ical, CONFIG_DOWNLOAD_FILE);
	if (err != 0) {
		printk("ical_parser_start fail. Reconnect after 3 seconds\n");
		ical_parser_disconnect(&ical);
		k_delayed_work_submit(&ical_work, K_SECONDS(3));
	}
}

/**@brief Initialize iCalendar parser and downloading thread */
static int calendar_init(void)
{
	int err;

	err = ical_parser_init(&ical, icalendar_parser_callback);
	if (err != 0) {
		printk("ical_parser_init error, err %d\n", err);
		return err;
	}

	k_delayed_work_init(&ical_work, ical_work_handler);
	k_delayed_work_submit(&ical_work, 1000);

	return 0;
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
	struct ical_component *p_com;
	u32_t dtstart_date;
	u32_t dtstart_time;
	u32_t dtend_date;
	u32_t dtend_time;

	/* Look up all calendar components */
	for (int i = 0; i < component_cnt; i++) {
		p_com = ical_coms[i];
		if (p_com) {
			dtstart_date = atoi(p_com->dtstart);
			dtstart_time = atoi(p_com->dtstart
					    + strlen("YYYYMMDDT"));
			dtend_date = atoi(p_com->dtend);
			dtend_time = atoi(p_com->dtend
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

	if (ical.state == ICAL_PARSER_STATE_DOWNLOADING) {
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

	err = calendar_init();
	if (err != 0) {
		return;
	}

	k_delayed_work_init(&datetime_update_work, datetime_work_handler);
	k_delayed_work_submit(&datetime_update_work, ICAL_DATETIME_PERIOD);

	while (1) {
		leds_update();
	}
}
