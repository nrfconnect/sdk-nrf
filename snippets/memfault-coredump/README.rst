.. _snippet-memfault-coredump:

Memfault coredump snippet (memfault-coredump)
#############################################

.. code-block:: console

   west build -S memfault-coredump [...]

Overview
********

This snippet enables flash-backed Memfault coredump storage without requiring
Partition Manager. It sets :kconfig:option:`CONFIG_MEMFAULT_NCS_INTERNAL_FLASH_BACKED_COREDUMP`
and applies a per-board devicetree overlay that defines a fixed partition
labelled ``memfault_coredump_partition`` at the end of internal flash.

Supported boards
****************

* ``nrf52840dk/nrf52840``
* ``nrf5340dk/nrf5340/cpuapp`` and ``nrf5340dk/nrf5340/cpuapp/ns``
* ``nrf9160dk/nrf9160/ns``
* ``nrf9151dk/nrf9151/ns``
* ``nrf9161dk/nrf9161/ns``
* ``thingy91/nrf9160/ns``
* ``thingy91x/nrf9151/ns``

For other boards, add a ``fixed-partition`` node labelled
``memfault_coredump_partition`` under the internal flash controller in your
own overlay, sized to a multiple of the flash erase block (typically 4 kB).

Notes
*****

When using sysbuild, also set ``SB_CONFIG_PARTITION_MANAGER=n`` (for example
via a ``Kconfig.sysbuild`` fragment in your application) so that Partition
Manager is disabled at the sysbuild level as well.

The nRF91 NS overlays redefine the full flash and modem-shared SRAM layout
because disabling Partition Manager removes the TF-M and modem partition
generation that NCS would otherwise inject. The layouts mirror those used by
the ``samples/cellular/at_client`` sample.
