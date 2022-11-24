/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _SMS_CMS_CODES_INCLUDE_H_
#define _SMS_CMS_CODES_INCLUDE_H_

/**
 * @brief Retrieve a statically allocated textual description for a given CMS error reason.
 *
 * @param reason CMS error reason.
 * @return CMS error reason description.
 *	   If no textual description for the given error is found,
 *	   a placeholder string is returned instead.
 */
const char *sms_cms_error_str(int reason);

#endif /* _SMS_CMS_CODES_INCLUDE_H_ */
