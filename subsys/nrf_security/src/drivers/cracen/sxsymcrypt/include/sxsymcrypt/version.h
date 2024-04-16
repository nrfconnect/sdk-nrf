/** Semantic version numbers of the sxsymcrypt API.
 *
 * The version numbering used here adheres to the concepts outlined in
 * https://semver.org/.
 *
 * @file
 * @copyright Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SXSYMCRYPT_VERSION_HEADER_FILE
#define SXSYMCRYPT_VERSION_HEADER_FILE

/** Major version number of the sxsymcrypt API
 *
 * Changes made to the API with the same major version number remain
 * backwards compatible. Applications should check at compile time that
 * current major version number matches the one they were made for.
 */
#define SXSYMCRYPT_API_MAJOR 4

/** Minor version number of the sxsymcrypt API
 *
 * New features added while maintaining backwards compatibility increment
 * the minor version number. Applications should check that the minor
 * version number is equal or larger than the minor version number
 * they were written for.
 */
#define SXSYMCRYPT_API_MINOR 0

/** Check application has compatible version numbers.
 *
 * Non-zero if the API is compatible and 0 if incompatible.
 * The application is compatible if the major number does matches
 * the library major number and the application minor number is equal or
 * smaller than the library minor number.
 */
#define SXSYMCRYPT_API_IS_COMPATIBLE(appmajor, appminor)                                           \
	(((appmajor) == SXSYMCRYPT_API_MAJOR) && ((appminor) <= SXSYMCRYPT_API_MINOR))

/** Assert that the application is compatible with the library
 *
 * If the application is not compatible, this macro will cause a compile
 * time error.
 */
#define SXSYMCRYPT_API_ASSERT_COMPATIBLE(appmajor, appminor)                                       \
	typedef char sxsymcrypt_api_check_app_semver_fail                                          \
		[(SXSYMCRYPT_API_IS_COMPATIBLE(appmajor, appminor)) ? 1 : -1]

#endif
