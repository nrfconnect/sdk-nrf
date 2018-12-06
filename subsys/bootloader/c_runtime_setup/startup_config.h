/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef BOOTLOADER_STARTUP_CONFIG_H__
#define BOOTLOADER_STARTUP_CONFIG_H__

/* Configure stack size, stack alignment and heap size with a header file
 * instead of project settings or modification of Nordic provided assembler
 * files. Modify this file as needed.
 */

/* In order to make use this file,
 * 1. For Keil uVision IDE, in the Options for Target -> Asm tab, define symbol
 * __STARTUP_CONFIG and use the additional assembler option --cpreproc in Misc
 * Control text box.  2. For GCC compiling, add extra assembly option
 * -D__STARTUP_CONFIG.
 * 3. For IAR Embedded Workbench define symbol __STARTUP_CONFIG in the
 * Assembler options and define symbol __STARTUP_CONFIG=1 in the linker
 * options.
 */

/* This file is a template and should be copied to the project directory. */

/* Define size of stack. Size must be multiple of 4. */
#define __STARTUP_CONFIG_STACK_SIZE   0x1000

/* Define alignment of stack. Alignment will be 2 to the power of
 * __STARTUP_CONFIG_STACK_ALIGNMENT. Since calling convention requires that
 * the stack is aligned to 8-bytes when a function is called, the minimum
 * __STARTUP_CONFIG_STACK_ALIGNMENT is therefore 3.
 */
#define __STARTUP_CONFIG_STACK_ALIGNMENT 3

/* Define size of heap. Size must be multiple of 4. */
#define __STARTUP_CONFIG_HEAP_SIZE   0x1000

#endif
