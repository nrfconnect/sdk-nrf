.. _nrf71-idle-power-quiet:

nRF71 idle power quiet snippet (nrf71-idle-power-quiet)
########################################################

.. contents::
   :local:
   :depth: 2

.. code-block:: console

   west build -S nrf71-idle-power-quiet [...]

Overview
********

This snippet disables the console, logging, and boot banner (``CONFIG_SERIAL``,
``CONFIG_LOG``, ``CONFIG_PRINTK``, and related options) for a clean idle
current measurement, where any UART activity would otherwise add noise to the
reading.

Apply it together with ``nrf71-idle-power`` (or an equivalent
``CONFIG_NRF71_IDLE_POWER=y`` configuration), which does not depend on this
snippet on its own.

Supported boards
*****************

* ``nrf7120dk/nrf7120/cpuapp``
* ``nrf7120dk/nrf7120/cpuapp/ns``

Usage
*****

Apply the snippet when building, for example:

.. code-block:: console

   west build -b nrf7120dk/nrf7120/cpuapp -- -DSNIPPET="nrf71-idle-power;nrf71-idle-power-quiet"
