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
The device role can be either a HID dongle (:option:`CONFIG_DESKTOP_ROLE_HID_DONGLE`) or a HID peripheral (:option:`CONFIG_DESKTOP_ROLE_HID_PERIPHERAL`).
The HID peripheral role can also specify a peripheral type:

* HID mouse (:option:`CONFIG_DESKTOP_PERIPHERAL_TYPE_MOUSE`)
* HID keyboard (:option:`CONFIG_DESKTOP_PERIPHERAL_TYPE_KEYBOARD`)
* other HID device (:option:`CONFIG_DESKTOP_PERIPHERAL_TYPE_OTHER`)

Each role automatically implies the nRF Desktop modules needed for the role.
For example, :ref:`nrf_desktop_hid_state` is automatically enabled for the HID peripheral role.

By default, the nRF Desktop devices use a predefined format of HID reports.
The common HID report map is defined in the :file:`configuration/common/hid_report_desc.c` file.

The selected role implies a set of related HID reports.
For example, HID mouse automatically enables support for HID mouse report (:option:`CONFIG_DESKTOP_HID_REPORT_MOUSE_SUPPORT`).
You can manually enable support for additional HID reports if needed.
If you select the ``other HID device`` peripheral type, you need to explicitly enable all of the needed HID input reports in the configuration (the reports are not automatically implied by this peripheral type).

You can enable the following HID reports:

* HID mouse report (:option:`CONFIG_DESKTOP_HID_REPORT_MOUSE_SUPPORT`)
* HID keyboard report (:option:`CONFIG_DESKTOP_HID_REPORT_KEYBOARD_SUPPORT`)
* HID system control report (:option:`CONFIG_DESKTOP_HID_REPORT_SYSTEM_CTRL_SUPPORT`)
* HID consumer control report (:option:`CONFIG_DESKTOP_HID_REPORT_CONSUMER_CTRL_SUPPORT`)

.. note::
   nRF Desktop application allows you to modify the used HID input reports or introduce support for a new HID input report.
   This can be achieved by implementing a custom HID report provider that can be used together with the :ref:`nrf_desktop_hid_state`.
   For details, see the :ref:`nrf_desktop_hid_state_providing_hid_input_reports` documentation section.

Apart from this, you can specify the supported HID boot protocol interface as one of the following:

* mouse (:option:`CONFIG_DESKTOP_HID_BOOT_INTERFACE_MOUSE`)
* keyboard (:option:`CONFIG_DESKTOP_HID_BOOT_INTERFACE_KEYBOARD`)
* none (:option:`CONFIG_DESKTOP_HID_BOOT_INTERFACE_DISABLED`)

.. _nrf_desktop_hid_device_identifiers:

HID device identifiers
======================

The nRF Desktop application defines the following common device identifiers:

* Manufacturer (:option:`CONFIG_DESKTOP_DEVICE_MANUFACTURER`)
* Vendor ID (:option:`CONFIG_DESKTOP_DEVICE_VID`)
* Product name (:option:`CONFIG_DESKTOP_DEVICE_PRODUCT`)
* Product ID (:option:`CONFIG_DESKTOP_DEVICE_PID`)

These Kconfig options determine the default values of device identifiers used for:

* :ref:`nrf_desktop_usb_state_identifiers`
* BLE GATT Device Information Service (:kconfig:option:`CONFIG_BT_DIS`) that is required for :ref:`nrf_desktop_bluetooth_guide_peripheral`

.. note::
   Apart from the mentioned common device identifiers, the nRF Desktop application defines an application-specific string representing device generation (:option:`CONFIG_DESKTOP_DEVICE_GENERATION`).
   The generation allows to distinguish configurations that use the same board and bootloader, but are not interoperable.
   The value can be read through the :ref:`nrf_desktop_config_channel`.

Debug configuration
*******************

The nRF Desktop application introduces application-specific configuration options related to the ``debug`` configuration.
These options are defined in the :file:`Kconfig.debug` file.

The :option:`CONFIG_DESKTOP_LOG` Kconfig option enables support for logging in the nRF Desktop application.
This option overlays Kconfig option defaults from the Logging subsystem to align them with the nRF Desktop requirements.
The nRF Desktop configuration uses SEGGER J-Link RTT as the Logging subsystem backend.

The :option:`CONFIG_DESKTOP_SHELL` Kconfig option enables support for CLI in the nRF Desktop application.
This option overlays Kconfig option defaults from the Shell subsystem to align them with the nRF Desktop requirements.
The nRF Desktop configuration uses SEGGER J-Link RTT as the Shell subsystem backend.
If both shell and logging are enabled, logger uses shell as the logging backend.

See the :file:`Kconfig.debug` file content for details.

Default common configuration
****************************

The nRF Desktop application aligns the configuration with the nRF Desktop use case by overlaying Kconfig defaults and selecting or implying the required Kconfig options.
Among others, the Kconfig :ref:`app_event_manager` and :ref:`lib_caf` options are selected to ensure that they are enabled.
The :option:`CONFIG_DESKTOP_SETTINGS_LOADER` and :option:`CONFIG_DESKTOP_POWER_MANAGER` are implied to enable the :ref:`nrf_desktop_settings_loader` and :ref:`nrf_desktop_power_manager` modules, respectively.
See the :file:`Kconfig.defaults` file for details related to the default common configuration.

.. _nrf_desktop_bluetooth_configuration:

Bluetooth® configuration
************************

The nRF Desktop application introduces application-specific configuration options related to Bluetooth connectivity configuration.
These options are defined in :file:`Kconfig.ble` file.

The :option:`CONFIG_DESKTOP_BT` Kconfig option enables support for Bluetooth connectivity in the nRF Desktop application.
The option is enabled by default.

The nRF Desktop Bluetooth peripheral configuration (:option:`CONFIG_DESKTOP_BT_PERIPHERAL`) is automatically enabled for the nRF Desktop HID peripheral role (:option:`CONFIG_DESKTOP_ROLE_HID_PERIPHERAL`).
The nRF Desktop Bluetooth central configuration (:option:`CONFIG_DESKTOP_BT_CENTRAL`) is automatically enabled for the nRF Desktop HID dongle role (:option:`CONFIG_DESKTOP_ROLE_HID_DONGLE`)

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
