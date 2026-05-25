.. _spim_zephyr_spi_rtio:

SPIM Zephyr RTIO driver
#######################

Complete driver API reference can be found under the following links: `Zephyr's SPI driver API`_ and `Zephyr's SPI RTIO driver API`_.

Purpose of this page is to present peripheral usage scenarios covered by the RTIO-based SPI driver API.

Overview
********

The RTIO (Real-Time I/O) driver backend for the nRF SPIM peripheral provides a native, non-blocking I/O path built on top of Zephyr's :ref:`RTIO subsystem <rtio>`.
Instead of the traditional :c:func:`spi_transceive` call model, applications submit I/O requests to a submission queue (SQ) and later consume results from a completion queue (CQ).

This backend is selected automatically when ``CONFIG_SPI_RTIO=y`` is set and the ``nordic,nrf-spim`` compatible is present in the devicetree.
It is mutually exclusive with the standard ``SPI_NRFX_SPIM`` backend:

- ``CONFIG_SPI_NRFX_SPIM`` — selected when ``CONFIG_SPI_RTIO`` is **not** set.
- ``CONFIG_SPI_NRFX_SPIM_RTIO`` — selected when ``CONFIG_SPI_RTIO=y`` is set.

Configuration
*************

Static configuration
====================

The devicetree configuration is identical to the standard SPIM driver.
A typical SPI controller node in an application overlay looks as follows:

.. code-block:: devicetree

   &spi20 {
       status = "okay";
       pinctrl-0 = <&spi20_default>;
       pinctrl-names = "default";
       cs-gpios = <&gpio0 4 GPIO_ACTIVE_LOW>;

       my_device: my-device@0 {
           compatible = "vnd,my-spi-device";
           reg = <0>;
           spi-max-frequency = <DT_FREQ_M(8)>;
       };
   };

No additional devicetree properties are required to enable the RTIO backend.

Kconfig configuration
=====================

Enable the RTIO subsystem and the SPI RTIO support:

.. code-block:: kconfig

   CONFIG_RTIO=y
   CONFIG_SPI=y
   CONFIG_SPI_RTIO=y

When ``CONFIG_SPI_RTIO=y`` is set, the build system automatically selects ``CONFIG_SPI_NRFX_SPIM_RTIO`` for nRF SoCs with a ``nordic,nrf-spim`` devicetree node enabled.

Two additional Kconfig options control the sizes of the per-device RTIO context queues:

.. code-block:: kconfig

   # Number of submission queue entries (SQEs) per SPIM RTIO context (default: 8)
   CONFIG_SPI_NRFX_SPIM_RTIO_SQE_POOL_SIZE=8

   # Number of completion queue entries (CQEs) per SPIM RTIO context (default: 8)
   CONFIG_SPI_NRFX_SPIM_RTIO_CQE_POOL_SIZE=8

Increase these values if the application submits more than eight in-flight requests to a single SPI device.

Runtime configuration
=====================

The SPI iodev and the RTIO context are set up at build time using :c:macro:`SPI_IODEV_DEFINE` and :c:macro:`RTIO_DEFINE`.
The :c:macro:`SPI_IODEV_DEFINE` macro binds a devicetree SPI device node to an iodev object, including the operation flags, frequency, and chip-select configuration derived from the devicetree:

.. code-block:: c

   #include <zephyr/drivers/spi.h>
   #include <zephyr/rtio/rtio.h>
   #include <zephyr/devicetree.h>

   #define MY_SPI_NODE DT_NODELABEL(my_device)

   /* Define the iodev for the SPI device node */
   SPI_IODEV_DEFINE(spi_iodev, MY_SPI_NODE,
                    SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_MODE_CPOL | SPI_MODE_CPHA,
                    0 /* cs_delay_us */);

   /* Define an RTIO context with 4 SQE slots and 4 CQE slots */
   RTIO_DEFINE(spi_rtio_ctx, 4, 4);

   int main(void) {
       if (!spi_is_ready_iodev(&spi_iodev)) {
           return -ENODEV;
       }
   }

The second argument to :c:macro:`SPI_IODEV_DEFINE` is the ``operation`` bitmask (same flags as used with :c:macro:`SPI_DT_SPEC_GET`).
The third argument is an optional chip-select delay in microseconds.
Frequency and chip-select GPIO are derived automatically from the devicetree node.

:c:func:`spi_is_ready_iodev` checks that the SPI controller device and, if configured, the chip-select GPIO are ready for use.

Data transfer
*************

Transfers are performed by preparing submission queue entries (SQEs), submitting them to the RTIO context, and then consuming completion queue events (CQEs) to determine the result.

The helper :c:func:`spi_rtio_copy` copies a pair of :c:struct:`spi_buf_set` descriptors into a set of SQEs chained on the iodev.
This mirrors the buffer model of the traditional :c:func:`spi_transceive` API and is the recommended way to perform standard transfers.

Basic TX-RX transfer
====================

To perform a simultaneous transmit and receive transfer:

.. code-block:: c

   #include <zephyr/drivers/spi.h>
   #include <zephyr/rtio/rtio.h>

   static uint8_t tx_data[4] = { 0x01, 0x02, 0x03, 0x04 };
   static uint8_t rx_data[4];

   static const struct spi_buf tx_buf = {
       .buf = tx_data,
       .len = sizeof(tx_data),
   };
   static const struct spi_buf rx_buf = {
       .buf = rx_data,
       .len = sizeof(rx_data),
   };

   static const struct spi_buf_set tx_set = { .buffers = &tx_buf, .count = 1 };
   static const struct spi_buf_set rx_set = { .buffers = &rx_buf, .count = 1 };

   int main(void) {
       struct rtio_sqe *last_sqe = NULL;
       struct rtio_cqe *cqe;
       int err;

       /* Copy buf_sets into chained SQEs on the iodev */
       err = spi_rtio_copy(&spi_rtio_ctx, &spi_iodev,
                           &tx_set, &rx_set, &last_sqe);
       if (err < 0) {
           return err;
       }

       /* Submit all prepared SQEs */
       rtio_submit(&spi_rtio_ctx, 0);

       /* Wait for and consume the completion event */
       cqe = rtio_cqe_consume_block(&spi_rtio_ctx);
       err = cqe->result;
       rtio_cqe_release(&spi_rtio_ctx, cqe);

       if (err < 0) {
           /* Transfer failed */
       }

       /* rx_data now contains the received bytes */
       return 0;
   }

Passing ``NULL`` for either ``tx_bufs`` or ``rx_bufs`` in :c:func:`spi_rtio_copy` results in a transmit-only or receive-only transfer, respectively.

.. caution::

   Depending on the specific SoC, buffers provided via :c:struct:`spi_buf` may need to comply with specific alignment or RAM location requirements imposed by the underlying DMA hardware.
   Refer to the respective Product Specification for more details.

Scatter-gather transfer
=======================

Because :c:func:`spi_rtio_copy` accepts a :c:struct:`spi_buf_set` containing multiple :c:struct:`spi_buf` descriptors, scatter-gather transfers work the same way as with the standard API.
The following sends a two-part transmit buffer (header followed by payload) without copying into a single contiguous region:

.. code-block:: c

   static uint8_t header[2]  = { 0xAB, 0x01 };
   static uint8_t payload[16] = { ... };

   static const struct spi_buf tx_bufs[] = {
       { .buf = header,  .len = sizeof(header)  },
       { .buf = payload, .len = sizeof(payload) },
   };
   static const struct spi_buf_set tx_set = { .buffers = tx_bufs, .count = 2 };

   struct rtio_sqe *last_sqe = NULL;

   err = spi_rtio_copy(&spi_rtio_ctx, &spi_iodev, &tx_set, NULL, &last_sqe);
   rtio_submit(&spi_rtio_ctx, 0);

   struct rtio_cqe *cqe = rtio_cqe_consume_block(&spi_rtio_ctx);
   err = cqe->result;
   rtio_cqe_release(&spi_rtio_ctx, cqe);

Asynchronous (non-blocking) transfer
=====================================

The primary advantage of the RTIO backend is that submission and completion are decoupled.
An application can submit a transfer and continue doing other work, then check or wait for the CQE later:

.. code-block:: c

   struct rtio_sqe *last_sqe = NULL;

   /* Prepare and submit — returns immediately */
   err = spi_rtio_copy(&spi_rtio_ctx, &spi_iodev,
                       &tx_set, &rx_set, &last_sqe);
   if (err < 0) {
       return err;
   }
   rtio_submit(&spi_rtio_ctx, 0);

   /* ... do other work ... */

   /* Block until the transfer completes */
   struct rtio_cqe *cqe = rtio_cqe_consume_block(&spi_rtio_ctx);
   err = cqe->result;
   rtio_cqe_release(&spi_rtio_ctx, cqe);

   if (err < 0) {
       /* Transfer failed */
   }
   /* rx_data now contains received bytes */

To poll for completion without blocking, use :c:func:`rtio_cqe_consume` instead, which returns ``NULL`` if no CQE is available yet:

.. code-block:: c

   struct rtio_cqe *cqe = rtio_cqe_consume(&spi_rtio_ctx);
   if (cqe != NULL) {
       err = cqe->result;
       rtio_cqe_release(&spi_rtio_ctx, cqe);
   }

Chaining multiple transfers
============================

Multiple SQE chains can be queued before a single :c:func:`rtio_submit` call.
All operations targeting the same iodev are serialised by the RTIO executor.
Operations on different iodevs may proceed concurrently:

.. code-block:: c

   struct rtio_sqe *last_sqe = NULL;

   /* First transfer: write a command */
   err = spi_rtio_copy(&spi_rtio_ctx, &spi_iodev,
                       &cmd_tx_set, NULL, &last_sqe);

   /* Second transfer: read the response */
   err = spi_rtio_copy(&spi_rtio_ctx, &spi_iodev,
                       NULL, &resp_rx_set, &last_sqe);

   /* Submit both chains at once */
   rtio_submit(&spi_rtio_ctx, 0);

   /* Consume two CQEs */
   for (int i = 0; i < 2; i++) {
       struct rtio_cqe *cqe = rtio_cqe_consume_block(&spi_rtio_ctx);
       if (cqe->result < 0) { ... }
       rtio_cqe_release(&spi_rtio_ctx, cqe);
   }

.. note::

   The ordering of CQEs on the completion queue is not guaranteed to match the submission order across independent chains.
   Within a single chain, ordering and failure cascading are preserved.

Chip select management
======================

Chip select is managed automatically by the driver as part of each submission chain, using the GPIO specified in the ``cs-gpios`` devicetree property and the delay encoded in the iodev.
Manual chip-select control is not available when using the RTIO API.
