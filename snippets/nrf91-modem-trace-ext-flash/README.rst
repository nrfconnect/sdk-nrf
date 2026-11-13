.. _nrf91_modem_trace_ext_flash_snippet:
.. _snippet-nrf91-modem-trace-ext-flash:

nRF91 modem traces with flash backend snippet (nrf91-modem-trace-ext-flash)
###########################################################################

.. contents::
   :local:
   :depth: 2

The ``nrf91-modem-trace-ext-flash`` snippet enables modem tracing, the flash backend, and external flash and configures them to store modem traces to a dedicated partition on the external flash for supported boards.

Overview
********

If you want to use the external flash for other purposes, you need to define your own external flash partitions, including one named `modem_trace`.

Supported board targets
***********************

The snippet provides a devicetree overlay for the following board targets:

* ``nrf9151dk/nrf9151/ns``
* ``nrf9161dk/nrf9161/ns``
* ``nrf9160dk/nrf9160/ns``
* ``nrf9131ek/nrf9131/ns``

Requirements
************

You can only use this snippet with applications that use the RAM layout defined in :ncs-file:`/dts/samples/cellular/nrf91_sram_partitions.dtsi`.
If you are using a different RAM layout, for example, to use more TF-M functionalities, you need to add the ``cpucell_cpuapp_ipc_shm_trace`` partition yourself and copy the configurations from the :file:`.conf` file in the snippet.
See :ref:`devicetree_integration` for more information.

Usage
*****

To enable modem traces with the flash backend, add the ``nrf91-modem-trace-ext-flash`` snippet to the :term:`build configuration`.
You can do this in one of the following ways:

.. tabs::

   .. group-tab:: west

      To add the modem traces with the flash backend snippet when building an application with west, use the following command pattern, where *board_target* corresponds to your board target and `<image_name>` to your application image name:

      .. parsed-literal::
         :class: highlight

          west build --board *board_target* -- -D<image_name>_SNIPPET="nrf91-modem-trace-ext-flash"

   .. group-tab:: CMake

      To add the modem traces with the flash backend snippet when building an application with CMake, add the following command to the CMake arguments:

      .. code-block:: console

         -D<image_name>_SNIPPET="nrf91-modem-trace-ext-flash" [...]

      To build with |nRFVSC|, specify ``-D<image_name>_SNIPPET="nrf91-modem-trace-ext-flash" [...]`` in the **Extra CMake arguments** field.

      See :ref:`cmake_options` for more details.
