.. _ug_matter_configuring_protocol:

Configuring Matter in |NCS|
###########################

.. contents::
   :local:
   :depth: 2

This page describes what is needed to start working with Matter in the |NCS|.

.. _ug_matter_configuring_mandatory:

Mandatory configuration
***********************

To use the Matter protocol, set the following Kconfig options:

* :kconfig:option:`CONFIG_CHIP` - This option enables the Matter protocol stack and other associated Kconfig options, including :kconfig:option:`CONFIG_CHIP_ENABLE_DNSSD_SRP` that is required for the discovery of the Matter device using DNS-SD.
* :kconfig:option:`CONFIG_CHIP_PROJECT_CONFIG` - This option defines the path to the configuration file that specifies Vendor ID, Product ID, and other project-specific Matter settings.

Because Matter is an application layer protocol on top of the other IPv6-based transport protocols (see :ref:`ug_matter_architecture`), it uses multiple software modules with their own configuration options to provide the communication between the devices and the necessary functionalities.
It uses modules such as Bluetooth® LE, the IPv6 stack (currently, only :ref:`Thread <ug_thread_configuring>` is supported), :ref:`nRF Security <nrfxlib:nrf_security_config>`, or :ref:`MCUboot <mcuboot:mcuboot_ncs>`.
Make sure to review the configuration options of these modules when configuring Matter.

The Kconfig options for Matter applications in the nRF Connect SDK are stored in the following files:

* :file:`prj.conf` files, which are specific to the application.
* :file:`Kconfig.defaults` file, which is available in the :file:`module/lib/matter/config/nrfconnect/chip-module` directory and is used to populate :file:`prj.conf` with Kconfig option settings common to all samples.

The Matter samples use the same structure for other software images, such as MCUboot or Multiprotocol RPMsg, which have default options defined in the corresponding :file:`Kconfig.mcuboot.defaults` and :file:`Kconfig.multiprotocol_rpmsg.defaults` files.

For an example configuration, see the :ref:`Matter Template sample's <matter_template_sample>` :file:`prj.conf` files in the sample root directory.
For instructions about how to set Kconfig options, see :ref:`configure_application`.

.. _ug_matter_configuring_optional:

Optional configuration
**********************

After enabling the Matter protocol and defining the path to the Matter configuration file, you can enable additional options in Kconfig.

.. _ug_matter_configuring_optional_ot:

OpenThread configuration
========================

Enabling :kconfig:option:`CONFIG_CHIP` automatically enables the following options related to OpenThread:

* :kconfig:option:`CONFIG_OPENTHREAD_ECDSA` and :kconfig:option:`CONFIG_OPENTHREAD_SRP_CLIENT` - enabled through :kconfig:option:`CONFIG_CHIP_ENABLE_DNSSD_SRP`
* :kconfig:option:`CONFIG_OPENTHREAD_DNS_CLIENT` - enabled through :kconfig:option:`CONFIG_CHIP_ENABLE_DNS_CLIENT`

Additionally, you can enable the support for Thread :ref:`Sleepy End Device <thread_ot_device_types>` in Matter by using the :kconfig:option:`CONFIG_CHIP_ENABLE_SLEEPY_END_DEVICE_SUPPORT` Kconfig option.
This option sets the :kconfig:option:`CONFIG_OPENTHREAD_MTD_SED` Kconfig option.

For more information about configuring OpenThread in the |NCS|, see :ref:`ug_thread_configuring`.

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

* :kconfig:option:`CONFIG_MATTER_LOG_LEVEL_ERR`
* :kconfig:option:`CONFIG_MATTER_LOG_LEVEL_INFO`
* :kconfig:option:`CONFIG_MATTER_LOG_LEVEL_DBG`

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

.. _ug_matter_configuring_requirements:

Required components for Matter network
**************************************

The Matter protocol is centered around the Matter network, which requires the following components to operate properly:

* Matter controller - either CHIP Tool for Linux or macOS, or CHIP Tool for Android
* Thread Border Router - configured either on PC or Raspberry Pi

For information about how to configure these components, read :ref:`ug_matter_configuring_controller` and :ref:`ug_matter_configuring_env`.
