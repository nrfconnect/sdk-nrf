/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/* Purpose: events implementation */

#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "ctrl/ptt_trace.h"

#include "ptt_events_internal.h"

#ifdef TESTS
#include "test_events_conf.h"
#else
#include "ptt_ctrl_internal.h"
#endif

void ptt_events_init(void)
{
	PTT_TRACE("%s ->\n", __func__);

	struct ptt_evt_ctx_s *evt_ctx = ptt_ctrl_get_evt_ctx();

	*evt_ctx = (struct ptt_evt_ctx_s){ 0 };

	PTT_TRACE("%s -<\n", __func__);
}

void ptt_events_uninit(void)
{
	PTT_TRACE("%s\n", __func__);
}

void ptt_events_reset_all(void)
{
	PTT_TRACE("%s ->\n", __func__);

	ptt_events_uninit();
	ptt_events_init();

	PTT_TRACE("%s -<\n", __func__);
}

enum ptt_ret ptt_event_alloc(ptt_evt_id_t *evt_id)
{
	PTT_TRACE("%s -> evt %p\n", __func__, evt_id);

	uint8_t i;
	struct ptt_evt_ctx_s *evt_ctx = ptt_ctrl_get_evt_ctx();
	enum ptt_ret ret = PTT_RET_SUCCESS;

	assert(evt_ctx != NULL);

	if (evt_id == NULL) {
		ret = PTT_RET_NULL_PTR;
		PTT_TRACE("%s -< ret %d\n", __func__, ret);
		return ret;
	}

	for (i = 0; i < PTT_EVENT_POOL_N; ++i) {
		if (false == evt_ctx->evt_pool[i].use) {
			*evt_id = i;
			evt_ctx->evt_pool[i].use = true;

			ret = PTT_RET_SUCCESS;
			PTT_TRACE("%s -< ret %d\n", __func__, ret);
			return ret;
		}
	}

	ret = PTT_RET_NO_FREE_SLOT;
	PTT_TRACE("%s -< ret %d\n", __func__, ret);
	return ret;
}

enum ptt_ret ptt_event_free(ptt_evt_id_t evt_id)
{
	PTT_TRACE("%s -> evt_id %d\n", __func__, evt_id);

	enum ptt_ret ret = PTT_RET_SUCCESS;

	if (evt_id >= PTT_EVENT_POOL_N) {
		ret = PTT_RET_INVALID_VALUE;
		PTT_TRACE("%s -< ret %d\n", __func__, ret);
		return ret;
	}

	struct ptt_evt_ctx_s *evt_ctx = ptt_ctrl_get_evt_ctx();

	assert(evt_ctx != NULL);

	if (false == evt_ctx->evt_pool[evt_id].use) {
		/* evt already inactive */
		ret = PTT_RET_SUCCESS;
		PTT_TRACE("%s -< ret %d\n", __func__, ret);
		return ret;
	}

	evt_ctx->evt_pool[evt_id] = (struct ptt_event_s){ 0 };

	PTT_TRACE("%s -< ret %d\n", __func__, ret);
	return ret;
}

enum ptt_ret ptt_event_alloc_and_fill(ptt_evt_id_t *evt_id, const uint8_t *pkt, ptt_pkt_len_t len)
{
	PTT_TRACE("%s -> evt %p pkt %p len %d\n", __func__, evt_id, pkt, len);

	enum ptt_ret ret = PTT_RET_SUCCESS;

	if (pkt == NULL) {
		ret = PTT_RET_NULL_PTR;
		PTT_TRACE("%s -< ret %d\n", __func__, ret);
		return ret;
	}
	if (len > PTT_EVENT_DATA_SIZE) {
		ret = PTT_RET_INVALID_VALUE;
		PTT_TRACE("%s -< ret %d\n", __func__, ret);
		return ret;
	}
	if (len == 0) {
		ret = PTT_RET_INVALID_VALUE;
		PTT_TRACE("%s -< ret %d\n", __func__, ret);
		return ret;
	}

	ret = ptt_event_alloc(evt_id);

	if (ret != PTT_RET_SUCCESS) {
		PTT_TRACE("%s -< ret %d\n", __func__, ret);
		return ret;
	}

	struct ptt_event_s *event = ptt_get_event_by_id(*evt_id);

	assert(event != NULL);

	event->data.len = len;
	memcpy(event->data.arr, pkt, event->data.len);

	PTT_TRACE("%s -< ret %d\n", __func__, ret);
	return ret;
}

struct ptt_event_s *ptt_get_event_by_id(ptt_evt_id_t evt_id)
{
	PTT_TRACE("%s -> evt_id %d\n", __func__, evt_id);

	if (evt_id >= PTT_EVENT_POOL_N) {
		return NULL;
	}

	struct ptt_evt_ctx_s *evt_ctx = ptt_ctrl_get_evt_ctx();

	assert(evt_ctx != NULL);

	if (false == evt_ctx->evt_pool[evt_id].use) {
		/* event inactive */
		return NULL;
	}

	return &evt_ctx->evt_pool[evt_id];
}

void ptt_event_set_cmd(ptt_evt_id_t evt_id, ptt_evt_cmd_t cmd)
{
	PTT_TRACE("%s -> evt_id %d cmd %d\n", __func__, evt_id, cmd);

	struct ptt_event_s *event = ptt_get_event_by_id(evt_id);

	assert(event != NULL);

	event->cmd = cmd;

	PTT_TRACE("%s -<\n", __func__);
}

ptt_evt_cmd_t ptt_event_get_cmd(ptt_evt_id_t evt_id)
{
	PTT_TRACE("%s -> evt_id %d\n", __func__, evt_id);

	struct ptt_event_s *event = ptt_get_event_by_id(evt_id);

	assert(event != NULL);

	PTT_TRACE("%s -< cmd %d\n", __func__, event->cmd);
	return event->cmd;
}

void ptt_event_set_state(ptt_evt_id_t evt_id, ptt_evt_state_t state)
{
	PTT_TRACE("%s -> evt_id %d state %d\n", __func__, evt_id, state);

	struct ptt_event_s *event = ptt_get_event_by_id(evt_id);

	assert(event != NULL);

	event->state = state;

	PTT_TRACE("%s -<\n", __func__);
}

ptt_evt_state_t ptt_event_get_state(ptt_evt_id_t evt_id)
{
	PTT_TRACE("%s -> evt_id %d\n", __func__, evt_id);

	struct ptt_event_s *event = ptt_get_event_by_id(evt_id);

	assert(event != NULL);

	PTT_TRACE("%s -< state %d\n", __func__, event->state);
	return event->state;
}

void ptt_event_set_ctx_data(ptt_evt_id_t evt_id, const uint8_t *data, uint16_t len)
{
	PTT_TRACE("%s -> evt_id %d data %p len %d\n", __func__, evt_id, data, len);

	struct ptt_event_s *event = ptt_get_event_by_id(evt_id);

	assert(event != NULL);

	assert(data != NULL);
	assert(len <= PTT_EVENT_CTX_SIZE);

	event->ctx.len = len;
	memcpy(event->ctx.arr, data, event->ctx.len);

	PTT_TRACE("%s -<\n", __func__);
}

struct ptt_evt_ctx_data_s *ptt_event_get_ctx_data(ptt_evt_id_t evt_id)
{
	PTT_TRACE("%s -> evt_id %d\n", __func__, evt_id);

	struct ptt_event_s *event = ptt_get_event_by_id(evt_id);

	assert(event != NULL);

	PTT_TRACE("%s -< ctx %p\n", __func__, &event->ctx);
	return &event->ctx;
}

void ptt_event_set_data(ptt_evt_id_t evt_id, const uint8_t *data, uint16_t len)
{
	PTT_TRACE("%s -> evt_id %d data %p len %d\n", __func__, evt_id, data, len);

	struct ptt_event_s *event = ptt_get_event_by_id(evt_id);

	assert(event != NULL);

	assert(data != NULL);
	assert(len <= PTT_EVENT_DATA_SIZE);

	event->data.len = len;
	memcpy(event->data.arr, data, event->data.len);

	PTT_TRACE("%s -<\n", __func__);
}

struct ptt_evt_data_s *ptt_event_get_data(ptt_evt_id_t evt_id)
{
	PTT_TRACE("%s -> evt_id %d\n", __func__, evt_id);

	struct ptt_event_s *event = ptt_get_event_by_id(evt_id);

	assert(event != NULL);

	PTT_TRACE("%s -< ctx %p\n", __func__, &event->data);
	return &event->data;
}
