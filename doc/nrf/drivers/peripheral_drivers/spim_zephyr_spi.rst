.. _spim_zephyr_spi:

SPIM Zephyr driver
##################

The Zephyr SPI driver provides a standardized, general-purpose API for communicating with SPI peripherals on nRF SoCs.

For the complete API reference, see `Zephyr's SPI driver API`_.

This page describes the common usage scenarios covered by the driver API, including static and runtime configuration, chip select management, and different data transfer modes.

Configuration
*************

Configuring the Zephyr SPI driver involves three parts: enabling the driver through Kconfig, describing the bus and connected devices statically in the devicetree, and setting the parameters for each transfer at runtime in the application code.

Static configuration
====================

The Zephyr SPI driver is configured statically through the devicetree.
Before the driver can be used at runtime, the corresponding devicetree node must be enabled and its properties set according to the desired operating mode.

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

The ``pinctrl-0`` property references a pin control state defined in the board or application overlay, per `Zephyr's pinctrl pin configuration`_.
The ``cs-gpios`` property specifies the GPIO pin used for chip select control.
Each SPI peripheral device on the bus is represented as a child node, with the ``reg`` property indicating its chip select index.

Kconfig configuration
=====================

To enable the Zephyr SPI driver for nRF SoCs, set the following Kconfig option:

.. code-block:: kconfig

   CONFIG_SPI=y

Depending on the specific nRF SoC and SPIM instance used, the appropriate hardware-specific driver backend is selected automatically by Zephyr's build system.

Runtime configuration
=====================

At runtime, SPI transfers are configured using the :c:struct:`spi_config` structure.
This structure defines the operational parameters for each transfer, such as frequency, operation mode flags, and slave select.

To obtain a reference to the SPI device and prepare the configuration:

.. code-block:: c

   #include <zephyr/drivers/spi.h>
   #include <zephyr/devicetree.h>

   #define MY_SPI_NODE DT_NODELABEL(my_device)

   static const struct device *spi_dev = DEVICE_DT_GET(DT_BUS(MY_SPI_NODE));

   static const struct spi_config spi_cfg = {
       .frequency = DT_PROP(MY_SPI_NODE, spi_max_frequency),
       .operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_MODE_CPOL | SPI_MODE_CPHA,
       .slave = DT_REG_ADDR(MY_SPI_NODE),
       .cs = SPI_CS_CONTROL_INIT(DT_NODELABEL(my_device), 2),
   };

   int main(void) {
       if (!device_is_ready(spi_dev)) {
           return -ENODEV;
       }
   }

The ``operation`` field is composed of OR'ed bitmasks defined in ``<zephyr/drivers/spi.h>``, specifying word size, bit order, and SPI mode (clock polarity and phase).

The above approach requires manual population of :c:struct:`spi_config` from devicetree macros.
The preferred modern approach is to use :c:struct:`spi_dt_spec`, which bundles the device reference and :c:struct:`spi_config` into a single struct populated at build time from devicetree using the :c:macro:`SPI_DT_SPEC_GET` macro:

.. code-block:: c

   #include <zephyr/drivers/spi.h>
   #include <zephyr/devicetree.h>

   #define MY_SPI_NODE DT_NODELABEL(my_device)

   static const struct spi_dt_spec spi_spec =
       SPI_DT_SPEC_GET(MY_SPI_NODE, SPI_WORD_SET(8) | SPI_TRANSFER_MSB, 0);

   int main(void) {
       if (!spi_is_ready_dt(&spi_spec)) {
           return -ENODEV;
       }
   }

The second argument to :c:macro:`SPI_DT_SPEC_GET` is the ``operation`` bitmask, and the third is an optional delay in microseconds applied after asserting chip select.
Frequency and chip select configuration are derived automatically from the devicetree node.
When using :c:struct:`spi_dt_spec`, the corresponding :c:func:`spi_transceive_dt` API function should be used in place of :c:func:`spi_transceive`, passing the spec directly:

.. code-block:: c

   err = spi_transceive_dt(&spi_spec, &tx_set, &rx_set);

Chip select management
======================

The Zephyr SPI driver controls the chip select signal automatically as part of each transfer, using the GPIO specified in the ``cs-gpios`` devicetree property.
The :c:macro:`SPI_CS_CONTROL_INIT` macro initializes the :c:struct:`spi_cs_control` structure from devicetree, including the GPIO device reference, pin, flags, and delay.

If manual chip select control is required, the ``cs`` field of :c:struct:`spi_config` may be set to ``{0}`` and the chip select GPIO managed explicitly by the application using the GPIO driver API.

Data transfer
*************

After completing device and configuration initialization, the driver is ready to perform transfers using the :c:func:`spi_transceive` function, or its blocking and asynchronous variants.

Each transfer is described by two sets of :c:struct:`spi_buf_set` structures: one for the transmit buffers and one for the receive buffers.
Each :c:struct:`spi_buf_set` points to an array of :c:struct:`spi_buf` descriptors, allowing scatter-gather transfers composed of multiple non-contiguous memory regions.

See `Buffer requirements`_ for the constraints that the nRF SPIM peripheral imposes on the transfer buffers.

Basic TX-RX transfer
====================

To perform a simultaneous transmit and receive transfer, provide both a transmit and a receive :c:struct:`spi_buf_set` to :c:func:`spi_transceive`:

.. code-block:: c

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
       int err;

       err = spi_transceive(spi_dev, &spi_cfg, &tx_set, &rx_set);
       if (err < 0) {
           ...
       }
   }

The call blocks until the transfer completes.
Passing ``NULL`` for either ``tx_bufs`` or ``rx_bufs`` results in a transmit-only or receive-only transfer, respectively.

Scatter-gather transfer
=======================

The :c:struct:`spi_buf_set` abstraction supports scatter-gather transfers, where the data to be transmitted or received is spread across multiple non-contiguous buffers.
This is useful when sending a header followed by a variable-length payload without copying the data into a single contiguous buffer:

.. code-block:: c

   static uint8_t header[2] = { 0xAB, 0x01 };
   static uint8_t payload[16] = { ... };

   static const struct spi_buf tx_bufs[] = {
       { .buf = header,  .len = sizeof(header)  },
       { .buf = payload, .len = sizeof(payload) },
   };

   static const struct spi_buf_set tx_set = { .buffers = tx_bufs, .count = 2 };

   err = spi_transceive(spi_dev, &spi_cfg, &tx_set, NULL);

Asynchronous transfer
=====================

For non-blocking operation, use :c:func:`spi_transceive_cb`, which accepts a callback function invoked upon transfer completion:

.. code-block:: c

   void spi_callback(const struct device *dev, int result, void *data) {
       if (result < 0) {
           ...
       }
       /* Transfer complete — process rx_data here */
   }

   int main(void) {
       err = spi_transceive_cb(spi_dev, &spi_cfg, &tx_set, &rx_set,
                               spi_callback, NULL);
       if (err < 0) {
           ...
       }
   }

.. caution::

   Asynchronous transfer support depends on Zephyr's asynchronous SPI backend being enabled for the target SoC.
   Enable it with the following Kconfig option:

   .. code-block:: kconfig

      CONFIG_SPI_ASYNC=y

.. caution::

   The ``spi_callback`` function is invoked from IRQ context.
   Only ISR-safe Zephyr kernel APIs may be used inside the callback.
   :c:func:`k_sem_give` is ISR-safe and can be used to signal a waiting thread.
   The calling thread can then block on :c:func:`k_sem_take` after initiating the transfer and proceed once the callback has signalled completion:

   .. code-block:: c

      #include <zephyr/kernel.h>

      static K_SEM_DEFINE(spi_done, 0, 1);

      void spi_callback(const struct device *dev, int result, void *data) {
          k_sem_give(&spi_done);
      }

      int main(void) {
          err = spi_transceive_cb(spi_dev, &spi_cfg, &tx_set, &rx_set,
                                  spi_callback, NULL);
          if (err < 0) {
              ...
          }

          k_sem_take(&spi_done, K_FOREVER);
          /* Transfer complete — process rx_data here */
      }

Buffer requirements
===================

The nRF SPIM peripheral moves data using EasyDMA, which places two constraints on the buffers referenced by the :c:struct:`spi_buf` descriptors:

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
