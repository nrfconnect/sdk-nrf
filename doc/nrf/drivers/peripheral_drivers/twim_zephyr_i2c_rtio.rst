.. _twim_zephyr_i2c_rtio:

TWIM Zephyr RTIO driver
#######################

The RTIO-based Zephyr I2C driver provides a native, non-blocking API for communicating with I2C peripherals on nRF SoCs, built on top of Zephyr's :ref:`RTIO subsystem <rtio>`.

For the complete API reference, see `Zephyr's I2C driver API`_ and `Zephyr's RTIO subsystem`_.

This page describes the common usage scenarios covered by the RTIO-based API, including configuration, data transfer modes and transfer chaining.

Overview
********

The RTIO (Real-Time I/O) driver backend for the nRF TWIM peripheral provides a native, non-blocking I/O path built on top of Zephyr's :ref:`RTIO subsystem <rtio>`.
Instead of the traditional :c:func:`i2c_transfer` call model, applications submit I/O requests to a submission queue (SQ) and later consume results from a completion queue (CQ).

This backend is selected automatically when ``CONFIG_I2C_RTIO=y`` is set and the ``nordic,nrf-twim`` compatible is present in the devicetree.
It is mutually exclusive with the standard I2C TWIM backend:

* I2C TWIM backend — selected when ``CONFIG_I2C_RTIO`` is **not** set.
* I2C TWIM RTIO backend — selected when ``CONFIG_I2C_RTIO=y`` is set.

Configuration
*************

Static configuration
====================

The devicetree configuration is identical to the standard TWIM driver.
A typical I2C controller node in an application overlay looks as follows:

.. code-block:: devicetree

    &pinctrl {
        i2c20_default: i2c20_default {
            group1 {
                psels = <NRF_PSEL(TWIM_SDA, 0, 4)>,
                        <NRF_PSEL(TWIM_SCL, 0, 3)>;
                bias-pull-up;
            };
        };
    };

    &i2c20 {
        compatible = "nordic,nrf-twim";
        status = "okay";
        pinctrl-0 = <&i2c20_default>;
        pinctrl-names = "default";
        clock-frequency = <I2C_BITRATE_STANDARD>;

        zephyr,concat-buf-size = <16>;
        zephyr,flash-buf-max-size = <16>;

        my_device: my-device@0 {
            compatible = "vnd,my-i2c-device";
            reg = <0x55>;
            status = "okay";
        };
    };

No additional devicetree properties are required to enable the RTIO backend.

Kconfig configuration
=====================

Enable the RTIO subsystem and the I2C RTIO support:

.. code-block:: kconfig

    CONFIG_RTIO=y
    CONFIG_I2C=y
    CONFIG_I2C_RTIO=y

When ``CONFIG_I2C_RTIO=y`` is set, the build system automatically selects I2C TWIM RTIO backend for nRF SoCs with a ``nordic,nrf-twim`` devicetree node enabled.

Two additional Kconfig options control the sizes of the per-device RTIO context that the driver uses internally to service the legacy :c:func:`i2c_transfer` API when ``CONFIG_I2C_RTIO`` is enabled:

.. code-block:: kconfig

    # Number of submission queue entries (SQEs) per TWIM RTIO context (default: 4)
    CONFIG_I2C_RTIO_SQ_SIZE=4

    # Number of completion queue entries (CQEs) per TWIM RTIO context (default: 4)
    CONFIG_I2C_RTIO_CQ_SIZE=4

These options apply only to that internal context.
Increase them if the application submits more than four in-flight requests to a single I2C device through the legacy :c:func:`i2c_transfer` API.
When using the RTIO API directly, the number of in-flight requests is instead bounded by the submission queue entries (SQEs) allocated by the application's own :c:macro:`RTIO_DEFINE`, described in the `Runtime configuration`_ section.

Runtime configuration
=====================

The I2C iodev and the RTIO context are set up at build time using :c:macro:`I2C_DT_IODEV_DEFINE` and :c:macro:`RTIO_DEFINE`.
The :c:macro:`I2C_DT_IODEV_DEFINE` macro binds a devicetree I2C device node to an iodev object, including frequency derived from the devicetree:


.. code-block:: c

    #include <zephyr/drivers/i2c.h>
    #include <zephyr/rtio/rtio.h>
    #include <zephyr/devicetree.h>

    #define MY_I2C_NODE DT_NODELABEL(my_device)

    /* Define the iodev for the I2C device node */
    I2C_DT_IODEV_DEFINE(i2c_iodev, MY_I2C_NODE);

    /* Define an RTIO context with 4 SQE slots and 4 CQE slots */
    RTIO_DEFINE(i2c_rtio_ctx, 4, 4);

    int main(void) {
        if (!i2c_is_ready_iodev(&i2c_iodev)) {
            return -ENODEV;
        }
    }

Frequency is derived automatically from the bus devicetree node.

:c:func:`i2c_is_ready_iodev` checks that the I2C controller device is ready for use.

Data transfer
*************

Transfers are performed by acquiring submission queue entries (SQEs) from the RTIO context, preparing them against the I2C iodev, submitting them, and then consuming completion queue events (CQEs) to determine the result.

The SQEs are prepared using the generic submission helpers from Zephyr's :ref:`RTIO subsystem <rtio>`:

* :c:func:`rtio_sqe_prep_write` for a transmit-only transfer.
* :c:func:`rtio_sqe_prep_read` for a receive-only transfer.

Additionally, :c:func:`rtio_sqe_prep_tiny_write` can be used to write operations of at most 7 bytes.
Contrary to :c:func:`rtio_sqe_prep_write`, buffer supplied to :c:func:`rtio_sqe_prep_tiny_write` does not need to stay valid until transfer completion.
It is especially useful for write-read operations, typical for I2C uses.

Each helper takes a plain buffer pointer and its length, identical to regular I2C API.

See `Buffer requirements`_ for the constraints that the nRF TWIM peripheral imposes on the transfer buffers.

Basic write or read operations
==============================

To perform write or read operations, provide data buffer and its length to :c:func:`rtio_sqe_prep_write` or :c:func:`rtio_sqe_prep_read`, submit it, and wait for the completion:
Subsequent write and read operations can be prepared by chaining SQEs with the :c:macro:`RTIO_SQE_TRANSACTION` flag. The transaction produces a single CQE.
Set the :c:macro:`RTIO_SQE_TRANSACTION` flag on every SQE in the sequence except the last one.

The following performs a typical I2C write-read operation :

.. code-block:: c

    #include <zephyr/drivers/i2c.h>
    #include <zephyr/rtio/rtio.h>

    static uint8_t reg_addr = 0x10;
    static uint8_t data[4];

    int main(void) {
        struct rtio_sqe *sqe;
        struct rtio_cqe *cqe;
        int err;

        /* Write register address, then read (one I2C transaction, one CQE) */

        sqe = rtio_sqe_acquire(&i2c_rtio_ctx);
        if (sqe == NULL) {
            return -ENOMEM;
        }
        rtio_sqe_prep_write(sqe, &i2c_iodev, RTIO_PRIO_NORM,
                                &reg_addr, sizeof(reg_addr), NULL);
        sqe->flags |= RTIO_SQE_TRANSACTION;

        sqe = rtio_sqe_acquire(&i2c_rtio_ctx);
        if (sqe == NULL) {
            return -ENOMEM;
        }
        rtio_sqe_prep_read(sqe, &i2c_iodev, RTIO_PRIO_NORM,
                            data, sizeof(data), NULL);
        sqe->iodev_flags |= RTIO_IODEV_I2C_STOP | RTIO_IODEV_I2C_RESTART;

        rtio_submit(&i2c_rtio_ctx, 1);

        cqe = rtio_cqe_consume(&i2c_rtio_ctx);
        err = cqe->result;
        rtio_cqe_release(&i2c_rtio_ctx, cqe);

        if (err < 0) {
            /* Transfer failed */
        }

        /* data now contains the register value(s) */
        return 0;
    }

The second argument to :c:func:`rtio_submit` is the number of completions to wait for before returning; passing ``0`` submits without blocking.

Asynchronous (non-blocking) transfer
====================================

The primary advantage of the RTIO backend is that submission and completion are decoupled. This essentially is the only way to achieve non-blocking I2C transfer with nRF TWIM backend.
An application can submit a transfer without waiting, continue doing other work, and check or wait for the CQE later.
Submit with a wait count of ``0`` so :c:func:`rtio_submit` returns immediately:

.. code-block:: c

    struct rtio_sqe *sqe;
    struct rtio_cqe *cqe;
    int err;

    /* Prepare and submit without blocking */
    sqe = rtio_sqe_acquire(&i2c_rtio_ctx);
    rtio_sqe_prep_read(sqe, &i2c_iodev, RTIO_PRIO_NORM,
                            data, sizeof(data), NULL);
    rtio_submit(&i2c_rtio_ctx, 0);

    /* ... do other work ... */

    /* Block until the transfer completes */
    cqe = rtio_cqe_consume_block(&i2c_rtio_ctx);
    err = cqe->result;
    rtio_cqe_release(&i2c_rtio_ctx, cqe);

    if (err < 0) {
        /* Transfer failed */
    }
    /* data now contains received bytes */

To poll for completion without blocking, use :c:func:`rtio_cqe_consume` instead, which returns ``NULL`` if no CQE is available yet:

.. code-block:: c

    struct rtio_cqe *cqe = rtio_cqe_consume(&i2c_rtio_ctx);
    if (cqe != NULL) {
        err = cqe->result;
        rtio_cqe_release(&i2c_rtio_ctx, cqe);
    }

Chaining multiple transfers
============================

Independent operations can be queued as a chain using the :c:macro:`RTIO_SQE_CHAINED` flag.
Chained SQEs are executed in order, and each one produces its own CQE.
If an operation in the chain fails, the remaining chained operations are cancelled (failure cascading).

Do not use chaining for a write-read operations. Use :c:macro:`RTIO_SQE_TRANSACTION` for that pattern, as in `Basic write or read operations`_.
Chaining is useful when the application wants several distinct writes (or reads) submitted once and completed in order. For example, programming two registers:

.. code-block:: c

    static uint8_t reg0[2] = { 0x01, 0x00 };
    static uint8_t reg1[2] = { 0x02, 0x01 };

    struct rtio_sqe *sqe;

    /* First I2C write (full transaction) */
    sqe = rtio_sqe_acquire(&i2c_rtio_ctx);
    rtio_sqe_prep_write(sqe, &i2c_iodev, RTIO_PRIO_NORM,
                        reg0, sizeof(reg0), NULL);
    sqe->iodev_flags |= RTIO_IODEV_I2C_STOP;
    sqe->flags |= RTIO_SQE_CHAINED;

    /* Second I2C write (full transaction) */
    sqe = rtio_sqe_acquire(&i2c_rtio_ctx);
    rtio_sqe_prep_write(sqe, &i2c_iodev, RTIO_PRIO_NORM,
                        reg1, sizeof(reg1), NULL);
    sqe->iodev_flags |= RTIO_IODEV_I2C_STOP;

    /* Submit both operations and wait for both completions */
    rtio_submit(&i2c_rtio_ctx, 2);

    /* Consume one CQE per chained operation */
    for (int i = 0; i < 2; i++) {
        struct rtio_cqe *cqe = rtio_cqe_consume(&i2c_rtio_ctx);
        if (cqe->result < 0) { ... }
        rtio_cqe_release(&i2c_rtio_ctx, cqe);
    }

Set the :c:macro:`RTIO_SQE_CHAINED` flag on every SQE in the chain except the last one.

.. note::

    Within a chain, execution order is preserved and a failure cancels the subsequent operations.
    CQEs from independent submissions are not guaranteed to be completed in submission order.

Buffer requirements
===================

The nRF TWIM peripheral moves data using EasyDMA, which places two constraints on the buffers referenced by the I2C API descriptors:

* Maximum transfer size

  A single EasyDMA transfer is limited by the peripheral's ``MAXCNT`` register, whose width is SoC-specific.
  If a transfer segment exceeds this limit, the driver returns ``-ENOSPC``. Keep each ``i2c_msg`` length within the limit.

* Memory location

  EasyDMA can only access buffers located in specific memory regions.
  Buffers placed in flash (for example ``const`` data) or in RAM that is not reachable by EasyDMA cannot be used directly.

How buffers outside the EasyDMA-accessible region are handled depends on the SoC:

* On SoCs without Device Memory Management (DMM), the driver copies the data through an internal RAM bounce buffer whose size is set on the TWIM devicetree node by ``zephyr,concat-buf-size`` for adjacent ``i2c_msg`` chunks and ``zephyr,flash-buf-max-size`` for copying data from flash (default 16 bytes each).
  Setting both properties to ``0`` disables bounce buffering, in which case the application must ensure that every buffer is directly accessible by EasyDMA, otherwise the transfer fails.

* On SoCs with Device Memory Management (DMM), the driver allocates the buffers from the DMM memory region.

Bounce buffering and DMM re-allocation add a data copy on every affected transfer.
For the best performance, statically place the transmit and receive buffers in the memory region that EasyDMA can access for the given I2C controller, with the alignment required by the underlying DMA hardware.
The Nordic Device Memory Management (DMM) subsystem provides the ``DMM_MEMORY_SECTION(node_id)`` variable attribute in ``<dmm.h>`` for exactly this purpose.
It places a statically allocated buffer in the memory region associated with the given devicetree node and applies the alignment that the region requires.
Pass the I2C controller node (obtained with ``DT_BUS()`` from the device node) so that the buffers are placed in the region associated with that controller:

.. code-block:: c

    #include <dmm.h>

    #define MY_I2C_NODE DT_NODELABEL(my_device)

    static uint8_t data[4] DMM_MEMORY_SECTION(DT_BUS(MY_I2C_NODE));

Buffers declared this way are already in the correct region and alignment, so neither bounce buffering nor DMM re-allocation is needed at transfer time.
On SoCs that do not associate a dedicated memory region with the controller, ``DMM_MEMORY_SECTION`` expands to a no-op, so the same code remains portable across nRF devices.

Refer to the respective Product Specification for the exact memory regions, alignment, and ``MAXCNT`` limits.
