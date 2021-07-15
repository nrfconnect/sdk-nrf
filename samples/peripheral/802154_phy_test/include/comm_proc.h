/* Copyright (c) 2019, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Use in source and binary forms, redistribution in binary form only, with
 * or without modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 2. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 3. This software, with or without modification, must only be used with a Nordic
 *    Semiconductor ASA integrated circuit.
 *
 * 4. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/* Purpose: communication interfaces */

#ifndef COMM_PROC_H__
#define COMM_PROC_H__

#include <stdint.h>

#include <kernel.h>

/**< Maximum expected size of received packet */
#define COMM_MAX_TEXT_DATA_SIZE  (320u)

/**< Time to wait for a mark of end of a packet (new line) */
#define COMM_PER_COMMAND_TIMEOUT (500u)

/**< Maximum size of transmitted packet */
#define COMM_TX_BUFFER_SIZE      (320u)

/**< Additional symbols in the end of transmitted packet */
#define COMM_APPENDIX            "\r\n"
#define COMM_APPENDIX_SIZE       sizeof(COMM_APPENDIX)

/**< Stated of COMM packet parser */
typedef enum
{
    INPUT_STATE_IDLE = 0,            /**< Wait for the first symbol */
    INPUT_STATE_WAITING_FOR_NEWLINE, /**< Wait for new line symbol or timeout */
    INPUT_STATE_TEXT_PROCESSING,     /**< Passes received string to the library */
} input_state_t;

typedef struct text_proc_s
{
    struct k_timer timer;
    struct k_work  work;
    char           buf[COMM_MAX_TEXT_DATA_SIZE];
    uint16_t       len;
    input_state_t  state;
} text_proc_t;

void comm_init(void);

void comm_input_process(text_proc_t * text_proc, const uint8_t * buf, uint32_t len);

void comm_text_processor_fn(struct k_work * work);

void comm_proc(void);

void comm_input_timeout_handler(struct k_timer * timer);

#endif /* COMM_PROC_H__ */

