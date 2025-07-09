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
      * The reference design for the project-specific hardware is defined in the :file:`nrf/boards/nordic/nrf52840gmouse` directory.
      * To achieve gaming-grade performance:

        * The application is configured to act as a gaming mouse, with both Bluetooth® LE and USB transports enabled.
        * Bluetooth is configured to use Nordic's SoftDevice link layer.

      * The configuration with the B0 bootloader is set as default.
      * The board supports the ``debug`` (``fast_pair`` file suffix) and ``release`` (``release_fast_pair`` file suffix) configurations for :ref:`nrf_desktop_bluetooth_guide_fast_pair`.
        Both configurations use the MCUboot bootloader built in the direct-xip mode (``MCUBOOT+XIP``), and they support the firmware updates using the :ref:`nrf_desktop_dfu` and the :ref:`nrf_desktop_dfu_mcumgr`.

nRF52832 Desktop Mouse (``nrf52dmouse``)
      * The reference design for the project-specific hardware is defined in the :file:`nrf/boards/nordic/nrf52dmouse` directory.
      * The application is configured to act as a mouse.
      * Only the Bluetooth LE transport is enabled.
        Bluetooth uses Nordic's SoftDevice link layer with Low Latency Packet Mode (LLPM) support disabled.
      * The preconfigured build types do not use a bootloader.

Sample mouse, keyboard or dongle (``nrf52840dk/nrf52840``)
      * The configuration uses the nRF52840 DK.
      * The build types allow to build the application as mouse, keyboard or dongle.
      * Inputs are simulated based on the hardware button presses.
      * The configuration with the B0 bootloader is set as default.
      * The board supports ``debug`` :ref:`nrf_desktop_bluetooth_guide_fast_pair` configuration that acts as a mouse (``fast_pair`` file suffix).
        The configuration uses the MCUboot bootloader built in the direct-xip mode (``MCUBOOT+XIP``), and supports firmware updates using the :ref:`nrf_desktop_dfu` and the :ref:`nrf_desktop_dfu_mcumgr`.

Sample dongle (``nrf52833dk/nrf52833``)
      * The configuration uses the nRF52833 DK.
      * The application is configured to act as a dongle that forwards data from both mouse and keyboard.
        The dongle acts as a Bluetooth central.
        Input data comes from Bluetooth and is retransmitted to USB.
      * For most of the build types, Bluetooth uses Nordic Semiconductor's SoftDevice link layer.
      * The ``dongle_small`` configuration enables logs and mimics the dongle configuration used for small SoCs.
        The configuration is used to verify the correct behavior of the memory-tailored configurations.
      * The configuration with the MCUboot bootloader is set as default.

Sample dongle (``nrf52833dk/nrf52820``)
      * The configuration uses the nRF52820 emulation on the nRF52833 DK.
      * The application is configured to act as a dongle that forwards data from both mouse and keyboard.
      * Bluetooth uses Zephyr's software link layer and is configured to act as a central.
        Input data comes from Bluetooth and is retransmitted to USB.
      * |preconfigured_build_types|

nRF52832 Desktop Keyboard (``nrf52kbd``)
      * The reference design for the project-specific hardware is defined in the :file:`nrf/boards/nordic/nrf52kbd` directory.
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
     * The nRF5 MBR partition (``nrf5_mbr``) added by the ``nrf52840dongle/nrf52840`` board is not used.
       It is statically defined with address and size both set to zero to prevent Partition Manager from trying to place it dynamically.
       The application did not switch to the ``bare`` board variant to keep backwards compatibility.

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

Sample mouse or keyboard (``nrf54l15dk/nrf54l05/cpuapp``)
      * The configuration :ref:`emulates the nRF54L05 SoC <zephyr:nrf54l15dk_nrf54l05>` on the nRF54L15 DK.
      * The build types allow to build the application as a mouse or a keyboard.
      * Inputs are simulated based on the hardware button presses.
      * On the nRF54L05 SoC, you can only use the **GPIO1** port for PWM hardware peripheral output.
        Because of that, on the DK PCA10156 revision v0.9.3, **LED 0** and **LED 2** cannot be used for PWM output.
        You can still use these LEDs with the PWM LED driver, but you must set the LED color to ``LED_COLOR(255, 255, 255)`` or ``LED_COLOR(0, 0, 0)``.
        This ensures the PWM peripheral is not used for the mentioned LEDs.
      * Only Bluetooth LE transport is enabled.
        Bluetooth LE is configured to use Nordic Semiconductor's SoftDevice Link Layer and Low Latency Packet Mode (LLPM).
      * The preconfigured ``debug`` configuration does not use the bootloader due to memory size limits.
        In the ``debug`` configuration, logs are provided through the UART.
        For detailed information on working with the nRF54L15 DK, see the :ref:`ug_nrf54l15_gs` documentation.
      * The preconfigured ``release`` configurations use the MCUboot bootloader built in the direct-xip mode (``MCUBOOT+XIP``) and support firmware updates using the :ref:`nrf_desktop_dfu`.
        All of the ``release`` configurations enable hardware cryptography for the MCUboot bootloader.
        The application image is verified using a pure ED25519 signature.
        The public key that MCUboot uses for validating the application image is securely stored in the hardware Key Management Unit (KMU).
        For more details on nRF54L Series cryptography, see :ref:`ug_nrf54l_cryptography`.
      * The board supports the ``release`` :ref:`nrf_desktop_bluetooth_guide_fast_pair` configuration that acts as a mouse  (``release_fast_pair`` file suffix).

Sample mouse or keyboard (``nrf54l15dk/nrf54l10/cpuapp``)
      * The configuration :ref:`emulates the nRF54L10 SoC <zephyr:nrf54l15dk_nrf54l10>` on the nRF54L15 DK.
      * The build types allow to build the application as a mouse or a keyboard.
      * Inputs are simulated based on the hardware button presses.
      * On the nRF54L10 SoC, you can only use the **GPIO1** port for PWM hardware peripheral output.
        Because of that, on the DK PCA10156 revision v0.9.3, **LED 0** and **LED 2** cannot be used for PWM output.
        You can still use these LEDs with the PWM LED driver, but you must set the LED color to ``LED_COLOR(255, 255, 255)`` or ``LED_COLOR(0, 0, 0)``.
        This ensures the PWM peripheral is not used for the mentioned LEDs.
      * Only Bluetooth LE transport is enabled.
        Bluetooth LE is configured to use Nordic Semiconductor's SoftDevice Link Layer and Low Latency Packet Mode (LLPM).
      * In ``debug`` configurations, logs are provided through the UART.
        For detailed information on working with the nRF54L15 DK, see the :ref:`ug_nrf54l15_gs` documentation.
      * The configurations use the MCUboot bootloader built in the direct-xip mode (``MCUBOOT+XIP``) and support firmware updates using the :ref:`nrf_desktop_dfu`.
        All of the configurations enable hardware cryptography for the MCUboot bootloader.
        The application image is verified using a pure ED25519 signature.
        The public key that MCUboot uses for validating the application image is securely stored in the hardware Key Management Unit (KMU).
        For more details on nRF54L Series cryptography, see :ref:`ug_nrf54l_cryptography`.
      * The board supports the ``debug`` :ref:`nrf_desktop_bluetooth_guide_fast_pair` configuration that acts as a mouse (``fast_pair`` file suffix).
        The configuration uses the MCUboot bootloader built in the direct-xip mode (``MCUBOOT+XIP``), and supports firmware updates using the :ref:`nrf_desktop_dfu` and :ref:`nrf_desktop_dfu_mcumgr`.

Sample mouse or keyboard (``nrf54l15dk/nrf54l15/cpuapp``)
      * The configuration uses the nRF54L15 DK.
      * The build types allow to build the application as a mouse or a keyboard.
      * Inputs are simulated based on the hardware button presses.
      * On the nRF54L15 SoC, you can only use the **GPIO1** port for PWM hardware peripheral output.
        Because of that, on the DK PCA10156 revision v0.9.3, **LED 0** and **LED 2** cannot be used for PWM output.
        You can still use these LEDs with the PWM LED driver, but you must set the LED color to ``LED_COLOR(255, 255, 255)`` or ``LED_COLOR(0, 0, 0)``.
        This ensures the PWM peripheral is not used for the mentioned LEDs.
      * Only Bluetooth LE transport is enabled.
        Bluetooth LE is configured to use Nordic Semiconductor's SoftDevice Link Layer and Low Latency Packet Mode (LLPM).
      * In ``debug`` configurations, logs are provided through the UART.
        For detailed information on working with the nRF54L15 DK, see the :ref:`ug_nrf54l15_gs` documentation.
      * The configurations use the MCUboot bootloader built in the direct-xip mode (``MCUBOOT+XIP``) and support firmware updates using the :ref:`nrf_desktop_dfu`.
        All of the configurations enable hardware cryptography for the MCUboot bootloader.
        The application image is verified using a pure ED25519 signature.
        The public key that MCUboot uses for validating the application image is securely stored in the hardware Key Management Unit (KMU).
        For more details on nRF54L Series cryptography, see :ref:`ug_nrf54l_cryptography`.
      * The board supports the ``debug`` :ref:`nrf_desktop_bluetooth_guide_fast_pair` configuration that acts as a mouse (``fast_pair`` file suffix).
        The configuration uses the MCUboot bootloader built in the direct-xip mode (``MCUBOOT+XIP``), and supports firmware updates using the :ref:`nrf_desktop_dfu` and :ref:`nrf_desktop_dfu_mcumgr`.

Sample mouse (``nrf54lm20dk/nrf54lm20a/cpuapp``)
      * The configuration uses the nRF54LM20 DK.
      * The build types allow to build the application as a mouse.
      * Inputs are simulated based on the hardware button presses.
      * Bluetooth LE and USB High-Speed transports are enabled.
        Bluetooth LE is configured to use Nordic Semiconductor's SoftDevice Link Layer and Low Latency Packet Mode (LLPM).
        USB High-Speed is configured to use the USB next stack (:kconfig:option:`CONFIG_USB_DEVICE_STACK_NEXT`).
        The :kconfig:option:`CONFIG_DESKTOP_BLE_ADV_CTRL_ENABLE` and :kconfig:option:`CONFIG_DESKTOP_BLE_ADV_CTRL_SUSPEND_ON_USB` Kconfig options are enabled in mouse configurations to improve the USB High-Speed report rate.
      * In ``debug`` configurations, logs are provided through the UART.
        For detailed information on working with the nRF54LM20 DK, see the :ref:`ug_nrf54l15_gs` documentation.
      * In ``llvm`` configurations, the partition layout is different to accommodate for the higher memory footprint of the ``llvm``  toolchain.
      * The configurations use the MCUboot bootloader built in the direct-xip mode (``MCUBOOT+XIP``) and support firmware updates using the :ref:`nrf_desktop_dfu`.
        All of the configurations enable hardware cryptography for the MCUboot bootloader.
        The application image is verified using a pure ED25519 signature.
        The public key that MCUboot uses for validating the application image is securely stored in the hardware Key Management Unit (KMU).
        For more details on nRF54L Series cryptography, see :ref:`ug_nrf54l_cryptography`.

Sample mouse or dongle (``nrf54h20dk/nrf54h20/cpuapp``)
      * The configuration uses the nRF54H20 DK.
      * The build types allow to build the application as a mouse or dongle.
      * Inputs are simulated based on the hardware button presses.
      * Bluetooth LE and USB High-Speed transports are enabled.
        Bluetooth LE is configured to use Nordic Semiconductor's SoftDevice Link Layer and Low Latency Packet Mode (LLPM).
        USB High-Speed is configured to use the USB next stack (:kconfig:option:`CONFIG_USB_DEVICE_STACK_NEXT`).
        The :kconfig:option:`CONFIG_DESKTOP_BLE_ADV_CTRL_ENABLE` and :kconfig:option:`CONFIG_DESKTOP_BLE_ADV_CTRL_SUSPEND_ON_USB` Kconfig options are enabled in mouse configurations to improve the USB High-Speed report rate.
      * In ``debug`` configurations, logs are provided through the UART.
        For detailed information on working with the nRF54H20 DK, see the :ref:`ug_nrf54h20_gs` documentation.
      * The configurations use the Software Updates for Internet of Things (SUIT) and support firmware updates using the :ref:`nrf_desktop_dfu`.
        Configurations acting as HID peripherals also support firmware updates using the :ref:`nrf_desktop_dfu_mcumgr`.

      .. note::
         The nRF Desktop application does not build or run for the ``nrf54h20dk/nrf54h20/cpuapp`` board target due to the IronSide SE migration.
         See the ``NCSDK-34299`` in the :ref:`known_issues` page for more information.
         The :ref:`nrf_desktop` documentation may still refer to concepts that were valid before the IronSide SE migration (for example, to the SUIT solution).
         The codebase and documentation will be updated in the future releases to address this issue.
