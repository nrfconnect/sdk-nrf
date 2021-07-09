#ifndef _INT_H
#define _INT_H

/****************************************************************************
 *
 * File:          int.h
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

#include <os_layer.h>


/* Macro definitions ********************************************************/

/* Types ********************************************************************/

_BEGIN_CPLUSPLUS

typedef uint32_t	int_state_t;

typedef void	(* fn_int_handler)(void);


/* Global variables *********************************************************/

/* Function prototypes ******************************************************/

// low level interrupt services
void		(int_disable)(void);
void		(int_enable)(void);

int_state_t	(int_disable_nested)(void);
void		(int_enable_nested)(int_state_t int_state_old);

int_state_t(int_get_faultmask_or_primask)(void);
int_state_t(int_get_faultmask)(void);
int_state_t(int_get_primask)(void);
#define int_is_disabled int_get_primask

// interrupt services
int			int_source_disable(uint16_t int_num);
int			int_source_enable(uint16_t int_num);
int			int_source_configure(uint16_t int_num, uint16_t prio, uint16_t mode);

int			int_set_vector(uint16_t int_num, fn_int_handler int_handler);
fn_int_handler	int_get_vector(uint16_t int_num);

_END_CPLUSPLUS
#endif /* #ifndef _INT_H */
