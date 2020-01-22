.. _porting_guide:

nRF Desktop application porting guide
#####################################

This porting guide for the nRF Desktop project describes how to adapt the nRF Desktop application to different hardware.
It describes the configuration sources that are used for the default configuration, and lists steps required for adding a new board.

Configuration sources
*********************

The nRF Desktop project uses the following files as sources of its configuration:

    * DTS files -- reflect the hardware configuration (see :ref:`device-tree`).
    * ``_def`` files -- contain configuration arrays for the application modules and are specific to the nRF Desktop project.
    * Kconfig files -- reflect the software configuration (see :ref:`kconfig_tips_and_tricks`).

You must modify these configuration sources when `Adding a new board`_.
For information about differences between DTS files and Kconfig, see `Devicetree vs Kconfig`_.

Board configuration
*******************

The nRF Desktop application is modular.
Depending on requested functions, it can provide mouse, keyboard, or dongle functionality.
The selection of modules depends on the chosen role and also on the selected board.
For more information about modules available for each configuration, see :ref:`nrf_desktop_architecture`.

For a board to be supported by the application, you must provide a set of configuration files at ``applications/nrf_desktop/configuration/your_board_name``.
The application configuration files define both a set of options with which the nRF Desktop project will be created for your board and the selected :ref:`nrf_desktop_build_types`.
Include the following files in this directory:

Mandatory configuration files
    * Application configuration file for the ``ZDebug`` build type.
    * Configuration files for the selected modules.

Optional configuration files
    * Application configuration files for other build types.
    * Configuration file for the bootloader.
    * DTS overlay file.

nRF Desktop board configuration files
=====================================

The nRF Desktop project comes with configuration files for the following boards:

Gaming mouse (nrf52840_pca20041)
      * The board is defined in ``nrf/boards/arm/nrf52840_pca20041`` for the project-specific hardware.
      * To achieve gaming-grade performance:

        * The application is configured to act as a gaming mouse, with both BLE and USB transports enabled.
        * Bluetooth is configured to use Nordic's proprietary link layer.

      * |preconfigured_build_types|

Desktop mouse (nrf52_pca20044 and nrf52810_pca20045)
      * Both boards are meant for the project-specific hardware and are defined in ``nrf/boards/arm/nrf52_pca20044`` and ``nrf/boards/arm/nrf52810_pca20045``, respectively.
      * The application is configured to act as a mouse.
      * Only the Bluetooth Low Energy transport is enabled.
        Bluetooth uses Zephyr's software link layer.
      * There is no configuration with bootloader available.


Sample mouse or keyboard (nrf52840_pca10056)
      * The configuration uses the nRF52840 Development Kit.
      * The build types allow to build the application both as mouse and as keyboard.
      * Inputs are simulated based on the hardware button presses.
      * The configuration with bootloader is available.

Keyboard (nrf52_pca20037)
      * The board used is defined in ``nrf/boards/arm/nrf52_pca20037`` for the project-specific hardware.
      * The application is configured to act as a keyboard, with the BLE transport enabled.
      * Bluetooth is configured to use Nordic's proprietary link layer.
      * |preconfigured_build_types|

Dongle (nrf52840_pca10059)
      * This configuration uses Nordic's nRF52840 dongle defined in Zephyr.
      * Since the board is generic, project-specific changes are applied in the DTS overlay file.
      * The application is configured to act as both mouse and keyboard.
      * Bluetooth uses Nordic's proprietary link layer and is configured to act as a central.
        Input data comes from Bluetooth and is retransmitted to USB.
      * |preconfigured_build_types|

.. _porting_guide_adding_board:

Adding a new board
******************

When adding a new board for the first time, focus on a single configuration.
Moreover, keep the default ``ZDebug`` build type that the application is built with, and do not add any additional build type parameters.

.. note::
    The following procedure uses the gaming mouse configuration as an example.

To use the nRF Desktop project with your custom board:

1. Define the board by copying the nRF Desktop board files that are the closest match for your hardware.
   For example, for gaming mouse use ``nrf/boards/arm/nrf52840_pca20041``.
#. Edit the DTS files to make sure they match the hardware configuration.
   Pay attention to pins that are used and to the bus configuration for optical sensor.
#. Edit the board's Kconfig files to make sure they match the required system configuration.
   For example, disable the drivers that will not be used by your device.
#. Copy the project files for the device that is the closest match for your hardware.
   For example, for gaming mouse these are located at ``applications/nrf_desktop/configure/nrf52840_pca20041``.
#. Optionally, depending on the board, edit the DTS overlay file.
   This step is not required if you have created a new board and its DTS files fully describe your hardware.
   In such case, the overlay file can be left empty.
#. In Kconfig, ensure that the following modules that are specific for gaming mouse are enabled:

    * :ref:`motion`
    * :ref:`wheel`
    * :ref:`buttons`
    * :ref:`battery_meas`
    * :ref:`leds`

#. For each module enabled, change its ``_def`` file to match your hardware:

    * Motion module
        * The ``nrf52840_pca20041`` uses the PMW3360 optical motion sensor.
          The sensor is configured in DTS, and the sensor type is selected in the application configuration.
          To add a new sensor, expand the application configuration.
    * Wheel module
        * The wheel is based on the QDEC peripheral of the nRF52840 device and the hardware-related part is configured in DTS.
    * Buttons module
        * To simplify the configuration of arrays, the nRF Desktop project uses ``_def`` files.
        * The ``_def`` file of the buttons module contains pins assigned to rows and columns.
    * Battery measurement module
        * The ``_def`` file of the battery measurement module contains the mapping needed to match the voltage read from ADC to the battery level.
    * LEDs module
        * The application uses two logical LEDs -- one for the peers state, and one for the system state indication.
	* Each of the logical LEDs can have either one (monochromatic) or three color channels (RGB).
	  Such color channel is a physical LED.
        * The project uses PWM channels to control the brightness of each physical LED.
          The PWM peripheral must be configured in DTS files, and the ``_def`` file of the LEDs module must be configured to indicate which PWM channel is assigned to each LED color.
	  Ensure that PWM channels are correctly configured in DTS and PWM driver is enabled in Kconfig file.

#. Review Bluetooth options in Kconfig.
   Refer to the Bluetooth configuration page for the list of available options.
   Ensure that the Bluetooth role is configured appropriately.
   For mouse, it should be configured as peripheral.
#. Edit Kconfig to disable options that you do not use.
   Some options have dependencies that might not be needed when these options are disabled.
   For example, when the LEDs module is disabled, the PWM driver is not needed.


Application modules
*******************

For more information about each application module and its configuration details, see the following pages:

.. toctree::
   :maxdepth: 2

   hw_interface.rst
   modules.rst

.. |preconfigured_build_types| replace:: The preconfigured build types configure the device with or without the bootloader and in debug or release mode.
