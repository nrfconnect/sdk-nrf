.. _nrf_desktop_application_Kconfig:

nRF Desktop: Application-specific Kconfig options
#################################################

.. contents::
   :local:
   :depth: 2

The nRF Desktop introduces Kconfig options that you can use to simplify an application configuration.
You can use these options to select a device role and to automatically apply a default configuration suitable for the selected role.

.. note::
   Part of the default configuration is applied by modifying the default values of Kconfig options.
   Changing configuration in menuconfig does not automatically adjust user-configurable values to the new defaults.
   So, you must update those values manually.
   For more information, see the Stuck symbols in menuconfig and guiconfig section on the :ref:`kconfig_tips_and_tricks` in the Zephyr documentation.

   The default Kconfig option values are automatically updated if configuration changes are applied directly in the configuration files.

.. _nrf_desktop_hid_configuration:

HID configuration
*****************

The nRF Desktop application introduces application-specific configuration options related to HID device configuration.
These options are defined in :file:`Kconfig.hid`.

The options define the nRF Desktop device role.
The device role can be either a HID dongle (:ref:`CONFIG_DESKTOP_ROLE_HID_DONGLE <config_desktop_app_options>`) or a HID peripheral (:ref:`CONFIG_DESKTOP_ROLE_HID_PERIPHERAL <config_desktop_app_options>`).
The HID peripheral role can also specify a peripheral type:

* HID mouse (:ref:`CONFIG_DESKTOP_PERIPHERAL_TYPE_MOUSE <config_desktop_app_options>`)
* HID keyboard (:ref:`CONFIG_DESKTOP_PERIPHERAL_TYPE_KEYBOARD <config_desktop_app_options>`)
* other HID device (:ref:`CONFIG_DESKTOP_PERIPHERAL_TYPE_OTHER <config_desktop_app_options>`)

Each role automatically implies the nRF Desktop modules needed for the role.
For example, :ref:`nrf_desktop_hid_state` is automatically enabled for the HID peripheral role.

By default, the nRF Desktop devices use a predefined format of HID reports.
The common HID report map is defined in the :file:`configuration/common/hid_report_desc.c` file.
The selected role implies a set of related HID reports.
For example, HID mouse automatically enables support for HID mouse report.
If you select ``other HID device`` peripheral type, you need to explicitly define the set of HID reports in the configuration.

Apart from this, you can specify the supported HID boot protocol interface as one of the following:

* mouse (:ref:`CONFIG_DESKTOP_HID_BOOT_INTERFACE_MOUSE <config_desktop_app_options>`)
* keyboard (:ref:`CONFIG_DESKTOP_HID_BOOT_INTERFACE_KEYBOARD <config_desktop_app_options>`)
* none (:ref:`CONFIG_DESKTOP_HID_BOOT_INTERFACE_DISABLED <config_desktop_app_options>`)

.. _nrf_desktop_hid_device_identifiers:

HID device identifiers
======================

The nRF Desktop application defines the following common device identifiers:

* Manufacturer (:ref:`CONFIG_DESKTOP_DEVICE_MANUFACTURER <config_desktop_app_options>`)
* Vendor ID (:ref:`CONFIG_DESKTOP_DEVICE_VID <config_desktop_app_options>`)
* Product name (:ref:`CONFIG_DESKTOP_DEVICE_PRODUCT <config_desktop_app_options>`)
* Product ID (:ref:`CONFIG_DESKTOP_DEVICE_PID <config_desktop_app_options>`)

These Kconfig options determine the default values of device identifiers used for:

* :ref:`nrf_desktop_usb_state_identifiers`
* BLE GATT Device Information Service (:kconfig:option:`CONFIG_BT_DIS`) that is required for :ref:`nrf_desktop_bluetooth_guide_peripheral`

.. note::
   Apart from the mentioned common device identifiers, the nRF Desktop application defines an application-specific string representing device generation (:ref:`CONFIG_DESKTOP_DEVICE_GENERATION <config_desktop_app_options>`).
   The generation allows to distinguish configurations that use the same board and bootloader, but are not interoperable.
   The value can be read through the :ref:`nrf_desktop_config_channel`.

Debug configuration
*******************

The nRF Desktop application introduces application-specific configuration options related to the ``debug`` configuration.
These options are defined in the :file:`Kconfig.debug` file.

The :ref:`CONFIG_DESKTOP_LOG <config_desktop_app_options>` Kconfig option enables support for logging in the nRF Desktop application.
This option overlays Kconfig option defaults from the Logging subsystem to align them with the nRF Desktop requirements.
The nRF Desktop configuration uses SEGGER J-Link RTT as the Logging subsystem backend.

The :ref:`CONFIG_DESKTOP_SHELL <config_desktop_app_options>` Kconfig option enables support for CLI in the nRF Desktop application.
This option overlays Kconfig option defaults from the Shell subsystem to align them with the nRF Desktop requirements.
The nRF Desktop configuration uses SEGGER J-Link RTT as the Shell subsystem backend.
If both shell and logging are enabled, logger uses shell as the logging backend.

See the :file:`Kconfig.debug` file content for details.

Default common configuration
****************************

The nRF Desktop application aligns the configuration with the nRF Desktop use case by overlaying Kconfig defaults and selecting or implying the required Kconfig options.
Among others, the Kconfig :ref:`app_event_manager` and :ref:`lib_caf` options are selected to ensure that they are enabled.
The :ref:`CONFIG_DESKTOP_SETTINGS_LOADER <config_desktop_app_options>` and :ref:`CONFIG_DESKTOP_POWER_MANAGER <config_desktop_app_options>` are implied to enable the :ref:`nrf_desktop_settings_loader` and :ref:`nrf_desktop_power_manager` modules, respectively.
See the :file:`Kconfig.defaults` file for details related to the default common configuration.

.. _nrf_desktop_bluetooth_configuration:

Bluetooth configuration
***********************

The nRF Desktop application introduces application-specific configuration options related to Bluetooth connectivity configuration.
These options are defined in :file:`Kconfig.ble` file.

The :ref:`CONFIG_DESKTOP_BT <config_desktop_app_options>` Kconfig option enables support for Bluetooth connectivity in the nRF Desktop application.
The option is enabled by default.

The nRF Desktop Bluetooth peripheral configuration (:ref:`CONFIG_DESKTOP_BT_PERIPHERAL <config_desktop_app_options>`) is automatically enabled for the nRF Desktop HID peripheral role (:ref:`CONFIG_DESKTOP_ROLE_HID_PERIPHERAL <config_desktop_app_options>`).
The nRF Desktop Bluetooth central configuration (:ref:`CONFIG_DESKTOP_BT_CENTRAL <config_desktop_app_options>`) is automatically enabled for the nRF Desktop HID dongle role (:ref:`CONFIG_DESKTOP_ROLE_HID_DONGLE <config_desktop_app_options>`)

The nRF Desktop Bluetooth configuration options perform the following:

* Imply Bluetooth-related application modules that are required for the selected device role.
* Select required functionalities in Zephyr's Bluetooth stack.
* Overlay Bluetooth Kconfig option defaults to align them with the nRF Desktop use case.

See :file:`Kconfig.ble` file content for details.
See the :ref:`nrf_desktop_bluetooth_guide` for more information about Bluetooth support in nRF Desktop application.

CAF configuration
******************

The nRF Desktop application overlays the defaults of the :ref:`lib_caf` related Kconfig options to align them with the nRF Desktop use case.
The files that apply the overlays are located in the :file:`src/modules` directory and are named :file:`Kconfig.caf_module_name.default`.
For example, the Kconfig defaults of :ref:`caf_settings_loader` are overlayed in the :file:`src/modules/Kconfig.caf_settings_loader.default`.
