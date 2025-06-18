/* Copyright (c) 2023, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice, this
 *      list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *
 *   3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *      contributors may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "unity.h"

#include "vendor_radio_power_map_psemi.h"

#include "crc32.h"
#include "vendor_radio_power_map_psemi_flash_traversal.h"
#include "vendor_radio_power_map_common.h"

#include "cexception_assert.h"

#include "mock_vendor_radio_power_settings.h"


#include <stdint.h>
#include <stdio.h>

#include "vendor_radio_power_map_psemi.c"

#define PAGE_SIZE 4096

#define ATT_ENTRY(_power) \
    (flash_att_entry_t) { .outPower = (_power) }

#define CHIP_TX_POWER_ENTRY(_start_power)   \
    (flash_tx_power_entry_t)               \
    {                                       \
        .entries = {                        \
            ATT_ENTRY((_start_power)),      \
            ATT_ENTRY((_start_power) + 1),  \
            ATT_ENTRY((_start_power) + 2),  \
            ATT_ENTRY((_start_power) + 3),  \
            ATT_ENTRY((_start_power) + 4),  \
            ATT_ENTRY((_start_power) + 5),  \
            ATT_ENTRY((_start_power) + 6),  \
            ATT_ENTRY((_start_power) + 7),  \
            ATT_ENTRY((_start_power) + 8),  \
            ATT_ENTRY((_start_power) + 9),  \
            ATT_ENTRY((_start_power) + 10), \
            ATT_ENTRY((_start_power) + 11), \
            ATT_ENTRY((_start_power) + 12), \
            ATT_ENTRY((_start_power) + 13), \
            ATT_ENTRY((_start_power) + 14), \
            ATT_ENTRY((_start_power) + 15)  \
        }                                   \
    }

#define CHIP_TX_POWER_ENTRY_DECREASING(_start_power)   \
    (flash_tx_power_entry_t)               \
    {                                       \
        .entries = {                        \
            ATT_ENTRY((_start_power) + 15),      \
            ATT_ENTRY((_start_power) + 14),  \
            ATT_ENTRY((_start_power) + 13),  \
            ATT_ENTRY((_start_power) + 12),  \
            ATT_ENTRY((_start_power) + 11),  \
            ATT_ENTRY((_start_power) + 10),  \
            ATT_ENTRY((_start_power) + 9),  \
            ATT_ENTRY((_start_power) + 8),  \
            ATT_ENTRY((_start_power) + 7),  \
            ATT_ENTRY((_start_power) + 6),  \
            ATT_ENTRY((_start_power) + 5), \
            ATT_ENTRY((_start_power) + 4), \
            ATT_ENTRY((_start_power) + 3), \
            ATT_ENTRY((_start_power) + 2), \
            ATT_ENTRY((_start_power) + 1), \
            ATT_ENTRY((_start_power))  \
        }                                   \
    }

static uint8_t m_memory_page[PAGE_SIZE] __attribute__((aligned(4)));

static struct {
    uint8_t * header;
    struct {
        uint8_t * start;
        uint8_t * chip_tx_power_values;
        uint8_t * channel_entry_table;
    }tx_path_0;
    struct {
        uint8_t * start;
        uint8_t * chip_tx_power_values;
        uint8_t * channel_entry_table;
    }tx_path_1;
    uint8_t * crc32;
} m_offsets;

static void power_table_write_length(uint8_t * p_start, uint8_t * p_curr)
{
    *(uint16_t *)p_start = htons(p_curr - (p_start + sizeof(uint16_t)) + sizeof(uint32_t));
}

static void init_test_power_table(void)
{
    uint8_t * curr = m_memory_page;

    flash_power_map_header_t header = {.lenght = 0, .version = htons(2), .nChipTxPower0 = 4, .nChipTxPower1 = 3};

    memset(m_memory_page, 0xffU, sizeof(m_memory_page));

    m_offsets.header = curr;
    memcpy(curr, &header, sizeof(header));
    curr += sizeof(header);

    // tx path 0
    uint16_t power;

    m_offsets.tx_path_0.start = curr;
    m_offsets.tx_path_0.chip_tx_power_values = curr;

    power = htons_signed(-16);
    memcpy(curr, &power, sizeof(power));
    curr += sizeof(power);
    power = htons_signed(5);
    memcpy(curr, &power, sizeof(power));
    curr += sizeof(power);
    power = htons_signed(7);
    memcpy(curr, &power, sizeof(power));
    curr += sizeof(power);
    power = htons_signed(8);
    memcpy(curr, &power, sizeof(power));
    curr += sizeof(power);

    m_offsets.tx_path_0.channel_entry_table = curr;

    for (uint8_t channel_id = 0; channel_id < 16; channel_id++)
    {
        flash_tx_power_entry_t entry;

        entry = CHIP_TX_POWER_ENTRY(-16 + 5);
        memcpy(curr, &entry, sizeof(entry));
        curr += sizeof(entry);

        entry = CHIP_TX_POWER_ENTRY(5 + 5);
        memcpy(curr, &entry, sizeof(entry));
        curr += sizeof(entry);

        entry = CHIP_TX_POWER_ENTRY(7 + 5);
        memcpy(curr, &entry, sizeof(entry));
        curr += sizeof(entry);

        entry = CHIP_TX_POWER_ENTRY(8 + 5);
        memcpy(curr, &entry, sizeof(entry));
        curr += sizeof(entry);
    }

    // tx path 1
    m_offsets.tx_path_1.start = curr;
    m_offsets.tx_path_1.chip_tx_power_values = curr;

    power = htons_signed(0);
    memcpy(curr, &power, sizeof(power));
    curr += sizeof(power);
    power = htons_signed(2);
    memcpy(curr, &power, sizeof(power));
    curr += sizeof(power);
    power = htons_signed(4);
    memcpy(curr, &power, sizeof(power));
    curr += sizeof(power);

    // reserved
    curr += sizeof(power);

    m_offsets.tx_path_1.channel_entry_table = curr;

    for (uint8_t channel_id = 0; channel_id < 16; channel_id++)
    {
        flash_tx_power_entry_t entry;

        entry = CHIP_TX_POWER_ENTRY(0 + 5);
        memcpy(curr, &entry, sizeof(entry));
        curr += sizeof(entry);

        entry = CHIP_TX_POWER_ENTRY(2 + 5);
        memcpy(curr, &entry, sizeof(entry));
        curr += sizeof(entry);

        entry = CHIP_TX_POWER_ENTRY(4 + 5);
        memcpy(curr, &entry, sizeof(entry));
        curr += sizeof(entry);
    }

    power_table_write_length(m_memory_page, curr);

    uint32_t crc32 = crc32_compute(m_memory_page, (curr - m_memory_page), NULL);

    m_offsets.crc32 = curr;

    memcpy(curr, &crc32, sizeof(crc32));
    curr += sizeof(crc32);
}

static void init_test_power_table_decreasing(void)
{
    uint8_t * curr = m_memory_page;

    flash_power_map_header_t header = {.lenght = 0, .version = htons(2), .nChipTxPower0 = 2, .nChipTxPower1 = 3};

    memset(m_memory_page, 0xffU, sizeof(m_memory_page));

    m_offsets.header = curr;
    memcpy(curr, &header, sizeof(header));
    curr += sizeof(header);

    // tx path 0
    uint16_t power;

    m_offsets.tx_path_0.start = curr;
    m_offsets.tx_path_0.chip_tx_power_values = curr;

    power = htons_signed(-20);
    memcpy(curr, &power, sizeof(power));
    curr += sizeof(power);
    power = htons_signed(-4);
    memcpy(curr, &power, sizeof(power));
    curr += sizeof(power);

    m_offsets.tx_path_0.channel_entry_table = curr;

    for (uint8_t channel_id = 0; channel_id < 16; channel_id++)
    {
        flash_tx_power_entry_t entry;

        entry = CHIP_TX_POWER_ENTRY(0);
        memcpy(curr, &entry, sizeof(entry));
        curr += sizeof(entry);

        entry = CHIP_TX_POWER_ENTRY_DECREASING(0);
        memcpy(curr, &entry, sizeof(entry));
        curr += sizeof(entry);
    }

    // tx path 1
    m_offsets.tx_path_1.start = curr;
    m_offsets.tx_path_1.chip_tx_power_values = curr;

    power = htons_signed(0);
    memcpy(curr, &power, sizeof(power));
    curr += sizeof(power);
    power = htons_signed(2);
    memcpy(curr, &power, sizeof(power));
    curr += sizeof(power);
    power = htons_signed(4);
    memcpy(curr, &power, sizeof(power));
    curr += sizeof(power);

    // reserved
    curr += sizeof(power);

    m_offsets.tx_path_1.channel_entry_table = curr;

    for (uint8_t channel_id = 0; channel_id < 16; channel_id++)
    {
        flash_tx_power_entry_t entry;

        entry = CHIP_TX_POWER_ENTRY_DECREASING(0 + 5);
        memcpy(curr, &entry, sizeof(entry));
        curr += sizeof(entry);

        entry = CHIP_TX_POWER_ENTRY_DECREASING(2 + 5);
        memcpy(curr, &entry, sizeof(entry));
        curr += sizeof(entry);

        entry = CHIP_TX_POWER_ENTRY_DECREASING(4 + 5);
        memcpy(curr, &entry, sizeof(entry));
        curr += sizeof(entry);
    }

    power_table_write_length(m_memory_page, curr);

    uint32_t crc32 = crc32_compute(m_memory_page, (curr - m_memory_page), NULL);

    m_offsets.crc32 = curr;

    memcpy(curr, &crc32, sizeof(crc32));
    curr += sizeof(crc32);
}

static void init_test_power_table_with_one_path(void)
{
    uint8_t * curr = m_memory_page;

    flash_power_map_header_t header = {.lenght = 0, .version = htons(2), .nChipTxPower0 = 4, .nChipTxPower1 = 0};

    memset(m_memory_page, 0xffU, sizeof(m_memory_page));

    m_offsets.header = curr;
    memcpy(curr, &header, sizeof(header));
    curr += sizeof(header);

    // tx path 0
    uint16_t power;

    m_offsets.tx_path_0.start = curr;
    m_offsets.tx_path_0.chip_tx_power_values = curr;

    power = htons_signed(-16);
    memcpy(curr, &power, sizeof(power));
    curr += sizeof(power);
    power = htons_signed(5);
    memcpy(curr, &power, sizeof(power));
    curr += sizeof(power);
    power = htons_signed(7);
    memcpy(curr, &power, sizeof(power));
    curr += sizeof(power);
    power = htons_signed(8);
    memcpy(curr, &power, sizeof(power));
    curr += sizeof(power);

    m_offsets.tx_path_0.channel_entry_table = curr;

    for (uint8_t channel_id = 0; channel_id < 16; channel_id++)
    {
        flash_tx_power_entry_t entry;

        entry = CHIP_TX_POWER_ENTRY(-16 + 5);
        memcpy(curr, &entry, sizeof(entry));
        curr += sizeof(entry);

        entry = CHIP_TX_POWER_ENTRY(5 + 5);
        memcpy(curr, &entry, sizeof(entry));
        curr += sizeof(entry);

        entry = CHIP_TX_POWER_ENTRY(7 + 5);
        memcpy(curr, &entry, sizeof(entry));
        curr += sizeof(entry);

        entry = CHIP_TX_POWER_ENTRY(8 + 5);
        memcpy(curr, &entry, sizeof(entry));
        curr += sizeof(entry);
    }

    power_table_write_length(m_memory_page, curr);

    uint32_t crc32 = crc32_compute(m_memory_page, (curr - m_memory_page), NULL);

    m_offsets.crc32 = curr;

    memcpy(curr, &crc32, sizeof(crc32));
    curr += sizeof(crc32);
}

void setUp(void)
{
    init_test_power_table();

    vendor_radio_power_map_psemi_data_get()->mp_power_map_addr = m_memory_page;
}

void tearDown(void)
{
}

void test_print(void)
{
    for (int i =0; i < (PAGE_SIZE/16); i++)
    {
        printf("%04u: %02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x\n",
               16 * i,
               m_memory_page[16 * i], m_memory_page[16 * i + 1],
               m_memory_page[16 * i + 2], m_memory_page[16 * i + 3],

               m_memory_page[16 * i + 4], m_memory_page[16 * i + 5],
               m_memory_page[16 * i + 6], m_memory_page[16 * i + 7],

               m_memory_page[16 * i + 8], m_memory_page[16 * i + 9],
               m_memory_page[16 * i + 10], m_memory_page[16 * i + 11],

               m_memory_page[16 * i + 12], m_memory_page[16 * i + 13],
               m_memory_page[16 * i + 14], m_memory_page[16 * i + 15]);
    }
}

void test_out_power_find_finds_exact_match(void)
{
    channel_entry_ref_t channel;
    tx_path_ref_t   tx_path;
    power_map_ref_t power_map;
    int8_t match_tx_power;
    uint8_t match_att_state;

    power_map_get(&power_map, vendor_radio_power_map_psemi_data_get()->mp_power_map_addr);
    tx_path_entry_get(&tx_path, &power_map, 0);
    channel_entry_get(&channel, &tx_path, 11);

    TEST_ASSERT_EQUAL_INT8(-8, out_power_find(&channel, -8, &match_tx_power, &match_att_state));

    TEST_ASSERT_EQUAL_INT8(-16, match_tx_power);
    TEST_ASSERT_EQUAL_UINT8(12, match_att_state);
}

void test_out_power_find_finds_exact_match_not_in_first_chip_power_entry(void)
{
    channel_entry_ref_t channel;
    tx_path_ref_t   tx_path;
    power_map_ref_t power_map;
    int8_t match_tx_power;
    uint8_t match_att_state;

    power_map_get(&power_map, vendor_radio_power_map_psemi_data_get()->mp_power_map_addr);
    tx_path_entry_get(&tx_path, &power_map, 0);
    channel_entry_get(&channel, &tx_path, 11);

    TEST_ASSERT_EQUAL_INT8(10, out_power_find(&channel, 10, &match_tx_power, &match_att_state));

    TEST_ASSERT_EQUAL_INT8(5, match_tx_power);
    TEST_ASSERT_EQUAL_UINT8(15, match_att_state);
}

void test_out_power_find_finds_one_of_exact_matches(void)
{
    channel_entry_ref_t channel;
    tx_path_ref_t   tx_path;
    power_map_ref_t power_map;
    int8_t match_tx_power;
    uint8_t match_att_state;

    power_map_get(&power_map, vendor_radio_power_map_psemi_data_get()->mp_power_map_addr);
    tx_path_entry_get(&tx_path, &power_map, 0);
    channel_entry_get(&channel, &tx_path, 11);

    TEST_ASSERT_EQUAL_INT8(14, out_power_find(&channel, 14, &match_tx_power, &match_att_state));

    TEST_ASSERT_EQUAL_INT8(5, match_tx_power);
    TEST_ASSERT_EQUAL_UINT8(11, match_att_state);
}

void test_out_power_find_finds_max_power(void)
{
    channel_entry_ref_t channel;
    tx_path_ref_t   tx_path;
    power_map_ref_t power_map;
    int8_t match_tx_power;
    uint8_t match_att_state;

    power_map_get(&power_map, vendor_radio_power_map_psemi_data_get()->mp_power_map_addr);
    tx_path_entry_get(&tx_path, &power_map, 0);
    channel_entry_get(&channel, &tx_path, 11);

    TEST_ASSERT_EQUAL_INT8(28, out_power_find(&channel, 99, &match_tx_power, &match_att_state));

    TEST_ASSERT_EQUAL_INT8(8, match_tx_power);
    TEST_ASSERT_EQUAL_UINT8(0, match_att_state);
}

void test_out_power_find_does_not_find_conforming_power(void)
{
    channel_entry_ref_t channel;
    tx_path_ref_t   tx_path;
    power_map_ref_t power_map;
    int8_t match_tx_power;
    uint8_t match_att_state;

    power_map_get(&power_map, vendor_radio_power_map_psemi_data_get()->mp_power_map_addr);
    tx_path_entry_get(&tx_path, &power_map, 0);
    channel_entry_get(&channel, &tx_path, 11);

    TEST_ASSERT_EQUAL_INT8(INT8_MIN, out_power_find(&channel, -50, &match_tx_power, &match_att_state));

    TEST_ASSERT_EQUAL_INT8(0, match_tx_power);
    TEST_ASSERT_EQUAL_UINT8(15, match_att_state);
}

void test_out_power_find_finds_best_match_for_missing_power(void)
{
    channel_entry_ref_t channel;
    tx_path_ref_t   tx_path;
    power_map_ref_t power_map;
    int8_t match_tx_power;
    uint8_t match_att_state;

    power_map_get(&power_map, vendor_radio_power_map_psemi_data_get()->mp_power_map_addr);
    tx_path_entry_get(&tx_path, &power_map, 0);
    channel_entry_get(&channel, &tx_path, 11);

    TEST_ASSERT_EQUAL_INT8(4, out_power_find(&channel, 5, &match_tx_power, &match_att_state));

    TEST_ASSERT_EQUAL_INT8(-16, match_tx_power);
    TEST_ASSERT_EQUAL_UINT8(0, match_att_state);
}

void test_power_map_build_for_channel_decreasing(void)
{
    channel_entry_ref_t channel;
    tx_path_ref_t       tx_path;
    power_map_ref_t     power_map;
    out_power_entry_t   result[NUM_SUPPORTED_POWERS]   = {0};
    out_power_entry_t   expected[NUM_SUPPORTED_POWERS] = {{.actual_power = 0, .radio_tx_power_db = -20, .att_state = 15},
                                                        {.actual_power = 1, .radio_tx_power_db = -20, .att_state = 14},
                                                        {.actual_power = 2, .radio_tx_power_db = -20, .att_state = 13},
                                                        {.actual_power = 3, .radio_tx_power_db = -20, .att_state = 12},
                                                        {.actual_power = 4, .radio_tx_power_db = -20, .att_state = 11},
                                                        {.actual_power = 5, .radio_tx_power_db = -20, .att_state = 10},
                                                        {.actual_power = 6, .radio_tx_power_db = -20, .att_state = 9},
                                                        {.actual_power = 7, .radio_tx_power_db = -20, .att_state = 8},
                                                        {.actual_power = 8, .radio_tx_power_db = -20, .att_state = 7},
                                                        {.actual_power = 9, .radio_tx_power_db = -20, .att_state = 6},
                                                        {.actual_power = 10, .radio_tx_power_db = -20, .att_state = 5},
                                                        {.actual_power = 11, .radio_tx_power_db = -20, .att_state = 4},
                                                        {.actual_power = 12, .radio_tx_power_db = -20, .att_state = 3},
                                                        {.actual_power = 13, .radio_tx_power_db = -20, .att_state = 2},
                                                        {.actual_power = 14, .radio_tx_power_db = -20, .att_state = 1},
                                                        {.actual_power = 15, .radio_tx_power_db = -20, .att_state = 0},
                                                        {.actual_power = 15, .radio_tx_power_db = -4, .att_state = 15},
                                                        {.actual_power = 15, .radio_tx_power_db = -4, .att_state = 15},
                                                        {.actual_power = 15, .radio_tx_power_db = -4, .att_state = 15},
                                                        {.actual_power = 15, .radio_tx_power_db = -4, .att_state = 15},
                                                        {.actual_power = 15, .radio_tx_power_db = -4, .att_state = 15}};

    init_test_power_table_decreasing();

    TEST_ASSERT_EQUAL_UINT(sizeof(result), sizeof(expected));

    power_map_get(&power_map, vendor_radio_power_map_psemi_data_get()->mp_power_map_addr);
    tx_path_entry_get(&tx_path, &power_map, 0);
    channel_entry_get(&channel, &tx_path, 11);

    power_map_build_for_channel(&channel, result);

    for (int i = 0; i < NUM_SUPPORTED_POWERS; i++)
    {
        printf("looking for %2u: found: %2u chip_tx_power: %3d att:%02x\n", i, result[i].actual_power,
               result[i].radio_tx_power_db, result[i].att_state);
    }

    TEST_ASSERT_EQUAL_MEMORY(expected, result, sizeof(expected));
}

void test_power_map_build_for_channel(void)
{
    channel_entry_ref_t channel;
    tx_path_ref_t       tx_path;
    power_map_ref_t     power_map;
    out_power_entry_t   result[NUM_SUPPORTED_POWERS]   = {0};
    out_power_entry_t   expected[NUM_SUPPORTED_POWERS] = {{.actual_power = 0, .radio_tx_power_db = -16, .att_state = 4},
                                                        {.actual_power = 1, .radio_tx_power_db = -16, .att_state = 3},
                                                        {.actual_power = 2, .radio_tx_power_db = -16, .att_state = 2},
                                                        {.actual_power = 3, .radio_tx_power_db = -16, .att_state = 1},
                                                        {.actual_power = 4, .radio_tx_power_db = -16, .att_state = 0},
                                                        {.actual_power = 4, .radio_tx_power_db = -16, .att_state = 0},
                                                        {.actual_power = 4, .radio_tx_power_db = -16, .att_state = 0},
                                                        {.actual_power = 4, .radio_tx_power_db = -16, .att_state = 0},
                                                        {.actual_power = 4, .radio_tx_power_db = -16, .att_state = 0},
                                                        {.actual_power = 4, .radio_tx_power_db = -16, .att_state = 0},
                                                        {.actual_power = 10, .radio_tx_power_db = 5, .att_state = 15},
                                                        {.actual_power = 11, .radio_tx_power_db = 5, .att_state = 14},
                                                        {.actual_power = 12, .radio_tx_power_db = 5, .att_state = 13},
                                                        {.actual_power = 13, .radio_tx_power_db = 5, .att_state = 12},
                                                        {.actual_power = 14, .radio_tx_power_db = 5, .att_state = 11},
                                                        {.actual_power = 15, .radio_tx_power_db = 5, .att_state = 10},
                                                        {.actual_power = 16, .radio_tx_power_db = 5, .att_state = 9},
                                                        {.actual_power = 17, .radio_tx_power_db = 5, .att_state = 8},
                                                        {.actual_power = 18, .radio_tx_power_db = 5, .att_state = 7},
                                                        {.actual_power = 19, .radio_tx_power_db = 5, .att_state = 6},
                                                        {.actual_power = 20, .radio_tx_power_db = 5, .att_state = 5}};

    TEST_ASSERT_EQUAL_UINT(sizeof(result), sizeof(expected));

    power_map_get(&power_map, vendor_radio_power_map_psemi_data_get()->mp_power_map_addr);
    tx_path_entry_get(&tx_path, &power_map, 0);
    channel_entry_get(&channel, &tx_path, 11);

    power_map_build_for_channel(&channel, result);

    for (int i = 0; i < NUM_SUPPORTED_POWERS; i++)
    {
        printf("looking for %2u: found: %2u chip_tx_power: %3d att:%02x\n", i, result[i].actual_power,
               result[i].radio_tx_power_db, result[i].att_state);
    }

    TEST_ASSERT_EQUAL_MEMORY(expected, result, sizeof(expected));
}

// regression
void test_flash_data_is_valid_with_invalid_length(void)
{
    uint8_t invalid_len_buf[PAGE_SIZE];

    memset(invalid_len_buf, 0xff, sizeof(invalid_len_buf));

    vendor_radio_power_map_psemi_data_get()->mp_power_map_addr = invalid_len_buf;
    power_map_get(&vendor_radio_power_map_psemi_data_get()->m_power_map_ref, vendor_radio_power_map_psemi_data_get()->mp_power_map_addr);

    TEST_ASSERT_EQUAL(POWER_MAP_STATUS_MISSING, vendor_radio_psemi_power_map_flash_data_is_valid());
}

// regression
void test_flash_data_is_valid_with_length_zero(void)
{
    uint8_t invalid_len_buf[PAGE_SIZE];

    memset(invalid_len_buf, 0xff, sizeof(invalid_len_buf));
    memset(invalid_len_buf, 0x00, sizeof(uint16_t));

    vendor_radio_power_map_psemi_data_get()->mp_power_map_addr = invalid_len_buf;
    power_map_get(&vendor_radio_power_map_psemi_data_get()->m_power_map_ref, vendor_radio_power_map_psemi_data_get()->mp_power_map_addr);

    TEST_ASSERT_EQUAL(POWER_MAP_STATUS_PARSING_ERR, vendor_radio_psemi_power_map_flash_data_is_valid());
}

// void test_flash_data_is_valid_with_length_exceeding_memory_page(void)
// {
//     TEST_FAIL_MESSAGE("implement it");
// }

void test_flash_data_is_valid_validates_correct_table(void)
{
    power_map_get(&vendor_radio_power_map_psemi_data_get()->m_power_map_ref, vendor_radio_power_map_psemi_data_get()->mp_power_map_addr);

    TEST_ASSERT_EQUAL(POWER_MAP_STATUS_OK, vendor_radio_psemi_power_map_flash_data_is_valid());
}

void test_flash_data_is_valid_fails_with_incorrect_crc(void)
{
    power_map_get(&vendor_radio_power_map_psemi_data_get()->m_power_map_ref, vendor_radio_power_map_psemi_data_get()->mp_power_map_addr);

    m_memory_page[12]++;

    TEST_ASSERT_EQUAL(POWER_MAP_STATUS_INVALID_CRC, vendor_radio_psemi_power_map_flash_data_is_valid());
}

void test_flash_data_is_valid_fails_with_incorrect_version(void)
{
    power_map_get(&vendor_radio_power_map_psemi_data_get()->m_power_map_ref, vendor_radio_power_map_psemi_data_get()->mp_power_map_addr);

    m_memory_page[2] = 0;
    m_memory_page[3] = 0;

    TEST_ASSERT_EQUAL(POWER_MAP_STATUS_INVALID_VERSION, vendor_radio_psemi_power_map_flash_data_is_valid());
}

void test_flash_data_is_valid_with_zero_tx_power_path_0(void)
{
    power_map_get(&vendor_radio_power_map_psemi_data_get()->m_power_map_ref, vendor_radio_power_map_psemi_data_get()->mp_power_map_addr);

    m_memory_page[6] = 0;

    TEST_ASSERT_EQUAL(POWER_MAP_STATUS_MISSING_TX_POWER_PATH, vendor_radio_psemi_power_map_flash_data_is_valid());
}

void test_flash_data_is_valid_with_zero_tx_power_path_1(void)
{
    power_map_get(&vendor_radio_power_map_psemi_data_get()->m_power_map_ref, vendor_radio_power_map_psemi_data_get()->mp_power_map_addr);

    init_test_power_table_with_one_path();

    TEST_ASSERT_EQUAL(POWER_MAP_STATUS_OK, vendor_radio_psemi_power_map_flash_data_is_valid());
}


void test_flash_data_read(void)
{
    power_map_get(&vendor_radio_power_map_psemi_data_get()->m_power_map_ref, vendor_radio_power_map_psemi_data_get()->mp_power_map_addr);

    TEST_ASSERT_EQUAL(POWER_MAP_STATUS_OK, vendor_radio_power_map_flash_data_read());
}

// regression
void test_flash_data_apply(void)
{
    // calculations for every channel shall yield the same results
    out_power_entry_t   expected[NUM_SUPPORTED_POWERS] = {{.actual_power = 0, .radio_tx_power_db = -16, .att_state = 4},
                                                        {.actual_power = 1, .radio_tx_power_db = -16, .att_state = 3},
                                                        {.actual_power = 2, .radio_tx_power_db = -16, .att_state = 2},
                                                        {.actual_power = 3, .radio_tx_power_db = -16, .att_state = 1},
                                                        {.actual_power = 4, .radio_tx_power_db = -16, .att_state = 0},
                                                        {.actual_power = 4, .radio_tx_power_db = -16, .att_state = 0},
                                                        {.actual_power = 4, .radio_tx_power_db = -16, .att_state = 0},
                                                        {.actual_power = 4, .radio_tx_power_db = -16, .att_state = 0},
                                                        {.actual_power = 4, .radio_tx_power_db = -16, .att_state = 0},
                                                        {.actual_power = 4, .radio_tx_power_db = -16, .att_state = 0},
                                                        {.actual_power = 10, .radio_tx_power_db = 5, .att_state = 15},
                                                        {.actual_power = 11, .radio_tx_power_db = 5, .att_state = 14},
                                                        {.actual_power = 12, .radio_tx_power_db = 5, .att_state = 13},
                                                        {.actual_power = 13, .radio_tx_power_db = 5, .att_state = 12},
                                                        {.actual_power = 14, .radio_tx_power_db = 5, .att_state = 11},
                                                        {.actual_power = 15, .radio_tx_power_db = 5, .att_state = 10},
                                                        {.actual_power = 16, .radio_tx_power_db = 5, .att_state = 9},
                                                        {.actual_power = 17, .radio_tx_power_db = 5, .att_state = 8},
                                                        {.actual_power = 18, .radio_tx_power_db = 5, .att_state = 7},
                                                        {.actual_power = 19, .radio_tx_power_db = 5, .att_state = 6},
                                                        {.actual_power = 20, .radio_tx_power_db = 5, .att_state = 5}};

    power_map_get(&vendor_radio_power_map_psemi_data_get()->m_power_map_ref, vendor_radio_power_map_psemi_data_get()->mp_power_map_addr);

    vendor_radio_psemi_power_map_tx_path_id_set(TX_PATH_ID_0);

    TEST_ASSERT_EQUAL(POWER_MAP_STATUS_OK, vendor_radio_psemi_power_map_flash_data_apply());
    TEST_ASSERT_EQUAL_PTR(&vendor_radio_power_map_psemi_data_get()->m_power_map_flash, vendor_radio_power_map_psemi_data_get()->m_power_map.power_map_current);
    for (uint8_t channel_num = MIN_802154_CHANNEL; channel_num <= MAX_802154_CHANNEL; channel_num++)
    {
        TEST_ASSERT_EQUAL_MEMORY(expected, vendor_radio_power_map_psemi_data_get()->m_power_map_flash.channels[channel_to_id(channel_num)].entries,
                                 sizeof(expected));
    }
}

void test_flash_data_apply_path1(void)
{
    // calculations for every channel shall yield the same results
    out_power_entry_t expected[NUM_SUPPORTED_POWERS] = {{.actual_power = -128, .radio_tx_power_db = 0, .att_state = 15},
                                                        {.actual_power = -128, .radio_tx_power_db = 0, .att_state = 15},
                                                        {.actual_power = -128, .radio_tx_power_db = 0, .att_state = 15},
                                                        {.actual_power = -128, .radio_tx_power_db = 0, .att_state = 15},
                                                        {.actual_power = -128, .radio_tx_power_db = 0, .att_state = 15},
                                                        {.actual_power = 5, .radio_tx_power_db = 0, .att_state = 15},
                                                        {.actual_power = 6, .radio_tx_power_db = 0, .att_state = 14},
                                                        {.actual_power = 7, .radio_tx_power_db = 0, .att_state = 13},
                                                        {.actual_power = 8, .radio_tx_power_db = 0, .att_state = 12},
                                                        {.actual_power = 9, .radio_tx_power_db = 0, .att_state = 11},
                                                        {.actual_power = 10, .radio_tx_power_db = 0, .att_state = 10},
                                                        {.actual_power = 11, .radio_tx_power_db = 0, .att_state = 9},
                                                        {.actual_power = 12, .radio_tx_power_db = 0, .att_state = 8},
                                                        {.actual_power = 13, .radio_tx_power_db = 0, .att_state = 7},
                                                        {.actual_power = 14, .radio_tx_power_db = 0, .att_state = 6},
                                                        {.actual_power = 15, .radio_tx_power_db = 0, .att_state = 5},
                                                        {.actual_power = 16, .radio_tx_power_db = 0, .att_state = 4},
                                                        {.actual_power = 17, .radio_tx_power_db = 0, .att_state = 3},
                                                        {.actual_power = 18, .radio_tx_power_db = 0, .att_state = 2},
                                                        {.actual_power = 19, .radio_tx_power_db = 0, .att_state = 1},
                                                        {.actual_power = 20, .radio_tx_power_db = 0, .att_state = 0}};

    power_map_get(&vendor_radio_power_map_psemi_data_get()->m_power_map_ref, vendor_radio_power_map_psemi_data_get()->mp_power_map_addr);

    vendor_radio_psemi_power_map_tx_path_id_set(TX_PATH_ID_1);

    TEST_ASSERT_EQUAL(POWER_MAP_STATUS_OK, vendor_radio_psemi_power_map_flash_data_apply());
    TEST_ASSERT_EQUAL_PTR(&vendor_radio_power_map_psemi_data_get()->m_power_map_flash, vendor_radio_power_map_psemi_data_get()->m_power_map.power_map_current);
    for (uint8_t channel_num = MIN_802154_CHANNEL; channel_num <= MAX_802154_CHANNEL; channel_num++)
    {
        TEST_ASSERT_EQUAL_MEMORY(expected, vendor_radio_power_map_psemi_data_get()->m_power_map_flash.channels[channel_to_id(channel_num)].entries,
                                 sizeof(expected));
    }
}

// regression
void test_flash_data_apply_fails_with_incorrect_crc_and_fem_active(void)
{
    power_map_get(&vendor_radio_power_map_psemi_data_get()->m_power_map_ref, vendor_radio_power_map_psemi_data_get()->mp_power_map_addr);

    m_memory_page[12]++;

    TEST_ASSERT_EQUAL(POWER_MAP_STATUS_INVALID_CRC, vendor_radio_psemi_power_map_flash_data_apply());
    TEST_ASSERT_EQUAL_PTR(&m_power_map_default_fem_enabled, vendor_radio_power_map_psemi_data_get()->m_power_map.power_map_current);
}

// regression
void test_power_map_init_path0(void)
{
    // calculations for every channel shall yield the same results
    out_power_entry_t expected[NUM_SUPPORTED_POWERS] = {{.actual_power = 0, .radio_tx_power_db = -16, .att_state = 4},
                                                        {.actual_power = 1, .radio_tx_power_db = -16, .att_state = 3},
                                                        {.actual_power = 2, .radio_tx_power_db = -16, .att_state = 2},
                                                        {.actual_power = 3, .radio_tx_power_db = -16, .att_state = 1},
                                                        {.actual_power = 4, .radio_tx_power_db = -16, .att_state = 0},
                                                        {.actual_power = 4, .radio_tx_power_db = -16, .att_state = 0},
                                                        {.actual_power = 4, .radio_tx_power_db = -16, .att_state = 0},
                                                        {.actual_power = 4, .radio_tx_power_db = -16, .att_state = 0},
                                                        {.actual_power = 4, .radio_tx_power_db = -16, .att_state = 0},
                                                        {.actual_power = 4, .radio_tx_power_db = -16, .att_state = 0},
                                                        {.actual_power = 10, .radio_tx_power_db = 5, .att_state = 15},
                                                        {.actual_power = 11, .radio_tx_power_db = 5, .att_state = 14},
                                                        {.actual_power = 12, .radio_tx_power_db = 5, .att_state = 13},
                                                        {.actual_power = 13, .radio_tx_power_db = 5, .att_state = 12},
                                                        {.actual_power = 14, .radio_tx_power_db = 5, .att_state = 11},
                                                        {.actual_power = 15, .radio_tx_power_db = 5, .att_state = 10},
                                                        {.actual_power = 16, .radio_tx_power_db = 5, .att_state = 9},
                                                        {.actual_power = 17, .radio_tx_power_db = 5, .att_state = 8},
                                                        {.actual_power = 18, .radio_tx_power_db = 5, .att_state = 7},
                                                        {.actual_power = 19, .radio_tx_power_db = 5, .att_state = 6},
                                                        {.actual_power = 20, .radio_tx_power_db = 5, .att_state = 5}};

    vendor_radio_flash_memory_for_power_map_start_address_get_ExpectAndReturn(m_memory_page);

    vendor_radio_psemi_power_map_init();

    TEST_ASSERT_EQUAL(POWER_MAP_STATUS_OK, vendor_radio_psemi_power_map_flash_data_last_flash_validation_status());
    TEST_ASSERT_EQUAL_PTR(&vendor_radio_power_map_psemi_data_get()->m_power_map_flash, vendor_radio_power_map_psemi_data_get()->m_power_map.power_map_current);
    for (uint8_t channel_num = MIN_802154_CHANNEL; channel_num <= MAX_802154_CHANNEL; channel_num++)
    {
        TEST_ASSERT_EQUAL_MEMORY(expected, vendor_radio_power_map_psemi_data_get()->m_power_map_flash.channels[channel_to_id(channel_num)].entries,
                                 sizeof(expected));
    }
}

void test_power_map_init_path1(void)
{
    // calculations for every channel shall yield the same results
    out_power_entry_t expected[NUM_SUPPORTED_POWERS] = {{.actual_power = -128, .radio_tx_power_db = 0, .att_state = 15},
                                                        {.actual_power = -128, .radio_tx_power_db = 0, .att_state = 15},
                                                        {.actual_power = -128, .radio_tx_power_db = 0, .att_state = 15},
                                                        {.actual_power = -128, .radio_tx_power_db = 0, .att_state = 15},
                                                        {.actual_power = -128, .radio_tx_power_db = 0, .att_state = 15},
                                                        {.actual_power = 5, .radio_tx_power_db = 0, .att_state = 15},
                                                        {.actual_power = 6, .radio_tx_power_db = 0, .att_state = 14},
                                                        {.actual_power = 7, .radio_tx_power_db = 0, .att_state = 13},
                                                        {.actual_power = 8, .radio_tx_power_db = 0, .att_state = 12},
                                                        {.actual_power = 9, .radio_tx_power_db = 0, .att_state = 11},
                                                        {.actual_power = 10, .radio_tx_power_db = 0, .att_state = 10},
                                                        {.actual_power = 11, .radio_tx_power_db = 0, .att_state = 9},
                                                        {.actual_power = 12, .radio_tx_power_db = 0, .att_state = 8},
                                                        {.actual_power = 13, .radio_tx_power_db = 0, .att_state = 7},
                                                        {.actual_power = 14, .radio_tx_power_db = 0, .att_state = 6},
                                                        {.actual_power = 15, .radio_tx_power_db = 0, .att_state = 5},
                                                        {.actual_power = 16, .radio_tx_power_db = 0, .att_state = 4},
                                                        {.actual_power = 17, .radio_tx_power_db = 0, .att_state = 3},
                                                        {.actual_power = 18, .radio_tx_power_db = 0, .att_state = 2},
                                                        {.actual_power = 19, .radio_tx_power_db = 0, .att_state = 1},
                                                        {.actual_power = 20, .radio_tx_power_db = 0, .att_state = 0}};

    vendor_radio_flash_memory_for_power_map_start_address_get_ExpectAndReturn(m_memory_page);

    vendor_radio_psemi_power_map_init();

    // TX_PATH_ID_0 is set in the init function by default
    vendor_radio_psemi_power_map_tx_path_id_set(TX_PATH_ID_1);
    vendor_radio_psemi_power_map_flash_data_apply();

    TEST_ASSERT_EQUAL(POWER_MAP_STATUS_OK, vendor_radio_psemi_power_map_flash_data_last_flash_validation_status());
    TEST_ASSERT_EQUAL_PTR(&vendor_radio_power_map_psemi_data_get()->m_power_map_flash, vendor_radio_power_map_psemi_data_get()->m_power_map.power_map_current);
    for (uint8_t channel_num = MIN_802154_CHANNEL; channel_num <= MAX_802154_CHANNEL; channel_num++)
    {
        TEST_ASSERT_EQUAL_MEMORY(expected, vendor_radio_power_map_psemi_data_get()->m_power_map_flash.channels[channel_to_id(channel_num)].entries,
                                 sizeof(expected));
    }
}

void test_power_map_flash_data_max_size_get_queries_power_settings_module(void)
{
    vendor_radio_flash_memory_for_power_map_size_get_ExpectAndReturn(PAGE_SIZE);

    TEST_ASSERT_EQUAL_UINT32(PAGE_SIZE, vendor_radio_psemi_power_map_flash_data_max_size_get());
}
