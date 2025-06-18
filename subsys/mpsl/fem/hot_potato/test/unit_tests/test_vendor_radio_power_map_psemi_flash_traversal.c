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

#include "vendor_radio_power_map_psemi_flash_traversal.h"

#include "crc32.h"

#include "cexception_assert.h"

#include <stdint.h>
#include <stdio.h>

#include "vendor_radio_power_map_psemi_flash_traversal.c"

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

static uint8_t   m_memory_page[PAGE_SIZE] __attribute__((aligned(4)));
static uint8_t * mp_power_map_addr;

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

    flash_power_map_header_t header = {.lenght = 0, .nChipTxPower0 = 4, .nChipTxPower1 = 3};

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

void setUp(void)
{
    init_test_power_table();

    mp_power_map_addr = m_memory_page;
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

void test_power_map_get_asserts_when_power_map_is_null(void)
{
    power_map_ref_t power_map;

    TEST_ASSERT_FAIL_ASSERT(power_map_get(&power_map, NULL));
}

void test_power_map_get(void)
{
    power_map_ref_t power_map;

    power_map_get(&power_map, mp_power_map_addr);

    TEST_ASSERT_EQUAL_PTR(m_offsets.header, power_map.p_flash_header);
    TEST_ASSERT_EQUAL_PTR(m_offsets.tx_path_0.start, power_map.p_flash_tx_path_entry_0);
    TEST_ASSERT_EQUAL_PTR(m_offsets.tx_path_1.start, power_map.p_flash_tx_path_entry_1);
}

void test_tx_path_entry_get_assert_for_invalid_id(void)
{
    tx_path_ref_t   tx_path;
    power_map_ref_t power_map;

    power_map_get(&power_map, mp_power_map_addr);

    TEST_ASSERT_FAIL_ASSERT(tx_path_entry_get(&tx_path, &power_map, 30));
}

void test_tx_path_entry_get_id_0(void)
{
    tx_path_ref_t   tx_path;
    power_map_ref_t power_map;

    power_map_get(&power_map, mp_power_map_addr);

    tx_path_entry_get(&tx_path, &power_map, TX_PATH_ID_0);

    TEST_ASSERT_EQUAL_PTR(m_offsets.tx_path_0.start, tx_path.p_flash);
    TEST_ASSERT_EQUAL_PTR(4, tx_path.num_chip_tx_power);
}

void test_tx_path_entry_get_id_1(void)
{
    tx_path_ref_t   tx_path;
    power_map_ref_t power_map;

    power_map_get(&power_map, mp_power_map_addr);

    tx_path_entry_get(&tx_path, &power_map, TX_PATH_ID_1);

    TEST_ASSERT_EQUAL_PTR(m_offsets.tx_path_1.start, tx_path.p_flash);
    TEST_ASSERT_EQUAL_PTR(3, tx_path.num_chip_tx_power);
}

void test_channel_entry_get_assert_for_invalid_channel(void)
{
    channel_entry_ref_t channel;
    tx_path_ref_t   tx_path;
    power_map_ref_t power_map;

    power_map_get(&power_map, mp_power_map_addr);
    tx_path_entry_get(&tx_path, &power_map, TX_PATH_ID_1);

    TEST_ASSERT_FAIL_ASSERT(channel_entry_get(&channel, &tx_path, 30));
}

void test_channel_entry_get_returns_appropriate_channel_entry(void)
{
    channel_entry_ref_t channel;
    tx_path_ref_t   tx_path;
    power_map_ref_t power_map;

    power_map_get(&power_map, mp_power_map_addr);
    tx_path_entry_get(&tx_path, &power_map, TX_PATH_ID_0);

    for (size_t i = 0U; i < VENDOR_RADIO_SETTINGS_RADIO_CHANNEL_COUNT; i++)
    {
        memset(&channel, 0U, sizeof(channel));
        channel_entry_get(&channel, &tx_path, 11);
        TEST_ASSERT_EQUAL_PTR(m_offsets.tx_path_0.channel_entry_table + 0 * 4 * sizeof(flash_tx_power_entry_t),
                              channel.p_flash);
    }
}

void test_chip_tx_power_entry_iterator(void)
{
    chip_tx_power_iter_t iter;
    channel_entry_ref_t channel;
    tx_path_ref_t   tx_path;
    power_map_ref_t power_map;
    chip_tx_power_entry_ref_t chip_power;

    power_map_get(&power_map, mp_power_map_addr);
    tx_path_entry_get(&tx_path, &power_map, TX_PATH_ID_0);
    channel_entry_get(&channel, &tx_path, 11);

    chip_tx_power_iter_init(&iter, &channel);

    TEST_ASSERT_TRUE(chip_tx_power_next(&iter, &chip_power));
    TEST_ASSERT_EQUAL_PTR(channel.p_flash, chip_power.p_flash);
    TEST_ASSERT_EQUAL_INT8(-16, chip_power.chip_tx_power);

    TEST_ASSERT_TRUE(chip_tx_power_next(&iter, &chip_power));
    TEST_ASSERT_EQUAL_PTR(channel.p_flash + sizeof(flash_tx_power_entry_t), chip_power.p_flash);
    TEST_ASSERT_EQUAL_INT8(5, chip_power.chip_tx_power);

    TEST_ASSERT_TRUE(chip_tx_power_next(&iter, &chip_power));
    TEST_ASSERT_EQUAL_PTR(channel.p_flash + 2*sizeof(flash_tx_power_entry_t), chip_power.p_flash);
    TEST_ASSERT_EQUAL_INT8(7, chip_power.chip_tx_power);

    TEST_ASSERT_TRUE(chip_tx_power_next(&iter, &chip_power));
    TEST_ASSERT_EQUAL_PTR(channel.p_flash + 3*sizeof(flash_tx_power_entry_t), chip_power.p_flash);
    TEST_ASSERT_EQUAL_INT8(8, chip_power.chip_tx_power);

    TEST_ASSERT_FALSE(chip_tx_power_next(&iter, &chip_power));

    tx_path_entry_get(&tx_path, &power_map, TX_PATH_ID_1);
    channel_entry_get(&channel, &tx_path, 11);

    chip_tx_power_iter_init(&iter, &channel);

    TEST_ASSERT_TRUE(chip_tx_power_next(&iter, &chip_power));
    TEST_ASSERT_EQUAL_PTR(channel.p_flash, chip_power.p_flash);
    TEST_ASSERT_EQUAL_INT8(0, chip_power.chip_tx_power);

    TEST_ASSERT_TRUE(chip_tx_power_next(&iter, &chip_power));
    TEST_ASSERT_EQUAL_PTR(channel.p_flash + sizeof(flash_tx_power_entry_t), chip_power.p_flash);
    TEST_ASSERT_EQUAL_INT8(2, chip_power.chip_tx_power);

    TEST_ASSERT_TRUE(chip_tx_power_next(&iter, &chip_power));
    TEST_ASSERT_EQUAL_PTR(channel.p_flash + 2*sizeof(flash_tx_power_entry_t), chip_power.p_flash);
    TEST_ASSERT_EQUAL_INT8(4, chip_power.chip_tx_power);

    TEST_ASSERT_FALSE(chip_tx_power_next(&iter, &chip_power));
}

void test_att_entry_iterator(void)
{
    chip_tx_power_iter_t chip_power_iter;
    channel_entry_ref_t channel;
    tx_path_ref_t   tx_path;
    power_map_ref_t power_map;
    chip_tx_power_entry_ref_t chip_power;
    att_iter_t iter;
    att_entry_ref_t att;

    power_map_get(&power_map, mp_power_map_addr);
    tx_path_entry_get(&tx_path, &power_map, TX_PATH_ID_0);
    channel_entry_get(&channel, &tx_path, 11);

    chip_tx_power_iter_init(&chip_power_iter, &channel);

    TEST_ASSERT_TRUE(chip_tx_power_next(&chip_power_iter, &chip_power));
    TEST_ASSERT_TRUE(chip_tx_power_next(&chip_power_iter, &chip_power));

    att_iter_init(&iter, &chip_power);

    for (size_t i = 0U; i < NUM_ATT_STATES; i++)
    {
        TEST_ASSERT_TRUE(att_next(&iter, &att));
        TEST_ASSERT_EQUAL_PTR(chip_power.p_flash + i*sizeof(flash_att_entry_t), att.p_flash);
        TEST_ASSERT_EQUAL_UINT8(MAX_ATT_STATE - i, att.att_state);
    }

    TEST_ASSERT_FALSE(att_next(&iter, &att));
}
