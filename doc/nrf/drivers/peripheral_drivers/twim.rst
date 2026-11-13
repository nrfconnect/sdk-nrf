.. _twim_peripheral:

TWIM
####

.. contents::
   :local:
   :depth: 2

The TWI controller peripheral (TWIM) provides a half-duplex, 2-wire synchronous serial communication interface.

You can find more details about the TWIM hardware peripheral in the respective Product Specification documents:

* `nRF52805 TWIM`_
* `nRF52810 TWIM`_
* `nRF52811 TWIM`_
* `nRF52820 TWIM`_
* `nRF52832 TWIM`_
* `nRF52833 TWIM`_
* `nRF52840 TWIM`_
* `nRF5340 TWIM`_
* `nRF9151 TWIM`_
* `nRF9160 TWIM`_
* `nRF9161 TWIM`_
* `nRF54L15 TWIM`_
* `nRF54LM20A TWIM`_
* `nRF54LV10A TWIM`_
* `nRF54LC10A TWIM`_

Related software components
***************************

The following software components can be used to interact with the TWIM peripheral:

.. list-table::
   :header-rows: 1

   * - Component
     - Link
     - Description
   * - ``i2c`` Zephyr driver
     - `Zephyr's I2C driver API`_
     - general-purpose, standardized Zephyr driver API for I2C peripherals
   * - ``i2c_rtio`` Zephyr driver
     - `Zephyr's RTIO subsystem`_
     - general-purpose, standardized Zephyr driver API utilizing RTIO framework for I2C peripherals
   * - ``sensor`` driver
     - `Zephyr's sensor driver API`_
     - general-purpose, standardized Zephyr driver API for interacting with external sensors

Reference code
**************

The following samples and tests use the TWIM peripheral and can serve as a usage reference:

.. list-table::
   :header-rows: 1

   * - Application
     - Comment
   * - `Zephyr's I2C driver twister test`_
     - Test utilizing `Zephyr's I2C driver API`_ to send data between two I2C controllers, one acting as controller and the other as peripheral
   * - `Zephyr's I2C RTIO loopback sample`_
     - Sample utilizing the RTIO-based I2C API to perform transfers between two I2C controllers, one acting as controller and the other as peripheral
   * - `Zephyr's heart rate sensor sample`_
     - Sample utilizing `Zephyr's sensor driver API`_ to communicate with external heart rate and blood oxygen saturation sensor over I2C bus.

Usage guides
************

The following guides describe how the peripheral's hardware capabilities are mapped to software component interfaces, along with their limitations.

.. toctree::
   :maxdepth: 1
   :caption: Subpages:
   :glob:

   twim_zephyr_i2c
   twim_zephyr_i2c_rtio
