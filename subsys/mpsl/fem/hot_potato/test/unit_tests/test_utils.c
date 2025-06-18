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

#include "utils.h"

void setUp(void)
{
}

void tearDown(void)
{
}

void test_swap_uint16(void)
{
    TEST_ASSERT_EQUAL_UINT16(0xfdab, swap_uint16(0xabfd));
}

void test_swap_int16(void)
{
    TEST_ASSERT_EQUAL_INT16(0xedab, swap_int16(0xabed));
    TEST_ASSERT_EQUAL_INT16(0x7bfd, swap_int16((int16_t)0xfd7b));
}

void test_htons(void)
{
    TEST_ASSERT_EQUAL_UINT16(0xfdab, htons(0xabfd));
}

void test_ntohs(void)
{
    TEST_ASSERT_EQUAL_UINT16(0xfdab, ntohs(0xabfd));
}

void test_htons_signed(void)
{
    TEST_ASSERT_EQUAL_INT16((int16_t)0xfdab, htons_signed(0xabfd));
    TEST_ASSERT_EQUAL_INT16(0x7bfd, htons_signed((uint16_t)0xfd7b));
}

void test_ntohs_signed(void)
{
    TEST_ASSERT_EQUAL_INT16((int16_t)0xfdab, ntohs_signed(0xabfd));
    TEST_ASSERT_EQUAL_INT16(0x7bfd, ntohs_signed((int16_t)0xfd7b));
}

void test_divide_round_up(void)
{
    TEST_ASSERT_EQUAL_UINT32(1, divide_round_up(200, 200));
    TEST_ASSERT_EQUAL_UINT32(0, divide_round_up(0, 2));
    TEST_ASSERT_EQUAL_UINT32(2, divide_round_up(4, 3));
}

void test_round_up_to_multiple_of(void)
{
    TEST_ASSERT_EQUAL_UINT32(200, round_up_to_multiple_of(200, 200));
    TEST_ASSERT_EQUAL_UINT32(0, round_up_to_multiple_of(0, 2));
    TEST_ASSERT_EQUAL_UINT32(12, round_up_to_multiple_of(10, 4));
    TEST_ASSERT_EQUAL_UINT32(2, round_up_to_multiple_of(1, 2));
}
