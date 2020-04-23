.. _nrf_desktop_constlat:

Constant latency hotfix
#######################

Enable the constant latency hotfix module to use device configuration with constant latency interrupts.
This reduces interrupt propagation time, but increases the power consumption.

Module Events
*************

.. include:: event_propagation.rst
    :start-after: table_constlat_start
    :end-after: table_constlat_end

Refer to the :ref:`nrf_desktop_architecture` for basic information about the event-based communication in the nRF Desktop application.

Configuration
*************

Enable the module with the ``CONFIG_DESKTOP_CONSTLAT_ENABLE`` Kconfig option.

You can set the ``CONFIG_DESKTOP_CONSTLAT_DISABLE_ON_STANDBY`` to disable the constant latency interrupts when the device goes to the low power mode (on ``power_down_event``).
The constant latency interrupts are reenabled on ``wake_up_event``.
