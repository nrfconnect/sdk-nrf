/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef LC3_FILE_DATA_H__
#define LC3_FILE_DATA_H__

#include <zephyr/types.h>

/*******************
 * VALID DATASET 1 *
 *******************/
extern const uint8_t lc3_file_dataset1_valid[];
extern const size_t lc3_file_dataset1_valid_size;

extern const uint8_t lc3_file_dataset1_valid_frame1[];
extern const size_t lc3_file_dataset1_valid_frame1_size;

extern const uint8_t lc3_file_dataset1_valid_frame2[];
extern const size_t lc3_file_dataset1_valid_frame2_size;

extern const uint8_t lc3_file_dataset1_valid_frame3[];
extern const size_t lc3_file_dataset1_valid_frame3_size;

extern const uint8_t lc3_file_dataset1_valid_frame4[];
extern const size_t lc3_file_dataset1_valid_frame4_size;

extern const uint8_t lc3_file_dataset1_valid_frame5[];
extern const size_t lc3_file_dataset1_valid_frame5_size;

/*************************
 * INVALID FRAME DATASET *
 *************************/
/* LC3 dataset where the header is valid, but the frame contains less data than the frame header
 * specifies
 */
extern const uint8_t lc3_file_dataset_invalid_frame[];
extern const size_t lc3_file_dataset_invalid_frame_size;

/**************************
 * INVALID HEADER DATASET *
 **************************/
/* LC3 dataset header where the header ID is invalid */
extern const uint8_t lc3_file_dataset_invalid_header[];
extern const size_t lc3_file_dataset_invalid_header_size;

#endif /* LC3_FILE_DATA_H__ */
