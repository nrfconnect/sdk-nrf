/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <nrf_modem_gnss.h>
#include <nrf_modem_at.h>
#include <cJSON.h>
#include <modem/modem_info.h>
#include <net/nrf_cloud_codec.h>
#include <net/nrf_cloud_defs.h>
#include <net/nrf_cloud_agnss.h>
#if defined(CONFIG_NRF_CLOUD_PGPS)
#include <net/nrf_cloud_pgps.h>
#endif
#include <stdio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(nrf_cloud_agnss, CONFIG_NRF_CLOUD_GPS_LOG_LEVEL);

#include "nrf_cloud_codec_internal.h"
#include "nrf_cloud_transport.h"
#include "nrf_cloud_agnss_schema_v1.h"

extern void agnss_print(enum nrf_cloud_agnss_type type, void *data);

static K_SEM_DEFINE(agnss_injection_active, 1, 1);

static bool agnss_print_enabled;
static struct nrf_modem_gnss_agnss_data_frame processed;
static K_MUTEX_DEFINE(processed_lock);
static atomic_t request_in_progress;

#if defined(CONFIG_NRF_CLOUD_AGNSS_FILTERED)
/** In filtered ephemeris mode, request A-GNSS data no more often than
 *  every ~2 hours (time in milliseconds).  Give it a 10 minute margin
 *  of error so slightly early assistance requests are not ignored.
 */
#define MARGIN_MINUTES 10
#define AGNSS_UPDATE_PERIOD ((120 - MARGIN_MINUTES) * 60 * MSEC_PER_SEC)

static int64_t last_request_timestamp;
#endif

void agnss_print_enable(bool enable)
{
	agnss_print_enabled = enable;
}

bool nrf_cloud_agnss_request_in_progress(void)
{
	return atomic_get(&request_in_progress) != 0;
}

#if IS_ENABLED(CONFIG_NRF_CLOUD_AGNSS)
int nrf_cloud_agnss_request(const struct nrf_modem_gnss_agnss_data_frame *request)
{
	/* GPS data need is always expected to be present and first in list. */
	__ASSERT(request->system_count > 0,
		 "GNSS system data need not found");
	__ASSERT(request->system[0].system_id == NRF_MODEM_GNSS_SYSTEM_GPS,
		 "GPS data need not found");

#if IS_ENABLED(CONFIG_NRF_CLOUD_MQTT)
	if (nfsm_get_current_state() != STATE_DC_CONNECTED) {
		return -EACCES;
	}
	if (!request) {
		return -EINVAL;
	}

	int err;
	cJSON *agnss_req_obj;
	/* Copy the request so that the ephemeris mask can be modified if necessary */
	struct nrf_modem_gnss_agnss_data_frame req = *request;

	atomic_set(&request_in_progress, 0);

	k_mutex_lock(&processed_lock, K_FOREVER);
	memset(&processed, 0, sizeof(processed));
	processed.system_count = 1;
	processed.system[0].system_id = NRF_MODEM_GNSS_SYSTEM_GPS;
	k_mutex_unlock(&processed_lock);

#if defined(CONFIG_NRF_CLOUD_AGNSS_FILTERED)
	/**
	 * Determine if we processed ephemerides assistance less than 2 hours ago; if so,
	 * we can skip this.
	 */
	if (req.system[0].sv_mask_ephe &&
	    (last_request_timestamp != 0) &&
	    ((k_uptime_get() - last_request_timestamp) < AGNSS_UPDATE_PERIOD)) {
		LOG_WRN("A-GNSS request was sent less than 2 hours ago");
		req.system[0].sv_mask_ephe = 0;
	}
#endif

	agnss_req_obj = cJSON_CreateObject();
	err = nrf_cloud_agnss_req_json_encode(&req, agnss_req_obj);
	if (!err) {
		err = json_send_to_cloud(agnss_req_obj);
		if (!err) {
			atomic_set(&request_in_progress, 1);
		}
	} else {
		err = -ENOMEM;
	}

	cJSON_Delete(agnss_req_obj);
	return err;
#else /* IS_ENABLED(CONFIG_NRF_CLOUD_MQTT) */
	LOG_ERR("CONFIG_NRF_CLOUD_MQTT must be enabled in order to use this API");
	return -ENOTSUP;
#endif
}

static bool qzss_assistance_is_supported(void)
{
	char resp[32];

	if (nrf_modem_at_cmd(resp, sizeof(resp), "AT+CGMM") == 0) {
		/* nRF9160 does not support QZSS assistance, while nRF91x1 do. */
		if (strstr(resp, "nRF9160") != NULL) {
			return false;
		}
	}

	return true;
}

int nrf_cloud_agnss_request_all(void)
{
	const uint32_t mask = IS_ENABLED(CONFIG_NRF_CLOUD_PGPS) ? 0u : 0xFFFFFFFFu;
	struct nrf_modem_gnss_agnss_data_frame request = {
		.data_flags =
			NRF_MODEM_GNSS_AGNSS_GPS_UTC_REQUEST |
			NRF_MODEM_GNSS_AGNSS_KLOBUCHAR_REQUEST |
			NRF_MODEM_GNSS_AGNSS_NEQUICK_REQUEST |
			NRF_MODEM_GNSS_AGNSS_GPS_SYS_TIME_AND_SV_TOW_REQUEST |
			NRF_MODEM_GNSS_AGNSS_POSITION_REQUEST |
			NRF_MODEM_GNSS_AGNSS_INTEGRITY_REQUEST,
		.system_count = 1,
		.system[0].system_id = NRF_MODEM_GNSS_SYSTEM_GPS,
		.system[0].sv_mask_ephe = mask,
		.system[0].sv_mask_alm = mask
	};

	if (qzss_assistance_is_supported()) {
		request.system_count = 2;
		request.system[1].system_id = NRF_MODEM_GNSS_SYSTEM_QZSS;
		request.system[1].sv_mask_ephe = 0x3ff;
		request.system[1].sv_mask_alm = 0x3ff;
	}

	return nrf_cloud_agnss_request(&request);
}
#endif /* CONFIG_NRF_CLOUD_AGNSS */

static int send_to_modem(void *data, size_t data_len, uint16_t type)
{
	if (agnss_print_enabled) {
		agnss_print(type, data);
	}

	return nrf_modem_gnss_agnss_write(data, data_len, type);
}

static void copy_gps_utc(struct nrf_modem_gnss_agnss_gps_data_utc *dst,
		     struct nrf_cloud_agnss_element *src)
{
	__ASSERT_NO_MSG(dst != NULL);
	__ASSERT_NO_MSG(src != NULL);

	dst->a1		= src->utc->a1;
	dst->a0		= src->utc->a0;
	dst->tot	= src->utc->tot;
	dst->wn_t	= src->utc->wn_t;
	dst->delta_tls	= src->utc->delta_tls;
	dst->wn_lsf	= src->utc->wn_lsf;
	dst->dn		= src->utc->dn;
	dst->delta_tlsf	= src->utc->delta_tlsf;
}

static void copy_almanac(struct nrf_modem_gnss_agnss_gps_data_almanac *dst,
			 struct nrf_cloud_agnss_element *src)
{
	__ASSERT_NO_MSG(dst != NULL);
	__ASSERT_NO_MSG(src != NULL);

	dst->sv_id	= src->almanac->sv_id;
	dst->wn		= src->almanac->wn;
	dst->toa	= src->almanac->toa;
	dst->ioda	= src->almanac->ioda;
	dst->e		= src->almanac->e;
	dst->delta_i	= src->almanac->delta_i;
	dst->omega_dot	= src->almanac->omega_dot;
	dst->sv_health	= src->almanac->sv_health;
	dst->sqrt_a	= src->almanac->sqrt_a;
	dst->omega0	= src->almanac->omega0;
	dst->w		= src->almanac->w;
	dst->m0		= src->almanac->m0;
	dst->af0	= src->almanac->af0;
	dst->af1	= src->almanac->af1;
}

static void copy_ephemeris(struct nrf_modem_gnss_agnss_gps_data_ephemeris *dst,
			   struct nrf_cloud_agnss_element *src)
{
	__ASSERT_NO_MSG(dst != NULL);
	__ASSERT_NO_MSG(src != NULL);

	dst->sv_id	= src->ephemeris->sv_id;
	dst->health	= src->ephemeris->health;
	dst->iodc	= src->ephemeris->iodc;
	dst->toc	= src->ephemeris->toc;
	dst->af2	= src->ephemeris->af2;
	dst->af1	= src->ephemeris->af1;
	dst->af0	= src->ephemeris->af0;
	dst->tgd	= src->ephemeris->tgd;
	dst->ura	= src->ephemeris->ura;
	dst->fit_int	= src->ephemeris->fit_int;
	dst->toe	= src->ephemeris->toe;
	dst->w		= src->ephemeris->w;
	dst->delta_n	= src->ephemeris->delta_n;
	dst->m0		= src->ephemeris->m0;
	dst->omega_dot	= src->ephemeris->omega_dot;
	dst->e		= src->ephemeris->e;
	dst->idot	= src->ephemeris->idot;
	dst->sqrt_a	= src->ephemeris->sqrt_a;
	dst->i0		= src->ephemeris->i0;
	dst->omega0	= src->ephemeris->omega0;
	dst->crs	= src->ephemeris->crs;
	dst->cis	= src->ephemeris->cis;
	dst->cus	= src->ephemeris->cus;
	dst->crc	= src->ephemeris->crc;
	dst->cic	= src->ephemeris->cic;
	dst->cuc	= src->ephemeris->cuc;
}

static void copy_klobuchar(struct nrf_modem_gnss_agnss_data_klobuchar *dst,
			   struct nrf_cloud_agnss_element *src)
{
	__ASSERT_NO_MSG(dst != NULL);
	__ASSERT_NO_MSG(src != NULL);

	dst->alpha0	= src->ion_correction.klobuchar->alpha0;
	dst->alpha1	= src->ion_correction.klobuchar->alpha1;
	dst->alpha2	= src->ion_correction.klobuchar->alpha2;
	dst->alpha3	= src->ion_correction.klobuchar->alpha3;
	dst->beta0	= src->ion_correction.klobuchar->beta0;
	dst->beta1	= src->ion_correction.klobuchar->beta1;
	dst->beta2	= src->ion_correction.klobuchar->beta2;
	dst->beta3	= src->ion_correction.klobuchar->beta3;
}

static void copy_nequick(struct nrf_modem_gnss_agnss_data_nequick *dst,
			 struct nrf_cloud_agnss_element *src)
{
	__ASSERT_NO_MSG(dst != NULL);
	__ASSERT_NO_MSG(src != NULL);

	dst->ai0 = src->ion_correction.nequick->ai0;
	dst->ai1 = src->ion_correction.nequick->ai1;
	dst->ai2 = src->ion_correction.nequick->ai2;
	dst->storm_cond = src->ion_correction.nequick->storm_cond;
	dst->storm_valid = src->ion_correction.nequick->storm_valid;
}

static void copy_location(struct nrf_modem_gnss_agnss_data_location *dst,
			  struct nrf_cloud_agnss_element *src)
{
	__ASSERT_NO_MSG(dst != NULL);
	__ASSERT_NO_MSG(src != NULL);

	dst->latitude		= src->location->latitude;
	dst->longitude		= src->location->longitude;
	dst->altitude		= src->location->altitude;
	dst->unc_semimajor	= src->location->unc_semimajor;
	dst->unc_semiminor	= src->location->unc_semiminor;
	dst->orientation_major	= src->location->orientation_major;
	dst->unc_altitude	= src->location->unc_altitude;
	dst->confidence		= src->location->confidence;
}

static void copy_time_and_tow(struct nrf_modem_gnss_agnss_gps_data_system_time_and_sv_tow *dst,
			      struct nrf_cloud_agnss_element *src)
{
	__ASSERT_NO_MSG(dst != NULL);
	__ASSERT_NO_MSG(src != NULL);

	dst->date_day		= src->time_and_tow->date_day;
	dst->time_full_s	= src->time_and_tow->time_full_s;
	dst->time_frac_ms	= src->time_and_tow->time_frac_ms;
	dst->sv_mask		= src->time_and_tow->sv_mask;

	if (src->time_and_tow->sv_mask == 0U) {
		LOG_DBG("SW TOW mask is zero, not copying TOW array");
		memset(dst->sv_tow, 0, sizeof(dst->sv_tow));

		return;
	}

	for (size_t i = 0; i < NRF_CLOUD_AGNSS_MAX_SV_TOW; i++) {
		dst->sv_tow[i].flags = src->time_and_tow->sv_tow[i].flags;
		dst->sv_tow[i].tlm = src->time_and_tow->sv_tow[i].tlm;
	}
}

static void copy_integrity_gps(struct nrf_modem_gnss_agps_data_integrity *dst,
			       struct nrf_cloud_agnss_element *src)
{
	__ASSERT_NO_MSG(dst != NULL);
	__ASSERT_NO_MSG(src != NULL);

	dst->integrity_mask = src->integrity->integrity_mask;
}

static void copy_integrity_qzss(struct nrf_modem_gnss_agnss_data_integrity *dst,
				struct nrf_cloud_agnss_element *src)
{
	__ASSERT_NO_MSG(dst != NULL);
	__ASSERT_NO_MSG(src != NULL);

	dst->signal_count = 1;
	dst->signal[0].signal_id = NRF_MODEM_GNSS_SIGNAL_QZSS_L1_CA;
	dst->signal[0].integrity_mask = src->integrity->integrity_mask;
}

static int agnss_send_to_modem(struct nrf_cloud_agnss_element *agnss_data)
{
	atomic_set(&request_in_progress, 0);

	switch (agnss_data->type) {
	case NRF_CLOUD_AGNSS_GPS_UTC_PARAMETERS: {
		struct nrf_modem_gnss_agnss_gps_data_utc gps_utc;

		processed.data_flags |= NRF_MODEM_GNSS_AGNSS_GPS_UTC_REQUEST;
		copy_gps_utc(&gps_utc, agnss_data);
#if defined(CONFIG_NRF_CLOUD_PGPS)
		nrf_cloud_pgps_set_leap_seconds(gps_utc.delta_tls);
#endif
		LOG_DBG("A-GNSS type: NRF_CLOUD_AGNSS_UTC_PARAMETERS");

		return send_to_modem(&gps_utc, sizeof(gps_utc),
				     NRF_MODEM_GNSS_AGNSS_GPS_UTC_PARAMETERS);
	}
	case NRF_CLOUD_AGNSS_GPS_EPHEMERIDES:
	case NRF_CLOUD_AGNSS_QZSS_EPHEMERIDES: {
		struct nrf_modem_gnss_agnss_gps_data_ephemeris ephemeris;

		if (agnss_data->type == NRF_CLOUD_AGNSS_GPS_EPHEMERIDES) {
			processed.system[0].sv_mask_ephe |=
				(1 << (agnss_data->ephemeris->sv_id - 1));
#if defined(CONFIG_NRF_CLOUD_PGPS)
			if (agnss_data->ephemeris->health ==
			    NRF_CLOUD_PGPS_EMPTY_EPHEM_HEALTH) {
				LOG_DBG("Skipping empty ephemeris for sv %u",
					agnss_data->ephemeris->sv_id);
				return 0;
			}
#endif
		}

		copy_ephemeris(&ephemeris, agnss_data);
		LOG_DBG("A-GNSS type: %s EPHEMERIDES %d",
			(agnss_data->type == NRF_CLOUD_AGNSS_GPS_EPHEMERIDES) ? "GPS" : "QZSS",
			agnss_data->ephemeris->sv_id);

		return send_to_modem(&ephemeris, sizeof(ephemeris),
				     NRF_MODEM_GNSS_AGNSS_GPS_EPHEMERIDES);
	}
	case NRF_CLOUD_AGNSS_GPS_ALMANAC:
	case NRF_CLOUD_AGNSS_QZSS_ALMANAC: {
		struct nrf_modem_gnss_agnss_gps_data_almanac almanac;

		if (agnss_data->type == NRF_CLOUD_AGNSS_GPS_ALMANAC) {
			processed.system[0].sv_mask_alm |= (1 << (agnss_data->almanac->sv_id - 1));
		}

		copy_almanac(&almanac, agnss_data);
		LOG_DBG("A-GNSS type: %s ALMANAC %d",
			(agnss_data->type == NRF_CLOUD_AGNSS_GPS_ALMANAC) ? "GPS" : "QZSS",
			agnss_data->almanac->sv_id);

		return send_to_modem(&almanac, sizeof(almanac),
				     NRF_MODEM_GNSS_AGNSS_GPS_ALMANAC);
	}
	case NRF_CLOUD_AGNSS_KLOBUCHAR_CORRECTION: {
		struct nrf_modem_gnss_agnss_data_klobuchar klobuchar;

		processed.data_flags |= NRF_MODEM_GNSS_AGNSS_KLOBUCHAR_REQUEST;
		copy_klobuchar(&klobuchar, agnss_data);
		LOG_DBG("A-GNSS type: NRF_CLOUD_AGNSS_KLOBUCHAR_CORRECTION");

		return send_to_modem(&klobuchar, sizeof(klobuchar),
				     NRF_MODEM_GNSS_AGNSS_KLOBUCHAR_IONOSPHERIC_CORRECTION);
	}
	case NRF_CLOUD_AGNSS_NEQUICK_CORRECTION: {
		struct nrf_modem_gnss_agnss_data_nequick nequick;

		processed.data_flags |= NRF_MODEM_GNSS_AGNSS_NEQUICK_REQUEST;
		copy_nequick(&nequick, agnss_data);
		LOG_DBG("A-GNSS type: NRF_CLOUD_AGNSS_NEQUICK_CORRECTION");

		return send_to_modem(&nequick, sizeof(nequick),
				     NRF_MODEM_GNSS_AGNSS_NEQUICK_IONOSPHERIC_CORRECTION);
	}
	case NRF_CLOUD_AGNSS_GPS_SYSTEM_CLOCK: {
		struct nrf_modem_gnss_agnss_gps_data_system_time_and_sv_tow time_and_tow;

		processed.data_flags |= NRF_MODEM_GNSS_AGNSS_GPS_SYS_TIME_AND_SV_TOW_REQUEST;
		copy_time_and_tow(&time_and_tow, agnss_data);
		LOG_DBG("A-GNSS type: NRF_CLOUD_AGNSS_GPS_SYSTEM_CLOCK");

		return send_to_modem(&time_and_tow, sizeof(time_and_tow),
				     NRF_MODEM_GNSS_AGNSS_GPS_SYSTEM_CLOCK_AND_TOWS);
	}
	case NRF_CLOUD_AGNSS_LOCATION: {
		struct nrf_modem_gnss_agnss_data_location location = {0};

		processed.data_flags |= NRF_MODEM_GNSS_AGNSS_POSITION_REQUEST;
		copy_location(&location, agnss_data);
#if defined(CONFIG_NRF_CLOUD_PGPS)
		nrf_cloud_pgps_set_location_normalized(location.latitude,
						  location.longitude);
#endif
		LOG_DBG("A-GNSS type: NRF_CLOUD_AGNSS_LOCATION");

		return send_to_modem(&location, sizeof(location),
				     NRF_MODEM_GNSS_AGNSS_LOCATION);
	}
	case NRF_CLOUD_AGNSS_GPS_INTEGRITY: {
		struct nrf_modem_gnss_agps_data_integrity integrity = {0};

		processed.data_flags |= NRF_MODEM_GNSS_AGNSS_INTEGRITY_REQUEST;
		copy_integrity_gps(&integrity, agnss_data);
		LOG_DBG("A-GNSS type: NRF_CLOUD_AGNSS_INTEGRITY");

		return send_to_modem(&integrity, sizeof(integrity),
				     NRF_MODEM_GNSS_AGPS_INTEGRITY);
	}
	case NRF_CLOUD_AGNSS_QZSS_INTEGRITY: {
		struct nrf_modem_gnss_agnss_data_integrity integrity = {0};

		processed.data_flags |= NRF_MODEM_GNSS_AGNSS_INTEGRITY_REQUEST;
		copy_integrity_qzss(&integrity, agnss_data);
		LOG_DBG("A-GNSS type: NRF_CLOUD_AGNSS_QZSS_INTEGRITY");

		return send_to_modem(&integrity, sizeof(integrity),
				     NRF_MODEM_GNSS_AGNSS_INTEGRITY);
	}
	default:
		LOG_WRN("Unknown AGNSS data type: %d", agnss_data->type);
		break;
	}

	return 0;
}

static size_t get_next_agnss_element(struct nrf_cloud_agnss_element *element,
				    const char *buf,
				    size_t buf_len)
{
	static uint16_t elements_left_to_process;
	static enum nrf_cloud_agnss_type element_type;
	size_t len = 0;

	/* Check if there are more elements left in the array to process.
	 * The element type is only given once before the array, and not for
	 * each element.
	 */
	if (elements_left_to_process == 0) {
		/* Check that there's enough data for type and count. */
		if (buf_len < NRF_CLOUD_AGNSS_BIN_TYPE_SIZE + NRF_CLOUD_AGNSS_BIN_COUNT_SIZE) {
			LOG_ERR("Unexpected end of data");
			return 0;
		}

		element->type =
			(enum nrf_cloud_agnss_type)buf[NRF_CLOUD_AGNSS_BIN_TYPE_OFFSET];
		element_type = element->type;
		elements_left_to_process =
			*(uint16_t *)&buf[NRF_CLOUD_AGNSS_BIN_COUNT_OFFSET] - 1;
		len += NRF_CLOUD_AGNSS_BIN_TYPE_SIZE +
			NRF_CLOUD_AGNSS_BIN_COUNT_SIZE;
	} else {
		element->type = element_type;
		elements_left_to_process -= 1;
	}

	switch (element->type) {
	case NRF_CLOUD_AGNSS_GPS_UTC_PARAMETERS:
		element->utc = (struct nrf_cloud_agnss_utc *)(buf + len);
		len += sizeof(struct nrf_cloud_agnss_utc);
		break;
	case NRF_CLOUD_AGNSS_GPS_EPHEMERIDES:
	case NRF_CLOUD_AGNSS_QZSS_EPHEMERIDES:
		element->ephemeris = (struct nrf_cloud_agnss_ephemeris *)(buf + len);
		len += sizeof(struct nrf_cloud_agnss_ephemeris);
		break;
	case NRF_CLOUD_AGNSS_GPS_ALMANAC:
	case NRF_CLOUD_AGNSS_QZSS_ALMANAC:
		element->almanac = (struct nrf_cloud_agnss_almanac *)(buf + len);
		len += sizeof(struct nrf_cloud_agnss_almanac);
		break;
	case NRF_CLOUD_AGNSS_KLOBUCHAR_CORRECTION:
		element->ion_correction.klobuchar =
			(struct nrf_cloud_agnss_klobuchar *)(buf + len);
		len += sizeof(struct nrf_cloud_agnss_klobuchar);
		break;
	case NRF_CLOUD_AGNSS_NEQUICK_CORRECTION:
		element->ion_correction.nequick =
			(struct nrf_cloud_agnss_nequick *)(buf + len);
		len += sizeof(struct nrf_cloud_agnss_nequick);
		break;
	case NRF_CLOUD_AGNSS_GPS_SYSTEM_CLOCK:
		element->time_and_tow =
			(struct nrf_cloud_agnss_system_time *)(buf + len);
		len += sizeof(struct nrf_cloud_agnss_system_time) -
			sizeof(element->time_and_tow->sv_tow) + 4;
		break;
	case NRF_CLOUD_AGNSS_GPS_TOWS:
		element->tow =
			(struct nrf_cloud_agnss_tow_element *)(buf + len);
		len += sizeof(struct nrf_cloud_agnss_tow_element);
		break;
	case NRF_CLOUD_AGNSS_LOCATION:
		element->location = (struct nrf_cloud_agnss_location *)(buf + len);
		len += sizeof(struct nrf_cloud_agnss_location);
		break;
	case NRF_CLOUD_AGNSS_GPS_INTEGRITY:
	case NRF_CLOUD_AGNSS_QZSS_INTEGRITY:
		element->integrity =
			(struct nrf_cloud_agnss_integrity *)(buf + len);
		len += sizeof(struct nrf_cloud_agnss_integrity);
		break;
	default:
		LOG_DBG("Unhandled A-GNSS data type: %d", element->type);
		elements_left_to_process = 0;
		return 0;
	}

	/* Check that there's enough data for the element. */
	if (buf_len < len) {
		LOG_ERR("Unexpected end of data");
		elements_left_to_process = 0;
		return 0;
	}

	return len;
}

int nrf_cloud_agnss_process(const char *buf, size_t buf_len)
{
	int err;
	struct nrf_cloud_agnss_element element = {0};
	struct nrf_cloud_agnss_system_time sys_time = {0};
	uint32_t sv_mask = 0;
	size_t parsed_len = 0;
	uint8_t version;
#if defined(CONFIG_NRF_CLOUD_AGNSS_FILTERED)
	bool ephemerides_processed = false;
#endif

	if (!buf || (buf_len == 0)) {
		return -EINVAL;
	}

	/* Check for a potential A-GNSS JSON error message from nRF Cloud */
	enum nrf_cloud_error nrf_err;

	err = nrf_cloud_error_msg_decode(buf, NRF_CLOUD_JSON_APPID_VAL_AGNSS,
		NRF_CLOUD_JSON_MSG_TYPE_VAL_DATA, &nrf_err);
	if (!err) {
		LOG_ERR("nRF Cloud returned A-GNSS error: %d", nrf_err);
		return -EFAULT;
	} else if (err == -ENODATA) { /* Not a JSON message, try to parse it as A-GNSS data */

	} else { /* JSON message received but no valid error code found */
		return -ENOMSG;
	}

	version = buf[NRF_CLOUD_AGNSS_BIN_SCHEMA_VERSION_INDEX];
	parsed_len += NRF_CLOUD_AGNSS_BIN_SCHEMA_VERSION_SIZE;

	if (version != NRF_CLOUD_AGNSS_BIN_SCHEMA_VERSION) {
		LOG_ERR("Cannot parse schema version: %d", version);
		return -EBADMSG;
	}

	LOG_DBG("Received AGNSS data. Schema version: %d, length: %d",
		version, buf_len);

	err = k_sem_take(&agnss_injection_active, K_FOREVER);
	if (err) {
		LOG_ERR("A-GNSS injection already active.");
		return err;
	}

	LOG_DBG("A-GNSS_injection_active LOCKED");

	while (parsed_len < buf_len) {
		size_t element_size =
			get_next_agnss_element(&element, &buf[parsed_len], buf_len - parsed_len);

		if (element_size == 0) {
			LOG_DBG("Parsing finished\n");
			break;
		}

		parsed_len += element_size;

		LOG_DBG("Parsed_len: %d", parsed_len);

		if (element.type == NRF_CLOUD_AGNSS_GPS_TOWS) {
			memcpy(&sys_time.sv_tow[element.tow->sv_id - 1],
				element.tow,
				sizeof(sys_time.sv_tow[0]));
			if (element.tow->flags || element.tow->tlm) {
				sv_mask |= 1 << (element.tow->sv_id - 1);
			}

			LOG_DBG("TOW %d copied", element.tow->sv_id - 1);

			continue;
		} else if (element.type == NRF_CLOUD_AGNSS_GPS_SYSTEM_CLOCK) {
			memcpy(&sys_time, element.time_and_tow,
				sizeof(sys_time) - sizeof(sys_time.sv_tow));
			sys_time.sv_mask = sv_mask | element.time_and_tow->sv_mask;
			LOG_DBG("TOWs copied, bitmask: 0x%08x",
				sys_time.sv_mask);
			element.time_and_tow = &sys_time;
#if defined(CONFIG_NRF_CLOUD_AGNSS_FILTERED)
		} else if (element.type == NRF_CLOUD_AGNSS_GPS_EPHEMERIDES) {
			ephemerides_processed = true;
#endif
		}

		/* The processed variable is read/written by agnss_send_to_modem() and
		 * nrf_cloud_agnss_processed() which can be called from different contexts.
		 */
		k_mutex_lock(&processed_lock, K_FOREVER);
		err = agnss_send_to_modem(&element);
		k_mutex_unlock(&processed_lock);
		if (err) {
			LOG_WRN("Failed to send data to modem, error: %d", err);
		}
	}

#if defined(CONFIG_NRF_CLOUD_AGNSS_FILTERED)
	/**
	 * In filtered mode, because fewer than the full set of ephemerides is sent to
	 * the modem, determine here if we correctly received them from the cloud and
	 * sent them to the modem.
	 */
	if (!err && ephemerides_processed) {
		last_request_timestamp = k_uptime_get();
	}
#endif

	LOG_DBG("A-GNSS_inject_active UNLOCKED");
	k_sem_give(&agnss_injection_active);

	return err;
}

void nrf_cloud_agnss_processed(struct nrf_modem_gnss_agnss_data_frame *received_elements)
{
	if (received_elements) {
		k_mutex_lock(&processed_lock, K_FOREVER);
		memcpy(received_elements, &processed, sizeof(*received_elements));
		k_mutex_unlock(&processed_lock);
	}
}
