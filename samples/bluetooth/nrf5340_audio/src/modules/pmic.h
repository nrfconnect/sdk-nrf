/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _PMIC_H_
#define _PMIC_H_

/**@brief Function for setting the correct PMIC values.
 *
 * @retval 0 if successful.
 */
int pmic_defaults_set(void);

/**@brief Turns off the PMIC
 *
 * @note This function will power off the PMIC. Hence,
 * a return will not be expected.
 *
 * @retval 0  if successful.
 */
int pmic_pwr_off(void);

/**@brief Function for initializing the PMIC (Power Management IC) driver.
 *
 * @note This function does not change any values in the PMIC registers.
 *
 * @retval  0 if successful.
 */
int pmic_init(void);

#endif /* _PMIC_H_ */
