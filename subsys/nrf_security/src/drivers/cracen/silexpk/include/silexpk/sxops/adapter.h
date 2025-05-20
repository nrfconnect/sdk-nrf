/** "sxops" interface to read & write operands from/to memory
 *
 * @file
 *
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ADAPTER_HEADER_FILE
#define ADAPTER_HEADER_FILE

#include <stdint.h>

/** Write the EC operand into memory filling 'sz' bytes, 0-pading if needed
 *
 * @param[in] op EC Operand written to memory. Data should have
 * a size smaller or equal to 'sz'
 * @param[in] mem Memory address to write the operand to
 * @param[in] sz Size in bytes of the operand
 */
void sx_pk_ecop2mem(const sx_ecop *op, char *mem, int sz);

/** Write the operand into memory which has the exact size needed
 *
 * @param[in] op Operand written to memory. Data should be in big endian
 * @param[in] mem Memory address to write the operand to
 */
void sx_pk_op2vmem(const sx_op *op, char *mem);

/** Convert raw bytes to operand
 *
 * @param[in] mem Memory address to read the operand from
 * @param[in] sz Size in bytes of the memory to read.
 * @param[out] op Operand in which the raw little endian bytes are written.
 * Its size should be bigger or equal to 'sz'
 */
void sx_pk_mem2op(const char *mem, int sz, sx_op *op);

/** Convert raw bytes to EC operand
 *
 * @param[in] mem Memory address to read the operand from
 * @param[in] sz Size in bytes of the memory to read.
 * @param[out] op EC Operand in which the raw little endian bytes are written.
 * Its size should be bigger or equal to 'sz'
 */
void sx_pk_mem2ecop(const char *mem, int sz, sx_ecop *op);

/** Convert raw bytes to an affine point operand
 *
 * @param[in] mem_x Memory address to read x-coordinate of the operand from
 * @param[in] mem_y Memory address to read y-coordinate of the operand from
 * @param[in] sz Size in bytes of the memory to read for each coordinate
 * @param[out] op Affine point operand in which the raw little endian bytes are
 * written. Its size should be bigger or equal to 'sz'
 */
void sx_pk_mem2affpt(const char *mem_x, const char *mem_y, int sz, sx_pk_affine_point *op);

/** Write the affine point operand into memory filling 'sz' bytes, 0-pading if
 * needed
 *
 * @param[in] op Affine point operand written to memory. Data should have
 * a size smaller or equal to 'sz'
 * @param[in] mem_x Memory address to write the x-coordinate of the operand to
 * @param[in] mem_y Memory address to write the y-coordinate of the operand to
 * @param[in] sz Size in bytes of a single coordinate
 */
void sx_pk_affpt2mem(const sx_pk_affine_point *op, char *mem_x, char *mem_y, int sz);

/** Return the size in bytes of the operand.
 *
 * @param[in] op Operand
 * @return Operand size in bytes
 */
int sx_op_size(const sx_op *op);

#endif
