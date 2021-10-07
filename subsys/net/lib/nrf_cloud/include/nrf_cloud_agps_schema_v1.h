/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
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

#define NRF_CLOUD_AGPS_MAX_SV				(32U)
/* Maximum number of satellites in time-of-week data array. */
#define NRF_CLOUD_AGPS_MAX_SV_TOW			(32U)

/* Define sizes of data actually sent from the cloud; sizes provided to the modem
 * will be smaller (no _HSIZE header).  The TOWS and System Time structures from the
 * file will be combined and sent to the modem as one structure.
 */
#define _HSZ					(NRF_CLOUD_AGPS_BIN_TYPE_SIZE + \
						 NRF_CLOUD_AGPS_BIN_COUNT_SIZE)
#define NRF_CLOUD_AGPS_UTC_SIZE			(_HSZ + sizeof(struct nrf_cloud_agps_utc))
#define NRF_CLOUD_AGPS_EPHEMERIS_SIZE		(_HSZ + \
						 sizeof(struct nrf_cloud_agps_ephemeris) * \
						 NRF_CLOUD_AGPS_MAX_SV)
#define NRF_CLOUD_AGPS_FILTERED_EPHEMERIS_SIZE	(_HSZ + \
						 sizeof(struct nrf_cloud_agps_ephemeris) * \
						 NRF_CLOUD_AGPS_MAX_SV / 2)
#define NRF_CLOUD_AGPS_ALMANAC_SIZE		(_HSZ + \
						 sizeof(struct nrf_cloud_agps_almanac) * \
						 NRF_CLOUD_AGPS_MAX_SV)
#define NRF_CLOUD_AGPS_KLOBUCHAR_SIZE		(_HSZ + sizeof(struct nrf_cloud_agps_klobuchar))
#define NRF_CLOUD_AGPS_NEQUICK_SIZE		(_HSZ + sizeof(struct nrf_cloud_agps_nequick))
#define NRF_CLOUD_AGPS_TOWS_SIZE		(_HSZ + \
						 sizeof(struct nrf_cloud_agps_tow_element) * \
						 NRF_CLOUD_AGPS_MAX_SV_TOW)
#define NRF_CLOUD_AGPS_SYSTEM_TIME_SIZE		(_HSZ + \
						 sizeof(struct nrf_cloud_agps_system_time) - \
						 sizeof(struct nrf_cloud_agps_tow_element) * \
						 NRF_CLOUD_AGPS_MAX_SV_TOW)
#define NRF_CLOUD_AGPS_LOCATION_SIZE		(_HSZ + sizeof(struct nrf_cloud_agps_location))
#define NRF_CLOUD_AGPS_INTEGRITY_SIZE		(_HSZ + sizeof(struct nrf_cloud_agps_integrity))

/* Absolute largest possible A-GPS file size */
#define NRF_CLOUD_AGPS_MAX_FILE_SIZE ( \
	NRF_CLOUD_AGPS_BIN_SCHEMA_VERSION_SIZE * 2 + NRF_CLOUD_AGPS_UTC_SIZE + \
	NRF_CLOUD_AGPS_EPHEMERIS_SIZE + NRF_CLOUD_AGPS_ALMANAC_SIZE + \
	NRF_CLOUD_AGPS_KLOBUCHAR_SIZE + NRF_CLOUD_AGPS_NEQUICK_SIZE + \
	NRF_CLOUD_AGPS_TOWS_SIZE + NRF_CLOUD_AGPS_SYSTEM_TIME_SIZE + \
	NRF_CLOUD_AGPS_LOCATION_SIZE + NRF_CLOUD_AGPS_INTEGRITY_SIZE)

/* Absolute largest possible A-GPS file size when filtering ephemeris to only those visible,
 * which is one half of the constellation size.
 */
#define NRF_CLOUD_AGPS_FILTERED_MAX_FILE_SIZE ( \
	NRF_CLOUD_AGPS_BIN_SCHEMA_VERSION_SIZE * 2 + NRF_CLOUD_AGPS_UTC_SIZE + \
	NRF_CLOUD_AGPS_FILTERED_EPHEMERIS_SIZE + NRF_CLOUD_AGPS_ALMANAC_SIZE + \
	NRF_CLOUD_AGPS_KLOBUCHAR_SIZE + NRF_CLOUD_AGPS_NEQUICK_SIZE + \
	NRF_CLOUD_AGPS_TOWS_SIZE + NRF_CLOUD_AGPS_SYSTEM_TIME_SIZE + \
	NRF_CLOUD_AGPS_LOCATION_SIZE + NRF_CLOUD_AGPS_INTEGRITY_SIZE)

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
	NRF_CLOUD_AGPS__LAST				= NRF_CLOUD_AGPS_INTEGRITY
};

struct nrf_cloud_agps_utc {
	int32_t a1;
	int32_t a0;
	uint8_t tot;
	uint8_t wn_t;
	int8_t delta_tls;
	uint8_t wn_lsf;
	int8_t dn;
	int8_t delta_tlsf;
} __packed;

struct nrf_cloud_agps_ephemeris {
	uint8_t sv_id;
	uint8_t health;
	uint16_t iodc;
	uint16_t toc;
	int8_t af2;
	int16_t af1;
	int32_t af0;
	int8_t tgd;
	uint8_t ura;
	uint8_t fit_int;
	uint16_t toe;
	int32_t w;
	int16_t delta_n;
	int32_t m0;
	int32_t omega_dot;
	uint32_t e;
	int16_t idot;
	uint32_t sqrt_a;
	int32_t i0;
	int32_t omega0;
	int16_t crs;
	int16_t cis;
	int16_t cus;
	int16_t crc;
	int16_t cic;
	int16_t cuc;
} __packed;

struct nrf_cloud_agps_almanac {
	uint8_t sv_id;
	uint8_t wn;
	uint8_t toa;
	uint8_t ioda;
	uint16_t e;
	int16_t delta_i;
	int16_t omega_dot;
	uint8_t sv_health;
	uint32_t sqrt_a;
	int32_t omega0;
	int32_t w;
	int32_t m0;
	int16_t af0;
	int16_t af1;
} __packed;

struct nrf_cloud_agps_klobuchar {
	int8_t alpha0;
	int8_t alpha1;
	int8_t alpha2;
	int8_t alpha3;
	int8_t beta0;
	int8_t beta1;
	int8_t beta2;
	int8_t beta3;
} __packed;

struct nrf_cloud_agps_nequick {
	int16_t ai0;
	int16_t ai1;
	int16_t ai2;
	uint8_t storm_cond;
	uint8_t storm_valid;
} __packed;

struct nrf_cloud_agps_tow_element {
	uint8_t sv_id;
	uint16_t tlm;
	uint8_t flags;
} __packed;

struct nrf_cloud_agps_system_time {
	uint16_t date_day;
	uint32_t time_full_s;
	uint16_t time_frac_ms;
	uint32_t sv_mask;
	struct nrf_cloud_agps_tow_element sv_tow[NRF_CLOUD_AGPS_MAX_SV_TOW];
} __packed;

struct nrf_cloud_agps_location {
	int32_t latitude;
	int32_t longitude;
	int16_t altitude;
	uint8_t unc_semimajor;
	uint8_t unc_semiminor;
	uint8_t orientation_major;
	uint8_t unc_altitude;
	uint8_t confidence;
} __packed;

struct nrf_cloud_agps_integrity {
	uint32_t integrity_mask;
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
