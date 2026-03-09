.. _spim_peripheral:

SPIM
####

.. contents::
   :local:
   :depth: 2

The SPI controller peripheral (SPIM) provides a full duplex, 4-wire synchronous serial communication interface.

The main hardware features of SPIM are the following:

* DMA support to and from RAM without CPU intervention
* SPI modes 0-3
* individual selection of I/O pins (with some limitations, depending on the SoC)
* optional D/CX output line for distinguishing between command and data bytes
* optional hardware controlled chip select (CSN)

Features
********

Not all hardware features are available on every nRF SoC.
Not all hardware features are supported in software drivers.

Here you can find matrix confronting hardware feature presence and software driver support.

.. list-table::
   :header-rows: 1

   * - SoC
     - >8 MHz SCK frequency
     - D/CX and CSN lines
     - Pattern matching
   * - | nRF52820
       | nRF52832
     - --
     - --
     - --
   * - | nRF52833
       | nRF52840
     - | Zephyr
       | nrfx
       | SPIM3 only
     - | nrfx
       | SPIM3 only
     - --
   * - nRF5340
     - | Zephyr
       | nrfx
       | SPIM4 only
     - | nrfx
       | SPIM4 only
     - --
   * - nRF91 Series
     - --
     - --
     - --
   * - | nRF54L05
       | nRF54L10
       | nRF54L15
       | nRF54LM20
       | nRF54LV10
     - | Zephyr
       | nrfx
       | SPIM00 only
     - nrfx
     - --

Related software components
***************************

Here you can find a list of software components that can be used to interact with the SPIM peripheral:

.. list-table::
   :header-rows: 1

   * - Component
     - Link
     - Description
   * - `nrfx_spim` driver
     - `nrfx SPIM driver`_
     - general-purpose, Nordic-specific, low-level driver API
   * - `spi` Zephyr driver
     - `Zephyr's SPI driver API`_
     - general-purpose, standardized Zephyr driver API for SPI peripherals
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
   * - nrfx SPIM loopback sample
     - `nrfx SPIM sample`_
     - Sample utilizing `nrfx SPIM driver`_ to send data between MOSI and MISO pins connected using jumper wire
   * - Zephyr driver `spi` twister loopback test
     - `Zephyr's SPI driver twister test`_
     - Sample utilizing `Zephyr's SPI driver API`_ to send data between MOSI and MISO pins connected using jumper wire

Usage guides
************

Here you can find a description on how the peripheral hardware capabilities are mapped into software component interfaces and what are their limitations.

.. toctree::
   :maxdepth: 1
   :caption: Subpages:
   :glob:

   spim_nrfx
   spim_zephyr_spi
