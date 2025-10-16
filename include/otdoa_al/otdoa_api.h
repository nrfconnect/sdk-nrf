/*
 * Copyright (c) 2025 PHY Wireless, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-PHYW
 */

#ifndef PHYWI_OTDOA_API__
#define PHYWI_OTDOA_API__

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file otdoa_api.h
 *
 * @defgroup phywi_otdoa_api hellaPHY OTDOA module
 * @{
 * @brief API Definitions for the hellaPHY OTDOA module.
 */

#define OTDOA_API_MAX_CELLS	  75
#define OTDOA_API_MAX_SESSION_LEN 250

#ifdef HOST
#define LOG2_COMPRESS_WINDOW                                                                       \
	15 /* use large decomp. window on HOST so we can decompress files compressed with gzip */
#else
#define LOG2_COMPRESS_WINDOW                                                                       \
	9 /* Log2 of compression window size (i.e. 10=>1024B compression window) */
#endif

/**
 * @brief Maximum length of the uBSA file path
 */
#define OTDOA_API_MAX_UBSA_FILE_PATH 50

typedef enum {
	/** Success */
	OTDOA_API_SUCCESS = 0,

	/** Parameter error in API */
	OTDOA_API_ERROR_PARAM = -1,

	/** Internal error in OTDOA library. */
	OTDOA_API_INTERNAL_ERROR = -2,

	/** Failure occurred in the uBSA download */
	OTDOA_EVENT_FAIL_UBSA_DL = 1,

	/** Network failure occurred during OTDOA processing */
	OTDOA_EVENT_FAIL_NO_CELL = 2,

	/** Network connection issue */
	OTDOA_EVENT_FAIL_NTWK_CONN = 3,

	/** PRS session cancelled by application */
	OTDOA_EVENT_FAIL_CANCELLED = 4,

	/** Other failure occurred in OTDOA processing */
	OTDOA_EVENT_FAIL_OTDOA_PROC = 5,

	/** Session timeout occurred during OTDOA processing or uBSA DL */
	OTDOA_EVENT_FAIL_TIMEOUT = 6,

	/** Error occurred in nrf modem RS capture API */
	OTDOA_EVENT_FAIL_NRF_RS_CAPTURE = 7,

	/** Error in response from Modem */
	OTDOA_EVENT_FAIL_BAD_MODEM_RESP = 8,

	/** Modem is not registered to LTE network */
	OTDOA_EVENT_FAIL_NOT_REGISTERED = 9,

	/** Modem has stopped, a higher prority activity stopped the session. */
	OTDOA_EVENT_FAIL_STOPPED = 10,

	/** Failed to get DLEARFCN from Modem */
	OTDOA_EVENT_FAIL_NO_DLEARFCN = 11,

	/** System Mode is not LTE (e.g. it may be NBIoT mode) */
	OTDOA_EVENT_FAIL_NOT_LTE_MODE = 12,

	/** Failure to parse the uBSA file */
	OTDOA_EVENT_FAIL_UBSA_PARSING = 13,

	/** Bad Config file */
	OTDOA_EVENT_FAIL_BAD_CFG = 14,

	/** Operation is not permitted */
	OTDOA_EVENT_FAIL_UNAUTHORIZED = 15,

	/** The requested ECGI has been blacklisted */
	OTDOA_EVENT_FAIL_BLACKLISTED = 16,

	/** Failed to get PCI from Modem */
	OTDOA_EVENT_FAIL_NO_PCI = 17,

	/** The uBSA is still being generated */
	OTDOA_EVENT_HTTP_NOT_READY = 202,

	/** The uBSA is only partially downloaded */
	OTDOA_EVENT_HTTP_PARTIAL_CONTENT = 206,

	/** Request parameters malformed or invalid */
	OTDOA_EVENT_FAIL_HTTP_BAD_REQUEST = 400,

	/** Unable to validate JWT */
	OTDOA_EVENT_FAIL_HTTP_UNAUTHORIZED = 401,

	/** Requested resource not found */
	OTDOA_EVENT_FAIL_HTTP_NOT_FOUND = 404,

	/** User already has a pending uBSA */
	OTDOA_EVENT_FAIL_HTTP_CONFLICT = 409,

	/** uBSA generation was not possible */
	OTDOA_EVENT_FAIL_HTTP_UNPROCESSABLE_CONTENT = 422,

	/** Requested too many uBSAs */
	OTDOA_EVENT_FAIL_HTTP_TOO_MANY_REQUESTS = 429,

	/** Server error while generating uBSA */
	OTDOA_EVENT_FAIL_HTTP_INTERNAL_SERVER_ERROR = 500,
} otdoa_api_error_codes_t;

/** @brief OTDOA Session Parameters
 * These parameters are sent to the OTDOA library as part of
 * the @ref otdoa_api_start_session() API call.
 */
typedef struct {
	/** session length in PRS occasions */
	uint32_t session_length;

	/** Test/Debug flags */
	uint32_t capture_flags;

	/** session timeout in msec. */
	uint32_t timeout;

} otdoa_api_session_params_t;

/**
 * @brief Details of the OTDOA results
 * This structure is returned along with the position
 * estimate as a part of the OTDOA_EVENT_RESULTS event.
 */
typedef struct {

	/** ECGI of the serving cell during the OTDOA processing*/
	uint32_t serving_cell_ecgi;

	/** RSSI of the serving cell, in dBm */
	int32_t serving_rssi_dbm;

	/** DLEARFCN of serving cell */
	uint32_t dlearfcn;

	/** Text description of the algorithm used (OTDOA, eCID)*/
	char estimate_algorithm[30];

	/** The length of the PRS session in PRS occasions */
	uint32_t session_length;

	/** Number of cells that we measured in the OTDOA process */
	uint32_t num_measured_cells;

	/** Array containing the ECGIs of the measured cells
	 *   only the first num_measured_cells entries in this array are valid
	 */
	uint32_t ecgi_list[OTDOA_API_MAX_CELLS];

	/**
	 * Array containing the number of times each cell has its TOA successfully detected
	 * only the first num_measured_cells entries in this array are valid
	 */
	uint16_t toa_detect_count[OTDOA_API_MAX_CELLS];

} otdoa_api_result_details_t;

/**
 * @brief OTDOA Results
 * This structure contains the results of the OTDOA position estimation
 * It is returned as a part of the OTDOA_EVENT_RESULTS event.
 */
typedef struct {
	/** Geodetic latitude (deg) in WGS-84. */
	double latitude;
	/** Geodetic longitude (deg) in WGS-84. */
	double longitude;

	/** Location accuracy estimate in ./otdoa_lib/src/top/meters. */
	float accuracy;

	/** Details of the OTDOA estimated */
	otdoa_api_result_details_t details;
} otdoa_api_results_t;

/**
 * @brief Parameters of the uBSA download request.
 * This structure is sent along with the @ref OTDOA_EVENT_UBSA_DL_REQ event.
 * It is also returned to the OTDOA library in the @ref otdoa_api_ubsa_download()
 * API call.
 */
typedef struct {
	/** PLMN ID.
	 *
	 * Bytes encoded as follows:
	 *     mcc2_mcc1, mnc3_mcc3, mnc2_mnc1
	 */
	uint8_t plmn[3];
	/** 28bit cell id. */
	uint32_t ecgi;
	/** Mobile Country Code. Range: 0...999. */
	uint16_t mcc;
	/** Mobile Network Code. Range: 0...999. */
	uint16_t mnc;
	/** Cell PCI. Range: 0...503. */
	uint16_t pci;

	/** Cell DLEARFCN */
	uint32_t dlearfcn;

	/** Maximum number of cells in the uBSA */
	uint32_t max_cells;

	/** Maximum uBSA radius in meters */
	uint32_t ubsa_radius_meters;

} otdoa_api_ubsa_dl_req_t;

typedef struct {
	/** Status of the uBSA download */
	otdoa_api_error_codes_t status;
} otdoa_api_ubsa_dl_compl_t;

typedef struct {
	/** Status of the results upload*/
	otdoa_api_error_codes_t status;
} otdoa_api_ubsa_ul_compl_t;

/** @brief enum defining the type of event retured by the OTDOA library */
enum otdoa_api_event_id {
	/** Position estimate results from OTDOA */
	OTDOA_EVENT_RESULTS = 1,

	/** OTDOA library requests download of a uBSA */
	OTDOA_EVENT_UBSA_DL_REQ,

	/** OTDOA library indicates download of uBSA is complete */
	OTDOA_EVENT_UBSA_DL_COMPL,

	/** OTDOA Library indicates completion of results upload */
	OTDOA_EVENT_RESULTS_UL_COMPL,

	/** Failure */
	OTDOA_EVENT_FAIL
};

typedef struct {
	enum otdoa_api_event_id event;

	/** Event-specific data */
	union {
		/** OTDOA_EVENT_RESULTS event details */
		otdoa_api_results_t results;

		/** OTDOA_EVENT_UBSA_DL_REQ event details */
		otdoa_api_ubsa_dl_req_t dl_request;

		/* OTDOA_EVENT_UBSA_DL_COMPL event details */
		otdoa_api_ubsa_dl_compl_t dl_compl;

		/* OTDOA_EVENT_UBSA_UL_COMPL event details */
		otdoa_api_ubsa_ul_compl_t ul_compl;

		/** OTDOA_EVENT_FAIL event details*/
		int32_t failure_code;
	};
} otdoa_api_event_data_t;

/**
 * @brief OTDOA event callback prototype
 *
 * @param[in] event_data Event Data
 */
typedef void (*otdoa_api_callback_t)(const otdoa_api_event_data_t *event_data);

/**
 * @brief Initialize the OTDOA library
 * @param[in] ubsa_file_path Points to a string containing the full path to where
 *                           the uBSA file resides
 * @param[in] callback Callback function used by the library
 *                     to return results and status to the client
 * @return 0 on success
 */
int32_t otdoa_api_init(const char *const ubsa_file_path, otdoa_api_callback_t callback);

/**
 * @brief Initiates an OTDOA positioning session
 * @param[in] params Parameters of the session, including
 *                   session length, timeout, etc.
 * @retval 0 on success
 */
int32_t otdoa_api_start_session(const otdoa_api_session_params_t *params);

/**
 * @brief Cancels an on-going OTDOA session
 * @return 0 on success
 */
int32_t otdoa_api_cancel_session(void);

/**
 * @brief Requests that the OTDOA library initiate download of a new uBSA file.
 * @param[in] dl_request Structure containing the parameters of the requested uBSA
 * @param[in] reset_blacklist If OTDOA should reset the list of blocked ECGIs
 * @retval Error codes as defined in otdoa_api_error_codes_t
 */
int32_t otdoa_api_ubsa_download(const otdoa_api_ubsa_dl_req_t *dl_request,
				bool reset_blacklist);

/**
 * @brief Requests that the OTDOA library initiate download of a new configuration file.
 *
 * @retval Error codes as defined in otdoa_api_error_codes_t
 */
int otdoa_api_cfg_download(void);

/*
 * @brief Indicates that an updated uBSA is available to the OTDOA library
 * @param[in] status A non-zero value indicates failure to update the uBSA.
 * @retval Error codes as defined in otdoa_api_error_codes_t
 */
int32_t otdoa_api_ubsa_available(int32_t status);

/**
 * @brief Upload the OTDOA results to the PhyWi Server
 * @param[in] p_results Points to a structure containing the
 *                      postion estimate results
 * @param true_lat[in]  Points to a string containing the "ground truth" latitude
 * @param true_lon[in]  Points to a string containing the "ground truth" longitude
 * @param notes[in]     Points to a string containing notes included with the upload
 *
 * @retval Error codes as defined in otdoa_api_error_codes_t
 *
 * @note Any of true_lat, true_lon and notes may be NULL.
 */
#define OTDOA_NOTES_MAX 81 /* Maximum size of the notes string uploaded with results */

int32_t otdoa_api_upload_results(const otdoa_api_results_t *p_results, const char *true_lat,
				 const char *true_lon, const char *notes);

/**
 * @brief Installs a TLS certificate to use when connecting to OTDOA servers
 *
 * @param[in] tls_cert String containing PEM data
 * @param[in] cert_len Length of PEM data
 * @retval 0 on success
 *        -1 on failure
 *
 * @note Kconfig option OTDOA_API_TLS_CERT_INSTALL must be enabled to use this function
 */
int otdoa_api_install_tls_cert(const char *tls_cert, size_t cert_len);

/**
 * @brief Provisions a private key for authenticating with OTDOA servers
 *
 * @retval 0 on success
 *        -1 on failure
 */
int otdoa_api_provision(const char *key_string);

/**
 * @brief Returns a constant string containing the OTDOA library version
 */
const char *const otdoa_api_get_version(void);

/**
 * @brief Returns a constant string containing a shortened form of the OTDOA library version,
 * without any whitespace or special characters. Suitable for use in HTTP parameters.
 */
const char *const otdoa_api_get_short_version(void);

/**
 * @brief Sets the path used for the config file
 */
void otdoa_api_cfg_set_file_path(const char *psz_path);

/**
 * @brief Gets the path used for the config file
 */
const char *otdoa_api_cfg_get_file_path(void);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* PHYWI_OTDOA_API__ */
