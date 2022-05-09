/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>
#include <zb_nrf_platform.h>
#include <zigbee/zigbee_zcl_scenes.h>

LOG_MODULE_REGISTER(zigbee_zcl_scenes, CONFIG_ZIGBEE_SCENES_LOG_LEVEL);

static zb_uint8_t scene_table_get_entry(zb_uint16_t group_id, zb_uint8_t scene_id);
static void scene_table_remove_entries_by_group(zb_uint16_t group_id);
static void scene_table_init(void);

struct zb_zcl_scenes_fieldset_data_on_off {
	zb_bool_t  has_on_off;
	zb_uint8_t on_off;
};

struct zb_zcl_scenes_fieldset_data_level_control {
	zb_bool_t  has_current_level;
	zb_uint8_t current_level;
};

struct zb_zcl_scenes_fieldset_data_window_covering {
	zb_bool_t  has_current_position_lift_percentage;
	zb_uint8_t current_position_lift_percentage;
	zb_bool_t  has_current_position_tilt_percentage;
	zb_uint8_t current_position_tilt_percentage;
};

struct scene_table_on_off_entry {
	zb_zcl_scene_table_record_fixed_t                  common;
	struct zb_zcl_scenes_fieldset_data_on_off          on_off;
	struct zb_zcl_scenes_fieldset_data_level_control   level_control;
	struct zb_zcl_scenes_fieldset_data_window_covering window_covering;
};

static struct scene_table_on_off_entry scenes_table[CONFIG_ZIGBEE_SCENE_TABLE_SIZE];

struct response_info {
	zb_zcl_parsed_hdr_t cmd_info;
	zb_zcl_scenes_view_scene_req_t view_scene_req;
	zb_zcl_scenes_get_scene_membership_req_t get_scene_membership_req;
};

static struct response_info resp_info;

static int scenes_table_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg)
{
	const char *next;
	int rc;

	if (settings_name_steq(name, "scenes_table", &next) && !next) {
		if (len != sizeof(scenes_table)) {
			return -EINVAL;
		}

		rc = read_cb(cb_arg, scenes_table, sizeof(scenes_table));
		if (rc >= 0) {
			return 0;
		}

		return rc;
	}

	return -ENOENT;
}

static void scenes_table_save(void)
{
	settings_save_one("scenes/scenes_table", scenes_table, sizeof(scenes_table));
}

struct settings_handler scenes_conf = {
	.name = "scenes",
	.h_set = scenes_table_set
};

static zb_bool_t has_cluster(zb_uint16_t cluster_id)
{
	return (get_endpoint_by_cluster(cluster_id, ZB_ZCL_CLUSTER_SERVER_ROLE)
		== CONFIG_ZIGBEE_SCENES_ENDPOINT)
		? ZB_TRUE : ZB_FALSE;
}

static zb_bool_t add_fieldset(zb_zcl_scenes_fieldset_common_t *fieldset,
			      struct scene_table_on_off_entry *entry)
{
	zb_uint8_t *fs_data_ptr;

	if (fieldset->cluster_id == ZB_ZCL_CLUSTER_ID_ON_OFF &&
	    fieldset->fieldset_length >= 1 &&
	    has_cluster(ZB_ZCL_CLUSTER_ID_ON_OFF)) {
		fs_data_ptr = (zb_uint8_t *)fieldset + sizeof(zb_zcl_scenes_fieldset_common_t);

		entry->on_off.has_on_off = ZB_TRUE;
		entry->on_off.on_off = *fs_data_ptr;

		LOG_INF("Add fieldset: cluster_id=0x%x, length=%d, on/off=%d",
			fieldset->cluster_id, fieldset->fieldset_length, entry->on_off.on_off);

		return ZB_TRUE;
	}
	if (fieldset->cluster_id == ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL &&
	    fieldset->fieldset_length >= 1 &&
	    has_cluster(ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL)) {
		fs_data_ptr = (zb_uint8_t *)fieldset + sizeof(zb_zcl_scenes_fieldset_common_t);

		entry->level_control.has_current_level = ZB_TRUE;
		entry->level_control.current_level = *fs_data_ptr;

		LOG_INF("Add fieldset: cluster_id=0x%x, length=%d, level=%d",
			fieldset->cluster_id,
			fieldset->fieldset_length,
			entry->level_control.current_level);

		return ZB_TRUE;
	}
	if (fieldset->cluster_id == ZB_ZCL_CLUSTER_ID_WINDOW_COVERING &&
	    fieldset->fieldset_length >= 1 &&
	    has_cluster(ZB_ZCL_CLUSTER_ID_WINDOW_COVERING)) {
		fs_data_ptr = (zb_uint8_t *)fieldset + sizeof(zb_zcl_scenes_fieldset_common_t);

		entry->window_covering.has_current_position_lift_percentage = ZB_TRUE;
		entry->window_covering.current_position_lift_percentage = *fs_data_ptr;

		if (fieldset->fieldset_length >= 2) {
			entry->window_covering.has_current_position_tilt_percentage = ZB_TRUE;
			entry->window_covering.current_position_tilt_percentage =
				*(fs_data_ptr + 1);
		}

		LOG_INF("Add fieldset: cluster_id=0x%x, length=%d, lift=%d",
			fieldset->cluster_id, fieldset->fieldset_length,
			entry->window_covering.current_position_lift_percentage);

		return ZB_TRUE;
	}

	LOG_INF("Ignore fieldset: cluster_id = 0x%x, length = %d",
		fieldset->cluster_id, fieldset->fieldset_length);

	return ZB_FALSE;
}

static zb_uint8_t *dump_fieldsets(struct scene_table_on_off_entry *entry,
				  zb_uint8_t *payload_ptr)
{
	if (entry->on_off.has_on_off == ZB_TRUE) {
		LOG_INF("Append On/Off fieldset");

		/* Extension set: Cluster ID = On/Off */
		ZB_ZCL_PACKET_PUT_DATA16_VAL(payload_ptr, ZB_ZCL_CLUSTER_ID_ON_OFF);

		/* Extension set: Fieldset length = 1 */
		ZB_ZCL_PACKET_PUT_DATA8(payload_ptr, 1);

		/* Extension set: On/Off state */
		ZB_ZCL_PACKET_PUT_DATA8(payload_ptr, entry->on_off.on_off);
	}

	if (entry->level_control.has_current_level == ZB_TRUE) {
		LOG_INF("Append level control fieldset");

		/* Extension set: Cluster ID = Level Control */
		ZB_ZCL_PACKET_PUT_DATA16_VAL(payload_ptr, ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL);

		/* Extension set: Fieldset length = 1 */
		ZB_ZCL_PACKET_PUT_DATA8(payload_ptr, 1);

		/* Extension set: current_level state */
		ZB_ZCL_PACKET_PUT_DATA8(payload_ptr, entry->level_control.current_level);
	}

	if (entry->window_covering.has_current_position_lift_percentage == ZB_TRUE) {
		LOG_INF("Append window covering fieldset");

		/* Extension set: Cluster ID = Window covering */
		ZB_ZCL_PACKET_PUT_DATA16_VAL(payload_ptr, ZB_ZCL_CLUSTER_ID_WINDOW_COVERING);

		/* Extension set: Fieldset length = 1 or 2*/
		if (entry->window_covering.has_current_position_tilt_percentage == ZB_TRUE) {
			ZB_ZCL_PACKET_PUT_DATA8(payload_ptr, 2);
		} else {
			ZB_ZCL_PACKET_PUT_DATA8(payload_ptr, 1);
		}
		/* Extension set: current_position_lift_percentage state */
		ZB_ZCL_PACKET_PUT_DATA8(payload_ptr,
			entry->window_covering.current_position_lift_percentage);

		if (entry->window_covering.has_current_position_tilt_percentage == ZB_TRUE) {
			/* Extension set: current_position_tilt_percentage state */
			ZB_ZCL_PACKET_PUT_DATA8(payload_ptr,
				entry->window_covering.current_position_tilt_percentage);
		}
	}

	/* Pass the updated data pointer. */
	return payload_ptr;
}

static zb_ret_t get_on_off_value(zb_uint8_t *on_off)
{
	zb_zcl_attr_t *attr_desc = zb_zcl_get_attr_desc_a(
		CONFIG_ZIGBEE_SCENES_ENDPOINT,
		ZB_ZCL_CLUSTER_ID_ON_OFF,
		ZB_ZCL_CLUSTER_SERVER_ROLE,
		ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID);

	if (attr_desc != NULL) {
		*on_off = (zb_bool_t)ZB_ZCL_GET_ATTRIBUTE_VAL_8(attr_desc);
		return RET_OK;
	}

	return RET_ERROR;
}

static zb_ret_t get_current_level_value(zb_uint8_t *current_level)
{
	zb_zcl_attr_t *attr_desc = zb_zcl_get_attr_desc_a(
		CONFIG_ZIGBEE_SCENES_ENDPOINT,
		ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL,
		ZB_ZCL_CLUSTER_SERVER_ROLE,
		ZB_ZCL_ATTR_LEVEL_CONTROL_CURRENT_LEVEL_ID);

	if (attr_desc != NULL) {
		*current_level = ZB_ZCL_GET_ATTRIBUTE_VAL_8(attr_desc);
		return RET_OK;
	}

	return RET_ERROR;
}

static zb_ret_t get_current_lift_value(zb_uint8_t *percentage)
{
	zb_zcl_attr_t *attr_desc = zb_zcl_get_attr_desc_a(
		CONFIG_ZIGBEE_SCENES_ENDPOINT,
		ZB_ZCL_CLUSTER_ID_WINDOW_COVERING,
		ZB_ZCL_CLUSTER_SERVER_ROLE,
		ZB_ZCL_ATTR_WINDOW_COVERING_CURRENT_POSITION_LIFT_PERCENTAGE_ID);

	if (attr_desc != NULL) {
		*percentage = ZB_ZCL_GET_ATTRIBUTE_VAL_8(attr_desc);
		return RET_OK;
	}

	return RET_ERROR;
}

static zb_ret_t get_current_tilt_value(zb_uint8_t *percentage)
{
	zb_zcl_attr_t *attr_desc = zb_zcl_get_attr_desc_a(
		CONFIG_ZIGBEE_SCENES_ENDPOINT,
		ZB_ZCL_CLUSTER_ID_WINDOW_COVERING,
		ZB_ZCL_CLUSTER_SERVER_ROLE,
		ZB_ZCL_ATTR_WINDOW_COVERING_CURRENT_POSITION_TILT_PERCENTAGE_ID);

	if (attr_desc != NULL) {
		*percentage = ZB_ZCL_GET_ATTRIBUTE_VAL_8(attr_desc);
		return RET_OK;
	}

	return RET_ERROR;
}

static void save_state_as_scene(struct scene_table_on_off_entry *entry)
{
	if (has_cluster(ZB_ZCL_CLUSTER_ID_ON_OFF) &&
	    get_on_off_value(&entry->on_off.on_off) == RET_OK) {
		LOG_INF("Save On/Off state inside scene table");
		entry->on_off.has_on_off = ZB_TRUE;
	}
	if (has_cluster(ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL) &&
	    get_current_level_value(&entry->level_control.current_level) == RET_OK) {
		LOG_INF("Save level control state inside scene table");
		entry->level_control.has_current_level = ZB_TRUE;
	}
	if (has_cluster(ZB_ZCL_CLUSTER_ID_WINDOW_COVERING)) {
		LOG_INF("Save window covering state inside scene table");
		if (get_current_lift_value(
		    &entry->window_covering.current_position_lift_percentage) == RET_OK) {
			entry->window_covering.has_current_position_lift_percentage = ZB_TRUE;
		}
		if (get_current_tilt_value(
		    &entry->window_covering.current_position_tilt_percentage) == RET_OK) {
			entry->window_covering.has_current_position_tilt_percentage = ZB_TRUE;
		}
	}
}

static void recall_scene(struct scene_table_on_off_entry *entry)
{
	zb_bufid_t buf = zb_buf_get_any();
	zb_zcl_attr_t *attr_desc;
	zb_ret_t result;

	if (entry->on_off.has_on_off) {
		LOG_INF("Recall On/Off state");

		attr_desc = zb_zcl_get_attr_desc_a(
			CONFIG_ZIGBEE_SCENES_ENDPOINT,
			ZB_ZCL_CLUSTER_ID_ON_OFF,
			ZB_ZCL_CLUSTER_SERVER_ROLE,
			ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID);

		ZB_ZCL_INVOKE_USER_APP_SET_ATTR_WITH_RESULT(
			buf,
			CONFIG_ZIGBEE_SCENES_ENDPOINT,
			ZB_ZCL_CLUSTER_ID_ON_OFF,
			attr_desc,
			&entry->on_off.on_off,
			result
		);
	}
	if (entry->level_control.has_current_level) {
		LOG_INF("Recall level control state");

		attr_desc = zb_zcl_get_attr_desc_a(
			CONFIG_ZIGBEE_SCENES_ENDPOINT,
			ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL,
			ZB_ZCL_CLUSTER_SERVER_ROLE,
			ZB_ZCL_ATTR_LEVEL_CONTROL_CURRENT_LEVEL_ID);

		ZB_ZCL_INVOKE_USER_APP_SET_ATTR_WITH_RESULT(
			buf,
			CONFIG_ZIGBEE_SCENES_ENDPOINT,
			ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL,
			attr_desc,
			&entry->level_control.current_level,
			result
		);
	}
	if (entry->window_covering.has_current_position_lift_percentage) {
		LOG_INF("Recall window covering lift state");

		attr_desc = zb_zcl_get_attr_desc_a(
			CONFIG_ZIGBEE_SCENES_ENDPOINT,
			ZB_ZCL_CLUSTER_ID_WINDOW_COVERING,
			ZB_ZCL_CLUSTER_SERVER_ROLE,
			ZB_ZCL_ATTR_WINDOW_COVERING_CURRENT_POSITION_LIFT_PERCENTAGE_ID);

		ZB_ZCL_INVOKE_USER_APP_SET_ATTR_WITH_RESULT(
				buf,
				CONFIG_ZIGBEE_SCENES_ENDPOINT,
				ZB_ZCL_CLUSTER_ID_WINDOW_COVERING,
				attr_desc,
				&entry->window_covering.current_position_lift_percentage,
				result
		);
	}
	if (entry->window_covering.has_current_position_tilt_percentage) {
		LOG_INF("Recall window covering tilt state");

		attr_desc = zb_zcl_get_attr_desc_a(
			CONFIG_ZIGBEE_SCENES_ENDPOINT,
			ZB_ZCL_CLUSTER_ID_WINDOW_COVERING,
			ZB_ZCL_CLUSTER_SERVER_ROLE,
			ZB_ZCL_ATTR_WINDOW_COVERING_CURRENT_POSITION_TILT_PERCENTAGE_ID);

		ZB_ZCL_INVOKE_USER_APP_SET_ATTR_WITH_RESULT(
			buf,
			CONFIG_ZIGBEE_SCENES_ENDPOINT,
			ZB_ZCL_CLUSTER_ID_WINDOW_COVERING,
			attr_desc,
			&entry->window_covering.current_position_tilt_percentage,
			result
		);
	}

	zb_buf_free(buf);
}

static void send_view_scene_resp(zb_bufid_t bufid, zb_uint16_t idx)
{
	zb_uint8_t *payload_ptr;
	zb_uint8_t view_scene_status = ZB_ZCL_STATUS_NOT_FOUND;

	LOG_DBG(">> %s bufid %hd idx %d", __func__, bufid, idx);

	if (idx != 0xFF &&
	    scenes_table[idx].common.group_id != ZB_ZCL_SCENES_FREE_SCENE_TABLE_RECORD) {
		/* Scene found */
		view_scene_status = ZB_ZCL_STATUS_SUCCESS;
	} else if (!zb_aps_is_endpoint_in_group(resp_info.view_scene_req.group_id,
			ZB_ZCL_PARSED_HDR_SHORT_DATA(&resp_info.cmd_info).dst_endpoint)) {
		/* Not in the group */
		view_scene_status = ZB_ZCL_STATUS_INVALID_FIELD;
	}

	ZB_ZCL_SCENES_INIT_VIEW_SCENE_RES(
		bufid,
		payload_ptr,
		resp_info.cmd_info.seq_number,
		view_scene_status,
		resp_info.view_scene_req.group_id,
		resp_info.view_scene_req.scene_id);

	if (view_scene_status == ZB_ZCL_STATUS_SUCCESS) {
		ZB_ZCL_SCENES_ADD_TRANSITION_TIME_VIEW_SCENE_RES(
			payload_ptr,
			scenes_table[idx].common.transition_time);

		ZB_ZCL_SCENES_ADD_SCENE_NAME_VIEW_SCENE_RES(
			payload_ptr,
			scenes_table[idx].common.scene_name);

		payload_ptr = dump_fieldsets(&scenes_table[idx], payload_ptr);
	}

	ZB_ZCL_SCENES_SEND_VIEW_SCENE_RES(
		bufid,
		payload_ptr,
		ZB_ZCL_PARSED_HDR_SHORT_DATA(&resp_info.cmd_info).source.u.short_addr,
		ZB_ZCL_PARSED_HDR_SHORT_DATA(&resp_info.cmd_info).src_endpoint,
		ZB_ZCL_PARSED_HDR_SHORT_DATA(&resp_info.cmd_info).dst_endpoint,
		resp_info.cmd_info.profile_id,
		NULL);

	LOG_DBG("<< %s", __func__);
}

static void send_get_scene_membership_resp(zb_bufid_t bufid)
{
	zb_uint8_t *payload_ptr;
	zb_uint8_t *capacity_ptr;
	zb_uint8_t *scene_count_ptr;

	LOG_DBG(">> %s bufid %hd", __func__, bufid);

	if (!zb_aps_is_endpoint_in_group(
			resp_info.get_scene_membership_req.group_id,
			ZB_ZCL_PARSED_HDR_SHORT_DATA(&resp_info.cmd_info).dst_endpoint)) {
		/* Not in the group */
		ZB_ZCL_SCENES_INIT_GET_SCENE_MEMBERSHIP_RES(
			bufid,
			payload_ptr,
			resp_info.cmd_info.seq_number,
			capacity_ptr,
			ZB_ZCL_STATUS_INVALID_FIELD,
			ZB_ZCL_SCENES_CAPACITY_UNKNOWN,
			resp_info.get_scene_membership_req.group_id);
	} else {
		zb_uint8_t i = 0;

		ZB_ZCL_SCENES_INIT_GET_SCENE_MEMBERSHIP_RES(
			bufid,
			payload_ptr,
			resp_info.cmd_info.seq_number,
			capacity_ptr,
			ZB_ZCL_STATUS_SUCCESS,
			0,
			resp_info.get_scene_membership_req.group_id);

		scene_count_ptr = payload_ptr;
		ZB_ZCL_SCENES_ADD_SCENE_COUNT_GET_SCENE_MEMBERSHIP_RES(payload_ptr, 0);

		while (i < CONFIG_ZIGBEE_SCENE_TABLE_SIZE) {
			if (scenes_table[i].common.group_id ==
			    resp_info.get_scene_membership_req.group_id) {
				/* Add to payload */
				LOG_INF("add scene_id %hd", scenes_table[i].common.scene_id);
				++(*scene_count_ptr);
				ZB_ZCL_SCENES_ADD_SCENE_ID_GET_SCENE_MEMBERSHIP_RES(
					payload_ptr,
					scenes_table[i].common.scene_id);
			} else if (scenes_table[i].common.group_id ==
				   ZB_ZCL_SCENES_FREE_SCENE_TABLE_RECORD) {
				LOG_INF("add capacity num");
				++(*capacity_ptr);
			}
			++i;
		}
	}

	ZB_ZCL_SCENES_SEND_GET_SCENE_MEMBERSHIP_RES(
		bufid,
		payload_ptr,
		ZB_ZCL_PARSED_HDR_SHORT_DATA(&resp_info.cmd_info).source.u.short_addr,
		ZB_ZCL_PARSED_HDR_SHORT_DATA(&resp_info.cmd_info).src_endpoint,
		ZB_ZCL_PARSED_HDR_SHORT_DATA(&resp_info.cmd_info).dst_endpoint,
		resp_info.cmd_info.profile_id,
		NULL);

	LOG_DBG("<< %s", __func__);
}

static void scene_table_init(void)
{
	zb_uint8_t i = 0;

	memset(scenes_table, 0, sizeof(scenes_table));
	while (i < CONFIG_ZIGBEE_SCENE_TABLE_SIZE) {
		scenes_table[i].common.group_id = ZB_ZCL_SCENES_FREE_SCENE_TABLE_RECORD;
		++i;
	}
}

static zb_uint8_t scene_table_get_entry(zb_uint16_t group_id, zb_uint8_t scene_id)
{
	zb_uint8_t i = 0;
	zb_uint8_t idx = 0xFF, free_idx = 0xFF;

	while (i < CONFIG_ZIGBEE_SCENE_TABLE_SIZE) {
		if (scenes_table[i].common.group_id == group_id &&
		    scenes_table[i].common.scene_id == scene_id) {
			idx = i;
			break;
		} else if ((free_idx == 0xFF) &&
			 (scenes_table[i].common.group_id ==
			  ZB_ZCL_SCENES_FREE_SCENE_TABLE_RECORD)) {
			/* Remember free index */
			free_idx = i;
		}
		++i;
	}

	return ((idx != 0xFF) ? idx : free_idx);
}

static void scene_table_remove_entries_by_group(zb_uint16_t group_id)
{
	zb_uint8_t i = 0;

	LOG_DBG(">> %s: group_id 0x%x", __func__, group_id);
	while (i < CONFIG_ZIGBEE_SCENE_TABLE_SIZE) {
		if (scenes_table[i].common.group_id == group_id) {
			LOG_INF("removing scene: entry idx %hd", i);
			memset(&scenes_table[i], 0, sizeof(scenes_table[i]));
			scenes_table[i].common.group_id = ZB_ZCL_SCENES_FREE_SCENE_TABLE_RECORD;
		}
		++i;
	}
	LOG_DBG("<< %s", __func__);
}

static zb_ret_t get_scene_valid_value(zb_bool_t *scene_valid)
{
	zb_zcl_attr_t *attr_desc = zb_zcl_get_attr_desc_a(
		CONFIG_ZIGBEE_SCENES_ENDPOINT,
		ZB_ZCL_CLUSTER_ID_SCENES,
		ZB_ZCL_CLUSTER_SERVER_ROLE,
		ZB_ZCL_ATTR_SCENES_SCENE_VALID_ID);

	if (attr_desc != NULL) {
		*scene_valid = (zb_bool_t)ZB_ZCL_GET_ATTRIBUTE_VAL_8(attr_desc);
		return RET_OK;
	}

	return RET_ERROR;
}

static zb_ret_t set_scene_valid_value(zb_bool_t scene_valid)
{
	zb_zcl_attr_t *attr_desc = zb_zcl_get_attr_desc_a(
		CONFIG_ZIGBEE_SCENES_ENDPOINT,
		ZB_ZCL_CLUSTER_ID_SCENES,
		ZB_ZCL_CLUSTER_SERVER_ROLE,
		ZB_ZCL_ATTR_SCENES_SCENE_VALID_ID);

	if (attr_desc != NULL) {
		ZB_ZCL_SET_DIRECTLY_ATTR_VAL8(attr_desc, scene_valid);
		return RET_OK;
	}

	return RET_ERROR;
}

static zb_ret_t get_current_scene_scene_id_value(zb_uint8_t *scene_id)
{
	zb_zcl_attr_t *attr_desc = zb_zcl_get_attr_desc_a(
		CONFIG_ZIGBEE_SCENES_ENDPOINT,
		ZB_ZCL_CLUSTER_ID_SCENES,
		ZB_ZCL_CLUSTER_SERVER_ROLE,
		ZB_ZCL_ATTR_SCENES_CURRENT_SCENE_ID);

	if (attr_desc != NULL) {
		*scene_id = (zb_bool_t)ZB_ZCL_GET_ATTRIBUTE_VAL_8(attr_desc);
		return RET_OK;
	}

	return RET_ERROR;
}

static zb_ret_t get_current_scene_group_id_value(zb_uint16_t *group_id)
{
	zb_zcl_attr_t *attr_desc = zb_zcl_get_attr_desc_a(
		CONFIG_ZIGBEE_SCENES_ENDPOINT,
		ZB_ZCL_CLUSTER_ID_SCENES,
		ZB_ZCL_CLUSTER_SERVER_ROLE,
		ZB_ZCL_ATTR_SCENES_CURRENT_GROUP_ID);

	if (attr_desc != NULL) {
		*group_id = (zb_bool_t)ZB_ZCL_GET_ATTRIBUTE_VAL_8(attr_desc);
		return RET_OK;
	}

	return RET_ERROR;
}

static void update_scene_valid_value(void)
{
	/* ZBOSS stack automatically sets the scene_valid attribute to true after
	 * scene recall, but is unable to set it back to false if the device state
	 * has changed.
	 */
	zb_bool_t scene_valid = ZB_FALSE;
	zb_uint8_t scene_id = 0xFF;
	zb_uint16_t group_id = ZB_ZCL_SCENES_FREE_SCENE_TABLE_RECORD;

	if (get_scene_valid_value(&scene_valid) == RET_OK &&
	    get_current_scene_scene_id_value(&scene_id) == RET_OK &&
	    get_current_scene_group_id_value(&group_id) == RET_OK &&
	    scene_valid == ZB_TRUE) {
		/* Verify if scene_valid should be reset. */
		zb_uint8_t idx = scene_table_get_entry(group_id, scene_id);

		if (group_id == ZB_ZCL_SCENES_FREE_SCENE_TABLE_RECORD ||
		    scene_id < CONFIG_ZIGBEE_SCENE_TABLE_SIZE ||
		    idx == ZB_ZCL_SCENES_FREE_SCENE_TABLE_RECORD) {
			(void)set_scene_valid_value(ZB_FALSE);
			return;
		}

		if (scenes_table[idx].on_off.has_on_off) {
			zb_uint8_t on_off;

			(void)get_on_off_value(&on_off);
			if (on_off != scenes_table[idx].on_off.on_off) {
				(void)set_scene_valid_value(ZB_FALSE);
				return;
			}
		}

		if (scenes_table[idx].level_control.has_current_level) {
			zb_uint8_t current_level;

			(void)get_current_level_value(&current_level);
			if (current_level != scenes_table[idx].level_control.current_level) {
				(void)set_scene_valid_value(ZB_FALSE);
				return;
			}
		}

		if (scenes_table[idx].window_covering.has_current_position_lift_percentage) {
			zb_uint8_t lift;

			(void)get_current_lift_value(&lift);
			if (lift !=
			    scenes_table[idx].window_covering.current_position_lift_percentage) {
				(void)set_scene_valid_value(ZB_FALSE);
				return;
			}
		}
		if (scenes_table[idx].window_covering.has_current_position_tilt_percentage) {
			zb_uint8_t tilt;

			(void)get_current_lift_value(&tilt);
			if (tilt !=
			    scenes_table[idx].window_covering.current_position_tilt_percentage) {
				(void)set_scene_valid_value(ZB_FALSE);
				return;
			}
		}
	}
}

void zcl_scenes_init(void)
{
	scene_table_init();
	settings_register(&scenes_conf);
}

zb_bool_t zcl_scenes_cb(zb_bufid_t bufid)
{
	zb_zcl_device_callback_param_t *device_cb_param =
			ZB_BUF_GET_PARAM(bufid, zb_zcl_device_callback_param_t);
	LOG_DBG(">> %s bufid %hd id %hd", __func__, bufid, device_cb_param->device_cb_id);

	update_scene_valid_value();

	switch (device_cb_param->device_cb_id) {
	case ZB_ZCL_SCENES_ADD_SCENE_CB_ID: {
		const zb_zcl_scenes_add_scene_req_t *add_scene_req =
			ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(
				bufid,
				zb_zcl_scenes_add_scene_req_t);
		zb_uint8_t idx = 0xFF;
		zb_uint8_t *add_scene_status =
			ZB_ZCL_DEVICE_CMD_PARAM_OUT_GET(bufid, zb_uint8_t);

		LOG_INF("ZB_ZCL_SCENES_ADD_SCENE_CB_ID: "
			"group_id 0x%x scene_id %hd transition_time %d",
			add_scene_req->group_id,
			add_scene_req->scene_id,
			add_scene_req->transition_time);

		*add_scene_status = ZB_ZCL_STATUS_INVALID_FIELD;
		idx = scene_table_get_entry(add_scene_req->group_id,
					    add_scene_req->scene_id);

		if (idx != 0xFF) {
			zb_zcl_scenes_fieldset_common_t *fieldset;
			zb_uint8_t fs_content_length;

			if (scenes_table[idx].common.group_id !=
			    ZB_ZCL_SCENES_FREE_SCENE_TABLE_RECORD) {
				/* Indicate that we overwriting existing record */
				device_cb_param->status = RET_ALREADY_EXISTS;
			}
			zb_bool_t empty_entry = ZB_TRUE;

			ZB_ZCL_SCENES_GET_ADD_SCENE_REQ_NEXT_FIELDSET_DESC(
				bufid,
				fieldset,
				fs_content_length);
			while (fieldset) {
				if (add_fieldset(fieldset, &scenes_table[idx]) == ZB_TRUE) {
					empty_entry = ZB_FALSE;
				}
				ZB_ZCL_SCENES_GET_ADD_SCENE_REQ_NEXT_FIELDSET_DESC(
					bufid,
					fieldset,
					fs_content_length);
			}

			if (empty_entry) {
				LOG_WRN("Saving empty scene.");
			}
			/* Store this scene */
			scenes_table[idx].common.group_id = add_scene_req->group_id;
			scenes_table[idx].common.scene_id = add_scene_req->scene_id;
			scenes_table[idx].common.transition_time = add_scene_req->transition_time;
			*add_scene_status = ZB_ZCL_STATUS_SUCCESS;
			scenes_table_save();
		} else {
			LOG_ERR("Unable to add scene: ZB_ZCL_STATUS_INSUFF_SPACE");
			*add_scene_status = ZB_ZCL_STATUS_INSUFF_SPACE;
		}
	}
	break;

	case ZB_ZCL_SCENES_VIEW_SCENE_CB_ID: {
		const zb_zcl_scenes_view_scene_req_t *view_scene_req =
			ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(bufid, zb_zcl_scenes_view_scene_req_t);
		const zb_zcl_parsed_hdr_t *in_cmd_info = ZB_ZCL_DEVICE_CMD_PARAM_CMD_INFO(bufid);
		zb_uint8_t idx = 0xFF;

		LOG_INF("ZB_ZCL_SCENES_VIEW_SCENE_CB_ID: group_id 0x%x scene_id %hd",
			view_scene_req->group_id,
			view_scene_req->scene_id);

		idx = scene_table_get_entry(view_scene_req->group_id, view_scene_req->scene_id);
		LOG_INF("Scene table entry index: %d", idx);

		/* Send View Scene Response */
		ZB_MEMCPY(&resp_info.cmd_info, in_cmd_info, sizeof(zb_zcl_parsed_hdr_t));
		ZB_MEMCPY(&resp_info.view_scene_req, view_scene_req,
			  sizeof(zb_zcl_scenes_view_scene_req_t));
		zb_buf_get_out_delayed_ext(send_view_scene_resp, idx, 0);
	}
	break;

	case ZB_ZCL_SCENES_REMOVE_SCENE_CB_ID: {
		const zb_zcl_scenes_remove_scene_req_t *remove_scene_req =
			ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(bufid, zb_zcl_scenes_remove_scene_req_t);
		zb_uint8_t idx = 0xFF;
		zb_uint8_t *remove_scene_status =
			ZB_ZCL_DEVICE_CMD_PARAM_OUT_GET(bufid, zb_uint8_t);
		const zb_zcl_parsed_hdr_t *in_cmd_info = ZB_ZCL_DEVICE_CMD_PARAM_CMD_INFO(bufid);

		LOG_INF("ZB_ZCL_SCENES_REMOVE_SCENE_CB_ID: group_id 0x%x scene_id %hd",
			remove_scene_req->group_id,
			remove_scene_req->scene_id);

		*remove_scene_status = ZB_ZCL_STATUS_NOT_FOUND;
		idx = scene_table_get_entry(remove_scene_req->group_id,
					    remove_scene_req->scene_id);

		if (idx != 0xFF &&
		    scenes_table[idx].common.group_id != ZB_ZCL_SCENES_FREE_SCENE_TABLE_RECORD) {
			/* Remove this entry */
			memset(&scenes_table[idx], 0, sizeof(scenes_table[idx]));
			scenes_table[idx].common.group_id = ZB_ZCL_SCENES_FREE_SCENE_TABLE_RECORD;
			LOG_INF("removing scene: entry idx %hd", idx);
			*remove_scene_status = ZB_ZCL_STATUS_SUCCESS;
			scenes_table_save();
		} else if (!zb_aps_is_endpoint_in_group(
				remove_scene_req->group_id,
				ZB_ZCL_PARSED_HDR_SHORT_DATA(in_cmd_info).dst_endpoint)) {
			*remove_scene_status = ZB_ZCL_STATUS_INVALID_FIELD;
		}
	}
	break;

	case ZB_ZCL_SCENES_REMOVE_ALL_SCENES_CB_ID: {
		const zb_zcl_scenes_remove_all_scenes_req_t *remove_all_scenes_req =
			ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(
				bufid, zb_zcl_scenes_remove_all_scenes_req_t);
		zb_uint8_t *remove_all_scenes_status =
			ZB_ZCL_DEVICE_CMD_PARAM_OUT_GET(bufid, zb_uint8_t);
		const zb_zcl_parsed_hdr_t *in_cmd_info = ZB_ZCL_DEVICE_CMD_PARAM_CMD_INFO(bufid);

		LOG_INF("ZB_ZCL_SCENES_REMOVE_ALL_SCENES_CB_ID: group_id 0x%x",
			remove_all_scenes_req->group_id);

		if (!zb_aps_is_endpoint_in_group(
				remove_all_scenes_req->group_id,
				ZB_ZCL_PARSED_HDR_SHORT_DATA(in_cmd_info).dst_endpoint)) {
			*remove_all_scenes_status = ZB_ZCL_STATUS_INVALID_FIELD;
		} else {
			scene_table_remove_entries_by_group(remove_all_scenes_req->group_id);
			*remove_all_scenes_status = ZB_ZCL_STATUS_SUCCESS;
			scenes_table_save();
		}
	}
	break;

	case ZB_ZCL_SCENES_STORE_SCENE_CB_ID: {
		const zb_zcl_scenes_store_scene_req_t *store_scene_req =
			ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(bufid, zb_zcl_scenes_store_scene_req_t);
		zb_uint8_t idx = 0xFF;
		zb_uint8_t *store_scene_status =
			ZB_ZCL_DEVICE_CMD_PARAM_OUT_GET(bufid, zb_uint8_t);
		const zb_zcl_parsed_hdr_t *in_cmd_info = ZB_ZCL_DEVICE_CMD_PARAM_CMD_INFO(bufid);

		LOG_INF("ZB_ZCL_SCENES_STORE_SCENE_CB_ID: group_id 0x%x scene_id %hd",
			store_scene_req->group_id,
			store_scene_req->scene_id);

		if (!zb_aps_is_endpoint_in_group(
				store_scene_req->group_id,
				ZB_ZCL_PARSED_HDR_SHORT_DATA(in_cmd_info).dst_endpoint)) {
			*store_scene_status = ZB_ZCL_STATUS_INVALID_FIELD;
		} else {
			idx = scene_table_get_entry(store_scene_req->group_id,
						    store_scene_req->scene_id);

			if (idx != 0xFF) {
				if (scenes_table[idx].common.group_id !=
				    ZB_ZCL_SCENES_FREE_SCENE_TABLE_RECORD) {
					/* Update existing entry with current On/Off state */
					device_cb_param->status = RET_ALREADY_EXISTS;
					LOG_INF("update existing scene: entry idx %hd", idx);
				} else {
					/* Create new entry with empty name
					 * and 0 transition time
					 */
					scenes_table[idx].common.group_id =
						store_scene_req->group_id;
					scenes_table[idx].common.scene_id =
						store_scene_req->scene_id;
					scenes_table[idx].common.transition_time = 0;
					LOG_INF("create new scene: entry idx %hd", idx);
				}
				save_state_as_scene(&scenes_table[idx]);
				*store_scene_status = ZB_ZCL_STATUS_SUCCESS;
				scenes_table_save();
			} else {
				*store_scene_status = ZB_ZCL_STATUS_INSUFF_SPACE;
			}
		}
	}
	break;

	case ZB_ZCL_SCENES_RECALL_SCENE_CB_ID: {
		const zb_zcl_scenes_recall_scene_req_t *recall_scene_req =
			ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(bufid, zb_zcl_scenes_recall_scene_req_t);
		zb_uint8_t idx = 0xFF;
		zb_uint8_t *recall_scene_status =
			ZB_ZCL_DEVICE_CMD_PARAM_OUT_GET(bufid, zb_uint8_t);

		LOG_INF("ZB_ZCL_SCENES_RECALL_SCENE_CB_ID: group_id 0x%x scene_id %hd",
			recall_scene_req->group_id,
			recall_scene_req->scene_id);

		idx = scene_table_get_entry(recall_scene_req->group_id,
					    recall_scene_req->scene_id);

		if (idx != 0xFF &&
		    scenes_table[idx].common.group_id != ZB_ZCL_SCENES_FREE_SCENE_TABLE_RECORD) {
			/* Recall this entry */
			recall_scene(&scenes_table[idx]);
			*recall_scene_status = ZB_ZCL_STATUS_SUCCESS;
		} else {
			*recall_scene_status = ZB_ZCL_STATUS_NOT_FOUND;
		}
	}
	break;

	case ZB_ZCL_SCENES_GET_SCENE_MEMBERSHIP_CB_ID: {
		const zb_zcl_scenes_get_scene_membership_req_t *get_scene_membership_req =
			ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(bufid,
				zb_zcl_scenes_get_scene_membership_req_t);
		const zb_zcl_parsed_hdr_t *in_cmd_info = ZB_ZCL_DEVICE_CMD_PARAM_CMD_INFO(bufid);

		LOG_INF("ZB_ZCL_SCENES_GET_SCENE_MEMBERSHIP_CB_ID: group_id 0x%x",
			get_scene_membership_req->group_id);

		/* Send View Scene Response */
		ZB_MEMCPY(&resp_info.cmd_info, in_cmd_info, sizeof(zb_zcl_parsed_hdr_t));
		ZB_MEMCPY(&resp_info.get_scene_membership_req,
			  get_scene_membership_req,
			  sizeof(zb_zcl_scenes_get_scene_membership_req_t));
		zb_buf_get_out_delayed(send_get_scene_membership_resp);
	}
	break;

	case ZB_ZCL_SCENES_INTERNAL_REMOVE_ALL_SCENES_ALL_ENDPOINTS_CB_ID: {
		const zb_zcl_scenes_remove_all_scenes_req_t *remove_all_scenes_req =
			ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(bufid,
				zb_zcl_scenes_remove_all_scenes_req_t);

		LOG_INF("ZB_ZCL_SCENES_INTERNAL_REMOVE_ALL_SCENES_ALL_ENDPOINTS_CB_ID: "
			"group_id 0x%x", remove_all_scenes_req->group_id);

		/* Have only one endpoint */
		scene_table_remove_entries_by_group(remove_all_scenes_req->group_id);
		scenes_table_save();
	}
	break;

	case ZB_ZCL_SCENES_INTERNAL_REMOVE_ALL_SCENES_ALL_ENDPOINTS_ALL_GROUPS_CB_ID: {
		scene_table_init();
		scenes_table_save();
	}
	break;

	default:
		/* Return false if the packet was not processed. */
		return ZB_FALSE;
	}

	LOG_DBG("<< %s %hd", __func__, device_cb_param->status);
	return ZB_TRUE;
}
