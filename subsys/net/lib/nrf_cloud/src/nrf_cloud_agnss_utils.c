/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#include <net/nrf_cloud_agnss.h>
#include <nrf_modem_gnss.h>

static void print_utc(struct nrf_modem_gnss_agnss_gps_data_utc *data)
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

static void print_ephemeris(struct nrf_modem_gnss_agnss_gps_data_ephemeris *data)
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

static void print_almanac(struct nrf_modem_gnss_agnss_gps_data_almanac *data)
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

static void print_klobuchar(struct nrf_modem_gnss_agnss_data_klobuchar *data)
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

static void print_nequick(struct nrf_modem_gnss_agnss_data_nequick *data)
{
	printk("nequick\n");
	printk("\tai0: %d\n", data->ai0);
	printk("\tai1: %d\n", data->ai1);
	printk("\tai2: %d\n", data->ai2);
	printk("\tstorm_cond: %d\n", data->storm_cond);
	printk("\tstorm_valid: %d\n", data->storm_valid);
}

static void print_clock_and_tows(struct nrf_modem_gnss_agnss_gps_data_system_time_and_sv_tow *data)
{
	printk("clock_and_tows\n");
	printk("\tdate_day: %d\n", data->date_day);
	printk("\ttime_full_s: %d\n", data->time_full_s);
	printk("\ttime_frac_ms: %d\n", data->time_frac_ms);
	printk("\tsv_mask: 0x%08x\n", data->sv_mask);
	printk("\tsv_tow\n");

	for (size_t i = 0; i < NRF_MODEM_GNSS_AGNSS_GPS_MAX_SV_TOW; i++) {
		printk("\t\tsv_tow[%d]\n", i);
		printk("\t\t\ttlm: %d\n", data->sv_tow[i].tlm);
		printk("\t\t\tflags: 0x%02x\n", data->sv_tow[i].flags);
	}
}

static void print_location(struct nrf_modem_gnss_agnss_data_location *data)
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

static void print_gps_integrity(struct nrf_modem_gnss_agps_data_integrity *data)
{
	printk("integrity\n");
	printk("\tintegrity_mask: 0x%08x\n", data->integrity_mask);
}

static void print_gnss_integrity(struct nrf_modem_gnss_agnss_data_integrity *data)
{
	printk("integrity\n");
	for (int i = 0; i < data->signal_count; i++) {
		printk("\tsignal_id: %d, integrity_mask: 0x%llx\n",
		       data->signal[i].signal_id, data->signal[i].integrity_mask);
	}
}

void agnss_print(uint16_t type, void *data)
{

	switch (type) {
	case NRF_MODEM_GNSS_AGNSS_GPS_UTC_PARAMETERS: {
		print_utc((struct nrf_modem_gnss_agnss_gps_data_utc *)data);
		break;
	}
	case NRF_MODEM_GNSS_AGNSS_GPS_EPHEMERIDES: {
		print_ephemeris((struct nrf_modem_gnss_agnss_gps_data_ephemeris *)data);
		break;
	}
	case NRF_MODEM_GNSS_AGNSS_GPS_ALMANAC: {
		print_almanac((struct nrf_modem_gnss_agnss_gps_data_almanac *)data);
		break;
	}
	case NRF_MODEM_GNSS_AGNSS_KLOBUCHAR_IONOSPHERIC_CORRECTION: {
		print_klobuchar((struct nrf_modem_gnss_agnss_data_klobuchar *)data);
		break;
	}
	case NRF_MODEM_GNSS_AGNSS_NEQUICK_IONOSPHERIC_CORRECTION: {
		print_nequick((struct nrf_modem_gnss_agnss_data_nequick *)data);
		break;
	}
	case NRF_MODEM_GNSS_AGNSS_GPS_SYSTEM_CLOCK_AND_TOWS: {
		print_clock_and_tows(
			(struct nrf_modem_gnss_agnss_gps_data_system_time_and_sv_tow *)data);
		break;
	}
	case NRF_MODEM_GNSS_AGNSS_LOCATION: {
		print_location((struct nrf_modem_gnss_agnss_data_location *)data);
		break;
	}
	case NRF_MODEM_GNSS_AGPS_INTEGRITY: {
		print_gps_integrity((struct nrf_modem_gnss_agps_data_integrity *)data);
		break;
	}
	case NRF_MODEM_GNSS_AGNSS_INTEGRITY: {
		print_gnss_integrity((struct nrf_modem_gnss_agnss_data_integrity *)data);
		break;
	}
	default:
		printk("Unknown AGNSS data type\n");
		break;
	}
}
