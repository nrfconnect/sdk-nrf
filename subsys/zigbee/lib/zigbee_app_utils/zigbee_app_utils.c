/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <ctype.h>
#include <stddef.h>
#include <zephyr/sys/util.h>

#include <dk_buttons_and_leds.h>
#include <zephyr/logging/log.h>

#include <zigbee/zigbee_app_utils.h>
#include <zigbee/zigbee_error_handler.h>
#include <zb_nrf_platform.h>
#include <zboss_api.h>

/* Number of retries until the pin value stabilizes. */
#define READ_RETRIES  10

/* Timeout after which End Device stops to send beacons
 * if can not join/rejoin a network.
 */
#ifndef ZB_DEV_REJOIN_TIMEOUT_MS
#define ZB_DEV_REJOIN_TIMEOUT_MS (1000 * 200)
#endif

/* Maximum interval between join/rejoin attempts. */
#define REJOIN_INTERVAL_MAX_S    (15 * 60)

/* Rejoin interval, after which the device will perform Trust Center Rejoin
 * instead of a secure rejoin.
 */
#define TC_REJOIN_INTERVAL_THRESHOLD_S (2 * 60)
#define ZB_SECUR_PROVISIONAL_KEY 2

#define IEEE_ADDR_BUF_SIZE       17

#if defined CONFIG_ZIGBEE_FACTORY_RESET
#define FACTORY_RESET_PROBE_TIME K_SECONDS(1)
#endif /* CONFIG_ZIGBEE_FACTORY_RESET */

LOG_MODULE_REGISTER(zigbee_app_utils, CONFIG_ZIGBEE_APP_UTILS_LOG_LEVEL);

/* Rejoin-procedure related variables. */
static bool stack_initialised;
static bool is_rejoin_procedure_started;
static bool is_rejoin_stop_requested;
static bool is_rejoin_in_progress;
static uint8_t rejoin_attempt_cnt;
#if defined CONFIG_ZIGBEE_ROLE_END_DEVICE
static volatile bool wait_for_user_input;
static volatile bool is_rejoin_start_scheduled;
#endif

/* Forward declarations. */
static void rejoin_the_network(zb_uint8_t param);
static void start_network_rejoin(void);
static void stop_network_rejoin(zb_uint8_t was_scheduled);

/* A ZBOSS internal API needed for workaround for KRKNWK-14112 */
struct zb_aps_device_key_pair_set_s ZB_PACKED_PRE
{
	zb_ieee_addr_t  device_address;
	zb_uint8_t      link_key[ZB_CCM_KEY_SIZE];
#ifndef ZB_LITE_NO_GLOBAL_VS_UNIQUE_KEYS
	zb_bitfield_t   aps_link_key_type:1;
#endif
	zb_bitfield_t   key_source:1;
	zb_bitfield_t   key_attributes:2;
	zb_bitfield_t   reserved:4;
	zb_uint8_t      align[3];
} ZB_PACKED_STRUCT;
extern void zb_nwk_forget_device(zb_uint8_t addr_ref);
extern struct zb_aps_device_key_pair_set_s  *zb_secur_get_link_key_by_address(
							zb_ieee_addr_t address,
							zb_uint8_t attr);


#if defined CONFIG_ZIGBEE_FACTORY_RESET
/* Factory Reset related variables. */
struct factory_reset_context_t {
	uint32_t button;
	bool reset_done;
	bool pibcache_pan_id_needs_reset;
	struct k_timer timer;
};
static struct factory_reset_context_t factory_reset_context;
#endif /* CONFIG_ZIGBEE_FACTORY_RESET */

/**@brief Function to set the Erase persistent storage
 *        depending on the erase pin
 */
void zigbee_erase_persistent_storage(zb_bool_t erase)
{
#ifdef ZB_USE_NVRAM
	zb_set_nvram_erase_at_start(erase);
#endif
}

int to_hex_str(char *out, uint16_t out_size, const uint8_t *in,
	       uint8_t in_size, bool reverse)
{
	int bytes_written = 0;
	int status;
	int i = reverse ? in_size - 1 : 0;

	for (; in_size > 0; in_size--) {
		status = snprintf(out + bytes_written,
				  out_size - bytes_written,
				  "%02x",
				  in[i]);
		if (status < 0) {
			return status;
		}

		bytes_written += status;
		i += reverse ? -1 : 1;
	}

	return bytes_written;
}

int ieee_addr_to_str(char *str_buf, uint16_t buf_len,
		     const zb_ieee_addr_t addr)
{
	return to_hex_str(str_buf, buf_len, (const uint8_t *)addr,
			  sizeof(zb_ieee_addr_t), true);
}

bool parse_hex_str(char const *in_str, uint8_t in_str_len, uint8_t *out_buff,
		   uint8_t out_buff_size, bool reverse)
{
	uint8_t i = 0;
	int8_t delta = 1;

	/* Skip 0x suffix if present. */
	if ((in_str_len > 2) && (in_str[0] == '0') &&
	    (tolower(in_str[1]) == 'x')) {
		in_str_len -= 2;
		in_str += 2;
	}

	if (reverse) {
		in_str = in_str + in_str_len - 1;
		delta = -1;
	}

	/* Check if we have enough output space */
	if (in_str_len > 2 * out_buff_size) {
		return false;
	}

	memset(out_buff, 0, out_buff_size);

	while (i < in_str_len) {
		uint8_t nibble = 0;

		if (char2hex(*in_str, &nibble)) {
			break;
		}

		if (i & 0x01) {
			*out_buff |= reverse ? nibble << 4 : nibble;
			out_buff++;
		} else {
			*out_buff = reverse ? nibble : nibble << 4;
		}

		i += 1;
		in_str += delta;
	}

	return (i == in_str_len);
}

addr_type_t parse_address(const char *input, zb_addr_u *addr,
			  addr_type_t addr_type)
{
	addr_type_t result = ADDR_INVALID;
	size_t len;

	if (!input || !addr) {
		return ADDR_INVALID;
	}

	len = strlen(input);
	if (!len) {
		return ADDR_INVALID;
	}

	/* Skip 0x suffix if present. */
	if ((input[0] == '0') && (tolower(input[1]) == 'x')) {
		input += 2;
		len -= 2;
	}

	if ((len == 2 * sizeof(zb_ieee_addr_t)) &&
	    (addr_type == ADDR_ANY || addr_type == ADDR_LONG)) {
		result = ADDR_LONG;
	} else if ((len == 2 * sizeof(uint16_t)) &&
		   (addr_type == ADDR_ANY || addr_type == ADDR_SHORT)) {
		result = ADDR_SHORT;
	} else {
		return ADDR_INVALID;
	}

	return parse_hex_str(input, len, (uint8_t *)addr, len / 2, true) ?
	       result :
	       ADDR_INVALID;
}

zb_ret_t zigbee_default_signal_handler(zb_bufid_t bufid)
{
	zb_zdo_app_signal_hdr_t *sig_hndler = NULL;
	zb_zdo_app_signal_type_t sig = zb_get_app_signal(bufid, &sig_hndler);
	zb_ret_t status = ZB_GET_APP_SIGNAL_STATUS(bufid);
	zb_nwk_device_type_t role = zb_get_network_role();
	zb_ret_t ret_code = RET_OK;
	zb_bool_t comm_status = ZB_TRUE;

	switch (sig) {
	case ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY:
		/* At this point Zigbee stack attempted to load production
		 * configuration from NVRAM.
		 * This step is performed each time the stack is initialized.
		 *
		 * Note: if it is necessary for a device to have
		 *       a valid production configuration to operate
		 *       (e.g. due to legal reasons), the application should
		 *       implement the customized logic for this signal
		 *       (e.g. assert on the signal status code).
		 */
		if (status != RET_OK) {
			LOG_INF("Production configuration is not present or invalid (status: %d)",
				status);
		} else {
			LOG_INF("Production configuration successfully loaded");
		}
		break;

	case ZB_ZDO_SIGNAL_SKIP_STARTUP:
		/* At this point Zigbee stack:
		 *  - Initialized the scheduler.
		 *  - Initialized and read NVRAM configuration.
		 *  - Initialized all stack-related global variables.
		 *
		 * Next step: perform BDB initialization procedure,
		 *            (see BDB specification section 7.1).
		 */
		ZB_ERROR_CHECK(zb_zcl_set_backward_comp_mode(ZB_ZCL_AUTO_MODE));
		ZB_ERROR_CHECK(
			zb_zcl_set_backward_compatible_statuses_mode(ZB_ZCL_STATUSES_ZCL8_MODE));
		if (IS_ENABLED(CONFIG_ZIGBEE_PANID_CONFLICT_RESOLUTION)) {
			zb_enable_panid_conflict_resolution(
				CONFIG_ZIGBEE_PANID_CONFLICT_RESOLUTION);
		}
		stack_initialised = true;
		LOG_INF("Zigbee stack initialized");
		comm_status = bdb_start_top_level_commissioning(
			ZB_BDB_INITIALIZATION);
		break;

	case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
		/* At this point Zigbee stack is ready to operate and the BDB
		 * initialization procedure has finished.
		 * There is no network configuration stored inside NVRAM.
		 *
		 * Next step:
		 *  - If the device implements Zigbee router
		 *    or Zigbee end device, perform network steering
		 *    for a node not on a network,
		 *    (see BDB specification section 8.3).
		 *  - If the device implements Zigbee coordinator,
		 *    perform network formation,
		 *    (see BDB specification section 8.4).
		 */
		LOG_INF("Device started for the first time");
		if (status == RET_OK) {
			if (role != ZB_NWK_DEVICE_TYPE_COORDINATOR) {
				LOG_INF("Start network steering");
				start_network_rejoin();
			} else {
				LOG_INF("Start network formation");
				comm_status = bdb_start_top_level_commissioning(
					ZB_BDB_NETWORK_FORMATION);
			}
		} else {
			LOG_ERR("Failed to initialize Zigbee stack (status: %d)",
				status);
		}
		break;

	case ZB_BDB_SIGNAL_DEVICE_REBOOT:
		/* At this point Zigbee stack is ready to operate and the BDB
		 * initialization procedure has finished. There is network
		 * configuration stored inside NVRAM, so the device
		 * will try to rejoin.
		 *
		 * Next step: if the device implement Zigbee router or
		 *            end device, and the initialization has failed,
		 *            perform network steering for a node on a network,
		 *            (see BDB specification section 8.2).
		 */
		if (status == RET_OK) {
			zb_ext_pan_id_t extended_pan_id;
			char ieee_addr_buf[IEEE_ADDR_BUF_SIZE] = { 0 };
			int addr_len;

			zb_get_extended_pan_id(extended_pan_id);
			addr_len = ieee_addr_to_str(ieee_addr_buf,
						    sizeof(ieee_addr_buf),
						    extended_pan_id);
			if (addr_len < 0) {
				strcpy(ieee_addr_buf, "unknown");
			}

			/* Device has joined the network so stop the network
			 * rejoin procedure.
			 */
			stop_network_rejoin(ZB_FALSE);
			LOG_INF("Joined network successfully on reboot signal (Extended PAN ID: %s, PAN ID: 0x%04hx)",
				ieee_addr_buf,
				ZB_PIBCACHE_PAN_ID());
		} else {
			if (role != ZB_NWK_DEVICE_TYPE_COORDINATOR) {
				LOG_INF("Unable to join the network, start network steering");
				start_network_rejoin();
			} else {
				LOG_ERR("Failed to initialize Zigbee stack using NVRAM data (status: %d)",
					status);
			}
		}
		break;

	case ZB_BDB_SIGNAL_STEERING:
		/* At this point the Zigbee stack has finished network steering
		 * procedure. The device may have rejoined the network,
		 * which is indicated by signal's status code.
		 *
		 * Next step:
		 *  - If the device implements Zigbee router and the steering
		 *    is not successful, retry joining Zigbee network
		 *    by starting network steering after 1 second.
		 *  - It is not expected to finish network steering with error
		 *    status if the device implements Zigbee coordinator,
		 *    (see BDB specification section 8.2).
		 */
		if (status == RET_OK) {
			zb_ext_pan_id_t extended_pan_id;
			char ieee_addr_buf[IEEE_ADDR_BUF_SIZE] = { 0 };
			int addr_len;

			zb_get_extended_pan_id(extended_pan_id);
			addr_len = ieee_addr_to_str(ieee_addr_buf,
						    sizeof(ieee_addr_buf),
						    extended_pan_id);
			if (addr_len < 0) {
				strcpy(ieee_addr_buf, "unknown");
			}

			LOG_INF("Joined network successfully (Extended PAN ID: %s, PAN ID: 0x%04hx)",
				ieee_addr_buf,
				ZB_PIBCACHE_PAN_ID());
			/* Device has joined the network so stop the network
			 * rejoin procedure.
			 */
			if (role != ZB_NWK_DEVICE_TYPE_COORDINATOR) {
				stop_network_rejoin(ZB_FALSE);
			}
		} else {
			if (role != ZB_NWK_DEVICE_TYPE_COORDINATOR) {
				LOG_INF("Network steering was not successful (status: %d)",
					status);
				start_network_rejoin();
			} else {
				LOG_INF("Network steering failed on Zigbee coordinator (status: %d)",
					status);
			}
		}
		break;

	case ZB_BDB_SIGNAL_FORMATION:
		/* At this point the Zigbee stack has finished network formation
		 * procedure. The device may have created a new Zigbee network,
		 * which is indicated by signal's status code.
		 *
		 * Next step:
		 *  - If the device implements Zigbee coordinator
		 *    and the formation is not successful, try to form a new
		 *    Zigbee network by performing network formation after
		 *    1 second (see BDB specification section 8.4).
		 *  - If the network formation was successful, open the newly
		 *    created network for other devices to join by starting
		 *    network steering for a node on a network,
		 *    (see BDB specification section 8.2).
		 *  - If the device implements Zigbee router or end device,
		 *    this signal is not expected.
		 */
		if (status == RET_OK) {
			zb_ext_pan_id_t extended_pan_id;
			char ieee_addr_buf[IEEE_ADDR_BUF_SIZE] = { 0 };
			int addr_len;

			zb_get_extended_pan_id(extended_pan_id);
			addr_len = ieee_addr_to_str(ieee_addr_buf,
						    sizeof(ieee_addr_buf),
						    extended_pan_id);
			if (addr_len < 0) {
				strcpy(ieee_addr_buf, "unknown");
			}

			LOG_INF("Network formed successfully, start network steering (Extended PAN ID: %s, PAN ID: 0x%04hx)",
				ieee_addr_buf,
				ZB_PIBCACHE_PAN_ID());
			comm_status = bdb_start_top_level_commissioning(
				ZB_BDB_NETWORK_STEERING);
		} else {
			LOG_INF("Restart network formation (status: %d)",
				status);
			ret_code = ZB_SCHEDULE_APP_ALARM(
				(zb_callback_t)
				bdb_start_top_level_commissioning,
				ZB_BDB_NETWORK_FORMATION,
				ZB_TIME_ONE_SECOND);
		}
		break;

	case ZB_ZDO_SIGNAL_LEAVE:
		/* This signal is generated when the device itself has left
		 * the network by sending leave command.
		 *
		 * Note: this signal will be generated if the device tries
		 *       to join legacy Zigbee network and the TCLK exchange
		 *       cannot be completed. In such situation,
		 *       the ZB_BDB_NETWORK_STEERING signal will be generated
		 *       afterwards, so this case may be left unimplemented.
		 */
		if (status == RET_OK) {
			zb_zdo_signal_leave_params_t *leave_params =
				ZB_ZDO_SIGNAL_GET_PARAMS(
					sig_hndler,
					zb_zdo_signal_leave_params_t);
			LOG_INF("Network left (leave type: %d)",
				leave_params->leave_type);

			if (zb_get_network_role() ==
			    ZB_NWK_DEVICE_TYPE_COORDINATOR) {
				if (factory_reset_context.pibcache_pan_id_needs_reset) {
					zigbee_pibcache_pan_id_clear();
					factory_reset_context.pibcache_pan_id_needs_reset = false;
				}
				/* For coordinator node,
				 * start network formation.
				 */
				comm_status = bdb_start_top_level_commissioning(
					ZB_BDB_NETWORK_FORMATION);
			} else {
				/* Start network rejoin procedure. */
				start_network_rejoin();
			}
		} else {
			LOG_ERR("Unable to leave network (status: %d)", status);
		}
		break;

	case ZB_ZDO_SIGNAL_LEAVE_INDICATION: {
		/* This signal is generated on the parent to indicate, that one
		 * of its child nodes left the network.
		 */
		zb_zdo_signal_leave_indication_params_t
		*leave_ind_params = ZB_ZDO_SIGNAL_GET_PARAMS(
			sig_hndler,
			zb_zdo_signal_leave_indication_params_t);
		char ieee_addr_buf[IEEE_ADDR_BUF_SIZE] = { 0 };
		int addr_len;

		addr_len = ieee_addr_to_str(
			ieee_addr_buf, sizeof(ieee_addr_buf),
			leave_ind_params->device_addr);
		if (addr_len < 0) {
			strcpy(ieee_addr_buf, "unknown");
		}
		LOG_INF("Child left the network (long: %s, rejoin flag: %d)",
			ieee_addr_buf,
			leave_ind_params->rejoin);
		break;
	}

	case ZB_COMMON_SIGNAL_CAN_SLEEP:
		/* Zigbee stack can enter sleep state. If the application wants
		 * to proceed, it should call zb_sleep_now() function.
		 *
		 * Note: if the application shares some resources between Zigbee
		 *       stack and other tasks/contexts, device disabling should
		 *       be overwritten by implementing one of the weak
		 *       functions inside zb_nrf_pwr_mgmt.c.
		 */
		zb_sleep_now();
		break;

	case ZB_ZDO_SIGNAL_DEVICE_UPDATE: {
		/* This signal notifies the Zigbee Trust center (usually
		 * implemented on the coordinator node) or parent router
		 * application once a device joined, rejoined,
		 * or left the network.
		 *
		 * For more information see table 4.14
		 * of the Zigbee Specification (R21).
		 */
		zb_zdo_signal_device_update_params_t *update_params =
			ZB_ZDO_SIGNAL_GET_PARAMS(
				sig_hndler,
				zb_zdo_signal_device_update_params_t);
		char ieee_addr_buf[IEEE_ADDR_BUF_SIZE] = { 0 };
		int addr_len;

		addr_len = ieee_addr_to_str(ieee_addr_buf,
					    sizeof(ieee_addr_buf),
					    update_params->long_addr);
		if (addr_len < 0) {
			strcpy(ieee_addr_buf, "unknown");
		}
		LOG_INF("Device update received (short: 0x%04hx, long: %s, status: %d)",
			update_params->short_addr,
			ieee_addr_buf,
			update_params->status);

		if (IS_ENABLED(CONFIG_ZIGBEE_TC_REJOIN_ENABLED)) {
			/* Workaround: KRKNWK-14112 */
			zb_address_ieee_ref_t addr_ref;

			if ((zb_address_by_ieee(update_params->long_addr, ZB_FALSE, ZB_FALSE,
									&addr_ref) == RET_OK)
				&& zb_secur_get_link_key_by_address(update_params->long_addr,
					ZB_SECUR_PROVISIONAL_KEY)) {
				ZB_SCHEDULE_APP_ALARM_CANCEL(zb_nwk_forget_device, addr_ref);
			}
		}
		break;
	}

	case ZB_ZDO_SIGNAL_DEVICE_ANNCE: {
		/* This signal is generated when a Device Announcement command
		 * is received by the device. Such packet is generated whenever
		 * a node joins or rejoins the network, so this signal may be
		 * used to track the number of devices.
		 *
		 * Note: since the Device Announcement command is sent to the
		 *       broadcast address, this method may miss some devices.
		 *       The complete knowledge about nodes has only
		 *       the coordinator.
		 *
		 * Note: it may happen, that a device broadcasts the Device
		 *       Announcement command and is removed by the coordinator
		 *       afterwards, due to security policy
		 *       (lack of TCLK exchange).
		 */
		zb_zdo_signal_device_annce_params_t *dev_annce_params =
			ZB_ZDO_SIGNAL_GET_PARAMS(
				sig_hndler,
				zb_zdo_signal_device_annce_params_t);
		LOG_INF("New device commissioned or rejoined (short: 0x%04hx)",
			dev_annce_params->device_short_addr);
		break;
	}

#ifndef CONFIG_ZIGBEE_ROLE_END_DEVICE
	case ZB_ZDO_SIGNAL_DEVICE_AUTHORIZED: {
		/* This signal notifies the Zigbee Trust center application
		 * (usually implemented on the coordinator node) about
		 * authorization of a new device in the network.
		 *
		 * For Zigbee 3.0 (and newer) devices this signal
		 * is generated if:
		 *  - TCKL exchange procedure was successful
		 *  - TCKL exchange procedure timed out
		 *
		 * If the coordinator allows for legacy devices to join
		 * the network (enabled by zb_bdb_set_legacy_device_support(1)
		 * API call), this signal is generated:
		 *  - If the parent router generates Update Device command and
		 *    the joining device does not perform TCLK exchange
		 *    within timeout.
		 *  - If the TCLK exchange is successful.
		 */
		zb_zdo_signal_device_authorized_params_t
		*authorize_params = ZB_ZDO_SIGNAL_GET_PARAMS(
			sig_hndler,
			zb_zdo_signal_device_authorized_params_t);
		char ieee_addr_buf[IEEE_ADDR_BUF_SIZE] = { 0 };
		int addr_len;

		addr_len =
			ieee_addr_to_str(ieee_addr_buf,
					 sizeof(ieee_addr_buf),
					 authorize_params->long_addr);
		if (addr_len < 0) {
			strcpy(ieee_addr_buf, "unknown");
		}
		LOG_INF("Device authorization event received"
			" (short: 0x%04hx, long: %s, authorization type: %d,"
			" authorization status: %d)",
			authorize_params->short_addr, ieee_addr_buf,
			authorize_params->authorization_type,
			authorize_params->authorization_status);
		break;
	}
#endif

	case ZB_NWK_SIGNAL_NO_ACTIVE_LINKS_LEFT:
		/* This signal informs the application that all links to other
		 * routers has expired. In such situation, the node can
		 * communicate only with its children.
		 *
		 * Example reasons of signal generation:
		 *  - The device was brought too far from the rest
		 *    of the network.
		 *  - There was a power cut and the whole network
		 *    suddenly disappeared.
		 *
		 * Note: This signal is not generated for the coordinator node,
		 *       since it may operate alone in the network.
		 */
		LOG_WRN("Parent is unreachable");
		break;

	case ZB_BDB_SIGNAL_FINDING_AND_BINDING_TARGET_FINISHED:
		/* This signal informs the Finding & Binding target device that
		 * the procedure has finished and the other device has
		 * been bound or the procedure timed out.
		 */
		LOG_INF("Find and bind target finished (status: %d)", status);
		break;

#ifndef CONFIG_ZIGBEE_ROLE_END_DEVICE
	case ZB_NWK_SIGNAL_PANID_CONFLICT_DETECTED: {
		/* This signal informs the Router and Coordinator that conflict
		 * PAN ID has been detected and needs to be resolved. In order
		 * to do that *zb_start_pan_id_conflict_resolution* is called.
		 */
		LOG_INF("PAN ID conflict detected, trying to resolve. ");

		zb_bufid_t buf_copy = zb_buf_get_out();

		if (buf_copy) {
			zb_buf_copy(buf_copy, bufid);
			ZVUNUSED(ZB_ZDO_SIGNAL_CUT_HEADER(buf_copy));

			zb_start_pan_id_conflict_resolution(buf_copy);
		} else {
			LOG_ERR("No free buffer available, skipping conflict resolving this time.");
		}
		break;
	}
#endif

	case ZB_ZDO_SIGNAL_DEFAULT_START:
	case ZB_NWK_SIGNAL_DEVICE_ASSOCIATED:
		/* Obsolete signals, used for pre-R21 ZBOSS API. Ignore. */
		break;

	case ZB_BDB_SIGNAL_TC_REJOIN_DONE:
		/* This signal informs that Trust Center Rejoin is completed.
		 * The signal status indicates if the device has successfully
		 * rejoined the network.
		 *
		 * Next step: if the device implement Zigbee router or
		 *            end device, and the Trust Center Rejoin has failed,
		 *            perform restart the generic rejoin procedure.
		 */
		if (IS_ENABLED(CONFIG_ZIGBEE_TC_REJOIN_ENABLED)) {
			if (status == RET_OK) {
				zb_ext_pan_id_t extended_pan_id;
				char ieee_addr_buf[IEEE_ADDR_BUF_SIZE] = { 0 };
				int addr_len;

				zb_get_extended_pan_id(extended_pan_id);
				addr_len = ieee_addr_to_str(ieee_addr_buf,
								sizeof(ieee_addr_buf),
								extended_pan_id);
				if (addr_len < 0) {
					strcpy(ieee_addr_buf, "unknown");
				}

				LOG_INF("Joined network successfully after TC rejoin"
						" (Extended PAN ID: %s, PAN ID: 0x%04hx)",
					ieee_addr_buf,
					ZB_PIBCACHE_PAN_ID());
				/* Device has joined the network so stop the network
				 * rejoin procedure.
				 */
				if (role != ZB_NWK_DEVICE_TYPE_COORDINATOR) {
					stop_network_rejoin(ZB_FALSE);
				}
			} else {
				if (role != ZB_NWK_DEVICE_TYPE_COORDINATOR) {
					LOG_INF("TC Rejoin was not successful (status: %d)",
						status);
					start_network_rejoin();
				} else {
					LOG_INF("TC Rejoin failed on Zigbee coordinator"
							" (status: %d)", status);
				}
			}
			break;
		}

		/*
		 * Fall-through to the default case if Trust Center Rejoin is disabled
		 */

	default:
		/* Unimplemented signal. For more information,
		 * see: zb_zdo_app_signal_type_e and zb_ret_e.
		 */
		LOG_INF("Unimplemented signal (signal: %d, status: %d)",
			sig, status);
		break;
	}

	/* If configured, process network rejoin procedure. */
	rejoin_the_network(0);

	if ((ret_code == RET_OK) && (comm_status != ZB_TRUE)) {
		ret_code = RET_ERROR;
	}

	return ret_code;
}

void zigbee_led_status_update(zb_bufid_t bufid, uint32_t led_idx)
{
	zb_zdo_app_signal_hdr_t *p_sg_p = NULL;
	zb_zdo_app_signal_type_t sig = zb_get_app_signal(bufid, &p_sg_p);
	zb_ret_t status = ZB_GET_APP_SIGNAL_STATUS(bufid);

	switch (sig) {
	case ZB_BDB_SIGNAL_DEVICE_REBOOT:
	/* fall-through */
	case ZB_BDB_SIGNAL_STEERING:
		if (status == RET_OK) {
			dk_set_led_on(led_idx);
		} else {
			dk_set_led_off(led_idx);
		}
		break;

	case ZB_ZDO_SIGNAL_LEAVE:
		/* Update network status LED */
		dk_set_led_off(led_idx);
		break;

	default:
		break;
	}
}

/**@brief Start network steering.
 */
static void start_network_steering(zb_uint8_t param)
{
	ZVUNUSED(param);
	ZVUNUSED(bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING));
}

/**@brief Process rejoin procedure. To be called in signal handler.
 */
static void rejoin_the_network(zb_uint8_t param)
{
	ZVUNUSED(param);

	if (stack_initialised && is_rejoin_procedure_started) {
		if (is_rejoin_stop_requested) {
			is_rejoin_procedure_started = false;
			is_rejoin_stop_requested = false;

#if defined CONFIG_ZIGBEE_ROLE_END_DEVICE
			LOG_INF("Network rejoin procedure stopped as %sscheduled.",
				(wait_for_user_input) ? "" : "NOT ");
#else
			LOG_INF("Network rejoin procedure stopped.");
#endif
		} else if (!is_rejoin_in_progress) {
			/* Calculate new timeout */
			zb_time_t timeout_s;
			zb_ret_t zb_err_code;
			zb_callback_t alarm_cb = start_network_steering;
			zb_uint8_t alarm_cb_param = ZB_FALSE;

			if ((1 << rejoin_attempt_cnt) > REJOIN_INTERVAL_MAX_S) {
				timeout_s = REJOIN_INTERVAL_MAX_S;
			} else {
				timeout_s = (1 << rejoin_attempt_cnt);
				rejoin_attempt_cnt++;
			}

			if (IS_ENABLED(CONFIG_ZIGBEE_TC_REJOIN_ENABLED)) {
				if ((timeout_s > TC_REJOIN_INTERVAL_THRESHOLD_S)
					&& !zb_bdb_is_factory_new()) {
					alarm_cb = zb_bdb_initiate_tc_rejoin;
					alarm_cb_param = ZB_UNDEFINED_BUFFER;
				}
			}

			zb_err_code = ZB_SCHEDULE_APP_ALARM(
				alarm_cb,
				alarm_cb_param,
				ZB_MILLISECONDS_TO_BEACON_INTERVAL(timeout_s * 1000));

			ZB_ERROR_CHECK(zb_err_code);
			is_rejoin_in_progress = true;
		}
	}
}

/**@brief Function for starting rejoin network procedure.
 *
 * @note  For Router device if stack is initialised, device is not joined
 *        and rejoin procedure is not running, start rejoin procedure.
 *
 * @note  For End Device if stack is initialised, rejoin procedure
 *        is not running, device is not joined and device is not waiting
 *        for the user input, start rejoin procedure. Additionally,
 *        schedule alarm to stop rejoin procedure after the timeout
 *        defined by ZB_DEV_REJOIN_TIMEOUT_MS.
 */
static void start_network_rejoin(void)
{
#if defined CONFIG_ZIGBEE_ROLE_END_DEVICE
	if (!ZB_JOINED() && stack_initialised && !wait_for_user_input) {
#else
	if (!ZB_JOINED() && stack_initialised) {
#endif
		is_rejoin_in_progress = false;

		if (!is_rejoin_procedure_started) {
			is_rejoin_procedure_started = true;
			is_rejoin_stop_requested = false;
			is_rejoin_in_progress = false;
			rejoin_attempt_cnt = 0;

#if defined CONFIG_ZIGBEE_ROLE_END_DEVICE
			wait_for_user_input = false;
			is_rejoin_start_scheduled = false;

			zb_ret_t zb_err_code = ZB_SCHEDULE_APP_ALARM(
				stop_network_rejoin,
				ZB_TRUE,
				ZB_MILLISECONDS_TO_BEACON_INTERVAL(
					ZB_DEV_REJOIN_TIMEOUT_MS));
			ZB_ERROR_CHECK(zb_err_code);
#endif

			LOG_INF("Started network rejoin procedure.");
		}
	}
}

/**@brief Function for stopping rejoin network procedure
 *        and related scheduled alarms.
 *
 * @param[in] was_scheduled   Zigbee flag to indicate if the function
 *                            was scheduled or called directly.
 */
static void stop_network_rejoin(zb_uint8_t was_scheduled)
{
	/* For Router and End Device:
	 *   Try to stop scheduled network steering. Stop rejoin procedure
	 *   or if no network steering was scheduled, request rejoin stop
	 *   on next rejoin_the_network() call.
	 * For End Device only:
	 *   If stop_network_rejoin() was called from scheduler, the rejoin
	 *   procedure has reached timeout, set wait_for_user_input
	 *   to true so the rejoin procedure can only be started by calling
	 *   user_input_indicate(). If not, set wait_for_user_input to false.
	 */

	zb_ret_t zb_err_code;

#if defined CONFIG_ZIGBEE_ROLE_END_DEVICE
	/* Set wait_for_user_input depending on if the device should retry
	 * joining on user_input_indication().
	 */
	wait_for_user_input = was_scheduled;
#else
	ZVUNUSED(was_scheduled);
#endif

	if (is_rejoin_procedure_started) {
		zb_err_code = ZB_SCHEDULE_APP_ALARM_CANCEL(
			start_network_steering,
			ZB_ALARM_ANY_PARAM);
		if (zb_err_code == RET_OK) {
			/* Stop rejoin procedure */
			is_rejoin_procedure_started = false;
			is_rejoin_stop_requested = false;
#if defined CONFIG_ZIGBEE_ROLE_END_DEVICE
			LOG_INF("Network rejoin procedure stopped as %sscheduled.",
				(wait_for_user_input) ? "" : "not ");
#else
			LOG_INF("Network rejoin procedure stopped.");
#endif
		} else {
			/* Request rejoin procedure stop */
			is_rejoin_stop_requested = true;
		}
	}

	if (IS_ENABLED(CONFIG_ZIGBEE_ROLE_END_DEVICE)) {
		/* Make sure scheduled stop alarm is canceled. */
		zb_err_code = ZB_SCHEDULE_APP_ALARM_CANCEL(
			stop_network_rejoin,
			ZB_ALARM_ANY_PARAM);
		if (zb_err_code != RET_NOT_FOUND) {
			ZB_ERROR_CHECK(zb_err_code);
		}
	}
}

#if defined CONFIG_ZIGBEE_ROLE_END_DEVICE
/* Function to be scheduled when user_input_indicate() is called
 * and wait_for_user_input is true.
 */
static void start_network_rejoin_ED(zb_uint8_t param)
{
	ZVUNUSED(param);
	if (!ZB_JOINED() && wait_for_user_input) {
		zb_ret_t zb_err_code;

		wait_for_user_input = false;
		start_network_rejoin();

		zb_err_code = ZB_SCHEDULE_APP_ALARM(
			rejoin_the_network,
			0,
			ZB_TIME_ONE_SECOND);
		ZB_ERROR_CHECK(zb_err_code);
	}
	is_rejoin_start_scheduled = false;
}

/* Function to be called by an application
 * e.g. inside button handler function
 */
void user_input_indicate(void)
{
	if (wait_for_user_input && !(is_rejoin_start_scheduled)) {
		zb_ret_t zb_err_code = RET_OK;

		zb_err_code =
			zigbee_schedule_callback(start_network_rejoin_ED, 0);
		ZB_ERROR_CHECK(zb_err_code);

		/* Prevent scheduling multiple rejoin starts */
		if (!zb_err_code) {
			is_rejoin_start_scheduled = true;
		}
	}
}

/* Function to enable sleepy behavior for End Device. */
void zigbee_configure_sleepy_behavior(bool enable)
{
	if (enable) {
		zb_set_rx_on_when_idle(ZB_FALSE);
		LOG_INF("Enabled sleepy end device behavior.");
	} else {
		zb_set_rx_on_when_idle(ZB_TRUE);
		LOG_INF("Disabling sleepy end device behavior.");
	}
}
#endif /* CONFIG_ZIGBEE_ROLE_END_DEVICE */

#if defined CONFIG_ZIGBEE_FACTORY_RESET

static void factory_reset_timer_expired(struct k_timer *timer_id)
{
	uint32_t button_state = 0;
	uint32_t has_changed = 0;

	dk_read_buttons(&button_state, &has_changed);
	if (button_state & factory_reset_context.button) {
		LOG_DBG("FR button pressed for %d [s]", timer_id->status);
		if (timer_id->status >= CONFIG_FACTORY_RESET_PRESS_TIME_SECONDS) {
			/* Schedule a callback so that Factory Reset is started
			 * from ZBOSS scheduler context
			 */
			LOG_DBG("Schedule Factory Reset; stop timer; set factory_reset_done flag");
			factory_reset_context.pibcache_pan_id_needs_reset = true;
			ZB_SCHEDULE_APP_CALLBACK(zb_bdb_reset_via_local_action, 0);
			k_timer_stop(timer_id);
			factory_reset_context.reset_done = true;
		}
	} else {
		LOG_DBG("FR button released prematurely");
		k_timer_stop(timer_id);
	}
}

void register_factory_reset_button(uint32_t button)
{
	factory_reset_context.button = button;
	factory_reset_context.reset_done = false;
	factory_reset_context.pibcache_pan_id_needs_reset = false;

	k_timer_init(&factory_reset_context.timer, factory_reset_timer_expired, NULL);
}

void check_factory_reset_button(uint32_t button_state, uint32_t has_changed)
{
	if (button_state & has_changed & factory_reset_context.button) {
		LOG_DBG("Clear factory_reset_done flag; start Factory Reset timer");

		/* Reset flag indicating that Factory Reset was initiated */
		factory_reset_context.reset_done = false;
		factory_reset_context.pibcache_pan_id_needs_reset = false;

		/* Start timer checking button press time */
		k_timer_start(&factory_reset_context.timer,
			      FACTORY_RESET_PROBE_TIME,
			      FACTORY_RESET_PROBE_TIME);
	}
}

bool was_factory_reset_done(void)
{
	return factory_reset_context.reset_done;
}
#endif /* CONFIG_ZIGBEE_FACTORY_RESET */
