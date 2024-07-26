.. _ug_radio_fem_nrf21540_spi_gpio:

Enabling GPIO+SPI mode support for nRF21540
###########################################

The `nRF21540`_ device is a range extender that you can use with nRF52 and nRF53 Series devices.
The nRF21540 features an SPI interface.
You can use it to fully control your front-end module or you can use a combination of SPI and GPIO interface.
The SPI interface enables you, for example, to :ref:`set the output power <ug_radio_fem_sw_support_mpsl_fem_output>` of the nRF21540.

.. include:: fem_nrf21540_gpio.rst
    :start-after: ncs_implementation_desc_start
    :end-before: ncs_implementation_desc_end

.. note::
    See also :ref:`ug_radio_fem_optional_properties` when enabling support for nRF21540.

To use nRF21540 in GPIO+SPI mode, complete the following steps:

1. Add the following node in the devicetree file:

   .. code-block::

      / {
            nrf_radio_fem: name_of_fem_node {
               compatible  = "nordic,nrf21540-fem";
               tx-en-gpios = <&gpio0 13 GPIO_ACTIVE_HIGH>;
               rx-en-gpios = <&gpio0 14 GPIO_ACTIVE_HIGH>;
               pdn-gpios   = <&gpio0 15 GPIO_ACTIVE_HIGH>;
               spi-if = <&nrf_radio_fem_spi>
         };
      };
#. Optionally replace the device name ``name_of_fem_node``.
#. Replace the pin numbers provided for each of the required properties:

   * ``tx-en-gpios`` - GPIO characteristic of the device that controls the ``TX_EN`` signal of nRF21540.
   * ``rx-en-gpios`` - GPIO characteristic of the device that controls the ``RX_EN`` signal of nRF21540.
   * ``pdn-gpios`` - GPIO characteristic of the device that controls the ``PDN`` signal of nRF21540.

   These properties correspond to ``TX_EN``, ``RX_EN``, and ``PDN`` pins of nRF21540 that are supported by software FEM.

   The``phandle-array`` type is commonly used for describing GPIO signals in Zephyr's devicetree.
   The first element ``&gpio0`` refers to the GPIO port ("port 0" has been selected in the example shown).
   The second element is the pin number on that port.
   The last element must be ``GPIO_ACTIVE_HIGH`` for nRF21540, but for a different FEM module you can use ``GPIO_ACTIVE_LOW``.

   Set the state of the remaining control pins according to the `nRF21540 Product Specification`_.
#. Add a following SPI bus device node on the devicetree file:

   .. code-block:: devicetree

      &pinctrl {
         spi3_default_alt: spi3_default_alt {
            group1 {
               psels = <NRF_PSEL(SPI_SCK, 1, 15)>,
                       <NRF_PSEL(SPI_MISO, 1, 14)>,
                       <NRF_PSEL(SPI_MOSI, 1, 13)>;
            };
         };

         spi3_sleep_alt: spi3_sleep_alt {
            group1 {
               psels = <NRF_PSEL(SPI_SCK, 1, 15)>,
                       <NRF_PSEL(SPI_MISO, 1, 14)>,
                       <NRF_PSEL(SPI_MOSI, 1, 13)>;
               low-power-enable;
            };
         };
      };

      fem_spi: &spi3 {
         status = "okay";
         pinctrl-0 = <&spi3_default_alt>;
         pinctrl-1 = <&spi3_sleep_alt>;
         pinctrl-names = "default", "sleep";
         cs-gpios = <&gpio0 21 GPIO_ACTIVE_LOW>;

         nrf_radio_fem_spi: nrf21540_fem_spi@0 {
            compatible = "nordic,nrf21540-fem-spi";
            status = "okay";
            reg = <0>;
            spi-max-frequency = <8000000>;
         };
      };

   In this example, the nRF21540 is controlled by the ``spi3`` bus.
   Replace the SPI bus according to your hardware design, and create alternative entries for SPI3 with different ``pinctrl-N`` and ``pinctrl-names`` properties.
#. On nRF53 devices, the devicetree nodes described above must be added to the network core.
   For the application core, you must also add a GPIO forwarder node to its devicetree file:

   .. code-block:: devicetree

      &gpio_fwd {
         nrf21540-gpio-if {
            gpios = <&gpio0 11 0>,   /* tx-en-gpios */
                    <&gpio0 12 0>,   /* rx-en-gpios */
                    <&gpio0 13 0>,   /* pdn-gpios */
                    <&gpio0 14 0>,   /* ant-sel-gpios */
                    <&gpio0 15 0>;   /* mode-gpios */
         };
         nrf21540-spi-if {
            gpios = <&gpio0 21 0>,   /* cs-gpios */
                    <&gpio1 15 0>,   /* SPIM_SCK */
                    <&gpio1 14 0>,   /* SPIM_MISO */
                    <&gpio1 13 0>;   /* SPIM_MOSI */
         };
      };

   The pins defined in the GPIO forwarder node in the application core's devicetree file must match the pins defined in the FEM nodes in the network core's devicetree file.

#. On nRF53 devices, ``SPIM0`` and ``UARTE0`` are mutually exclusive AHB bus masters on the network core as described in the `Product Specification <nRF5340 Product Specification_>`_, Section 6.4.3.1, Table 22.
   As a result, they cannot be used simultaneously.
   For the SPI part of the nRF21540 interface to be functional, you must disable the ``UARTE0`` node in the network core's devicetree file.

   .. code-block:: devicetree

      &uart0 {
         status = "disabled";
      };

.. note::
   The nRF21540 GPIO-only mode of operation is selected by default in Kconfig when an nRF21540 node is provided in devicetree, as mentioned in the :ref:`ug_radio_fem_sw_support` section.
   To enable the GPIO+SPI mode of operation, you must explicitly set the :kconfig:option:`CONFIG_MPSL_FEM_NRF21540_GPIO_SPI` Kconfig option to ``y``.
