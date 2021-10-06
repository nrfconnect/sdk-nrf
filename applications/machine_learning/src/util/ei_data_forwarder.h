/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _EI_DATA_FORWARDER_H_
#define _EI_DATA_FORWARDER_H_


int ei_data_forwarder_parse_data(const float *data_ptr, size_t data_cnt,
				 char *buf, size_t buf_size);

#endif /* _EI_DATA_FORWARDER_H_ */
