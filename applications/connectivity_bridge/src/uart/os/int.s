/****************************************************************************
 *
 * File:          interrupt.s
 * Author(s):     Georgi Valkov (GV)
 * Project:       nano RTOS
 * Description:   low level interrupt services
 *
 * Copyright (C) 2021 Georgi Valkov
 *                      Sedma str. 23, 4491 Zlokuchene, Bulgaria
 *                      e-mail: gvalkov@abv.bg
 *
 ****************************************************************************/

// exported function in separate section
	.macro _global_func func_name
	.text
	.code 16
	.balign 2
	.thumb_func
	.global \func_name
	.type \func_name,#function
\func_name:
	.endm


.text
.thumb
.balign 2
.syntax unified


/**********************************
 *  void int_disable(void)
 **********************************/
_global_func int_disable

	cpsid i
	bx lr

/**********************************
 *  void int_enable(void)
 **********************************/
_global_func int_enable

	cpsie i
	bx lr

/**********************************
 *  int_state_t int_disable_nested(void)
 **********************************/
_global_func int_disable_nested

	mrs r0, primask
	cpsid i
	bx lr

/**********************************
 *  void int_enable_nested(int_state_t)
 **********************************/
_global_func int_enable_nested

	msr primask, r0
	bx lr


/* returns (faultmask | primask) faults or interrupts disabled */
/**********************************
 *  int_state_t int_get_faultmask_or_primask(void)
 **********************************/
_global_func int_get_faultmask_or_primask

	mrs r0, faultmask
	mrs r1, primask
	orr r0, r1
	bx lr

/* returns faultmask which is 1 when faults are disabled */
/**********************************
 *  int_state_t int_get_faultmask(void)
 **********************************/
_global_func int_get_faultmask

	mrs r0, faultmask
	bx lr

/* returns primask which is 1 when interrupts are disabled */
/**********************************
 *  int_state_t int_get_primask(void)
 **********************************/
_global_func int_get_primask

	mrs r0, primask
	bx lr


.end
