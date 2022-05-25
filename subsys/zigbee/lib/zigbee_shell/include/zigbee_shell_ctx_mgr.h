/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ZIGBEE_SHELL_CTX_MGR_H__
#define ZIGBEE_SHELL_CTX_MGR_H__

#include <zephyr/shell/shell.h>

#include "zigbee_shell_zdo_types.h"
#include "zigbee_shell_zcl_types.h"

#define CTX_MGR_ENTRY_IVALID_INDEX 0xFF


/* Type of a frame a context manager entry is associated with. */
enum ctx_entry_type {
	CTX_MGR_INVALID_ENTRY_TYPE,
	CTX_MGR_ZDO_ENTRY_TYPE,
	CTX_MGR_PING_REQ_ENTRY_TYPE,
	CTX_MGR_PING_REPLY_ENTRY_TYPE,
	CTX_MGR_ATTR_REQ_ENTRY_TYPE,
	CTX_MGR_CFG_REPORT_REQ_ENTRY_TYPE,
	CTX_MGR_GENERIC_CMD_ENTRY_TYPE,
	CTX_MGR_GROUPS_CMD_ENTRY_TYPE
};

/* A context manager entry structure associated with a given frame,
 * used to store a frame related data and a shell to be used for logging a data.
 * Frame is identified by the type and its unique identifier number:
 *     - ZDO frames are using the transaction number (TSN) as identifier number.
 *     - ZCL frames are identified using the ZCL Sequence number.
 *     - PING frames are using the custom counter, implemented along
 *       the PING commands.
 */
struct ctx_entry {
	bool taken;
	/* Unique identifier of a frame. Type of used identifier depends on
	 * the type of frame the entry is associated with.
	 */
	uint8_t id;
	enum ctx_entry_type type;
	/* Pointer to the shell to be used for printing logs. */
	const struct shell *shell;
	/* Union of structures used to store a frame related data,
	 * shared between the different frame types.
	 */
	union {
		struct zdo_data zdo_data;
		struct zcl_data zcl_data;
	};
};

/**@brief Delete context manager entry to mark it free and prepare to be
 *        reused later.
 *
 * @param ctx_entry  Pointer to the context manager entry to be deleted.
 */
void ctx_mgr_delete_entry(struct ctx_entry *ctx_entry);

/**@brief Get a pointer to a unused context manager entry.
 *
 * @param type       Type of a data to be stored in an entry.
 *
 * @return  Pointer to the context manager entry if an unused entry was found,
 *          NULL otherwise.
 */
struct ctx_entry *ctx_mgr_new_entry(enum ctx_entry_type type);

/**@brief Return a pointer to the context manager entry which is associated with
 *        the given frame type and the given identifier.
 *
 * @param id         Unique identifier of the frame the context manager
 *                   entry is associated with. Type of the identifier used
 *                   in the entry depends on the entry type.
 *
 * @param type       Type of the frame the entry is associated with.
 *
 * @return  Pointer to the context manager entry if the entry was found,
 *          NULL otherwise.
 */
struct ctx_entry *ctx_mgr_find_ctx_entry(uint8_t id, enum ctx_entry_type type);

/**@brief Return an index of the given context manager entry.
 *
 * @param ctx_entry  Pointer to the context manager entry.
 *
 * @return  Index of given context manager entry.
 */
uint8_t ctx_mgr_get_index_by_entry(struct ctx_entry *ctx_entry);

/**@brief Return a pointer to the context manager entry by given index.
 *
 * @param index      Index of the context manager entry.
 *
 * @return  Pointer to the context manager entry, NULL if not found.
 */
struct ctx_entry *ctx_mgr_get_entry_by_index(uint8_t index);

/**@brief Return a pointer to the context manager entry of the given type
 *        and associated with the given buffer ID.
 *
 * @param bufid      Buffer ID associated with the context manager entry.
 *
 * @param type       Type of a data to be stored in an entry.
 *
 * @return  Pointer to the context manager entry if the entry was found, NULL otherwise.
 */
struct ctx_entry *ctx_mgr_find_zcl_entry_by_bufid(zb_bufid_t bufid, enum ctx_entry_type type);

#endif /* ZIGBEE_SHELL_CTX_MGR_H__ */
