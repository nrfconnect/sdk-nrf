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
It applies the ``UNUSED_ONLY`` RAM retention level, which lets the ``ram_pwrdn`` library compute the unused RAM range from the image layout at boot and power it down, without touching any RAM the image itself uses.

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
