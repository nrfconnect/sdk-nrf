/**
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef IOMEM_HEADER_FILE
#define IOMEM_HEADER_FILE

/** Clear device memory
 *
 * @param[in] dst Memory to clear.
 * Will be zeroed after this call
 * @param[in] sz Number of bytes to clear
 */
void sx_clrpkmem(void *dst, size_t sz);

/** Write src to device memory at dst.
 *
 * The write to device memory will always use write instructions at naturally
 * aligned addresses.
 *
 * @param[out] dst Destination of write operation.
 * Will be modified after this call
 * @param[in] src Source of write operation
 * @param[in] sz The number of bytes to write from src to dst
 */
void sx_wrpkmem(void *dst, const void *src, size_t sz);

/** Write a byte to device memory at dst.
 *
 * Byte accesses don't have alignments requirements
 * since they are always aligned.
 * This function is created to have a consistent strategy
 * when we write to device memory.
 *
 * @param[out] dst Destination of write operation.
 * Will be modified after this call
 * @param[in] input_byte The byte value to be written.
 */
static inline void sx_wrpkmem_byte(void *dst, char input_byte)
{
	volatile char *d = (volatile char *)dst;
	*d = input_byte;
}

/** Read from device memory at src into normal memory at dst.
 *
 * The read from device memory will always use read instructions at naturally
 * aligned addresses.
 *
 * @param[out] dst Destination of read operation.
 * Will be modified after this call
 * @param[in] src Source of read operation
 * @param[in] sz The number of bytes to read from src to dst
 */
void sx_rdpkmem(void *dst, const void *src, size_t sz);

/** Read a byte from device memory at src.
 *
 * Byte accesses don't have alignment requirements
 * since they are always aligned.
 * This function is created to have a consistent strategy
 * when we write to device memory.
 *
 * @param[in] src Address of read operation.
 *
 * @return Byte value from the src.
 */
static inline unsigned char sx_rdpkmem_byte(const void *src)
{
	volatile unsigned char *s = (volatile unsigned char *)src;
	return s[0];
}

#endif
