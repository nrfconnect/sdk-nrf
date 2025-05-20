/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <assert.h>

#include <silexpk/core.h>
#include <silexpk/sxbuf/sxbufop.h>
#include <silexpk/iomem.h>

#ifndef SX_PK_MAX_OP_SZ
#define SX_PK_MAX_OP_SZ (4096 / 8)
#endif

/** Little endian function of sx_pk_ecop2mem() */
void sx_pk_ecop2mem_le(const sx_ecop *op, char *mem, int sz)
{
	int cursz = op->sz;
	int diff = sz - cursz;

	assert(cursz <= sz);
	sx_clrpkmem(mem + cursz, diff);

	char buf[SX_PK_MAX_OP_SZ];
	char *ptr = op->bytes;

	assert(cursz <= SX_PK_MAX_OP_SZ);
	for (int i = 0; i < cursz; i += 1) {
		buf[i] = ptr[cursz - i - 1];
	}
	sx_wrpkmem(mem, buf, cursz);
}

/** Big endian function of sx_pk_ecop2mem() */
void sx_pk_ecop2mem_be(const sx_ecop *op, char *mem, int sz)
{
	int cursz = op->sz;
	int diff = sz - cursz;

	assert(cursz <= sz);
	sx_clrpkmem(mem, diff);

	sx_wrpkmem(mem + diff, op->bytes, cursz);
}

void sx_pk_ecop2mem(const sx_ecop *op, char *mem, int sz)
{
	sx_pk_ecop2mem_be(op, mem, sz);
}

/** Little endian function of sx_pk_op2vmem() */
void sx_pk_op2vmem_le(const sx_op *op, char *mem)
{
	int sz = op->sz;
	char buf[SX_PK_MAX_OP_SZ];
	char *ptr = op->bytes;

	assert(sz <= SX_PK_MAX_OP_SZ);
	for (int i = 0; i < sz; i += 1) {
		buf[i] = ptr[sz - i - 1];
	}
	sx_wrpkmem(mem, buf, sz);
}

/** Big endian function of sx_pk_op2vmem() */
void sx_pk_op2vmem_be(const sx_op *op, char *mem)
{
	sx_wrpkmem(mem, op->bytes, op->sz);
}

void sx_pk_op2vmem(const sx_op *op, char *mem)
{
	sx_pk_op2vmem_be(op, mem);
}

int sx_op_size(sx_op *op)
{
	return op->sz;
}

/** Big endian function of sx_pk_mem2op() */
void sx_pk_mem2op_be(const char *mem, int sz, sx_op *op)
{
	int cursz = op->sz;

	if (sz <= cursz) {
		sx_clrpkmem(op->bytes, cursz - sz);
		sx_rdpkmem(op->bytes + cursz - sz, mem, sz);
	} else {
		sx_rdpkmem(op->bytes, mem + sz - cursz, cursz);
	}
}

void sx_pk_mem2op(const char *mem, int sz, sx_op *op)
{
	sx_pk_mem2op_be(mem, sz, op);
}

void sx_pk_mem2ecop(const char *mem, int sz, sx_ecop *op)
{
	/* for now sx_op and sx_ecop have the same type in sxbuf */
	sx_pk_mem2op(mem, sz, op);
}

void sx_pk_mem2affpt(const char *mem_x, const char *mem_y, int sz, sx_pk_affine_point *op)
{
	sx_pk_mem2ecop(mem_x, sz, &op->x);
	sx_pk_mem2ecop(mem_y, sz, &op->y);
}

void sx_pk_affpt2mem(const sx_pk_affine_point *op, char *mem_x, char *mem_y, int sz)
{
	sx_pk_ecop2mem(&op->x, mem_x, sz);
	sx_pk_ecop2mem(&op->y, mem_y, sz);
}
