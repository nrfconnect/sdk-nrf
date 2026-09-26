.. _nrf71-idle-power:

nRF71 idle power snippet (nrf71-idle-power)
############################################

.. contents::
   :local:
   :depth: 2

.. code-block:: console

   west build -S nrf71-idle-power [...]

Overview
********

This snippet enables the :ref:`lib_nrf71_idle_power` library, which lowers the
System ON idle current of an application running on the nRF71 Series
application core. It applies the ``UNUSED_ONLY`` RAM retention level, which
lets the ``ram_pwrdn`` library compute the unused RAM range from the image
layout at boot and power it down, without touching any RAM the image itself
uses.

Combine it with ``nrf71-idle-power-quiet`` for a clean current measurement
build, or with ``nrf71-idle-power-diag`` to print a power-state register
snapshot for debugging.

Supported boards
*****************

* ``nrf7120dk/nrf7120/cpuapp``
* ``nrf7120dk/nrf7120/cpuapp/ns``

Usage
*****

Apply the snippet when building, for example:

.. code-block:: console

   west build -b nrf7120dk/nrf7120/cpuapp -- -DSNIPPET=nrf71-idle-power

An application that wants the console/UART to also suspend while idle should
call :c:func:`nrf71_idle_power_suspend_console` right before its idle
``k_sleep()`` or ``k_sem_take()`` call, and
:c:func:`nrf71_idle_power_resume_console` again before printing.
