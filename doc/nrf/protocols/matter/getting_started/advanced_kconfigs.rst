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

.. _ug_matter_configuring_optional_ble_advertising:

Bluetooth LE advertising
========================

The Matter specification requires the accessory device to advertise Matter service over Bluetooth® Low Energy (LE) for commissioning purposes.
By default, the Bluetooth LE advertising start has to be requested by the application (for example, as a result of a button press) and lasts for a maximum duration of 15 minutes.
This is appropriate for a device with high security requirements that should not advertise its service without a direct trigger, for example a door lock.

You can configure when the device will start advertising and how long it will advertise with the following Kconfig options:

* Set :kconfig:option:`CONFIG_CHIP_ENABLE_PAIRING_AUTOSTART` to ``y`` to open the commissioning window and start the Bluetooth LE advertising automatically at application boot, if the device is not already commissioned.
* Set :kconfig:option:`CONFIG_CHIP_BLE_EXT_ADVERTISING` to ``y`` to enable Extended Announcement (also called Extended Beaconing), which allows a device to advertise for a duration of more than 15 minutes.
  The advertising duration can be extended to a maximum duration of 48 hours, however the set of advertised data is changed to increase the user privacy.
* Set :kconfig:option:`CONFIG_CHIP_BLE_ADVERTISING_DURATION` to a value of time in minutes to specify how long the device will advertise Matter service over Bluetooth LE.
  It cannot be set to values higher than 15 minutes unless the Extended Announcement feature is enabled.

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

.. _ug_matter_configuring_settings_shell:

Matter Settings shell commands
------------------------------

You can enable the Matter Settings shell commands to monitor the current usage of the Zephyr Settings NVS.
These commands are useful for verifying that the ``settings`` partition has the proper size and meets the application requirements.

To enable the Matter Settings shell module, set the :kconfig:option:`CONFIG_NCS_SAMPLE_MATTER_SETTINGS_SHELL` Kconfig option to ``y``.

You can use the following shell commands:

* ``matter_settings peak`` - Read the maximum settings usage peak.
* ``matter_settings reset`` - Reset the peak value.
* ``matter_settings get_size <name>`` - Get the size of the specific entry.
* ``matter_settings current`` - Get the size of the current settings usage.
* ``matter_settings free`` - Get the size of the current free settings space.

.. note::

  The Matter Settings shell module is available only for the NVS Zephyr Settings backend.

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

Every Matter device must use a unique device identifier for rotating device identifier calculation purpose.
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

.. _ug_matter_persistent_storage:

Persistent storage
==================

The persistent storage module allows for the application data and configuration to survive a device reboot.
|NCS| Matter applications use one generic Persistent Storage API that can be enabled by the :kconfig:option:`CONFIG_NCS_SAMPLE_MATTER_PERSISTENT_STORAGE` Kconfig option.
This API consists of methods with ``Secure`` and ``NonSecure`` prefixes, which handle secure (ARM Platform Security Architecture Persistent Storage) and non-secure (raw Zephyr settings) storage operations, respectively.

You can learn more details about the Persistent Storage API from the :file:`ncs/nrf/samples/matter/common/src/persistent_storage/persistent_storage.h` header file.

The interface is implemented by two available backends.
Both can be used simultaneously by controlling the following Kconfig options:

* :kconfig:option:`CONFIG_NCS_SAMPLE_MATTER_SETTINGS_STORAGE_BACKEND` - Activates the implementation that takes advantage of the raw :ref:`Zephyr settings<zephyr:settings_api>`.
  This backend implements ``NonSecure`` methods of the Persistent Storage API and returns ``PSErrorCode::NotSupported`` for ``Secure`` methods.
* :kconfig:option:`CONFIG_NCS_SAMPLE_MATTER_SECURE_STORAGE_BACKEND` - Activates the module based on the ARM PSA Protected Storage API implementation from the :ref:`trusted_storage_readme` |NCS| library.
  This backend implements ``Secure`` methods of the Persistent Storage API and returns ``PSErrorCode::NotSupported`` for ``NonSecure`` methods.

Both backends allow you to control the maximum length of a string-type key under which an asset can be stored.
You can do this using the :kconfig:option:`CONFIG_NCS_SAMPLE_MATTER_STORAGE_MAX_KEY_LEN` Kconfig option.

If both backends are activated at the same time (:kconfig:option:`CONFIG_NCS_SAMPLE_MATTER_SETTINGS_STORAGE_BACKEND` and :kconfig:option:`CONFIG_NCS_SAMPLE_MATTER_SECURE_STORAGE_BACKEND` enabled) all methods of the generic interface are supported.

Similarly to the non-secure backend, the secure backend leverages the Zephyr Settings to interface with the FLASH memory.

Additionally, in case of the secure storage backend, the following Kconfig options control the storage limits:

* :kconfig:option:`CONFIG_NCS_SAMPLE_MATTER_SECURE_STORAGE_MAX_ENTRY_NUMBER` - Defines the maximum number or assets that can be stored in the secure storage.
* :kconfig:option:`CONFIG_TRUSTED_STORAGE_BACKEND_AEAD_MAX_DATA_SIZE` - Defines the maximum length of the secret that is stored.

.. _ug_matter_configuration_diagnostic_logs:

Diagnostic logs
===============

The diagnostic logs module implements all functionalities needed for the Matter controller to obtain logs.
The controller obtains the logs from the connected Matter devices according to the chosen intent.

To use the diagnostic logs module, add the ``Diagnostic Logs`` cluster as ``server`` in the ZAP configuration.
To learn how to add a new cluster to the ZAP configuration, see the :ref:`ug_matter_gs_adding_cluster` page.

To enable diagnostic log support, you must use the :ref:`diagnostic logs snippet <ug_matter_diagnostic_logs_snippet>`, which contains required devicetree overlays.

Currently, the following intents are defined within the ``IntentEnum`` enumerator in the Matter stack:

  * ``kEndUserSupport`` - Logs created by the product maker to be used for end-user support.
  * ``kNetworkDiag``- Logs created by the network layer to be used for network diagnostic.
  * ``kCrashLogs`` - Logs created during a device crash, to be obtained from the node.

Crash logs
----------

The crash logs module is a part of the diagnostic log module and contains the data of the most recent device crash.
When a crash occurs, the device saves the crash data to the defined retained RAM partition.
Because it uses the retained RAM partition, the :ref:`ug_matter_diagnostic_logs_snippet` must be added to the build to enable crash log support.

Only the latest crash data will be stored in the device's memory, meaning that if another crash occurs, the previous data will be overwritten.
After receiving the read request from the Matter controller, the device reads the crash data and creates human readable logs at runtime.
The device sends converted logs to the Matter controller as a response.

After the crash data is successfully read, it will be removed and further read attempts will notify the user that there is no available data to read.
To keep the crash log in the memory after reading it, set the :kconfig:option:`CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_REMOVE_CRASH_AFTER_READ` Kconfig option to ``n``.

Network and end user logs
-------------------------

The diagnostic network and end user logs are saved in the dedicated retained RAM partitions.
The logs are not removed after reading, but when attempting to write new logs to an already full buffer, the oldest logs are replaced.

The diagnostic network and end user logs are designed to be pushed when requested by the user.
This can result in the same information being passed by multiple APIs, which is usually not desirable behavior.
Because of this, for the network and the end user logs the :kconfig:option:`CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_REDIRECT` Kconfig option is enabled by default.

With the Kconfig option enabled, the redirect functionality takes logs passed to the Zephyr logger and saves them in the retained RAM as Matter diagnostic logs.
Only the following logs are redirected:

* Logs from the ``chip`` module are redirected into diagnostic network logs.
* Logs from the ``app`` module are redirected into diagnostic end user logs.

You can disable the redirect functionality by disabling the :kconfig:option:`CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_REDIRECT` Kconfig option.
You can then push the network or end user logs using dedicated API in your application, like in the following code snippet:

.. code-block:: C++

   #include "diagnostic_logs_provider.h"

   Nrf::Matter::DiagnosticLogProvider::GetInstance().PushLog(chip::app::Clusters::DiagnosticLogs::IntentEnum::kNetworkDiag, "Example network log", sizeof("Example network log"));
   Nrf::Matter::DiagnosticLogProvider::GetInstance().PushLog(chip::app::Clusters::DiagnosticLogs::IntentEnum::kEndUserSupport, "Example end user log", sizeof("Example end user log"));

.. _ug_matter_diagnostic_logs_snippet:

Diagnostic logs snippet
-----------------------

The diagnostic logs snippet enables the set of configurations needed for full Matter diagnostic logs support.
The configuration set consist of devicetree overlays for each supported target board, and a config file that enables all diagnostic logs features by default.
The devicetree overlays add new RAM partitions which are configured as retained to keep the log data persistent and survive the device reboot.
They also reduce the SRAM size according to the size of the retained partition.
The partition sizes are configured using example values and may not be sufficient for all use cases.
To change the partition sizes, you need to change the configuration in the devicetree overlay.
You can, for example, increase the partition sizes to be able to store more logs.

The snippet sets the following kconfig options:

  * :kconfig:option:`CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS` to ``y``.
  * :kconfig:option:`CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_CRASH_LOGS` to ``y``.
  * :kconfig:option:`CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_REMOVE_CRASH_AFTER_READ` to ``y``.
  * :kconfig:option:`CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_END_USER_LOGS` to ``y``.
  * :kconfig:option:`CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_NETWORK_LOGS` to ``y``.
  * :kconfig:option:`CONFIG_LOG_MODE_DEFERRED` to ``y``.
  * :kconfig:option:`CONFIG_LOG_RUNTIME_FILTERING` to ``n``.

Deferred logs mode (:kconfig:option:`CONFIG_LOG_MODE_DEFERRED`) is enabled because it is required by the log redirection functionality (:kconfig:option:`CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_REDIRECT`), which is enabled by default for diagnostic network and end user logs.

.. note::

  You cannot set the :kconfig:option:`CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS` Kconfig option separately without adding the devicetree overlays contained in the snippet.
  Instead, if you want to use just some of the diagnostic logs functionality, use the snippet and set the Kconfig options for the other functionalities to ``n``.

To use the snippet when building a sample, add ``-D<project_name>_SNIPPET=diagnostic-logs`` to the west arguments list.

Example for the ``nrf52840dk_nrf52840`` target board and the :ref:`matter_lock_sample` sample:

.. parsed-literal::
   :class: highlight

   west build -b nrf52840dk/nrf52840 -- -Dlock_SNIPPET=diagnostic-logs
