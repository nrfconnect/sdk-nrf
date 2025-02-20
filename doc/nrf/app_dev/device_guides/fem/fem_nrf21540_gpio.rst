.. _ug_radio_fem_nrf21540_gpio:

Enabling GPIO mode support for nRF21540
#######################################

The `nRF21540`_ device is a range extender that you can use with nRF52, nRF53 and nRF54L Series devices.
The nRF21540 GPIO mode implementation of FEM is compatible with the nRF21540 device and implements the 3-pin PA/LNA interface.

.. ncs_implementation_desc_start

The |NCS| provides code that configures FEM based on devicetree (DTS) and Kconfig information using the :ref:`nrfxlib:mpsl` (MPSL) library.
The FEM hardware description in the application's devicetree file is an essential part of the configuration.
To enable FEM support, an ``nrf_radio_fem`` node must be present in the application's devicetree file.
The node can be provided by the devicetree file of the target board, by an overlay file, or through the :makevar:`SHIELD` CMake variable (see :ref:`cmake_options`).
See :ref:`zephyr:dt-guide` for more information about the DTS data structure, and :ref:`zephyr:dt_vs_kconfig` for information about differences between DTS and Kconfig.

.. ncs_implementation_desc_end

.. note::
    - See also :ref:`ug_radio_fem_optional_properties` when enabling support for nRF21540.
    - In the naming convention used in the API of the MPSL library, the functionalities designated as ``PA`` and ``LNA`` apply to the ``tx-en-gpios`` and ``rx-en-gpios`` pins listed below, respectively.

To use nRF21540 in GPIO mode, complete the following steps:

1. Add the following node in the devicetree file:

   .. code-block::

      / {
            nrf_radio_fem: name_of_fem_node {
               compatible  = "nordic,nrf21540-fem";
               tx-en-gpios = <&gpio0 13 GPIO_ACTIVE_HIGH>;
               rx-en-gpios = <&gpio0 14 GPIO_ACTIVE_HIGH>;
               pdn-gpios   = <&gpio0 15 GPIO_ACTIVE_HIGH>;
         };
      };

   When you add the above node to the devicetree and build the application, FEM-related Kconfig options are automatically enabled and FEM support is enabled.
   Additionally, you can consider setting the following Kconfig options:

   * :kconfig:option:`CONFIG_MPSL_FEM_NRF21540_TX_GAIN_DB`
   * :kconfig:option:`CONFIG_MPSL_FEM_NRF21540_TX_GAIN_DB_POUTA`
   * :kconfig:option:`CONFIG_MPSL_FEM_NRF21540_TX_GAIN_DB_POUTB`
   * :kconfig:option:`CONFIG_MPSL_FEM_NRF21540_RX_GAIN_DB`
   * :kconfig:option:`CONFIG_MPSL_FEM_NRF21540_RUNTIME_PA_GAIN_CONTROL`

#. Optionally replace the node name ``name_of_fem_node``.
#. Replace the pin numbers provided for each of the required properties:

   * ``tx-en-gpios`` - GPIO characteristic of the device that controls the ``TX_EN`` signal of nRF21540.
   * ``rx-en-gpios`` - GPIO characteristic of the device that controls the ``RX_EN`` signal of nRF21540.
   * ``pdn-gpios`` - GPIO characteristic of the device that controls the ``PDN`` signal of nRF21540.

   These properties correspond to ``TX_EN``, ``RX_EN``, and ``PDN`` pins of nRF21540 that are supported by software FEM.

   Type ``phandle-array`` is used here, which is common in Zephyr's devicetree to describe GPIO signals.
   The first element ``&gpio0`` refers to the GPIO port ("port 0" has been selected in the example shown).
   The second element is the pin number on that port.
   The last element must be ``GPIO_ACTIVE_HIGH`` for nRF21540, but for a different FEM module you can use ``GPIO_ACTIVE_LOW``.

   The state of the remaining control pins should be set in other ways and according to `nRF21540 Product Specification`_.

#. On nRF53 Series devices, add the devicetree nodes described above to the network core.
   For the application core, add a GPIO forwarder node to its devicetree file to pass control over given pins from application core to the network core:

   .. code-block:: devicetree

      &gpio_fwd {
         nrf21540-gpio-if {
            gpios = <&gpio0 13 0>,   /* tx-en-gpios */
                    <&gpio0 14 0>,   /* rx-en-gpios */
                    <&gpio0 15 0>;   /* pdn-gpios */
         };
      };

   The pins defined in the GPIO forwarder node in the application core's devicetree file must match the pins defined in the FEM nodes in the network core's devicetree file.

#. On nRF54L Series devices, make sure the GPIO pins of the SoC selected to control ``tx-en-gpios``, ``rx-en-gpios`` and ``pdn-gpios`` support GPIOTE.
   For example, on the nRF54L15 device, use pins belonging to GPIO P1 or GPIO P0 only.
   You cannot use the GPIO P2 pins, because there is no related GPIOTE peripheral.
   It is recommended to use the GPIO pins that belong to the PERI Power Domain of the nRF54L device.
   For example, on the nRF54L15, these are pins belonging to GPIO P1.
   Using pins belonging to Low Power Domain (GPIO P0 on nRF54L15) is supported but requires more DPPI and PPIB channels of the SoC.
   The nRF54L devices contain only four PPIB channels between PERI Power Domain and Low Power Domain.
   Due to this limitation, only two out of three pins from group ``tx-en-gpios``, ``rx-en-gpios`` and ``pdn-gpios`` (for example, ``tx-en-gpios`` and ``rx-en-gpios``) can be controlled by GPIO P0.
   Select other GPIO port for the one remaining pin of the pin group (for example ``pdn-gpios``).
   To ensure proper timing, set the ``tx-en-settle-time-us`` and ``rx-en-settle-time-us`` devicetree properties of the ``nrf_radio_fem`` node to the value ``12``.
   Enable appropriate instances of the ``DPPIC`` and ``PPIB`` peripherals in the devicetree file:

   .. code-block:: devicetree

      &dppic10 {
            status = "okay";
      };

      &ppib11 {
            status = "okay";
      };

      &ppib21 {
            status = "okay";
      };

      &dppic20 {
            status = "okay";
      };

      &ppib22 {
            status = "okay";
      };

      &ppib30 {
            status = "okay";
      };

      &dppic30 {
            status = "okay";
      };
