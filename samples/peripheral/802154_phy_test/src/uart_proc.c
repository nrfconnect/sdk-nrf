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

/* Purpose: UART configuring and processing */

/* UART handler waits for a string with new line symbol in the end. The handler reads a packet
   from UARTE driver per a byte and looks for a new line symbol or waits until a timer is expired
   to process received sequence without new line symbol. Then the packet is pushed to the library. */

#include <assert.h>
#include <stdint.h>

#include <drivers/uart.h>
#include <sys/ring_buffer.h>

#include "comm_proc.h"
#include "uart_proc.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(uart);

static text_proc_t      m_text_proc_uart;
static struct k_timer   m_uart_timer;
static const struct device * m_uart_dev;

RING_BUF_DECLARE(uart_rb, 10240);

static void uart_irq_handler(const struct device * dev, void * context)
{
    uint8_t *m_data_tx;
    uart_irq_update(dev);
    if (uart_irq_tx_ready(dev))
    {
        int nr_bytes_read = ring_buf_get_claim(&uart_rb, &m_data_tx, CONFIG_UART_0_NRF_TX_BUFFER_SIZE);
        int sent = uart_fifo_fill(dev, m_data_tx, nr_bytes_read);
        ring_buf_get_finish(&uart_rb, sent);
        if (ring_buf_is_empty(&uart_rb))
        {
            uart_irq_tx_disable(dev);
        }
    }

    if (uart_irq_rx_ready(dev))
    {
        uint8_t buf[10];
        int     len = uart_fifo_read(dev, buf, sizeof(buf));

        if (len)
        {
            /* Call this */
            comm_input_process(&m_text_proc_uart, buf, UART_BYTES_TO_READ);
        }
    }
}

void uart_init(void)
{
    LOG_INF("Init");

    m_uart_dev = device_get_binding("UART_0");
    __ASSERT(m_uart_dev, "Failed to get the device");
    uart_irq_callback_user_data_set(m_uart_dev, uart_irq_handler, NULL);
    uart_irq_rx_enable(m_uart_dev);

    m_text_proc_uart       = (text_proc_t){0};
    m_text_proc_uart.timer = m_uart_timer;
    k_timer_init(&m_text_proc_uart.timer, comm_input_timeout_handler, NULL);
    k_work_init(&m_text_proc_uart.work, comm_text_processor_fn);
}

int32_t uart_send(const uint8_t * p_pkt, ptt_pkt_len_t len, bool add_crlf)
{
    while(ring_buf_space_get(&uart_rb) < len)
    {
        LOG_WRN("queue full, waiting for free space");
        k_sleep(K_MSEC(1));
    }

    uart_irq_tx_disable(m_uart_dev);
    ring_buf_put(&uart_rb, p_pkt, len);
    if (add_crlf)
    {
        ring_buf_put(&uart_rb, COMM_APPENDIX, COMM_APPENDIX_SIZE);
    }
    uart_irq_tx_enable(m_uart_dev);

    return len;
}

