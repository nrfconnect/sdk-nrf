.. _nrf71-idle-power:

nRF71 idle power snippet (nrf71-idle-power)
###########################################

.. contents::
   :local:
   :depth: 2

The nRF71 idle power snippet enables the :ref:`lib_nrf71_idle_power` library for nRF71 idle-current measurement builds on supported nRF7120 DK board targets.

.. code-block:: console

   west build -S nrf71-idle-power [...]

Overview
********

This snippet enables the :ref:`lib_nrf71_idle_power` library, which reduces the System ON idle current of an application running on the nRF71 Series application core.
It keeps the default ``RAM_RETAIN_FULL`` tier from the library.
Use the :kconfig:option:`CONFIG_NRF71_IDLE_POWER_RAM_RETAIN_USED_ONLY` Kconfig option in the application if you need image-sized RAM retention instead of a fixed KiB boundary.
Do not select it for Wi-Fi builds.
Only image-linked RAM through ``_image_ram_end`` stays powered, see :ref:`lib_ram_pwrdn` for heap limits.

Combine it with ``nrf71-idle-power-quiet`` snippet for a clean current measurement build, or with ``nrf71-idle-power-diag`` to print a power-state register snapshot for debugging.

Supported boards
****************

The snippet supports the following board targets:

* ``nrf7120dk/nrf7120/cpuapp``
* ``nrf7120dk/nrf7120/cpuapp/ns``

Usage
*****

Apply the snippet when building, for example:

.. code-block:: console

   west build -b nrf7120dk/nrf7120/cpuapp -- -DSNIPPET=nrf71-idle-power

An application that requires the console or UART to also suspend while idle should call the :c:func:`nrf71_idle_power_suspend_console` function right before its idle ``k_sleep()`` or ``k_sem_take()`` call.
It should then call the :c:func:`nrf71_idle_power_resume_console` function again before printing.
