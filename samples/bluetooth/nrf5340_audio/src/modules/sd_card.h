/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _SD_CARD_H_
#define _SD_CARD_H_

#include <stddef.h>

/**@brief Print out the contents under SD card root path
 *
 * @param[in] path	Path of the folder which going to list
 *			If assigned path is null, then listing the contents under root
 *			If assigned path doesn't exist, an error will be returned
 *
 * @return	0 on success.
 *              -ENODEV SD init failed. SD card likely not inserted
 *		Otherwise, error from underlying drivers
 */
int sd_card_list_files(char *path);

/**@brief Write data from buffer into the file
 *
 * @note If the file already exists, data will be appended to the end of the file.
 *
 * @param[in] filename	Name of the target file for writing, the default location is the
 *			root directoy of SD card, accept absolute path under root of SD card
 * @param[in] data	Data which going to be written into the file
 * @param[in,out] size	Pointer to the number of bytes which is going to be written
 *			The actual written size will be returned
 *
 * @return	0 on success.
 *              -ENODEV SD init failed. SD card likely not inserted
 *		Otherwise, error from underlying drivers
 */
int sd_card_write(char const *const filename, char const *const data, size_t *size);

/**@brief Read data from file into the buffer
 *
 * @param[in] filename	Name of the target file for reading, the default location is the
 *			root directoy of SD card, accept absolute path under root of SD card
 * @param[in] data	The buffer which will be filled by read file contents
 * @param[in,out] size	Pointer to the number of bytes which wait to be read from the file
 *			The actual read size will be returned
 *			If the actual read size is 0, there will be a warning message which
 *			indicates the file is empty
 * @return	0 on success.
 *              -ENODEV SD init failed. SD card likely not inserted
 *		Otherwise, error from underlying drivers
 */
int sd_card_read(char const *const filename, char *const data, size_t *size);

/**@brief  Initialize the SD card interface and print out SD card details.
 *
 * @retval	0 on success
 *              -ENODEV SD init failed. SD card likely not inserted
 *		Otherwise, error from underlying drivers
 */
int sd_card_init(void);

#endif /* _SD_CARD_H_ */
