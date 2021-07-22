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

/* Purpose: Communication interface for library */

#include <assert.h>
#include <stdint.h>

#include "uart_proc.h"
#ifdef USE_USB
#include "usb_proc.h"
#endif
#include "comm_proc.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(comm);

#include "ptt_uart_api.h"

static int32_t comm_send_cb(const uint8_t * p_pkt, ptt_pkt_len_t len, bool add_crlf);

static void comm_symbols_process(text_proc_t * p_text_proc, const uint8_t * buf, uint32_t len);
static inline void text_process(text_proc_t * p_text_proc);
static inline void input_state_reset(text_proc_t * text_proc);

void comm_input_process(text_proc_t * p_text_proc, const uint8_t * buf, uint32_t len)
{
    switch (p_text_proc->state)
    {
        case INPUT_STATE_IDLE:
            if (0 != p_text_proc->len)
            {
                LOG_WRN("p_text_proc->len is not 0 when input state is idle");
                p_text_proc->len = 0;
            }

            /* start the timer after first received symbol */
            k_timer_start(&p_text_proc->timer, K_MSEC(COMM_PER_COMMAND_TIMEOUT), K_MSEC(0));

            p_text_proc->state = INPUT_STATE_WAITING_FOR_NEWLINE;

            comm_symbols_process(p_text_proc, buf, len);
            break;

        case INPUT_STATE_WAITING_FOR_NEWLINE:
            comm_symbols_process(p_text_proc, buf, len);
            break;

        case INPUT_STATE_TEXT_PROCESSING:
            LOG_WRN(
                "received a command when processing previous one, discarded");
            break;

        default:
            LOG_ERR("incorrect input state");
            break;
    }
}

K_FIFO_DEFINE(text_processing_fifo);

void comm_text_processor_fn(struct k_work * work)
{
    LOG_DBG("Called work queue function");
    text_proc_t * p_text_proc = CONTAINER_OF(work, text_proc_t, work);

    /* force string termination for further processing */
    p_text_proc->buf[p_text_proc->len] = '\0';
    LOG_DBG("received packet to lib: len %d, %s",
            p_text_proc->len,
            log_strdup(p_text_proc->buf));
    text_process(p_text_proc);
}

void comm_input_timeout_handler(struct k_timer * timer)
{
    LOG_DBG("push packet to lib via timer");
    text_proc_t * p_text_proc = CONTAINER_OF(timer, text_proc_t, timer);

    k_work_submit(&p_text_proc->work);
}

static void comm_symbols_process(text_proc_t * p_text_proc, const uint8_t * buf, uint32_t len)
{
    for (uint32_t cnt = 0; cnt < len; cnt++)
    {
        if (('\n' == buf[cnt]) || ('\r' == buf[cnt]))
        {
            LOG_DBG("push packet to lib via newline");
            /* Stop the timeout timer */
            k_timer_stop(&p_text_proc->timer);
            k_work_submit(&p_text_proc->work);

            p_text_proc->state = INPUT_STATE_TEXT_PROCESSING;
            break;
        }
        else
        {
            p_text_proc->buf[p_text_proc->len] = buf[cnt];
            ++p_text_proc->len;

            if (p_text_proc->len >= COMM_MAX_TEXT_DATA_SIZE)
            {
                LOG_ERR("received %d bytes and do not able to parse it, freeing input buffer",
                        COMM_MAX_TEXT_DATA_SIZE);

                /* Stop the timeout timer */
                k_timer_stop(&p_text_proc->timer);
                input_state_reset(p_text_proc);
            }
        }
    }
}

static inline void input_state_reset(text_proc_t * p_text_proc)
{
    p_text_proc->len   = 0;
    p_text_proc->state = INPUT_STATE_IDLE;
}

static inline void text_process(text_proc_t * p_text_proc)
{
    ptt_uart_push_packet((uint8_t *)p_text_proc->buf, p_text_proc->len);
    input_state_reset(p_text_proc);
}

void comm_init(void)
{
    /* won't initialize UART if the device has DUT mode only */
#if CONFIG_DUT_MODE
    LOG_INF("Device is in DUT mode only.");
#else
    LOG_INF("Init");
    uart_init();
#ifdef USE_USB
    usb_init();
#endif
    ptt_uart_init(comm_send_cb);
#endif
}

static int32_t comm_send_cb(const uint8_t * p_pkt, ptt_pkt_len_t len, bool add_crlf)
{
    if ((add_crlf && (len >= (COMM_TX_BUFFER_SIZE - COMM_APPENDIX_SIZE))) ||
        (!add_crlf && (len >= COMM_TX_BUFFER_SIZE)))
    {
        LOG_WRN("comm_send_cb: not enough of tx buffer size");
        return -1;
    }

    uart_send(p_pkt, len, add_crlf);

#ifdef USE_USB
    usb_send(p_pkt, len, add_crlf);
#endif

    return len;
}

void comm_proc(void)
{
#ifdef USE_USB
    usb_task();
#endif
}