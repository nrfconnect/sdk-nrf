/****************************************************************************
 *
 * File:          os_zephyr.c
 * Author(s):     Georgi Valkov (GV)
 * Project:       compatibility layer for Zephyr
 * Description:   compatibility layer for Zephyr
 *
 * Copyright (C) 2021 Georgi Valkov
 *                      Sedma str. 23, 4491 Zlokuchene, Bulgaria
 *                      e-mail: gvalkov@abv.bg
 *
 ****************************************************************************/

/* Include files ************************************************************/

#include <kernel.h>
#include <os_zephyr.h>


/* Macros *******************************************************************/

/* Types ********************************************************************/

/* Local Prototypes *********************************************************/

/* Global Prototypes ********************************************************/

/* Function prototypes ******************************************************/

// returns task ID if running as a task
// returns FAIL if the current context is not a valid task
// or systems calls are not allowed
inline int task_id_get(void)
{
	// https://docs.zephyrproject.org/latest/reference/kernel/threads/index.html
	return (int)k_current_get();
}

inline int task_wait(void)
{
	return k_sleep(K_FOREVER);
}

int task_wake(int task_id)
{
	int b = k_thread_timeout_remaining_ticks((k_tid_t)task_id);
	k_wakeup((k_tid_t)task_id);

	return b ? OK : FAIL;
}


/* Global variables *********************************************************/

/* End of file **************************************************************/
