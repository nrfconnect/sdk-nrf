/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef LWM2M_DUMMY_H__
#define LWM2M_DUMMY_H__

struct lwm2m_obj_path {
	uint16_t obj_id;
	uint16_t obj_inst_id;
	uint16_t res_id;
	uint16_t res_inst_id;
	uint8_t  level;  /* 0/1/2/3/4 (4 = resource instance) */
};

#endif
