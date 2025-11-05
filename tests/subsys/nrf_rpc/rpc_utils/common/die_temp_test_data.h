/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DIE_TEMP_TEST_DATA_H_
#define DIE_TEMP_TEST_DATA_H_

/**
 * @file die_temp_test_data.h
 * @brief Common test data for die temperature RPC tests
 *
 * This header defines test cases using X-macros to ensure client and server
 * tests use identical test values. This guarantees consistent testing of the
 * RPC protocol encoding/decoding on both sides.
 *
 * Usage:
 *   1. Define the X macro to match your test case structure:
 *      #define X(name, centidegrees, cbor_bytes) ...
 *   2. Invoke DIE_TEMP_TEST_CASES
 *   3. Undefine X: #undef X
 *
 * X-macro parameters:
 * - name:          Test case identifier string (e.g., "positive_25.50C")
 * - centidegrees:  Temperature value in centidegrees (0.01°C units)
 * - cbor_bytes:    CBOR-encoded byte sequence in parentheses (byte1, byte2, ...)
 *                  Use UNPACK(cbor_bytes) to expand bytes in variadic contexts
 *                  Use {UNPACK(cbor_bytes)} for array initialization
 *
 * CBOR encoding sizes (covering all buffer length ranges per RFC 8949):
 * - 1 byte (header only):    0-23, or -1 to -24
 * - 2 bytes (header + u8):   24-255, or -25 to -256
 * - 3 bytes (header + u16):  256-65535, or -257 to -65536
 * - 5 bytes (header + u32):  65536-4294967295, or -65537 to -4294967296
 *
 * Note: Temperature in centidegrees = (degrees_celsius * 100)
 * Examples:
 *   25.50°C = 2550 centidegrees
 *    0.10°C = 10 centidegrees
 *   -1.00°C = -100 centidegrees
 */

/* Helper macro to unpack byte sequences for use in variadic contexts */
#define UNPACK(...) __VA_ARGS__

#define DIE_TEMP_TEST_CASES                                                                        \
	X("extreme_positive_1000.00C", 100000, (0x1A, 0x00, 0x01, 0x86, 0xA0))                     \
	X("positive_25.50C", 2550, (0x19, 0x09, 0xF6))                                             \
	X("positive_1.00C", 100, (0x18, 0x64))                                                     \
	X("small_positive_0.10C", 10, (0x0A))                                                      \
	X("zero_0.00C", 0, (0x00))                                                                 \
	X("small_negative_-0.10C", -10, (0x29))                                                    \
	X("negative_-1.00C", -100, (0x38, 0x63))                                                   \
	X("negative_-10.25C", -1025, (0x39, 0x04, 0x00))                                           \
	X("extreme_negative_-1000.00C", -100000, (0x3A, 0x00, 0x01, 0x86, 0x9F))

#endif /* DIE_TEMP_TEST_DATA_H_ */
