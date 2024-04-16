/** Semantic version numbers of the SilexPK sxops API.
 *
 * The version numbering used here adheres to the concepts outlined in
 * https://semver.org/.
 *
 * @file
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SX_PK_SXOPS_VERSION_HEADER_FILE
#define SX_PK_SXOPS_VERSION_HEADER_FILE

/** Major version number of the SilexPK sxops API
 *
 * Changes made to the API with the same major version number remain
 * backwards compatible. Applications should check at compile time that
 * current major version number matches the one they were made for.
 */
#define SX_PK_SXOPS_API_MAJOR 2

/** Minor version number of the SilexPK sxops API
 *
 * New features added while maintaining backwards compatibility increment
 * the minor version number. Applications should check that the minor
 * version number is equal or larger than the minor version number
 * they were written for.
 */
#define SX_PK_SXOPS_API_MINOR 0

/** Check application has compatible version numbers.
 *
 * Non-zero if the API is compatible and zero if incompatible.
 * The application is compatible if the major number does matches
 * the library major number and the application minor number is equal or
 * smaller than the library minor number.
 */
#define SX_PK_SXOPS_API_IS_COMPATIBLE(appmajor, appminor)                                          \
	(((appmajor) == SX_PK_SXOPS_API_MAJOR) && ((appminor) <= SX_PK_SXOPS_API_MINOR))

/** Assert that the application is compatible with the sxops library
 *
 * If the application is not compatible, this macro will cause a compile
 * time error.
 */
#define SX_PK_SXOPS_API_ASSERT_COMPATIBLE(appmajor, appminor)                                      \
	typedef char sx_pk_sxops_api_check_app_semver_fail                                         \
		[(SX_PK_SXOPS_API_IS_COMPATIBLE(appmajor, appminor)) ? 1 : -1]

#endif
