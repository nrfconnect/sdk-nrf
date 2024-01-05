.. _ug_pinctrl:

Pin control
###########

.. contents::
   :local:
   :depth: 2

Pin control, also referred to as ``pinctrl``, is a Zephyr RTOS API introduced as an effort to standardize how SoC peripheral pins are configured.
This enables mapping peripheral signals to the required pins as well as configuring necessary pin properties.
For example, using pin control, you can route ``UART0`` ``RX`` signal to pin ``P1.01`` and configure the pin pull-up resistor.

Before pin control, the same functionality was available in a vendor specific way.
For the previous example, Nordic SoCs had to be configured in the following way:

.. code-block:: dts

    &uart0 {
        ...
        rx-pin = <33>;
        rx-pull-up;
        ...
    };

Pin control works using a *state model*, where each *state* encodes the pin configurations for a state of the peripheral.
Following are the standard states:

* ``default`` - Peripheral in working mode.
* ``sleep`` - Peripheral in sleep mode or low power mode.

A similar approach is also used in other systems like Linux.
For more details including nRF specific bits, see the next section.

For more general information, see the :ref:`Zephyr documentation on pin control <pinctrl-guide>`.

Pin control in nRF
******************

The nRF pin controller is represented in :ref:`devicetree <dt-guide>` with the ``pinctrl`` node label.
This node contains all pin configurations and you can modify it as shown in the following snippet:

.. code-block:: dts

   &pinctrl {
           /* Add your modifications here */
   };

All device pin configurations must be placed in child nodes of the ``pinctrl`` node, as shown in the following example:

.. code-block:: dts

   &pinctrl {
      /* configuration for uart0 device, default state */
      uart0_default: uart0_default {
        /* group 1 ('group1' name is arbitrary) */
        group1 {
          /* configure P0.1 as UART_TX and P0.2 as UART_RTS */
          psels = <NRF_PSEL(UART_TX, 0, 1)>, <NRF_PSEL(UART_RTS, 0, 2)>;
        };
        /* group 2 */
        group2 {
          /* configure P0.3 as UART_RX and P0.4 as UART_CTS */
          psels = <NRF_PSEL(UART_RX, 0, 3)>, <NRF_PSEL(UART_CTS, 0, 4)>;
          /* both P0.3 and P0.4 are configured with pull-up */
          bias-pull-up;
        };
      };
      /* configuration for uart0 device, sleep state */
      uart0_sleep: uart0_sleep {
        /* group 1 ('group1' name is arbitrary) */
        group1 {
          /* configure P0.1 as UART_TX, P0.2 as UART_RTS P0.3 as UART_RX and
           * P0.4 as UART_CTS
           */
          psels = <NRF_PSEL(UART_TX, 0, 1)>, <NRF_PSEL(UART_RTS, 0, 2)>,
                  <NRF_PSEL(UART_RX, 0, 3)>, <NRF_PSEL(UART_CTS, 0, 4)>;
          /* all pins are configured in low power mode */
          low-power-enable;
        };
      };
    };

You can put the previous code in a :file:`board-pinctrl.dtsi` file in your board directory, or a devicetree overlay in your application.

The ``uart0_default`` child node encodes the pin configurations for a particular state of a device; in this case, the ``default`` active state.
Similarly, ``uart0_sleep`` child node encodes the pin configurations for the ``sleep`` state.

.. note::
   The ``sleep`` state is only required if :kconfig:option:`CONFIG_PM_DEVICE` is set to ``y``.

As shown in the code snippet, the pin configurations are organized in groups within each child node.
Each group can specify a list of pin function selections in the ``psels`` property.
The ``NRF_PSEL`` macro is used to specify a pin function selection.
Available pin functions can be found in the :file:`include/dt-bindings/pinctrl/nrf-pinctrl.h` header file in the `Zephyr repository`_.

A group can also specify shared pin properties common to all the specified pins, such as the ``bias-pull-up`` property in group 2.
Here is a list of the supported standard pin properties:

* ``bias-disable`` - Disable pull-up/down (default behavior, not required).
* ``bias-pull-up`` - Enable pull-up resistor.
* ``bias-pull-down`` - Enable pull-down resistor.
* ``low-power-enable`` - Configure pin as an input with input buffer disconnected.

.. note::

   The bias options are mutually exclusive.

The following non-standard, nRF-specific properties are also supported:

* ``nordic,drive-mode`` - Pin output drive mode. Available drive modes are pre-defined in :file:`include/dt-bindings/pinctrl/nrf-pinctrl.h`. Note that extra modes might not be available on certain devices. When not specified, pin defaults to standard mode for 0 and 1 (``NRF_DRIVE_S0S1``).
* ``nordic,invert`` - Invert pin polarity  (set active state to low). This property is valid only for PWM channel output pins.

To link this pin configuration with a device, use a pinctrl-N property where N is the state index starting from zero.
You can add the following code in the DTS file of your board:

.. code-block:: dts

   #include "board-pinctrl.dtsi"

    &uart0 {
        ...
        /* default state */
        pinctrl-0 = <&uart0_default>;
        /* sleep state (only applicable if CONFIG_PM_DEVICE=y) */
        pinctrl-1 = <&uart0_sleep>;
        /* state assigned to each pinctrl-N property by index */
        pinctrl-names = "default", "sleep";
        ...
    };

Migration of the devicetree files
*********************************

To migrate the devicetree files of any Nordic-based board, you must complete the following steps:

1. Create a :file:`board-pinctrl.dtsi` file, defining each peripheral state pin configuration.
2. Include this file in the :file:`board.dts` file.
3. Replace all ``-pin`` related properties with pin control states.
4. Enable pin control by setting :kconfig:option:`CONFIG_PINCTRL` to ``y`` in the :file:`board.conf` file.

.. note::

   The old method of implementing pin control using ``-pin`` properties is supported on custom boards if the configuration option :kconfig:option:`CONFIG_PINCTRL`  is set to ``n``. However, this method will be deprecated and removed in the upcoming Zephyr releases.


The following example illustrates the migration of a board file:

.. code-block:: dts

    /* old board.dts */
    &uart0 {
        ...
        tx-pin = <5>;
        rx-pin = <33>;
        rx-pull-up;
        ...
    };

.. code-block:: dts

    /* new board-pinctrl.dtsi file */
    &pinctrl {
        uart0_default: uart0_default {
            group1 {
                psels = <NRF_PSEL(UART_TX, 0, 5)>;
            };
            group2 {
                psels = <NRF_PSEL(UART_RX, 1, 1)>;
                bias-pull-up;
            };
        };
        uart0_sleep: uart0_sleep {
            group1 {
                psels = <NRF_PSEL(UART_TX, 0, 5)>,
                        <NRF_PSEL(UART_RX, 1, 1)>;
                low-power-enable;
            };
        };
    };

.. code-block:: dts

    /* adjusted board.dts */
    ...
    #include "board-pinctrl.dtsi"
    ...
    &uart0 {
        ...
        pinctrl-0 = <&uart0_default>;
        pinctrl-1 = <&uart0_sleep>;
        pinctrl-names = "default", "sleep";
        ...
    };

Overlay files
=============

For overlay files, the migration procedure is same as that of devicetree files.
In the case of overlays, the name of the state configuration nodes must have a different name, for example, ``uart0_default_alt``.

All upstream boards define both ``default`` and ``sleep`` states.
Overlays also need to define both.
However, if a particular sample or application does not make use of device power management, you can configure the overlay file in the following way:

.. code-block:: dts

    &pinctrl {
        uart0_default_alt: uart0_default_alt {
            ...
        };
    };

    &uart0 {
        ...
        pinctrl-0 = <&uart0_default_alt>;
        /delete-property/ pinctrl-1;
        pinctrl-names = "default";
        ...
    };


Migration script
================

A utility script is provided to automatically migrate the devicetree files of any Nordic-based board to include pin control.
The script modifies the file initially by removing old pin-related properties and replacing them with pin control states.

The script is located in the :file:`zephyr/scripts/utils/` folder, and can be used in the following way:

.. code-block::

   python pinctrl_nrf_migrate.py -i path/to/board.dts

A :file:`board-pinctrl.dtsi` file containing the configuration for all pinctrl states is generated.
The script has a known limitation that all SPI nodes will be assumed to be a master device.

To know about the additional options that are available, see the `Pinctrl Migration Utility Script documentation`_ for more details.

.. caution::

   This script uses a basic line based parser, therefore not all valid devicetree files will be converted correctly.
   The adjusted or generated files must be manually reviewed.
