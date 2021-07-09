/****************************************************************************
 *
 * File:          int.c
 * Author(s):     Georgi Valkov (GV)
 * Project:       nano RTOS
 * Description:   interrupt services
 *
 * Copyright (C) 2021 Georgi Valkov
 *                      Sedma str. 23, 4491 Zlokuchene, Bulgaria
 *                      e-mail: gvalkov@abv.bg
 *
 ****************************************************************************/

/* Include files ************************************************************/

#include <int.h>


/* Macros *******************************************************************/

/* Types ********************************************************************/

/* Local Prototypes *********************************************************/

/* Global Prototypes ********************************************************/

/* Function prototypes ******************************************************/

#define NUM_SOURCES                  63
#define NVIC_CLRENA_BASE             0xE000E180
#define NVIC_SETENA_BASE             0xE000E100
#define NVIC_INTPRIORITY_BASE        0xE000E400


int int_source_disable(uint16_t int_num)
{
	if (int_num <= 15)
	{
		// 0-15 system vectors
		return OK;
	}

	if (int_num >= NUM_SOURCES)
	{
		errno = EINVAL;
		return FAIL;
	}

	int_num -= 16;

	int index = (uint32_t)int_num >> 5;
	volatile uint32_t * nvic_icer = (volatile uint32_t *)NVIC_CLRENA_BASE;
	nvic_icer[index] = (uint32_t)(1 << ((uint32_t)int_num & 0x1f));

	return OK;
}

int int_source_enable(uint16_t int_num)
{
	if (int_num <= 15)
	{
		// 0-15 system vectors
		return OK;
	}

	if (int_num >= NUM_SOURCES)
	{
		errno = EINVAL;
		return FAIL;
	}

	int_num -= 16;

	int index = (((uint32_t)int_num) >> 5);
	volatile uint32_t * nvic_iser = (volatile uint32_t *)NVIC_SETENA_BASE;
	nvic_iser[index] = (uint32_t)(1 << (((uint32_t)int_num) & 0x1f));

	return OK;
}

int int_source_configure(uint16_t int_num, uint16_t prio, uint16_t mode)
{
	if (int_num <= 15)
	{
		// 0-15 system vectors
		return OK;
	}

	if (int_num >= NUM_SOURCES)
	{
		errno = EINVAL;
		return FAIL;
	}

	int_num -= 16;

	*((uint8_t *)(NVIC_INTPRIORITY_BASE + int_num)) = (prio << 4) & 0xe0;

	return OK;
}

struct isr_t
{
	uint32_t param;
	uint32_t func;
};

int int_set_vector(uint16_t int_num, fn_int_handler int_handler)
{
	if (int_num >= NUM_SOURCES)
	{
		errno = EINVAL;
		return FAIL;
	}

	int_num -= 16;

	struct isr_t * isr = (struct isr_t *)&_sw_isr_table[int_num];
	isr->param = 0;
	isr->func = (uint32_t)int_handler | 1;

	return OK;
}

fn_int_handler int_get_vector(uint16_t int_num)
{
	if (int_num >= NUM_SOURCES)
	{
		errno = EINVAL;
		return NULL;
	}

	int_num -= 16;

	struct isr_t * isr = (struct isr_t *)&_sw_isr_table[int_num];
	return (fn_int_handler)(isr->func & ~1);
}


/* Global variables *********************************************************/

/* End of file **************************************************************/
