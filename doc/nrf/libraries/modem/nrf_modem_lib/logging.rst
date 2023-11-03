.. _nrf_modem_log:

Logging
#######

.. contents::
   :local:
   :depth: 2

The Modem library is released in two versions:

* :file:`libmodem.a`, without logs
* :file:`libmodem_log.a`, which is capable of printing logs

These logs are in plaintext format and come directly from the Modem library.
They are not to be confused with the modem traces, which instead come from the modem and are in binary format.
The logs can provide insight into Modem library operation, and aid the developer during development and debugging.

Logs are sent to the :c:func:`nrf_modem_os_log` function in the OS glue.

.. important::

   Logging may happen in interrupt service routines.
   In the |NCS|, the implementation of :c:func:`nrf_modem_os_log` defers logging to a thread.

In the |NCS|, the version of the library with logs can be linked with the application by adding the following to your project configuration:

* ``CONFIG_NRF_MODEM_LOG=y`` - To link the application with the logging version of the library.
* ``CONFIG_LOG=y`` - To enable the logging subsystem on which the implementation of the :c:func:`nrf_modem_os_log` function in |NCS| depends.

The :kconfig:option:`CONFIG_NRF_MODEM_LIB_LOG_LEVEL` option can be used to specify the log level.

.. note::

   The log version of the Modem library has an increased flash footprint.
