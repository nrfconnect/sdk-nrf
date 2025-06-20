.. _ug_radio_fem_nrf2220:

Enabling support for nRF2220
############################

The nRF2220 device is a range extender that you can use with nRF52, nRF53 and nRF54L Series devices.
The nRF2220 features a GPIO and I2C interface.
You can use it to fully control your front-end module.
To use nRF2220, complete the following steps:

1. Add the following node in the devicetree file:

   .. code-block::

      / {
            nrf_radio_fem: name_of_fem_node {
               compatible = "nordic,nrf2220-fem";
               cs-gpios = <&gpio1 10 GPIO_ACTIVE_HIGH>;
               md-gpios = <&gpio1 8 GPIO_ACTIVE_HIGH>;
               twi-if = <&nrf_radio_fem_twi>;
               output-power-dbm = <10>;
         };
      };

   Additionally, you can consider setting the :kconfig:option:`CONFIG_MPSL_FEM_NRF2220_TEMPERATURE_COMPENSATION` Kconfig option.

#. Optionally replace the device name ``name_of_fem_node``.
#. Replace the pin numbers provided for each of the required properties:

   * ``cs-gpios`` - GPIO characteristic of the device that controls the ``CS`` signal of the nRF2220.
   * ``md-gpios`` - GPIO characteristic of the device that controls the ``MD`` signal of the nRF2220.

   These properties correspond to ``CS`` and ``MD`` pins of nRF2220 that are supported by software FEM.

   The ``phandle-array`` type is commonly used for describing GPIO signals in Zephyr's devicetree.
   The first element ``&gpio1`` refers to the GPIO port (``port 1`` has been selected in the example shown).
   The second element is the pin number on that port.
   The last element must be ``GPIO_ACTIVE_HIGH`` for nRF2220.

#. Optionally, set the value of the ``output-power-dbm`` property to a desired output power of the nRF2220 that is to be used when power amplifier of the nRF2220 is used.
   Allowed range is 7 to 14 dBm.
   To achieve 12 dBm or more output power the power supply voltage of the nRF2220 must be minimum 3.0 V.
   The software assumes that proper supply voltage is assured.
   The nRF2220 device is configured through I2C on boot up to achieve given output power when the power amplifier of the nRF2220 is used.
   The nRF2220 features also a built-in low-attenuation bypass circuit.
   Either the bypass or power amplifier will be used when transmitting RF signals depending on requests made by a protocol driver.
   The control of the output power of the SoC and decision to use either the bypass or the power amplifier occurs automatically.
#. Add the following I2C bus device node on the devicetree file:

   .. code-block:: devicetree

      &pinctrl {
            i2c0_default: i2c0_default {
                  group1 {
                        psels = <NRF_PSEL(TWIM_SDA, 0, 26)>,
                                <NRF_PSEL(TWIM_SCL, 0, 27)>;
                  };
            };

            i2c0_sleep: i2c0_sleep {
                  group1 {
                        psels = <NRF_PSEL(TWIM_SDA, 0, 26)>,
                                <NRF_PSEL(TWIM_SCL, 0, 27)>;
                        low-power-enable;
                  };
            };
      };

      fem_twi: &i2c0 {
            status = "okay";
            compatible = "nordic,nrf-twim";
            pinctrl-0 = <&i2c0_default>;
            pinctrl-1 = <&i2c0_sleep>;
            pinctrl-names = "default", "sleep";

            nrf_radio_fem_twi: nrf2220_fem_twi@36 {
                  compatible = "nordic,nrf2220-fem-twi";
                  status = "okay";
                  reg = <0x36>;
            };
      };

   In this example, the nRF2220 is controlled by the ``i2c0`` bus of the SoC and its slave device address is ``0x36``.
   Replace the I2C bus according to your hardware design, and create alternative entries for I2C with different ``pinctrl-N`` and ``pinctrl-names`` properties.

   .. note::

      On nRF54L Series devices, instead of ``i2c0`` from the above example, use one of the instances ``i2c20``, ``i2c21``, ``i2c22`` if the I2C pins belong to *PERI Power domain* or ``i2c30`` if the I2C pins belong to *LP Power Domain*.

#. The nRF2220 device supports alternative I2C slave address selection.
   Instead of using the default ``0x36`` I2C slave address for nRF2220 device you can use the value ``0x34``.
   In this case, the alternative address selection procedure when switching from ``Power Off`` to ``Bypass`` states of the nRF2220 is used automatically.
#. On nRF53 Series devices, add the devicetree nodes described above the network core.
   Use the ``i2c0`` instance of the network core.
   For the application core, add a GPIO forwarder node to its devicetree file to pass control over given pins from application core to the network core:

   .. code-block:: devicetree

      &gpio_fwd {
         nrf2220-gpio-if {
            gpios = <&gpio0 10 0>,   /* cs-gpios */
                    <&gpio0 8 0>;    /* md-gpios */
         };
         nrf2220-twi-if {
            gpios = <&gpio0 26 0>,   /* TWIM_SDA */
                    <&gpio0 27 0>;   /* TWIM_SCL */
         };
      };

   The pins defined in the GPIO forwarder node in the application core's devicetree file must match the pins defined in the FEM nodes in the network core's devicetree file.

#. On nRF53 Series devices, ``TWIM0`` and ``UARTE0`` are mutually exclusive AHB bus masters on the network core as described in the `Product Specification <nRF5340 Product Specification_>`_, Section 6.4.3.1, Table 22.
   As a result, they cannot be used simultaneously.
   For the I2C part of the nRF2220 interface to be functional, disable the ``UARTE0`` node in the network core's devicetree file.

   .. code-block:: devicetree

      &uart0 {
         status = "disabled";
      };

#. On nRF54L Series devices, make sure the GPIO pins of the SoC selected to control ``cs-gpios`` and ``md-gpios`` support GPIOTE.
   For example, on the nRF54L15 device, use pins belonging to GPIO P1 or GPIO P0 only.
   You cannot use the GPIO P2 pins, because there is no related GPIOTE peripheral.
   It is recommended to use the GPIO pins that belong to the PERI Power Domain of the nRF54L device.
   For example, on the nRF54L15, these are pins belonging to GPIO P1.
   Using pins belonging to Low Power Domain (GPIO P0 on nRF54L15) is supported but requires more DPPI and PPIB channels of the SoC.
   Ensure that the following devicetree instances are enabled (have ``status = "okay"``):

   * ``dppic10``
   * ``dppic20``
   * ``dppic30``
   * ``ppib11``
   * ``ppib21``
   * ``ppib22``
   * ``ppib30``
