/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <kernel.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include <bluetooth/services/hids_c.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(hids_c, CONFIG_BT_GATT_HIDS_C_LOG_LEVEL);

/* Real report structure definition */
struct bt_gatt_hids_c_rep_info {
	/** HIDS client object
	 *  @note Do not use it directly as it may be changed to some kind of
	 *        connection context management
	 */
	struct bt_gatt_hids_c *hids_c;
	bt_gatt_hids_c_read_cb  read_cb;   /**< Read function callback        */
	bt_gatt_hids_c_write_cb write_cb;  /**< Write function callback       */
	bt_gatt_hids_c_read_cb  notify_cb; /**< Notification function (Input) */
	struct bt_gatt_read_params      read_params;   /**< Read params   */
	struct bt_gatt_write_params     write_params;  /**< Write params  */
	struct bt_gatt_subscribe_params notify_params; /**< Notify params */

	/** User data to be used freely by the application */
	void *user_data;
	/** Handlers */
	struct {
		u16_t ref; /**< Report Reference handler */
		u16_t val; /**< Value handler            */
		u16_t ccc; /**< CCC handler (Input)      */
	} handlers;
	/** Report reference information */
	struct {
		u8_t id;                              /**< Report identifier */
		enum bt_gatt_hids_report_type type; /**< Report type       */
	} ref;
	u8_t size; /**< The size of the value */
};

/* Memory slab used for reports */
K_MEM_SLAB_DEFINE(bt_gatt_hids_c_reports_mem,
		  sizeof(struct bt_gatt_hids_c_rep_info),
		  CONFIG_BT_GATT_HIDS_C_REPORTS_MAX,
		  4);


/**
 * @brief Allocate memory for report structure
 *
 * Function allocates memory for report structure.
 * The structure would be cleared.
 *
 * @return The pointer to the structure or NULL if no memory is available.
 *
 * @sa rep_new
 */
static struct bt_gatt_hids_c_rep_info *rep_alloc(void)
{
	struct bt_gatt_hids_c_rep_info *rep;

	if (k_mem_slab_alloc(&bt_gatt_hids_c_reports_mem,
			     (void **)&rep, K_NO_WAIT)) {
		rep = NULL;
	} else {
		memset(rep, 0, sizeof(*rep));
	}
	return rep;
}

/**
 * @brief Free the allocated memory for report
 *
 * Frees the memory allocated for report and stores NULL in the pointed
 * report pointer.
 *
 * @param repp Pointer to the report.
 */
static void rep_free(struct bt_gatt_hids_c_rep_info **repp)
{
	k_mem_slab_free(&bt_gatt_hids_c_reports_mem, (void **)repp);
	*repp = NULL;
}

/**
 * @brief Create new report structure
 *
 * Allocate memory for new report and fill it with data from given report
 * characteristic.
 *
 * @param hids_c   HIDS client object.
 * @param repp     Pointer with would be set to newly created report.
 * @param dm       Discovery object.
 * @param rep_chrc Report characteristic.
 *
 * @return 0 or negative error code.
 *
 * @sa rep_alloc
 * @sa rep_free
 *
 * @note If error is found during report initialization,
 *       allocated report memory is freed.
 */
static int rep_new(struct bt_gatt_hids_c *hids_c,
		   struct bt_gatt_hids_c_rep_info **repp,
		   const struct bt_gatt_dm *dm,
		   const struct bt_gatt_dm_attr *rep_chrc)
{
	struct bt_gatt_hids_c_rep_info *rep;
	const struct bt_gatt_chrc *chrc_val;
	const struct bt_gatt_dm_attr *gatt_desc;

	rep = rep_alloc();
	if (!rep) {
		LOG_ERR("No memory to create new report");
		return -ENOMEM;
	}
	chrc_val = bt_gatt_dm_attr_chrc_val(rep_chrc);
	__ASSERT_NO_MSG(chrc_val); /* This is internal function and it has to
				    * be called with characteristic attribute
				    */
	/* Guess the type depending on the flags */
	if (chrc_val->properties & BT_GATT_CHRC_WRITE_WITHOUT_RESP) {
		/* Only Out Report has this properties */
		rep->ref.type = BT_GATT_HIDS_REPORT_TYPE_OUTPUT;
	} else if (chrc_val->properties & BT_GATT_CHRC_NOTIFY) {
		/* Only In Report has this properties */
		rep->ref.type = BT_GATT_HIDS_REPORT_TYPE_INPUT;
		/* And it has CCC descriptor */
		gatt_desc = bt_gatt_dm_desc_by_uuid(dm, rep_chrc,
						    BT_UUID_GATT_CCC);
		if (!gatt_desc) {
			LOG_ERR("Missing Input Report CCC.");
			rep_free(&rep);
			return -EINVAL;
		}
		rep->handlers.ccc = gatt_desc->handle;
	} else {
		/* It has to be feature report */
		rep->ref.type = BT_GATT_HIDS_REPORT_TYPE_FEATURE;
	}
	/* Get common report handlers */
	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, rep_chrc,
					    BT_UUID_HIDS_REPORT_REF);
	if (!gatt_desc) {
		/* Not required for BOOT report */
		if (!bt_uuid_cmp(chrc_val->uuid, BT_UUID_HIDS_REPORT)) {
			LOG_ERR("Missing Report Reference.");
			rep_free(&rep);
			return -EINVAL;
		}
	} else {
		rep->handlers.ref = gatt_desc->handle;
	}
	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, rep_chrc, chrc_val->uuid);
	if (!gatt_desc) {
		LOG_ERR("Missing Report Value.");
		rep_free(&rep);
		return -EINVAL;
	}
	rep->handlers.val = gatt_desc->handle;
	/* Remember the parent object */
	rep->hids_c = hids_c;

	*repp = rep;
	return 0;
}

/**
 * @brief Create new array of pointers to the reports
 *
 * Create array of pointers to reports of given number of entities.
 *
 * @param cnt Array size.
 *
 * @return Pointer to the array of NULL if no memory is available.
 *
 * @sa repptr_array_free
 */
static struct bt_gatt_hids_c_rep_info **repptr_array_new(size_t cnt)
{
	struct bt_gatt_hids_c_rep_info **repp = k_malloc(cnt * sizeof(void *));

	if (repp) {
		memset(repp, 0, cnt * sizeof(*repp));
	}
	return repp;
}

/**
 * @brief Free given array
 *
 * Frees the memory taken by @ref repptr_array_new.
 *
 * @param repp Pointer to the array.
 */
static void repptr_array_free(struct bt_gatt_hids_c_rep_info **repp)
{
	k_free(repp);
}

/**
 * @brief Get characteristic value handle by its UUID
 *
 * Function searches for characteristic with selected UUID
 * and then the value inside this characteristic.
 *
 * It is used only for required characteristic.
 *
 * @param dm     Discovery object.
 * @param hids_c HIDS client object.
 * @param uuid   UUID of the characteristic.
 *
 * @return Handle of the characteristic value of 0 if it cannot be found.
 */
static u16_t chrc_value_handle_by_uuid(struct bt_gatt_dm *dm,
				       struct bt_gatt_hids_c *hids_c,
				       const struct bt_uuid *uuid)
{
	const struct bt_gatt_dm_attr *gatt_chrc;
	const struct bt_gatt_dm_attr *gatt_desc;

	gatt_chrc = bt_gatt_dm_char_by_uuid(dm, uuid);
	if (!gatt_chrc) {
		return 0;
	}
	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, uuid);
	if (!gatt_desc) {
		LOG_ERR("No characteristic value found.");
		return 0;
	}
	return gatt_desc->handle;
}

/**
 * @brief Mark hids ready to work
 *
 * This function marks ready flags and calls required callbacks.
 * It releases parameter read semaphore.
 *
 * @param hids_c HIDS client object.
 */
static void hids_mark_ready(struct bt_gatt_hids_c *hids_c)
{
	k_sem_give(&hids_c->read_params_sem);
	hids_c->ready = true;
	if (hids_c->ready_cb) {
		hids_c->ready_cb(hids_c);
	}
}

/**
 * @brief Call error callback
 *
 * This function calls HIDS client callback for error.
 * It releases parameter read semaphore.
 *
 * @param hids_c HIDS client object.
 * @param err    Error code.
 */
static void hids_prep_error(struct bt_gatt_hids_c *hids_c, int err)
{
	k_sem_give(&hids_c->read_params_sem);
	if (hids_c->prep_error_cb) {
		hids_c->prep_error_cb(hids_c, err);
	}
}

/**
 * @brief Process protocol mode read
 *
 * This function processed protocol mode read that is started when
 * HID server is connected.
 *
 * @param conn   Connection handler.
 * @param err    Read ATT error code.
 * @param params Notification parameters structure - the pointer
 *               to the structure provided to read function.
 * @param data   Pointer to the data buffer.
 * @param length The size of the received data.
 *
 * @retval BT_GATT_ITER_STOP     Stop notification
 * @retval BT_GATT_ITER_CONTINUE Continue notification
 */
static u8_t pm_read_process(struct bt_conn *conn, u8_t err,
			    struct bt_gatt_read_params *params,
			    const void *data, u16_t length);

/**
 * @brief Start protocol mode read
 *
 * Function that starts protocol mode read at the HID connection.
 * @note
 * Read semaphore should be already taken in @ref repref_read_start.
 *
 * @param hids_c  See @ref bt_gatt_hids_c_handles_assign.
 *
 * @return 0 or negative error value.
 */
static int pm_read_start(struct bt_gatt_hids_c *hids_c)
{
	int err;

	__ASSERT_NO_MSG(hids_c);
	if (hids_c->handlers.pm == 0) {
		LOG_DBG("Device ready without boot protocol");
		hids_mark_ready(hids_c);
		return 0;
	}
	LOG_DBG("PM read start");
	hids_c->read_params.func = pm_read_process;
	hids_c->read_params.handle_count  = 1;
	hids_c->read_params.single.handle = hids_c->handlers.pm;
	hids_c->read_params.single.offset = 0;
	err = bt_gatt_read(hids_c->conn, &(hids_c->read_params));
	if (err) {
		LOG_ERR("PM read error (err: %d)", err);
		return err;
	}
	return 0;
}

static u8_t pm_read_process(struct bt_conn *conn, u8_t err,
			    struct bt_gatt_read_params *params,
			    const void *data, u16_t length)
{
	struct bt_gatt_hids_c *hids_c;

	hids_c = CONTAINER_OF(params,
			      struct bt_gatt_hids_c,
			      read_params);

	if (err) {
		LOG_ERR("PM read error");
		hids_prep_error(hids_c, err);
		return BT_GATT_ITER_STOP;
	}
	if (length != 1 || !data) {
		LOG_ERR("Unexpected PM size");
		hids_prep_error(hids_c, -ENOTSUP);
		return BT_GATT_ITER_STOP;
	}

	hids_c->pm = (enum bt_gatt_hids_pm)((u8_t *)data)[0];
	LOG_DBG("Read PM success: %d", (int)hids_c->pm);
	hids_mark_ready(hids_c);
	return BT_GATT_ITER_STOP;
}

/**
 * @brief Process report reference read
 *
 * Report reference read is started after connection to HID server.
 *
 * @param conn   Connection handler.
 * @param err    Read ATT error code.
 * @param params Notification parameters structure - the pointer
 *               to the structure provided to read function.
 * @param data   Pointer to the data buffer.
 * @param length The size of the received data.
 *
 * @retval BT_GATT_ITER_STOP     Stop notification
 * @retval BT_GATT_ITER_CONTINUE Continue notification
 */
static u8_t repref_read_process(struct bt_conn *conn, u8_t err,
				struct bt_gatt_read_params *params,
				const void *data, u16_t length);

/**
 * @brief Start Report Reference read
 *
 * Internal function that initializes report reference value read.
 *
 * @param hids_c  See @ref bt_gatt_hids_c_handles_assign.
 * @param rep_idx Index in the report array
 *
 * @return 0 or negative error value.
 */
static int repref_read_start(struct bt_gatt_hids_c *hids_c, size_t rep_idx)
{
	int err;
	struct bt_gatt_hids_c_rep_info *rep;

	__ASSERT_NO_MSG(hids_c);
	if (rep_idx >= hids_c->rep_cnt) {
		err = pm_read_start(hids_c);
		if (err) {
			LOG_ERR("Cannot start boot protocol read (err: %d)",
				err);
		}
		return err;
	}
	LOG_DBG("Report (id: %u) reference read start", rep_idx);
	rep = hids_c->rep_info[rep_idx];
	hids_c->init_repref.rep_idx = rep_idx;
	hids_c->read_params.func = repref_read_process;
	hids_c->read_params.handle_count  = 1;
	hids_c->read_params.single.handle = rep->handlers.ref;
	hids_c->read_params.single.offset = 0;
	err = bt_gatt_read(hids_c->conn, &(hids_c->read_params));
	if (err) {
		LOG_ERR("Report reference read error (err: %d)", err);
		return err;
	}
	return 0;
}

static u8_t repref_read_process(struct bt_conn *conn, u8_t err,
				struct bt_gatt_read_params *params,
				const void *data, u16_t length)
{
	int ret;
	struct bt_gatt_hids_c *hids_c;
	struct bt_gatt_hids_c_rep_info *rep;
	size_t rep_idx;
	const u8_t *bdata = data;

	hids_c = CONTAINER_OF(params,
			      struct bt_gatt_hids_c,
			      read_params);

	rep_idx = hids_c->init_repref.rep_idx;
	__ASSERT_NO_MSG(rep_idx < hids_c->rep_cnt);

	if (err) {
		LOG_ERR("Report (idx: %u) reference read error (err: %d)",
			rep_idx, err);
		hids_prep_error(hids_c, err);
		return BT_GATT_ITER_STOP;
	}
	if (length != 2 || !data) {
		LOG_ERR("Report (idx: %u) reference unexpected size (%u)",
			rep_idx, length);
		hids_prep_error(hids_c, -ENOTSUP);
		return BT_GATT_ITER_STOP;
	}

	rep = hids_c->rep_info[rep_idx];
	if ((u8_t)rep->ref.type != bdata[1]) {
		LOG_ERR("Unexpected report type (%u while expecting %u)",
			bdata[1], rep->ref.type);
		hids_prep_error(hids_c, -EINVAL);
		return BT_GATT_ITER_STOP;
	}
	rep->ref.id = bdata[0];
	LOG_DBG("Report reference read (idx: %u, id: %u)",
		rep_idx, rep->ref.id);


	/* Next */
	ret = repref_read_start(hids_c, rep_idx + 1);
	if (ret) {
		hids_prep_error(hids_c, ret);
		return BT_GATT_ITER_STOP;
	}

	return BT_GATT_ITER_STOP;
}

/**
 * @brief HIDS information read
 *
 * HIDS information read is started after connection to HID server.
 *
 * @param conn   Connection handler.
 * @param err    Read ATT error code.
 * @param params Notification parameters structure - the pointer
 *               to the structure provided to read function.
 * @param data   Pointer to the data buffer.
 * @param length The size of the received data.
 *
 * @retval BT_GATT_ITER_STOP     Stop notification
 * @retval BT_GATT_ITER_CONTINUE Continue notification
 */
static u8_t hid_info_read_process(struct bt_conn *conn, u8_t err,
				   struct bt_gatt_read_params *params,
				   const void *data, u16_t length);

static int hid_info_read_start(struct bt_gatt_hids_c *hids_c)
{
	int err;

	__ASSERT_NO_MSG(hids_c);
	if (hids_c->handlers.info == 0) {
		LOG_ERR("Device ready without HID information characteristic");
		return -EINVAL;
	}
	LOG_DBG("HID information read start");
	hids_c->read_params.func = hid_info_read_process;
	hids_c->read_params.handle_count  = 1;
	hids_c->read_params.single.handle = hids_c->handlers.info;
	hids_c->read_params.single.offset = 0;
	err = bt_gatt_read(hids_c->conn, &(hids_c->read_params));
	if (err) {
		LOG_ERR("HID information read error (err: %d)", err);
		return err;
	}
	return 0;
}

static u8_t hid_info_read_process(struct bt_conn *conn, u8_t err,
				   struct bt_gatt_read_params *params,
				   const void *data, u16_t length)
{
	struct bt_gatt_hids_c *hids_c;
	const u8_t *bdata = data;

	hids_c = CONTAINER_OF(params,
			      struct bt_gatt_hids_c,
			      read_params);

	if (err) {
		LOG_ERR("HID information read error");
		hids_prep_error(hids_c, err);
		return BT_GATT_ITER_STOP;
	}
	if (length != 4 || !data) {
		LOG_ERR("Unexpected HID information size: %u", length);
		hids_prep_error(hids_c, -ENOTSUP);
		return BT_GATT_ITER_STOP;
	}

	hids_c->info_val.bcd_hid = (u16_t)bdata[0] | (((u16_t)bdata[1]) << 8);
	hids_c->info_val.b_country_code = bdata[2];
	hids_c->info_val.flags = bdata[3];

	LOG_DBG("HID information success:");
	LOG_DBG("  bcdHID: %x", hids_c->info_val.bcd_hid);
	LOG_DBG("  bCountryCode: 0x%x", hids_c->info_val.b_country_code);
	LOG_DBG("  Flags: 0x%x", hids_c->info_val.flags);

	err = repref_read_start(hids_c, 0);
	if (err) {
		hids_prep_error(hids_c, err);
	}

	return BT_GATT_ITER_STOP;
}

/**
 * @brief Start anything that should be started after discovery
 *
 * This function is called when handles assign function finishes and
 * starts the reading of the additional data from the HID server
 * that is required for HIDS client to work.
 *
 * @param hids_c  See @ref bt_gatt_hids_c_handles_assign.
 *
 * @return 0 or negative error value.
 */
static int post_discovery_start(struct bt_gatt_hids_c *hids_c)
{
	int err;

	__ASSERT_NO_MSG(hids_c);
	err = k_sem_take(&hids_c->read_params_sem, K_NO_WAIT);
	if (err) {
		return err;
	}

	err = hid_info_read_start(hids_c);
	if (err) {
		k_sem_give(&hids_c->read_params_sem);
		return err;
	}

	return 0;
}

/**
 * @brief Auxiliary internal function for handles assign
 *
 * This is internal function for @ref bt_gatt_hids_c_handles_assign.
 * It takes care about detecting and saving all the handlers.
 * The only thing it is missing is releasing memory on error and calling
 * error callback.
 *
 * @param dm     See @ref bt_gatt_hids_c_handles_assign.
 * @param hids_c See @ref bt_gatt_hids_c_handles_assign.
 *
 * @return See @ref bt_gatt_hids_c_handles_assign.
 *
 * @sa bt_gatt_hids_c_handles_assign
 */
static int handles_assign_internal(struct bt_gatt_dm *dm,
				   struct bt_gatt_hids_c *hids_c)
{
	const struct bt_gatt_dm_attr *gatt_service_attr =
			bt_gatt_dm_service_get(dm);
	const struct bt_gatt_service_val *gatt_service =
			bt_gatt_dm_attr_service_val(gatt_service_attr);
	const struct bt_gatt_dm_attr *gatt_chrc;
	u16_t handle;
	bool boot_protocol_required;
	int ret;

	bt_gatt_hids_c_release(hids_c);

	if (bt_uuid_cmp(gatt_service->uuid, BT_UUID_HIDS)) {
		return -ENOTSUP;
	}
	LOG_DBG("Getting handles from HID service.");

	/* Control Point Characteristic (Mandatory) */
	handle = chrc_value_handle_by_uuid(dm, hids_c,
					   BT_UUID_HIDS_CTRL_POINT);
	if (handle == 0) {
		LOG_ERR("Missing HIDS CP characteristic.");
		return -EINVAL;
	}
	LOG_DBG("Found handle for HIDS CP characteristic: 0x%x.", handle);
	hids_c->handlers.cp = handle;

	/* HID Information Characteristic (Mandatory) */
	handle = chrc_value_handle_by_uuid(dm, hids_c,
					   BT_UUID_HIDS_INFO);
	if (handle == 0) {
		LOG_ERR("Missing HIDS Info characteristic.");
		return -EINVAL;
	}
	LOG_DBG("Found handle for HIDS Info characteristic: 0x%x.", handle);
	hids_c->handlers.info = handle;

	/* HID Report Map Characteristic (Mandatory)  */
	handle = chrc_value_handle_by_uuid(dm, hids_c,
					   BT_UUID_HIDS_REPORT_MAP);
	if (handle == 0) {
		LOG_ERR("Missing Report Map characteristic.");
		return -EINVAL;
	}
	LOG_DBG("Found handle for Report Map characteristic: 0x%x.", handle);
	hids_c->handlers.rep_map = handle;

	/* If we have any of the boot report - protocol mode is mandatory,
	 * otherwise optional, but it does not make any sense to keep it
	 */
	boot_protocol_required = false;
	/* Mouse boot record */
	gatt_chrc = bt_gatt_dm_char_by_uuid(dm,
					    BT_UUID_HIDS_BOOT_MOUSE_IN_REPORT);
	if (gatt_chrc) {
		LOG_DBG("Mouse boot IN characteristic found.");
		ret = rep_new(hids_c, &(hids_c->rep_boot.mouse_inp),
			      dm, gatt_chrc);
		if (ret) {
			LOG_ERR("Report cannot be created.");
			return ret;
		}
		boot_protocol_required = true;
	} else {
		LOG_DBG("No boot mouse report detected.");
	}

	/* Keyboard boot records */
	const struct bt_gatt_dm_attr *gatt_chrc_kbd; /* Used temporary */

	gatt_chrc = bt_gatt_dm_char_by_uuid(dm,
			BT_UUID_HIDS_BOOT_KB_IN_REPORT);
	gatt_chrc_kbd =  bt_gatt_dm_char_by_uuid(dm,
			BT_UUID_HIDS_BOOT_KB_OUT_REPORT);
	if (gatt_chrc && gatt_chrc_kbd) {
		/* Boot keyboard present */
		LOG_DBG("KBD boot characteristics found.");
		ret = rep_new(hids_c, &(hids_c->rep_boot.kbd_inp),
			      dm, gatt_chrc);
		if (ret) {
			LOG_ERR("KBD IN Report cannot be created.");
			return ret;
		}
		ret = rep_new(hids_c, &(hids_c->rep_boot.kbd_out),
			      dm, gatt_chrc_kbd);
		if (ret) {
			LOG_ERR("KBD OUT Report cannot be created.");
			return ret;
		}
		boot_protocol_required = true;
	} else if (gatt_chrc || gatt_chrc_kbd) {
		/* Inconsistent configuration */
		/* Ignore this error but not boot keyboard would be supported */
		LOG_WRN("Inconsistent boot keyboard configuration. Ignoring.");
	} else {
		/* No keyboard present */
		LOG_DBG("No boot keyboard reports detected.");
	}

	/* HID Protocol Mode (Optional) */
	if (boot_protocol_required) {
		LOG_DBG("HIDS Protocol Mode characteristic required.");
		handle = chrc_value_handle_by_uuid(dm, hids_c,
						   BT_UUID_HIDS_PROTOCOL_MODE);
		if (handle == 0) {
			LOG_ERR("Missing Protocol Mode characteristic.");
			return -EINVAL;
		}
		LOG_DBG("Found handle for Protocol Mode characteristic.");
		hids_c->handlers.pm = handle;
	} else {
		LOG_DBG("HIDS Protocol Mode characteristic ignored.");
		hids_c->handlers.pm = 0;
	}

	/* Count number of records */
	size_t rep_cnt = 0;

	gatt_chrc = NULL;
	while (NULL != (gatt_chrc = bt_gatt_dm_char_next(dm, gatt_chrc))) {
		struct bt_gatt_chrc *chrc_val =
				bt_gatt_dm_attr_chrc_val(gatt_chrc);
		if (!bt_uuid_cmp(chrc_val->uuid, BT_UUID_HIDS_REPORT)) {
			++rep_cnt;
		}
	}
	LOG_DBG("%u report(s) found", rep_cnt);
	if (rep_cnt > UINT8_MAX) {
		LOG_ERR("Number of records cannot fit into 8 bit identifier.");
		return -EINVAL;
	}

	hids_c->rep_info = repptr_array_new(rep_cnt);
	if (!(hids_c->rep_info)) {
		LOG_ERR("Cannot create report pointers array.");
		return -ENOMEM;
	}
	hids_c->rep_cnt = (u8_t)rep_cnt;

	/* Process all the records */
	rep_cnt = 0;
	gatt_chrc = NULL;
	while (NULL != (gatt_chrc = bt_gatt_dm_char_next(dm, gatt_chrc))) {
		struct bt_gatt_chrc *chrc_val =
				bt_gatt_dm_attr_chrc_val(gatt_chrc);
		if (!bt_uuid_cmp(chrc_val->uuid, BT_UUID_HIDS_REPORT)) {
			LOG_DBG("Creating report at index %u, handle: 0x%.4x",
				rep_cnt, gatt_chrc->handle);
			ret = rep_new(hids_c,
				      &(hids_c->rep_info[rep_cnt]),
				      dm,
				      gatt_chrc);
			if (ret) {
				LOG_ERR("Cannot create report, error: %d", ret);
				return ret;
			}
			++rep_cnt;
		}
	}

	LOG_DBG("Report memory allocated, entities used: %u",
		k_mem_slab_num_used_get(&bt_gatt_hids_c_reports_mem));

	/* Finally - save connection object */
	hids_c->conn = bt_gatt_dm_conn_get(dm);

	return post_discovery_start(hids_c);
}

void bt_gatt_hids_c_init(struct bt_gatt_hids_c *hids_c,
			const struct bt_gatt_hids_c_init_params *params)
{
	memset(hids_c, 0, sizeof(*hids_c));
	hids_c->ready_cb      = params->ready_cb;
	hids_c->prep_error_cb = params->prep_error_cb;
	hids_c->pm_update_cb  = params->pm_update_cb;
	k_sem_init(&hids_c->read_params_sem, 1, 1);
}

int bt_gatt_hids_c_handles_assign(struct bt_gatt_dm *dm,
				  struct bt_gatt_hids_c *hids_c)
{
	int ret = handles_assign_internal(dm, hids_c);

	if (ret) {
		bt_gatt_hids_c_release(hids_c);
	}
	return ret;
}

void bt_gatt_hids_c_release(struct bt_gatt_hids_c *hids_c)
{
	if (hids_c->rep_info) {
		u8_t rep_cnt = hids_c->rep_cnt;

		while (rep_cnt) {
			rep_free(&(hids_c->rep_info[--rep_cnt]));
		}

		hids_c->rep_cnt = 0;
		repptr_array_free(hids_c->rep_info);
		hids_c->rep_info = NULL;
	}
	if (hids_c->rep_boot.kbd_inp) {
		rep_free(&(hids_c->rep_boot.kbd_inp));
	}
	if (hids_c->rep_boot.kbd_out) {
		rep_free(&(hids_c->rep_boot.kbd_out));
	}
	if (hids_c->rep_boot.mouse_inp) {
		rep_free(&(hids_c->rep_boot.mouse_inp));
	}
	LOG_DBG("Report memory released, entities used: %u",
		k_mem_slab_num_used_get(&bt_gatt_hids_c_reports_mem));

	memset(&hids_c->info_val, 0, sizeof(hids_c->info_val));
	memset(&hids_c->handlers, 0, sizeof(hids_c->handlers));
	hids_c->map_cb  = NULL;
	hids_c->ready   = false;
	hids_c->conn    = NULL;
	k_sem_give(&hids_c->read_params_sem);
}

void bt_gatt_hids_c_abort_all(struct bt_gatt_hids_c *hids_c)
{
	k_sem_give(&hids_c->read_params_sem);
}

bool bt_gatt_hids_c_assign_check(const struct bt_gatt_hids_c *hids_c)
{
	return hids_c->conn != NULL;
}

bool bt_gatt_hids_c_ready_check(const struct bt_gatt_hids_c *hids_c)
{
	return hids_c->ready;
}

/**
 * @brief Process report read
 *
 * Internal function to process report read and pass it further.
 *
 * @param conn   Connection handler.
 * @param err    Read ATT error code.
 * @param params Notification parameters structure - the pointer
 *               to the structure provided to read function.
 * @param data   Pointer to the data buffer.
 * @param length The size of the received data.
 *
 * @retval BT_GATT_ITER_STOP     Stop notification
 * @retval BT_GATT_ITER_CONTINUE Continue notification
 */
static u8_t rep_read_process(struct bt_conn *conn, u8_t err,
			     struct bt_gatt_read_params *params,
			     const void *data, u16_t length)
{
	struct bt_gatt_hids_c_rep_info *rep;

	rep = CONTAINER_OF(params,
			   struct bt_gatt_hids_c_rep_info,
			   read_params);
	if (!rep->read_cb) {
		LOG_ERR("No read callback present");
		return BT_GATT_ITER_STOP;
	}
	if (length > UINT8_MAX) {
		LOG_WRN("Data size too big, truncating");
		length = UINT8_MAX;
	}
	rep->size = (u8_t)length;
	(void)rep->read_cb(rep->hids_c, rep, 0, data);
	rep->read_cb = NULL;
	return BT_GATT_ITER_STOP;
}

int bt_gatt_hids_c_rep_read(struct bt_gatt_hids_c *hids_c,
			    struct bt_gatt_hids_c_rep_info *rep,
			    bt_gatt_hids_c_read_cb func)
{
	int err;

	if (!hids_c || !rep || !func) {
		return -EINVAL;
	}
	if (rep->read_cb) {
		return -EBUSY;
	}
	rep->read_cb = func;
	rep->read_params.func = rep_read_process;
	rep->read_params.handle_count  = 1;
	rep->read_params.single.handle = rep->handlers.val;
	rep->read_params.single.offset = 0;

	err = bt_gatt_read(hids_c->conn, &(rep->read_params));
	if (err) {
		rep->read_cb = NULL;
		return err;
	}
	return 0;
}

/**
 *  @brief Process report write
 *
 *  @param conn Connection object.
 *  @param err ATT error code.
 *  @param params Write parameters used.
 */
static void rep_write_process(struct bt_conn *conn, u8_t err,
			      struct bt_gatt_write_params *params)
{
	struct bt_gatt_hids_c_rep_info *rep;

	rep = CONTAINER_OF(params,
			   struct bt_gatt_hids_c_rep_info,
			   write_params);
	if (!rep->write_cb) {
		LOG_ERR("No write callback present");
		return;
	}
	rep->write_cb(rep->hids_c, rep, err);
	rep->write_cb = NULL;
}

int bt_gatt_hids_c_rep_write(struct bt_gatt_hids_c *hids_c,
			     struct bt_gatt_hids_c_rep_info *rep,
			     bt_gatt_hids_c_write_cb func,
			     const void *data, u8_t length)
{
	int err;

	if (!hids_c || !rep || !func) {
		return -EINVAL;
	}
	if (rep->write_cb) {
		return -EBUSY;
	}
	rep->write_cb = func;
	rep->write_params.func   = rep_write_process;
	rep->write_params.data   = data;
	rep->write_params.handle = rep->handlers.val;
	rep->write_params.length = length;

	err = bt_gatt_write(hids_c->conn, &(rep->write_params));
	if (err) {
		rep->write_cb = NULL;
		return err;
	}
	return 0;
}

int bt_gatt_hids_c_rep_write_wo_rsp(struct bt_gatt_hids_c *hids_c,
				   struct bt_gatt_hids_c_rep_info *rep,
				   const void *data, u8_t length)
{
	int err;

	if (!hids_c || !rep) {
		return -EINVAL;
	}
	if (rep->ref.type != BT_GATT_HIDS_REPORT_TYPE_OUTPUT) {
		return -ENOTSUP;
	}

	err = bt_gatt_write_without_response(hids_c->conn,
					     rep->handlers.val,
					     data,
					     length,
					     false);
	return err;
}

/**
 * @brief Process report notification
 *
 * Internal function to process report notification and pass it further.
 *
 * @param conn   Connection handler.
 * @param params Notification parameters structure - the pointer
 *               to the structure provided to subscribe function.
 * @param data   Pointer to the data buffer.
 * @param length The size of the received data.
 *
 * @retval BT_GATT_ITER_STOP     Stop notification
 * @retval BT_GATT_ITER_CONTINUE Continue notification
 */
static u8_t rep_notify_process(struct bt_conn *conn,
			       struct bt_gatt_subscribe_params *params,
			       const void *data, u16_t length)
{
	struct bt_gatt_hids_c_rep_info *rep;

	rep = CONTAINER_OF(params,
			   struct bt_gatt_hids_c_rep_info,
			   notify_params);
	if (!rep->notify_cb) {
		LOG_ERR("No notification callback present");
		return BT_GATT_ITER_STOP;
	}
	if (length > UINT8_MAX) {
		LOG_WRN("Data size too big, truncating");
		length = UINT8_MAX;
	}
	/* Zephyr uses the callback with data set to NULL to inform about the
	 * subscription removal. Do not update the report size in that case.
	 */
	if (data != NULL) {
		rep->size = (u8_t)length;
	}

	return rep->notify_cb(rep->hids_c, rep, 0, data);
}

int bt_gatt_hids_c_rep_subscribe(struct bt_gatt_hids_c *hids_c,
				 struct bt_gatt_hids_c_rep_info *rep,
				 bt_gatt_hids_c_read_cb func)
{
	int err;

	if (!hids_c || !rep || !func) {
		return -EINVAL;
	}
	if (rep->ref.type != BT_GATT_HIDS_REPORT_TYPE_INPUT) {
		return -ENOTSUP;
	}
	if (rep->notify_cb) {
		return -EALREADY;
	}

	rep->notify_cb = func;

	rep->notify_params.notify = rep_notify_process;
	rep->notify_params.value = BT_GATT_CCC_NOTIFY;
	rep->notify_params.value_handle = rep->handlers.val;
	rep->notify_params.ccc_handle = rep->handlers.ccc;
	atomic_set_bit(rep->notify_params.flags,
		       BT_GATT_SUBSCRIBE_FLAG_VOLATILE);

	LOG_DBG("Subscribe: val: %u, ccc: %u",
		rep->notify_params.value_handle,
		rep->notify_params.ccc_handle);
	err = bt_gatt_subscribe(hids_c->conn, &rep->notify_params);
	if (err) {
		LOG_ERR("Report notification subscribe error: %d.", err);
		rep->notify_cb = NULL;
		return err;
	}
	LOG_DBG("Report subscribed.");
	return err;
}

int bt_gatt_hids_c_rep_unsubscribe(struct bt_gatt_hids_c *hids_c,
				   struct bt_gatt_hids_c_rep_info *rep)
{
	if (!hids_c || !rep) {
		return -EINVAL;
	}
	return bt_gatt_unsubscribe(hids_c->conn, &rep->notify_params);
}

/**
 * @brief Process map read
 *
 * Internal function to process report map read and pass it further.
 *
 * @param conn   Connection handler.
 * @param err    Read ATT error code.
 * @param params Notification parameters structure - the pointer
 *               to the structure provided to read function.
 * @param data   Pointer to the data buffer.
 * @param length The size of the received data.
 *
 * @retval BT_GATT_ITER_STOP     Stop notification
 * @retval BT_GATT_ITER_CONTINUE Continue notification
 */
static u8_t map_read_process(struct bt_conn *conn, u8_t err,
			     struct bt_gatt_read_params *params,
			     const void *data, u16_t length)
{
	struct bt_gatt_hids_c *hids_c;
	size_t offset;

	hids_c = CONTAINER_OF(params,
			      struct bt_gatt_hids_c,
			      read_params);
	if (!hids_c->map_cb) {
		LOG_ERR("No map read callback present");
		return BT_GATT_ITER_STOP;
	}

	offset = hids_c->read_params.single.offset;
	k_sem_give(&hids_c->read_params_sem);
	hids_c->map_cb(hids_c, err, data, length, offset);
	return BT_GATT_ITER_STOP;
}

int bt_gatt_hids_c_map_read(struct bt_gatt_hids_c *hids_c,
			    bt_gatt_hids_c_map_cb func,
			    size_t offset,
			    k_timeout_t timeout)
{
	int err;

	if (!hids_c || !func) {

		return -EINVAL;
	}
	err = k_sem_take(&hids_c->read_params_sem, timeout);
	if (err) {
		return err;
	}
	hids_c->map_cb = func;
	hids_c->read_params.func = map_read_process;
	hids_c->read_params.handle_count  = 1;
	hids_c->read_params.single.handle = hids_c->handlers.rep_map;
	hids_c->read_params.single.offset = offset;
	err = bt_gatt_read(hids_c->conn, &(hids_c->read_params));
	if (err) {
		hids_c->map_cb = NULL;
		k_sem_give(&hids_c->read_params_sem);
		return err;
	}
	return 0;
}

/**
 * @brief Protocol mode update process
 *
 * Function that process protocol mode update and updates
 * the HIDS client object.
 *
 * @param conn   Connection handler.
 * @param err    Read ATT error code.
 * @param params Notification parameters structure - the pointer
 *               to the structure provided to read function.
 * @param data   Pointer to the data buffer.
 * @param length The size of the received data.
 *
 * @retval BT_GATT_ITER_STOP     Stop notification
 * @retval BT_GATT_ITER_CONTINUE Continue notification
 */
static u8_t pm_update_process(struct bt_conn *conn, u8_t err,
			     struct bt_gatt_read_params *params,
			     const void *data, u16_t length)
{
	struct bt_gatt_hids_c *hids_c;

	hids_c = CONTAINER_OF(params,
			      struct bt_gatt_hids_c,
			      read_params);
	k_sem_give(&hids_c->read_params_sem);
	if (err || !data) {
		LOG_ERR("PM read error");
	} else if (length != 1) {
		LOG_ERR("Unexpected PM size");
	} else {
		hids_c->pm = (enum bt_gatt_hids_pm)((u8_t *)data)[0];
	}

	if (hids_c->pm_update_cb) {
		hids_c->pm_update_cb(hids_c);
	}

	return BT_GATT_ITER_STOP;
}

int bt_gatt_hids_c_pm_update(struct bt_gatt_hids_c *hids_c, k_timeout_t timeout)
{
	int err;

	if (!hids_c) {
		return -EINVAL;
	}
	err = k_sem_take(&hids_c->read_params_sem, timeout);
	if (err) {
		return err;
	}
	hids_c->read_params.func = pm_update_process;
	hids_c->read_params.handle_count  = 1;
	hids_c->read_params.single.handle = hids_c->handlers.pm;
	hids_c->read_params.single.offset = 0;
	err = bt_gatt_read(hids_c->conn, &(hids_c->read_params));
	if (err) {
		k_sem_give(&hids_c->read_params_sem);
		return err;
	}
	return 0;
}

enum bt_gatt_hids_pm bt_gatt_hids_c_pm_get(
	const struct bt_gatt_hids_c *hids_c)
{
	return hids_c->pm;
}

int bt_gatt_hids_c_pm_write(struct bt_gatt_hids_c *hids_c,
			    enum bt_gatt_hids_pm pm)
{
	int err;
	u8_t data[] = {(u8_t)pm};

	if (hids_c->handlers.pm == 0) {
		return -EOPNOTSUPP;
	}
	err = bt_gatt_write_without_response(hids_c->conn,
					     hids_c->handlers.pm,
					     data,
					     sizeof(data),
					     false);
	if (!err) {
		hids_c->pm = pm;
		if (hids_c->pm_update_cb) {
			hids_c->pm_update_cb(hids_c);
		}
	}

	return err;
}

int bt_gatt_hids_c_suspend(struct bt_gatt_hids_c *hids_c)
{
	int err;
	u8_t data[] = {BT_GATT_HIDS_CONTROL_POINT_SUSPEND};

	err = bt_gatt_write_without_response(hids_c->conn,
					     hids_c->handlers.cp,
					     data,
					     sizeof(data),
					     false);
	return err;
}

int bt_gatt_hids_c_exit_suspend(struct bt_gatt_hids_c *hids_c)
{
	int err;
	u8_t data[] = {BT_GATT_HIDS_CONTROL_POINT_EXIT_SUSPEND};

	err = bt_gatt_write_without_response(hids_c->conn,
					     hids_c->handlers.cp,
					     data,
					     sizeof(data),
					     false);
	return err;
}

struct bt_conn *bt_gatt_hids_c_conn(const struct bt_gatt_hids_c *hids_c)
{
	return hids_c->conn;
}

const struct bt_gatt_hids_info *bt_gatt_hids_c_conn_info_val(
	const struct bt_gatt_hids_c *hids_c)
{
	return &hids_c->info_val;
}

struct bt_gatt_hids_c_rep_info *bt_gatt_hids_c_rep_boot_kbd_in(
	struct bt_gatt_hids_c *hids_c)
{
	return hids_c->rep_boot.kbd_inp;
}

struct bt_gatt_hids_c_rep_info *bt_gatt_hids_c_rep_boot_kbd_out(
	struct bt_gatt_hids_c *hids_c)
{
	return hids_c->rep_boot.kbd_out;
}

struct bt_gatt_hids_c_rep_info *bt_gatt_hids_c_rep_boot_mouse_in(
	struct bt_gatt_hids_c *hids_c)
{
	return hids_c->rep_boot.mouse_inp;
}

size_t bt_gatt_hids_c_rep_cnt(const struct bt_gatt_hids_c *hids_c)
{
	return hids_c->rep_cnt;
}

struct bt_gatt_hids_c_rep_info *bt_gatt_hids_c_rep_next(
	struct bt_gatt_hids_c *hids_c,
	const struct bt_gatt_hids_c_rep_info *rep)
{
	struct bt_gatt_hids_c_rep_info **rep_arr;
	size_t n;

	/* No reports in HIDS */
	if (hids_c->rep_cnt == 0) {
		return NULL;
	}
	/* First element */
	if (rep == NULL) {
		return hids_c->rep_info[0];
	}
	/* Searching current report in array */
	rep_arr = hids_c->rep_info;

	for (n = 0; n + 1 < hids_c->rep_cnt; ++n) {
		if (*rep_arr++ == rep) {
			return *rep_arr;
		}
	}
	return NULL;
}

struct bt_gatt_hids_c_rep_info *bt_gatt_hids_c_rep_find(
	struct bt_gatt_hids_c *hids_c,
	enum bt_gatt_hids_report_type type,
	u8_t id)
{
	struct bt_gatt_hids_c_rep_info **rep_arr;
	size_t n;

	/* Searching the report */
	rep_arr = hids_c->rep_info;
	for (n = 0; n < hids_c->rep_cnt; ++n, ++rep_arr) {
		struct bt_gatt_hids_c_rep_info *rep = (*rep_arr);

		if ((rep->ref.id == id) && (rep->ref.type == type)) {
			return rep;
		}
	}
	return NULL;
}

void bt_gatt_hids_c_rep_user_data_set(struct bt_gatt_hids_c_rep_info *rep,
				      void *data)
{
	rep->user_data = data;
}

void *bt_gatt_hids_c_rep_user_data(const struct bt_gatt_hids_c_rep_info *rep)
{
	return rep->user_data;
}

u8_t bt_gatt_hids_c_rep_id(const struct bt_gatt_hids_c_rep_info *rep)
{
	return rep->ref.id;
}

enum bt_gatt_hids_report_type bt_gatt_hids_c_rep_type(
	const struct bt_gatt_hids_c_rep_info *rep)
{
	return rep->ref.type;
}

size_t bt_gatt_hids_c_rep_size(const struct bt_gatt_hids_c_rep_info *rep)
{
	return rep->size;
}
