.. _ug_matter_device_advanced_kconfigs:

Advanced Matter Kconfig options
###############################

.. contents::
   :local:
   :depth: 2

After :ref:`enabling the Matter protocol <ug_matter_gs_kconfig>` and testing some of the samples, you can enable additional options in Kconfig and start developing your own Matter application.

Defining path to project-specific Matter settings
=================================================

You can use the :kconfig:option:`CONFIG_CHIP_PROJECT_CONFIG` Kconfig option to define the path to the configuration file that contains project-specific Matter settings in the form of C preprocessor macros.
These macros cannot be altered using dedicated Kconfig options.

.. _ug_matter_configuring_optional_nfc:

Commissioning with NFC
======================

You can configure the Matter protocol to use NFC tag for commissioning, instead of the default QR code.

To enable NFC for commissioning and share the onboarding payload in an NFC tag, set the :kconfig:option:`CONFIG_CHIP_NFC_COMMISSIONING` Kconfig option.

.. _ug_matter_configuring_optional_persistent_subscriptions:

Persistent subscriptions
========================

You can configure the Matter protocol to keep Matter subscriptions on the publisher node.
This allows the device to save information about subscriptions and use it to re-establish the subscriptions immediately when coming back online after being offline for a longer time, for example due to a power outage.
This is a much faster way than waiting for the subscriber node to notice that the publisher node is again available in the network.

To enable persistent subscriptions, set the :kconfig:option:`CONFIG_CHIP_PERSISTENT_SUBSCRIPTIONS` Kconfig option.

.. _ug_matter_configuring_optional_log:

Logging configuration
=====================

Logging is handled with the :kconfig:option:`CONFIG_LOG` option.
This option enables logging for both the stack and Zephyr's :ref:`zephyr:logging_api` API.

Zephyr allows you to configure log levels of different software modules independently.
To change the log level configuration for the Matter module, set one of the available options:

* :kconfig:option:`CONFIG_MATTER_LOG_LEVEL_INF` (default)
* :kconfig:option:`CONFIG_MATTER_LOG_LEVEL_ERR`
* :kconfig:option:`CONFIG_MATTER_LOG_LEVEL_DBG`
* :kconfig:option:`CONFIG_MATTER_LOG_LEVEL_OFF`

.. note::
    :kconfig:option:`CONFIG_MATTER_LOG_LEVEL_WRN` is not used in Matter.

.. _ug_matter_configuring_optional_shell:

Matter shell commands
=====================

You can enable the Matter shell library using the :kconfig:option:`CONFIG_CHIP_LIB_SHELL` Kconfig option.
This option lets you use the Matter shell commands with :ref:`matter_samples`.

See :doc:`matter:nrfconnect_examples_cli` in the Matter documentation for the list of available Matter shell commands.

.. _ug_matter_configuring_device_identification:

Matter device identification
============================

Matter has many ways to identify a specific device, both mandatory and optional.
These can be used for various purposes, such as dividing devices into groups (by function, by vendor or by location), device commissioning or vendor-specific cases before the device was commissioned (for example, identifying factory software version or related features).

Some of these can be configured using the Kconfig options listed below:

* :kconfig:option:`CONFIG_CHIP_DEVICE_VENDOR_ID` sets the device manufacturer identifier that is assigned by the Connectivity Standards Alliance.
* :kconfig:option:`CONFIG_CHIP_DEVICE_PRODUCT_ID` sets the product identifier that is assigned by the product manufacturer.
* :kconfig:option:`CONFIG_CHIP_DEVICE_TYPE` sets the type of the device using the Matter Device Type Identifier, for example Door Lock (0x000A) or Dimmable Light Bulb (0x0101).
* :kconfig:option:`CONFIG_CHIP_COMMISSIONABLE_DEVICE_TYPE` enables including an optional device type subtype in the commissionable node discovery record.
  This allows filtering of the discovery results to find the nodes that match the device type.
* :kconfig:option:`CONFIG_CHIP_ROTATING_DEVICE_ID` enables an optional rotating device identifier feature that provides an additional unique identifier for each device.
  This identifier is similar to the serial number, but it additionally changes at predefined times to protect against long-term tracking of the device.

.. _ug_matter_configuring_ffs:

Amazon Frustration-Free Setup support
=====================================

Frustration-Free Setup (FFS) is Amazon's proprietary technology that uses the user's Amazon account and Alexa services to register an IoT device and provision it to a mesh network.
It does not replace the default commissioning process, but uses the cloud-based Amazon Device Setup Service to provide commissioning information to the Matter network commissioner, instead of involving the user to provide it.
FFS supports a variety of different network protocols, including Matter.
For more information about how FFS works, see the `Understanding Frustration-Free Setup`_ page in the Amazon developer documentation.

The support for FFS over Matter in the |NCS| allows Matter devices to be automatically commissioned to the Matter network using the Matter-enabled Amazon Echo device.
To enable the FFS support, set the following configuration options to meet the Amazon FFS setup prerequisites:

* :kconfig:option:`CONFIG_CHIP_COMMISSIONABLE_DEVICE_TYPE` to ``y``.
* :kconfig:option:`CONFIG_CHIP_ROTATING_DEVICE_ID` to ``y``.
* :kconfig:option:`CONFIG_CHIP_DEVICE_TYPE` to the appropriate value, depending on the device used.
  The value must be compliant with the Matter Device Type Identifier.

Every Matter device must use an unique device identifier for rotating device identifier calculation purpose.
By default, the identifier is set to a random value and stored in the factory data partition.
You can choose your own unique identifier value instead by setting the :kconfig:option:`CONFIG_CHIP_DEVICE_GENERATE_ROTATING_DEVICE_UID` Kconfig option to ``n`` and using the :kconfig:option:`CONFIG_CHIP_DEVICE_ROTATING_DEVICE_UID` Kconfig option.
When using your own identifier, the value can be stored in either firmware or factory data.
For more information about the factory data generation, see the :ref:`Matter Device Factory Provisioning<ug_matter_device_factory_provisioning>` page.

To read more about the FFS technology and its compatibility with Matter, see the following pages in the Amazon developer documentation:

* `Matter Simple Setup for Wi-Fi Overview`_
* `Matter Simple Setup for Thread Overview`_

Reaction to the last Matter fabric removal
==========================================

.. include:: ../end_product/last_fabric_removal_delegate.rst
    :start-after: matter_last_fabric_removal_description_start
    :end-before: matter_last_fabric_removal_description_end

When the device leaves the last fabric, one of several reactions can be set to happen.

To enable one of the reactions to the last fabric removal, set the corresponding Kconfig option to ``y``:

* :kconfig:option:`CONFIG_CHIP_LAST_FABRIC_REMOVED_NONE` - Do not react to the last fabric removal.
  The device will keep all saved data and network credentials, and will not reboot.
* :kconfig:option:`CONFIG_CHIP_LAST_FABRIC_REMOVED_ERASE_ONLY` - Remove all saved network credentials.
  The device will remove all saved network credentials, keep application-specific non-volatile data, and will not reboot.
* :kconfig:option:`CONFIG_CHIP_LAST_FABRIC_REMOVED_ERASE_AND_PAIRING_START` - Remove all saved network credentials and start Bluetooth LE advertising.
  The device will remove all saved network credentials, keep application-specific non-volatile data, and start advertising Bluetooth LE Matter service.
  After that, it will be ready for commissioning to Matter over Bluetooth LE.
* :kconfig:option:`CONFIG_CHIP_LAST_FABRIC_REMOVED_ERASE_AND_REBOOT` - Remove all saved network credentials and reboot the device.
  This option is selected by default.

  When the :kconfig:option:`CONFIG_CHIP_FACTORY_RESET_ERASE_NVS` Kconfig option is also set to ``y``, the device will also remove all non-volatile data stored on the device, including application-specific entries.
  This means the device is restored to the factory settings.

To create a delay between  the chosen reaction and the last fabric being removed, set the :kconfig:option:`CONFIG_CHIP_LAST_FABRIC_REMOVED_ACTION_DELAY` Kconfig option to a specific time in milliseconds.
By default this Kconfig option is set to 1 second.

.. note::
  The :kconfig:option:`CONFIG_CHIP_FACTORY_RESET_ERASE_NVS` Kconfig option is set to ``y`` by default.
  To disable removing application-specific non-volatile data when the :kconfig:option:`CONFIG_CHIP_LAST_FABRIC_REMOVED_ERASE_AND_REBOOT` Kconfig option is selected, set the :kconfig:option:`CONFIG_CHIP_FACTORY_RESET_ERASE_NVS` Kconfig option to ``n``.

.. _ug_matter_configuring_read_client:

Read Client functionality
=========================

The Read Client functionality is used for reading attributes from another device in the Matter network.
This functionality is disabled by default for Matter samples in the |NCS|, except for ones that need to read attributes from the bound devices, such as the :ref:`matter_light_switch_sample` and :ref:`matter_thermostat_sample` samples, and the :ref:`matter_bridge_app` application.
Enable the feature if your device needs to be able to access attributes from a different device within the Matter network using, for example, bindings.
