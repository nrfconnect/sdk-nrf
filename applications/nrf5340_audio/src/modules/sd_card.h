/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _SD_CARD_H_
#define _SD_CARD_H_

#include <stddef.h>
#include <zephyr/fs/fs.h>

/**
 * @brief	Print out the contents under SD card root path and write the content to buffer.
 *
 * @param[in]		path		Path of the folder which is going to be listed.
 *					If assigned path is null, then listing the contents under
 *					root. If assigned path doesn't exist, an error will be
 *					returned.
 * @param[out]		buf		Buffer where data is written. If set to NULL, it will be
 *					ignored.
 * @param[in, out]	buf_size	Buffer size.
 *
 * @retval	0 on success.
 * @retval	-EPERM SD card operation is ongoing somewhere else.
 * @retval	-ENODEV SD init failed. SD card likely not inserted.
 * @retval	-EINVAL Failed to append to buffer.
 * @retval	-FR_INVALID_NAME Path is too long.
 * @retval	Otherwise, error from underlying drivers.
 */
int sd_card_list_files(char const *const path, char *buf, size_t *buf_size);

/**
 * @brief	Write data from buffer into the file.
 *
 * @note	If the file already exists, data will be appended to the end of the file.
 *
 * @param[in]		filename	Name of the target file for writing, the default
 *					location is the root directoy of SD card, accept
					absolute path under root of SD card.
 * @param[in]		data		which is going to be written into the file.
 * @param[in, out]	size		Pointer to the number of bytes which is going to be written.
 *					The actual written size will be returned.
 *
 * @retval	0 on success.
 * @retval	-EPERM SD card operation is ongoing somewhere else.
 * @retval	-ENODEV SD init failed. SD card likely not inserted.
 * @retval	Otherwise, error from underlying drivers.
 */
int sd_card_open_write_close(char const *const filename, char const *const data, size_t *size);

/**
 * @brief	Read data from file into the buffer.
 *
 * @param[in]		filename	Name of the target file for reading, the default location is
 *					the root directoy of SD card, accept absolute path under
 *					root of SD card.
 * @param[out]		buf		Pointer to the buffer to write the read data into.
 * @param[in, out]	size		Pointer to the number of bytes which wait to be read from
 *					the file. The actual read size will be returned. If the
 *					actual read size is 0, there will be a warning message which
					indicates that the file is empty.
 *
 * @retval	0 on success.
 * @retval	-EPERM SD card operation is ongoing somewhere else.
 * @retval	-ENODEV SD init failed. SD card likely not inserted.
 * @retval	Otherwise, error from underlying drivers.
 */
int sd_card_open_read_close(char const *const filename, char *const buf, size_t *size);

/**
 * @brief	Open file on SD card.
 *
 * @param[in]		filename		Name of file to open. Default
 *						location is the root directoy of SD card.
 *						Absolute path under root of SD card is accepted.
 * @param[in, out]	f_seg_read_entry	Pointer to a file object.
 *						The pointer gets initialized and ready for use.
 *
 *
 * @retval	0 on success.
 * @retval	-EPERM SD card operation is ongoing somewhere else.
 * @retval	-ENODEV SD init failed. SD likely not inserted.
 * @retval	Otherwise, error from underlying drivers.
 */
int sd_card_open(char const *const filename, struct fs_file_t *f_seg_read_entry);

/**
 * @brief	Read segment on the open file on the SD card.
 *
 * @param[out]		buf			Pointer to the buffer to write the read data into.
 * @param[in, out]	size			Number of bytes to be read from file.
 *						The actual read size will be returned.
 *						If the actual read size is 0, there will be a
 *						warning message which indicates that the file is
 *						empty.
 * @param[in, out]	f_seg_read_entry	Pointer to a file object. After call to this
 *						function, the pointer gets updated and can be used
 *						as entry in next function call.
 *
 * @retval	0 on success.
 * @retval	-EPERM SD card operation is not ongoing.
 * @retval	-ENODEV SD init failed. SD likely not inserted.
 * @retval	Otherwise, error from underlying drivers.
 */
int sd_card_read(char *buf, size_t *size, struct fs_file_t *f_seg_read_entry);

/**
 * @brief	Close the file opened by the sd_card_segment_read_open function.
 *
 * @param[in, out]	f_seg_read_entry	Pointer to a file object. After call to this
 *						function, the pointer is reset and can be used for
 *						another file.
 *
 *
 * @retval	0 on success.
 * @retval	-EPERM SD card operation is not ongoing.
 * @retval	-EBUSY Segment read operation has not started.
 * @retval	Otherwise, error from underlying drivers.
 */
int sd_card_close(struct fs_file_t *f_seg_read_entry);

/**
 * @brief	Initialize the SD card interface and print out SD card details.
 *
 * @retval	0 on success.
 * @retval	-ENODEV SD init failed. SD card likely not inserted.
 * @retval	Otherwise, error from underlying drivers.
 */
int sd_card_init(void);

#endif /* _SD_CARD_H_ */
