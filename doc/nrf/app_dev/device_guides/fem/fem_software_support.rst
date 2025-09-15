.. _ug_radio_fem_sw_support:
.. _ug_radio_fem_hw_desc:

Enabling FEM support
####################

The following :term:`Front-End Module (FEM)` implementations are available in the |NCS|:

.. list-table::
   :header-rows: 1

   * - Implementation
     - Interface
     - Compatible hardware
     - Documentation for hardware implementation
   * - nRF21540 GPIO+SPI
     - 3-pin + SPI
     - nRF21540
     - :ref:`ug_radio_fem_nrf21540_spi_gpio`
   * - nRF21540 GPIO
     - 3-pin
     - nRF21540
     - :ref:`ug_radio_fem_nrf21540_gpio`
   * - Simple GPIO
     - 2-pin
     - SKY66112-11 and other compatible FEMs
     - :ref:`ug_radio_fem_skyworks`

To use any of these implementations with your application, first complete the following steps:

1. Enable the :ref:`nrfxlib:mpsl_fem` in the :ref:`nrfxlib:mpsl` (MPSL) library.
   It provides implementations of drivers for supported Front-end modules.

   * Enable support for MPSL by setting the :kconfig:option:`CONFIG_MPSL` Kconfig option to ``y``. For radio protocol drivers based on MPSL, this option is selected by default.
   * Enable support for the FEM subsystem by setting the :kconfig:option:`CONFIG_MPSL_FEM` Kconfig option to ``y``.
     This option is selected automatically if a supported FEM is provided in the application's devicetree file.

   .. note::
       If your application cannot use MPSL but you wish to use only the FEM driver provided by MPSL, refer to :ref:`ug_radio_fem_direct_support` for details.

#. Use a radio protocol driver that uses the FEM driver described in the previous step.
   Currently, the following protocols use the FEM support provided by MPSL:

   * :ref:`ug_thread`
   * :ref:`ug_zigbee`
   * :ref:`ug_ble_controller`
   * :ref:`ug_multiprotocol_support`

   For applications based on these protocols, the FEM driver provided by MPSL is used to correctly control the FEM depending on the current radio operation.
   For other radio protocol implementations and applications that control the radio directly, you must use the FEM driver according to the :ref:`nrfxlib:mpsl_api_fem` API.
   For reference, see the following samples that are not based on any of the radio protocol drivers listed above and support FEM control:

   * :ref:`radio_test`
   * :ref:`direct_test_mode`

#. Define the FEM in the devicetree file of the application.
   This is where you choose one of the available implementations listed in the table above.
   Depending on the use case, you can do this by using one of the following methods:

   * Providing a devicetree overlay to the build.
   * Defining the value of the :makevar:`SHIELD` CMake variable.
   * Modifying the target board devicetree file directly.

   Refer to the chosen :ref:`documentation for hardware implementation <ug_radio_fem_hw_desc>` for details.
#. Select the FEM driver implementation by setting one of the following Kconfig options to ``y`` for the chosen FEM support configuration:

   * :ref:`nRF21540 GPIO <ug_radio_fem_nrf21540_gpio>`: :kconfig:option:`CONFIG_MPSL_FEM_NRF21540_GPIO` Kconfig option.
     This Kconfig option is enabled by default if the `nRF21540`_ node is provided in devicetree.
   * :ref:`nRF21540 GPIO+SPI <ug_radio_fem_nrf21540_spi_gpio>`: :kconfig:option:`CONFIG_MPSL_FEM_NRF21540_GPIO_SPI` Kconfig option.
   * :ref:`2-pin simple GPIO <ug_radio_fem_skyworks>`: :kconfig:option:`CONFIG_MPSL_FEM_SIMPLE_GPIO` Kconfig option.
     This Kconfig option is enabled by default if a FEM compatible with generic two control pins is provided in devicetree.

After connecting to the development kit and connecting the shield to the kit, you can :ref:`build your application <building>` and :ref:`program <programming>` the development kit with the created binary file.
Use the ``SHIELD`` CMake variable for this purpose (see :ref:`cmake_options`).
If you are working with the nRF21540 EK, see also :ref:`ug_radio_fem_nrf21540ek`.

.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   fem_mpsl_fem_only
   fem_nrf21540_gpio
   fem_nrf21540_gpio_spi
   fem_nRF21540_optional_properties
   fem_simple_gpio
   fem_incomplete_connections
