.. _mpsl_lib:

Multiprotocol Service Layer library control
###########################################

.. contents::
   :local:
   :depth: 2

Methods in the Multiprotocol Service Layer (MPSL) library control library allow controlling the initialization and uninitialization of the :ref:`nrfxlib:mpsl` library and all required interrupt handlers.
When MPSL is uninitialized, the application has full control over the ``RADIO_IRQn``, ``RTC0_IRQn``, and ``TIMER0_IRQn`` interrupts.

The :kconfig:option:`CONFIG_MPSL_DYNAMIC_INTERRUPTS` Kconfig option enables dynamic interrupts for MPSL, and allows interrupts to be reconfigured.

API documentation
*****************

| Header file: :file:`include/mpsl/mpsl_lib.h`

.. doxygengroup:: mpsl_lib
   :project: nrf
   :members:
