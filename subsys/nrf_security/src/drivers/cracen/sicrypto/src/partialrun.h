/*
 *  Copyright (c) 2023 Nordic Semiconductor ASA
 *
 *  SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef PARTIALRUN_HEADER_FILE
#define PARTIALRUN_HEADER_FILE

/* possible values for the partialrun member of a struct sitask
 * - partialrun in struct sitask represents the state of partial HW operations
 * - a partial HW operation is an intermediate operation that is run on a HW engine
 * when using context-saving
 * - the final HW operation of a context-saving computation is not considered a
 * partial operation
 * - partialrun works like this: it is initialized to PARTIALRUN_NONE, a partial
 * run function sets it to PARTIALRUN_INPROGRESS, status and wait functions set it
 * to PARTIALRUN_FINISHED as soon as they detect that the HW engine has finished
 * the partial operation, finally a consume function sets it back to
 * PARTIALRUN_NONE
 */
#define PARTIALRUN_NONE	      0
#define PARTIALRUN_INPROGRESS 1
#define PARTIALRUN_FINISHED   2

#endif
