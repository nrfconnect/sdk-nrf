.. _nrf_desktop_porting_guide:

nRF Desktop: Integrating your own hardware
##########################################

.. contents::
   :local:
   :depth: 2

This page describes how to adapt the nRF Desktop application to different hardware and lists the required steps.

.. tip::
   Make sure to get familiar with the :ref:`nrf_desktop_configuration` section before porting the application to a new board.

.. _porting_guide_adding_board:

Adding a new board
******************

When adding a new board for the first time, focus on a single configuration.
Moreover, keep the default ``debug`` build type that the application is built with, and do not add any additional build type parameters.
The following procedure uses the gaming mouse configuration as an example.

Zephyr support for a board
==========================

Before introducing nRF Desktop application configuration for a given board, you need to ensure that the board is supported in Zephyr.

.. note::
   You can skip this step if your selected board is already supported in Zephyr.

Follow the Zephyr's :ref:`zephyr:board_porting_guide` for detailed instructions related to introducing Zephyr support for a new board.
Make sure that the following conditions are met:

* Edit the DTS files to make sure they match the hardware configuration.
   Pay attention to the following elements:

   * Pins that are used.
   * Bus configuration for optical sensor.
   * `Changing interrupt priority`_.

* Edit the board's Kconfig files to make sure they match the required system configuration.
   For example, disable the drivers that will not be used by your device.

.. tip::
   You can define the new board by copying the nRF Desktop reference design files that are the closest match for your hardware and then aligning the configuration to your hardware.
   For example, for gaming mouse use :file:`nrf/boards/nordic/nrf52840gmouse`.

nRF Desktop support for a board
===============================

Perform the following steps to add nRF Desktop application configuration for a board that is already supported in Zephyr.

1. Copy the project files for the device that is the closest match for your hardware.
   For example, for gaming mouse these are located at :file:`applications/nrf_desktop/configuration/nrf52840gmouse_nrf52840`.
#. Optionally, depending on the reference design, edit the DTS overlay file.
   This step is not required if you have created a new reference design and its DTS files fully describe your hardware.
   In such case, the overlay file can be left empty.
#. In Kconfig, ensure that the following hardware interface modules that are specific for gaming mouse are enabled:

   * :ref:`caf_buttons`
   * :ref:`caf_leds`
   * :ref:`nrf_desktop_motion`
   * :ref:`nrf_desktop_wheel`
   * :ref:`nrf_desktop_battery_meas`

#. For each module enabled, change its configuration to match your hardware.
   Apply the following changes, depending on the module:

   Motion module
     * The ``nrf52840gmouse`` uses the PMW3360 optical motion sensor.
       The sensor is configured in DTS, and the sensor type is selected in the application configuration.
       To add a new sensor, expand the application configuration.
   Wheel module
     * The wheel is based on the QDEC peripheral of the nRF52840 device and the hardware-related part is configured in DTS.
   Buttons module
     * To simplify the configuration of arrays, the nRF Desktop application uses :file:`_def` files.
     * The :file:`_def` file of the buttons module contains pins assigned to rows and columns.
   Battery measurement module
     * The :file:`_def` file of the battery measurement module contains the mapping needed to match the voltage that is read from ADC to the battery level.
   LEDs module
     * The application uses two logical LEDs - one for the peers state, and one for the system state indication.
     * Each of the logical LEDs can have either one (monochromatic) or three color channels (RGB).
       Such color channel is a physical LED.
     * The module uses Zephyr's :ref:`zephyr:led_api` driver for setting the LED color.
       Zephyr's LED driver can use the implementation based on either GPIO or PWM (Pulse-Width Modulation).
       The hardware configuration is described through DTS.
       See the :ref:`caf_leds` configuration section for details.

#. Review the :ref:`nrf_desktop_hid_configuration`.
#. By default, the nRF Desktop device enables Bluetooth connectivity support.
   Review the :ref:`nrf_desktop_bluetooth_configuration`.

   a. Ensure that the Bluetooth role is properly configured.
      For mouse, it should be configured as peripheral.
   #. Update the configuration related to peer control.
      You can also disable the peer control using the :ref:`CONFIG_DESKTOP_BLE_PEER_CONTROL <config_desktop_app_options>` option.
      Peer control details are described in the :ref:`nrf_desktop_ble_bond` documentation.

#. Edit Kconfig to disable options that you do not use.
   Some options have dependencies that might not be needed when these options are disabled.
   For example, when the LEDs module is disabled, the PWM driver is not needed.

.. _porting_guide_adding_sensor:

Adding a new motion sensor
**************************

This procedure describes how to add a new motion sensor into the project.
You can use it as a reference for adding other hardware components.

The nRF Desktop application comes with a :ref:`nrf_desktop_motion` that is able to read data from a motion sensor.
While |NCS| provides support for two motion sensor drivers (PMW3360 and PAW3212), you can add support for a different sensor, based on your development needs.

Complete the steps described in the following sections to add a new motion sensor.

.. rst-class:: numbered-step

Add a new sensor driver
=======================

First, create a new motion sensor driver that will provide code for communication with the sensor.
Use the two existing |NCS| sensor drivers as an example.

The communication between the application and the sensor happens through a sensor driver API (see :ref:`zephyr:sensor`).
For the motion module to work correctly, the driver must support a trigger (see ``sensor_trigger_set``) on a new data (see ``SENSOR_TRIG_DATA_READY`` trigger type).

When the motion data is ready, the driver calls a registered callback.
The application starts a process of retrieving a motion data sample.
The motion module calls ``sensor_sample_fetch`` and then ``sensor_channel_get`` on two sensor channels, ``SENSOR_CHAN_POS_DX`` and ``SENSOR_CHAN_POS_DY``.
The driver must support these two channels.

.. rst-class:: numbered-step

Create a DTS binding
====================

Zephyr recommends to use DTS for hardware configuration (see :ref:`zephyr:dt_vs_kconfig`).
For the new motion sensor configuration to be recognized by DTS, define a dedicated DTS binding.
See :ref:`dt-bindings` for more information, and refer to :file:`dts/bindings/sensor` for binding examples.

.. rst-class:: numbered-step

Configure sensor through DTS
============================

Once binding is defined, it is possible to set the sensor configuration.
To define the binding, edit the DTS file that describes the board.
For more information, see :ref:`devicetree-intro`.

As an example, take a look at the PMW3360 sensor that is already available in the |NCS|.
The following code excerpt is taken from :file:`boards/nordic/nrf52840gmouse/nrf52840gmouse_nrf52840.dts`:

.. code-block:: none

   &spi1 {
      compatible = "nordic,nrf-spim";
      status = "okay";
      cs-gpios = <&gpio0 13 0>;

    pinctrl-0 = <&spi1_default_alt>;
    pinctrl-1 = <&spi1_sleep_alt>;
    pinctrl-names = "default", "sleep";
        pmw3360@0 {
          compatible = "pixart,pmw3360";
          reg = <0>;
          irq-gpios = <&gpio0 21 0>;
          spi-max-frequency = <2000000>;
        };
    };

The communication with PMW3360 happens through the SPI, which makes the sensor a subnode of the SPI bus node.
SPI pins are defined as part of the bus configuration, as these are common among all devices connected to this bus.
In this case, the PMW3360 sensor is the only device on this bus, so there is only one pin specified for selecting the chip.

When the sensor's node is mentioned, you can read ``@0`` in ``pmw3360@0``.
For SPI devices, ``@0`` refers to the position of the chip select pin in the ``cs-gpios`` array for a corresponding device.

Note the string ``compatible = "pixart,pmw3360"`` in the subnode configuration.
This string indicates which DTS binding the node will use.
The binding should match with the DTS binding created earlier for the sensor.

The following options are inherited from the ``spi-device`` binding and are common to all SPI devices:

* ``reg`` - The slave ID number the device has on a bus.
* ``label`` - Used to generate a name of the device (for example, it will be added to generated macros).
* ``spi-max-frequency`` - Used for setting the bus clock frequency.

  .. note::
     To achieve full speed, data must be propagated through the application and reach Bluetooth LE a few hundred microseconds before the subsequent connection event.
     If you aim for the lowest latency through the LLPM (an interval of 1 ms), the sensor data readout should take no more than 250 µs.
     The bus and the sensor configuration must ensure that communication speed is high enough.

The remaining option ``irq-gpios`` is specific to ``pixart,pmw3360`` binding.
It refers to the PIN to which the motion sensor IRQ line is connected.

If a different kind of bus is used for the new sensor, the DTS layout will be different.

.. rst-class:: numbered-step

Include sensor in the application
=================================

Once the new sensor is supported by the |NCS| and the board configuration is updated, you can include it in the nRF Desktop application.

The nRF Desktop application selects a sensor using the configuration options defined in :file:`src/hw_interface/Kconfig.motion`.
Add the new sensor as a new choice option.

The :ref:`nrf_desktop_motion` of the nRF Desktop application has access to several sensor attributes.
These attributes are used to modify the sensor behavior in runtime.
Since the names of the attributes differ for each sensor, the :ref:`nrf_desktop_motion` uses a generic abstraction of them.
You can translate the new sensor-specific attributes to a generic abstraction by modifying the :file:`configuration/common/motion_sensor.h` file.

.. tip::
   If an attribute is not supported by the sensor, you do not need to define it.
   In such case, set the attribute to ``-ENOTSUP``.

.. rst-class:: numbered-step

Select the new sensor
=====================

The application can now use the new sensor.
Edit the application configuration files for your board to enable it.
See :ref:`nrf_desktop_board_configuration` for details.

To start using the new sensor, complete the following steps:

1. Enable all dependencies required by the driver (for example, bus driver).
#. Enable the new sensor driver.
#. Select the new sensor driver in the application configuration options.

Changing interrupt priority
===========================

You can edit the DTS files to change the priority of the peripheral's interrupt.
This can be useful when :ref:`adding a new custom board <porting_guide_adding_board>` or whenever you need to change the interrupt priority.

The ``interrupts`` property is an array, where the meaning of each element is defined by the specification of the interrupt controller.
These specification files are located at :file:`zephyr/dts/bindings/interrupt-controller/` DTS binding file directory.

For example, for nRF52840, the file is :file:`arm,v7m-nvic.yaml`.
This file defines the ``interrupts`` property in the ``interrupt-cells`` list.
For nRF52840, it contains two elements: ``irq`` and ``priority``.
The default values for these elements for the given peripheral are in the :file:`dtsi` file specific for the device.
In the case of nRF52840, this is :file:`zephyr/dts/arm/nordic/nrf52840.dtsi`, which has the following ``interrupts``:

.. code-block::

   spi1: spi@40004000 {
           /*
            * This spi node can be SPI, SPIM, or SPIS,
            * for the user to pick:
            * compatible = "nordic,nrf-spi" or
            *              "nordic,nrf-spim" or
            *              "nordic,nrf-spis".
            */
           #address-cells = <1>;
           #size-cells = <0>;
           reg = <0x40004000 0x1000>;
           interrupts = <4 1>;
           status = "disabled";
   };

To change the priority of the peripheral's interrupt, override the ``interrupts`` property of the peripheral node by including the following code snippet in the :file:`dts.overlay` file or directly in the board DTS:

.. code-block:: none

   &spi1 {
       interrupts = <4 2>;
   };

This code snippet changes the **SPI1** interrupt priority from default ``1`` to ``2``.
