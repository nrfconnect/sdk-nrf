/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <stdio.h>
#include <zephyr/bluetooth/mesh/access.h>
#include <bluetooth/mesh/models.h>
#include <zephyr/sys/byteorder.h>
#include "model_utils.h"
#include "mesh/net.h"
#include "mesh/access.h"

#define LOG_LEVEL CONFIG_BT_MESH_MODEL_LOG_LEVEL
#include "zephyr/logging/log.h"
LOG_MODULE_REGISTER(bt_mesh_scene_srv);

#include "common/bt_str.h"

#define SCENE_PAGE_SIZE SETTINGS_MAX_VAL_LEN
/* Account for company ID in data: */
#define VND_MODEL_SCENE_DATA_OVERHEAD sizeof(uint16_t)

struct __packed scene_data {
	uint8_t len;
	uint8_t elem_idx;
	uint16_t id;
	uint8_t data[];
};

static sys_slist_t scene_servers;

static char *scene_path(char *buf, uint16_t scene, bool vnd, uint8_t page)
{
	sprintf(buf, "%x/%c%x", scene, vnd ? 'v' : 's', page);
	return buf;
}

static inline void update_page_count(struct bt_mesh_scene_srv *srv, bool vnd,
			       uint8_t page)
{
	if (vnd) {
		srv->vndpages = MAX(page + 1, srv->vndpages);
	} else {
		srv->sigpages = MAX(page + 1, srv->sigpages);
	}
}

static const struct bt_mesh_scene_entry *
entry_find(const struct bt_mesh_model *mod, bool vnd)
{
	const struct bt_mesh_scene_entry *it;

	if (vnd) {
		extern const struct bt_mesh_scene_entry
			_bt_mesh_scene_entry_vnd_list_start[];
		extern const struct bt_mesh_scene_entry
			_bt_mesh_scene_entry_vnd_list_end[];

		for (it = _bt_mesh_scene_entry_vnd_list_start;
		     it != _bt_mesh_scene_entry_vnd_list_end; it++) {
			if (it->id.vnd.id == mod->vnd.id &&
			    it->id.vnd.company == mod->vnd.company) {
				return it;
			}
		}
	} else {
		extern const struct bt_mesh_scene_entry
			_bt_mesh_scene_entry_sig_list_start[];
		extern const struct bt_mesh_scene_entry
			_bt_mesh_scene_entry_sig_list_end[];

		for (it = _bt_mesh_scene_entry_sig_list_start;
		     it != _bt_mesh_scene_entry_sig_list_end; it++) {
			if (it->id.sig == mod->id) {
				return it;
			}
		}
	}

	return NULL;
}

static struct bt_mesh_scene_srv *srv_find(uint16_t elem_idx)
{
	struct bt_mesh_scene_srv *srv;

	SYS_SLIST_FOR_EACH_CONTAINER(&scene_servers, srv, n) {
		/* Scene servers are added to the link list in reverse
		 * composition data order. The first scene server that isn't
		 * after this element will be the right one:
		 */
		if (srv->model->elem_idx <= elem_idx) {
			return srv;
		}
	}

	return NULL;
}

static uint16_t current_scene(const struct bt_mesh_scene_srv *srv)
{
	if (srv->next && k_work_delayable_is_pending(&srv->work)) {
		/* MshMDLv1.1: 5.1.3.2.1: When we're in a transition, the current scene should be
		 * NONE. Check that we're not just in a delay phase:
		 */
		if (!srv->transition.delay ||
		    (k_ticks_to_ms_near64(k_work_delayable_remaining_get(&srv->work)) <=
		     srv->transition.time)) {
			return BT_MESH_SCENE_NONE;
		} else {
			return srv->prev;
		}
	}

	return srv->prev;
}

static uint16_t target_scene(const struct bt_mesh_scene_srv *srv)
{
	return srv->next;
}

static void scene_status_encode(struct bt_mesh_scene_srv *srv, struct net_buf_simple *buf,
				const struct bt_mesh_scene_state *state)
{
	bt_mesh_model_msg_init(buf, BT_MESH_SCENE_OP_STATUS);
	net_buf_simple_add_u8(buf, state->status);
	if (state->target != BT_MESH_SCENE_NONE) {
		net_buf_simple_add_le16(buf, state->current);
		net_buf_simple_add_le16(buf, state->target);
		net_buf_simple_add_u8(buf, model_transition_encode(state->remaining_time));
	} else {
		net_buf_simple_add_le16(buf, state->current);
	}
}

static int scene_status_send(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			     const struct bt_mesh_scene_state *state)
{
	struct bt_mesh_scene_srv *srv = model->user_data;

	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_SCENE_OP_STATUS,
				 BT_MESH_SCENE_MSG_MAXLEN_STATUS);

	scene_status_encode(srv, &buf, state);

	return bt_mesh_msg_send(model, ctx, &buf);
}

static void curr_scene_state_get(struct bt_mesh_scene_srv *srv, struct bt_mesh_scene_state *state)
{
	state->current = current_scene(srv);
	state->target = target_scene(srv);
	state->remaining_time = k_ticks_to_ms_near64(k_work_delayable_remaining_get(&srv->work));
	state->status = BT_MESH_SCENE_SUCCESS;
}

static int handle_get(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		      struct net_buf_simple *buf)
{
	struct bt_mesh_scene_srv *srv = model->user_data;
	struct bt_mesh_scene_state state;

	curr_scene_state_get(srv, &state);
	scene_status_send(model, ctx, &state);

	return 0;
}

static int scene_recall(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			 struct net_buf_simple *buf, bool ack)
{
	struct bt_mesh_scene_srv *srv = model->user_data;
	struct bt_mesh_model_transition transition;
	struct bt_mesh_scene_state state;
	uint16_t scene;
	bool has_trans;
	uint8_t tid;
	int err;

	scene = net_buf_simple_pull_le16(buf);
	if (scene == BT_MESH_SCENE_NONE) {
		return -EINVAL; /* Prohibited */
	}

	tid = net_buf_simple_pull_u8(buf);
	has_trans = !!model_transition_get(srv->model, &transition, buf);

	if (tid_check_and_update(&srv->tid, tid, ctx)) {
		LOG_DBG("Duplicate TID");
		curr_scene_state_get(srv, &state);
		scene_status_send(model, ctx, &state);
		return 0;
	}

	err = bt_mesh_scene_srv_set(srv, scene, has_trans ? &transition : NULL);
	if (err) {
		curr_scene_state_get(srv, &state);
		if (err == -ENOENT) {
			state.status = BT_MESH_SCENE_NOT_FOUND;
		}
	} else {
		state.status = BT_MESH_SCENE_SUCCESS;
		state.current = srv->prev;
		state.target = srv->next;
		state.remaining_time = transition.time;
	}

	if (ack) {
		scene_status_send(model, ctx, &state);
	}

	if (state.status == BT_MESH_SCENE_SUCCESS) {
		/* Publish */
		scene_status_send(model, NULL, &state);
	}

	return 0;
}

static int handle_recall(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			 struct net_buf_simple *buf)
{
	return scene_recall(model, ctx, buf, true);
}

static int handle_recall_unack(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			       struct net_buf_simple *buf)
{
	return scene_recall(model, ctx, buf, false);
}

static int scene_register_status_send(const struct bt_mesh_model *model,
				      struct bt_mesh_msg_ctx *ctx,
				      enum bt_mesh_scene_status status)
{
	struct bt_mesh_scene_srv *srv = model->user_data;

	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_SCENE_OP_REGISTER_STATUS,
				 BT_MESH_SCENE_MSG_MINLEN_REGISTER_STATUS +
					 2 * CONFIG_BT_MESH_SCENES_MAX);
	bt_mesh_model_msg_init(&buf, BT_MESH_SCENE_OP_REGISTER_STATUS);
	net_buf_simple_add_u8(&buf, status);
	net_buf_simple_add_le16(&buf, current_scene(srv));

	for (int i = 0; i < srv->count; i++) {
		net_buf_simple_add_le16(&buf, srv->all[i]);
	}

	return bt_mesh_msg_send(model, ctx, &buf);
}

static int handle_register_get(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			       struct net_buf_simple *buf)
{
	(void)scene_register_status_send(model, ctx, BT_MESH_SCENE_SUCCESS);

	return 0;
}

const struct bt_mesh_model_op _bt_mesh_scene_srv_op[] = {
	{
		BT_MESH_SCENE_OP_GET,
		BT_MESH_LEN_EXACT(BT_MESH_SCENE_MSG_LEN_GET),
		handle_get,
	},
	{
		BT_MESH_SCENE_OP_RECALL,
		BT_MESH_LEN_MIN(BT_MESH_SCENE_MSG_MINLEN_RECALL),
		handle_recall,
	},
	{
		BT_MESH_SCENE_OP_RECALL_UNACK,
		BT_MESH_LEN_MIN(BT_MESH_SCENE_MSG_MINLEN_RECALL),
		handle_recall_unack,
	},
	{
		BT_MESH_SCENE_OP_REGISTER_GET,
		BT_MESH_LEN_EXACT(BT_MESH_SCENE_MSG_LEN_REGISTER_GET),
		handle_register_get,
	},
	BT_MESH_MODEL_OP_END,
};

static uint16_t *scene_find(struct bt_mesh_scene_srv *srv, uint16_t scene)
{
	for (int i = 0; i < srv->count; i++) {
		if (srv->all[i] == scene) {
			return &srv->all[i];
		}
	}

	return NULL;
}

static void entry_recover(struct bt_mesh_scene_srv *srv, bool vnd,
			  const struct scene_data *data)
{
	const struct bt_mesh_elem *elem = &bt_mesh_comp_get()->elem[data->elem_idx];
	const size_t overhead = vnd ? VND_MODEL_SCENE_DATA_OVERHEAD : 0;
	const struct bt_mesh_scene_entry *entry;
	struct bt_mesh_model *mod;

	if (vnd) {
		mod = bt_mesh_model_find_vnd(elem, sys_get_le16(data->data), data->id);
	} else {
		mod = bt_mesh_model_find(elem, data->id);
	}

	if (!mod) {
		LOG_WRN("No model @%s", bt_hex(&data->elem_idx, vnd ? 5 : 3));
		return;
	}

	/* MshMDLv1.1: 5.1.3.1.1:
	 * If a model is extending another model, the extending model shall determine
	 * the Stored with Scene behavior of that model.
	 */
	if (bt_mesh_model_is_extended(mod)) {
		return;
	}

	entry = entry_find(mod, vnd);
	if (!entry) {
		LOG_WRN("No scene entry for %s", bt_hex(&data->elem_idx, vnd ? 5 : 3));
		return;
	}

	entry->recall(mod, &data->data[overhead], data->len - overhead,
		      &srv->transition);
}

static void page_recover(struct bt_mesh_scene_srv *srv, bool vnd,
			 const uint8_t buf[], size_t len)
{
	for (struct scene_data *data = (struct scene_data *)&buf[0];
	     data < (struct scene_data *)&buf[len];
	     data = (struct scene_data *)&data->data[data->len]) {
		entry_recover(srv, vnd, data);
	}
}

static ssize_t entry_store(const struct bt_mesh_model *mod,
			   const struct bt_mesh_scene_entry *entry, bool vnd,
			   uint8_t buf[])
{
	struct scene_data *data = (struct scene_data *)buf;
	ssize_t size;

	data->elem_idx = mod->elem_idx;

	if (vnd) {
		data->id = mod->vnd.id;
		sys_put_le16(mod->vnd.company, data->data);
		size = entry->store(mod,
				    &data->data[VND_MODEL_SCENE_DATA_OVERHEAD]);
		data->len = size + VND_MODEL_SCENE_DATA_OVERHEAD;
	} else {
		data->id = mod->id;
		size = entry->store(mod, &data->data[0]);
		data->len = size;
	}

	if (size > entry->maxlen) {
		LOG_ERR("Entry %s:%u:%u: data too large (%u bytes)",
		       vnd ? "vnd" : "sig", mod->elem_idx, mod->mod_idx, size);
		return -EINVAL;
	}

	if (size < 0) {
		LOG_WRN("Failed storing %s:%u:%u (%d)", vnd ? "vnd" : "sig",
			mod->elem_idx, mod->mod_idx, size);
		return size;
	}

	if (size == 0) {
		/* Silently ignore this entry */
		return 0;
	}

	return sizeof(struct scene_data) + data->len;
}

/** Store a single page of the Scene.
 *
 *  To accommodate large scene data, each scene is stored in pages of up to 256
 *  bytes.
 */
static void page_store(struct bt_mesh_scene_srv *srv, uint16_t scene,
		       uint8_t page, bool vnd, uint8_t buf[], size_t len)
{
	char path[9];
	int err;

	scene_path(path, scene, vnd, page);
	update_page_count(srv, vnd, page);

	err = bt_mesh_model_data_store(srv->model, false, path, buf, len);
	if (err) {
		LOG_ERR("Failed storing %s: %d", path, err);
	}
}

/** @brief Get the end of the Scene server's controlled elements.
 *
 *  A Scene Server controls all elements whose index is equal to or larger than
 *  its own, and smaller than the return value of this function.
 *
 *  @param[in] srv Scene Server to find control boundary of.
 *
 *  @return The element index of the next Scene Server, or the total element
 *          count.
 */
static uint16_t srv_elem_end(const struct bt_mesh_scene_srv *srv)
{
	uint16_t end = bt_mesh_elem_count();
	struct bt_mesh_scene_srv *it;

	/* As Scene Servers are added to the list in reverse order, we'll break
	 * when we find our scene server. When this happens, end will be the
	 * index of the previously checked Scene Server, which is the next Scene
	 * Server in the composition data.
	 */
	SYS_SLIST_FOR_EACH_CONTAINER(&scene_servers, it, n) {
		if (it == srv) {
			break;
		}

		end = it->model->elem_idx;
	}

	return end;
}

static void scene_recall_complete_mod(struct bt_mesh_scene_srv *srv, struct bt_mesh_model *models,
				      int model_count, bool vnd)
{
	for (int j = 0; j < model_count; j++) {
		const struct bt_mesh_scene_entry *entry;
		struct bt_mesh_model *mod = &models[j];

		if (mod == srv->model) {
			continue;
		}

		/* MshMDLv1.1: 5.1.3.1.1:
		 * If a model is extending another model, the extending
		 * model shall determine the Stored with Scene behavior
		 * of that model.
		 */
		if (bt_mesh_model_is_extended(mod)) {
			continue;
		}

		entry = entry_find(mod, vnd);
		if (!entry || !entry->recall_complete) {
			continue;
		}

		entry->recall_complete(mod);
	}
}

static void scene_recall_complete(struct bt_mesh_scene_srv *srv)
{
	const struct bt_mesh_comp *comp = bt_mesh_comp_get();
	uint16_t elem_end = srv_elem_end(srv);

	for (int i = srv->model->elem_idx; i < elem_end; i++) {
		const struct bt_mesh_elem *elem = &comp->elem[i];

		scene_recall_complete_mod(srv, elem->models, elem->model_count, false);
		scene_recall_complete_mod(srv, elem->vnd_models, elem->vnd_model_count, true);
	}
}

static void scene_store_mod(struct bt_mesh_scene_srv *srv, uint16_t scene,
			    bool vnd)
{
	const size_t data_overhead = sizeof(struct scene_data) + (vnd ? 2 : 0);
	const struct bt_mesh_comp *comp = bt_mesh_comp_get();
	uint16_t elem_end = srv_elem_end(srv);
	uint8_t buf[SCENE_PAGE_SIZE];
	uint8_t page = 0;
	size_t len = 0;

	for (int i = srv->model->elem_idx; i < elem_end; i++) {
		const struct bt_mesh_elem *elem = &comp->elem[i];
		struct bt_mesh_model *models = vnd ? elem->vnd_models : elem->models;
		int model_count = vnd ? elem->vnd_model_count : elem->model_count;

		for (int j = 0; j < model_count; j++) {
			const struct bt_mesh_scene_entry *entry;
			struct bt_mesh_model *mod = &models[j];
			ssize_t size;

			if (mod == srv->model) {
				continue;
			}

			/* MshMDLv1.1: 5.1.3.1.1:
			 * If a model is extending another model, the extending
			 * model shall determine the Stored with Scene behavior
			 * of that model.
			 */
			if (bt_mesh_model_is_extended(mod)) {
				continue;
			}

			entry = entry_find(mod, vnd);
			if (!entry) {
				continue;
			}

			if (len + data_overhead + entry->maxlen >= SCENE_PAGE_SIZE) {
				page_store(srv, scene, page++, vnd, buf, len);
				len = 0;
			}

			size = entry_store(mod, entry, vnd, &buf[len]);
			len += MAX(0, size);
		}
	}

	if (len) {
		page_store(srv, scene, page, vnd, buf, len);
	}
}

static enum bt_mesh_scene_status scene_store(struct bt_mesh_scene_srv *srv,
					     uint16_t scene)
{
	uint16_t *existing = scene_find(srv, scene);

	if (!existing) {
		if (srv->count == ARRAY_SIZE(srv->all)) {
			LOG_ERR("Out of space");
			return BT_MESH_SCENE_REGISTER_FULL;
		}

		srv->all[srv->count++] = scene;
	}

	scene_store_mod(srv, scene, false);
	scene_store_mod(srv, scene, true);

	srv->prev = scene;
	srv->next = BT_MESH_SCENE_NONE;
	/* We're checking srv->next in the handler, so failure to cancel is okay: */
	(void)k_work_cancel_delayable(&srv->work);

	return BT_MESH_SCENE_SUCCESS;
}

static void scene_delete(struct bt_mesh_scene_srv *srv, uint16_t *scene)
{
	uint8_t path[9];

	LOG_DBG("0x%x", *scene);

	for (int i = 0; i < srv->sigpages; i++) {
		scene_path(path, *scene, false, i);
		(void)bt_mesh_model_data_store(srv->model, false, path, NULL, 0);
	}

	for (int i = 0; i < srv->vndpages; i++) {
		scene_path(path, *scene, true, i);
		(void)bt_mesh_model_data_store(srv->model, false, path, NULL, 0);
	}

	uint16_t target = target_scene(srv);
	uint16_t current = current_scene(srv);

	if (target == *scene ||
	    (current == *scene && target == BT_MESH_SCENE_NONE)) {
		srv->next = BT_MESH_SCENE_NONE;
		srv->prev = BT_MESH_SCENE_NONE;

		/* Cancel failure checked in work handler. */
		(void)k_work_cancel_delayable(&srv->work);
	} else if (current == *scene && target != *scene) {
		srv->prev = BT_MESH_SCENE_NONE;
	}

	*scene = srv->all[--srv->count];
}

static int handle_store(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			struct net_buf_simple *buf)
{
	struct bt_mesh_scene_srv *srv = model->user_data;
	enum bt_mesh_scene_status status;
	uint16_t scene_number;

	scene_number = net_buf_simple_pull_le16(buf);
	if (scene_number == BT_MESH_SCENE_NONE) {
		return -EINVAL;
	}

	status = scene_store(srv, scene_number);
	scene_register_status_send(model, ctx, status);

	return 0;
}

static int handle_store_unack(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf)
{
	struct bt_mesh_scene_srv *srv = model->user_data;
	uint16_t scene_number;

	scene_number = net_buf_simple_pull_le16(buf);
	if (scene_number == BT_MESH_SCENE_NONE) {
		return -EINVAL;
	}

	(void)scene_store(srv, scene_number);

	return 0;
}

static int delete_scene(struct bt_mesh_scene_srv *srv, struct bt_mesh_msg_ctx *ctx,
			struct net_buf_simple *buf)
{
	uint16_t *scene;

	scene = scene_find(srv, net_buf_simple_pull_le16(buf));
	if (scene != BT_MESH_SCENE_NONE) {
		scene_delete(srv, scene);
	}

	return 0;
}

static int handle_delete(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			 struct net_buf_simple *buf)
{
	struct bt_mesh_scene_srv *srv = model->user_data;
	int err;

	err = delete_scene(srv, ctx, buf);
	if (err) {
		return err;
	}

	scene_register_status_send(model, ctx, BT_MESH_SCENE_SUCCESS);

	return 0;
}

static int handle_delete_unack(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			       struct net_buf_simple *buf)
{
	struct bt_mesh_scene_srv *srv = model->user_data;

	return delete_scene(srv, ctx, buf);
}

const struct bt_mesh_model_op _bt_mesh_scene_setup_srv_op[] = {
	{
		BT_MESH_SCENE_OP_STORE,
		BT_MESH_LEN_EXACT(BT_MESH_SCENE_MSG_LEN_STORE),
		handle_store,
	},
	{
		BT_MESH_SCENE_OP_STORE_UNACK,
		BT_MESH_LEN_EXACT(BT_MESH_SCENE_MSG_LEN_STORE),
		handle_store_unack,
	},
	{
		BT_MESH_SCENE_OP_DELETE,
		BT_MESH_LEN_EXACT(BT_MESH_SCENE_MSG_LEN_DELETE),
		handle_delete,
	},
	{
		BT_MESH_SCENE_OP_DELETE_UNACK,
		BT_MESH_LEN_EXACT(BT_MESH_SCENE_MSG_LEN_DELETE),
		handle_delete_unack,
	},
	BT_MESH_MODEL_OP_END,
};

static void scene_srv_transition_end(struct k_work *work)
{
	struct bt_mesh_scene_srv *srv =
		CONTAINER_OF(k_work_delayable_from_work(work), struct bt_mesh_scene_srv, work);

	if (srv->next == BT_MESH_SCENE_NONE) {
		/* This is already done */
		return;
	}

	srv->prev = srv->next;
	srv->next = BT_MESH_SCENE_NONE;

	/* Publish at the end of the transition */
	if (bt_mesh_model_transition_time(&srv->transition)) {
		bt_mesh_scene_srv_pub(srv, NULL);
	}
}

static int scene_srv_pub_update(const struct bt_mesh_model *model)
{
	struct bt_mesh_scene_srv *srv = model->user_data;
	struct bt_mesh_scene_state state;

	curr_scene_state_get(srv, &state);
	scene_status_encode(srv, &srv->pub_msg, &state);
	return 0;
}

static int scene_srv_init(const struct bt_mesh_model *model)
{
	struct bt_mesh_scene_srv *srv = model->user_data;

	sys_slist_prepend(&scene_servers, &srv->n);

	srv->model = model;

	k_work_init_delayable(&srv->work, scene_srv_transition_end);

	net_buf_simple_init_with_data(&srv->pub_msg, srv->buf,
				      sizeof(srv->buf));
	srv->pub.msg = &srv->pub_msg;
	srv->pub.update = scene_srv_pub_update;
	return 0;
}

static int scene_srv_set(const struct bt_mesh_model *model, const char *path,
			 size_t len_rd, settings_read_cb read_cb, void *cb_arg)
{
	struct bt_mesh_scene_srv *srv = model->user_data;
	uint8_t buf[SCENE_PAGE_SIZE];
	uint16_t scene;
	ssize_t size;
	uint8_t page;
	bool vnd;

	LOG_DBG("path: %s", path);

	/* The entire model data tree is loaded in this callback. Depending on
	 * the path and whether we have started the mesh, we'll handle the data
	 * differently:
	 *
	 * - Path "XXXX/vYY": Scene XXXX vendor model page YY
	 * - Path "XXXX/sYY": Scene XXXX sig model page YY
	 */
	scene = strtol(path, NULL, 16);
	if (scene == BT_MESH_SCENE_NONE) {
		LOG_ERR("Unknown data %s", path);
		return 0;
	}

	settings_name_next(path, &path);
	if (!path) {
		return 0;
	}

	vnd = path[0] == 'v';
	page = strtol(&path[1], NULL, 16);
	update_page_count(srv, vnd, page);

	/* Before starting the mesh, we'll just register that the scene exists:
	 * Once the mesh starts, we'll load the current scene, and end up in
	 * this callback again, but bt_mesh_is_provisioned() will be true.
	 */
	if (!bt_mesh_is_provisioned()) {
		if (scene_find(srv, scene)) {
			return 0;
		}

		if (srv->count == ARRAY_SIZE(srv->all)) {
			LOG_WRN("No room for scene 0x%x", scene);
			return 0;
		}

		LOG_DBG("Recovered scene 0x%x", scene);
		srv->all[srv->count++] = scene;
		return 0;
	}

	size = read_cb(cb_arg, &buf, sizeof(buf));
	if (size < 0) {
		LOG_ERR("Failed loading scene 0x%x", scene);
		return -EINVAL;
	}

	LOG_DBG("0x%x: %s", scene, bt_hex(buf, size));
	page_recover(srv, vnd, buf, size);
	return 0;
}

static void scene_srv_reset(const struct bt_mesh_model *model)
{
	struct bt_mesh_scene_srv *srv = model->user_data;

	srv->next = BT_MESH_SCENE_NONE;

	while (srv->count) {
		scene_delete(srv, &srv->all[0]);
	}

	srv->prev = BT_MESH_SCENE_NONE;
	/* We're checking srv->next in the handler, so failure to cancel is okay: */
	(void)k_work_cancel_delayable(&srv->work);
	srv->sigpages = 0;
	srv->vndpages = 0;
}

const struct bt_mesh_model_cb _bt_mesh_scene_srv_cb = {
	.init = scene_srv_init,
	.settings_set = scene_srv_set,
	.reset = scene_srv_reset,
};

static int scene_setup_srv_init(const struct bt_mesh_model *model)
{
	struct bt_mesh_scene_srv *srv = model->user_data;
	struct bt_mesh_dtt_srv *dtt_srv = NULL;
	int err;

	if (!srv) {
		return -EINVAL;
	}

	srv->setup_mod = model;

	if (IS_ENABLED(CONFIG_BT_MESH_DTT_SRV)) {
		dtt_srv = bt_mesh_dtt_srv_get(bt_mesh_model_elem(model));
	}

	if (!dtt_srv) {
		LOG_ERR("Failed to find Generic DTT Server on element");
		return -EINVAL;
	}

	err = bt_mesh_model_extend(srv->setup_mod, dtt_srv->model);
	if (err) {
		return err;
	}

#if defined(CONFIG_BT_MESH_COMP_PAGE_1)
	err = bt_mesh_model_correspond(srv->setup_mod, srv->model);
	if (err) {
		return err;
	}
#endif

	return bt_mesh_model_extend(srv->setup_mod, srv->model);
}

const struct bt_mesh_model_cb _bt_mesh_scene_setup_srv_cb = {
	.init = scene_setup_srv_init,
};

void bt_mesh_scene_invalidate(const struct bt_mesh_model *mod)
{
	struct bt_mesh_scene_srv *srv = srv_find(mod->elem_idx);

	if (!srv) {
		return;
	}

	srv->prev = BT_MESH_SCENE_NONE;
	/* We're checking srv->next in the handler, so failure to cancel is okay: */
	(void)k_work_cancel_delayable(&srv->work);
	srv->next = BT_MESH_SCENE_NONE;
}

int bt_mesh_scene_srv_set(struct bt_mesh_scene_srv *srv, uint16_t scene,
			  struct bt_mesh_model_transition *transition)
{
	int32_t transition_time;
	uint16_t curr;
	char path[25];
	int err;

	if (scene == BT_MESH_SCENE_NONE ||
	    model_transition_is_invalid(transition)) {
		return -EINVAL;
	}

	if (!scene_find(srv, scene)) {
		LOG_WRN("Unknown scene 0x%x", scene);
		return -ENOENT;
	}

	curr = current_scene(srv);
	if (scene == curr) {
		srv->prev = scene;
		srv->next = BT_MESH_SCENE_NONE;
		srv->transition.time = 0;
		srv->transition.delay = 0;
		/* We're checking srv->next in the handler, so failure to cancel is okay: */
		(void)k_work_cancel_delayable(&srv->work);
		return 0;
	}

	transition_time = bt_mesh_model_transition_time(transition);
	if (transition_time) {
		srv->transition = *transition;
		srv->prev = curr;
		srv->next = scene;
		k_work_reschedule(&srv->work, K_MSEC(transition_time));
	} else {
		srv->prev = scene;
		srv->next = BT_MESH_SCENE_NONE;
		srv->transition.time = 0;
		srv->transition.delay = 0;
		/* We're checking srv->next in the handler, so failure to cancel is okay: */
		(void)k_work_cancel_delayable(&srv->work);
	}

	sprintf(path, "bt/mesh/s/%x/data/%x",
		(srv->model->elem_idx << 8) | srv->model->mod_idx, scene);

	LOG_DBG("Loading %s", path);

	err = settings_load_subtree(path);
	if (!err) {
		scene_recall_complete(srv);
	}

	return err;
}

int bt_mesh_scene_srv_pub(struct bt_mesh_scene_srv *srv,
			  struct bt_mesh_msg_ctx *ctx)
{
	struct bt_mesh_scene_state state;

	curr_scene_state_get(srv, &state);
	return scene_status_send(srv->model, ctx, &state);
}

uint16_t
bt_mesh_scene_srv_current_scene_get(const struct bt_mesh_scene_srv *srv)
{
	return current_scene(srv);
}

uint16_t bt_mesh_scene_srv_target_scene_get(const struct bt_mesh_scene_srv *srv)
{
	return target_scene(srv);
}
