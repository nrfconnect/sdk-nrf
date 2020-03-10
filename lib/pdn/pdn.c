/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <pdn.h>
#include <at_cmd.h>
#include <at_notif.h>

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/** Maximum buffer size for AT commands.
 * Max size is defined to contain commands and responses.
 * Longest AT command is CONNECT:
 *   AT+CGDCONT=CID,"FAMILY","APN"
 *   where:
 *     CID     up to 2 octets
 *     FAMILY  up to 6 octets
 *     APN     up to 100 octets
 *   => 11+2+1+8+1+102+2 = 127 octets
 *
 * Longest received response is for PDN_STATE_GET
 *   "+CGACT: 0,1\r\n
 *    ...
 *    +CGACT: 11,1\r\n
 *    OK\r\n"
 *   => 10*(10+2)+2*(11+2)+2+2 = 150 octets
 */
#define PDN_AT_BUFFER_SIZE (192)

#define PDN_ID_INVALID (-1)
#define PDN_CID_INVALID (-1)

#define PDN_IDX_TO_HANDLE(_idx) ((int)_idx + CONFIG_PDN_NUM_CONTEXTS)
#define PDN_HANDLE_TO_IDX(_handle) ((int)_handle - CONFIG_PDN_NUM_CONTEXTS)

/** @brief PDN states
 * Internal definitions to track PDN state
 */
enum pdn_conn_state {
	/** PDN is free. */
	PDN_STATE_FREE = 0,
	/** PDN is allocated. */
	PDN_STATE_ALLOCATED,
	/** PDN is idle/unused. */
	PDN_STATE_IDLE,
	/** PDN is connected to an APN and activated. */
	PDN_STATE_ACTIVE
};

/** @brief AT commands
 * Internal definitions for supported AT command
 */
enum pdn_at_cmd {
	CMD_CONTEXT_CREATE = 0,
	CMD_APN_CONNECT,
	CMD_PDN_ACTIVATE,
	CMD_PDN_ID_GET,
	CMD_PDN_STATE_GET,
	CMD_PDN_DEACTIVATE,
	CMD_APN_DISCONNECT,
	CMD_TIME_GET,
	CMD_MAX
};

/** @brief AT command responses
 * Internal definitions for supported AT command responses
 */
enum pdn_at_evt {
	EVT_CONTEXT_CREATE = 0,
	EVT_PDN_ID_GET,
	EVT_PDN_STATE_GET,
	EVT_TIME_GET,
	EVT_MAX
};

/** Data type for PDN connection. */
struct pdn_connection {
	/* Client defined callback when PDN related nofication is received */
	pdn_notif_handler_t callback;
	/* Last connect / disconnect time */
	struct pdn_time event_time;
	/* PDN ID. */
	s8_t pdn_id;
	/* Indicates the connection state #pdn_conn_state of the PDN. */
	u8_t state;
	/* Supported address family. @ref #pdn_inet_type */
	u8_t family;
	/* Modem assigned identifier for uniquely identifying a PDN.  */
	s8_t context_id;
	/* Network error cause */
	u8_t nw_error;
};

/* Local PDN context container. */
static struct pdn_connection pdn_contexts[CONFIG_PDN_NUM_CONTEXTS];

static char pdn_at_cmd_buffer[PDN_AT_BUFFER_SIZE];

static const char *const pdn_at_cmds_list[CMD_MAX] = {
	[CMD_CONTEXT_CREATE] = "AT%XNEWCID?",
	[CMD_APN_CONNECT] = "AT+CGDCONT=",
	[CMD_PDN_ACTIVATE] = "AT+CGACT=1,",
	[CMD_PDN_ID_GET] = "AT%XGETPDNID=",
	[CMD_PDN_STATE_GET] = "AT+CGACT?",
	[CMD_PDN_DEACTIVATE] = "AT+CGACT=0,",
	[CMD_APN_DISCONNECT] = "AT+CGDCONT=",
	[CMD_TIME_GET] = "AT+CCLK?"
};

typedef int (*pdn_event_parser_t)(struct pdn_connection *const pdn_context,
				  const char *const at_event);

static int context_create_parser(struct pdn_connection *const pdn_context,
				 const char *const at_event);
static int pdn_id_get_parser(struct pdn_connection *const pdn_context,
			     const char *const at_event);
static int pdn_state_get_parser(struct pdn_connection *const pdn_context,
				const char *const at_event);
static int time_get_parser(struct pdn_connection *const pdn_context,
			   const char *const at_event);

struct pdn_at_evt_t {
	const char *at_str;
	pdn_event_parser_t parser_fn;
};

static const struct pdn_at_evt_t pdn_at_evts_list[EVT_MAX] = {
	[EVT_CONTEXT_CREATE] = { .at_str = "%XNEWCID:",
				 .parser_fn = context_create_parser },
	[EVT_PDN_ID_GET] = { .at_str = "%XGETPDNID:",
			     .parser_fn = pdn_id_get_parser },
	[EVT_PDN_STATE_GET] = { .at_str = "+CGACT:",
				.parser_fn = pdn_state_get_parser },
	[EVT_TIME_GET] = { .at_str = "+CCLK:", .parser_fn = time_get_parser }
};

static int index_from_handle(int handle)
{
	handle = PDN_HANDLE_TO_IDX(handle);
	if ((handle >= 0) && (handle < CONFIG_PDN_NUM_CONTEXTS)) {
		if (pdn_contexts[handle].state != PDN_STATE_FREE) {
			return handle;
		}
	}
	return -1;
}

static void at_notif_handler(void *context, const char *response)
{
	const char *sub_str;
	s8_t cid = PDN_CID_INVALID;
	const int index = index_from_handle((int)context);
	enum pdn_event event = PDN_EVENT_NA;

	/* Correct responses are at least 14 characters long
	 * skip all shorter strings
	 */
	if ((index < 0) || (response == NULL) || (strlen(response) < 14)) {
		return;
	}

	struct pdn_connection *const pdn_context =
		(struct pdn_connection *)&pdn_contexts[index];

	if (strstr(response, "CNEC_ESM") != NULL) {
		/* AT event: +CNEC_ESM: <cause>,<cid> */
		u8_t nw_error = 0;

		sub_str = strchr(response, ':');
		if (sub_str != NULL) {
			nw_error = atoi(sub_str + 2);
		}

		sub_str = strchr(sub_str, ',');
		if (sub_str != NULL) {
			cid = atoi(sub_str + 1);
		}

		/* Check if context id match */
		if (cid == pdn_context->context_id) {
			/* Store network failure cause */
			pdn_context->nw_error = nw_error;
			event = PDN_EVENT_NW_ERROR;
		}
	} else if (strstr(response, "CGEV: ME PDN") != NULL) {
		/* AT event: +CGEV: ME PDN ACT <cid> */
		sub_str = strstr(response, "ACT ");
		if (sub_str != NULL) {
			cid = atoi(sub_str + 4);
			/* Update if context id is correct */
			if (cid == pdn_context->context_id) {
				/* PDN deactivated */
				pdn_context->state = PDN_STATE_IDLE;
				event = PDN_EVENT_DEACTIVATED;
			}
		}
	}

	if ((event != PDN_EVENT_NA) && (pdn_context->callback != NULL)) {
		pdn_context->callback(index, event);
	}
}

static int local_context_reserve(const enum pdn_inet_type family)
{
	/* Search free PDN context index */
	int i;

	for (i = 0; i < CONFIG_PDN_NUM_CONTEXTS; i++) {
		struct pdn_connection *const pdn_context = &pdn_contexts[i];

		if (pdn_context->state == PDN_STATE_FREE) {
			/* Clear any existing data */
			memset(pdn_context, 0, sizeof(struct pdn_connection));

			pdn_context->state = PDN_STATE_ALLOCATED;
			pdn_context->family = (family == PDN_INET_DEFAULT) ?
						      PDN_INET_IPV4V6 :
						      family;
			/* Set context id to invalid */
			pdn_context->context_id = PDN_CID_INVALID;
			/* Set PDN to indicate invalid/unread value. */
			pdn_context->pdn_id = PDN_ID_INVALID;

			/* Register AT event handler */
			at_notif_register_handler((void *)PDN_IDX_TO_HANDLE(i),
						  at_notif_handler);

			return i;
		}
	}

	return -ENOMEM;
}

static void local_context_release(const int index)
{
	struct pdn_connection *const pdn_context = &pdn_contexts[index];

	/* Remove AT event handler */
	at_notif_deregister_handler((void *)PDN_IDX_TO_HANDLE(index),
				    at_notif_handler);

	/* Mark to free */
	pdn_context->callback = NULL;
	pdn_context->state = PDN_STATE_FREE;
	pdn_context->context_id = PDN_CID_INVALID;
	pdn_context->pdn_id = PDN_ID_INVALID;
}

static int command_response_parse(struct pdn_connection *const pdn_context,
				  char *const resp,
				  const enum pdn_at_evt evt_id)
{
	const char *const compare_str = pdn_at_evts_list[evt_id].at_str;
	const int compare_length = strlen(compare_str);

	/* Check if a recieved response is the resp to sent command.
	 * Response should be always longer than compare_str to make sure
	 * it contains a data as well. That's why use expected str len
	 * to match the correct response.
	 */
	if (strncmp(compare_str, resp, compare_length) == 0) {
		/* Found the event we are waiting for.
		 * Parse parameters if any and return.
		 */
		return pdn_at_evts_list[evt_id].parser_fn(pdn_context, resp);
	}

	/* Invalid response received */
	return -ENOTSUP;
}

static int command_prepare_and_send(struct pdn_connection *const pdn_context,
				    const enum pdn_at_cmd cmd_id,
				    const char *const remote_addr)
{
	int err = 0;
	int offset = 0;
	const int context_id =
		(pdn_context != NULL) ? pdn_context->context_id : 0;
	enum pdn_at_evt resp_evt = EVT_MAX;

	char *const command_str = pdn_at_cmd_buffer;

	if (cmd_id < CMD_MAX) {
		/* write base if valid command, error is returned from
		 * switch default.
		 */
		offset = snprintf(&command_str[offset],
				  (PDN_AT_BUFFER_SIZE - offset), "%s",
				  pdn_at_cmds_list[cmd_id]);
	}

	switch (cmd_id) {
	case CMD_TIME_GET: {
		/* No additional parameters. */
		resp_evt = EVT_TIME_GET;
		break;
	}
	case CMD_CONTEXT_CREATE: {
		/* No additional parameters. */
		resp_evt = EVT_CONTEXT_CREATE;
		break;
	}
	case CMD_PDN_STATE_GET: {
		/* No additional parameters. */
		resp_evt = EVT_PDN_STATE_GET;
		break;
	}
	case CMD_APN_CONNECT: {
		/* Pack the context identifier.
		 * Pack the IP address family string.
		 * Pack the APN gateway string.
		 */

		offset += snprintf(&command_str[offset],
				   (PDN_AT_BUFFER_SIZE - offset), "%d,",
				   context_id);

		const enum pdn_inet_type family = pdn_context->family;
		char *family_string;

		if (family == PDN_INET_IPV4V6) {
			family_string = "IPV4V6";
		} else if (family == PDN_INET_IPV6) {
			family_string = "IPV6";
		} else if (family == PDN_INET_IPV4) {
			family_string = "IP";
		} else {
			return -EFAULT;
		}

		offset += snprintf(&command_str[offset],
				   (PDN_AT_BUFFER_SIZE - offset),
				   "\"%s\",\"%s\"", family_string, remote_addr);
		break;
	}
	case CMD_PDN_ID_GET:
		resp_evt = EVT_PDN_ID_GET;
		/* Note: Fall through to pack the CID */
	case CMD_PDN_ACTIVATE:
	case CMD_PDN_DEACTIVATE:
	case CMD_APN_DISCONNECT: {
		/* Pack the context identifier. */
		offset += snprintf(&command_str[offset],
				   (PDN_AT_BUFFER_SIZE - offset), "%d",
				   context_id);
		break;
	}
	default: {
		/* not a valid command, return error */
		return -ENOTSUP;
	}
	}

	if (offset > PDN_AT_BUFFER_SIZE) {
		return -E2BIG;
	}

	/* Send AT command and wait response */
	err = at_cmd_write(command_str, command_str, PDN_AT_BUFFER_SIZE, NULL);
	if (err < 0) {
		return err;
	}

	/* Parse response. OK / ERROR is already handled by at_cmd */
	if (resp_evt != EVT_MAX) {
		return command_response_parse(pdn_context, command_str,
					      resp_evt);
	}

	return 0;
}

static int context_create_parser(struct pdn_connection *const pdn_context,
				 const char *const at_event)
{
	const char *ptr_to_colon = strchr(at_event, ':');
	int context_id = PDN_CID_INVALID;

	if (ptr_to_colon != NULL) {
		context_id = atoi(ptr_to_colon + 1);

		pdn_context->context_id = context_id;
	}

	return context_id;
}

static int pdn_id_get_parser(struct pdn_connection *const pdn_context,
			     const char *const at_event)
{
	const char *ptr_to_colon = strchr(at_event, ':');

	if (ptr_to_colon != NULL) {
		pdn_context->pdn_id = atoi(ptr_to_colon + 1);
		return 0;
	}

	pdn_context->pdn_id = PDN_ID_INVALID;
	return -1;
}

static int pdn_state_get_parser(struct pdn_connection *const pdn_context,
				const char *const at_event)
{
	const int context_id =
		(pdn_context != NULL) ? pdn_context->context_id : 0;
	char buf[16] = { 0 };

	/* Search for string +CGACT: <cid>,<state> */
	snprintf(buf, sizeof(buf), "+CGACT: %d,1", context_id);

	if (strstr(at_event, buf) == NULL) {
		/* PDN is disconnected. */
		return PDN_INACTIVE;
	}

	/* PDN is connected. */
	return PDN_ACTIVE;
}

static int time_get_parser(struct pdn_connection *const pdn_context,
			   const char *const at_event)
{
	struct pdn_time *const time = &pdn_context->event_time;
	s8_t time_zone = PDN_INVALID_TZ;
	const char *time_str = strstr(at_event, ": \"");

	if (time_str != NULL) {
		time_str += 3;

		/* string format: +CCLK: "17/04/07,12:57:43+03" */

		/* parse date */
		time->year = atoi(time_str);
		time_str += 3;
		time->mon = atoi(time_str);
		time_str += 3;
		time->day = atoi(time_str);
		time_str += 3;

		/* parse time */
		time->hour = atoi(time_str);
		time_str += 3;
		time->min = atoi(time_str);
		time_str += 3;
		time->sec = atoi(time_str);
		time_str += 2;

		/* parse time zone */
		if (*time_str == '+') {
			time_zone = atoi(++time_str);
		} else if (*time_str == '-') {
			time_zone = -atoi(++time_str);
		}
		time->tz = time_zone;

		return 0;
	}

	return -ENOTSUP;
}

static int cmd_pdn_id_get(struct pdn_connection *const pdn_context)
{
	if (pdn_context->state == PDN_STATE_ACTIVE) {
		if (pdn_context->pdn_id == PDN_ID_INVALID) {
			return command_prepare_and_send(pdn_context,
							CMD_PDN_ID_GET, NULL);
		}
	}
	return pdn_context->pdn_id;
}

static int cmd_apn_connect(struct pdn_connection *const pdn_context,
			   const char *const remote_addr)
{
	int err = command_prepare_and_send(pdn_context, CMD_APN_CONNECT,
					   remote_addr);
	if (err >= 0) {
		pdn_context->state = PDN_STATE_IDLE;
	}
	return err;
}

static int cmd_pdn_activate(struct pdn_connection *const pdn_context)
{
	int err;

	if (pdn_context->state == PDN_STATE_ACTIVE) {
		return 0;
	}

	err = command_prepare_and_send(pdn_context, CMD_PDN_ACTIVATE, NULL);
	if (err >= 0) {
		pdn_context->state = PDN_STATE_ACTIVE;
		/* Query time from the modem */
		err = command_prepare_and_send(pdn_context, CMD_TIME_GET, NULL);
		/* Query PDN ID */
		(void)cmd_pdn_id_get(pdn_context);
	}
	return err;
}

static int cmd_pdn_deactivate(struct pdn_connection *const pdn_context)
{
	int err;

	if (pdn_context->state == PDN_STATE_IDLE) {
		return 0;
	}

	err = command_prepare_and_send(pdn_context, CMD_PDN_DEACTIVATE, NULL);
	if (err >= 0) {
		pdn_context->state = PDN_STATE_IDLE;
		pdn_context->pdn_id = PDN_ID_INVALID;
		/* Query time from the modem */
		err = command_prepare_and_send(pdn_context, CMD_TIME_GET, NULL);
	}

	return err;
}

static int cmd_pdn_disconnect(struct pdn_connection *const pdn_context)
{
	int err =
		command_prepare_and_send(pdn_context, CMD_APN_DISCONNECT, NULL);
	if (err >= 0) {
		pdn_context->state = PDN_STATE_ALLOCATED;
	}
	return err;
}

static int cmd_pdn_state_get(struct pdn_connection *const pdn_context)
{
	int res;

	/* Return local state if disconnected */
	if (pdn_context->state < PDN_STATE_IDLE) {
		return PDN_DISCONNECTED;
	}

	/* Get current state from modem */
	res = command_prepare_and_send(pdn_context, CMD_PDN_STATE_GET, NULL);
	if (res < 0) {
		return -1;
	}

	/* Update local state if there is a mismatch */
	switch (res) {
	case PDN_INACTIVE:
		pdn_context->state = PDN_STATE_IDLE;
		break;
	case PDN_ACTIVE:
		pdn_context->state = PDN_STATE_ACTIVE;
		break;
	default:
		break;
	}

	return res;
}

static const int context_create(const enum pdn_inet_type family)
{
	int err;
	const int handle = local_context_reserve(family);

	if (handle >= 0) {
		/* Request a context identifier from the modem. */
		err = command_prepare_and_send(&pdn_contexts[handle],
					       CMD_CONTEXT_CREATE, NULL);
		if (err < 0) {
			/* No more contexts available */
			local_context_release(handle);
			return err;
		}
	}

	return handle;
}

static void context_free(const int handle)
{
	(void)cmd_pdn_disconnect(&pdn_contexts[handle]);
	local_context_release(handle);
}

int pdn_connect(const char *const remote_addr, const enum pdn_inet_type family)
{
	int err;
	const unsigned int addrlen = strlen(remote_addr);

	if ((remote_addr == NULL) || (addrlen == 0) ||
	    (family < PDN_INET_DEFAULT) || (family > PDN_INET_IPV4V6)) {
		return -EINVAL;
	}

	int index = context_create(family);

	if (index >= 0) {
		err = cmd_apn_connect(&pdn_contexts[index], remote_addr);
		if (err < 0) {
			/* error in connect, free local and modem contexts */
			context_free(index);
			return err;
		}
		/* Convert context index to handle */
		index = PDN_IDX_TO_HANDLE(index);
	}

	return index;
}

int pdn_disconnect(const int handle)
{
	const int index = index_from_handle(handle);

	if (index < 0) {
		return -EBADF;
	}

	/* Command modem to deactivate the PDN */
	(void)cmd_pdn_deactivate(&pdn_contexts[index]);

	/* Command modem to disconnect the PDN,
	 * Note: also PDN context is deleted in disconnect
	 */
	context_free(index);

	return 0;
}

int pdn_activate(const int handle)
{
	if (handle == 0) {
		/* Activate default PDN */
		return command_prepare_and_send(NULL, CMD_PDN_ACTIVATE, NULL);
	}

	const int index = index_from_handle(handle);

	if (index < 0) {
		return -EBADF;
	}

	return cmd_pdn_activate(&pdn_contexts[index]);
}

int pdn_deactivate(const int handle)
{
	const int index = index_from_handle(handle);

	if (index < 0) {
		return -EBADF;
	}

	return cmd_pdn_deactivate(&pdn_contexts[index]);
}

int pdn_family_get(const int handle)
{
	const int index = index_from_handle(handle);

	if (index < 0) {
		return -EBADF;
	}

	return pdn_contexts[index].family;
}

int pdn_state_get(const int handle)
{
	if (handle == 0) {
		/* Query default PDN state */
		return command_prepare_and_send(NULL, CMD_PDN_STATE_GET, NULL);
	}

	const int index = index_from_handle(handle);

	if (index < 0) {
		return -EBADF;
	}

	return cmd_pdn_state_get(&pdn_contexts[index]);
}

int pdn_context_id_get(const int handle)
{
	const int index = index_from_handle(handle);

	if (index < 0) {
		return -EBADF;
	}

	return pdn_contexts[index].context_id;
}

int pdn_id_get(const int handle)
{
	const int index = index_from_handle(handle);

	if (index < 0) {
		return -EBADF;
	}

	return cmd_pdn_id_get(&pdn_contexts[index]);
}

int pdn_register_handler(const int handle, const pdn_notif_handler_t callback)
{
	const int index = index_from_handle(handle);

	if (index < 0) {
		return -EBADF;
	}

	if (callback == NULL) {
		return -EINVAL;
	}

	pdn_contexts[index].callback = callback;

	return 0;
}

int pdn_deregister_handler(const int handle)
{
	const int index = index_from_handle(handle);

	if (index < 0) {
		return -EBADF;
	}

	pdn_contexts[index].callback = NULL;

	return 0;
}

int pdn_get_time(const int handle, struct pdn_time *const time)
{
	const int index = index_from_handle(handle);

	if (index < 0) {
		return -EBADF;
	}

	if (time == NULL) {
		return -EINVAL;
	}

	const struct pdn_connection *const pdn_context =
		(struct pdn_connection *)&pdn_contexts[index];

	if (pdn_context->state >= PDN_STATE_IDLE) {
		memcpy(time, &pdn_context->event_time, sizeof(struct pdn_time));
		return 0;
	}

	return -1;
}
