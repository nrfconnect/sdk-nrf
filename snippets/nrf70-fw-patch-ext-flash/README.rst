.. _snippet-nrf70-fw-patch-ext-flash:

nRF70 firmware patch in external memory snippet (nrf70-fw-patch-ext-flash)
##########################################################################

.. contents::
   :local:
   :depth: 2

The ``nrf70-fw-patch-ext-flash`` snippet stores the nRF70 Series firmware patches in external non-XIP memory instead of the on-chip memory, which increases the amount of on-chip memory available for your application.

Overview
********

The snippet provides a working starting point for the feature described in :ref:`ug_nrf70_developing_fw_patch_ext_flash`, together with the devicetree ``chosen`` property ``nordic,pm-ext-flash``.

The snippet applies the following configuration:

* Enables the storage of the firmware patch in external non-XIP memory, both in the application image (:kconfig:option:`CONFIG_NRF_WIFI_PATCHES_EXT_FLASH_STORE`) and in sysbuild (:kconfig:option:`SB_CONFIG_WIFI_PATCHES_EXT_FLASH_STORE`).
* Adds a devicetree overlay that defines a 128 KB ``nrf70_wifi_fw_partition`` partition on the ``mx25r64`` external flash.
* For the ``nrf7002dk/nrf5340/cpuapp`` board target, enables the GPIO, SPI, and SPI NOR flash drivers needed to access the external flash.

Requirements
************

The external memory must be available and configured in the devicetree, and it must be large enough to store the firmware patches.
If your board uses a different external flash part, adjust the parent node, offsets, and sizes to match your board's external flash layout.

Usage
*****

Apply the snippet when building, for example for the :ref:`wifi_shell_sample` sample on the nRF5340 DK with the nRF7002 EK shield:

With west

.. code-block:: console

   west build -p -b nrf5340dk/nrf5340/cpuapp samples/wifi/shell -- -Dshell_SHIELD=nrf7002ek -Dshell_SNIPPET="nrf70-fw-patch-ext-flash"

With CMake

.. code-block:: console

   cmake -GNinja -Bbuild -DBOARD=nrf5340dk/nrf5340/cpuapp -Dshell_SHIELD=nrf7002ek -Dshell_SNIPPET="nrf70-fw-patch-ext-flash"
   ninja -C build

See also
********

* :ref:`ug_nrf70_developing_fw_patch_ext_flash` for the supported platforms, the available options, and the programming instructions.
