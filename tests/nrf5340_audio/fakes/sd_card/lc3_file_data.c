/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/sys/util.h>

#include "lc3_file_data.h"

/*******************
 * VALID DATASET 1 *
 *******************/
const uint8_t lc3_file_dataset1_valid[] = {
	0x1c, 0xcc, 0x12, 0x00, 0xe0, 0x01, 0x40, 0x01, 0x01, 0x00, 0xe8, 0x03, 0x00, 0x00, 0x80,
	0x07, 0x00, 0x00, 0x28, 0x00, 0x50, 0x46, 0x07, 0x52, 0x03, 0x2c, 0x8f, 0xe1, 0xe9, 0x21,
	0x73, 0x81, 0x25, 0x82, 0x9f, 0x94, 0x7a, 0xa4, 0xa3, 0xa2, 0xc9, 0x50, 0xa2, 0xc3, 0xeb,
	0x43, 0xe0, 0xff, 0xf0, 0x14, 0x20, 0x86, 0xdd, 0x2f, 0xda, 0x48, 0xe2, 0x39, 0x92, 0xec,
	0x28, 0x00, 0x3f, 0x48, 0x6d, 0xc7, 0x7f, 0xae, 0x83, 0xad, 0x95, 0xd7, 0x66, 0xa1, 0x9b,
	0xcf, 0x3e, 0xdb, 0x1b, 0x41, 0xb4, 0xc8, 0x80, 0x6d, 0xf9, 0x13, 0x7b, 0x43, 0x36, 0x2f,
	0xc6, 0xe1, 0xaa, 0x11, 0xd2, 0xce, 0xe9, 0x01, 0x62, 0x0b, 0x94, 0xfc, 0x28, 0x00, 0x82,
	0x8a, 0x0f, 0x07, 0x74, 0x19, 0x24, 0x41, 0xbc, 0xcd, 0x38, 0x73, 0xba, 0x0a, 0x51, 0xde,
	0x2c, 0x9c, 0x77, 0x7b, 0xc9, 0x03, 0x7a, 0xee, 0x29, 0x1b, 0x0a, 0x02, 0xc7, 0x03, 0x7c,
	0xd6, 0xd0, 0xb6, 0xbb, 0x2d, 0x63, 0xbe, 0xb4, 0x54, 0x28, 0x00, 0x1e, 0xc2, 0x47, 0x7e,
	0xc7, 0x1a, 0xf8, 0x46, 0xde, 0x60, 0xbb, 0xbf, 0x7e, 0x67, 0x94, 0xbd, 0x96, 0x44, 0x36,
	0xbc, 0xcf, 0x59, 0x3f, 0xa0, 0xfd, 0x8f, 0x2c, 0x57, 0x89, 0x2a, 0xb3, 0x78, 0xdd, 0xe0,
	0xdc, 0xbb, 0x65, 0x0f, 0x54, 0x5c, 0x28, 0x00, 0xff, 0xed, 0x1b, 0x0e, 0x0b, 0x34, 0xdc,
	0xad, 0xe1, 0x34, 0xad, 0xba, 0xb9, 0xfc, 0x85, 0x72, 0x60, 0x9c, 0xac, 0x00, 0x00, 0x03,
	0xf2, 0x14, 0xc2, 0xf6, 0xc7, 0x5a, 0xd3, 0xd8, 0x94, 0x10, 0xce, 0x1c, 0xb8, 0x32, 0xb1,
	0xbd, 0xa3, 0x3c};
const size_t lc3_file_dataset1_valid_size = ARRAY_SIZE(lc3_file_dataset1_valid);

const uint8_t lc3_file_dataset1_valid_frame1[] = {
	0x50, 0x46, 0x07, 0x52, 0x03, 0x2c, 0x8f, 0xe1, 0xe9, 0x21, 0x73, 0x81, 0x25, 0x82,
	0x9f, 0x94, 0x7a, 0xa4, 0xa3, 0xa2, 0xc9, 0x50, 0xa2, 0xc3, 0xeb, 0x43, 0xe0, 0xff,
	0xf0, 0x14, 0x20, 0x86, 0xdd, 0x2f, 0xda, 0x48, 0xe2, 0x39, 0x92, 0xec};
const size_t lc3_file_dataset1_valid_frame1_size = ARRAY_SIZE(lc3_file_dataset1_valid_frame1);

const uint8_t lc3_file_dataset1_valid_frame2[] = {
	0x3f, 0x48, 0x6d, 0xc7, 0x7f, 0xae, 0x83, 0xad, 0x95, 0xd7, 0x66, 0xa1, 0x9b, 0xcf,
	0x3e, 0xdb, 0x1b, 0x41, 0xb4, 0xc8, 0x80, 0x6d, 0xf9, 0x13, 0x7b, 0x43, 0x36, 0x2f,
	0xc6, 0xe1, 0xaa, 0x11, 0xd2, 0xce, 0xe9, 0x01, 0x62, 0x0b, 0x94, 0xfc};
const size_t lc3_file_dataset1_valid_frame2_size = ARRAY_SIZE(lc3_file_dataset1_valid_frame2);

const uint8_t lc3_file_dataset1_valid_frame3[] = {
	0x82, 0x8a, 0x0f, 0x07, 0x74, 0x19, 0x24, 0x41, 0xbc, 0xcd, 0x38, 0x73, 0xba, 0x0a,
	0x51, 0xde, 0x2c, 0x9c, 0x77, 0x7b, 0xc9, 0x03, 0x7a, 0xee, 0x29, 0x1b, 0x0a, 0x02,
	0xc7, 0x03, 0x7c, 0xd6, 0xd0, 0xb6, 0xbb, 0x2d, 0x63, 0xbe, 0xb4, 0x54};
const size_t lc3_file_dataset1_valid_frame3_size = ARRAY_SIZE(lc3_file_dataset1_valid_frame3);

const uint8_t lc3_file_dataset1_valid_frame4[] = {
	0x1e, 0xc2, 0x47, 0x7e, 0xc7, 0x1a, 0xf8, 0x46, 0xde, 0x60, 0xbb, 0xbf, 0x7e, 0x67,
	0x94, 0xbd, 0x96, 0x44, 0x36, 0xbc, 0xcf, 0x59, 0x3f, 0xa0, 0xfd, 0x8f, 0x2c, 0x57,
	0x89, 0x2a, 0xb3, 0x78, 0xdd, 0xe0, 0xdc, 0xbb, 0x65, 0x0f, 0x54, 0x5c};
const size_t lc3_file_dataset1_valid_frame4_size = ARRAY_SIZE(lc3_file_dataset1_valid_frame4);

const uint8_t lc3_file_dataset1_valid_frame5[] = {
	0xff, 0xed, 0x1b, 0x0e, 0x0b, 0x34, 0xdc, 0xad, 0xe1, 0x34, 0xad, 0xba, 0xb9, 0xfc,
	0x85, 0x72, 0x60, 0x9c, 0xac, 0x00, 0x00, 0x03, 0xf2, 0x14, 0xc2, 0xf6, 0xc7, 0x5a,
	0xd3, 0xd8, 0x94, 0x10, 0xce, 0x1c, 0xb8, 0x32, 0xb1, 0xbd, 0xa3, 0x3c};
const size_t lc3_file_dataset1_valid_frame5_size = ARRAY_SIZE(lc3_file_dataset1_valid_frame5);

/*************************
 * INVALID FRAME DATASET *
 *************************/
const uint8_t lc3_file_dataset_invalid_frame[] = {
	0x1c, 0xcc, 0x12, 0x00, 0xe0, 0x01, 0x40, 0x01, 0x01, 0x00, 0xe8, 0x03,
	0x00, 0x00, 0x80, 0x07, 0x00, 0x00, 0x28, 0x00, 0x50, 0x46, 0x07, 0x52,
	0x03, 0x2c, 0x8f, 0xe1, 0xe9, 0x21, 0x73, 0x81, 0x25, 0x82, 0x9f, 0x94,
	0x7a, 0xa4, 0xa3, 0xa2, 0xc9, 0x50, 0xa2, 0xc3, 0xeb, 0x43};
const size_t lc3_file_dataset_invalid_frame_size = ARRAY_SIZE(lc3_file_dataset_invalid_frame);

/**************************
 * INVALID HEADER DATASET *
 **************************/
const uint8_t lc3_file_dataset_invalid_header[] = {
	0x1c, 0xFF, /* Invalid file id, is normally 0xcc1c */
	0x12, 0x00, /* hdr_size: 0x0012 */
	0x40, 0x01, /* sample_rate: 0x0140 */
	0xe0, 0x01, /* bit_rate: 0x01e0 */
	0x01, 0x00, /* channels: 0x0001 */
	0xe8, 0x03, /* frame_duration: 0x03e8 */
	0x00, 0x00, /* rfu: 0x0000 */
	0x6c, 0x00, /* signal_len_lsb: 0x006c */
	0x42, 0x00  /* signal_len_msb: 0x0042 */
};
const size_t lc3_file_dataset_invalid_header_size = ARRAY_SIZE(lc3_file_dataset_invalid_header);
