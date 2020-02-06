/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>

#include <net/nrf_cloud_agps.h>
#include <nrf_socket.h>

static void print_utc(nrf_gnss_agps_data_utc_t *data)
{
	printk("utc:\n");
	printk("\ta1: %d\n", data->a1);
	printk("\ta0: %d\n", data->a0);
	printk("\ttot: %d\n", data->tot);
	printk("\twn_t: %d\n", data->wn_t);
	printk("\tdelta_tls: %d\n", data->delta_tls);
	printk("\twn_lsf: %d\n", data->wn_lsf);
	printk("\tdn: %d\n", data->dn);
	printk("\tdelta_tlsf: %d\n", data->delta_tlsf);
}

static void print_ephemeris(nrf_gnss_agps_data_ephemeris_t *data)
{
	printk("ephemeris:\n");
	printk("\tsv_id: %d\n", data->sv_id);
	printk("\thealth: %d\n", data->health);
	printk("\tiodc: %d\n", data->iodc);
	printk("\ttoc: %d\n", data->toc);
	printk("\taf2: %d\n", data->af2);
	printk("\taf1: %d\n", data->af1);
	printk("\taf0: %d\n", data->af0);
	printk("\ttgd: %d\n", data->tgd);
	printk("\tura: %d\n", data->ura);
	printk("\tfit_int: %d\n", data->fit_int);
	printk("\ttoe: %d\n", data->toe);
	printk("\tw: %d\n", data->w);
	printk("\tdelta_n: %d\n", data->delta_n);
	printk("\tm0: %d\n", data->m0);
	printk("\tomega_dot: %d\n", data->omega_dot);
	printk("\te: %d\n", data->e);
	printk("\tidot: %d\n", data->idot);
	printk("\tsqrt_a: %d\n", data->sqrt_a);
	printk("\ti0: %d\n", data->i0);
	printk("\tomega0: %d\n", data->omega0);
	printk("\tcrs: %d\n", data->crs);
	printk("\tcis: %d\n", data->cis);
	printk("\tcus: %d\n", data->cus);
	printk("\tcrc: %d\n", data->crc);
	printk("\tcic: %d\n", data->cic);
	printk("\tcuc: %d\n", data->cuc);
}

static void print_almanac(nrf_gnss_agps_data_almanac_t *data)
{
	printk("almanac\n");
	printk("\tsv_id: %d\n", data->sv_id);
	printk("\twn: %d\n", data->wn);
	printk("\ttoa: %d\n", data->toa);
	printk("\tioda: %d\n", data->ioda);
	printk("\te: %d\n", data->e);
	printk("\tdelta_i: %d\n", data->delta_i);
	printk("\tomega_dot: %d\n", data->omega_dot);
	printk("\tsv_health: %d\n", data->sv_health);
	printk("\tsqrt_a: %d\n", data->sqrt_a);
	printk("\tomega0: %d\n", data->omega0);
	printk("\tw: %d\n", data->w);
	printk("\tm0: %d\n", data->m0);
	printk("\taf0: %d\n", data->af0);
	printk("\taf1: %d\n", data->af1);
}

static void print_klobuchar(nrf_gnss_agps_data_klobuchar_t *data)
{
	printk("klobuchar\n");
	printk("\talpha0: %d\n", data->alpha0);
	printk("\talpha1: %d\n", data->alpha1);
	printk("\talpha2: %d\n", data->alpha2);
	printk("\talpha3: %d\n", data->alpha3);
	printk("\tbeta0: %d\n", data->beta0);
	printk("\tbeta1: %d\n", data->beta1);
	printk("\tbeta2: %d\n", data->beta2);
	printk("\tbeta3: %d\n", data->beta3);
}

static void print_clock_and_tows(
	nrf_gnss_agps_data_system_time_and_sv_tow_t *data)
{
	printk("clock_and_tows\n");
	printk("\tdate_day: %d\n", data->date_day);
	printk("\ttime_full_s: %d\n", data->time_full_s);
	printk("\ttime_frac_ms: %d\n", data->time_frac_ms);
	printk("\tsv_mask: %d\n", data->sv_mask);
	printk("\tsv_tow\n");

	for (size_t i = 0; i < NRF_GNSS_AGPS_MAX_SV_TOW; i++) {
		printk("\t\tsv_tow[%d]\n", i);
		printk("\t\t\ttlm: %d\n", data->sv_tow[i].tlm);
		printk("\t\t\tflags: %d\n", data->sv_tow[i].flags);
	}
}

static void print_location(nrf_gnss_agps_data_location_t *data)
{
	printk("location\n");
	printk("\tlatitude: %d\n", data->latitude);
	printk("\tlongitude: %d\n", data->longitude);
	printk("\taltitude: %d\n", data->altitude);
	printk("\tunc_semimajor: %d\n", data->unc_semimajor);
	printk("\tunc_semiminor: %d\n", data->unc_semiminor);
	printk("\torientation_major: %d\n", data->orientation_major);
	printk("\tunc_altitude: %d\n", data->unc_altitude);
	printk("\tconfidence: %d\n", data->confidence);
}

static void print_integrity(nrf_gnss_agps_data_integrity_t *data)
{
	printk("integrity\n");
	printk("\tintegrity_mask: %d\n", data->integrity_mask);
}

void agps_print(nrf_gnss_agps_data_type_t type, void *data)
{

	switch (type) {
	case NRF_GNSS_AGPS_UTC_PARAMETERS: {
		print_utc((nrf_gnss_agps_data_utc_t *)data);
		break;
	}
	case NRF_GNSS_AGPS_EPHEMERIDES: {
		print_ephemeris((nrf_gnss_agps_data_ephemeris_t *)data);
		break;
	}
	case NRF_GNSS_AGPS_ALMANAC: {
		print_almanac((nrf_gnss_agps_data_almanac_t *)data);
		break;
	}
	case NRF_GNSS_AGPS_KLOBUCHAR_IONOSPHERIC_CORRECTION: {
		print_klobuchar((nrf_gnss_agps_data_klobuchar_t *)data);
		break;
	}
	case NRF_GNSS_AGPS_NEQUICK_IONOSPHERIC_CORRECTION: {
		printk("nequick unhandled\n");
		break;
	}
	case NRF_GNSS_AGPS_GPS_SYSTEM_CLOCK_AND_TOWS: {
		print_clock_and_tows(
			(nrf_gnss_agps_data_system_time_and_sv_tow_t *)data);
		break;
	}
	case NRF_GNSS_AGPS_LOCATION: {
		print_location((nrf_gnss_agps_data_location_t *)data);
		break;
	}
	case NRF_GNSS_AGPS_INTEGRITY: {
		print_integrity((nrf_gnss_agps_data_integrity_t *)data);
		break;
	}
	default:
		printk("Unknown AGPS data type\n");
		break;
	}
}
