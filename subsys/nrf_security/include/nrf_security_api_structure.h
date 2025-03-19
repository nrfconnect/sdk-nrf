/**
 * @defgroup psa_storage PSA Secure Storage APIs
 * @brief Platform Security Architecture (PSA) Secure Storage API
 *
 * @{
 */

/**
 * @defgroup psa_its Internal Trusted Storage API
 * @brief PSA Internal Trusted Storage (ITS) API for secure internal storage
 *
 * The Internal Trusted Storage API provides an interface to store data in a secure
 * area within the device's internal storage. This API is typically used for storing
 * small, security-critical data like keys, counters, and certificates.
 *
 * @file internal_trusted_storage.h
 * @brief PSA Internal Trusted Storage API
 */

/**
 * @defgroup psa_ps Protected Storage API
 * @brief PSA Protected Storage (PS) API for encrypted external storage
 *
 * The Protected Storage API provides an interface to store data securely in
 * external storage. Data is encrypted and authenticated to protect against
 * tampering and information disclosure, even when stored in untrusted areas.
 *
 * @file protected_storage.h
 * @brief PSA Protected Storage API
 */

/** @} */
