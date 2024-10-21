.. _ug_nrf91_snippet:

Snippets for an nRF91 Series device
###################################

.. contents::
   :local:
   :depth: 2

nRF91 Series' :ref:`app_build_snippets` are tailored for tracing on the nRF91 Series devices but can work with other boards as well.
On nRF91 Series devices, you can enable the following functionalities using snippets:

.. list-table::
   :header-rows: 1

   * - Functionality
     - Snippet name
     - Compatible board targets
   * - :ref:`nrf91_modem_trace_ext_flash_snippet`
     - ``nrf91-modem-trace-ext-flash``
     - ``nrf9151dk/nrf9151/ns``, ``nrf9161dk/nrf9161/ns``, ``nrf9160dk/nrf9160/ns``, ``nrf9131ek/nrf9131/ns``
   * - :ref:`nrf91_modem_trace_uart_snippet`
     - ``nrf91-modem-trace-uart``
     - :ref:`All nRF91 Series board targets <ug_nrf91>`
   * - :ref:`nrf91_modem_trace_rtt_snippet`
     - ``nrf91-modem-trace-rtt``
     - :ref:`All nRF91 Series board targets <ug_nrf91>`
   * - :ref:`nrf91_modem_trace_ram_snippet`
     - ``nrf91-modem-trace-ram``
     - :ref:`All nRF91 Series board targets <ug_nrf91>`
   * - :ref:`tfm_enable_share_uart`
     - ``tfm-enable-share-uart``
     - :ref:`All nRF91 Series board targets <ug_nrf91>`

.. _nrf91_modem_trace_ext_flash_snippet:

nRF91 modem traces with flash backend using snippets
****************************************************

The ``nrf91-modem-trace-ext-flash`` snippet enables modem tracing, the flash backend, and external flash and configures them to store modem traces to a dedicated partition on the external flash for supported boards.
To change the partition size, the project needs to configure the :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_FLASH_PARTITION_SIZE` Kconfig option.

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

      To build with the |nRFVSC|, specify ``-D<image_name>_SNIPPET="nrf91-modem-trace-ext-flash" [...]`` in the **Extra CMake arguments** field.

      See :ref:`cmake_options` for more details.

.. _nrf91_modem_trace_ram_snippet:

nRF91 modem traces with RAM backend using snippets
****************************************************

The ``nrf91-modem-trace-ram`` snippet enables modem tracing and configures it to store modem traces to a dedicated partition on the RAM.
To change the partition size, the project needs to configure the :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_RAM_LENGTH` Kconfig option.

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

      To build with the |nRFVSC|, specify ``-D<image_name>_SNIPPET="nrf91-modem-trace-ram" [...]`` in the **Extra CMake arguments** field.

      See :ref:`cmake_options` for more details.

.. _nrf91_modem_trace_uart_snippet:

nRF91 modem tracing with UART backend using snippets
****************************************************

The ``nrf91-modem-trace-uart`` snippet enables the :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE` Kconfig option and chooses the Zephyr UART driver for the backend, with the necessary Kconfig options.
The snippet also enables the UART1 peripheral with a baud rate of 1 Mbd and hardware flow control enabled.
If this configuration does not match your requirements, you can add a snippet or Kconfig and devicetree overlays to your application with the desired setup.

.. note::
    If you are using the nRF9160 DK, remember to :ref:`set the board controller switch to the **nRF91** position <build_pgm_nrf9160_board_controller>` before programming.

To enable modem tracing with the UART trace backend on a nRF91 Series device, add the ``nrf91-modem-trace-uart`` snippet to the :term:`build configuration`.
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

      To build with the |nRFVSC|, specify ``-D<image_name>_SNIPPET="nrf91-modem-trace-uart" [...]`` in the **Extra CMake arguments** field.

      See :ref:`cmake_options` for more details.

.. _nrf91_modem_trace_rtt_snippet:

nRF91 modem traces with RTT backend using snippets
**************************************************

The ``nrf91-modem-trace-rtt`` snippet enables the :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE` Kconfig option and chooses the RTT trace backend with the necessary Kconfig options.

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

       To build with the |nRFVSC|, specify ``-D<image_name>_SNIPPET="nrf91-modem-trace-rtt" [...]`` in the **Extra CMake arguments** field.

       See :ref:`cmake_options` for more details.

.. _tfm_enable_share_uart:

Shared UART for application and TF-M logging
********************************************

If you want to activate TF-M logging while having modem traces with the UART backend enabled, it can be useful to direct the TF-M logs to the UART (**UART0**) used by the application.

To enable modem traces and TF-M logs, add the ``nrf91-modem-trace-uart`` and ``tfm-enable-share-uart`` snippets to the :term:`build configuration`.
You can do this in one of the following ways:

.. tabs::

   .. group-tab:: west

      To activate both modem traces and TF-M logs when building an application with west, use the following command pattern, where *board_target* corresponds to your board target:

      .. parsed-literal::
         :class: highlight

         west build --board *board_target* -S nrf91-modem-trace-uart -S tfm-enable-share-uart

   .. group-tab:: CMake

      To activate TF-M logs when building an application with CMake, add the following command to the CMake arguments:

      .. code-block:: console

         -D<image_name>_SNIPPET="tfm-enable-share-uart" [...]

      To build with the |nRFVSC|, specify ``-D<image_name>_SNIPPET="tfm-enable-share-uart" [...]`` in the **Extra CMake arguments** field.

      See :ref:`cmake_options` for more details.
