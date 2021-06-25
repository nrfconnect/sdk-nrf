#ifndef OS_LAYER_H
#define OS_LAYER_H
/****************************************************************************
 *
 * File:          os_layer.h
 * Author(s):     Georgi Valkov (GV)
 * Project:       nano RTOS
 * Description:   OS compatibility layer
 *
 * Copyright (C) 2021 by Georgi Valkov
 *                      Sedma str. 23, 4491 Zlokuchene, Bulgaria
 *                      e-mail: gvalkov@abv.bg
 *
 ****************************************************************************/

/* Include files ************************************************************/

#if defined (EUROS) || defined (OSEK)
#include <os_euros.h>
#elif defined (__ZEPHYR__)
#include "os_zephyr.h"
#else
#error unsupported OS
#endif

#endif // OS_LAYER_H
