.. _matter_stack_config:

Shared configurations in Matter stack
#####################################

.. contents::
   :local:
   :depth: 2

This page lists Kconfig options and snippets shared by ``sdk-connectedhomeip``.
See the :ref:`ug_matter_device_advanced_kconfigs` page for more detailed information.

Configuration options
*********************

Check and configure the following configuration options:

.. _CONFIG_CHIP_LAST_FABRIC_REMOVED_ACTION_DELAY:

CONFIG_CHIP_LAST_FABRIC_REMOVED_ACTION_DELAY
  ``int`` - After removing the last fabric wait defined time [in milliseconds] to perform an action.
  After removing the last fabric the device will wait for the defined time and then perform an action chosen by the ``CHIP_LAST_FABRIC_REMOVED_ACTION`` option.
  This schedule will allow for avoiding race conditions before the device removes non-volatile data.

.. _CONFIG_FACTORY_DATA_CUSTOM_BACKEND:

CONFIG_FACTORY_DATA_CUSTOM_BACKEND
  ``bool`` - Enable a custom backend for factory data.
  This option is used to enable a custom backend for factory data.
  The custom backend is used to store the factory data in a custom location.

.. _CONFIG_CHIP_LAST_FABRIC_REMOVED_ERASE_AND_PAIRING_START:

CONFIG_CHIP_LAST_FABRIC_REMOVED_ERASE_AND_PAIRING_START
  ``bool`` - After removing the last fabric erase NVS and start Bluetooth LE advertising.
  After removing the last fabric the device will perform the factory reset without rebooting and start the Bluetooth LE advertisement automatically.
  The current RAM state will be saved and the new commissioning to the next fabric will use the next possible fabric index.
  This option should not be used for devices that normally do not advertise Bluetooth LE on boot to keep their original behavior.

.. _CONFIG_CHIP_LAST_FABRIC_REMOVED_ERASE_AND_REBOOT:

CONFIG_CHIP_LAST_FABRIC_REMOVED_ERASE_AND_REBOOT
  ``bool`` - After removing the last fabric erase NVS and reboot.
  After removing the last fabric the device will perform the factory reset and then reboot.
  The current RAM state will be removed and the new commissioning to the new fabric will use the initial fabric index.
  This option is the safest.

.. _CONFIG_CHIP_LAST_FABRIC_REMOVED_ERASE_ONLY:

CONFIG_CHIP_LAST_FABRIC_REMOVED_ERASE_ONLY
  ``bool`` - After removing the last fabric erase NVS only.
  After removing the last fabric the device will perform the factory reset only without rebooting.
  The current RAM state will be saved and the new commissioning to the next fabric will use the next possible fabric index.

.. _CONFIG_CHIP_LAST_FABRIC_REMOVED_NONE:

CONFIG_CHIP_LAST_FABRIC_REMOVED_NONE
  ``bool`` - After removing the last fabric do not perform any action.
  After removing the last fabric the device will not perform factory reset or reboot.
  The current state will be left as it is and the Bluetooth LE advertising will not start automatically.

.. _CONFIG_CHIP_MEMORY_PROFILING:

CONFIG_CHIP_MEMORY_PROFILING
  ``bool`` - Enable features for tracking memory usage.
  Enables features for tracking memory usage in Matter.

.. _CONFIG_CHIP_NUS:

CONFIG_CHIP_NUS
  ``bool`` - Enable Nordic UART service for Matter purposes.
  Enables Nordic UART service (NUS) for Matter samples.
  Using the NUS service, you can control a Matter sample using pre-defined Bluetooth LE commands and perform defined operations.
  The CHIP NUS service can be useful to keep communication with a smart home device when a connection within Matter network is lost.

.. _CONFIG_CHIP_FACTORY_DATA_ROTATING_DEVICE_UID_MAX_LEN:

CONFIG_CHIP_FACTORY_DATA_ROTATING_DEVICE_UID_MAX_LEN
	``int`` - Maximum length of rotating device ID unique ID in bytes
	Maximum acceptable length of rotating device ID unique ID in bytes.
