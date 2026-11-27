.. _nrf91_modem_trace_ram_snippet:
.. _snippet-nrf91-modem-trace-ram:

nRF91 modem traces with RAM backend snippet (nrf91-modem-trace-ram)
###################################################################

.. contents::
   :local:
   :depth: 2

The ``nrf91-modem-trace-ram`` snippet enables modem tracing on an nRF91 Series device and configures it to store modem traces to a dedicated partition on the RAM.

Overview
********

To change the partition size, the project needs to configure the :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_RAM_LENGTH` Kconfig option.

Requirements
************

You can only use this snippet with applications that use the RAM layout defined in :ncs-file:`/dts/samples/cellular/nrf91_sram_partitions.dtsi`.
If you are using a different RAM layout, for example, to use more TF-M functionalities, you need to add the ``cpucell_cpuapp_ipc_shm_trace`` partition yourself and copy the configurations from the :file:`.conf` file in the snippet.
See :ref:`devicetree_integration` for more information.

Usage
*****

To enable modem traces with the RAM backend, add the ``nrf91-modem-trace-ram`` snippet to the :term:`build configuration`.
You can do this in one of the following ways:

.. tabs::

   .. group-tab:: west

      To add modem traces with the RAM backend when building an application with west, use the following command pattern, where *board_target* corresponds to your board target and `<image_name>` to your application image name:

      .. parsed-literal::
         :class: highlight

         west build --board *board_target* -- -D<image_name>_SNIPPET="nrf91-modem-trace-ram"

   .. group-tab:: CMake

      To add the modem traces with the RAM backend snippet when building an application with CMake, add the following command to the CMake arguments:

      .. code-block:: console

         -D<image_name>_SNIPPET="nrf91-modem-trace-ram" [...]

      To build with |nRFVSC|, specify ``-D<image_name>_SNIPPET="nrf91-modem-trace-ram" [...]`` in the **Extra CMake arguments** field.

      See :ref:`cmake_options` for more details.
