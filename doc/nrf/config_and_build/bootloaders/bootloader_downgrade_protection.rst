.. _ug_fw_update_image_versions_mcuboot_downgrade:
.. _ug_fw_update_downgrade_protection:

Downgrade protection
####################

.. contents::
   :local:
   :depth: 2

The downgrade protection feature blocks downgrading the firmware version on the device.
This can be useful for security reasons, because firmware downgrade can allow attackers to bring back security vulnerabilities that were fixed by a particular firmware update.

As part of this process, the bootloader compares the new firmware image version with the version of the currently booted firmware.
The bootloader rejects the update if the new image version is lower than the version of the currently booted firmware.

The implementation is related to the :ref:`ug_fw_update_image_versions` and depends on the type of bootloader used:

* Software-based downgrade protection is supported only by MCUboot.
  In this kind of protection, the current firmware version is encoded into the currently active image.
* Hardware-based downgrade protection is supported by both MCUboot and |NSIB|.
  In this kind of protection, the current version is encoded into a non-volatile storage partition outside of any image.

.. _ug_fw_update_downgrade_protection_sw:

Software-based downgrade protection
***********************************

The |NCS| supports MCUboot's software-based downgrade prevention for application images, using semantic versioning.
This feature offers protection against any outdated firmware that is uploaded to a device.

To enable this feature, set the MCUboot-specific configuration options ``CONFIG_MCUBOOT_DOWNGRADE_PREVENTION`` and ``CONFIG_BOOT_UPGRADE_ONLY`` for the MCUboot image.

.. caution::
   Enabling ``CONFIG_BOOT_UPGRADE_ONLY`` prevents the fallback recovery of application images.
   Consult its Kconfig description and the :doc:`MCUboot Design documentation <mcuboot:design>` for more information on how to use it.

You can compile your application with this feature as follows:

.. parsed-literal::
   :class: highlight

   west build -b *board* *application* -- \\
   -DCONFIG_BOOTLOADER_MCUBOOT=y \\
   -DCONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION=\\"0.1.2\\+3\\" \\
   -Dmcuboot_CONFIG_MCUBOOT_DOWNGRADE_PREVENTION=y \\
   -Dmcuboot_CONFIG_BOOT_UPGRADE_ONLY=y

|how_to_configure|

After you upload a new image and reset the development kit, MCUboot attempts to boot the secondary image.
If this image has, in order of precedence, a *major*, *minor*, or *revision* value that is lower than the primary application image, it is considered invalid and the existing primary application boots instead.

.. note::
   The optional label or build number specified after the ``+`` character is ignored when evaluating the version.
   For example, an existing application image with version ``0.1.2+3`` can be overwritten by an uploaded image with ``0.1.2+2``, but not by one with ``0.1.1+2``.

.. _ug_fw_update_downgrade_protection_hw:
.. _bootloader_monotonic_counter:

Hardware-based downgrade protection
***********************************

.. bootloader_monotonic_counter_start

You can implement hardware-based downgrade protection using a non-volatile monotonic counter.

Counter updates are written to slots in the *Provision* area, with each new counter update occupying a new slot.
For this reason, the number of counter updates, and therefore firmware version updates, is limited.
The *Provision* is a partition in non-volatile memory, and its location can be found using :ref:`pm_generated_output_and_usage_pm_report`.

Using a counter is optional and can be configured for the application using configuration options.
You can also configure the supported number of updates, but the number is limited by the size of the *Provision* area and how much of that area is taken up by other features, like public key hashes.
In addition, you can configure what firmware version of the image you want to boot.

.. bootloader_monotonic_counter_end

.. _ug_fw_update_hw_downgrade_nsib:

Downgrade protection using |NSIB|
=================================

.. bootloader_monotonic_counter_nsib_start

To enable anti-rollback protection with monotonic counter for |NSIB|, set the following configurations in the application (parent image):

* :kconfig:option:`CONFIG_SB_MONOTONIC_COUNTER`
* :kconfig:option:`CONFIG_SB_NUM_VER_COUNTER_SLOTS`

Special handling is needed when updating the S1 variant of an image when :ref:`ug_bootloader_adding_upgradable`.
See :ref:`ug_bootloader_adding_presigned_variants` for details.

.. bootloader_monotonic_counter_nsib_end

To set options for child images, see the :ref:`ug_multi_image_variables` section.

.. _ug_fw_update_hw_downgrade_mcuboot:

Downgrade protection using MCUboot
==================================

To enable anti-rollback protection with monotonic counter for MCUboot, set the following configurations in the application (parent image):

* :kconfig:option:`CONFIG_MCUBOOT_HARDWARE_DOWNGRADE_PREVENTION`
* :kconfig:option:`CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION_COUNTER_SLOTS`
* :kconfig:option:`CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION_COUNTER_VALUE`

To set options for child images, see the :ref:`ug_multi_image_variables` section.
