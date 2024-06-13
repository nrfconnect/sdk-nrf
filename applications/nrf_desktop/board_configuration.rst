.. _nrf_desktop_board_configuration:

nRF Desktop: Board configuration
################################

.. contents::
   :local:
   :depth: 2

The nRF Desktop application is modular.
Depending on requested functions, it can provide mouse, keyboard, or dongle functionality.
The selection of modules depends on the chosen role and also on the selected reference design.
For more information about modules available for each configuration, see :ref:`nrf_desktop_architecture`.

For a board to be supported by the application, you must provide a set of configuration files at :file:`applications/nrf_desktop/configuration/your_board_name`.
The application configuration files define both a set of options with which the nRF Desktop application will be created for your board and the selected :ref:`nrf_desktop_requirements_build_types`.
Include the following files in this directory:

Mandatory configuration files
    * Application configuration file for the :ref:`debug build type <nrf_desktop_requirements_build_types>`.
    * Configuration files for the selected modules.

Optional configuration files
    * Application configuration files for other build types.
    * Configuration file for the bootloader.
    * Configuration file for the sysbuild.
    * Memory layout configuration.
    * DTS overlay file.

See :ref:`porting_guide_adding_board` for information about how to add these files.

.. _nrf_desktop_board_configuration_files:

nRF Desktop board configuration files
*************************************

The nRF Desktop application comes with configuration files for the following reference designs:

nRF52840 Gaming Mouse (``nrf52840gmouse``)
      * The reference design is defined in :file:`nrf/boards/nordic/nrf52840gmouse` for the project-specific hardware.
      * To achieve gaming-grade performance:

        * The application is configured to act as a gaming mouse, with both Bluetooth LE and USB transports enabled.
        * Bluetooth is configured to use Nordic's SoftDevice link layer.

      * The configuration with the B0 bootloader is set as default.
      * The board supports the ``debug`` (``fast_pair`` file suffix) and ``release`` (``release_fast_pair`` file suffix) configurations for :ref:`nrf_desktop_bluetooth_guide_fast_pair`.
        Both configurations use the MCUboot bootloader built in the direct-xip mode (``MCUBOOT+XIP``), and they support the firmware updates using the :ref:`nrf_desktop_dfu` and the :ref:`nrf_desktop_dfu_mcumgr`.

nRF52832 Desktop Mouse (``nrf52dmouse``) and nRF52810 Desktop Mouse (``nrf52810dmouse``)
      * Both reference designs are meant for the project-specific hardware and are defined in :file:`nrf/boards/nordic/nrf52dmouse` and :file:`nrf/boards/nordic/nrf52810dmouse`, respectively.
      * The application is configured to act as a mouse.
      * Only the Bluetooth LE transport is enabled.
        Bluetooth uses either Zephyr's software link layer (``nrf52810dmouse``) or Nordic's SoftDevice link layer (``nrf52dmouse``).
      * The preconfigured build types for both ``nrf52dmouse`` and ``nrf52810dmouse`` boards are without the bootloader due to memory size limits on the ``nrf52810dmouse`` board.

Sample mouse, keyboard or dongle (``nrf52840dk/nrf52840``)
      * The configuration uses the nRF52840 Development Kit.
      * The build types allow to build the application as mouse, keyboard or dongle.
      * Inputs are simulated based on the hardware button presses.
      * The configuration with the B0 bootloader is set as default.
      * The board supports ``debug`` :ref:`nrf_desktop_bluetooth_guide_fast_pair` configuration that acts as a mouse (``fast_pair`` file suffix).
        The configuration uses the MCUboot bootloader built in the direct-xip mode (``MCUBOOT+XIP``), and supports firmware updates using the :ref:`nrf_desktop_dfu` and the :ref:`nrf_desktop_dfu_mcumgr`.

Sample dongle (``nrf52833dk/nrf52833``)
      * The configuration uses the nRF52833 Development Kit.
      * The application is configured to act as a dongle that forwards data from both mouse and keyboard.
      * Bluetooth uses Nordic Semiconductor's SoftDevice link layer and is configured to act as a central.
        Input data comes from Bluetooth and is retransmitted to USB.
      * The configuration with the MCUboot bootloader is set as default.

Sample dongle (``nrf52833dk/nrf52820``)
      * The configuration uses the nRF52820 emulation on the nRF52833 Development Kit.
      * The application is configured to act as a dongle that forwards data from both mouse and keyboard.
      * Bluetooth uses Zephyr's software link layer and is configured to act as a central.
        Input data comes from Bluetooth and is retransmitted to USB.
      * |preconfigured_build_types|

nRF52832 Desktop Keyboard (``nrf52kbd``)
      * The reference design used is defined in :file:`nrf/boards/nordic/nrf52kbd` for the project-specific hardware.
      * The application is configured to act as a keyboard, with the Bluetooth LE transport enabled.
      * Bluetooth is configured to use Nordic Semiconductor's SoftDevice link layer.
      * The preconfigured build types configure the device without the bootloader in debug mode and with B0 bootloader in release mode due to memory size limits.
      * The board supports ``release`` :ref:`nrf_desktop_bluetooth_guide_fast_pair` configuration (``release_fast_pair`` file suffix).
        The configuration uses the MCUboot bootloader built in the direct-xip mode (``MCUBOOT+XIP``), and supports firmware updates using the :ref:`nrf_desktop_dfu` and the :ref:`nrf_desktop_dfu_mcumgr`.

nRF52840 USB Dongle (``nrf52840dongle/nrf52840``) and nRF52833 USB Dongle (``nrf52833dongle``)
      * Since the nRF52840 Dongle is generic and defined in Zephyr, project-specific changes are applied in the DTS overlay file.
      * The application is configured to act as a dongle that forwards data from both mouse and keyboard.
      * Bluetooth uses Nordic Semiconductor's SoftDevice link layer and is configured to act as a central.
        Input data comes from Bluetooth and is retransmitted to USB.
      * The configuration with the B0 bootloader is set as default for the ``nrf52840dongle/nrf52840`` board and with the MCUboot bootloader is set as default for the ``nrf52833dongle`` board.

nRF52820 USB Dongle (``nrf52820dongle``)
      * The application is configured to act as a dongle that forwards data from both mouse and keyboard.
      * Bluetooth uses Zephyr's software link layer and is configured to act as a central.
        Input data comes from Bluetooth and is retransmitted to USB.
      * |preconfigured_build_types|

Sample dongle (``nrf5340dk/nrf5340``)
      * The application is configured to act as a dongle that forwards data from both mouse and keyboard.
      * Bluetooth uses Nordic Semiconductor's SoftDevice link layer without LLPM and is configured to act as a central.
        Input data comes from Bluetooth and is retransmitted to USB.
      * The configuration with the B0 bootloader is set as default.

Sample mouse or keyboard (``nrf54l15pdk/nrf54l15/cpuapp``)
      * The configuration uses the nRF54L15 Preview Development Kit (PDK).
      * The build types allow to build the application as a mouse or a keyboard.
      * Inputs are simulated based on the hardware button presses.
        On the PDK PCA10156, revision v0.2.1, GPIOs assigned to **Button 3** and **Button 4** do not support interrupts.
        Because of this, the application cannot use those buttons.
      * On the nRF54L15 SoC, you can only use the **GPIO1** port for PWM hardware peripheral output.
        Because of that, the PDK PCA10156 has the following limitations:

        * On the PDK revision v0.2.1, **LED 1** cannot be used for PWM output.
        * On the PDK revision v0.3.0, **LED 0** and **LED 2** cannot be used for PWM output.

        You can still use these LEDs with the PWM LED driver, but you must set the LED color to ``LED_COLOR(255, 255, 255)`` or ``LED_COLOR(0, 0, 0)``.
        This ensures the PWM peripheral is not used for the mentioned LEDs.
      * Only Bluetooth LE transport is enabled.
        Bluetooth LE is configured to use Nordic Semiconductor's SoftDevice Link Layer and Low Latency Packet Mode (LLPM).
      * In debug configurations, logs are provided through the UART.
        For detailed information on working with the nRF54L15 PDK, see the :ref:`ug_nrf54l15_gs` documentation.
      * The configurations use the MCUboot bootloader built in the direct-xip mode (``MCUBOOT+XIP``) and support firmware updates using the :ref:`nrf_desktop_dfu`.

Sample mouse (``nrf54h20dk/nrf54h20/cpuapp``)
      * The configuration uses the nRF54H20 DK.
      * The build types allow to build the application as a mouse.
      * Inputs are simulated based on the hardware button presses.
      * Bluetooth LE and USB High-Speed transports are enabled.
        Bluetooth LE is configured to use Nordic Semiconductor's SoftDevice Link Layer and Low Latency Packet Mode (LLPM).
        USB High-Speed is configured to use the USB next stack (:kconfig:option:`CONFIG_USB_DEVICE_STACK_NEXT`).
      * In debug configurations, logs are provided through the UART.
        For detailed information on working with the nRF54H20 DK, see the :ref:`ug_nrf54h20_gs` documentation.
      * The configurations use the Software Updates for Internet of Things (SUIT) and supports firmware updates using the :ref:`nrf_desktop_dfu` and :ref:`nrf_desktop_smp`.
