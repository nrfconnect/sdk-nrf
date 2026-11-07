.. _twim_zephyr_i2c:

TWIM Zephyr driver
##################

The Zephyr I2C driver provides a standardized, general-purpose API for communicating with I2C peripherals on nRF SoCs.

For the complete API reference, see `Zephyr's I2C driver API`_.

This page describes the common usage scenarios covered by the driver API, including static and runtime configuration and different data transfer modes.

Configuration
*************

Configuring the Zephyr I2C driver involves three parts: enabling the driver through Kconfig, describing the bus and connected devices statically in the devicetree, and setting the parameters for each transfer at runtime in the application code.

Static configuration
====================

The Zephyr I2C driver is configured statically through the devicetree.
Before the driver can be used at runtime, the corresponding devicetree node must be enabled and its properties set according to the desired operating mode.

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

        my_device: my-device@55 {
            compatible = "vnd,my-i2c-device";
            reg = <0x55>;
            status = "okay";
        };
    };

The ``pinctrl-0`` property references a pin control state defined in the board or application overlay, per `Zephyr's pinctrl pin configuration`_.
The ``clock-frequency`` property specifies the I2C clock frequency in Hz, accepting both predefined ``I2C_BITRATE`` symbols and custom values.
``zephyr,concat-buf-size`` and ``zephyr,flash-buf-max-size`` set the size of the driver's internal bounce buffer (the greater of the two values).
See `Buffer requirements`_ for when it is used.
Each I2C peripheral device on the bus is represented as a child node, with the ``reg`` property indicating its I2C bus address.

Kconfig configuration
=====================

To enable the Zephyr I2C driver for nRF SoCs, set the following Kconfig option:

.. code-block:: kconfig

    CONFIG_I2C=y

Depending on the specific nRF SoC and TWIM instance used, the appropriate hardware-specific driver backend is selected automatically by Zephyr's build system.

Runtime configuration
=====================

By default, I2C driver will use configuration provided in devicetree properties.
At runtime, I2C transfers can be reconfigured using a single bit-packed ``uint32_t`` value, composed of OR'ed bitmasks defined in ``<zephyr/drivers/i2c.h>``, specifying transfer speed.

.. caution::

    Bit-packed configuration value provided by I2C API allows changing I2C bus between controller and peripheral mode.
    TWIM backend **does not** allow that.

To obtain a reference to the I2C device and apply the configuration:

.. code-block:: c

    #include <zephyr/drivers/i2c.h>
    #include <zephyr/devicetree.h>

    #define MY_I2C_NODE      DT_NODELABEL(my_device)
    #define MY_I2C_NODE_ADDR DT_REG_ADDR(MY_I2C_NODE)

    static const struct device *i2c_dev = DEVICE_DT_GET(DT_BUS(MY_I2C_NODE));

    uint32_t i2c_cfg = I2C_SPEED_SET(I2C_SPEED_STANDARD);

    int main(void) {
        if (!device_is_ready(i2c_dev)) {
            return -ENODEV;
        }

        if (i2c_configure(i2c_dev, i2c_cfg)) {
            return -EINVAL;
        }
    }

I2C device bus address is extracted from devicetree using ``DT_REG_ADDR``.
The TWIM backend supports 7-bit addresses only.
10-bit addressing **is not** supported.
The address can be then used to reference the device on bus when starting transfers using I2C API:

The alternative approach is to use :c:struct:`i2c_dt_spec`, which bundles the device reference and bus address into a single struct populated at build time from devicetree using the :c:macro:`I2C_DT_SPEC_GET` macro:

.. code-block:: c

    #include <zephyr/drivers/i2c.h>
    #include <zephyr/devicetree.h>

    #define MY_I2C_NODE DT_NODELABEL(my_device)

    static const struct i2c_dt_spec i2c_spec = I2C_DT_SPEC_GET(MY_I2C_NODE);

    uint32_t i2c_cfg = I2C_SPEED_SET(I2C_SPEED_STANDARD);

    int main(void) {
        if (!i2c_is_ready_dt(&i2c_spec)) {
            return -ENODEV;
        }

        if (i2c_configure_dt(&i2c_spec, i2c_cfg)) {
            return -EINVAL;
        }
    }

Data transfer
*************

After completing device and configuration initialization, the driver is ready to perform transfers using the read and write functions, or their composition into a transfer.

See `Buffer requirements`_ for the constraints that the nRF TWIM peripheral imposes on the transfer buffers.

Basic write or read operations
==============================

To perform write or read operations, provide data buffer and its length to :c:func:`i2c_write` or :c:func:`i2c_read`:

.. code-block:: c

    static uint8_t reg_addr = 0x10;
    static uint8_t data[4];

    if (i2c_write(i2c_dev, &reg_addr, sizeof(reg_addr), MY_I2C_NODE_ADDR)) {
        return -EIO;
    }

    if (i2c_read(i2c_dev, data, sizeof(data), MY_I2C_NODE_ADDR)) {
        return -EIO;
    }

When using :c:struct:`i2c_dt_spec`, the corresponding I2C API functions should be used, passing the spec directly:

.. code-block:: c

    static uint8_t reg_addr = 0x10;
    static uint8_t data[4];

    if (i2c_write_dt(&i2c_spec, &reg_addr, sizeof(reg_addr))) {
        return -EIO;
    }

    if (i2c_read_dt(&i2c_spec, data, sizeof(data))) {
        return -EIO;
    }

The call blocks until the transfer completes.

Transfer operations
===================

Separate write and read operations can be combined.

.. code-block:: c

    static uint8_t reg_addr = 0x10;
    static uint8_t data[4];

    if (i2c_write_read(i2c_dev, MY_I2C_NODE_ADDR,
                       &reg_addr, sizeof(reg_addr),
                       data, sizeof(data))) {
        return -EIO;
    }

Write and read operations can also be assembled into complex transfers described using array of :c:struct:`i2c_msg` objects, consisting of set of buffers with descriptors.

.. code-block:: c

    static uint8_t reg_addr = 0x10;
    static uint8_t data[4];

    struct i2c_msg msgs[2];

    msgs[0].buf = &reg_addr;
    msgs[0].len = sizeof(reg_addr);
    msgs[0].flags = I2C_MSG_WRITE;

    msgs[1].buf = data;
    msgs[1].len = sizeof(data);
    msgs[1].flags = I2C_MSG_READ | I2C_MSG_STOP;

    if (i2c_transfer(i2c_dev, msgs, ARRAY_SIZE(msgs), MY_I2C_NODE_ADDR)) {
        return -EIO;
    }

When using :c:struct:`i2c_dt_spec`, the corresponding :c:func:`i2c_write_read_dt` or :c:func:`i2c_transfer_dt` functions should be used.

Asynchronous operations
=======================

Standard I2C API calls block the caller until the transfer is done.
TWIM backend **does not** support asynchronous I2C API variant selected by ``CONFIG_I2C_CALLBACK``.
To achieve non-blocking I2C transfers, use RTIO API variant
See :ref:`twim_zephyr_i2c_rtio`.

Buffer requirements
===================

The nRF TWIM peripheral moves data using EasyDMA, which places two constraints on the buffers referenced by the I2C API descriptors:

* Maximum transfer size

  A single EasyDMA transfer is limited by the peripheral's ``MAXCNT`` register, whose width is SoC-specific.
  If a transfer segment exceeds this limit, the driver returns ``-ENOSPC``.
  Keep each ``i2c_msg`` length within the limit.

* Memory location

  EasyDMA can only access buffers located in specific memory regions.
  Buffers placed in flash (for example ``const`` data) or in RAM that is not reachable by EasyDMA cannot be used directly.

How buffers outside the EasyDMA-accessible region are handled depends on the SoC:

* On SoCs without Device Memory Management (DMM), the driver copies the data through an internal RAM bounce buffer whose size is set on the TWIM devicetree node by ``zephyr,concat-buf-size`` for adjacent ``i2c_msg`` chunks and ``zephyr,flash-buf-max-size`` for copying data from flash (default 16 bytes each).
  Setting both properties to ``0`` disables bounce buffering, in which case the application must ensure that every buffer is directly accessible by EasyDMA, otherwise the transfer fails.

* On SoCs with Device Memory Management (DMM), the driver allocates the buffers from the DMM memory region.
  See `Nordic DMM API`_ for more details.

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
