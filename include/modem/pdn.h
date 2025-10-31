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
#include <zephyr/net/net_ip.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file pdn.h
 * @brief Public APIs for the Packet Data Network (PDN) library.
 * @defgroup pdn PDN library
 * @{
 */

/** @brief Address family */
enum pdn_fam {
	PDN_FAM_IPV4   = 0, /**< IPv4 */
	PDN_FAM_IPV6   = 1, /**< IPv6 */
	PDN_FAM_IPV4V6 = 2, /**< IPv4 and IPv6 */
	PDN_FAM_NONIP  = 3, /**< Non-IP */
};

/** @brief Additional Packet Data Protocol (PDP) context configuration options */
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

/**
 * @brief PDN connection dynamic information structure.
 *
 * This structure holds dynamic information about the PDN connection.
 */
struct pdn_dynamic_info {
	/**
	 * @brief IPv4 Maximum Transmission Unit.
	 */
	uint32_t ipv4_mtu;
	/**
	 * @brief IPv6 Maximum Transmission Unit.
	 */
	uint32_t ipv6_mtu;
	/**
	 * @brief Primary IPv4 DNS address.
	 */
	struct in_addr dns_addr4_primary;
	/**
	 * @brief Secondary IPv4 DNS address.
	 */
	struct in_addr dns_addr4_secondary;
	/**
	 * @brief Primary IPv6 DNS address.
	 */
	struct in6_addr dns_addr6_primary;
	/**
	 * @brief Secondary IPv6 DNS address.
	 */
	struct in6_addr dns_addr6_secondary;
};

/** @brief PDN library event */
enum pdn_event {
	/** +CNEC ESM error code */
	PDN_EVENT_CNEC_ESM,
	/** PDN connection activated */
	PDN_EVENT_ACTIVATED,
	/** PDN connection deactivated */
	PDN_EVENT_DEACTIVATED,
	/** PDN has IPv6 connectivity */
	PDN_EVENT_IPV6_UP,
	/** PDN has lost IPv6 connectivity */
	PDN_EVENT_IPV6_DOWN,
	/** Network detached */
	PDN_EVENT_NETWORK_DETACH,
	/** APN rate control is ON for the given PDN */
	PDN_EVENT_APN_RATE_CONTROL_ON,
	/** APN rate control is OFF for the given PDN */
	PDN_EVENT_APN_RATE_CONTROL_OFF,
	/** PDP context is destroyed for the given PDN.
	 *  This happens if modem is switched to minimum functionality mode.
	 */
	PDN_EVENT_CTX_DESTROYED,
	/** A cellular profile is activated for the current access technology.
	 *  The ``cid`` parameter in the callback is the context ID of the default PDP context
	 *  for the active cellular profile.
	 */
	PDN_EVENT_CELLULAR_PROFILE_ACTIVE,
};

/** @brief Authentication method */
enum pdn_auth {
	PDN_AUTH_NONE = 0,	/**< No authentication */
	PDN_AUTH_PAP  = 1,	/**< Password Authentication Protocol (PAP) */
	PDN_AUTH_CHAP = 2,	/**< Challenge-Handshake Authentication Protocol (CHAP) */
};

/** @brief Cellular profile access technology type. Decides for which access technology the profile
 *	   is valid.
 */
enum pdn_act {
	/** LTE-M access technology */
	PDN_ACT_LTE_M		= 1,
	/** NB-IoT access technology */
	PDN_ACT_NB_IOT		= 2,
	/** LTE-M and NB-IoT access technology */
	PDN_ACT_LTE_M_NB_IOT	= 3,
	/** NTN access technology */
	PDN_ACT_NTN		= 4,
};

/** @brief SIM slot type, used for cellular profile selection */
enum pdn_sim_slot {
	/** SIM slot 1, also used for eSIM */
	PDN_SIM_SLOT_1 = 0,
	/** SIM slot for SoftSIM */
	PDN_SIM_SOFTSIM = 2,
};

/** @brief Cellular profile. Used to set the cellular profile for the PDN connection.
 *  @note The profile ID 0 is associated with PDN contexts in the range 0-9, while profile ID 1
 *	  is associated with PDN contexts in the range 10-19. The default context for each profile
 *	  is the first context in the range.
 */
struct pdn_cellular_profile {
	/** Cellular profile ID. Valid values are 0 and 1. */
	uint8_t id;
	/** Access technology */
	enum pdn_act act;
	/** SIM slot */
	enum pdn_sim_slot sim_slot;
};

/**
 * @typedef pdn_event_handler_t
 *
 * @brief Event handler for PDN events.
 *
 * If assigned during PDP context creation, the event handler will receive
 * status information relative to the PDN connection,
 * as reported by the AT notifications +CNEC and +GGEV.
 *
 * This handler is executed in the same context that dispatches AT notifications.
 *
 * @param cid The PDP context ID. For cellular profile selection events,
 * 	      this is the default context ID for the active cellular profile.
 * @param event The event.
 * @param reason The ESM error reason, if available.
 */
typedef void (*pdn_event_handler_t)(uint8_t cid, enum pdn_event event, int reason);

/**
 * @brief Create a PDP context.
 *
 * If a callback is provided via the @p cb parameter, the library will
 * generate events from the +CNEC and +GGEV AT notifications to report
 * state of the PDN connection.
 *
 * @param[out] cid The ID of the new PDP context.
 * @param cb Optional event handler.
 *
 * @return int Zero on success or a negative errno otherwise.
 */
int pdn_ctx_create(uint8_t *cid, pdn_event_handler_t cb);

/**
 * @brief Configure a PDP context.
 *
 * The PDN connection must be inactive when the PDP context is configured.
 *
 * @param cid The PDP context ID.
 * @param apn The Access Point Name.
 * @param family The address family.
 * @param opts Additional configuration options. Optional, can be NULL.
 *
 * @return int Zero on success or a negative errno otherwise.
 */
int pdn_ctx_configure(uint8_t cid, const char *apn, enum pdn_fam family,
		      struct pdn_pdp_opt *opts);

/**
 * @brief Set PDP context authentication parameters.
 *
 * @param cid The PDP context ID.
 * @param method The desired authentication method.
 * @param user The username to use for authentication.
 * @param password The password to use for authentication.
 *
 * @return int Zero on success or a negative errno otherwise.
 */
int pdn_ctx_auth_set(uint8_t cid, enum pdn_auth method,
		     const char *user, const char *password);

/**
 * @brief Destroy a PDP context.
 *
 * The PDN connection must be inactive when the PDP context is destroyed.
 *
 * @param cid The PDP context ID.
 *
 * @return int Zero on success or a negative errno otherwise.
 */
int pdn_ctx_destroy(uint8_t cid);

/**
 * @brief Activate a PDN connection.
 *
 * @param cid The PDP context ID.
 * @param[out] esm If provided, the function will block to return the ESM error reason.
 * @param[out] family If provided, the function will block to return @c PDN_FAM_IPV4 if only IPv4
 *		      is supported, or @c PDN_FAM_IPV6 if only IPv6 is supported.
 *		      Otherwise, this value will remain unchanged.
 *
 * @return int Zero on success or a negative errno otherwise.
 */
int pdn_activate(uint8_t cid, int *esm, enum pdn_fam *family);

/**
 * @brief Deactivate a PDN connection.
 *
 * @param cid The PDP context ID.
 *
 * @return int Zero on success or a negative errno otherwise.
 */
int pdn_deactivate(uint8_t cid);

/**
 * @brief Retrieve the PDN ID for a given PDP context.
 *
 * The PDN ID can be used to route traffic through a PDN connection.
 *
 * @param cid The PDP context ID.
 *
 * @return int A non-negative PDN ID on success, or a negative errno otherwise.
 */
int pdn_id_get(uint8_t cid);

/**
 * @brief Retrieve dynamic parameters of a given PDN connection.
 *
 * @param[in] cid The PDP context ID.
 * @param[out] pdn_info PDN dynamic info.
 *
 * @return Zero on success or an error code on failure.
 */
int pdn_dynamic_info_get(uint8_t cid, struct pdn_dynamic_info *pdn_info);

/**
 * @brief Retrieve the default Access Point Name (APN).
 *
 * The default APN is the APN of the default PDP context (zero).
 *
 * @param[out] buf The buffer to copy the APN into. The string is null-terminated.
 * @param len The size of the output buffer.
 *
 * @return int Zero on success or a negative errno otherwise.
 */
int pdn_default_apn_get(char *buf, size_t len);

/**
 * @brief Register a callback for events pertaining to the default PDP context (zero).
 *
 * @param cb The PDN event handler.
 *
 * @retval 0       Callback registered successfully, or the callback is already registered.
 * @retval -EFAULT The provided cb parameter was \c NULL.
 * @retval -ENOMEM Insufficient heap to allocate the PDP context.
 */
int pdn_default_ctx_cb_reg(pdn_event_handler_t cb);

/**
 * @brief Deregister a callback for events pertaining to the default PDP context (zero).
 *
 * @param cb The PDN event handler.
 *
 * @retval 0       Callback deregistered successfully.
 * @retval -EFAULT The provided cb parameter was \c NULL.
 * @retval -EINVAL PDP context with the provided callback was not found. This can be returned if
 *                 the callback was not registered upon calling this function.
 */
int pdn_default_ctx_cb_dereg(pdn_event_handler_t cb);

/**
 * @brief Configure a cellular profile. The cellular profile to be used for a specific access
 *	  technology is decided by the modem. The active cellular profile is reported in the
 *	  ``PDN_EVENT_CELLULAR_PROFILE_ACTIVE`` event if a callback is registered.
 *
 * @param profile The cellular profile to set.
 *
 * @return Zero on success or a negative error code on failure.
 */
int pdn_cellular_profile_configure(struct pdn_cellular_profile *profile);

#if CONFIG_PDN_ESM_STRERROR

/**
 * @brief Retrieve a statically allocated textual description for a given ESM error reason.
 *
 * @param reason ESM error reason.
 *
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
