.. _nrf71-idle-power-diag:

nRF71 idle power diagnostics snippet (nrf71-idle-power-diag)
############################################################

.. contents::
   :local:
   :depth: 2

The nRF71 idle power diagnostics snippet adds register-snapshot diagnostics to an nRF71 idle-current measurement build.

.. code-block:: console

   west build -S nrf71-idle-power-diag [...]

Overview
********

This snippet enables the :kconfig:option:`CONFIG_NRF71_IDLE_DIAGNOSTICS` Kconfig option, which adds the :c:func:`nrf71_idle_power_print_snapshot` function (see :ref:`lib_nrf71_idle_power`) to dump the POWER, MEMCONF, GRTC, LFXO, and Wi-Fi core LRC0 registers for correlation with a power analyzer trace.
It also registers an ``idle_snapshot`` shell command when the :kconfig:option:`CONFIG_SHELL` Kconfig option is enabled.

Apply it together with ``nrf71-idle-power`` (or an equivalent ``CONFIG_NRF71_IDLE_POWER=y`` configuration), as this snippet does not enable it on its own.

Since this snippet keeps logging enabled to print the snapshot, it is not meant to be combined with ``nrf71-idle-power-quiet`` in the same build.

Supported boards
****************

The snippet supports the following board targets:

* ``nrf7120dk/nrf7120/cpuapp``
* ``nrf7120dk/nrf7120/cpuapp/ns``

Usage
*****

Apply the snippet when building, for example:

.. code-block:: console

   west build -b nrf7120dk/nrf7120/cpuapp -- -DSNIPPET="nrf71-idle-power;nrf71-idle-power-diag"
