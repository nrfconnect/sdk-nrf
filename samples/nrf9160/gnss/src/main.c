/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <nrf_modem_gnss.h>
#include <string.h>
#include <modem/at_cmd.h>
#include <modem/lte_lc.h>

#ifdef CONFIG_SUPL_CLIENT_LIB
#include <supl_os_client.h>
#include <supl_session.h>
#include "supl_support.h"
#endif

#define AT_XSYSTEMMODE      "AT\%XSYSTEMMODE=0,0,1,0"
#define AT_ACTIVATE_GNSS    "AT+CFUN=31"

#define AT_CMD_SIZE(x) (sizeof(x) - 1)

static const char update_indicator[] = {'\\', '|', '/', '-'};
static const char *const at_commands[] = {
#if !defined(CONFIG_SUPL_CLIENT_LIB)
	AT_XSYSTEMMODE,
#endif
	CONFIG_GNSS_SAMPLE_AT_MAGPIO,
	CONFIG_GNSS_SAMPLE_AT_COEX0,
	AT_ACTIVATE_GNSS
};

static struct nrf_modem_gnss_pvt_data_frame last_pvt;
static volatile bool gnss_blocked;

#ifdef CONFIG_SUPL_CLIENT_LIB
static struct nrf_modem_gnss_agps_data_frame last_agps;
static struct k_work_q agps_work_q;
static struct k_work get_agps_data_work;

#define AGPS_WORKQ_THREAD_STACK_SIZE 2048
#define AGPS_WORKQ_THREAD_PRIORITY   5

K_THREAD_STACK_DEFINE(agps_workq_stack_area, AGPS_WORKQ_THREAD_STACK_SIZE);
#endif

K_MSGQ_DEFINE(nmea_queue, sizeof(struct nrf_modem_gnss_nmea_data_frame *), 10, 4);
K_SEM_DEFINE(pvt_data_sem, 0, 1);
K_SEM_DEFINE(lte_ready, 0, 1);

struct k_poll_event events[2] = {
	K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_SEM_AVAILABLE,
					K_POLL_MODE_NOTIFY_ONLY,
					&pvt_data_sem, 0),
	K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_MSGQ_DATA_AVAILABLE,
					K_POLL_MODE_NOTIFY_ONLY,
					&nmea_queue, 0),
};

void nrf_modem_recoverable_error_handler(uint32_t error)
{
	printk("Modem library recoverable error: %u\n", error);
}

static int setup_modem(void)
{
	for (int i = 0; i < ARRAY_SIZE(at_commands); i++) {
		if (at_commands[i][0] == '\0') {
			continue;
		}

		if (at_cmd_write(at_commands[i], NULL, 0, NULL) != 0) {
			return -1;
		}
	}

	return 0;
}

#ifdef CONFIG_SUPL_CLIENT_LIB
BUILD_ASSERT(IS_ENABLED(CONFIG_LTE_NETWORK_MODE_LTE_M_GPS) ||
	     IS_ENABLED(CONFIG_LTE_NETWORK_MODE_NBIOT_GPS) ||
	     IS_ENABLED(CONFIG_LTE_NETWORK_MODE_LTE_M_NBIOT_GPS),
	     "To use SUPL and GPS, CONFIG_LTE_NETWORK_MODE_LTE_M_GPS, "
	     "CONFIG_LTE_NETWORK_MODE_NBIOT_GPS or "
	     "CONFIG_LTE_NETWORK_MODE_LTE_M_NBIOT_GPS must be enabled");

static void lte_handler(const struct lte_lc_evt *const evt)
{
	switch (evt->type) {
	case LTE_LC_EVT_NW_REG_STATUS:
		if ((evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME) ||
		    (evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_ROAMING)) {
			printk("Connected to LTE network\n");
			k_sem_give(&lte_ready);
		}
		break;
	default:
		break;
	}
}

static int activate_lte(bool activate)
{
	int err;

	if (activate) {
		err = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_ACTIVATE_LTE);
		if (err) {
			printk("Failed to activate LTE, error: %d\n", err);
			return -1;
		}

		printk("LTE activated\n");
		k_sem_take(&lte_ready, K_FOREVER);
	} else {
		err = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_DEACTIVATE_LTE);
		if (err) {
			printk("Failed to deactivate LTE, error: %d\n", err);
			return -1;
		}

		printk("LTE deactivated\n");
	}

	return 0;
}

static void get_agps_data(struct k_work *item)
{
	ARG_UNUSED(item);

	printk("\033[1;1H");
	printk("\033[2J");
	printk("New A-GPS data requested, contacting SUPL server, flags %d\n",
			last_agps.data_flags);

	activate_lte(true);

	/* Wait for a while, because with IPv4v6 PDN the IPv6 activation takes a bit more time. */
	k_sleep(K_SECONDS(1));

	if (open_supl_socket() == 0) {
		printk("Starting SUPL session\n");
		supl_session(&last_agps);
		printk("Done\n");
		close_supl_socket();
	}
	activate_lte(false);
}

static int inject_agps_type(void *agps, size_t agps_size, uint16_t type, void *user_data)
{
	ARG_UNUSED(user_data);

	int retval = nrf_modem_gnss_agps_write(agps, agps_size, type);

	if (retval != 0) {
		printk("Failed to write A-GPS data, type: %d (errno: %d)\n", type, errno);
		return -1;
	}

	printk("Injected A-GPS data, type: %d, size: %d\n", type, agps_size);

	return 0;
}
#endif

static void gnss_event_handler(int event)
{
	int retval;
	struct nrf_modem_gnss_nmea_data_frame *nmea_data;

	switch (event) {
	case NRF_MODEM_GNSS_EVT_PVT:
		retval = nrf_modem_gnss_read(&last_pvt, sizeof(last_pvt), NRF_MODEM_GNSS_DATA_PVT);
		if (retval == 0) {
			k_sem_give(&pvt_data_sem);
		}
		break;

	case NRF_MODEM_GNSS_EVT_NMEA:
		nmea_data = k_malloc(sizeof(struct nrf_modem_gnss_nmea_data_frame));
		if (nmea_data == NULL) {
			printk("Failed to allocate memory for NMEA\n");
			break;
		}

		retval = nrf_modem_gnss_read(nmea_data,
					     sizeof(struct nrf_modem_gnss_nmea_data_frame),
					     NRF_MODEM_GNSS_DATA_NMEA);
		if (retval == 0) {
			retval = k_msgq_put(&nmea_queue, &nmea_data, K_NO_WAIT);
		}

		if (retval != 0) {
			k_free(nmea_data);
		}
		break;

	case NRF_MODEM_GNSS_EVT_AGPS_REQ:
#ifdef CONFIG_SUPL_CLIENT_LIB
		retval = nrf_modem_gnss_read(&last_agps,
					     sizeof(last_agps),
					     NRF_MODEM_GNSS_DATA_AGPS_REQ);
		if (retval == 0) {
			if (last_agps.sv_mask_ephe == 0 &&
			    last_agps.sv_mask_alm == 0 &&
			    last_agps.data_flags == NRF_MODEM_GNSS_AGPS_INTEGRITY_REQUEST) {
				/* Skip request because only integrity data is requested and
				 * Google SUPL server doesn't provide that.
				 */
				break;
			}
			k_work_submit_to_queue(&agps_work_q, &get_agps_data_work);
		}
#endif
		break;

	case NRF_MODEM_GNSS_EVT_BLOCKED:
		gnss_blocked = true;
		break;

	case NRF_MODEM_GNSS_EVT_UNBLOCKED:
		gnss_blocked = false;
		break;

	default:
		break;
	}
}

static int init_app(void)
{
#ifdef CONFIG_SUPL_CLIENT_LIB
	if (lte_lc_init()) {
		printk("Failed to initialize LTE link controller\n");
		return -1;
	}

	lte_lc_register_handler(lte_handler);

	static struct supl_api supl_api = {
		.read       = supl_read,
		.write      = supl_write,
		.handler    = inject_agps_type,
		.logger     = supl_logger,
		.counter_ms = k_uptime_get
	};

	k_work_queue_start(
		&agps_work_q,
		agps_workq_stack_area,
		K_THREAD_STACK_SIZEOF(agps_workq_stack_area),
		AGPS_WORKQ_THREAD_PRIORITY,
		NULL);

	k_work_init(&get_agps_data_work, get_agps_data);

	if (supl_init(&supl_api) != 0) {
		printk("Failed to initialize SUPL library\n");
		return -1;
	}
#endif /* CONFIG_SUPL_CLIENT_LIB */

	if (setup_modem() != 0) {
		printk("Failed to initialize modem\n");
		return -1;
	}

	/* Initialize and configure GNSS */
	if (nrf_modem_gnss_init() != 0) {
		printk("Failed to initialize GNSS interface\n");
		return -1;
	}

	if (nrf_modem_gnss_event_handler_set(gnss_event_handler) != 0) {
		printk("Failed to set GNSS event handler\n");
		return -1;
	}

	if (nrf_modem_gnss_nmea_mask_set(NRF_MODEM_GNSS_NMEA_RMC_MASK |
					 NRF_MODEM_GNSS_NMEA_GGA_MASK |
					 NRF_MODEM_GNSS_NMEA_GLL_MASK |
					 NRF_MODEM_GNSS_NMEA_GSA_MASK |
					 NRF_MODEM_GNSS_NMEA_GSV_MASK) != 0) {
		printk("Failed to set GNSS NMEA mask\n");
		return -1;
	}

	if (nrf_modem_gnss_fix_retry_set(0) != 0) {
		printk("Failed to set GNSS fix retry\n");
		return -1;
	}

	if (nrf_modem_gnss_fix_interval_set(1) != 0) {
		printk("Failed to set GNSS fix interval\n");
		return -1;
	}

	if (nrf_modem_gnss_start() != 0) {
		printk("Failed to start GNSS\n");
		return -1;
	}

	return 0;
}

static void print_satellite_stats(struct nrf_modem_gnss_pvt_data_frame *pvt_data)
{
	uint8_t tracked   = 0;
	uint8_t in_fix    = 0;
	uint8_t unhealthy = 0;

	for (int i = 0; i < NRF_MODEM_GNSS_MAX_SATELLITES; ++i) {
		if (pvt_data->sv[i].sv > 0) {
			tracked++;

			if (pvt_data->sv[i].flags & NRF_MODEM_GNSS_SV_FLAG_USED_IN_FIX) {
				in_fix++;
			}

			if (pvt_data->sv[i].flags & NRF_MODEM_GNSS_SV_FLAG_UNHEALTHY) {
				unhealthy++;
			}
		}
	}

	printk("Tracking: %d Using: %d Unhealthy: %d\n", tracked, in_fix, unhealthy);
}

static void print_fix_data(struct nrf_modem_gnss_pvt_data_frame *pvt_data)
{
	printk("Latitude:   %.06f\n", pvt_data->latitude);
	printk("Longitude:  %.06f\n", pvt_data->longitude);
	printk("Altitude:   %.01f m\n", pvt_data->altitude);
	printk("Accuracy:   %.01f m\n", pvt_data->accuracy);
	printk("Speed:      %.01f m/s\n", pvt_data->speed);
	printk("Heading:    %.01f deg\n", pvt_data->heading);
	printk("Date:       %02u-%02u-%02u\n", pvt_data->datetime.year,
					       pvt_data->datetime.month,
					       pvt_data->datetime.day);
	printk("Time (UTC): %02u:%02u:%02u\n", pvt_data->datetime.hour,
					       pvt_data->datetime.minute,
					       pvt_data->datetime.seconds);
}

static bool agps_data_download_ongoing(void)
{
#ifdef CONFIG_SUPL_CLIENT_LIB
	return k_work_is_pending(&get_agps_data_work);
#else
	return false;
#endif
}

int main(void)
{
	uint8_t cnt = 0;
	uint64_t fix_timestamp = 0;
	struct nrf_modem_gnss_nmea_data_frame *nmea_data;

	printk("Starting GNSS sample\n");

	if (init_app() != 0) {
		return -1;
	}

	printk("Getting GNSS data...\n");

	for (;;) {
		(void)k_poll(events, 2, K_FOREVER);

		if (events[0].state == K_POLL_STATE_SEM_AVAILABLE &&
		    k_sem_take(events[0].sem, K_NO_WAIT) == 0) {
			/* New PVT data available */

			if (!IS_ENABLED(CONFIG_GNSS_SAMPLE_NMEA_ONLY) &&
			    !agps_data_download_ongoing()) {
				printk("\033[1;1H");
				printk("\033[2J");
				print_satellite_stats(&last_pvt);

				if (gnss_blocked) {
					printk("GNSS operation blocked by LTE\n");
				}
				printk("---------------------------------\n");

				if (last_pvt.flags & NRF_MODEM_GNSS_PVT_FLAG_FIX_VALID) {
					fix_timestamp = k_uptime_get();
					print_fix_data(&last_pvt);
				} else {
					printk("Seconds since last fix: %lld\n",
							(k_uptime_get() - fix_timestamp) / 1000);
					cnt++;
					printk("Searching [%c]\n", update_indicator[cnt%4]);
				}

				printk("\nNMEA strings:\n\n");
			}
		}
		if (events[1].state == K_POLL_STATE_MSGQ_DATA_AVAILABLE &&
		    k_msgq_get(events[1].msgq, &nmea_data, K_NO_WAIT) == 0) {
			/* New NMEA data available */

			if (!agps_data_download_ongoing()) {
				printk("%s", nmea_data->nmea_str);
			}
			k_free(nmea_data);
		}

		events[0].state = K_POLL_STATE_NOT_READY;
		events[1].state = K_POLL_STATE_NOT_READY;
	}

	return 0;
}
