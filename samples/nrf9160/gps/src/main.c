/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <nrf_socket.h>
#include <net/socket.h>
#include <stdio.h>

#define AT_XSYSTEMMODE "AT\%XSYSTEMMODE=0,0,1,0"
#define AT_CFUN        "AT+CFUN=1"

#ifdef CONFIG_BOARD_NRF9160_PCA10090NS
#define AT_MAGPIO      "AT\%XMAGPIO=1,0,0,1,1,1574,1577"
#define AT_COEX0       "AT\%XCOEX0=1,1,1570,1580"
#endif

static const char     update_indicator[] = {'\\', '|', '/', '-'};
static const char     at_commands[][31]  = {
				AT_XSYSTEMMODE,
#ifdef CONFIG_BOARD_NRF9160_PCA10090NS
				AT_MAGPIO,
				AT_COEX0,
#endif
				AT_CFUN
			};

static int            fd;

static char           nmea_strings[10][NRF_GNSS_NMEA_MAX_LEN];
static u32_t          nmea_string_cnt;

static bool           got_first_fix;
static bool           update_terminal;
static u64_t          fix_timestamp;
nrf_gnss_data_frame_t last_fix;

void bsd_recoverable_error_handler(uint32_t error)
{
	printf("Err: %lu\n", (unsigned long)error);
}

static int enable_gps(void)
{
	int  at_sock;
	int  bytes_sent;
	int  bytes_received;
	char buf[2];

	at_sock = socket(AF_LTE, 0, NPROTO_AT);
	if (at_sock < 0) {
		return -1;
	}

	for (int i = 0; i < ARRAY_SIZE(at_commands); i++) {
		bytes_sent = send(at_sock, at_commands[i],
				  strlen(at_commands[i]), 0);

		if (bytes_sent < 0) {
			close(at_sock);
			return -1;
		}

		do {
			bytes_received = recv(at_sock, buf, 2, 0);
		} while (bytes_received == 0);

		if (memcmp(buf, "OK", 2) != 0) {
			close(at_sock);
			return -1;
		}
	}

	close(at_sock);

	return 0;
}

static int init_app(void)
{
	int retval;

	nrf_gnss_fix_retry_t    fix_retry    = 0;
	nrf_gnss_fix_interval_t fix_interval = 1;
	nrf_gnss_delete_mask_t	delete_mask  = 0;
	nrf_gnss_nmea_mask_t	nmea_mask    = NRF_GNSS_NMEA_GSV_MASK |
					       NRF_GNSS_NMEA_GSA_MASK |
					       NRF_GNSS_NMEA_GLL_MASK |
					       NRF_GNSS_NMEA_GGA_MASK |
					       NRF_GNSS_NMEA_RMC_MASK;

	if (enable_gps() != 0) {
		printk("Failed to enable GPS\n");
		return -1;
	}

	fd = nrf_socket(NRF_AF_LOCAL, NRF_SOCK_DGRAM, NRF_PROTO_GNSS);

	if (fd >= 0) {
		printk("Socket created\n");
	} else {
		printk("Could not init socket (err: %d)\n", fd);
		return -1;
	}

	retval = nrf_setsockopt(fd,
				NRF_SOL_GNSS,
				NRF_SO_GNSS_FIX_RETRY,
				&fix_retry,
				sizeof(fix_retry));

	if (retval != 0) {
		printk("Failed to set fix retry value\n");
		return -1;
	}

	retval = nrf_setsockopt(fd,
				NRF_SOL_GNSS,
				NRF_SO_GNSS_FIX_INTERVAL,
				&fix_interval,
				sizeof(fix_interval));

	if (retval != 0) {
		printk("Failed to set fix interval value\n");
		return -1;
	}

	retval = nrf_setsockopt(fd,
				NRF_SOL_GNSS,
				NRF_SO_GNSS_NMEA_MASK,
				&nmea_mask,
				sizeof(nmea_mask));

	if (retval != 0) {
		printk("Failed to set nmea mask\n");
		return -1;
	}

	retval = nrf_setsockopt(fd,
				NRF_SOL_GNSS,
				NRF_SO_GNSS_START,
				&delete_mask,
				sizeof(delete_mask));

	if (retval != 0) {
		printk("Failed to start GPS\n");
		return -1;
	}

	return 0;
}

static void print_satellite_stats(nrf_gnss_data_frame_t *pvt_data)
{
	u8_t  tracked          = 0;
	u8_t  in_fix           = 0;
	u8_t  unhealthy        = 0;

	for (int i = 0; i < NRF_GNSS_MAX_SATELLITES; ++i) {

		if ((pvt_data->pvt.sv[i].sv > 0) &&
		    (pvt_data->pvt.sv[i].sv < 33)) {

			tracked++;

			if (pvt_data->pvt.sv[i].flags &
					NRF_GNSS_SV_FLAG_USED_IN_FIX) {
				in_fix++;
			}

			if (pvt_data->pvt.sv[i].flags &
					NRF_GNSS_SV_FLAG_UNHEALTHY) {
				unhealthy++;
			}
		}
	}

	printk("Tracking: %d Using: %d Unhealthy: %d", tracked,
						       in_fix,
						       unhealthy);

	printk("\nSeconds since last fix %lld\n",
			(k_uptime_get() - fix_timestamp) / 1000);
}

static void print_pvt_data(nrf_gnss_data_frame_t *pvt_data)
{
	printf("Longitude:  %f\n", pvt_data->pvt.longitude);
	printf("Latitude:   %f\n", pvt_data->pvt.latitude);
	printf("Altitude:   %f\n", pvt_data->pvt.altitude);
	printf("Speed:      %f\n", pvt_data->pvt.speed);
	printf("Heading:    %f\n", pvt_data->pvt.heading);
	printk("Date:       %02u-%02u-%02u\n", pvt_data->pvt.datetime.day,
					       pvt_data->pvt.datetime.month,
					       pvt_data->pvt.datetime.year);
	printk("Time (UTC): %02u:%02u:%02u\n", pvt_data->pvt.datetime.hour,
					       pvt_data->pvt.datetime.minute,
					      pvt_data->pvt.datetime.seconds);
}

static void print_nmea_data(void)
{
	printk("NMEA strings:\n");

	for (int i = 0; i < nmea_string_cnt; ++i) {
		printk("%s\n", nmea_strings[i]);
	}
}

int process_gps_data(nrf_gnss_data_frame_t *gps_data)
{
	int retval;

	retval = nrf_recv(fd, gps_data, sizeof(nrf_gnss_data_frame_t), NRF_MSG_DONTWAIT);

	if (retval > 0) {

		switch (gps_data->data_id) {
		case NRF_GNSS_PVT_DATA_ID:

			if ((gps_data->pvt.flags &
				NRF_GNSS_PVT_FLAG_FIX_VALID_BIT)
				== NRF_GNSS_PVT_FLAG_FIX_VALID_BIT) {

				if (!got_first_fix) {
					got_first_fix = true;
				}

				fix_timestamp = k_uptime_get();
				memcpy(&last_fix, gps_data, sizeof(nrf_gnss_data_frame_t));

				nmea_string_cnt = 0;
				update_terminal = true;
			}
			break;

		case NRF_GNSS_NMEA_DATA_ID:
			if (nmea_string_cnt < 10) {
				memcpy(nmea_strings[nmea_string_cnt++],
				       gps_data->nmea,
				       retval);
			}
			break;

		default:
			break;
		}
	}

	return retval;
}

int main(void)
{
	nrf_gnss_data_frame_t gps_data;
	u8_t		      cnt = 0;

	printk("Staring GPS application\n");

	if (init_app() != 0) {
		return -1;
	}

	printk("Getting GPS data...\n");

	while (1) {

		do {
			/* Loop until we don't have more
			 * data to read
			 */
		} while (process_gps_data(&gps_data) > 0);

		if (!got_first_fix) {
			cnt++;
			printk("\033[1;1H");
			printk("\033[2J");
			print_satellite_stats(&gps_data);
			printk("\nScanning [%c] ",
					update_indicator[cnt%4]);
		}

		if (((k_uptime_get() - fix_timestamp) >= 1) &&
		     (got_first_fix)) {
			printk("\033[1;1H");
			printk("\033[2J");

			print_satellite_stats(&gps_data);

			printk("---------------------------------\n");
			print_pvt_data(&last_fix);
			printk("\n");
			print_nmea_data();
			printk("---------------------------------");

			update_terminal = false;
		}

		k_sleep(K_MSEC(500));
	}

	return 0;
}
