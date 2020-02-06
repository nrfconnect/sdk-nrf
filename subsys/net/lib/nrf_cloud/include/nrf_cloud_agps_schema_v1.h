/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>

#ifndef NRF_CLOUD_AGPS_SCHEMA_V1_H_
#define NRF_CLOUD_AGPS_SCHEMA_V1_H_

#ifdef __cplusplus
extern "C" {
#endif

#define NRF_CLOUD_AGPS_BIN_SCHEMA_VERSION		(1)

#define NRF_CLOUD_AGPS_BIN_SCHEMA_VERSION_INDEX		(0)
#define NRF_CLOUD_AGPS_BIN_SCHEMA_VERSION_SIZE		(1)
#define NRF_CLOUD_AGPS_BIN_TYPE_OFFSET			(0)
#define NRF_CLOUD_AGPS_BIN_TYPE_SIZE			(1)
#define NRF_CLOUD_AGPS_BIN_COUNT_OFFSET			(1)
#define NRF_CLOUD_AGPS_BIN_COUNT_SIZE			(2)


/* Maximum number of satellites in time-of-week data array. */
#define NRF_CLOUD_AGPS_MAX_SV_TOW			(32U)

enum nrf_cloud_agps_type {
	NRF_CLOUD_AGPS_UTC_PARAMETERS			= 1,
	NRF_CLOUD_AGPS_EPHEMERIDES			= 2,
	NRF_CLOUD_AGPS_ALMANAC				= 3,
	NRF_CLOUD_AGPS_KLOBUCHAR_CORRECTION		= 4,
	NRF_CLOUD_AGPS_NEQUICK_CORRECTION		= 5,
	NRF_CLOUD_AGPS_GPS_TOWS				= 6,
	NRF_CLOUD_AGPS_GPS_SYSTEM_CLOCK			= 7,
	NRF_CLOUD_AGPS_LOCATION				= 8,
	NRF_CLOUD_AGPS_INTEGRITY			= 9,
};

struct nrf_cloud_agps_utc {
	s32_t a1;
	s32_t a0;
	u8_t tot;
	u8_t wn_t;
	s8_t delta_tls;
	u8_t wn_lsf;
	s8_t dn;
	s8_t delta_tlsf;
} __packed;

struct nrf_cloud_agps_ephemeris {
	u8_t sv_id;
	u8_t health;
	u16_t iodc;
	u16_t toc;
	s8_t af2;
	s16_t af1;
	s32_t af0;
	s8_t tgd;
	u8_t ura;
	u8_t fit_int;
	u16_t toe;
	s32_t w;
	s16_t delta_n;
	s32_t m0;
	s32_t omega_dot;
	u32_t e;
	s16_t idot;
	u32_t sqrt_a;
	s32_t i0;
	s32_t omega0;
	s16_t crs;
	s16_t cis;
	s16_t cus;
	s16_t crc;
	s16_t cic;
	s16_t cuc;
} __packed;

struct nrf_cloud_agps_almanac {
	u8_t sv_id;
	u8_t wn;
	u8_t toa;
	u8_t ioda;
	u16_t e;
	s16_t delta_i;
	s16_t omega_dot;
	u8_t sv_health;
	u32_t sqrt_a;
	s32_t omega0;
	s32_t w;
	s32_t m0;
	s16_t af0;
	s16_t af1;
} __packed;

struct nrf_cloud_agps_klobuchar {
	s8_t alpha0;
	s8_t alpha1;
	s8_t alpha2;
	s8_t alpha3;
	s8_t beta0;
	s8_t beta1;
	s8_t beta2;
	s8_t beta3;
} __packed;

struct nrf_cloud_agps_nequick {
	s16_t ai0;
	s16_t ai1;
	s16_t ai2;
	u8_t storm_cond;
	u8_t storm_valid;
} __packed;

struct nrf_cloud_agps_tow_element {
	u8_t sv_id;
	u16_t tlm;
	u8_t flags;
} __packed;

struct nrf_cloud_agps_system_time {
	u16_t date_day;
	u32_t time_full_s;
	u16_t time_frac_ms;
	u32_t sv_mask;
	struct nrf_cloud_agps_tow_element sv_tow[NRF_CLOUD_AGPS_MAX_SV_TOW];
} __packed;

struct nrf_cloud_agps_location {
	s32_t latitude;
	s32_t longitude;
	s16_t altitude;
	u8_t unc_semimajor;
	u8_t unc_semiminor;
	u8_t orientation_major;
	u8_t unc_altitude;
	u8_t confidence;
} __packed;

struct nrf_cloud_agps_integrity {
	u32_t integrity_mask;
};

struct nrf_cloud_apgs_element {
	enum nrf_cloud_agps_type type;
	union {
		struct nrf_cloud_agps_utc *utc;
		struct nrf_cloud_agps_ephemeris *ephemeris;
		struct nrf_cloud_agps_almanac *almanac;
		union {
			struct nrf_cloud_agps_klobuchar *klobuchar;
			struct nrf_cloud_agps_nequick *nequick;
		} ion_correction;
		struct nrf_cloud_agps_tow_element *tow;
		struct nrf_cloud_agps_system_time *time_and_tow;
		struct nrf_cloud_agps_location *location;
		struct nrf_cloud_agps_integrity *integrity;
	};
};


#ifdef __cplusplus
}
#endif

#endif /* NRF_CLOUD_AGPS_SCHEMA_V1_H_ */
