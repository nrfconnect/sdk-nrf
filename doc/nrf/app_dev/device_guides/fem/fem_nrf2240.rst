.. _ug_radio_fem_nrf2240:

Enabling support for nRF2240
############################

The nRF2240 device is a range extender that you can use with nRF52, nRF53 and nRF54L Series devices.
For the reception the nRF2240 features a low-attenuation bypass circuit.
For the transmission, either the bypass circuit or the power amplifier of the nRF2240 can be used.
The power amplifier can be used with two different output power levels from range 7 to 21 dBm.
The decision to use either the bypass or one of the two power amplifier power levels for a transmission is made automatically by the FEM driver depending on the output power level requested by a protocol driver.
The nRF2240 is configured through I2C interface on boot up to achieve given output powers.
During the radio operations the nRF2240 is controlled through GPIO interface.
To use the nRF2240, complete the following steps:

1. Add the following node in the devicetree file:

   .. code-block::

      / {
            nrf_radio_fem: name_of_fem_node {
               compatible = "nordic,nrf2240-fem";
               cs-gpios = <&gpio1 10 GPIO_ACTIVE_HIGH>;
               md-gpios = <&gpio1 8 GPIO_ACTIVE_HIGH>;
               pwrmd-gpios = <&gpio1 9 GPIO_ACTIVE_HIGH>;
               twi-if = <&nrf_radio_fem_twi>;
               output-power-dbm = <14>;
               output-power-alt-dbm = <20>;
         };
      };

#. Optionally replace the device name ``name_of_fem_node``.
#. Replace the pin numbers provided for each of the required properties:

   * ``cs-gpios`` - GPIO characteristic of the device that controls the ``CS`` signal of the nRF2240.
   * ``md-gpios`` - GPIO characteristic of the device that controls the ``MD`` signal of the nRF2240.
   * ``pwrmd-gpios`` - GPIO characteristic of the device that controls the ``PWRMD`` signal of the nRF2240.
     This property can be omitted (deleted) if the nRF2240 is to be used with just one output power of the power amplifier.
     In this case, connect the ``PWRMD`` pin of the nRF2240 device to the ``GND``.

   The ``phandle-array`` type is commonly used for describing GPIO signals in Zephyr's devicetree.
   The first element ``&gpio1`` refers to the GPIO port (``port 1`` has been selected in the example shown).
   The second element is the pin number on that port.
   The last element must be ``GPIO_ACTIVE_HIGH`` for nRF2240.

#. Optionally, set the value of the ``output-power-dbm`` property to a desired output power of the nRF2240.
   The nRF2240 is configured on boot up to use this value for transmissions through the power amplifier with the ``PWRMD`` pin low.
   Allowed range is 7 to 21 dBm.
#. Optionally, set the value of the ``output-power-alt-dbm`` property to a desired output power of the nRF2240.
   The nRF2240 is configured on boot up to use this value for transmissions through the power amplifier with the ``PWRMD`` pin high.
   Allowed range is 7 to 21 dBm.
   For this value to take effect, the ``pwrmd-gpios`` property must be defined.
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

            nrf_radio_fem_twi: nrf2240_fem_twi@30 {
                  compatible = "nordic,nrf2240-fem-twi";
                  status = "okay";
                  reg = <0x30>;
            };
      };

   In this example, the nRF2240 is controlled by the ``i2c0`` bus of the SoC and its slave device address is ``0x30``.
   Replace the I2C bus according to your hardware design, and create alternative entries for I2C with different ``pinctrl-N`` and ``pinctrl-names`` properties.

   .. note::

      On nRF54L Series devices, instead of ``i2c0`` from the above example, use one of the instances ``i2c20``, ``i2c21``, ``i2c22`` if the I2C pins belong to *PERI Power domain* or ``i2c30`` if the I2C pins belong to *LP Power Domain*.

#. The nRF2240 device supports alternative I2C slave address selection.
   Instead of using the default ``0x30`` I2C slave address for nRF2240 device you can use the value ``0x32`` in the devicetree.
   In this case, the alternative address selection procedure when switching from ``Power Off`` to ``Bypass`` states of the nRF2240 is used automatically.
#. On nRF53 Series devices, add the devicetree nodes described above the network core.
   Use the ``i2c0`` instance of the network core.
   For the application core, add a GPIO forwarder node to its devicetree file to pass control over given pins from application core to the network core:

   .. code-block:: devicetree

      &gpio_fwd {
         nrf2240-gpio-if {
            gpios = <&gpio1 10 0>,   /* cs-gpios */
                    <&gpio1 8 0>,    /* md-gpios */
                    <&gpio1 9 0>;    /* pwrmd-gpios */
         };
         nrf2240-twi-if {
            gpios = <&gpio0 26 0>,   /* TWIM_SDA */
                    <&gpio0 27 0>;   /* TWIM_SCL */
         };
      };

   The pins defined in the GPIO forwarder node in the application core's devicetree file must match the pins defined in the FEM nodes in the network core's devicetree file.

#. On nRF53 Series devices, ``TWIM0`` and ``UARTE0`` are mutually exclusive AHB bus masters on the network core as described in the `Product Specification <nRF5340 Product Specification_>`_, Section 6.4.3.1, Table 22.
   As a result, they cannot be used simultaneously.
   For the I2C part of the nRF2240 interface to be functional, disable the ``UARTE0`` node in the network core's devicetree file.

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
