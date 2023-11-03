.. _gzll_integration_notes:

Integration notes
#################

.. contents::
   :local:
   :depth: 2

To integrate the Gazell Link Layer library in your application, you need to:

* Link in the GZLL library. See the :ref:`gzll_configuration` section.
* Supply the glue code used by the library. See the :ref:`gzll_glue_layer` section.

RTOS
****

Gazell Link Layer API is not reentrant.
It should be called by only a single thread in an RTOS.

.. _gzll_configuration:

Configuration
*************

In the |NCS|, you can enable the GZLL library using the :kconfig:option:`CONFIG_GZLL` Kconfig option.
Look for the menu item "Enable Gazell Link Layer".
The build system will link in the appropriate library for your SoC.

.. _gzll_glue_layer:

Glue layer
**********

The glue layer lets you to select the hardware resources for Gazell.

Radio
=====

Gazell accesses directly the 2.4 GHz radio peripheral.

When the 2.4 GHz radio makes an interrupt request, the glue function :c:func:`nrf_gzll_radio_irq_handler` needs to be called for Gazell processing.

Timer
=====

Gazell requires a timer peripheral for timing purposes.
It accesses directly the timer instance provided by the :c:var:`nrf_gzll_timer` variable.
It consults the :c:var:`nrf_gzll_timer_irqn` variable for the interrupt number of the timer.

When the timer makes an interrupt request, the glue function :c:func:`nrf_gzll_timer_irq_handler` needs to be called for Gazell processing.

Software interrupt
==================

Gazell consults the :c:var:`nrf_gzll_swi_irqn` variable for the software interrupt number to use.

When the software interrupt is triggered, the glue function :c:func:`nrf_gzll_swi_irq_handler` needs to be called for Gazell processing.

PPI channels
============

For the nRF52 Series, Gazell takes three PPI channels.
It consults the following variables for the PPI channel numbers, event end points (EEP) and task end points (TEP):

* :c:var:`nrf_gzll_ppi_eep0`
* :c:var:`nrf_gzll_ppi_tep0`
* :c:var:`nrf_gzll_ppi_eep1`
* :c:var:`nrf_gzll_ppi_tep1`
* :c:var:`nrf_gzll_ppi_eep2`
* :c:var:`nrf_gzll_ppi_tep2`
* :c:var:`nrf_gzll_ppi_chen_msk_0_and_1`
* :c:var:`nrf_gzll_ppi_chen_msk_2`

DPPI channels
=============

For the nRF53 Series, Gazell takes three DPPI channels.
It consults the following variables for the DPPI channel numbers:

* :c:var:`nrf_gzll_dppi_ch0`
* :c:var:`nrf_gzll_dppi_ch1`
* :c:var:`nrf_gzll_dppi_ch2`
* :c:var:`nrf_gzll_dppi_chen_msk_0_and_1`
* :c:var:`nrf_gzll_dppi_chen_msk_2`

High frequency clock
====================

You can configure Gazell to automatically switch on and off the high frequency oscillator (:c:enumerator:`NRF_GZLL_XOSC_CTL_AUTO`).
It calls the following glue functions for high frequency clock requests:

* :c:func:`nrf_gzll_request_xosc`
* :c:func:`nrf_gzll_release_xosc`

Microseconds delay
==================

Gazell calls the glue function :c:func:`nrf_gzll_delay_us` to delay a number of microseconds.
