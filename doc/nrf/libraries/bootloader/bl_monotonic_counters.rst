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

The monotonic counters are stored as part of the bootloader storage :ref:`doc_bl_storage`.
Each monotonic counter consists of a series of uint16_t slots where each counter update is written to the next available slot.

You can find tests for the library at :file:`tests/subsys/bootloader/bl_monotonic_counters/`.

Requirements
############

To use this library the counters must first be provisioned. See :ref:`bootloader_provisioning` for more information about the provisioned data and how the bootloader uses it.

Configuration
#############

This library is enabled by default when :kconfig:option:`CONFIG_SECURE_BOOT` is enabled.
You can disable it through :kconfig:option:`CONFIG_SB_MONOTONIC_COUNTER`.
The number of avaiable counter slots is configurable through :kconfig:option:`CONFIG_SB_NUM_VER_COUNTER_SLOTS`.

Samples using this library
##########################

The library is used by :ref:`bootloader` to keep track of the version of the application.
During the update the :ref:`doc_bl_validation` library checks if the new version of the validated image is newer than the current version and only accepts the new image if it is.
After the update, :ref:`bootloader` stores the new version of image in the bootloader stoage.


API documentation
*****************

| Header file: :file:`include/bl_monotonic_counters.h`
| Source files: :file:`subsys/bootloader/bl_monotonic_counters/`

.. doxygengroup:: bl_monotonic_counters
   :project: nrf
   :members:
