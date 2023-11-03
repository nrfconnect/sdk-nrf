.. _rd_hw_resources:

Hardware resources
##################

.. contents::
   :local:
   :depth: 2

The nRF IEEE 802.15.4 radio driver requires some hardware resources to operate correctly.

.. _rd_hw_resources_peripherals_directly:

Peripherals directly used by the driver
***************************************

The driver uses several peripherals, listed in the :file:`nrfxlib/nrf_802154/driver/src/nrf_802154_peripherals.h` file.

.. _rd_hw_resources_abstraction:

Functionalities provided by the platform abstraction
****************************************************

The driver also uses some peripherals indirectly through platform abstraction API:

* Clock - needed to ensure that the high-frequency clock is enabled during RADIO operations.
* GPIOTE - used by the coexistence interface to abort transmission if PTA denies RF access.
* IRQ - needed for indirect access to interrupt controller to manage interrupts for used peripherals.
* Random - needed to generate random backoff for CSMA-CA.
* Temperature - provides temperature data for RSSI correction.
* Timer:

  * Low frequency - Used by features like CSMA-CA, ACK timeout, or timestamps of received frames.
  * High precision - Used by features like received frame timestamps.

The platform or the application must provide an implementation of each of the driver's platform abstraction APIs.
Example implementations are located in the :file:`nrfxlib/nrf_802154/driver/src/platform` and :file:`nrfxlib/nrf_802154/sl/platform` directories.

.. note::
   For nRF Connect SDK, official implementations are present in the :file:`nrfxlib/nrf_802154/sl/platform` and :file:`zephyr/modules/hal_nordic/nrf_802154` directories.

You can select one of the example implementations for each of the platform abstraction APIs.
You can also implement platform abstraction API in any other module instead of using the provided example.
