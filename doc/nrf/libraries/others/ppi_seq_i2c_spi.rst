.. _ppi_seq_i2c_spi:

PPI Sequencer for I2C/SPI
#########################

.. contents::
   :local:
   :depth: 2

The PPI Sequencer for I2C/SPI is a helper module that simplifies using the :ref:`ppi_seq` library for periodic I2C or SPI transfers.

Overview
********

The PPI Sequencer for I2C/SPI is implemented as a Zephyr device with devicetree configuration and uses the ``nrfx_spim`` and ``nrfx_twim`` drivers internally.

The module provides a high-level interface for starting and stopping periodic transfer sequences.
It also supports single blocking transfers, which can be used for tasks such as initializing a sensor before starting the sequencer.

Configuration
*************

Use the dedicated bindings and the devicetree to configure the I2C or SPI node.
The configuration includes bus-specific properties (pin control, frequency, chip select) and PPI sequencer properties such as a handle to the RTC or TIMER instance used as the timing source.

The following example configures an SPI device to use the PPI sequencer with an RTC timing source and 8 MHz frequency:

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


Buffer management
=================

The sequencer uses a pair of buffers for each direction (TX and RX) and alternates between them so that one pair can be processed while the other is being filled.
Each RX buffer must be large enough to store all bytes in a full batch.

TX buffer behavior depends on the ``tx_postinc`` setting:

* When ``tx_postinc`` is ``false``, the same TX content is reused for every transfer in the batch.
* When ``tx_postinc`` is ``true``, each TX buffer must be large enough to store all bytes in a full batch, and you must fill both TX buffers before starting the sequence.

Job descriptor
==============

Each sequence is described by a :c:struct:`ppi_seq_i2c_spi_job` structure that contains the following:

* Transfer details: pointers and sizes of the TX and RX buffers.
* Pointers to the second pair of TX and RX buffers.
* Batch size (number of transfers per batch).
* Repeat count (number of batches to execute before the sequencer stops automatically, or ``UINT32_MAX`` to run indefinitely).
* TX post-incrementation setting.
* Callback and user data for batch completion notification.

Starting and stopping the sequencer
***********************************

Start the sequencer by calling :c:func:`ppi_seq_i2c_spi_start` with a period and a job descriptor.
Stop it at any time by calling :c:func:`ppi_seq_i2c_spi_stop`.

Example
=======

The following example starts an indefinite SPI sequence that reuses the same TX content for every transfer and calls ``ppi_seq_cb`` after each batch completes:

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

        /* Fill tx_buffer before starting the sequencer. */

        return ppi_seq_i2c_spi_start(ppi_seq_dev, PERIOD_US, &job);
    }

Dependencies
************

The modules uses the following |NCS| library:

* :ref:`ppi_seq`

The module uses the following ``nrfx`` libraries and drivers:

* ``nrfx_spim``
* ``nrfx_twim``

API documentation
*****************

| Header file: :file:`include/ppi_seq/ppi_seq_i2c_spi.h`
| Source files: :file:`lib/ppi_seq/`

.. doxygengroup:: ppi_seq_i2c_spi
