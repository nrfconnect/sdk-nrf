/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef PDN_H_
#define PDN_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file pdn.h
 * @brief Public APIs for the PDN library.
 * @defgroup pdn PDN library
 * @{
 */

/** @brief PDN family */
enum pdn_fam {
	PDN_FAM_IPV4   = 0, /**< IPv4 family */
	PDN_FAM_IPV6   = 1, /**< IPv6 family */
	PDN_FAM_IPV4V6 = 2, /**< IPv4 and IPv6 family */
	PDN_FAM_NONIP  = 3, /**< Non-IP family */
};

/** @brief Additional PDP Context configuration options */
struct pdn_pdp_opt {
	/**
	 * @brief IPv4 address allocation.
	 * 0 – IPv4 address via Non-access Stratum (NAS) signaling (default)
	 * 1 – IPv4 address via Dynamic Host Configuration Protocol (DHCP)
	 */
	uint8_t ip4_addr_alloc;
	/**
	 * @brief NAS Signalling Low Priority Indication.
	 * 0 – Use Non-access Stratum (NAS) Signalling Low Priority Indication (NSLPI) value
	 *     from configuration (default)
	 * 1 – Use value "Not configured" for NAS Signalling Low Priority Indication
	 */
	uint8_t nslpi;
	/**
	 * @brief Protected transmission of Protocol Configuration Options (PCO).
	 * 0 – Protected transmission of PCO is not requested (default)
	 * 1 – Protected transmission of PCO is requested
	 */
	uint8_t secure_pco;
};

/** @brief PDN library event */
enum pdn_event {
	PDN_EVENT_CNEC_ESM,		/**< +CNEC ESM error code */
	PDN_EVENT_ACTIVATED,		/**< PDN connection activated */
	PDN_EVENT_DEACTIVATED,		/**< PDN connection deactivated */
	PDN_EVENT_IPV6_UP,		/**< PDN has IPv6 connectivity */
	PDN_EVENT_IPV6_DOWN,		/**< PDN has lost IPv6 connectivity */
	PDN_EVENT_NETWORK_DETACH,	/**< Network detached */
};

/** @brief PDN authentication method */
enum pdn_auth {
	PDN_AUTH_NONE = 0,	/**< No authentication */
	PDN_AUTH_PAP  = 1,	/**< Password authentication protocol */
	PDN_AUTH_CHAP = 2,	/**< Challenge handshake authentication protocol */
};

/**
 * @typedef pdn_event_handler_t
 *
 * Event handler for PDN events.
 * If assigned during PDP context creation, the event handler will receive
 * status information relative to the Packet Data Network connection,
 * as reported by the AT notifications +CNEC and +GGEV.
 *
 * This handler is executed by the same context that dispatches AT notifications.
 *
 * @param cid The PDP context ID.
 * @param event The event.
 * @param reason The ESM error reason, if available.
 */
typedef void (*pdn_event_handler_t)(uint8_t cid, enum pdn_event event, int reason);

/**
 * @brief Create a Packet Data Protocol (PDP) context.
 *
 * If a callback is provided via the @p cb parameter, the library will
 * generate events from the +CNEC and +GGEV AT notifications to report
 * state of the Packet Data Network (PDN) connection.
 *
 * @param[out] cid The ID of the new PDP context.
 * @param cb Optional event handler.
 * @return int Zero on success or a negative errno otherwise.
 */
int pdn_ctx_create(uint8_t *cid, pdn_event_handler_t cb);

/**
 * @brief Configure a Packet Data Protocol context.
 *
 * The PDN connection must be inactive when the PDP context is configured.
 *
 * @param cid The PDP context to configure.
 * @param apn The Access Point Name to configure the PDP context with.
 * @param family The family to configure the PDN context for.
 * @param opts Optional additional configuration options.
 * @return int Zero on success or a negative errno otherwise.
 */
int pdn_ctx_configure(uint8_t cid, const char *apn, enum pdn_fam family,
		      struct pdn_pdp_opt *opts);

/**
 * @brief Set PDN connection authentication parameters.
 *
 * @param cid The PDP context to set authentication parameters for.
 * @param method The desired authentication method.
 * @param user The username to use for authentication.
 * @param password The password to use for authentication.
 * @return int Zero on success or a negative errno otherwise.
 */
int pdn_ctx_auth_set(uint8_t cid, enum pdn_auth method,
		     const char *user, const char *password);

/**
 * @brief Destroy a Packet Data Protocol context.
 *
 * The PDN connection must be inactive when the PDP context is destroyed.
 *
 * @param cid The PDP context to destroy.
 * @return int Zero on success or a negative errno otherwise.
 */
int pdn_ctx_destroy(uint8_t cid);

/**
 * @brief Activate a Packet Data Network (PDN) connection.
 *
 * @param cid The PDP context ID to activate a connection for.
 * @param[out] esm If provided, the function will block to return the ESM error reason.
 * @param[out] family If provided, the function will block to return @c PDN_FAM_IPV4 if only IPv4
 *		      is supported, or @c PDN_FAM_IPV6 if only IPv6 is supported.
 *		      Otherwise, this value will remain unchanged.
 * @return int Zero on success or a negative errno otherwise.
 */
int pdn_activate(uint8_t cid, int *esm, enum pdn_fam *family);

/**
 * @brief Deactivate a Packet Data Network (PDN) connection.
 *
 * @param cid The PDP context ID.
 * @return int Zero on success or a negative errno otherwise.
 */
int pdn_deactivate(uint8_t cid);

/**
 * @brief Retrieve the PDN ID for a given PDP Context.
 *
 * The PDN ID can be used to route traffic through a Packet Data Network.
 *
 * @param cid The context ID of the PDN connection.
 * @return int A non-negative PDN ID on success, or a negative errno otherwise.
 */
int pdn_id_get(uint8_t cid);

/**
 * @brief Retrieve the default Access Point Name (APN).
 *
 * The default APN is the APN of the default PDP context (zero).
 *
 * @param[out] buf The buffer to copy the APN into.
 * @param len The size of the output buffer.
 * @return int Zero on success or a negative errno otherwise.
 */
int pdn_default_apn_get(char *buf, size_t len);

/**
 * @brief Register a callback for events pertaining to the default PDP context (zero).
 *
 * @param cb The PDN event handler.
 */
int pdn_default_ctx_cb_reg(pdn_event_handler_t cb);

/**
 * @brief Deregister a callback for events pertaining to the default PDP context (zero).
 *
 * @param cb The PDN event handler.
 */
int pdn_default_ctx_cb_dereg(pdn_event_handler_t cb);

#if CONFIG_PDN_ESM_STRERROR

/**
 * @brief Retrieve a statically allocated textual description for a given ESM error reason.
 *
 * @param reason ESM error reason.
 * @return const char* ESM error reason description.
 *		       If no textual description for the given error is found,
 *		       a placeholder string is returned instead.
 */
const char *pdn_esm_strerror(int reason);

#endif

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* PDN_H_ */
