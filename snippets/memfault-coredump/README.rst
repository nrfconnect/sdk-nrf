.. _snippet-memfault-coredump:

Memfault coredump snippet (memfault-coredump)
#############################################

.. contents::
   :local:
   :depth: 2

.. code-block:: console

   west build -S memfault-coredump [...]

Overview
********

This snippet enables flash-backed Memfault coredump storage without requiring Partition Manager.
It sets the :kconfig:option:`CONFIG_MEMFAULT_NCS_INTERNAL_FLASH_BACKED_COREDUMP` Kconfig option and applies a board-specific devicetree overlay that defines a ``memfault_coredump_partition`` node within internal flash and sets the ``nordic,memfault-coredump-partition`` chosen property to point at it.

Supported boards
****************

* ``nrf52840dk/nrf52840``
* ``nrf5340dk/nrf5340/cpuapp`` and ``nrf5340dk/nrf5340/cpuapp/ns``
* ``nrf9160dk/nrf9160/ns``
* ``nrf9151dk/nrf9151/ns``
* ``nrf9161dk/nrf9161/ns``
* ``thingy91/nrf9160/ns``
* ``thingy91x/nrf9151/ns``

For other boards, add a ``zephyr,mapped-partition`` node under the internal flash controller in your overlay file and set the ``nordic,memfault-coredump-partition`` chosen property to point at it, ensuring the size is aligned to a multiple of the flash erase block (typically 4 kB).

Notes
*****

The nRF91 ``ns`` overlays redefine the full flash and modem-shared SRAM layout because disabling Partition Manager removes the TF-M and modem partition generation that the |NCS| would otherwise generate.
The layouts mirror those used by the :ref:`at_client_sample` sample.
