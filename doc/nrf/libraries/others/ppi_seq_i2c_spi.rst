.. _ppi_seq_i2c_spi:

PPI Sequencer for I2C/SPI
#########################

.. contents::
   :local:
   :depth: 2

The most common use case for the PPI sequencer is to use it with SPI or I2C.
The PPI Sequencer for I2C/SPI is a helper module that utilizes :ref:`ppi_seq`.
It is implemented as the Zephyr device with the devicetree configuration.
It supports the most common use case of the :ref:`ppi_seq` with periodic transfer using I2C/SPI.
It is using ``nrfx_spim`` and ``nrfx_twim`` drivers.

Configuration
*************

Use the dedicated bindings and the devicetree to configure the I2C or SPI node.
Configuration includes bus specific configuration and PPI sequencer specific configuration like a handle to the RTC/TIMER instance.

Example presents the configuration of the device which is using PPI sequencer for SPI.
It is configured to use RTC peripheral to trigger the transfer and 8 MHz frequency.

.. code-block:: c

    &spi130 {
        compatible = "nordic,ppi-seq-spi";
        status = "okay";
        pinctrl-0 = <&spi130_default_alt>;
        pinctrl-1 = <&spi130_sleep_alt>;
        pinctrl-names = "default", "sleep";
        overrun-character = <0x00>;
        memory-regions = <&cpuapp_dma_region>;
        cs-gpios = <&gpio0 8 GPIO_ACTIVE_LOW>;
        frequency = <8000000>;
        rtc = <&rtc130>;
    };

Usage
*****

Module allows to start and stop the sequencer.
It also allows to perform a single blocking transfer which can be used for example to perform an initialization of a sensor.

The sequencer is using pair of buffers in for each direction and pairs are used alternately.
Each RX buffer must be capable of storing all bytes in the batch.
It is possible to use the same TX content for each transfer by setting ``tx_postinc`` to false.
If ``tx_postinc`` is true then each TX buffer must be capable of storing all bytes in the batch and both TX buffers need to be filled by the user before starting the sequence.
Sequence is described by the :c:struct:`ppi_seq_i2c_spi_job` descriptor.
It contains information like:

- Transfer details: pointer and size of the RX and TX buffers
- Pointers to the second pair of TX and RX buffers
- Batch size
- After how many repetitions of the batch sequence is stopped
- Information about TX post-incrementation.
- Callback data


.. code-block:: c

    static void ppi_seq_cb(const struct device *dev, struct ppi_seq_i2c_spi_batch *batch,
                           bool last, void *user_data)
    {
        /* Callback called after completion of each batch. */
    }

    static int spi_seq_start(const struct device *ppi_seq_dev)
    {
        static uint8_t tx_buffer[TRANSFER_LENGTH];
        static uint8_t rx_buffer[2][TRANSFER_LENGTH * BATCH_LENGTH];

        struct ppi_seq_i2c_spi_job job = {
            .desc = {
                .spim = {
                    .p_tx_buffer = tx_buffer,
                    .tx_length = TRANSFER_LENGTH,
                    .p_rx_buffer = rx_buffer[0],
                    .rx_length = TRANSFER_LENGTH,
                }
            },
            .rx_second_buf = rx_buffer[1],
            .repeat = UINT32_MAX, /* Run indefinitely */
            .batch_cnt = BATCH_LENGTH,
            .tx_postinc = false,
            .cb = ppi_seq_cb
        };

        /* fill tx_buffer before starting the sequencer. */

        return ppi_seq_i2c_spi_start(ppi_seq_dev, PERIOD_US, &job);
    }

The sequencer can be stopped using :c:func:`ppi_seq_i2c_spi_stop`.
