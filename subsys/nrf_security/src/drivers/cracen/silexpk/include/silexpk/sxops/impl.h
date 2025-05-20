/** "sxops" interface to finalise acceleration requests
 * and get the output operands
 *
 * @file
 *
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SXOPS_IMPL_HEADER_FILE
#define SXOPS_IMPL_HEADER_FILE

#include <silexpk/version.h>

/** Make sure the application is compatible with SilexPK API version **/
SX_PK_API_ASSERT_SRC_COMPATIBLE(2, 0, sxopsimpl);

/** Finish an operation with one result operands
 *
 * Write the single result in the result operand
 * and release the reserved resources.
 *
 * @pre The operation on the accelerator must be finished before
 * calling this function.
 *
 * @param[in] pkhw The acceleration request where an
 * operation with 1 output operands has finished
 * @param[out] result Result operand of a single output operand operation
 */
static inline void sx_async_finish_single(sx_pk_req *pkhw, sx_op *result)
{
	const char **outputs = sx_pk_get_output_ops(pkhw);
	const int opsz = sx_pk_get_opsize(pkhw);

	sx_pk_mem2op(outputs[0], opsz, result);

	sx_pk_release_req(pkhw);
}

/** Finish an operation with one result EC operands
 *
 * Write the single result in the result operand
 * and release the reserved resources.
 *
 * @pre The operation on the accelerator must be finished before
 * calling this function.
 *
 * @param[in] pkhw The acceleration request where an
 * operation with 1 output operands has finished
 * @param[out] result Result EC operand of a single output operand operation
 */
static inline void sx_async_finish_single_ec(sx_pk_req *pkhw, sx_ecop *result)
{
	const char **outputs = sx_pk_get_output_ops(pkhw);
	const int opsz = sx_pk_get_opsize(pkhw);

	sx_pk_mem2ecop(outputs[0], opsz, result);

	sx_pk_release_req(pkhw);
}

/** Finish an operation with two result operands
 *
 * Write the two results in the result operand
 * and release the reserved resources.
 *
 * @pre The operation on the accelerator must be finished before
 * calling this function.
 *
 * @param[in] pkhw The acceleration request where the
 * operation with 2 output operands has finished
 * @param[out] r1 First result operand of the operation
 * @param[out] r2 Second result operand of the operation
 */
static inline void sx_async_finish_pair(sx_pk_req *pkhw, sx_op *r1, sx_op *r2)
{
	const char **outputs = sx_pk_get_output_ops(pkhw);
	const int opsz = sx_pk_get_opsize(pkhw);

	sx_pk_mem2op(outputs[0], opsz, r1);
	sx_pk_mem2op(outputs[1], opsz, r2);

	sx_pk_release_req(pkhw);
}

/** Finish an operation with two result EC operands
 *
 * Write the two results in the result operand
 * and release the reserved resources.
 *
 * @pre The operation on the accelerator must be finished before
 * calling this function.
 *
 * @param[in] pkhw The acceleration request where the
 * operation with 2 output operands has finished
 * @param[out] r1 First result EC operand of the operation
 * @param[out] r2 Second result EC operand of the operation
 */
static inline void sx_async_finish_ec_pair(sx_pk_req *pkhw, sx_ecop *r1, sx_ecop *r2)
{
	const char **outputs = sx_pk_get_output_ops(pkhw);
	const int opsz = sx_pk_get_opsize(pkhw);

	sx_pk_mem2ecop(outputs[0], opsz, r1);
	sx_pk_mem2ecop(outputs[1], opsz, r2);

	sx_pk_release_req(pkhw);
}

/** Finish an operation with a affine point
 *
 * Write the two results in the affine point
 * result operand and release the reserved resources.
 *
 * @pre The operation on the accelerator must be finished before
 * calling this function.
 *
 * @param[in] pkhw The acceleration request where the
 * operation with 2 output operands has finished
 * @param[out] r Resulting affine point
 */
static inline void sx_async_finish_affine_point(sx_pk_req *pkhw, sx_pk_affine_point *r)
{
	const char **outputs = sx_pk_get_output_ops(pkhw);
	const int opsz = sx_pk_get_opsize(pkhw);

	sx_pk_mem2affpt(outputs[0], outputs[1], opsz, r);

	sx_pk_release_req(pkhw);
}

/** Finish an operation with four result operands
 *
 * Write the four results in the result operand buffers
 * and release the reserved resources.
 *
 * @pre The operation on the accelerator must be finished before
 * calling this function.
 *
 * @param[in] pkhw The acceleration request where the
 * operation with 4 resulting operands has finished
 * @param[out] r1 First result operand of the operation
 * @param[out] r2 Second result operand of the operation
 * @param[out] r3 Third result operand of the operation
 * @param[out] r4 Fourth result operand of the operation
 */
static inline void sx_async_finish_quad(sx_pk_req *pkhw, sx_op *r1, sx_op *r2, sx_op *r3, sx_op *r4)
{
	const char **outputs = sx_pk_get_output_ops(pkhw);
	const int opsz = sx_pk_get_opsize(pkhw);

	sx_pk_mem2op(outputs[0], opsz, r1);
	sx_pk_mem2op(outputs[1], opsz, r2);
	sx_pk_mem2op(outputs[2], opsz, r3);
	sx_pk_mem2op(outputs[3], opsz, r4);

	sx_pk_release_req(pkhw);
}

/** Finish an operation with any number of result operands
 *
 * Write results in the result operand buffer
 * and release the reserved resources.
 *
 * @pre The operation on the accelerator must be finished before
 * calling this function.
 *
 * @param[in] pkhw The acceleration request where the
 * operation with 'count' resulting operands has finished
 * @param[out] results Buffer of result operands of the operation
 * @param[in] count The number of result operands
 */
static inline void sx_async_finish_any(sx_pk_req *pkhw, sx_op **results, int count)
{
	const char **outputs = sx_pk_get_output_ops(pkhw);
	const int opsz = sx_pk_get_opsize(pkhw);

	for (int i = 0; i < count; i++) {
		sx_pk_mem2op(outputs[i], opsz, results[i]);
	}
	sx_pk_release_req(pkhw);
}
#endif
