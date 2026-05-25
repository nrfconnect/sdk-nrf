.. _spim_peripheral:

SPIM
####

.. contents::
   :local:
   :depth: 2

The SPI controller peripheral (SPIM) provides a full duplex, 4-wire synchronous serial communication interface.

You can find more details about SPIM hardware peripheral in the respective Product Specification:

* `nRF52805 SPIM`_
* `nRF52810 SPIM`_
* `nRF52811 SPIM`_
* `nRF52832 SPIM`_
* `nRF52833 SPIM`_
* `nRF52840 SPIM`_
* `nRF5340 SPIM`_
* `nRF9151 SPIM`_
* `nRF9160 SPIM`_
* `nRF9161 SPIM`_
* `nRF54L15 SPIM`_
* `nRF54LM20A SPIM`_
* `nRF54LV10A SPIM`_

Related software components
***************************

Here you can find a list of software components that can be used to interact with the SPIM peripheral:

.. list-table::
   :header-rows: 1

   * - Component
     - Link
     - Description
   * - `spi` Zephyr driver
     - `Zephyr's SPI driver API`_
     - general-purpose, standardized Zephyr driver API for SPI peripherals
   * - `spi_rtio` Zephyr driver
     - `Zephyr's SPI RTIO driver API`_
     - general-purpose, standardized Zephyr driver API utilizing RTIO framework for SPI peripherals
   * - `sensor` driver
     - `Zephyr's sensor driver API`_
     - general-purpose, standardized Zephyr driver API for interacting with external sensors

Reference code
**************

Here you can find a list of samples or tests that utilize SPIM peripheral and can be used as a usage reference:

.. list-table::
   :header-rows: 1

   * - Application
     - Link
     - Comment
   * - Zephyr `spi` driver twister loopback test
     - `Zephyr's SPI driver twister test`_
     - Test utilizing `Zephyr's SPI driver API`_ to send data between MOSI and MISO pins connected using jumper wire
   * - Zephyr `sensor` driver sample for pressure sensor
     - `Zephyr's pressure sensor sample`_
     - Sample utilizing `Zephyr's sensor driver API`_ to communicate with external pressure sensor over SPI bus.

Usage guides
************

Here you can find a description on how the peripheral hardware capabilities are mapped into software component interfaces and what are their limitations.

.. toctree::
   :maxdepth: 1
   :caption: Subpages:
   :glob:

   spim_zephyr_spi
   spim_zephyr_spi_rtio
