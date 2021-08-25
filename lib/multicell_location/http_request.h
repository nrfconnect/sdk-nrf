/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef HTTP_REQUEST_H_
#define HTTP_REQUEST_H_

int execute_http_request(const char *request, size_t request_len,
			 char *response, uint16_t response_len);

#endif /* HTTP_REQUEST_H_ */
