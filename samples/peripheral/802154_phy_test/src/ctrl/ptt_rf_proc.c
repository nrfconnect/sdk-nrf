/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/* Purpose: processing external events from RF */

#include <stddef.h>

#include "ptt_ctrl_internal.h"
#include "ptt_events_internal.h"
#include "rf/ptt_rf.h"

#include "ptt_proto.h"

#include "ctrl/ptt_trace.h"

#ifdef TESTS
#include "test_rf_proc_conf.h"
#endif /* TESTS */

void ptt_ctrl_rf_tx_started(ptt_evt_id_t evt_id)
{
	PTT_TRACE_FUNC_ENTER_WITH_EVT(evt_id);

	ptt_ext_evt_handler handler = ptt_ctrl_get_handler_rf_tx_started();

	if (handler != NULL) {
		handler(evt_id);
	} else {
		ptt_event_free(evt_id);
	}

	PTT_TRACE_FUNC_EXIT();
}

void ptt_ctrl_rf_tx_finished(ptt_evt_id_t evt_id)
{
	PTT_TRACE_FUNC_ENTER_WITH_EVT(evt_id);

	ptt_ext_evt_handler handler = ptt_ctrl_get_handler_rf_tx_finished();

	if (handler != NULL) {
		handler(evt_id);
	} else {
		ptt_event_free(evt_id);
	}

	PTT_TRACE_FUNC_EXIT();
}

void ptt_ctrl_rf_tx_failed(ptt_evt_id_t evt_id, ptt_rf_tx_error_t tx_error)
{
	PTT_TRACE("%s -> evt %d res %d\n", __func__, evt_id, tx_error);

	ptt_ext_evt_handler handler = ptt_ctrl_get_handler_rf_tx_failed();

	ptt_event_set_ctx_data(evt_id, PTT_CAST_TO_UINT8_P(&tx_error), sizeof(tx_error));

	if (handler != NULL) {
		handler(evt_id);
	} else {
		ptt_event_free(evt_id);
	}

	PTT_TRACE_FUNC_EXIT();
}

void ptt_ctrl_rf_cca_done(ptt_evt_id_t evt_id, bool result)
{
	PTT_TRACE("%s -> evt %d res %d\n", __func__, evt_id, result);

	ptt_ext_evt_handler handler = ptt_ctrl_get_handler_rf_cca_done();

	ptt_event_set_ctx_data(evt_id, PTT_CAST_TO_UINT8_P(&result), sizeof(result));

	if (handler != NULL) {
		handler(evt_id);
	} else {
		ptt_event_free(evt_id);
	}

	PTT_TRACE_FUNC_EXIT();
}

void ptt_ctrl_rf_cca_failed(ptt_evt_id_t evt_id)
{
	PTT_TRACE_FUNC_ENTER_WITH_EVT(evt_id);

	ptt_ext_evt_handler handler = ptt_ctrl_get_handler_rf_cca_failed();

	if (handler != NULL) {
		handler(evt_id);
	} else {
		ptt_event_free(evt_id);
	}

	PTT_TRACE_FUNC_EXIT();
}

void ptt_ctrl_rf_ed_detected(ptt_evt_id_t evt_id, ptt_ed_t result)
{
	PTT_TRACE("%s -> evt %d res %d\n", __func__, evt_id, result);

	ptt_ext_evt_handler handler = ptt_ctrl_get_handler_rf_ed_detected();

	ptt_event_set_ctx_data(evt_id, PTT_CAST_TO_UINT8_P(&result), sizeof(result));

	if (handler != NULL) {
		handler(evt_id);
	} else {
		ptt_event_free(evt_id);
	}

	PTT_TRACE_FUNC_EXIT();
}

void ptt_ctrl_rf_ed_failed(ptt_evt_id_t evt_id)
{
	PTT_TRACE_FUNC_ENTER_WITH_EVT(evt_id);

	ptt_ext_evt_handler handler = ptt_ctrl_get_handler_rf_ed_failed();

	if (handler != NULL) {
		handler(evt_id);
	} else {
		ptt_event_free(evt_id);
	}

	PTT_TRACE_FUNC_EXIT();
}

void ptt_ctrl_rf_push_packet(const uint8_t *pkt, ptt_pkt_len_t len, int8_t rssi, uint8_t lqi)
{
	PTT_TRACE_FUNC_ENTER();

	enum ptt_ret ret;
	ptt_evt_id_t evt_id;
	ptt_ext_evt_handler handler = ptt_ctrl_get_handler_rf_rx_done();

	ret = ptt_event_alloc_and_fill(&evt_id, pkt, len);

	if (ret != PTT_RET_SUCCESS) {
		PTT_TRACE("%s: ptt_event_alloc_and_fill returned error code: %d", __func__, ret);
	} else {
		struct ptt_rf_packet_info_s pkt_info = { .rssi = rssi, .lqi = lqi };

		ptt_event_set_ctx_data(evt_id, (uint8_t *)(&pkt_info), sizeof(pkt_info));

		if (handler != NULL) {
			handler(evt_id);
		} else {
			ptt_event_free(evt_id);
		}
	}
}

void ptt_ctrl_rf_rx_failed(ptt_rf_rx_error_t rx_error)
{
	PTT_TRACE("%s -> res %d\n", __func__, rx_error);

	ptt_ext_evt_handler handler = ptt_ctrl_get_handler_rf_rx_failed();

	if (handler != NULL) {
		enum ptt_ret ret;
		ptt_evt_id_t evt_id;

		ret = ptt_event_alloc(&evt_id);

		if (ret != PTT_RET_SUCCESS) {
			PTT_TRACE("%s: ptt_event_alloc returned error code: %d", __func__, ret);
		} else {
			ptt_event_set_ctx_data(evt_id, PTT_CAST_TO_UINT8_P(&rx_error),
					       sizeof(rx_error));
			handler(evt_id);
		}
	}

	PTT_TRACE_FUNC_EXIT();
}

#ifdef TESTS
#include "test_rf_proc_wrappers.c"
#endif /* TESTS */
