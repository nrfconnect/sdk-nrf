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

You can configure the Matter protocol to use NFC tag for :doc:`commissioning <matter:nrfconnect_android_commissioning>`, instead of the default QR code.

To enable NFC for commissioning and share the onboarding payload in an NFC tag, set the :kconfig:option:`CONFIG_CHIP_NFC_COMMISSIONING` Kconfig option.

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

Amazon FFS support
==================

Matter in the |NCS| supports `Amazon Frustration-Free Setup (FFS)`_ that allows Matter devices to be automatically commissioned to the Matter network using the Matter-enabled Amazon Echo device.
To enable the FFS support, set the following configuration options to meet the Amazon FFS setup prerequisites:

* :kconfig:option:`CONFIG_CHIP_COMMISSIONABLE_DEVICE_TYPE` to ``y``.
* :kconfig:option:`CONFIG_CHIP_ROTATING_DEVICE_ID` to ``y``.
* :kconfig:option:`CONFIG_CHIP_DEVICE_TYPE` to the appropriate value, depending on the device used.
  The value must be compliant with the Matter Device Type Identifier.
