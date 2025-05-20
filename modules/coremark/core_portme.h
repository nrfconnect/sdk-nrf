/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Code is based on template: https://github.com/eembc/coremark/blob/main/simple/core_portme.h
 */

#ifndef CORE_PORTME_H
#define CORE_PORTME_H

#include <stdint.h>
#include <stdlib.h>

#include <zephyr/kernel.h>

/* Basic CoreMark configuration */

/* Configuration : HAS_FLOAT
 *Define to 1 if the platform supports floating point.
 */
#ifndef HAS_FLOAT
  #define HAS_FLOAT 1
#endif

/* Configuration : HAS_TIME_H
 * Define to 1 if platform has the time.h header file,
 * and implementation of functions thereof.
 */
#ifndef HAS_TIME_H
  #define HAS_TIME_H 0
#endif

/* Configuration : USE_CLOCK
 * Define to 1 if platform has the time.h header file,
 * and implementation of functions thereof.
 */
#ifndef USE_CLOCK
  #define USE_CLOCK 0
#endif

/* Configuration : HAS_STDIO
 * Define to 1 if the platform has stdio.h.
 */
#ifndef HAS_STDIO
  #define HAS_STDIO 0
#endif

/* Configuration : HAS_PRINTF
 * Define to 1 if the platform has stdio.h and implements the printf function.
 */
#ifndef HAS_PRINTF
  #define HAS_PRINTF 0
#endif

/* Configuration : MAIN_HAS_NOARGC
 * Needed if platform does not support getting arguments to main.
 *
 * Valid values :
 * 0 - argc/argv to main is supported
 * 1 - argc/argv to main is not supported
 *
 * Note :
 * This flag only matters if MULTITHREAD has been defined to a value greater then 1.
 */
#ifndef MAIN_HAS_NOARGC
  #define MAIN_HAS_NOARGC 1
#endif

/* Configuration : MAIN_HAS_NORETURN
 * Needed if platform does not support returning a value from main.
 *
 * Valid values :
 * 0 - main returns an int, and return value will be 0.
 * 1 - platform does not support returning a value from main
 */
#ifndef MAIN_HAS_NORETURN
  #define MAIN_HAS_NORETURN 1
#endif

/* Configuration : SEED_METHOD
 * Defines method to get seed values that cannot be computed at compile time.
 *
 * Valid values :
 * SEED_ARG - from command line.
 * SEED_FUNC - from a system function.
 * SEED_VOLATILE - from volatile variables.
 */
#ifndef SEED_METHOD
  #define SEED_METHOD SEED_VOLATILE
#endif

/* Definitions : COMPILER_VERSION, COMPILER_FLAGS, MEM_LOCATION
 * Initialize these strings per platform
 */
#ifndef COMPILER_VERSION
  #ifdef __GNUC__
    #define COMPILER_VERSION "GCC"__VERSION__
  #else
    #define COMPILER_VERSION "Please put compiler version here (e.g. gcc 4.1)"
  #endif
#endif

#ifndef COMPILER_FLAGS
  #define COMPILER_FLAGS CONFIG_COMPILER_OPT " + see compiler flags added by Zephyr"
#endif

/* Configuration : MEM_METHOD
 * Defines method to get a block of memory.
 * Valid values :
 * MEM_MALLOC - for platforms that implement malloc and have malloc.h.
 * MEM_STATIC - to use a static memory array.
 * MEM_STACK - to allocate the data block on the stack.
 */
#ifndef MEM_METHOD
  #ifdef CONFIG_COREMARK_MEMORY_METHOD_STACK
    #define MEM_METHOD    MEM_STACK
    #define MEM_LOCATION  "STACK"
  #elif defined(CONFIG_COREMARK_MEMORY_METHOD_STATIC)
    #define MEM_METHOD    MEM_STATIC
    #define MEM_LOCATION  "STATIC"
  #elif defined(CONFIG_COREMARK_MEMORY_METHOD_MALLOC)
    #define MEM_METHOD    MEM_MALLOC
    #define MEM_LOCATION  "MALLOC"
  #endif
#endif

/* Configuration : MULTITHREAD
 * Define for parallel execution
 *
 * Valid values :
 * 1 - only one context (default).
 * N>1 - will execute N copies in parallel.
 *
 * Note :
 * If this flag is defined to more then 1, an implementation for launching
 * parallel contexts must be defined.
 *
 * Two sample implementations are provided. Use <USE_PTHREAD> or <USE_FORK>
 * to enable them.
 *
 * It is valid to have a different implementation of <core_start_parallel>
 * and <core_end_parallel> in <core_portme.c>, to fit a particular architecture.
 */
#ifndef MULTITHREAD
  #define MULTITHREAD CONFIG_COREMARK_THREADS_NUMBER
#endif

#if (MULTITHREAD > 1)
  #define PARALLEL_METHOD "Zephyr Threads"
#endif

/* Depending on the benchmark run type different data size is required.
 * Default values for TOTAL_DATA_SIZE is predefined in coremark.h
 * Redefine it here and get value according to CoreMark configuration.
 */
#undef TOTAL_DATA_SIZE
#define TOTAL_DATA_SIZE  CONFIG_COREMARK_DATA_SIZE

/* CoreMark's entry point, called 'main' by default.
 * As CoreMark sources are read-only this conflict must be solved in other way.
 * You need to rename the CoreMark main function.
 */
#define main       coremark_run

/* The crc16 function is present both in Zephyr and CoreMark.
 * To avoid multiple definition error without changing benchmark's code,
 * CoreMark crc16 instance is renamed.
 */
#define crc16      coremark_crc16

/* This function will be used to output benchmark results */
#define ee_printf  printk

/* Functions used by CoreMark for MEM_METHOD MALLOC case */
#define portable_malloc  k_malloc
#define portable_free    k_free

/* CoreMark-specific data type definition.
 * ee_ptr_int must be the data type used for holding pointers.
 */
typedef signed short	ee_s16;
typedef unsigned short	ee_u16;
typedef signed int	ee_s32;
typedef double		ee_f32;
typedef unsigned char	ee_u8;
typedef unsigned int	ee_u32;
typedef ee_u32		ee_ptr_int;
typedef size_t		ee_size_t;
typedef uint32_t	CORE_TICKS;

/* The align_mem macro is used to align an offset to point to a 32 b value. It is
 * used in the matrix algorithm to initialize the input memory blocks.
 */
#define align_mem(x) (void *)(4 + (((ee_ptr_int)(x) - 1) & ~3))

typedef struct CORE_PORTABLE_S {
	ee_u8 portable_id;
} core_portable;

/* Variable : default_num_contexts
 * Not used for this simple port, must contain the value 1.
 */
extern ee_u32 default_num_contexts;

/* target specific init/fini */
void portable_init(core_portable *p, int *argc, char *argv[]);
void portable_fini(core_portable *p);

#endif /* CORE_PORTME_H */
