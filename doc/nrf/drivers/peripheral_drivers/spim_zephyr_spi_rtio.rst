.. _spim_zephyr_spi_rtio:

SPIM Zephyr RTIO driver
#######################

The RTIO-based Zephyr SPI driver provides a native, non-blocking API for communicating with SPI peripherals on nRF SoCs, built on top of Zephyr's :ref:`RTIO subsystem <rtio>`.

For the complete API reference, see `Zephyr's SPI driver API`_ and `Zephyr's RTIO subsystem`_.

This page describes the common usage scenarios covered by the RTIO-based API, including configuration, data transfer modes, transfer chaining, and chip select management.

Overview
********

The RTIO (Real-Time I/O) driver backend for the nRF SPIM peripheral provides a native, non-blocking I/O path built on top of Zephyr's :ref:`RTIO subsystem <rtio>`.
Instead of the traditional :c:func:`spi_transceive` call model, applications submit I/O requests to a submission queue (SQ) and later consume results from a completion queue (CQ).

This backend is selected automatically when ``CONFIG_SPI_RTIO=y`` is set and the ``nordic,nrf-spim`` compatible is present in the devicetree.
It is mutually exclusive with the standard ``SPI_NRFX_SPIM`` backend:

* ``CONFIG_SPI_NRFX_SPIM`` — selected when ``CONFIG_SPI_RTIO`` is **not** set.
* ``CONFIG_SPI_NRFX_SPIM_RTIO`` — selected when ``CONFIG_SPI_RTIO=y`` is set.

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

Two additional Kconfig options control the sizes of the per-device RTIO context that the driver uses internally to service the legacy :c:func:`spi_transceive` API when ``CONFIG_SPI_RTIO`` is enabled:

.. code-block:: kconfig

   # Number of submission queue entries (SQEs) per SPIM RTIO context (default: 8)
   CONFIG_SPI_NRFX_SPIM_RTIO_SQE_POOL_SIZE=8

   # Number of completion queue entries (CQEs) per SPIM RTIO context (default: 8)
   CONFIG_SPI_NRFX_SPIM_RTIO_CQE_POOL_SIZE=8

These options apply only to that internal context.
Increase them if the application submits more than eight in-flight requests to a single SPI device through the legacy :c:func:`spi_transceive` API.
When using the RTIO API directly, the number of in-flight requests is instead bounded by the submission queue entries (SQEs) allocated by the application's own :c:macro:`RTIO_DEFINE`, described in the `Runtime configuration`_ section.

Runtime configuration
=====================

The SPI iodev and the RTIO context are set up at build time using :c:macro:`SPI_DT_IODEV_DEFINE` and :c:macro:`RTIO_DEFINE`.
The :c:macro:`SPI_DT_IODEV_DEFINE` macro binds a devicetree SPI device node to an iodev object, including the operation flags, frequency, and chip-select configuration derived from the devicetree:

.. code-block:: c

   #include <zephyr/drivers/spi.h>
   #include <zephyr/rtio/rtio.h>
   #include <zephyr/devicetree.h>

   #define MY_SPI_NODE DT_NODELABEL(my_device)

   /* Define the iodev for the SPI device node */
   SPI_DT_IODEV_DEFINE(spi_iodev, MY_SPI_NODE,
                       SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_MODE_CPOL | SPI_MODE_CPHA,
                       0 /* cs_delay_us */);

   /* Define an RTIO context with 4 SQE slots and 4 CQE slots */
   RTIO_DEFINE(spi_rtio_ctx, 4, 4);

   int main(void) {
       if (!spi_is_ready_iodev(&spi_iodev)) {
           return -ENODEV;
       }
   }

The second argument to :c:macro:`SPI_DT_IODEV_DEFINE` is the ``operation`` bitmask (same flags as used with :c:macro:`SPI_DT_SPEC_GET`).
The third argument is an optional chip-select delay in microseconds.
Frequency and chip-select GPIO are derived automatically from the devicetree node.

:c:func:`spi_is_ready_iodev` checks that the SPI controller device and, if configured, the chip-select GPIO are ready for use.

Chip select management
======================

Chip select is managed automatically by the driver as part of each submission chain, using the GPIO specified in the ``cs-gpios`` devicetree property and the delay encoded in the iodev.
Manual chip-select control is not available when using the RTIO API.

Data transfer
*************

Transfers are performed by acquiring submission queue entries (SQEs) from the RTIO context, preparing them against the SPI iodev, submitting them, and then consuming completion queue events (CQEs) to determine the result.

The SQEs are prepared using the generic submission helpers from Zephyr's :ref:`RTIO subsystem <rtio>`:

* :c:func:`rtio_sqe_prep_transceive` for a simultaneous transmit and receive (full-duplex) transfer.
* :c:func:`rtio_sqe_prep_write` for a transmit-only transfer.
* :c:func:`rtio_sqe_prep_read` for a receive-only transfer.

Each helper takes a plain buffer pointer and length rather than a :c:struct:`spi_buf_set`.
:c:func:`rtio_sqe_prep_transceive` requires both the transmit and receive buffers to be valid.

See `Buffer requirements`_ for the constraints that the nRF SPIM peripheral imposes on the transfer buffers.

Basic TX-RX transfer
====================

To perform a simultaneous transmit and receive transfer, acquire an SQE, prepare it with :c:func:`rtio_sqe_prep_transceive`, submit it, and wait for the completion:

.. code-block:: c

   #include <zephyr/drivers/spi.h>
   #include <zephyr/rtio/rtio.h>

   static uint8_t tx_data[4] = { 0x01, 0x02, 0x03, 0x04 };
   static uint8_t rx_data[4];

   int main(void) {
       struct rtio_sqe *sqe;
       struct rtio_cqe *cqe;
       int err;

       /* Acquire an SQE and prepare a full-duplex transfer */
       sqe = rtio_sqe_acquire(&spi_rtio_ctx);
       if (sqe == NULL) {
           return -ENOMEM;
       }
       rtio_sqe_prep_transceive(sqe, &spi_iodev, RTIO_PRIO_NORM,
                                tx_data, rx_data, sizeof(tx_data), NULL);

       /* Submit the prepared SQE and wait for one completion */
       rtio_submit(&spi_rtio_ctx, 1);

       /* Consume the completion event */
       cqe = rtio_cqe_consume(&spi_rtio_ctx);
       err = cqe->result;
       rtio_cqe_release(&spi_rtio_ctx, cqe);

       if (err < 0) {
           /* Transfer failed */
       }

       /* rx_data now contains the received bytes */
       return 0;
   }

The second argument to :c:func:`rtio_submit` is the number of completions to wait for before returning; passing ``0`` submits without blocking.
For a transmit-only or receive-only transfer, use :c:func:`rtio_sqe_prep_write` or :c:func:`rtio_sqe_prep_read` respectively, each of which takes a single buffer.

Multi-part (transaction) transfer
=================================

A single logical transfer can be split across multiple buffers by chaining SQEs with the :c:macro:`RTIO_SQE_TRANSACTION` flag.
All SQEs in a transaction are executed against the same iodev without interruption, so the chip select stays asserted for the whole sequence, and the transaction produces a single CQE.
This replaces the multi-buffer :c:struct:`spi_buf_set` scatter-gather model of the traditional API.

The following sends a two-part transmit buffer (header followed by payload) without copying into a single contiguous region:

.. code-block:: c

   static uint8_t header[2]   = { 0xAB, 0x01 };
   static uint8_t payload[16] = { ... };

   struct rtio_sqe *sqe;
   struct rtio_cqe *cqe;
   int err;

   /* First part: mark it as part of a transaction */
   sqe = rtio_sqe_acquire(&spi_rtio_ctx);
   rtio_sqe_prep_write(sqe, &spi_iodev, RTIO_PRIO_NORM,
                       header, sizeof(header), NULL);
   sqe->flags |= RTIO_SQE_TRANSACTION;

   /* Second (final) part of the transaction */
   sqe = rtio_sqe_acquire(&spi_rtio_ctx);
   rtio_sqe_prep_write(sqe, &spi_iodev, RTIO_PRIO_NORM,
                       payload, sizeof(payload), NULL);

   rtio_submit(&spi_rtio_ctx, 1);

   cqe = rtio_cqe_consume(&spi_rtio_ctx);
   err = cqe->result;
   rtio_cqe_release(&spi_rtio_ctx, cqe);

Set the :c:macro:`RTIO_SQE_TRANSACTION` flag on every SQE in the sequence except the last one.

Asynchronous (non-blocking) transfer
=====================================

The primary advantage of the RTIO backend is that submission and completion are decoupled.
An application can submit a transfer without waiting, continue doing other work, and check or wait for the CQE later.
Submit with a wait count of ``0`` so :c:func:`rtio_submit` returns immediately:

.. code-block:: c

   struct rtio_sqe *sqe;
   struct rtio_cqe *cqe;
   int err;

   /* Prepare and submit without blocking */
   sqe = rtio_sqe_acquire(&spi_rtio_ctx);
   rtio_sqe_prep_transceive(sqe, &spi_iodev, RTIO_PRIO_NORM,
                            tx_data, rx_data, sizeof(tx_data), NULL);
   rtio_submit(&spi_rtio_ctx, 0);

   /* ... do other work ... */

   /* Block until the transfer completes */
   cqe = rtio_cqe_consume_block(&spi_rtio_ctx);
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

Independent operations can be queued as a chain using the :c:macro:`RTIO_SQE_CHAINED` flag.
Chained SQEs are executed in order, and each one produces its own CQE.
If an operation in the chain fails, the remaining chained operations are cancelled (failure cascading).
This is useful for command and response sequences, such as writing a command and then reading the response:

.. code-block:: c

   struct rtio_sqe *sqe;

   /* First transfer: write a command */
   sqe = rtio_sqe_acquire(&spi_rtio_ctx);
   rtio_sqe_prep_write(sqe, &spi_iodev, RTIO_PRIO_NORM,
                       cmd_data, sizeof(cmd_data), NULL);
   sqe->flags |= RTIO_SQE_CHAINED;

   /* Second transfer: read the response */
   sqe = rtio_sqe_acquire(&spi_rtio_ctx);
   rtio_sqe_prep_read(sqe, &spi_iodev, RTIO_PRIO_NORM,
                      resp_data, sizeof(resp_data), NULL);

   /* Submit both operations and wait for both completions */
   rtio_submit(&spi_rtio_ctx, 2);

   /* Consume one CQE per chained operation */
   for (int i = 0; i < 2; i++) {
       struct rtio_cqe *cqe = rtio_cqe_consume(&spi_rtio_ctx);
       if (cqe->result < 0) { ... }
       rtio_cqe_release(&spi_rtio_ctx, cqe);
   }

Set the :c:macro:`RTIO_SQE_CHAINED` flag on every SQE in the chain except the last one.

.. note::

   Within a chain, execution order is preserved and a failure cancels the subsequent operations.
   CQEs from independent submissions are not guaranteed to be completed in submission order.

Buffer requirements
===================

The nRF SPIM peripheral moves data using EasyDMA, which places two constraints on the transmit and receive buffers passed to the RTIO submission helpers:

* Maximum transfer size

  A single EasyDMA transfer is limited by the peripheral's ``MAXCNT`` register, whose width is SoC-specific.
  The driver automatically splits larger transfers into chunks that fit this limit, so no action is required from the application beyond being aware that a large transfer is carried out as several DMA operations.

* Memory location

  EasyDMA can only access buffers located in specific memory regions.
  Buffers placed in flash (for example ``const`` data) or in RAM that is not reachable by EasyDMA cannot be used directly.

How buffers outside the EasyDMA-accessible region are handled depends on the SoC:

* On SoCs without Device Memory Management (DMM), the driver copies the data through an internal RAM bounce buffer whose size is set by ``CONFIG_SPI_NRFX_RAM_BUFFER_SIZE`` (default ``8`` bytes per driver instance, applied to both the TX and RX paths).
  Setting this option to ``0`` disables bounce buffering, in which case the application must ensure that every buffer is directly accessible by EasyDMA, otherwise the transfer fails.

* On SoCs with Device Memory Management (DMM), the driver allocates the buffers from the DMM memory region and splits transfers into chunks limited by ``CONFIG_SPI_NRFX_DMM_BUFFER_SIZE`` (default ``256`` bytes). For a full-duplex transfer, twice the chunk size is allocated.

Bounce buffering and DMM re-allocation add a data copy on every affected transfer.
For the best performance, statically place the transmit and receive buffers in the memory region that EasyDMA can access for the given SPI controller, with the alignment required by the underlying DMA hardware.
The Nordic Device Memory Management (DMM) subsystem provides the ``DMM_MEMORY_SECTION(node_id)`` variable attribute in ``<dmm.h>`` for exactly this purpose.
It places a statically allocated buffer in the memory region associated with the given devicetree node and applies the alignment that the region requires.
Pass the SPI controller node (obtained with ``DT_BUS()`` from the device node) so that the buffers are placed in the region associated with that controller:

.. code-block:: c

   #include <dmm.h>

   #define MY_SPI_NODE DT_NODELABEL(my_device)

   static uint8_t tx_data[4] DMM_MEMORY_SECTION(DT_BUS(MY_SPI_NODE));
   static uint8_t rx_data[4] DMM_MEMORY_SECTION(DT_BUS(MY_SPI_NODE));

Buffers declared this way are already in the correct region and alignment, so neither bounce buffering nor DMM re-allocation is needed at transfer time.
On SoCs that do not associate a dedicated memory region with the controller, ``DMM_MEMORY_SECTION`` expands to a no-op, so the same code remains portable across nRF devices.

Refer to the respective Product Specification for the exact memory regions, alignment, and ``MAXCNT`` limits.
