.. _nrf91_modem_trace_uart_snippet:
.. _snippet-nrf91-modem-trace-uart:

nRF91 modem traces with UART backend snippet (nrf91-modem-trace-uart)
#####################################################################

.. contents::
   :local:
   :depth: 2

The ``nrf91-modem-trace-uart`` snippet enables modem tracing on an nRF91 Series device and configures the UART trace backend.

Overview
********

The snippet enables the :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE` Kconfig option and chooses the Zephyr UART driver for the backend, with the necessary Kconfig options.
The snippet also enables the UART1 peripheral with a baud rate of 1 Mbd and hardware flow control enabled.
If this configuration does not match your requirements, you can add a snippet or Kconfig and devicetree overlays to your application with the desired setup.

Requirements
************

You can only use this snippet with applications that use the RAM layout defined in :ncs-file:`/dts/samples/cellular/nrf91_sram_partitions.dtsi`.
If you are using a different RAM layout, for example, to use more TF-M functionalities, you need to add the ``cpucell_cpuapp_ipc_shm_trace`` partition yourself and copy the configurations from the :file:`.conf` file in the snippet.
See :ref:`devicetree_integration` for more information.

.. note::
    If you are using the nRF9160 DK, remember to :ref:`set the board controller switch to the **nRF91** position <build_pgm_nrf9160_board_controller>` before programming.

Usage
*****

To enable modem tracing with the UART trace backend on an nRF91 Series device, add the ``nrf91-modem-trace-uart`` snippet to the :term:`build configuration`.
You can do this in one of the following ways:

.. tabs::

   .. group-tab:: west

      To add the modem trace UART snippet when building an application with west, use the following command pattern, where *board_target* corresponds to your board target and `<image_name>` to your application image name:

      .. parsed-literal::
        :class: highlight

        west build --board *board_target* -- -D<image_name>_SNIPPET="nrf91-modem-trace-uart"

      .. note::
          With :ref:`sysbuild <configuration_system_overview_sysbuild>`, using the ``west build -S`` option applies the snippet to all images.
          Therefore, use the CMake argument instead, specifying the application image.

   .. group-tab:: CMake

      To add the modem trace UART snippet when building an application with CMake, add the following command to the CMake arguments:

      .. code-block:: console

         -D<image_name>_SNIPPET="nrf91-modem-trace-uart" [...]

      To build with |nRFVSC|, specify ``-D<image_name>_SNIPPET="nrf91-modem-trace-uart" [...]`` in the **Extra CMake arguments** field.

      See :ref:`cmake_options` for more details.

To also direct the TF-M logs to the UART used by the application, combine this snippet with the :ref:`snippet-tfm-enable-share-uart` snippet.
