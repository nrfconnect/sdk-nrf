.. _doc_bl_monotonic_counters:

Monotonic counters
##################

.. contents::
   :local:
   :depth: 2


Overview
########

The monotonic counters library implements uint16_t counters that can only increase in value.
This library is used by bootloaders to provide rollback protection.

Implementation
##############

The monotonic counters are stored as part of the bootloader storage :ref:`doc_bl_storage` when :ref:`bootloader` is enabled.
When :ref:`MCUboot <mcuboot:mcuboot_ncs>` is the only bootloader in the build the monotonic counters are stored in the OTP region of the device.
Each monotonic counter consists of a series of uint16_t slots where each counter update is written to the next available slot.

You can find tests for the library at :file:`tests/subsys/bootloader/bl_monotonic_counters/`.

Requirements
############

To use this library the counters must first be provisioned. See :ref:`bootloader_provisioning` for more information about the provisioned data and how the bootloader uses it.

Configuration
#############

:ref:`bootloader` configuration

This library is enabled by default when :kconfig:option:`CONFIG_SECURE_BOOT` is enabled.
You can disable it through :kconfig:option:`CONFIG_SB_MONOTONIC_COUNTER`.
The number of avaiable counter slots is configurable through :kconfig:option:`CONFIG_SB_NUM_VER_COUNTER_SLOTS`.

:ref:`MCUboot <mcuboot:mcuboot_ncs>` configuration

The option is disabled by default and it can be enabled by setting :kconfig:option:`CONFIG_MCUBOOT_HARDWARE_DOWNGRADE_PREVENTION`.
The number of available slots for :ref:`MCUboot <mcuboot:mcuboot_ncs>` is configurable through :kconfig:option:`MCUBOOT_HW_DOWNGRADE_PREVENTION_COUNTER_SLOTS`.
The version of the validated image by :ref:`MCUboot <mcuboot:mcuboot_ncs>` is configurable through :kconfig:option:`MCUBOOT_HW_DOWNGRADE_PREVENTION_COUNTER_VALUE`.

Samples using this library
##########################

The library is used by :ref:`bootloader` to keep track of the version of the application.
During the update the :ref:`doc_bl_validation` library checks if the new version of the validated image is newer than the current version and only accepts the new image if it is.
After the update, :ref:`bootloader` stores the new version of image in the bootloader stoage.

Additional information
######################

This library is also used by :ref:`MCUboot <mcuboot:mcuboot_ncs>` bootloader when the hardware downgrade prevention option is enabled.
When both bootloaders :ref:`bootloader` and :ref:`MCUboot <mcuboot:mcuboot_ncs>` are enabled the separate monotonic counters will be generated for each of them.

:ref:`MCUboot <mcuboot:mcuboot_ncs>` is using one monotonic counter for each image it can update.
When :ref:`MCUboot <mcuboot:mcuboot_ncs>` and :ref:`bootloader` are both enabled, :ref:`MCUboot <mcuboot:mcuboot_ncs>` considers itself an updateable image.
A monotonic counter is not reserved for the :ref:`MCUboot <mcuboot:mcuboot_ncs>` image since it's version is verified by :ref:`bootloader`.

API documentation
*****************

| Header file: :file:`include/bl_monotonic_counters.h`
| Source files: :file:`subsys/bootloader/bl_monotonic_counters/`

.. doxygengroup:: bl_monotonic_counters
   :project: nrf
   :members:
