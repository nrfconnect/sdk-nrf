.. _nrf91_modem_trace_rtt_snippet:
.. _snippet-nrf91-modem-trace-rtt:

nRF91 modem traces with RTT backend snippet (nrf91-modem-trace-rtt)
###################################################################

.. contents::
   :local:
   :depth: 2

The ``nrf91-modem-trace-rtt`` snippet enables modem tracing on an nRF91 Series device and configures the RTT trace backend.

Overview
********

The snippet enables the :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE` Kconfig option and chooses the RTT trace backend with the necessary Kconfig options.

Requirements
************

You can only use this snippet with applications that use the RAM layout defined in :ncs-file:`/dts/samples/cellular/nrf91_sram_partitions.dtsi`.
If you are using a different RAM layout, for example, to use more TF-M functionalities, you need to add the ``cpucell_cpuapp_ipc_shm_trace`` partition yourself and copy the configurations from the :file:`.conf` file in the snippet.
See :ref:`devicetree_integration` for more information.

Usage
*****

To enable modem tracing with the RTT backend, add the ``nrf91-modem-trace-rtt`` snippet to the :term:`build configuration`.
You can do this in one of the following ways:

.. tabs::

   .. group-tab:: west

      To add modem traces with the RTT backend when building an application with west, use the following command pattern, where *board_target* corresponds to your board target and `<image_name>` to your application image name:

      .. parsed-literal::
         :class: highlight

         west build --board *board_target* -- -D<image_name>_SNIPPET="nrf91-modem-trace-rtt"

   .. group-tab:: CMake

       To add the modem traces with the RTT backend snippet when building an application with CMake, add the following command to the CMake arguments:

       .. code-block:: console

          -D<image_name>_SNIPPET="nrf91-modem-trace-rtt" [...]

       To build with |nRFVSC|, specify ``-D<image_name>_SNIPPET="nrf91-modem-trace-rtt" [...]`` in the **Extra CMake arguments** field.

       See :ref:`cmake_options` for more details.
