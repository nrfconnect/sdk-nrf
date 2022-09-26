.. _mpsl_lib:

Multiprotocol Service Layer library control
###########################################

.. contents::
   :local:
   :depth: 2

The Multiprotocol Service Layer library control methods make it possible to control the initialization and uninitialization of the :ref:`nrfxlib:mpsl` library and all required interrupt handlers.
Uninitializing MPSL allows the application to have full control over the `RADIO_IRQn`, `RTC0_IRQn`, and `TIMER0_IRQn` interrupts.

:kconfig:option:`CONFIG_MPSL_DYNAMIC_INTERRUPTS` enables use of dynamic interrupts for MPSL and allows interrupt reconfiguration.

API documentation
*****************

| Header file: :file:`include/mpsl/mpsl_lib.h`

.. doxygengroup:: mpsl_lib
   :project: nrf
   :members:
