#include<zephyr.h>
#include<bsd.h>
#include<nrf_socket.h>
#include<net/socket.h>
#include<stdio.h>

#define AT_XSYSTEMMODE "AT\%XSYSTEMMODE=0,0,1,0"
#define AT_MAGPIO      "AT\%XMAGPIO=1,0,0,1,1,1574,1577"
#define AT_CFUN        "AT+CFUN=1"

static const char update_indicator[] = {'\\', '|', '/', '-'};
static const char at_commands[][31]  = { AT_XSYSTEMMODE, AT_MAGPIO, AT_CFUN };
static int        fd;
static u64_t      fix_timestamp;
static char       nmea_strings[10][NRF_GNSS_NMEA_MAX_LEN];
static u32_t      nmea_string_cnt;

void bsd_recoverable_error_handler(uint32_t error)
{
	printk("Err: %lu\n", error);
}

void bsd_irrecoverable_error_handler(uint32_t error)
{
	printk("Irrecoverable: %lu\n", error);
}

static int enable_gps(void)
{
	int  at_sock;
	int  bytes_sent;
	int  bytes_received;
	char buf[3] = {0};

	at_sock = socket(AF_LTE, 0, NPROTO_AT);
	if (at_sock < 0) {
		return -1;
	}

	for (int i = 0; i < 3; i++) {
		bytes_sent = send(at_sock, at_commands[i],
				  strlen(at_commands[i]), 0);

		if (bytes_sent < 0) {
			return -1;
		}

		do {
			bytes_received = recv(at_sock, buf, 2, 0);
		} while (bytes_received == 0);

		if (strstr(buf, "OK") == NULL) {
			return -1;
		}
	}

	close(at_sock);

	return 0;
}

static int init_app(void)
{
	u16_t fix_retry     = 0;
	u32_t fix_len       = sizeof(uint16_t);
	u16_t nmea_mask     = 1;
	u32_t nmea_mask_len = sizeof(uint16_t);
	int   retval;

	if (enable_gps() != 0) {
		printk("Failed to enable GPS\n");
		return -1;
	}

	fd = nrf_socket(NRF_AF_LOCAL, NRF_SOCK_DGRAM, NRF_PROTO_GNSS);

	if (fd < 0) {
		printk("Could not init socket (err: %d)\n", fd);
		return -1;
	} else {
		printk("Socket created\n");
	}

	retval = nrf_setsockopt(fd,
		       NRF_SOL_GNSS,
		       NRF_SO_GNSS_FIX_RETRY,
		       &fix_retry,
		       fix_len);

	if (retval != 0) {
		printk("Failed to set fix retry value\n");
		return -1;
	}

	retval = nrf_setsockopt(fd,
			NRF_SOL_GNSS,
			NRF_SO_GNSS_NMEA_MASK,
			&nmea_mask,
			nmea_mask_len);

	if (retval != 0) {
		printk("Failed to set nmea mask\n");
		return -1;
	}

	retval = nrf_setsockopt(fd,
		       NRF_SOL_GNSS,
		       NRF_SO_GNSS_START,
		       NULL,
		       0);

	if (retval != 0) {
		printk("Failed to start GPS\n");
		return -1;
	}

	return 0;
}

static void get_satellite_stats(nrf_gnss_data_frame_t *pvt_data)
{
	u8_t  tracked          = 0;
	u8_t  in_fix           = 0;
	u8_t  unhealthy        = 0;

	for (int i = 0; i < NRF_GNSS_MAX_SATELLITES; ++i) {

		if ((pvt_data->pvt.sv[i].sv > 0) &&
		    (pvt_data->pvt.sv[i].sv < 32)) {

			tracked++;

			if (pvt_data->pvt.sv[i].flags &
					NRF_GNSS_PVT_FLAG_FIX_VALID_BIT) {
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

	printk("\nSeconds since last fix %lld\n", (k_uptime_get() - fix_timestamp) / 1000);
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

	nmea_string_cnt = 0;
}

int main(void)
{
	nrf_gnss_data_frame_t tmp_data;
	nrf_gnss_data_frame_t gps_data;

	u8_t cnt           = 0;
	bool got_first_fix = false;
	int  retval        = 0;


	printk("Staring GPS application\n");

	if (init_app() != 0) {
		return -1;
	}

	printk("Getting GPS data...\n");

	while (1) {
		do {
			retval = nrf_recv(fd,
					  &tmp_data, sizeof(tmp_data),
					  NRF_MSG_DONTWAIT);

			if (retval > 0) {
				switch (tmp_data.data_id) {
				case NRF_GNSS_PVT_DATA_ID:
					if ((tmp_data.pvt.flags &
						NRF_GNSS_PVT_FLAG_FIX_VALID_BIT)
						== NRF_GNSS_PVT_FLAG_FIX_VALID_BIT) {
						if (!got_first_fix) {
							got_first_fix = true;
						}

						fix_timestamp = k_uptime_get();

					}

					memcpy(&gps_data,
					       &tmp_data,
					       retval);

					cnt++;
					break;

				case NRF_GNSS_NMEA_DATA_ID:
					if ((gps_data.pvt.flags &
						NRF_GNSS_PVT_FLAG_FIX_VALID_BIT)
						== NRF_GNSS_PVT_FLAG_FIX_VALID_BIT) {
						if (nmea_string_cnt < 10) {
							memcpy(nmea_strings[nmea_string_cnt++],
							       tmp_data.nmea,
							       retval);
						}

					}
					break;

				default:
					break;
				}
			}
		} while (retval > 0);

		printk("\033[2J");
		get_satellite_stats(&gps_data);

		if (!got_first_fix) {
			printk("\nScanning [%c] ",
					update_indicator[cnt%4]);
		} else {

			printk("---------------------------------\n");
			print_pvt_data(&gps_data);
			printk("\n");
			print_nmea_data();
			printk("---------------------------------");

		}

		k_sleep(K_MSEC(1000));
	}

	return 0;

}
