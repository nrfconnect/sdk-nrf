.. _ug_fw_update_image_versions:

Image versions
##############

.. contents::
   :local:
   :depth: 2

Each release of an application must come with a specific version.
The version can have the form of a single number.
Alternatively, it can be based on a more advanced versioning scheme.
For example, the semantic versioning scheme uses three numbers to denote major, minor, and patch versions, respectively.

The choice of the versioning scheme depends on the :ref:`selected bootloader <ug_bootloader_adding>` and the configuration of the bootloader, including :ref:`ug_fw_update_downgrade_protection`.
The |NCS| :ref:`build system <app_build_system>` can automatically handle building of the bootloader together with the application.
The build system can also sign the application images and provide versioning information for the images.

See the following sections for details related to application versioning for bootloaders supported in the |NCS|.

.. _ug_fw_update_image_versions_b0:

Configuring image version with |NSIB|
*************************************

You can set the image version for your application using the :kconfig:option:`CONFIG_FW_INFO_FIRMWARE_VERSION` Kconfig option.
This option can refer to two different things:

* If NSIB directly boots your application image, the Kconfig option denotes the application image version.
* If NSIB boots a secondary-stage bootloader, the Kconfig option denotes the version of the secondary-stage bootloader.
  In such case, the application is booted by the secondary-state bootloader and the application image version is configured using the versioning scheme of the secondary-stage bootloader.
  For example, if you opted for :ref:`ug_bootloader_adding_upgradable_mcuboot`, the application image versioning configuration is described in :ref:`ug_fw_update_image_versions_mcuboot`.

.. _ug_fw_update_image_versions_mcuboot:

Configuring image version with MCUboot
**************************************

To assign a semantic version number to your application when you have opted for booting application directly by the MCUboot bootloader (that is, if you have opted for either :ref:`ug_bootloader_adding_immutable_mcuboot` or :ref:`ug_bootloader_adding_upgradable_mcuboot`), it is recommended to use the :file:`VERSION` file that contains the version information for the application image.
Using a :file:`VERSION` file allows you to independently configure the version value for each build instance of the application.
See Zephyr's :ref:`zephyr:app-version-details` page for more information.

Alternatively, you can also add the version string to the :kconfig:option:`CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION` Kconfig option for the application:

.. code-block:: console

   CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION="0.1.2+3"

See the `Semantic versioning`_ webpage or :doc:`mcuboot:imgtool` for details on version syntax.

Read the :ref:`app_build_mcuboot_output` page for the list of all the FOTA upgrade files that are automatically generated when using MCUboot.
